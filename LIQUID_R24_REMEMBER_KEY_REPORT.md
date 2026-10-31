# LIQUID r24: Remember License Key

Base: local r23 archive. GitHub was not consulted or modified.

## Implementation

- The login panel reads the saved license once at startup and fills the input field.
- Remember key is enabled by default. Disabling it removes the stored key.
- Forget saved key deletes the stored value and clears the input.
- A license is persisted only after a successful signed server validation.
- Every subsequent login still calls the existing license server; remembered credentials do not bypass expiry or device binding.
- AndroidKeyStore generates and stores a non-exportable AES key. AES-GCM ciphertext, a 12-byte IV, and a version byte are saved to app-private SharedPreferences. No plaintext license is persisted.
- Existing license session gating was initialized to logged in. r24 switches it to logged out so server validation runs before the UI is unlocked.
- Keystore availability requires supported Android APIs (23+). If the Keystore is unavailable, the key is not saved, and normal manual sign-in remains available.

## Changed files

- `src/main/jni/UI/RememberedLicense.h` (new)
- `src/main/jni/UI/ApplicationUI.h`
- `src/main/jni/Main.cpp`
- `tests/test_remembered_license.cpp` (new)

## Verification

- 44 host test runs passed (GCC + Clang).
- Android-specific JNI source compiled with both GCC and Clang against JNI headers.
- Standalone payload encoding tests passed. Host tests do not exercise a real Android Keystore.
- No Android NDK or device is available for runtime verification.
- Neither game dumps nor compiled shared libraries are in delivered archives.
