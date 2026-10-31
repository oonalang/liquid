# Triggerbot r26 — low cover, firing delay, per-frame cost

Baseline: repository `main` (`d5460f7`). Only the triggerbot files and their tests changed.

## Reported symptoms

1. With low cover right in front of the player (the obstacle covers the feet) the triggerbot stops shooting even though the enemy is visible.
2. A noticeable delay before the shot, which the user does not want.
3. FPS degrades the longer the triggerbot is used.

## Cause 1 — low cover blocked every candidate ray

r21 changed the gate to `muzzle path clear AND camera path clear`. Every candidate sample
had to be visible from *both* the firing origin and the camera. That is wrong for 1.6.57:

- The firing origin (`Pawn.GetFirePosition`, RVA `0x51B8900`) is the weapon muzzle. Standing
  flush against low cover puts the muzzle below/inside the obstacle, so the muzzle trace
  reports a hit while the player is clearly looking at - and hitting - the enemy.
- A trace that *starts* on or inside a collider returns a hit at ~0 distance. The old code
  treated that as a wall on the very first cast, so every origin reported "blocked".
- The self-collider skip budget (2 skips / 2.5 units) also ran out inside the player's own
  body, arms and weapon model, which made muzzle traces fail even in the open.

Fixes in `Features/Triggerbot1657.h` and `Features/Triggerbot1657Policy.h`:

- One clear origin is now enough. Origins are tried in order (muzzle, eye, head, camera) and
  the first clear path wins. The game itself spawns its bullet ray at the camera
  (`WeaponFireComponent.CreatBulletLine` uses the main camera transform), so a clear camera
  path is what the game will actually hit. Solid cover blocks all origins, so this does not
  shoot through walls; smoke still rejects a candidate point.
- A zero-distance hit on the first cast is treated as an embedded start and stepped out of
  (bounded: 2 steps of 0.25 units), instead of killing the line of sight.
- The self-hit budget is raised to 6 skips / 5.5 units, enough to pass the whole view model,
  while remaining bounded by the existing 24-cast per-evaluation cap.

## Cause 2 — firing delay

- `PlayerController.StopFire(bool isImmidiately)` was always called with `false`, which runs
  the gradual weapon-lower transition. The next `StartFire` is not accepted until the game
  finishes that transition, so every release inserted a re-arm delay. All releases now pass
  `true` (immediate).
- An unclassified weapon (`FireMode::Unknown`, e.g. before the magazine-based classification
  finished, or a weapon whose clip cannot be read) was handled as a tap: hold `tapHoldMs`
  (60 ms), release, press again, over and over. Each cycle released and re-pressed the game's
  fire state machine. Automatic weapons and weapons that are still being classified now stay
  held; only a verified single-fire weapon taps. If a weapon never classifies, tapping is
  kept as a fallback so it still shoots.
- The fire-mode probe no longer requires releasing the trigger to retry: a retry starts on the
  held trigger.

## Cause 3 — per-frame cost

- A clear target now costs **one** physics trace instead of two (the r21 AND-gate always ran
  the muzzle trace plus the camera trace). Verified by assertion in the native test.
- Verified auto/semi-auto classification removes the release/press churn: previously up to
  ~16 release+press cycles per second, each running the game's complete fire pipeline (fire
  request/RPC, muzzle effects, audio). That per-press work is what piles up during a long
  session; the trigger now holds.
- No triggerbot-owned state grows over time: all lifecycle state is a fixed set of scalars and
  the diagnostics are atomics. If FPS still decays over a match, the remaining candidates are
  the game's own effect/particle pools or thermal throttling, and need `adb logcat` / a
  device profile rather than another guess.

## Tests

Host tests (`g++ -std=c++17 -Wall -Wextra`), all passing:

- `tests/test_triggerbot1657_native.cpp` — updated to the new contract and extended:
  - muzzle blocked + camera clear fires (the reported low-cover case), muzzle+eye+head blocked
    + camera clear fires, all four blocked refuses;
  - a clear path uses exactly one trace;
  - an embedded (zero-distance) hit is stepped out of, a persistent one still refuses;
  - unclassified weapons hold instead of tapping, verified single-fire weapons still tap;
  - probe retry happens on the held trigger; probe budget still bounds retries.
- `tests/test_triggerbot1657.cpp` — new self-skip/embedded-hit bounds.
- `src/main/jni/tests/test_triggerbot_strict_zone.cpp`, `tests/test_triggerbot_hitboxes.cpp`,
  `tests/test_triggerbot1657_runtime.cpp`, `tests/test_enemy_list_snapshot1657.cpp`,
  `tests/test_linecast1657_abi.cpp`, `tests/test_dot_geometry.cpp`,
  `tests/test_triggerbot_header_compile.cpp` — pass.

## Not verified

- No Android NDK build and no on-device match in this environment. Native ARM64 compilation,
  the live collider layout, real weapon behaviour and the actual FPS delta are unverified.
- The camera-authoritative decision assumes the 1.6.57 shot ray originates at the camera (as
  the existing silent-aim hook in `Hacks/Feature/DrawESP.h` already does). If the game
  validates shots from the muzzle instead, a blocked muzzle may still produce a blocked shot
  in game even though the trigger now fires.
- `Tools::IsPtrValid` (one `write()` to `/dev/random` per uncached pointer) was left unchanged;
  it is still the per-frame cost floor of the eligibility path.
