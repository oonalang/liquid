# ETHNIR NOIR — branding and violet theme

Baseline: CODM_SHAI_original_key_login_fixed.zip. This is an additive visual rebrand.

## Changes
- Brand name: ETHNIR NOIR across native login, control center, Android application label, and Java fallback screen.
- Theme: violet accent (#7744E8), near-black plum surfaces, lavender edge highlights, consistent popup/button/slider/selection colors.
- Android: purple app/window/status palette and custom vector EN launcher icon.
- Preview: purple default accent and backdrop.

## Deliberately unchanged
- Login endpoint `https://shaii.x10.mx/connect`, request fields, legacy response validation, and device-binding behavior.
- Native library identifiers now match: the production NDK module is `V2`, Java loads `V2`, and a fresh build emits `libV2.so`. Previous mismatched identifiers are no longer used in source.
- Every gameplay hook, offset, function body, and toggle mapping.
- Dot crosshair color defaults: an independent visual setting, not the menu theme.
- Generated `build/` outputs and the obsolete bundled native library have been excluded from the V2 source archive to prevent stale artifacts or duplicate native-library packaging. Rebuild to produce `libV2.so`.

## Build
Rebuild the production Android ARM64 library to see the changes in game. The opt-in standalone preview will show the new UI theme when rebuilt as well.

## Verification
- C++17 host syntax check of the actual ImGui application header: PASSED with minimal preview declarations for externally supplied gameplay state.
- Existing 13 authentication parser checks: PASSED.
- Original dot geometry test binary: PASSED.
- Android XML parse: PASSED.
- File hash comparison: gameplay headers, hooking code and authentication files UNCHANGED.
- Android NDK build and live game validation: NOT RUN (NDK and target runtime unavailable).
- Host-side ImGui frame/style runtime test (after font atlas and frame initialization): PASSED, violet accent applied to checkmarks and popup surfaces.
