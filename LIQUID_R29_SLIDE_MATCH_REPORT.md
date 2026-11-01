# LIQUID r29: slide fatigue and the second-match crash

## 1. "No slide fatigue" did not stop slides getting shorter

### Why the old switch did nothing

The toggle hooked `SlideTackleComponent.SlideTackleSpeedDownPct()` (`0x8BFE2CC`) and
returned `0.0f` for the local player. That method is the speed penalty a slide
applies, not the budget that ends it. The slide budget lives in the same component
and is spent by `LimitSlideTackle()` (`0x8BF7234`, non-virtual, so every slide
component reaches the same code). Its configuration fields are named accordingly:

| Field | Offset | Role |
|---|---|---|
| `m_SlideTackleLimitDecreaseRate` | `0x48` | how fast the budget is spent |
| `m_SlideTackleIdleTime` | `0xB0` | full slide length |
| `m_CurrentMaxIdleTime` | `0x174` | the shortened length actually used |
| `m_SlideTackleLimitCDTime` | `0x44` | cooldown once the budget is gone |
| `m_SlideTackleLimitMinIdleTime` | `0x50` | shortest slide before it refuses |

That matches the reported behaviour exactly: the third slide is cut short
(`m_CurrentMaxIdleTime` reduced), and the fourth is refused (`m_SlideTackleLimitCDTime`).

### Change

- `Features/Gameplay1657.h`: new `HookLimitSlideTackle`, which skips the budget update
  for components whose `m_IsLocalPlayer` (`0x38`) is set while "No slide fatigue" is on,
  and otherwise forwards to the original. Remote players keep their normal slide limit.
- `Main.cpp`: registers the hook at `0x8BF7234` next to the existing slide hooks.
- The previous `SlideTackleSpeedDownPct` hook is kept; it removes the speed penalty.

## 2. Triggerbot crashed when a second match was started

### Cause

`MatchGame` is a new object for every match, and the previous match's pawn, controller
and weapon are destroyed around that swap. The triggerbot kept its press state
(`triggerHeldFire`, `fireModeWeapon`, probe timers) in globals that only reset on a
controller or alive-state change. When a new match replaced the old one, a press still
held from the previous match kept issuing `StartFire`/`StopFire` against objects the
old match had already torn down, and the newly spawned weapon was used before it was
finished initialising.

### Change (`Features/Triggerbot1657.h`)

- Track `MatchGame` each frame. When it changes: release a held press, reset all trigger
  state, and mark the match unsettled.
- While unsettled (1500 ms after a match swap; the first match after injection is not
  delayed), the trigger is not evaluated and nothing is pressed, so the scene load and
  the first spawn are not touched. Firing resumes on its own afterwards.
- The equipped weapon pointer must also be readable (`Tools::IsPtrValid`) before a press
  is considered ready.
- Existing guards are unchanged: local pawn/controller match, alive state, `CanFire`,
  occlusion and smoke checks.

## Verification

- `tests/test_gameplay1657.cpp`: extended. Asserts the fatigue routine runs while the
  toggle is off, is skipped for the local component while it is on, still runs for a
  non-local component, and runs again after the toggle is turned off.
- `tests/test_triggerbot1657_native.cpp`: extended. Asserts a held press is released when
  `MatchGame` changes, that no press is issued until the settle window expires, and that
  firing resumes once it has.
- All host tests that build without OpenSSL pass under `-std=c++17 -Wall -Wextra -Werror`:
  gameplay, native triggerbot, header compile, policy, hitboxes, linecast ABI, runtime.
  `test_triggerbot1657_exports` still requires its two command-line arguments (it reads an
  exported symbol table) and was not run.
- `test_signed_session` / `test_ethnir_auth_reply` still cannot be built here: no OpenSSL
  development headers and no root to install them.

## Not verified

- No Android build and no device here, so neither fix is confirmed in a live match.
- `LimitSlideTackle()` is identified from the 1.6.57 metadata as the slide-budget routine
  by name, its config fields and its placement in `SlideTackleComponent`. The dump has no
  function bodies. If slides still shorten, the next place to look is the pawn-level
  `CheckSlideTackle` gate (`Pawn` slot 482, `0x51F1FF8`).
- The crash fix is lifecycle hardening, not a diagnosed crash. A tombstone or
  `adb logcat -b crash -d -v threadtime` captured right after the crash would name the
  faulting frame and confirm whether the triggerbot, the skin system or the ESP is the
  one that dereferences the dead object.
