#pragma once

namespace OblivionSkinHax {

inline int SelectKillEffectId(int configuredId, int matchingItemId) {
    if (configuredId > 0) return configuredId;
    return matchingItemId > 0 ? matchingItemId : 0;
}

}
