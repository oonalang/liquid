# ETHNIR NOIR — V2 native library rename

## What was changed
- `src/main/jni/Android-game.mk`: `LOCAL_MODULE := V2`.
- `src/main/java/com/android/support/MainActivity.java`: `System.loadLibrary(REAL_LIBRARY)` now receives `V2`.
- The NDK output filename is `libV2.so`. The module identifier intentionally omits the `lib` prefix.
- Stale generated `build/` outputs and the old precompiled `jniLibs` library were excluded from the source archive. The old binary had an obsolete license URL and SONAME and does **not** include the latest source changes.
- No `src/main/jni/System` directory was added because it was absent from both the original uploaded project and the ETHNIR NOIR source archive.

## Rebuild requirement
Build the production native module with the project's Gradle/NDK environment. `src/main/jni/Android.mk` selects `Android-game.mk` by default. Verify the fresh output is named `libV2.so` for ABI `arm64-v8a` and the APK contains one `lib/arm64-v8a/libV2.so`. The Java loader must request `V2`.

Android NDK/Gradle and a device runtime were not available for this archive update. A compilation or live login result is **not** claimed.

## GitHub `System` folder
A push to an existing repository does not remove unrelated already tracked files. Deleting a directory locally and uploading new files is insufficient unless Git records removals. Inspect `git status` and `git ls-files src/main/jni/System`, and remove the tracked folder only if you deliberately want to drop those files. The archive itself never contained this folder.

## Preserved
- All gameplay offsets and hook implementations.
- SHAI authentication endpoint and login parser source.
- ImGui controls and purple brand styling.
