# Additive Feature Report

Scope: Uploaded `#liquid glass With Attachment` source archive.

## Preserved

- All existing offsets, game-hook addresses, patched byte arrays, and IL2CPP field layouts.
- `src/main/jni/Main.cpp`, `src/main/jni/hooks.h`, `src/main/jni/Patches.h`, and original game integration: **byte-identical** to the uploaded archive.
- Existing Long slide feature, including its original runtime logic. No migration to No Slide Fatigue was attempted.
- `src/main/jni/saved.h` binary configuration format, to avoid breaking previous saves.

## Added

- `src/main/jni/Features/CustomDotCrosshair.h`: separate default-OFF crosshair settings, resolution-aware drawing geometry, adjustable dot size, RGB color, opacity, outline, outline thickness, horizontal/vertical center calibration, and optional guide ring with adjustable radius.
- `src/main/jni/UI/ApplicationUI.h`: adds a CUSTOM DOT section to the existing Aim tab and renders the dot behind menu windows from the same ImGui display-coordinate space.
- `src/main/jni/UI/ControlDescriptions.h`: concise descriptions for added options.
- `tests/test_dot_geometry.cpp`: standalone host tests for crosshair geometry at multiple resolutions and invalid rectangles.

## Limitations

- **Triggerbot: BLOCKED.** No game version-specific verified target/visibility/fire integration exists. The `Firing zone guide` is a visual guide only; it does not shoot or validate targets.
- **Smoke and wall visibility: BLOCKED.** No reliable local-view smoke and geometry occlusion pipeline verified.
- **No Slide Fatigue: BLOCKED.** Related current hooks exist, but no independent, confirmed fatigue mechanism was verified. Existing Long slide preserved.
- **No Crouch: BLOCKED.** No verified, distinct crouch input interception path identified.
- **No Aim Drift: BLOCKED.** No verified ADS-duration sway source identified.
- **No Sprint-to-Fire Delay: BLOCKED.** No verified sprint/fire-state transition hook identified.
- New crosshair controls live in memory for the current session; the old binary save structure has not been extended. Persistence is not implemented in this change.

## Testing

- PASSED: Host C++17 syntax-only compilation of the menu in standalone preview-state harness with its normal preview-only stubs.
- PASSED: Standalone crosshair geometry tests on five display sizes and invalid targets.
- PASSED: Original ZIP integrity comparison found only `UI/ApplicationUI.h` and `UI/ControlDescriptions.h` edited among existing files.
- BLOCKED: Android APK/ARM64 build and device runtime tests (Android NDK and game runtime unavailable here).

Compile the standalone geometry test on a machine with Clang C++17:

```sh
cd 'src/main/jni'
clang++ -std=c++17 -I. -IImGui ../../../tests/test_dot_geometry.cpp -o /tmp/test_dot_geometry
/tmp/test_dot_geometry
```

This build command is for isolated geometry, **not** the CODM native library.
