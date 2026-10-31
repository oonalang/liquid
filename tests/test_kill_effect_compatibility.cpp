#include <cassert>
#include <unordered_map>
#include <vector>
#include "Hacks/Feature/KillEffectCompatibility.h"

int main() {
    using OblivionSkinHax::IndexKillEffectCompatibility;
    std::unordered_map<int, int> byWeapon;
    const std::vector<int> items{201, 202, 203};
    IndexKillEffectCompatibility(byWeapon, 7001, static_cast<int>(items.size()),
        [&](int i) { return items.at(i); });
    assert(byWeapon.size() == 3);
    assert(byWeapon.at(201) == 7001 && byWeapon.at(202) == 7001 && byWeapon.at(203) == 7001);
    IndexKillEffectCompatibility(byWeapon, 7002, 0, [&](int) -> int { assert(false); return 0; });
    IndexKillEffectCompatibility(byWeapon, 7002, 257, [&](int) -> int { assert(false); return 0; });
    assert(byWeapon.size() == 3);
    const std::vector<int> others{202, 0, -1, 204};
    IndexKillEffectCompatibility(byWeapon, 7002, static_cast<int>(others.size()),
        [&](int i) { return others.at(i); });
    assert(byWeapon.at(201) == 7001 && byWeapon.at(202) == 7002 && byWeapon.at(204) == 7002);
}
