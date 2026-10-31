# LIQUID: partial-cover visibility correction

## Evidence from the supplied screenshot
- Enemy list: 4 enemies / 4 bots.
- Crosshair: 2 dot matches.
- Ray obstruction: "Other collider" at 7.49 units; selected target at 21.06.
- All four candidate origins (muzzle, eye, head, camera) report obstruction.
- Fire requests: zero. The firing stage is not reached because the visibility gate rejects the candidates.

## Diagnosis
`src/main/jni/Features/Triggerbot1657.h` already uses Unity Physics.Linecast, a segment raycast that returns the first collider. Changing the API to another single-ray cast does not remove an actual obstruction. The original target selection chooses only the nearest point on the projected head-to-root segment. A waist-high box can obscure that single point even if an upper point in the same firing dot is clear.

## Source changes
1. Keep the current closest-on-body ray and existing four origin tests.
2. If blocked, try up to two additional sample points, one above and one below the nearest point, bounded by the original crosshair radius plus the existing projected body-width allowance.
3. Reproject each candidate into screen coordinates and apply the existing eligibility policy *before* doing physics checks. These alternate points never expand the firing area beyond the existing rule.
4. For every candidate, run the existing collider-identity, local-body handling, and smoke checks. An unrelated collider blocks its own ray, including thin boxes or distant walls.
5. Surface `LOS samples` and `Last body fraction` in firing diagnostics. Fraction 0 means projected head; 1 means projected mesh/root.
6. Count wall/smoke rejections per candidate enemy, not per attempted sample.

## Modified files
- `src/main/jni/Features/Triggerbot1657.h`
- `src/main/jni/Features/Triggerbot1657Diagnostics.h`
- `src/main/jni/UI/ApplicationUI.h`
- `tests/test_triggerbot1657_native.cpp`

## Validation
- Native source regression: passed with GCC and Clang, 5 test binaries per compiler.
- Sanitizer run: Clang AddressSanitizer + UndefinedBehaviorSanitizer passed for `tests/test_triggerbot1657_native.cpp`.
- New regression: blocked torso with another point in the dot visible => eligibility passes; exposed head outside dot => blocked; full obstruction => blocked; smoke across alternate point => blocked.
- `git diff --check`: passed.

## Runtime/build limitations
- Android ARM64 production build not performed; Android NDK is unavailable in this environment.
- On-device gameplay behavior, true collider geometry, firing and hit registration are not verified.
- The uploaded prebuilt `libV2.so` is **not** updated. Build from the modified source with your configured Android NDK (project specifies 22.0.7026061). Do not install an APK containing the old shared library expecting the patch to be present.
- An exposed enemy with every permitted ray blocked should still be refused. The code intentionally does not treat "can see on screen" as proof of an unblocked physics path.
