# context.md — Liquid (CODM 1.6.57 native mod) handoff

Written 2026-11-01. Read this before touching anything. It covers the project, the
human's constraints, what is proven vs. guessed about the crashes, every change made
so far, and how to verify work in an environment that has no device and no NDK.

---

## 1. What this project is

`Liquid` is a **native arm64 mod for Call of Duty: Mobile (CODM) 1.6.57**, compiled as a
shared library and loaded into the game process by a wrapper APK. Everything is C++ and
JNI:

- `src/main/jni/Main.cpp` — the entry point (`Init_Thread`, EGL/ImGui startup, ~1,800 lines,
  the mod's monolithic glue).
- `src/main/jni/Features/` — the newer, self-contained feature headers (`Triggerbot1657.h`,
  `Gameplay1657.h`, `EnemyListSnapshot1657.h`, `EnemyReadGuard1657.h`, diagnostics, hitboxes,
  linecast ABI, policy, runtime).
- `src/main/jni/Hacks/Feature/` — the older features (`DrawESP.h`, `Chams.h`, skin/effect systems,
  `ItemResource1657.h`).
- `src/main/jni/UI/` — ImGui menu, license UI.
- `src/main/jni/*/` — vendored libraries (ImGui, And64InlineHook, KittyMemory, curl, dobby,
  oxorany, Substrate, `IL2CppSDKGenerator` with `dump 1.6.57.cs` in the repo root).
- `tests/` — **host** C++ programs (compiled with g++ on a normal machine, no Android).
  `src/main/jni/tests/` holds one more (`test_triggerbot_strict_zone.cpp`).
- `scripts/termux-crash-capture.sh` — on-device crash/log capture helper for the human's phone.
- `*_REPORT.md` at the repo root — the running engineering log (r12 → r32). Read
  `TRIGGERBOT_R32_ROUND_CRASH_REPORT.md` first; it is the current one.

Build config: `src/main/jni/Application.mk` → `arm64-v8a`, `android-21`, `c++_static`,
release, PIE. Module `V2` in `Android-game.mk`.

---

## 2. The human's environment — this is the dominant constraint

- **No PC.** They build and test from the phone: **AIDE** for the APK/`.so`, **Termux** for
  shell/adb/git.
- Phone: **Realme RMX3363 (GT Master Edition)**, **Android 13**. The game runs inside the
  **Gspace** virtual container (`com.garena.game.codm` seen with uid ~10478 and renamed
  processes, e.g. `…:p0`).
- ADB is **wireless ADB to the phone itself** (Termux `pkg install android-tools`, `adb pair`,
  `adb connect 127.0.0.1:<port>`). No root assumed; the capture script falls back to adb.
- **No NDK, no device, no browser, no emulator is available to the AI.** Anything claiming an
  Android build or on-device behaviour "works" is fabrication. Only host tests are runnable.
- They push feedback in short, sometimes interrupted messages; they frequently interrupt a
  command because they think it is empty/stuck. Explain what a command is *waiting for*.

### Working agreements established with them

- Fix **only** what was reported. No new features, no refactors, no dependency changes.
- **Never add input latency** to the triggerbot in steady state. Any per-frame work added is
  a real cost and they notice it.
- Commit and **push to `main`** when a change is finished — they pull it and build in AIDE.
  Commit footer convention used in this repo:
  `🤖 Generated with Codebuff` + `Co-Authored-By: Codebuff <noreply@codebuff.com>`.
- They judge results by feel in match: "hitch", "crash", "flicker". Ask for *when* it happens
  (mode, action, round state) rather than dumping options at them.

---

## 3. Reported symptoms and current status

| # | Symptom | Status |
|---|---|---|
| 1 | Crash in ranked matches with the mod loaded; also reported as "crashes at the end of round 1 / start of round 2" | **Open.** Three code-level hazards removed (r30, r32, ESP guard). Not reproduced-and-fixed on device; one real SIGSEGV captured and analysed (see §4). |
| 2 | Game stutters/hitches for a moment every time the player scopes in, **only** with triggerbot mode `ADS Only`, not `Both` | **Fixed in code** (`d036ada`), pending on-device confirmation. See §6.1. |
| 3 | `adb logcat -s LIQUID:I` shows nothing even after 10 minutes with the game open | **Explained + tooling fixed** (`542f193`). See §7.2. |
| 4 | Crash capture produced a **0-byte** `.tar.gz` | **Fixed** (`542f193`), pack now builds in Termux storage and verifies the archive. |

---

## 4. Crash investigation: everything that is actually known

### 4.1 The first log dumps (`codm-crash-part01..03.txt`) are not the reported crash

269,175 lines, 20:27–22:19. Only three fatal signals, and none is a match/round crash:

| time | process | signal | note |
|---|---|---|---|
| 20:27:29 | `lelspace:lbcore` | SIGBUS | unrelated app |
| 22:16:07.630 | `com.garena.game.codm` | SIGSEGV | **launch-time**, 1.7 s after `CODM: onCreate start` |
| 22:16:15.206 | `com.garena.game.codm` | SIGSEGV | identical to the above |

Both game crashes: 5 frames, `libanort.so+0x1336e8 / +0x11b7e4 / +0x11d0f8 / +0x032d20` then
`libunity.so+0xe742170`, with `x2 = 0xffffffffffffffc0` (negative memcpy/loop length). That is
inside the game's ACE-side library at **load**, not a mod frame, not a round change. Still
unexplained and worth a separate look, but do not treat it as the reported crash.

### 4.2 The one real in-match capture (2026-11-02 04:56:54.808)

```
pid 9922, tid 11393, name UnityMain  >>> com.garena.game.codm <<<
signal 11 (SIGSEGV), code -6, fault addr 000028ee000026c2
pc 00000073e7780d9c    x30 00000073e7780d8c
#00 libunity.(:0)
#01 libunity.(:0)
#02 libsigchain._ZN3art11SignalChain7HandlerE…
```

**The important insight:** the fault address is not a heap address. It is

```
0x28ee000026c2 == (10478 << 32) | 9922
```

i.e. **the game's uid (10478 — the uid named in the Gspace `SecurityException` storm in the same
log) packed in front of the crashing pid (9922)**. A process-identity key was dereferenced as a
pointer. No fire path and no enemy scan in this mod produces that pattern; those fault at heap
addresses or at zero. Frames below `libunity` are missing, so the function cannot be named from
this dump alone.

