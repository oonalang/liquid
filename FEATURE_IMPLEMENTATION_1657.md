# ETHNIR NOIR V2, CODM Garena 1.6.57 feature integration

## Baseline and constraints

- Baseline: `ETHNIR_NOIR_latest_Main_feature_status.zip`, specifically the user's uploaded `Main.cpp` with six existing `libanogs.so` registrations and `isLogin = true`.
- Reference: user-provided `dump 1.6.57.cs` (IL2CPP metadata). This metadata does not include native function bodies or establish live calling-ABI compatibility.
- All original `Main.cpp` offsets, hooks and anti-cheat-related modifications are preserved verbatim. Four **additional** `libunity.so` hook registrations were added, without replacing pre-existing registrations.
- Existing auth configuration, `libV2` output name, and fixed-size `saved.h` save format are unchanged.
- Every new feature defaults OFF and is configured independently. New settings are session-only, not saved to the older binary format.

## New code

| Feature | New 1.6.57 method entry | C++ path | Source integration |
|---|---|---|---|
| No Slide Fatigue | `SlideTackleComponent.SlideTackleSpeedDownPct()`, `0x8BFE2CC` | `Gameplay1657::HookSlideSpeedDownPct` | Returns zero reduction for components with `m_IsLocalPlayer` true (field offset `0x38`) when enabled |
| No Crouch | `PlayerPawn.CanCrouch()`, `0x54B6820` | `Gameplay1657::HookCanCrouch` | Returns false for the local pawn when enabled, otherwise forwards the original |
| No Aim Drift | `WeaponFireComponent_Instant.GetBreatheTremblingPos()`, `0xC9BD880` | `Gameplay1657::HookBreatheTremblingPos` | Returns zero breathing motion only when owner equals local pawn, component belongs to equipped weapon, `Pawn.IsAiming()` and `Inventory.IsSniper()` are both true |
| No Sprint-to-Fire Delay | `Weapon.get_DelaySprintFire()`, `0x513CBC4` | `Gameplay1657::HookDelaySprintFire` | Returns zero for the local pawn's equipped weapon when enabled, otherwise forwards the original |

`Pawn.IsAiming()` and `Inventory.IsSniper()` have metadata RVAs `0x519BB10` and `0xC364C94`. The following fields are also drawn directly from the supplied dump: `WeaponFireComponent.m_WeaponEntity` `0x10`, `WeaponFireComponent.m_Owner` `0x28`.

### Menus

- Movement: No slide fatigue, No crouch, No sprint-to-fire delay.
- Weapon handling: No aim drift (sniper ADS).
- Enabled-counter totals reflect the toggles. No inactive placeholders were added.

## Still not integrated

**Triggerbot, line-of-sight, and smoke concealment.** The dump lists target and smoke methods but doesn't verify the mapping from live target entities to a local crosshair hitbox, authoritative visible pixels, the correct firing input/state machine, or whether smoke effects block target visibility. Providing automatic firing without those checks violates the request's fail-closed design. The visual dot and firing-zone drawing remain as before, without automated firing.

## Verification performed

- PASSED: GCC and Clang C++17 host compilation of `tests/test_gameplay1657.cpp` with `-Wall -Wextra -Werror`.
- PASSED: Host assertions covering all switches OFF, per-feature activation, local-versus-remote gating, non-sniper/non-ADS bypass, and restoration after disabling.
- PASSED: All six new method/helper addresses match named declarations in the exact 1.6.57 dump.
- PASSED: Original gameplay hooks and old source files (apart from Main/UI/preview integration) were retained, no original offsets replaced.
- UNVERIFIED: Android ARM64 build and all in-match behavior. No Android NDK `ndk-build` and no matching live runtime available in this environment.
- UNVERIFIED: Whether each metadata-derived method is the full mechanism controlling the specified behavior. In particular, `CanCrouch()` might be shared by slide input, `SlideTackleSpeedDownPct()` might not account for all cumulative fatigue, and `get_DelaySprintFire()` might not cover every sprint-fire transition.

## Files modified/added

- Modified: `src/main/jni/Main.cpp`, `src/main/jni/UI/ApplicationUI.h`, `src/main/jni/Preview/PreviewState.h`.
- Added: `src/main/jni/Features/Gameplay1657.h`, `src/main/jni/Features/Gameplay1657Settings.h`, `tests/test_gameplay1657.cpp`, this report.

## Release limitations

This is a **source integration**, not a confirmed working CODM build. Compile with the project's Android ARM64 NDK/AIDE setup and test in a controlled, authorized environment. An IL2CPP dump alone does not support a credible "in-game PASSED" result.
