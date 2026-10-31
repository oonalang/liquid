#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <atomic>

struct Vector2 { float x,y; Vector2(float a,float b):x(a),y(b) {} };
struct Weapon {};
struct Pawn { Weapon* current = nullptr; Weapon* get_CurrentWeapon() { return current; } };
struct GamePlay {
    static Pawn* local;
    static Pawn* get_LocalPawn() { return local; }
};
Pawn* GamePlay::local = nullptr;
std::uintptr_t m_unity = 1;
#include "../src/main/jni/Features/Gameplay1657.h"
static float originalFloat(void*) {return 2.4f;}
static bool originalBool(void*) {return true;}
static Vector2 originalSway(void*) {return Vector2(0.3f, 0.7f);}
static bool testAiming = true;
static bool testSniper = true;
static bool mockedAim(void* p) { return testAiming && p == GamePlay::local; }
static bool mockedSniper(void* p) { return testSniper && p == GamePlay::local->current; }
int main() {
    using namespace Gameplay1657;
    origCanCrouch = originalBool;
    origSlideSpeedDownPct = originalFloat;
    origDelaySprintFire = originalFloat;
    origBreatheTremblingPos = originalSway;
    fnPawnIsAiming = mockedAim;
    fnInventoryIsSniper = mockedSniper;
    Pawn mine, enemy;
    Weapon sniper, other;
    mine.current = &sniper;
    enemy.current = &other;
    GamePlay::local = &mine;
    alignas(8) unsigned char slide[0x40]{};
    slide[0x38] = 1;
    alignas(8) unsigned char enemySlide[0x40]{};
    alignas(8) unsigned char fireComponent[0x38]{};
    void* minePtr = &mine;
    void* weaponPtr = &sniper;
    std::memcpy(fireComponent + 0x28, &minePtr, sizeof(void*));
    std::memcpy(fireComponent + 0x10, &weaponPtr, sizeof(void*));
    assert(HookCanCrouch(&mine));
    assert(HookSlideSpeedDownPct(slide) == 2.4f);
    assert(HookBreatheTremblingPos(fireComponent).x == 0.3f);
    assert(HookDelaySprintFire(&sniper) == 2.4f);
    uiNoCrouch = uiNoSlideFatigue = uiNoAimDrift = uiNoSprintToFireDelay = true;
    SyncFromUi();
    assert(!HookCanCrouch(&mine) && HookCanCrouch(&enemy));
    assert(HookSlideSpeedDownPct(slide) == 0.0f);
    assert(HookSlideSpeedDownPct(enemySlide) == 2.4f);
    assert(HookDelaySprintFire(&sniper) == 0.0f && HookDelaySprintFire(&other) == 2.4f);
    assert(HookBreatheTremblingPos(fireComponent).x == 0.0f);
    testAiming = false;
    assert(HookBreatheTremblingPos(fireComponent).x == 0.3f);
    testAiming = true;
    testSniper = false;
    assert(HookBreatheTremblingPos(fireComponent).x == 0.3f);
    testSniper = true;

    void* enemyPtr = &enemy;
    std::memcpy(fireComponent+0x28, &enemyPtr, sizeof(void*));
    assert(HookBreatheTremblingPos(fireComponent).x == 0.3f);
    uiNoCrouch = uiNoSlideFatigue = uiNoAimDrift = uiNoSprintToFireDelay = false;
    SyncFromUi();
    assert(HookCanCrouch(&mine));
    assert(HookSlideSpeedDownPct(slide) == 2.4f);
    assert(HookDelaySprintFire(&sniper) == 2.4f);
    puts("PASS: CODM 1.6.57 hook policy tests (off/on, local/remote, restoration)");
}