**Consequence:** a tombstone or `pc - libunity_base` is required to go further. `Main.cpp` now
prints the base once per launch (§7.2). Do **not** invent a cause for this one; the fix list in
§5 is justified by its own separate reasoning (stale/destroyed objects), not by this signature.

### 4.3 The isolation experiment the human ran (this drove the ESP fix)

- ESP on, **boxes only** → crashed **every match at the round flip**.
- Triggerbot only → no crash in that window.
- These tests were still run inside Gspace, so Gspace's own identity exceptions were still in
  the log and are not excluded as a factor.

Interpretation: at a round flip the game destroys/rebuilds pawns. `Tools::IsPtrValid` only
proves a page is **mapped and writable** — a destroyed pawn passes it, and the very next bone
read is a use-after-free. The triggerbot had already been given guards (r30/r32); the ESP had
**nothing** between the enemy list and the bone read. That is the change in §6.2.

---

## 5. Changes landed (all on `main`, newest last)

```
542f193  Report whether the mod logged its base, and stop losing packs on shared storage
d036ada  Keep the triggerbot scan running in ADS Only mode instead of at scope-in
4cfdf75  Guard the ESP pawn reads the same way the triggerbot's are guarded
bfe3ea1  Guard the round-transition fire paths and stop guessing at crashes blind   (r32)
5a1a111  Add files via upload
699acf7  Guard triggerbot lifecycle gaps, reduce smoke work, and preserve slide init (r31)
744db7c  LIQUID r30: keep the trigger out of the fire path across a round change
```

### 5.1 r32 — round-transition fire paths (`Features/Triggerbot1657.h`, `Features/Gameplay1657.h`)

Problem: a player who **survives** into the next round keeps the same pawn, so the
"pawn changed" gates never fire while the game rebuilds weapon/physics underneath. Two hazards:

1. A remembered release was issued via `stopFire(self, …)` on the **replacement weapon** that
   had never seen our press — game code reached from our hook, on an object still being built.
   That is exactly the kind of call that leaves no mod frame in the stack.
