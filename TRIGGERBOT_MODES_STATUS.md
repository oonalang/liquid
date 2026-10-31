# ETHNIR NOIR V2 — Standalone Triggerbot and ADS modes

## Source changes
- `src/main/jni/Features/Triggerbot1657Policy.h`: independent ADS Only / Both eligibility.
- `src/main/jni/Features/Triggerbot1657.h`: local controller fire ownership, independent firing state, target/occlusion/smoke validation, game-thread StartFire / StopFire calls.
- `src/main/jni/Main.cpp`: restores Quick Firing's own auto-fire eligibility hook and installs separate local-player update hook.
- `src/main/jni/UI/ApplicationUI.h`: triggerbot toggle and two-mode selector.
- `src/main/jni/Preview/PreviewState.h`: preview mode UI.
- `tests/test_triggerbot1657.cpp`, `tests/test_triggerbot1657_native.cpp`: host behavior tests.

## Method evidence
1.6.57 IL2CPP dump: PlayerController.UpdateLocalPawn(float) `0x666894C`, PlayerController.StartFire() `0x66704C4`, PlayerController.StopFire(bool) `0x667062C`, Pawn.IsAiming() `0x519BB10`, Pawn.CanFire(bool) `0x5260EC0`; Controller.m_Pawn `0x78`. Smoke/physics remain the prior mapped APIs.

## Limitations and safety
- Source inspection and host simulation are NOT proof of functioning multiplayer features. An Android ARM64 compile, ABI validation against `libunity.so`, and an authorized in-game test are still required.
- Physics Linecast is fail-closed and may reject enemies if their hitbox collider intersects the segment.
- Manual input and fire-mode interactions have not been validated, especially for single-shot weapons and forced stop when manually firing.
- Existing user gameplay offsets and anti-cheat hook registrations are unchanged; no new bypasses were added.
- Direct IL2CPP function pointers may have additional metadata calling conventions. Crash risks remain without matching game binary validation.
