#pragma once

#include <array>
#include <cstddef>
#include <cstdint>



struct ItemResourceFields {
    uint16_t UIMiniSpriteName;   // instance + 0x10
    uint16_t UISmallSpriteName;  // instance + 0x12
    uint16_t UISpriteName;       // instance + 0x14
    uint16_t UISquareSpriteName; // instance + 0x16
    int32_t FxAssetID;          // instance + 0x18
    int32_t ID;                 // instance + 0x1C
    int32_t InventoryModelID;   // instance + 0x20
    int32_t ModelAssetIDRaw;    // instance + 0x24
    uint64_t AvatarModelID;     // instance + 0x28
};

static_assert(offsetof(ItemResourceFields, FxAssetID) == 0x08);
static_assert(offsetof(ItemResourceFields, ID) == 0x0C);
static_assert(offsetof(ItemResourceFields, InventoryModelID) == 0x10);
static_assert(offsetof(ItemResourceFields, ModelAssetIDRaw) == 0x14);
static_assert(offsetof(ItemResourceFields, AvatarModelID) == 0x18);
static_assert(sizeof(ItemResourceFields) == 0x20);

inline void ApplyItemResourceFields(
    ItemResourceFields& fields,
    const std::array<int, 3>& models,
    const std::array<uint16_t, 4>& sprites) {
    fields.FxAssetID = models[0];
    fields.InventoryModelID = models[1];
    fields.ModelAssetIDRaw = models[2];
    fields.UIMiniSpriteName = sprites[0];
    fields.UISmallSpriteName = sprites[1];
    fields.UISpriteName = sprites[2];
    fields.UISquareSpriteName = sprites[3];
}
