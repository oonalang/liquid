#include "../src/main/jni/Includes/ImGui3SignedSession.h"
#include <cassert>
#include <cstdio>
#include <openssl/evp.h>

static std::string Encode(const std::vector<unsigned char>& bytes) {
    std::string str(((bytes.size()+2)/3)*4, '\0');
    int size = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(&str[0]),
                               bytes.data(), static_cast<int>(bytes.size()));
    str.resize(size);
    return str;
}
int main() {
    BIGNUM* exponent = BN_new();
    assert(exponent && BN_set_word(exponent, imgui3_license::kExponent));
    RSA* rsa = RSA_new();
    assert(rsa && RSA_generate_key_ex(rsa, 2048, exponent, nullptr));
    BN_free(exponent);
    const BIGNUM* modulus = nullptr;
    RSA_get0_key(rsa, &modulus, nullptr, nullptr);
    char* modulusHex = BN_bn2hex(modulus);
    assert(modulusHex);
    const std::string key = "TEST-KEY";
    const std::string uuid = "test-device-uuid";
    const std::time_t now = 1800000000;
    const std::string payload = "v1|" + key + "|" + uuid + "|1802500000|1";
    auto sign = [&](const std::string& msg) {
        unsigned char digest[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(msg.data()), msg.size(), digest);
        std::vector<unsigned char> sig(RSA_size(rsa));
        unsigned int length = 0;
        assert(RSA_sign(NID_sha256, digest, SHA256_DIGEST_LENGTH,
                        sig.data(), &length, rsa) == 1);
        sig.resize(length);
        return Encode(sig);
    };
    const std::string signature = sign(payload);
    using imgui3_license::Check;
    assert(Check(payload, signature, key, uuid, now, modulusHex).ok);
    assert(!Check(payload, signature, "WRONG", uuid, now, modulusHex).ok);
    assert(!Check(payload, signature, key, "WRONG", now, modulusHex).ok);
    assert(!Check(payload, signature, key, uuid, 1803000000, modulusHex).ok);
    assert(!Check(payload, signature, key, uuid, now).ok); // Untrusted test signer.
    assert(!Check(payload+"changed", signature, key, uuid, now, modulusHex).ok);
    assert(!Check(payload, "%%%", key, uuid, now, modulusHex).ok);
    assert(!Check("v1|"+key+"|"+uuid+"|1802500000|2",
                  sign("v1|"+key+"|"+uuid+"|1802500000|2"),
                  key, uuid, now, modulusHex).ok);
    assert(!Check(payload, signature, key, uuid, now, "").ok);
    OPENSSL_free(modulusHex);
    RSA_free(rsa);
    std::puts("PASSED: signed session verification (9 assertions)");
}
