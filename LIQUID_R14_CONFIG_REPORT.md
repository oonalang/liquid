# LIQUID r14: Named configuration profiles

## Implemented

- New **Configs** page in ETHNIR NOIR navigation.
- Save custom-name JSON profile (1–40 ASCII letters/digits/spaces/dash/underscore); existing names are not overwritten inadvertently.
- List, select, load, overwrite and two-click delete existing profiles.
- Select one saved profile as autoload on the next startup, or disable autoload.
- Versioned JSON (`version: 1`) and atomic replacement (`.tmp` then rename).
- Reject path traversal, invalid names, oversized files, unsupported versions and invalid field types.
- App-private writable directory resolved via JNI `Application.getFilesDir()`; no general storage permission required. In virtualized apps the location depends on the virtualization container.
- Persist selected ESP, movement, aim, memory, performance, dot crosshair, UI, and older skin flag fields. Selected SkinHax runtime inventory item is NOT persisted; the runtime asset subsystem is separate.
- Restores derived movement/aim visibility sliders and Gameplay1657 UI atomics after profile load.
- Config actions are event-driven; no disk I/O on the per-frame render path after the one-time startup check.
- Existing `saved.h` is preserved for compatibility. Current UI uses the new versioned JSON path instead of the legacy `wc_config.dat` fixed binary file.
- Preview now resolves app-private storage via JNI and includes missing preexisting preview-only globals for successful syntax checks.

## Not implemented

- The `DoReport`, `DoReportPlayer`, `BeginReport` and `EndReport` suppression hooks were not installed. These intercept normal reporting functions, including potentially security/moderation-relevant reporting. This release does not modify reporting behavior.
- No native ARM64 `.so` was compiled and no in-game test was run.
- No game dump included.

## Verification

- Named JSON storage test: GCC and Clang passed.
- JSON storage test: Clang AddressSanitizer/UndefinedBehaviorSanitizer passed.
- UI bindings: GCC C++17 syntax-only preview headers passed.
- UI state round-trip: save/capture/apply validation test passed.
- Archive integrity checked with Python zipfile and `unzip -t`.

## Changed source

- `src/main/jni/UI/ApplicationUI.h`
- `src/main/jni/UI/NamedConfigUI.h` (new)
- `src/main/jni/UI/NamedConfigStorage.h` (new)
- `src/main/jni/Preview/PreviewNative.cpp`
- `src/main/jni/Preview/PreviewState.h`
- `src/main/jni/tests/test_named_config_storage.cpp` (new)
