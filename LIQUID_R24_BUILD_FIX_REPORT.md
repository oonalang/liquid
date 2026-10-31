# LIQUID r24 Android Compilation Fix

Base: local `LIQUID_r24_Remember_Key_No_Dump.zip`. No GitHub commits were fetched or changed.

## Fixed compilation errors

1. `src/main/jni/UI/RememberedLicense.h`: `AttachCurrentThread` received a `void**` cast, but the Android NDK's `JavaVM` C++ API expects `JNIEnv**`. Added a signature-selecting wrapper so the code supports both Android NDK (`JNIEnv**`) and desktop JNI (`void**`) conventions.
2. `src/main/jni/UI/ApplicationUI.h`: `OblivionSkinHax::gHasSelectedSkin` does not exist. Replaced it with a mutex-protected check of the existing `OblivionSkinHax::gAppliedSkins` collection. The SkinHax tab's active counter now represents at least one applied skin.

## Verification

- 21 standalone host test programs pass with GCC and Clang (42 test runs total).
- Added `tests/test_remembered_license_jni_types.cpp`, which exercises both JNI `AttachCurrentThread` argument signatures.
- Forced Android-branch syntax check on `RememberedLicense.h` passes with both compilers using available host JNI headers.
- No Android NDK is installed in this environment. The full `Main.cpp` Android build, app startup, and Keystore behavior remain unverified.
- Both ZIP archives were integrity-checked and exclude IL2CPP dumps and generated shared libraries. Existing `.a` static build dependencies are retained in the full source archive.

## Modified files

- `src/main/jni/UI/RememberedLicense.h`
- `src/main/jni/UI/ApplicationUI.h`
- `tests/test_remembered_license_jni_types.cpp` (new test)
