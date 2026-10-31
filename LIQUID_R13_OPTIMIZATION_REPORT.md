# LIQUID 1.6.57 r13 - Low-overhead visibility path

## Goal
Reduce the frame-time cost of r12's working triggerbot while retaining its filtered solid-only physics, partial-cover sampling, selected-enemy identification, smoke and firing checks.

## Confirmed performance issues in r12
1. After every clear filtered Linecast, r12 ran another *unfiltered* Physics.Linecast only to update a diagnostic counter. This could double the number of physics queries on clear paths.
2. `Tools::IsPtrValid` in `src/main/jni/IL2CppSDKGenerator/IL2Cpp/Tools.cpp` performs a `write()` to `/dev/random` per validation. Rechecking the same Pawn, Collider and Transform multiple times within one eligibility update incurs repeated kernel work, especially across obstructed origin and sample rays.

## r13 changes
- Keep the existing filtered Linecast result authoritative. Run the legacy comparison query at most once per 1000 ms across all origins/targets, and only after a clear filtered result. It never decides firing eligibility.
- Preserve `QueryTriggerInteraction.Ignore` and **do not skip** solid surfaces.
- Keep the legacy trigger-hit diagnostic as a *cumulative sampled count* instead of a per-update count. Reset at `Initialize`.
- Introduce a bounded, allocation-free pointer validation cache owned by one call to `EligibleFromLocalView`. A validation result is never reused between calls or game frames. Existing pointer validation is still performed for cache misses.
- Display live per-evaluation `Physics casts`, `Diagnostic casts`, `Pointer checks`, and `Reused` on the Aim diagnostics panel.
- No changes to firing thresholds, modes, target-dot geometry, the partial-cover sampling algorithm, smoke logic, or hook RVAs.

## Deterministic call-count comparison
Using the same mocked native test scene and 1,000 eligibility evaluations per case:

| Scenario | r12 | r13 | Reduction |
| --- | ---: | ---: | ---: |
| Clear path: total physics calls | 2,000 | 1,001 | 49.95% |
| Clear path: pointer validations | 10,000 | 10,000 | 0% |
| Obstructed path: physics calls | 6,000 | 6,000 | 0%, intentionally unchanged |
| Obstructed path: pointer validations | 34,000 | 13,000 | 61.76% |

The test mocks use an inexpensive pointer checker and simulated physics, so their measured execution times are **not** valid Android frame-time estimates. The call counts are valid for these test scenes, not guarantees of performance improvement on every device.

## Verification
- 12 native/unit regression tests passed with GCC.
- 12 native/unit regression tests passed with Clang.
- Focused triggerbot native test passed with Clang AddressSanitizer and UndefinedBehaviorSanitizer.
- Only changes to Triggerbot1657.h, Triggerbot1657Diagnostics.h, UI/ApplicationUI.h and tests/test_triggerbot1657_native.cpp relative to r12.
- No game dumps, `.so`, or generated object files in the ZIP. Static `.a` build dependencies are retained.
- No Android NDK toolchain is available in this environment. Real device FPS, thermal performance, binary compatibility and live firing behavior remain unverified.

## Device evaluation
Compare r12/r13 in the same scene using the same graphics settings. Test clear visibility, partial cover, solid walls, smoke, ADS Only and Both modes, and watch the new physics and pointer-check counters while observing game FPS. If FPS remains low, profile other per-frame hooks and ESP rendering rather than removing visibility checks.
