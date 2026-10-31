# LIQUID r16: License TLS trust-store repair

## Observed failure

`curl 60, HTTP 0`: SSL certificate problem: unable to get local issuer certificate.

This is a TLS certificate-chain validation failure before the license server responds. It does not indicate an invalid license key.

## Changes

- Replaced the single hard-coded `CURLOPT_CAPATH` with a libcurl OpenSSL `CURLOPT_SSL_CTX_FUNCTION` callback on Android.
- The callback reads Android-installed root CA certificates from `/apex/com.android.conscrypt/cacerts` and `/system/etc/security/cacerts` into the TLS trust store directly, avoiding differences in OpenSSL hashed-directory lookup.
- Failure to load trusted roots fails closed.
- Kept `CURLOPT_SSL_VERIFYPEER=1` and `CURLOPT_SSL_VERIFYHOST=2`; no certificate verification bypass or untrusted fallback.
- Kept the original license endpoint, signed-response validation, request method, and license format.
- SSL errors now include libcurl's error string for further diagnosis.
- No gameplay changes. No game dump in the packaged source.

## Verification

- `tests/test_android_trust_store.cpp` passed under GCC and Clang, using a valid PEM root certificate.
- Test validates failure on missing directories and successful loading of an installed PEM anchor (including loading duplicates).
- No Android NDK build or device test was available.
- The external server `ethnir.x10.mx` was unreachable from the test environment, so its current certificate chain is not verified.

## Follow-up if curl 60 persists

Inspect the certificate presented for `https://ethnir.x10.mx/connect` with a trusted TLS inspector. The shared-host server must present a hostname-matching leaf certificate and the complete intermediate chain. Correct the server configuration rather than turning off certificate checks.
