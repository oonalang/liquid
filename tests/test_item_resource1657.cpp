#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <unordered_map>

#include "Hacks/Feature/ItemResource1657.h"

template <typename T, size_t N>
static T Read(const std::array<unsigned char, N>& bytes, size_t offset) {
    T value;
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}

template <typename T, size_t N>
static void Write(std::array<unsigned char, N>& bytes, size_t offset, T value) {
    std::memcpy(bytes.data() + offset, &value, sizeof(value));
}

int main() {


    alignas(ItemResourceFields) std::array<unsigned char, 0x40> row;
    row.fill(0xA5);
    Write<uint16_t>(row, 0x10, 11);
    Write<uint16_t>(row, 0x12, 22);
    Write<uint16_t>(row, 0x14, 33);
    Write<uint16_t>(row, 0x16, 44);
    Write<int32_t>(row, 0x18, 101);
    Write<int32_t>(row, 0x1C, 100100001);
    Write<int32_t>(row, 0x20, 202);
    Write<int32_t>(row, 0x24, 303);
    Write<uint64_t>(row, 0x28, 0x123456789ABCDEF0ULL);

    auto* fields = reinterpret_cast<ItemResourceFields*>(row.data() + 0x10);
    std::unordered_map<int, ItemResourceFields*> byId;
    byId[fields->ID] = fields;
    assert(byId.at(100100001) == fields);
    assert(fields->InventoryModelID == 202 && fields->ModelAssetIDRaw == 303);
    assert(fields->UIMiniSpriteName == 11 && fields->UISmallSpriteName == 22);

    const auto before = row;
    ApplyItemResourceFields(*fields, {404, 505, 606}, {55, 66, 77, 88});
    assert(Read<uint16_t>(row, 0x10) == 55);
    assert(Read<uint16_t>(row, 0x12) == 66);
    assert(Read<uint16_t>(row, 0x14) == 77);
    assert(Read<uint16_t>(row, 0x16) == 88);
    assert(Read<int32_t>(row, 0x18) == 404);
    assert(Read<int32_t>(row, 0x20) == 505);
    assert(Read<int32_t>(row, 0x24) == 606);


    assert(Read<int32_t>(row, 0x1C) == 100100001);
    assert(Read<uint64_t>(row, 0x28) == 0x123456789ABCDEF0ULL);
    for (size_t i = 0; i < 0x10; ++i) assert(row[i] == before[i]);
    for (size_t i = 0x30; i < row.size(); ++i) assert(row[i] == before[i]);
}
