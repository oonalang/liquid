# ETHNIR NOIR V2 - CODM 1.6.57 native auto-fire visibility gate

## Scope

This update adds a **native auto-fire eligibility gate**, not an independently verified triggerbot. The hook targets `PlayerController.IsAimTargetCanAutoFire(Weapon)` at RVA `0x66613A8` (method declaration in user-provided 1.6.57 dump). The game must already execute `UpdateAutoFireTarget`, otherwise this hook cannot initiate shots. No direct `StartFire` injection is implemented.

The gate requires: local player alive, current weapon, valid enemy head bone, projected target point within configured crosshair radius, current ImGui frame, no physics collision from local camera to target, and no smoke at camera, target, or between them. Missing data always denies firing. It preserves the original method result, so weapon readiness and game firing state are not forcibly bypassed.

Dump method RVAs: `UnityEngine.Physics.Linecast(Vector3,Vector3)` `0x506D04C`; `BaseGame.IsSmokeInMiddle(Vector3,Vector3)` `0x9CC36E0`; `BaseGame.IsInSmoke(Vector3)` `0x9CC3518`.

## Limitations and risk

- **Not runtime verified:** IL2CPP dump signatures and RVAs do not establish native ARM64 calling conventions or method behavior. The hidden MethodInfo* convention is an assumption to be verified against a matching libunity binary.
- `Physics.Linecast` with the default mask might include target colliders, causing false negatives. It also might omit geometry on other layers. The smoke API might not represent every visual smoke effect. Therefore this is **not verified reliable wall/smoke protection**.
- The dot check uses the projected head-bone point, not a verified target hitbox. A head point intersection is narrower than a full hitbox.
- This gate does not generate firing events and will have no effect if native auto-fire logic is inactive. Do not claim it is a working standalone triggerbot.
- Only a matching executable and on-device testing could resolve these uncertainties. Avoid enabling on ranked/live multiplayer without safety verification.
- Original offsets and hook bodies were not modified; this adds one new hook and two new headers plus a test.

## Verification on this environment

- PASSED: Exact four RVA and method declarations matched against uploaded 1.6.57 dump.
- PASSED: GCC and Clang C++17 tests for core fail-closed firing decision.
- PASSED: GCC and Clang C++17 simulated native-adapter tests, including clear view, wall obstruction, smoke between points, smoke at endpoints, stale frame, missing callbacks, invalid enemy, weapon mismatch, weapon not ready, dot disabled.
- PASSED: The four previously integrated movement/weapon feature host tests still pass.
- PASSED: Original hook bodies, gameplay offsets, license system and Android module configuration remain byte-identical.
- UNVERIFIED: Android ARM64 compilation, SDK signatures at runtime, live camera projection and native autofire call path.
- NOT IMPLEMENTED: An independent automatic firing input mechanism. The game must already invoke its own auto-fire controller.

## Source changes from previous release

- Modified `src/main/jni/Main.cpp`: include, init, and one additional PlayerController eligibility hook.
- Modified `src/main/jni/UI/ApplicationUI.h`: triggerbot eligibility toggle, description, and published geometry.
- Modified `src/main/jni/Preview/PreviewState.h`: preserve preview compilation without linking game hooks.
- Added `src/main/jni/Features/Triggerbot1657Policy.h`: pure fail-closed decision policy.
- Added `src/main/jni/Features/Triggerbot1657.h`: native game adapter for head-bone projection, camera linecast and smoke checks.
- Added two unit tests under `tests/`.
