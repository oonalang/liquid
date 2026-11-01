#pragma once






#include <atomic>
#include <chrono>
#include <cstdint>

static_assert(sizeof(Vector2) == 8, "Expected IL2CPP Unity Vector2 layout");

#include "Gameplay1657Settings.h"
namespace Gameplay1657 {

inline float (*origSlideSpeedDownPct)(void*) = nullptr;
inline bool (*origCanCrouch)(void*) = nullptr;
inline Vector2 (*origBreatheTremblingPos)(void*) = nullptr;
inline float (*origDelaySprintFire)(void*) = nullptr;
inline bool (*fnPawnIsAiming)(void*) = nullptr;
inline bool (*fnInventoryIsSniper)(void*) = nullptr;

inline void InitializeApiPointers(std::uintptr_t unityBase) {
    if (!unityBase) return;
    fnPawnIsAiming = reinterpret_cast<bool (*)(void*)>(unityBase + 0x519BB10);
    fnInventoryIsSniper = reinterpret_cast<bool (*)(void*)>(unityBase + 0xC364C94);
}


inline bool IsLocalSlideComponent(const void* self) {
    return self && *(reinterpret_cast<const std::uint8_t*>(self) + 0x38) != 0;
}

inline bool IsLocalPawn(const void* self) {
    return self && m_unity && GamePlay::get_LocalPawn() == self;
}

inline bool IsLocalEquippedWeapon(const void* self) {
    if (!self || !m_unity) return false;
    Pawn* pawn = GamePlay::get_LocalPawn();
    return pawn && pawn->get_CurrentWeapon() == self;
}



inline bool IsLocalAimingSniperComponent(const void* self) {
    if (!self || !m_unity) return false;
    Pawn* local = GamePlay::get_LocalPawn();
    if (!local || !fnPawnIsAiming || !fnInventoryIsSniper) return false;
    auto bytes = reinterpret_cast<const std::uint8_t*>(self);
    const void* owner = *reinterpret_cast<void* const*>(bytes + 0x28);
    const void* weapon = *reinterpret_cast<void* const*>(bytes + 0x10);
    if (owner != local || weapon == nullptr || weapon != local->get_CurrentWeapon())
        return false;

    return fnPawnIsAiming(const_cast<void*>(owner)) &&
           fnInventoryIsSniper(const_cast<void*>(weapon));
}

inline float HookSlideSpeedDownPct(void* self) {
    // Metadata does not establish whether this is a penalty or a multiplier.
    // Preserve the game's speed curve instead of forcing an unverified zero.
    return origSlideSpeedDownPct ? origSlideSpeedDownPct(self) : 0.0f;
}

// Keep the routine's initialization and recovery work. Only suppress its two
// metadata-defined decrease rates for the local call; restore configuration so
// disabling the option does not leave persistent movement changes.
inline void (*origLimitSlideTackle)(void*) = nullptr;
inline void HookLimitSlideTackle(void* self) {
    if (!origLimitSlideTackle) return;
    if (!noSlideFatigue.load(std::memory_order_relaxed) || !IsLocalSlideComponent(self)) {
        origLimitSlideTackle(self);
        return;
    }
    auto* bytes = reinterpret_cast<std::uint8_t*>(self);
    auto& speedDecrease = *reinterpret_cast<float*>(bytes + 0x48);
    auto& idleDecrease = *reinterpret_cast<float*>(bytes + 0x4C);
    const float savedSpeed = speedDecrease, savedIdle = idleDecrease;
    speedDecrease = idleDecrease = 0.0f;
    origLimitSlideTackle(self);
    speedDecrease = savedSpeed;
    idleDecrease = savedIdle;
}

inline bool HookCanCrouch(void* self) {
    if (noCrouch.load(std::memory_order_relaxed) && IsLocalPawn(self))
        return false;
    return origCanCrouch ? origCanCrouch(self) : false;
}

inline Vector2 HookBreatheTremblingPos(void* self) {
    if (noAimDrift.load(std::memory_order_relaxed) && IsLocalAimingSniperComponent(self))
        return Vector2(0.0f, 0.0f);
    return origBreatheTremblingPos ? origBreatheTremblingPos(self) : Vector2(0.0f, 0.0f);
}

// The weapon consumes this value as the length of its sprint-to-fire raise. Returning
// exactly zero makes that raise take no time at all: the game starts firing on the
// frame the sprint ends, while the fire component is still being re-enabled, and any
// "rate = something / delay" step turns into inf/NaN. Cap it at one hand-off instead:
// imperceptible, but the game still runs its own transition.
inline constexpr float kMinSprintToFireDelay = 0.05f;
// A weapon created a moment ago has no completed raise to shorten. That is the state
// right after a respawn or a round change, and enabling the option while dead put this
// hook exactly there on the next spawn. Keep the game's own delay until the same pawn
// and weapon have been local for this long, then cap.
inline void* sprintFirePawn = nullptr;
inline void* sprintFireWeapon = nullptr;
inline std::int64_t sprintFireChangedMs = 0;
inline std::int64_t sprintFireSettleMs = 400;

inline std::int64_t SprintFireNowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

inline float HookDelaySprintFire(void* self) {
    Pawn* pawn = (self && m_unity) ? GamePlay::get_LocalPawn() : nullptr;
    if (!noSprintToFireDelay.load(std::memory_order_relaxed) || !pawn ||
        pawn->get_CurrentWeapon() != self)
        return origDelaySprintFire ? origDelaySprintFire(self) : 0.0f;

    const std::int64_t now = SprintFireNowMs();
    if (pawn != sprintFirePawn || self != sprintFireWeapon) {
        sprintFirePawn = pawn;
        sprintFireWeapon = self;
        sprintFireChangedMs = now;
    }
    if (now - sprintFireChangedMs < sprintFireSettleMs)
        return origDelaySprintFire ? origDelaySprintFire(self) : 0.0f;

    const float original = origDelaySprintFire ? origDelaySprintFire(self) : 0.0f;
    if (original <= 0.0f) return original;                       // already instant: leave it
    return original < kMinSprintToFireDelay ? original : kMinSprintToFireDelay;
}

inline void (*origWeaponHitFlinch)(void*, float) = nullptr;
inline void (*origFireComponentHitFlinch)(void*, float) = nullptr;
inline void (*origInstantHitFlinch)(void*, float) = nullptr;
inline void (*origProjectileHitFlinch)(void*, float) = nullptr;

inline bool IsLocalFireComponent(const void* self) {
    if (!self || !m_unity) return false;
    const auto* local = GamePlay::get_LocalPawn();
    if (!local) return false;
    auto* owner = *reinterpret_cast<void* const*>(
        reinterpret_cast<const std::uint8_t*>(self) + 0x28);
    return owner == local;
}

inline void HookWeaponHitFlinch(void* self, float multiplier) {
    if (noHitFlinch.load(std::memory_order_relaxed) && IsLocalEquippedWeapon(self)) return;
    if (origWeaponHitFlinch) origWeaponHitFlinch(self,multiplier);
}
inline void HookFireComponentHitFlinch(void* self, float multiplier) {
    if (noHitFlinch.load(std::memory_order_relaxed) && IsLocalFireComponent(self)) return;
    if (origFireComponentHitFlinch) origFireComponentHitFlinch(self,multiplier);
}
inline void HookInstantHitFlinch(void* self, float multiplier) {
    if (noHitFlinch.load(std::memory_order_relaxed) && IsLocalFireComponent(self)) return;
    if (origInstantHitFlinch) origInstantHitFlinch(self,multiplier);
}
inline void HookProjectileHitFlinch(void* self, float multiplier) {
    if (noHitFlinch.load(std::memory_order_relaxed) && IsLocalFireComponent(self)) return;
    if (origProjectileHitFlinch) origProjectileHitFlinch(self,multiplier);
}
} // namespace Gameplay1657
