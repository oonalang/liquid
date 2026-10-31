#pragma once

namespace OblivionSkinHax {

template <typename Map, typename ReadAt>
inline void IndexKillEffectCompatibility(Map& byWeaponId, int itemId,
                                          int length, ReadAt readAt) {
    if (itemId <= 0 || length <= 0 || length > 256) return;
    for (int index = 0; index < length; ++index) {
        const int weaponId = readAt(index);
        if (weaponId > 0) byWeaponId[weaponId] = itemId;
    }
}

}
