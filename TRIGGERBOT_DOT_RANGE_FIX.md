# ETHNIR NOIR V2 — Triggerbot dot/range consistency patch

## Scope
- Baseline: `ETHNIR_NOIR_V2_TRIGGERBOT_ADS_BOTH.zip` (latest complete source).
- Target: CODM Garena 1.6.57, using the already-provided metadata dump.
- Requested maximum distance: **5,000 world units**, interpreted in the same way the original ESP displays Unity position distances as meters. This is an eligibility ceiling, not a guarantee that distant actors are loaded, visible, or within a weapon's effective firing range.
- Existing hook addresses, `Main.cpp`, `hooks.h`, `Patches.h`, authentication, and native module naming remain byte-for-byte unchanged.

## Source-level changes
1. `src/main/jni/Features/Triggerbot1657Policy.h`
   - Added a screen-space enemy-body segment projection with approximate body half-width.
   - Added a degenerate/subpixel fallback for enemies whose head and body project to effectively the same pixel.
   - Added an explicit `<= 5000` world-unit distance limit and finite-value validation.
   - Raised the ImGui snapshot freshness window from 125 ms to 350 ms. Unknown/stale snapshots still deny activation.
2. `src/main/jni/Features/Triggerbot1657.h`
   - Uses projected head and body/root, with nearest point on segment aligned to crosshair.
   - Visibility checks use that corresponding body point, rather than always linecasting to the head.
   - Handles a partly clipped body at close range by sampling the visible part of the projected head-to-root segment.
   - Publishes new ImGui state without momentarily clearing `enabled` on each normal UI frame.
   - Preserves ADS Only / Both, independent StartFire/StopFire, enemy checks, physics linecast and smoke checks.
3. `src/main/jni/UI/ApplicationUI.h`
   - Updated the explanatory text for dot overlap and 5,000-unit ceiling.
4. `tests/test_triggerbot1657.cpp` and `tests/test_triggerbot1657_native.cpp`
   - Added assertions for close torso overlap, tiny distant projections, near-plane partial visibility, the distance ceiling, and old hold/stop behavior.

## Tests run
- `g++` C++17 with warnings-as-errors: triggerbot policy and native simulation PASSED.
- `clang++` C++17 with warnings-as-errors: triggerbot policy and native simulation PASSED.
- Clang AddressSanitizer + UndefinedBehaviorSanitizer: native simulation PASSED.
- Existing dot geometry and gameplay 1.6.57 policy tests: PASSED.
- Existing `Main.cpp`, `hooks.h`, `Patches.h`: byte-for-byte unchanged.

## Remaining limitations
- **UNVERIFIED:** Android ARM64 compilation, native ABI, on-device firing state, and live enemy/visibility behavior. The build environment here lacks Android NDK / AIDE.
- **UNVERIFIED:** The game's Unity world distance unit exactly equals one physical meter. The 5,000 check is in world units.
- **POSSIBLE FALSE NEGATIVE:** `Physics.Linecast` returns a simple boolean, which may treat the target's own collider as an obstruction; there is no validated collider-identity filter here. Occlusion is deliberately kept fail-closed rather than firing through walls.
- **POSSIBLE FALSE NEGATIVE:** Targets absent from `BaseGame.EnemyPawns`, not projected by the camera, or outside actual rendering/simulation range cannot be detected. This patch does not load distant actors.
- **POSSIBLE FALSE NEGATIVE:** The visible enemy body is approximated by the projected head/root segment and width; it is not the game's authoritative hitbox shape.
- The existing project's `Tools::IsPtrValid` uses `/dev/random` writes, which do not establish whether a pointer is safely readable; this pre-existing implementation was not modified.

## Delivery status
**PASSED**: isolated source edits, host simulation, regression checks, and archive integrity.
**UNVERIFIED**: actual CODM 1.6.57 runtime behavior, weapon/match network logic, native visibility result accuracy.
