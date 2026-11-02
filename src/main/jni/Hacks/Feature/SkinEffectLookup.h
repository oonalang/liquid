#pragma once
#include <array>
#include <unordered_map>

namespace SkinEffectLookup {
inline int Find(const std::unordered_map<int,int>& values,
                const std::array<int,3>& ids) {
    for (const int id : ids) {
        if (id<=0) continue;
        const auto found=values.find(id);
        if (found!=values.end() && found->second>0) return found->second;
    }
    return 0;
}
}