2. `isAiming`/`canFire`/clip count/bone+physics scan were only gated on pawn/match settle, not
   on the weapon.

Guards added:

- stability window on the **(match, pawn, weapon)** triple — any change re-arms
  `settleFrames = 8` frames and the hook returns before any aim/fire/scan call. In steady state
  it only re-arms on a real change → **no added latency**;
- a held press is remembered (`pendingRelease`) the moment the triple changes and is only ever
  released on the **same weapon instance** that was pressed (`pressWeapon`); if the round
  replaced the weapon, the press is dropped instead of released;
- `pressWeapon` cleared wherever a release is actually performed.

`Gameplay1657.h` (sprint-to-fire option): the hooked delay now returns a floor of
`kMinSprintToFireDelay = 0.05f` instead of `0.0f`, and only after a 400 ms
`SprintFireSettleMs` window per (pawn, weapon). Reason: returning 0 makes the weapon
re-enable and fire on the same frame the sprint ends, while the fire component is still being
re-enabled. Cost: sprint-to-fire becomes 50 ms, not instant.

### 5.2 ESP pawn read guard (`Features/EnemyReadGuard1657.h`, used only by `Hacks/Feature/DrawESP.h`)

A tiny time-based guard, independent of the triggerbot (no cross-dependency, so ESP-on +
tbot-off is still guarded and tbot-on + ESP-off is unaffected):

- `settleMs = 300`, `forgetMs = 1000`, 128 slots keyed on pawn address;
- a pawn not seen on the previous frame is **not read** on this one; a pawn that returns after
  being forgotten starts a fresh window (i.e. it is a new object);
- `ReadFlag(base, offset, value)` only accepts bytes `0/1` for `IsAlive` / `IsBot`; anything
  else is a torn read or a reused object and that pawn is skipped;
- applied in `DrawESP.h` at the three places that do a real read pass: closest target, FOV
  target, and the draw loop (`Settled` + `ReadFlag`), plus `ReadFlag` on the bot flag where the
  loop skips bots.
- **Known remaining gap:** the ESP count panel (`if (espCountPending …)` near the end of
  `DrawESP.h`) does its own loop over `enemySnapshot` with `IsPtrValid` + `ReadFlag` but **no**
  `Settled` call, so it is still only protected against torn/reused objects, not against a pawn
  destroyed on the previous frame. Adding `Settled` there is a one-line change if that panel is
  ever implicated; it is left alone because it was never in a crash path.

**Known visual cost:** a newly appearing enemy's box shows ~300 ms late. The human pushed back
on this ("that ESP doesn't need that thing"). It was explained that the guards are independent
and the window can be lowered (~120 ms) if the delay is objectionable — **no change was made**;
if they complain about late boxes, drop `settleMs` to ~120 and re-run
`tests/test_enemy_read_guard1657.cpp` (it parameterises `settleMs`).

### 5.3 ADS Only scope-in hitch (`Features/Triggerbot1657.h`, `EligibleFromLocalView`)

Diagnosis: the ADS gate sat **before** the enemy evaluation, so in `ADS Only` mode every
unscoped frame did nearly nothing and the entire scan (camera transform, up to four ray origins,
walk over `EnemyPawns` reading head/mesh transforms, raycast batch, smoke probes) landed on the
**first scoped frame** — the exact frame that already pays for the scope transition. `Both`
mode spread that work over every frame, which is why it never hitched.

Fix: the mode check became `modeAllows` and now gates the **shot**, not the **work**:

- the scan runs every frame in both modes;
- at the acceptance point: `if (!modeAllows) { Report(DiagnosticStage::AdsRequired); return false; }`;
- the same check was added at the end of the scan (when nothing was usable) so the diagnostic
  still reads "Waiting for ADS" instead of the misleading `SnapshotUnavailable`.

Firing decision and latency are unchanged — the first scoped frame now does no more work than
any other frame. The tradeoff is that `ADS Only` costs the same per-frame CPU while hip-firing
as `Both` always did. That is the price, and it was accepted.

Tests updated in `tests/test_triggerbot1657_native.cpp`: the unscoped ADS-only tick must now
prove the scan **ran** (`diagEnemies > 0`) *and* still refuses with `AdsRequired`.

---

## 6. Diagnostics you can use from a log

