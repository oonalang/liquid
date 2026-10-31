# Triggerbot r27 — instant semi-auto taps and ADS firing delay

Baseline: repository `main` (`ec89b54`, r26). Only the triggerbot headers, their native test
and this report changed.

## Reported symptoms

1. ADS Only: a long delay before the bot shoots.
2. A semi-automatic assault rifle is handled as if it were automatic: the fire button is held
   instead of tapped.
3. In ADS Only and Both, the bot waits / "processes" before shooting an enemy in front, so the
   shooting stops and resumes late.

## Cause 1 — fire mode was only guessed from the magazine

`FireMode` was never read from the weapon. It started as `Unknown` and stayed there until a
magazine probe finished: press, wait 200 ms for a mid sample, wait 350 ms for a final sample,
and only then decide. While that ran the trigger was held.

- On a semi-automatic weapon that is exactly the wrong input: the game accepts one round per
  press, so holding the button fires the first round and nothing after it. The bot looks like
  it "holds the fire button" and only taps once the probe finally ends — the long delay.
- If the clip count could not be read, no probe could ever succeed, `probeFailures` never
  advanced, and the weapon stayed unclassified and held forever: no second shot at all.

Fixes in `Features/Triggerbot1657.h`:

- Fire mode now comes from the game's own `Weapon` flags, resolved at `Initialize`:
  - `Weapon.IsSingleFire()` — RVA `0x50EC158`, dump `public bool IsSingleFire()` (line 844002)
  - `Weapon.get_CanAutoFire()` — RVA `0x512A0E8`, dump `public bool CanAutoFire { get; }`
  - `Weapon.get_IsSingleShot()` — RVA `0x50EBA5C`, dump `public bool IsSingleShot { get; }`
  These are non-virtual instance getters on the same class as the already-used
  `Weapon.get_ClipAmmoCount` (`0x50EC614`), so they are called the same way
  (`(weapon, nullptr)`).
- `IdentifyFireMode` maps them to `Single` / `Auto` / `Unknown`. A weapon that reports
  single-fire or single-shot, or that explicitly cannot auto-fire, taps; only an explicit
  auto-fire capability holds. Anything the flags cannot answer stays `Unknown` for the probe.
- The metadata is read on the first accepted press, when the weapon is fully active (the
  weapon-change path only resets state and spends the probe budget when no clip can be read).
  A semi-auto therefore taps from its very first press, with no probe and no hold.
- The magazine probe remains as a fallback for weapons whose metadata is unavailable, and is
  now strictly bounded: an unreadable clip spends the probe budget immediately so the tap
  fallback takes over instead of holding a trigger that never fires again.

## Cause 2 — a single-fire press was released after a fixed 60 ms

A single-fire press was released once `tapHoldMs` (60 ms) had elapsed, regardless of whether
the game had delivered the round yet. The game schedules a shot after the press, and in ADS the
aim-to-fire interval delays it. Releasing first cancelled that pending shot; the bot then
re-pressed, waited, released again, and the shot only landed when the timing happened to line
up — the reported ADS delay / stop.

Fix: for `Single` the press is now held until its round is consumed:

- release as soon as the clip count drops below the value sampled at the press (the shot has
  left the weapon),
- or when the weapon reports it can fire again after the minimum hold (the press did nothing),
- or when `kMaxTapHoldMs` (400 ms, `Triggerbot1657Policy.h`) is reached, so a weapon that never
  answers can never leave the trigger stuck down.

With an unreadable clip the old minimum hold is kept as the fallback because there is no
evidence to wait for.

## ADS Only

ADS Only still requires `Pawn.IsAiming()`, so the bot does not fire before the game considers
the player scoped. What changed is that once the press is sent the shot is no longer thrown
away by the 60 ms release, and a semi-automatic weapon is tapped from the first press instead
of being held through a magazine probe. No ADS timing constant or mode gate was weakened.

## Files

- `src/main/jni/Features/Triggerbot1657.h` — weapon fire-mode getters, `IdentifyFireMode`,
  press classification, bounded probe fallback, ammo-confirmed single-fire release,
  `pressAmmoBefore` lifecycle.
- `src/main/jni/Features/Triggerbot1657Policy.h` — `kMaxTapHoldMs`.
- `src/main/jni/Features/Triggerbot1657Diagnostics.h` — build label r27.
- `tests/test_triggerbot1657_native.cpp` — metadata classification (semi taps on first press
  with no probe, auto holds with no probe, no-single-flag + no-auto-fire taps), unreadable
  clip taps instead of holding forever, and the ammo-confirmed delayed-shot release.

## Tests run

- `g++ -std=c++17 -Wall -Wextra` native simulation: PASS.
- `g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined` native simulation: PASS.
- Policy/geometry/hitbox/strict-zone/linecast/snapshot/runtime host tests: PASS.

## Not verified

- Android ARM64 NDK build and a live match were not run in this environment.
- The three new RVAs are taken from the same 1.6.57 metadata dump as the existing calls and
  were not executed against the game binary. If a weapon's flags disagree with its real fire
  behaviour, the magazine probe no longer corrects a wrong metadata answer.
- Whether the game actually cancels a pending single-fire shot on `StopFire` is inferred from
  the reported symptoms, not observed on a device; holding until the clip drops is safe either
  way but the exact ADS improvement is unmeasured.
