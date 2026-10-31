# LIQUID r12 - explicit trigger-ignore line of sight

## Observation
The r11 screenshots establish that a target inside the dot fires on a clear path, but when a box is near the player's feet all four candidate lines report the same blocking Collider A, even with no smoke. The screenshots alone do not identify whether A is a solid collider, an oversized solid collider, or a trigger volume.

## Verified 1.6.57 metadata
- `UnityEngine.Physics.Linecast(start, end, out RaycastHit)`: RVA `0x506D104`, forwards to `0x506D07C` using default layer mask `-5` and `QueryTriggerInteraction.UseGlobal` (0).
- `UnityEngine.Physics.Linecast(start, end, out RaycastHit, long layerMask, QueryTriggerInteraction)`: RVA `0x506D07C`.
- `UnityEngine.Collider.get_isTrigger()`: RVA `0x50279D4`.
- `QueryTriggerInteraction.Ignore` is 1.
- Existing ARM64 wrapper decoding determines whether the out-hit register is x0 or x1. r12 preserves this verified ABI for the full overload. The actual game library and its exact instructions were not available for native compilation in this environment.

## Changes
- Use the full `Physics.Linecast` overload with default raycast layers and explicit `QueryTriggerInteraction.Ignore`, instead of relying on `UseGlobal` (which can count non-solid trigger volumes as obstacles).
- Preserve collision with non-trigger environment geometry, selected enemy collider detection, existing bounded self-collider skipping, 3-point sampling, smoke checks, ADS/Both modes and StartFire/StopFire logic.
- For diagnostic evidence only, when the filtered ray is clear, run a legacy ray and check `Collider.get_isTrigger`. Count intersections where a trigger would have blocked the old ray. This extra query cannot change firing eligibility.
- Display `LOS filter: solid geometry (ignore triggers) | Trigger hits excluded: N` in the Aim diagnostics. UI reads only atomic diagnostics.

## Verification
- 12 native/unit host tests passed with GCC and 12 with Clang (including 2 signed-auth tests linked to the required OpenSSL libraries).
- Triggerbot native regression passed with Clang AddressSanitizer/UndefinedBehaviorSanitizer.
- New regression cases: a trigger-only box no longer blocks, a solid obstacle behind the trigger still blocks, solid-only cover still blocks, smoke still blocks, normal clear path still passes, explicit mask/query ABI receives -5/1 for both verified register conventions.
- Android NDK full build and in-game testing not performed here. If the object tagged A is a genuinely solid collider, the new query correctly keeps blocking it; it is not evidence of a confirmed in-game fix.

## Delivery
- Full source-only archive based on r11.
- No `dump 1.6.57.cs`, other game dumps, Android compiled `.so`, or generated object files.
- Required static `.a` build dependencies retained.
