# LIQUID r21: Triggerbot logic and performance hardening

Baseline: local `LIQUID_r20_Mythic_Effects_No_Dump.zip`, **not** GitHub `main`.
No repository push was performed.

## Confirmed source issues

- Visibility checks accepted a clear ray from *any* of four origins, including the camera, even when the firing origin was blocked.
- Up to three aim samples and four origins could generate 12 or more physics calls for one candidate; additional candidates increased the workload without an overall limit.
- Successful automatic-fire classification always released an active firing request at the end of the classification interval.
- The render-thread configuration snapshot had only one attempt; concurrent UI updates could temporarily invalidate the read.
- After eight invalid muzzle/eye origin readings, those getters were never retried until initialization.
- When both projected body endpoints were invalid, the projection fallback sampled only five evenly spaced points, potentially missing a narrow near-camera visible segment.

## Modifications

- Require one valid firing-origin path (muzzle, otherwise eye/head) **and** the camera-to-target path to be clear for the same eligible target point. Camera-only evidence no longer permits firing. Solid colliders and smoke still reject.
- Limit total Linecast calls to 24 per eligibility evaluation, including self-hit retries, and reject safely once the budget is consumed. Clear targets normally use two casts, rather than one, as a security/performance trade-off.
- Preserve held fire when a weapon is classified automatic, avoiding a forced StopFire/StartFire cycle at the classification boundary. Semi-auto behavior is unchanged.
- Retry the configuration snapshot up to three times and periodically retry invalid muzzle/eye getters. Reset origin failure counts when the local controller changes.
- Expand the partially clipped target fallback to nine projection samples, invoked only if one/both body endpoints fail projection.

## Tests

- All 19 host test programs passed using GCC and Clang: 38 successful runs.
- Native triggerbot tests passed AddressSanitizer and UndefinedBehaviorSanitizer.
- Additional simulated tests cover camera-clear/muzzle-blocked, muzzle-clear/camera-blocked, continued automatic firing at classification, origin recovery, and 24 simultaneous blocked candidates obeying the Linecast budget.
- All packaged ZIP entries passed CRC verification.

## Unverified on device

- No Android NDK build or live CODM match was performed. Host tests do not prove real game performance, weapon firing consistency, or correct collider layers.
- The line-of-sight cap might reject eligible targets in crowded encounters. Requiring both paths may reject a visible target when the firing origin is transiently invalid.
- Close-/far-range failures might also involve native WorldToScreenPoint behavior, frame timing, weapon-specific events, or unverified game offsets. These cannot be conclusively resolved without device telemetry.
- FPS improvement is not measured. Worst-case physics-call count is bounded per evaluation, but clear targets now require two independent traces.

## Scope

Only `src/main/jni/Features/Triggerbot1657.h` and its native regression test changed, plus this report. Skin systems, cosmetics, other gameplay features, config storage, authentication, and UI remain from local r20. The public diagnostics panel remains disabled. No game dumps are packaged; existing static build dependencies are preserved.
