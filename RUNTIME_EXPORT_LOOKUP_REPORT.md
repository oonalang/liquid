# Runtime export lookup correction

The latest screenshot confirms controller updates, three enemies and one dot match. The runtime adapter stops before collision invocation because one or more required API pointers are missing. This does not identify which symbol is absent or prove that the game exports those APIs.

The previous loader only attempted `dlopen("libunity.so")` once during initialization. This update inspects the already loaded `libunity.so` and `libil2cpp.so` modules through normal linker APIs, tries their actual loaded paths and names, and retries incomplete resolution once per second when invocation is needed. It does not load another engine, mix partial API tables from different handles, or restore the raw calls associated with the earlier crash report. A complete runtime handle is retained for the lifetime of its function pointers.

Diagnostics now distinguish failure to open a loaded runtime library from missing exports within an opened library. For missing exports, the firing diagnostics list their exact names. Metadata lookup and firing behavior are otherwise unchanged. Health and Mythic changes from the previous package are preserved.

## Validation

- Production ARM64 library compile/link: PASS, Android NDK 22.0.7026061.
- Loader integration test: PASS under GCC, Clang, and GCC AddressSanitizer/UndefinedBehaviorSanitizer. The test starts without runtime libraries, loads a partial Unity fixture missing `il2cpp_runtime_invoke`, then loads a complete IL2CPP fixture. It verifies exact missing-symbol reporting, retry recovery, selection of a complete API table from one handle, and retained handle lifetime. Fixture symbols are never invoked as managed APIs.
- Existing runtime-invocation, native triggerbot, health geometry and resource-layout tests: PASS under GCC with warnings treated as errors.

Reproducing the loader test requires compiling `tests/fixture_triggerbot_exports.cpp` as a shared library twice: `-DOMIT_INVOKE` to `libunity.so`, and without that flag to `libil2cpp.so`. Compile `tests/test_triggerbot1657_exports.cpp` with `-I src/main/jni -ldl -pthread`, then pass those two absolute library paths as arguments.

## Remaining evidence needed

No game runtime is available here, so successful symbol resolution and firing on the user's device remain unverified. If the new diagnostics still list missing APIs, the matching game `libunity.so` and, if present, `libil2cpp.so` are needed to inspect actual exports. A C# metadata dump contains declarations and RVAs, but cannot establish exported symbols or native calling conventions. No replacement native offsets were guessed.
