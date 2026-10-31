#include "../src/main/jni/UI/RememberedLicense.h"
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
int main() {
    std::vector<uint8_t> iv(12, 42);
    std::vector<uint8_t> cipher(32, 77);
    auto packed = RememberedLicense::Pack(iv, cipher);
    assert(packed.size() == 45);
    std::vector<uint8_t> outIv, outCipher;
    assert(RememberedLicense::Unpack(packed, outIv, outCipher));
    assert(outIv == iv && outCipher == cipher);
    packed[0] = 2;
    assert(!RememberedLicense::Unpack(packed, outIv, outCipher));
    packed[0] = 1;
    packed.pop_back();
    packed.resize(28);
    assert(!RememberedLicense::Unpack(packed, outIv, outCipher));
    assert(RememberedLicense::Pack(std::vector<uint8_t>(11), cipher).empty());
    assert(RememberedLicense::Pack(iv, std::vector<uint8_t>(15)).empty());
    assert(RememberedLicense::Pack(iv, std::vector<uint8_t>(4097)).empty());
    assert(!RememberedLicense::Unpack(std::vector<uint8_t>(4110),outIv,outCipher));
}
