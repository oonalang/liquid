#include <cassert>
#include <cmath>
#include <limits>
#include "Features/Triggerbot1657Policy.h"
using namespace Triggerbot1657;
int main() {
    Evidence e{};
    assert(!Allowed(e));
    e.enabled=e.weaponReady=e.localPlayerAlive=e.validEnemy=e.depthValid=e.pointValid=true;
    assert(!ModeAllows(Mode::AdsOnly,false));
    assert(ModeAllows(Mode::AdsOnly,true));
    assert(ModeAllows(Mode::Both,false));
    assert(ModeAllows(Mode::Both,true));
    e.mode=Mode::Both;
    e.wall=e.smoke=Visibility::Clear;
    e.crosshair={150.f,200.f}; e.target={150.f,200.f};e.radius=10.f;e.ageMs=10;
    assert(Allowed(e));
    e.mode=Mode::AdsOnly; e.isAds=false; assert(!Allowed(e));
    e.isAds=true; assert(Allowed(e)); e.mode=Mode::Both; e.isAds=false;
    e.wall=Visibility::Unknown;assert(!Allowed(e));e.wall=Visibility::Blocked;assert(!Allowed(e));e.wall=Visibility::Clear;
    e.smoke=Visibility::Unknown;assert(!Allowed(e));e.smoke=Visibility::Blocked;assert(!Allowed(e));e.smoke=Visibility::Clear;
    e.target={162.f,200.f};assert(!Allowed(e));e.target={159.f,200.f};assert(Allowed(e));
    e.ageMs=351;assert(!Allowed(e));e.ageMs=-1;assert(!Allowed(e));e.ageMs=20;
    e.pointValid=false;assert(!Allowed(e));e.pointValid=true;
    e.weaponReady=false;assert(!Allowed(e));e.weaponReady=true;
    e.validEnemy=false;assert(!Allowed(e));e.validEnemy=true;
    e.localPlayerAlive=false;assert(!Allowed(e));e.localPlayerAlive=true;
    e.target.x=std::numeric_limits<float>::quiet_NaN();assert(!Allowed(e));
    e.target={150,200};e.enabled=false;assert(!Allowed(e));
    assert(WithinRange(0.f));
    assert(WithinRange(5000.f));

    assert(WithinRange(5000.01f));
    assert(WithinRange(1.0e9f));
    assert(!WithinRange(-1.f));
    assert(!WithinRange(std::numeric_limits<float>::infinity()));
    assert(!WithinRange(std::numeric_limits<float>::quiet_NaN()));

    auto close=ClosestOnBodySegment({150,220},{150,50},{150,350});
    assert(close.valid && close.fraction>0.5f && close.fraction<0.6f);
    e.enabled=true;e.radius=5;e.worldDistance=2.0f;e.target=close.point;
    e.targetHalfWidth=close.halfWidth;e.ageMs=100;e.wall=e.smoke=Visibility::Clear;
    assert(!Allowed(e));
    e.crosshair={150.f,220.f};assert(Allowed(e));
    e.crosshair={150.f,200.f};

    auto distant=ClosestOnBodySegment({150,200},{150,197},{150,203});
    assert(distant.valid);
    e.target=distant.point;e.targetHalfWidth=distant.halfWidth;e.worldDistance=5000.f;
    assert(Allowed(e));
    e.worldDistance=5000.1f;assert(Allowed(e));
    e.worldDistance=1.0e9f;assert(Allowed(e));
    auto offScreen=ClosestOnBodySegment({150,200},{350,100},{350,300});
    e.worldDistance=15;e.target=offScreen.point;e.targetHalfWidth=offScreen.halfWidth;
    assert(!Allowed(e));
    auto subpixel=ClosestOnBodySegment({20,20},{20,20},{20,20});
    assert(subpixel.valid && std::abs(subpixel.halfWidth-.75f)<.01f);
    assert(!ClosestOnBodySegment({0,0},{NAN,20},{20,20}).valid);


    assert(SkipSelfHit(0,0.f,0.66f));
    assert(SkipSelfHit(1,0.71f,0.66f));
    // The whole first-person model (body, arms, weapon) must be passable.
    assert(SkipSelfHit(5,4.26f,0.66f));
    assert(!SkipSelfHit(6,4.97f,0.66f));
    assert(!SkipSelfHit(0,0.f,-1.f));
    assert(!SkipSelfHit(0,NAN,0.5f));
    assert(!SkipSelfHit(0,4.9f,0.66f));

    // A cast that starts on/inside low cover reports a ~zero-distance hit.
    assert(EmbeddedHit(0.f));
    assert(EmbeddedHit(kEmbeddedHitDistance));
    assert(!EmbeddedHit(kEmbeddedHitDistance+0.01f));
    assert(!EmbeddedHit(-1.f));
    assert(!EmbeddedHit(std::numeric_limits<float>::quiet_NaN()));


    assert(OriginOffsetSane(0.f));
    assert(OriginOffsetSane(16.f));   // exactly the 4 m bound
    assert(!OriginOffsetSane(16.01f));
    assert(!OriginOffsetSane(-1.f));
    assert(!OriginOffsetSane(std::numeric_limits<float>::infinity()));
    assert(!OriginOffsetSane(std::numeric_limits<float>::quiet_NaN()));


    assert(kMaxProbeAttempts>=1);

    assert(ClassifyFireMode(30,27,24)==FireMode::Auto);    // still consuming late
    assert(ClassifyFireMode(30,29,28)==FireMode::Auto);    // any late decrement
    assert(ClassifyFireMode(30,29,29)==FireMode::Single);  // one press, one round
    assert(ClassifyFireMode(30,27,27)==FireMode::Single);  // fixed burst -> tap
    assert(ClassifyFireMode(30,30,30)==FireMode::Unknown); // blocked/reload/wrong source
    assert(ClassifyFireMode(-1,29,29)==FireMode::Unknown);
    assert(ClassifyFireMode(30,-1,29)==FireMode::Unknown);
    assert(ClassifyFireMode(30,29,-1)==FireMode::Unknown);
}
