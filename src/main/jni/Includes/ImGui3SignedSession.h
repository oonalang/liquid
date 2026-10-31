#pragma once

// Compatible with IMGUI3's signed session protocol. The embedded modulus is a
// PUBLIC verification key. No signing credentials are stored in the client.
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/obj_mac.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>

namespace imgui3_license {
static constexpr const char* kEndpoint = "https://ethnir.x10.mx/connect";
static constexpr const char* kGame = "CODMGR";
static constexpr const char* kModulus =
    "C61010F4D55F6586D543EE585BA0884E687FCFC6C397316664E4EB483F602D41304F60BC7BB37DA65046522663D79E81"
    "012F84492A1721FFEAA23FD32AF9A0C7762182F4708B6467ABACE56D3B9906F6F0BAE315DE04F0D12BE664A646A5D95C"
    "9CF5B538006179D8C38D03216380834513B0A9F8E85466DD8EFD0BEAD4365D28750B0430749E178304874CE50EC39E75"
    "5F772BF735683847EF9F4FD81B117FCA08D3C146C8704DE06E541B679907728BB3632B168E751B2308B7362AC2D4EF1C"
    "1168B89E168DC4C169D81AA28A23B53AC3CF514227A184E3C93C1983D37DFBF26AAE0FFECA875CAE18FA8AB00B12806C"
    "009D22BA93C6A51B592A0FB2A67FA52B";
static constexpr unsigned long kExponent = 65537;
static constexpr int kBuild = 1;

struct Verdict {
    bool ok = false;
    std::string reason;
    std::time_t expires = 0;
};

inline bool DecodeBase64(const std::string& input, std::vector<unsigned char>& decoded) {
    if (input.empty() || input.size() > 8192 || (input.size() % 4) != 0) return false;
    size_t pad = 0;
    if (input.back() == '=') ++pad;
    if (input.size() > 1 && input[input.size() - 2] == '=') ++pad;
    for (size_t i = 0; i < input.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(input[i]);
        if (c == '=') { if (i < input.size() - pad) return false; }
        else if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                   (c >= '0' && c <= '9') || c == '+' || c == '/')) return false;
    }
    decoded.resize(input.size() / 4 * 3);
    const int count = EVP_DecodeBlock(decoded.data(),
        reinterpret_cast<const unsigned char*>(input.data()), static_cast<int>(input.size()));
    if (count < 0 || static_cast<size_t>(count) < pad) return false;
    decoded.resize(static_cast<size_t>(count) - pad);
    return true;
}

inline bool VerifyRsaSha256(const std::string& payload,
                           const std::vector<unsigned char>& signature,
                           const char* modulus = kModulus) {
    if (!modulus || !*modulus || signature.empty()) return false;
    BIGNUM* n = nullptr;
    if (!BN_hex2bn(&n, modulus) || !n) return false;
    BIGNUM* e = BN_new();
    if (!e || BN_set_word(e, kExponent) != 1) {
        BN_free(n); BN_free(e); return false;
    }
    RSA* key = RSA_new();
    if (!key) { BN_free(n); BN_free(e); return false; }
    if (RSA_set0_key(key, n, e, nullptr) != 1) {
        BN_free(n); BN_free(e); RSA_free(key); return false;
    }
    // RSA takes ownership of n and e after RSA_set0_key succeeds.
    const size_t expected = static_cast<size_t>(RSA_size(key));
    bool ok = false;
    if (signature.size() == expected && payload.size() <= 8192) {
        unsigned char digest[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(payload.data()), payload.size(), digest);
        ok = RSA_verify(NID_sha256, digest, SHA256_DIGEST_LENGTH,
                        signature.data(), static_cast<unsigned int>(signature.size()), key) == 1;
    }
    RSA_free(key);
    return ok;
}

inline Verdict Check(const std::string& payload, const std::string& signatureB64,
                     const std::string& typedKey, const std::string& deviceUuid,
                     std::time_t now = std::time(nullptr), const char* modulus = kModulus) {
    if (payload.empty() || payload.size() > 8192 || !modulus || !*modulus)
        return {false, "License verifier is not configured", 0};
    std::vector<unsigned char> signature;
    if (!DecodeBase64(signatureB64, signature))
        return {false, "Invalid license signature encoding", 0};
    if (!VerifyRsaSha256(payload, signature, modulus))
        return {false, "License signature verification failed", 0};
    std::string fields[5];
    size_t field = 0;
    for (char ch : payload) {
        if (ch == '|') {
            if (++field >= 5) return {false, "Malformed license token", 0};
        } else {
            fields[field].push_back(ch);
        }
    }
    if (field != 4 || fields[0] != "v1") return {false, "Malformed license token", 0};
    if (fields[1] != typedKey) return {false, "License key mismatch", 0};
    if (fields[2] != deviceUuid) return {false, "License device mismatch", 0};
    if (fields[4] != std::to_string(kBuild)) return {false, "License build mismatch", 0};
    errno = 0;
    char* end = nullptr;
    const long long epoch = std::strtoll(fields[3].c_str(), &end, 10);
    if (errno != 0 || end == fields[3].c_str() || !end || *end != '\0')
        return {false, "Invalid license expiration", 0};
    if (epoch <= static_cast<long long>(now)) return {false, "License expired", 0};
    return {true, "OK", static_cast<std::time_t>(epoch)};
}
} // namespace imgui3_license
