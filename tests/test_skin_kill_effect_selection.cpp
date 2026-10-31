#include <cassert>
#include "Hacks/Feature/KillEffectSelection.h"

int main() {
    using OblivionSkinHax::SelectKillEffectId;
    assert(SelectKillEffectId(401, 9001) == 401);
    assert(SelectKillEffectId(0, 9001) == 9001);
    assert(SelectKillEffectId(-1, 9001) == 9001);
    assert(SelectKillEffectId(401, 0) == 401);
    assert(SelectKillEffectId(0, 0) == 0);
    assert(SelectKillEffectId(-1, -2) == 0);

    using OblivionSkinHax::ResolveKillEffectId;
    // The skin's own config wins, then the base weapon's config (the usual place for
    // a mythic kill effect), then the compatibility table.
    assert(ResolveKillEffectId(401, 402, 9001) == 401);
    assert(ResolveKillEffectId(0, 402, 9001) == 402);
    assert(ResolveKillEffectId(-1, 402, 9001) == 402);
    assert(ResolveKillEffectId(0, 0, 9001) == 9001);
    assert(ResolveKillEffectId(0, -1, 9001) == 9001);
    assert(ResolveKillEffectId(0, 0, 0) == 0);
    assert(ResolveKillEffectId(-1, -1, -1) == 0);
}
