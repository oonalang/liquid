# SHAI / AIZEN original-key login repair

## Source baseline
Restored from `CODM_additive_dot_crosshair_source.zip`, not from the IMGUI3 signed-license conversion. The original ImGui login panel design and all gameplay feature implementations remain intact.

## Changes
- `src/main/jni/Includes/Jni_Stuff.h`: use `https://shaii.x10.mx/connect`, keep original `game=CODMGR&user_key=...&serial=...` POST fields and the existing device fingerprint algorithm. Repair JNI string ownership and application lookup, escape the form fields, add network timeouts and TLS verification, and enforce expected HTTP status. Replaced the nonfunctional placeholder token comparison with validation of the legacy API response. Bound returned token to a successful HTTPS response from the configured panel; reset authentication state before new attempts.
- `src/main/jni/Includes/LegacyAuthResponse.h` (new): strict response checks for `status`, `data.token`, and recent `data.rng`; optional `EXP` and `mod_status`. Reject HTML, invalid JSON, stale sessions, and incomplete server responses without exposing the HTML in the UI.
- `src/main/jni/UI/ApplicationUI.h`: retain original ImGui login appearance; correct the invalid `g_Auth == g_Token` secondary gate; process login asynchronously to prevent the UI thread from blocking during network requests.
- `tests/test_shai_auth.cpp` (new): 13 focused validation checks.

## IMPORTANT: Backend protocol
The old client expects JSON with an authenticated success response in the format:
`{"status":true,"data":{"token":"<server-issued-token>","rng":<current-unix-time>,"EXP":"<optional>","mod_status":"<optional>"}}`

The panel must validate the license and device server-side before returning `status:true`. These changes do not implement a token signature or guarantee the server's protocol agrees with this older response shape. The server's `/connect` route must return JSON on POST, including on invalid keys (`status:false`, `reason`). A hosting-generated HTML page or JavaScript challenge cannot be treated as an authorized login. No attempt is made to bypass hosting protection.

## Verification
- PASSED: 13 local response parser checks using native `g++ -std=c++17 -Wall -Wextra -Werror`.
- PASSED: host C++17 JNI login header syntax check using OpenJDK 21 JNI headers and the project headers, with `-fpermissive` to match the project's existing legacy JNI calls. The existing keyboard/clipboard JNI functions still generate conversion warnings; these functions were not edited.
- PASSED: source-file comparison against the baseline archive for all untouched paths.
- BLOCKED: live `shaii.x10.mx/connect` response verification. Network resolution/web access was unavailable; no real key or server-side panel files were provided.
- UNVERIFIED: full Android NDK link, APK installation, device login, gameplay runtime behavior.

## Expected use
Rebuild the native ARM64 library from this source. The original prebuilt `.so` copied in the archive is not rebuilt, and will not contain the source changes.
