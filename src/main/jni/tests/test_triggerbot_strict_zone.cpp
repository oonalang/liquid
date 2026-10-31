#include "../Features/Triggerbot1657Policy.h"
#include <cassert>
#include <iostream>
using namespace Triggerbot1657;
int main() {
    Evidence e;
    e.enabled=true;
    e.mode=Mode::Both;
    e.weaponReady=true;
    e.localPlayerAlive=true;
    e.validEnemy=true;
    e.depthValid=true;
    e.pointValid=true;
    e.wall=Visibility::Clear;
    e.smoke=Visibility::Clear;
    e.radius=14.f;
    e.targetHalfWidth=90.f;
    e.worldDistance=30.f;
    e.ageMs=10;
    e.crosshair={100,100};
    e.target={115,100};
    assert(!Allowed(e));
    e.target={114,100};
    assert(Allowed(e));
    e.target={100,114.01f};
    assert(!Allowed(e));
    e.target={100,100};
    assert(Allowed(e));
    e.wall=Visibility::Blocked;
    assert(!Allowed(e));
    e.wall=Visibility::Clear;
    e.smoke=Visibility::Blocked;
    assert(!Allowed(e));
    e.smoke=Visibility::Clear;
    e.mode=Mode::AdsOnly;
    assert(!Allowed(e));
    e.isAds=true;
    assert(Allowed(e));
    std::cout << "strict zone tests passed\n";
}