`Features/Triggerbot1657Diagnostics.h` defines `DiagnosticStage` (atomic `diagStage`) plus
counters `diagEnemies`, `diagBots`, `diagDotMatches`, hit kind/distance, ray origin. Order:

```
HookNotObserved, Disabled, SnapshotUnavailable, AdsRequired,
LocalControllerMismatch, LocalPlayerUnavailable, WeaponNotReady,
NoEnemies, NoDotMatch, WallBlocked, SmokeBlocked,
EligibilityPassed, FireRequested, FireHeld, FireReleased, FireTapped,
CameraUnavailable, MatchUnavailable, EnemyListUnavailable,
RuntimeExportsUnavailable, …, LinecastPending, SmokePending,
LinecastAbiUnavailable, ColliderPending, FireModeProbe, FireModeProbed
```

`Report()` stores unconditionally (last writer wins). When adding an early-out, set the stage to
what the user would *believe* is happening (e.g. "waiting for ADS"), not to the internal
reasoning that got there.

`RayOrigin` (`None, Fire, Eye, Head, Camera`) records which origin got a clear line. In ADS the
camera origin is tried first because the game fires from the camera and the muzzle ray starts
inside the player's own view model.

---

## 7. Verification and capture

### 7.1 Host tests — the only verification available

```bash
cd <repo>
for t in tests/test_triggerbot1657_native.cpp tests/test_triggerbot1657.cpp \
         tests/test_triggerbot_hitboxes.cpp src/main/jni/tests/test_triggerbot_strict_zone.cpp \
         tests/test_triggerbot_header_compile.cpp tests/test_gameplay1657.cpp \
         tests/test_enemy_read_guard1657.cpp; do
  g++ -std=c++17 -O1 -Wall -Wextra -Isrc/main/jni -I. "$t" -o /tmp/$(basename "$t" .cpp) &&
  /tmp/$(basename "$t" .cpp) && echo "PASS $t"
done
g++ -std=c++17 -O1 -g -fsanitize=address,undefined -Isrc/main/jni \
    tests/test_triggerbot1657_native.cpp -o /tmp/asan && /tmp/asan
```

Notes:

- `tests/test_triggerbot1657_exports.cpp` needs **3 argv** and will look like a failure if run
  bare — it is not a regression.
- Clang is not installed here; the reports that mention both compilers were written elsewhere.
- Run ASan on the native triggerbot test after any change in that header; it has caught real
  out-of-bounds in the fixture before.
- Tests use small in-file fakes for Unity/IL2CPP (`ComponentTransform`, `physicsLinecast`,
  `Camera::main`, `EnemyPawns`, …). Follow that pattern; do not add a framework.

### 7.2 The `LIQUID` base line and why `logcat -s LIQUID:I` looked empty

`Main.cpp` (`Init_Thread`, right after the `while (!m_unity)` loop) prints **once per launch**:

```cpp
__android_log_print(ANDROID_LOG_INFO, "LIQUID", "libunity base %p", …);
```

Nothing else in the project uses the `LIQUID` tag (`ZEUS-ANTIHEAT`, `Chitoge`, `Putri-Il2Cpp`,
`A64_HOOK` are the other tags). So:

- the tag filter is empty at any time other than a launch;
- a 10-minute CODM session rotates that single line out of the ring buffer;
- starting the filter on an already-running game waits forever for a line that will not repeat.

**Correct order: start capture → launch CODM → check.**

`RVA = pc - libunity_base`, compared against the known offset table:

| symbol | RVA (1.6.57) |
|---|---|
| `StartFire` | `0x66704C4` |
| `StopFire` | `0x667062C` |
| `CanFire` | `0x5260EC0` |
| `get_ClipAmmoCount` | `0x50EC614` |
| `physicsLinecast` | `0x506D104` |
| masked linecast | `0x506D07C` |
| controller hook | `0x666894C` |
| `HookDelaySprintFire` | `0x513CBC4` |
| `Pawn.IsAiming` | `0x519BB10` |
| `IsSniper` | `0xC364C94` |
| `animatorBoneTransform` | `0x50182B8` |

### 7.3 `scripts/termux-crash-capture.sh`

