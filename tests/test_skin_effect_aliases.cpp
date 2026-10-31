#include <cassert>
#include <unordered_map>
#include <vector>
#include "Hacks/Feature/SkinEffectAliases.h"

int main() {
    using OblivionSkinHax::ApplyEffectAliases;
    std::unordered_map<int, int> effects;
    const std::vector<int> mythic{101, 201, 301};
    const std::vector<int> legendary{102, 202, 302};
    ApplyEffectAliases(effects, mythic, 7101);
    ApplyEffectAliases(effects, legendary, 7202);
    assert(effects.size() == 6);
    for (int id : mythic) assert(effects.at(id) == 7101);
    for (int id : legendary) assert(effects.at(id) == 7202);
    ApplyEffectAliases(effects, legendary, 7303);
    for (int id : mythic) assert(effects.at(id) == 7101);
    for (int id : legendary) assert(effects.at(id) == 7303);
    ApplyEffectAliases(effects, legendary, 0);
    assert(effects.size() == 3);
    for (int id : mythic) assert(effects.at(id) == 7101);
    ApplyEffectAliases(effects, std::vector<int>{0, -1}, 9999);
    assert(effects.size() == 3);
    return 0;
}
