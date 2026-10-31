# Loadout model and triggerbot corrections

## Reference

Compared against the user's [CODM 1.6.57 metadata dump](https://huggingface.co/datasets/jeudhehs/dump/blob/main/dump%201.6.57.cs).
Downloaded dump SHA-256: `dd94b4b41a2e5fffaff3d1635d23f9290ab70ca4960d7be853d95381cced758f` (matches the dataset's LFS object hash).

## Confirmed defects corrected

1. **Loadout resource layout.** `ItemResourceConfig` contains four `ushort` sprite-pool indices at object offsets `0x10`, `0x12`, `0x14`, `0x16`, followed by `FxAssetID` at `0x18`, `ID` at `0x1C`, `InventoryModelID` at `0x20`, and `ModelAssetIDRaw` at `0x24`. The former four-pointer declaration shifted resource lookups and model access by `0x18` and allowed writes beyond the resource row. The corrected layout has compile-time offset/size assertions. Selection copies the correct sprite indices and model fields while preserving the row key, avatar field, and surrounding memory. Evidence: dump lines 918011–918028.
2. **Default skin identity.** Selection already captured `WeaponConfConfig.DefWeaponSkinID`, but application replaced it with the separate weapon item ID. Application now uses the captured `WeaponConf[4]` value. Evidence: dump lines 930329–930365 and the existing capture/application code.
3. **Triggerbot camera transform.** The shared legacy wrapper points to `AtmosphericScatteringSun.get_transform` (`0x87192AC`), which reads that class's cached field; it is not a valid getter for a Camera. The triggerbot now binds `UnityEngine.Component.get_transform` (`0x50291B0`) directly for its camera and target hierarchy. Evidence: dump lines 252502–252503 and 1503023–1503042. The unrelated shared wrapper is outside this fix's scope.
4. **Target collider rejection.** The old boolean linecast classified any collision, including the selected enemy, as a wall. The triggerbot now uses the overload returning `RaycastHit`, accepts a first hit belonging to the selected enemy's transform hierarchy, and rejects other or unidentified hits. Smoke checks still run. Evidence: `Physics.Linecast(Vector3, Vector3, out RaycastHit)` at `0x506D104` (dump lines 269824–269825), `RaycastHit` layout at 271388–271394, its `get_transform` at `0x30BDB88`, and `Transform.IsChildOf` at `0x509DF80`.
5. **Stale ADS status.** Previously, camera/match/list failures could return without replacing an earlier "Waiting for ADS" status, even after choosing Both. Those paths now report their current failure. Mode is sampled with the same configuration snapshot as dot geometry, and UI settings publish after controls update.

The existing firing, ADS, controller-update, and smoke method offsets match the dump and were retained. No method implementations were available in this metadata dump to establish another firing failure.

## Changed files

- `src/main/jni/Hacks/Feature/Fields.h`
- `src/main/jni/Hacks/Feature/SkinHaxSystem.h`
- `src/main/jni/Hacks/Feature/ItemResource1657.h` (new)
- `src/main/jni/Features/Triggerbot1657.h`
- `src/main/jni/Features/Triggerbot1657Diagnostics.h`
- `src/main/jni/UI/ApplicationUI.h` (move configuration publication after UI edits)
- `tests/test_triggerbot1657_native.cpp`
- `tests/test_item_resource1657.cpp` (new)
- This report (new)

Every other original archive file remains byte-identical. No build configuration changes were required.

## Validation

- Full production `libV2.so` ARM64 compile and link: **PASS**, Android NDK **22.0.7026061**, Android API 21, using the existing Android.mk/Application.mk. No compiler errors or warnings were emitted under the project's existing flags.
- GCC C++17: **PASS**, resource regression, native triggerbot regression, triggerbot policy, dot geometry, existing gameplay policy, and execution distance guard.
- Clang C++17: **PASS**, resource and native triggerbot regressions.
- GCC AddressSanitizer + UndefinedBehaviorSanitizer: **PASS**, resource and native triggerbot regressions.
- Independent review against dump definitions: **PASS**.

The resource regression populates raw dump offsets and checks model fields, sprite order, preserved identifiers, and memory canaries. The native triggerbot simulation covers ADS/Both behavior, camera getter isolation, selected-enemy and child collider hits, walls, missing hit identity, smoke, and stale diagnostic replacement.

## Remaining verification

There is no running CODM instance or attached Android device in this environment. Actual loadout rendering and shots have **not** been observed. Metadata matching, compilation, and simulated native tests establish the source corrections, not a successful live-game result. The dump does not contain asset bundles or live configuration values; the patch uses the game's captured resource values rather than invented model IDs.

For a device check, restart the game to clear previously modified resource rows, select the skin, and reopen the loadout preview. For the triggerbot, Both should allow unscoped eligibility; ADS Only should wait while unscoped. The dot must remain enabled, as in the existing implementation. If firing still fails, the current diagnostic stage and fire-request counter now distinguish camera, target, obstruction, and native-fire issues.
