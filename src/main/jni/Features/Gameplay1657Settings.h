#pragma once


#include <atomic>
namespace Gameplay1657 {
inline std::atomic<bool> noSlideFatigue{false};
inline bool uiNoSlideFatigue = false;
inline std::atomic<bool> noCrouch{false};
inline bool uiNoCrouch = false;
inline std::atomic<bool> noAimDrift{false};
inline bool uiNoAimDrift = false;
inline std::atomic<bool> noHitFlinch{false};
inline bool uiNoHitFlinch = false;
inline std::atomic<bool> noSprintToFireDelay{false};
inline bool uiNoSprintToFireDelay = false;
inline void SyncFromUi() {
    noSlideFatigue.store(uiNoSlideFatigue, std::memory_order_relaxed);
    noCrouch.store(uiNoCrouch, std::memory_order_relaxed);
    noAimDrift.store(uiNoAimDrift, std::memory_order_relaxed);
    noHitFlinch.store(uiNoHitFlinch, std::memory_order_relaxed);
    noSprintToFireDelay.store(uiNoSprintToFireDelay, std::memory_order_relaxed);
}
} // namespace Gameplay1657