```
tb-crash.sh start    clear buffers, log `-b all`, watchdog, watch for the LIQUID base line
tb-crash.sh status   dir, mode, game running?, log lines, mod base found/missing
tb-crash.sh base     print the libunity base line (exit 1 if absent)
tb-crash.sh snap     crash buffer + dropbox (native/java/tombstone) right now
tb-crash.sh stop     stop logging, collect, pack
tb-crash.sh pack     repack the current run
```

Behaviour worth knowing:

- `BASE` defaults to `$SELF_DIR/capture`; `current` holds the active run dir, so `status`/`stop`
  work from a new shell.
- `game_running()` falls back to `ps -A -o NAME | grep` because Gspace renames the process
  (`…:p0`), so `pidof com.garena.game.codm` alone finds nothing.
- `start` polls for the base line for up to 2 minutes and reports it; if it says missing after a
  launch, the likely cause is a **stale APK** (the line only exists from `bfe3ea1` onward).
- `pack` builds the archive in `$TMPDIR`/`$HOME`, requires it to be a readable tar, then copies
  and compares sizes. Direct tar onto shared storage had produced **0-byte** archives.
- Root is used automatically if `su` works; otherwise adb. `FORCE_MODE=root|adb` overrides.

---

## 8. Traps and oddities in the code (do not "clean these up" unasked)

- `DrawESP.h`: `isEspReady` is a global that is never written (always false), so the
  `if (!isEspReady)` block always runs. Deliberately left alone.
- `DrawESP.h` does not skip bots in the enemy loop — bots are only counted. Changing that is a
  behaviour change, not a fix.
- `Main.cpp` patches ~50 `libunity` offsets and hooks `libanogs` blindly at fixed offsets; those
  are version-pinned to 1.6.57. A wrong offset there is a plausible launch-time crash source
  (see §4.1).
- `.bak` files (`Android-game.mk.bak`, `DrawESP.h.bak`, `Init_Hacks.h.bak`) are intentional
  human backups in-tree — not build inputs, but do not delete them.
- Tests live in two places: `tests/` and `src/main/jni/tests/`. Both are host programs.
- Do not introduce per-frame heap allocation, logging, or a second raycast batch into the
  triggerbot/ESP hot path; the human measures "hitch" by feel and per-scope-in work was the
  whole of §5.3.
- `settleFrames`, `kPawnSettleMs`, `kMatchSettleMs` (1500), guard `settleMs` (300) /
  `forgetMs` (1000), `kMinSprintToFireDelay` (0.05f) / 400 ms are all tunable knobs; changing
  them changes feel. Only touch them with a reason, and re-run the matching test.

---

## 9. Open questions, in priority order

1. **Does the r32 + ESP-guard build still crash?** Needs the human to run a match with the base
   line captured. If it crashes, compute `RVA = pc - base` and map it against §7.2's table:
   - inside `animatorBoneTransform` / a bone read → the ESP/pawn path;
   - inside `StartFire`/`StopFire`/`CanFire` → the triggerbot's fire path;
   - inside `physicsLinecast` → the line-of-sight path;
   - elsewhere in `libunity` → likely the game's own code and the identity-key signature of
     §4.2, i.e. a different problem.
2. **Is Gspace a factor?** The identity-key fault address (uid << 32 | pid) and the Gspace
   `SecurityException` storm in the same log suggest the container may be involved. The human
   said they would test without Gspace but the 06:03–06:11 isolation runs were still inside it.
   Ask for one clean run without the container before assuming mod code.
3. **Does the ADS Only hitch actually disappear on device?** Code-level fix is in; needs a feel
   check. If it persists, the next step is a frame-time probe around
   `EligibleFromLocalView` rather than more speculation.
4. **The launch-time `libanort.so` SIGSEGV pair** (§4.1) is still unexplained and is a real,
   separate bug candidate — negative memcpy length at load.
5. **ESP box delay** (~300 ms) — only if the human finds it objectionable; lower `settleMs`.

## 10. Reporting style that works with this human

- Short, direct, no options menus. State the cause, the change, what to do next.
- Never say "fixed" for something only compiled/tested on the host. Say what was verified and
  what was not, explicitly.
- Give one copy-pasteable command block per step.
- If a capture is empty, explain *what the command was waiting for* before asking them to redo
  it — that mistake cost a full round of capture data.
