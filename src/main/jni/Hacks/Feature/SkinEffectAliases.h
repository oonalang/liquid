#pragma once

namespace OblivionSkinHax {

template <typename Map, typename Ids>
inline void ApplyEffectAliases(Map& effects, const Ids& aliases, int effectId) {
    for (const int alias : aliases) {
        if (alias <= 0) continue;
        if (effectId > 0)
            effects[alias] = effectId;
        else
            effects.erase(alias);
    }
}

}
