# ETHNIR NOIR login endpoint and signed-session alignment

## Scope
Updated the AIDE source to use the same login endpoint and signed-session verifier as the inspected IMGUI3 source. No gameplay offsets, hooks, patch sites, module names, or gameplay settings were edited.

## Changed files
- `src/main/jni/Includes/Jni_Stuff.h`: requests `https://ethnir.x10.mx/connect`, sends `game=CODMGR`, `user_key` and `serial`, enables detailed libcurl diagnostics, verifies signed license reply, preserves JNI compatibility.
- `src/main/jni/Includes/ImGui3SignedSession.h` (added): RSA-SHA256 signature check against the IMGUI3 public verification key, validates key binding, device binding, build and expiration.
- `src/main/jni/Includes/EthnirAuthReply.h` (added): JSON response validation. Does not silently accept unsigned legacy responses.
- `tests/test_signed_session.cpp` (added): host-side verifier tests.

## Important limitations
- The remote server was not accessible from available web checks. Server availability, TLS chain compatibility on the user's phone, license acceptance and Android in-game login remain UNVERIFIED.
- This ZIP is source-only; rebuild the native library and the app.
- If the panel issues legacy `token`/`rng` instead of signed `payload`/`sig`, the new client reports a protocol mismatch rather than bypassing verification.
- If the server redirects at `/connect`, the client rejects the redirect to avoid sending the license key to another domain. Fix panel routing or use the exact destination after inspection.
- TLS certificate verification stays ON. If the bundle of trusted certificates is not available to the bundled OpenSSL/libcurl, repair CA provisioning rather than disabling verification.

## Build/runtime diagnosis
The UI now displays `curl <numeric code>, HTTP <status>` and the libcurl error buffer, without printing the submitted key or device identifier. A failure before HTTP response generally shows `HTTP 0` and requires resolving network, DNS, or TLS separately from license verification.
