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
static int limitCalls = 0;
static float seenSpeedDecrease = -1, seenIdleDecrease = -1;
static void originalLimit(void* self) {
    ++limitCalls;
    auto* bytes = static_cast<unsigned char*>(self);
    std::memcpy(&seenSpeedDecrease, bytes + 0x48, sizeof(float));
    std::memcpy(&seenIdleDecrease, bytes + 0x4C, sizeof(float));
    // Simulate initialization performed by the original routine.
    bytes[0x54] = 1;
}
static Vector2 originalSway(void*) {return Vector2(0.3f, 0.7f);}
static bool testAiming = true;
static bool testSniper = true;
static bool mockedAim(void* p) { return testAiming && p == GamePlay::local; }
static bool mockedSniper(void* p) { return testSniper && p == GamePlay::local->current; }
int main() {
    using namespace Gameplay1657;
    origCanCrouch = originalBool;
    origSlideSpeedDownPct = originalFloat;
    origLimitSlideTackle = originalLimit;
    origDelaySprintFire = originalFloat;
    origBreatheTremblingPos = originalSway;
    fnPawnIsAiming = mockedAim;
    fnInventoryIsSniper = mockedSniper;
    Pawn mine, enemy;
    Weapon sniper, other;
    mine.current = &sniper;
    enemy.current = &other;
    GamePlay::local = &mine;
    alignas(8) unsigned char slide[0x60]{};
    slide[0x38] = 1;
    const float speedRate = 0.25f, idleRate = 0.5f;
    std::memcpy(slide + 0x48, &speedRate, sizeof(float));
    std::memcpy(slide + 0x4C, &idleRate, sizeof(float));
    alignas(8) unsigned char enemySlide[0x60]{};
    alignas(8) unsigned char fireComponent[0x38]{};
    void* minePtr = &mine;
    void* weaponPtr = &sniper;
    std::memcpy(fireComponent + 0x28, &minePtr, sizeof(void*));
    std::memcpy(fireComponent + 0x10, &weaponPtr, sizeof(void*));
    assert(HookCanCrouch(&mine));
    assert(HookSlideSpeedDownPct(slide) == 2.4f);
    HookLimitSlideTackle(slide);
    assert(limitCalls == 1);
    assert(HookBreatheTremblingPos(fireComponent).x == 0.3f);
    assert(HookDelaySprintFire(&sniper) == 2.4f);
    uiNoCrouch = uiNoSlideFatigue = uiNoAimDrift = uiNoSprintToFireDelay = true;
    SyncFromUi();
    assert(!HookCanCrouch(&mine) && HookCanCrouch(&enemy));
    assert(HookSlideSpeedDownPct(slide) == 2.4f);
    assert(HookSlideSpeedDownPct(enemySlide) == 2.4f);
    slide[0x54] = 0;
    HookLimitSlideTackle(slide);
    assert(limitCalls == 2 && slide[0x54] == 1);
    assert(seenSpeedDecrease == 0.0f && seenIdleDecrease == 0.0f);
    float restoredSpeed, restoredIdle;
    std::memcpy(&restoredSpeed, slide + 0x48, sizeof(float));
    std::memcpy(&restoredIdle, slide + 0x4C, sizeof(float));
    assert(restoredSpeed == speedRate && restoredIdle == idleRate);
    HookLimitSlideTackle(enemySlide);
    assert(limitCalls == 3); // remote players keep their limit
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
    HookLimitSlideTackle(slide);
    assert(limitCalls == 4); // restored
    assert(seenSpeedDecrease == speedRate && seenIdleDecrease == idleRate);
    assert(HookDelaySprintFire(&sniper) == 2.4f);
    puts("PASS: CODM 1.6.57 hook policy tests (off/on, local/remote, restoration)");
}
