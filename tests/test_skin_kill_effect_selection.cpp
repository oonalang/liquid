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
}
