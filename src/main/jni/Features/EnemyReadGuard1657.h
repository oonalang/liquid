#pragma once
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace EnemyReadGuard1657 {
// A pawn the game is tearing down - the round flip, a respawn, a player leaving - stays
// in EnemyPawns for a frame or two while the bones, mesh and PlayerInfo behind it are
// already freed. Tools::IsPtrValid only proves that the page is mapped and writable, so
// every one of those stale addresses passes it. The only warning available before the
// first read is time: a pawn that was not in the list on the previous frame is not read
// on this one, and a pawn that comes back was re-created and starts a fresh window.
constexpr int kSlots = 128;
struct Entry {
    std::uintptr_t pawn = 0;
    std::int64_t firstMs = 0;
    std::int64_t lastMs = 0;
};

inline std::array<Entry, kSlots> entries{};
inline unsigned entryCount = 0;
inline std::int64_t settleMs = 300;
inline std::int64_t forgetMs = 1000;

inline std::int64_t NowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

inline bool Settled(std::uintptr_t pawn, std::int64_t now) {
    for (unsigned i = 0; i < entryCount; ++i) {
        if (entries[i].pawn != pawn) continue;
        entries[i].lastMs = now;
        return settleMs <= 0 || now - entries[i].firstMs >= settleMs;
    }
    unsigned keep = 0;
    for (unsigned i = 0; i < entryCount; ++i)
        if (now - entries[i].lastMs <= forgetMs) entries[keep++] = entries[i];
    entryCount = keep;
    if (entryCount < static_cast<unsigned>(kSlots)) entries[entryCount++] = Entry{pawn, now, now};
    return settleMs <= 0;
}

inline void Reset() {
    entries = {};
    entryCount = 0;
}

// IsAlive and IsBot are written as 0 or 1. Anything else is a torn read or a reused
// object, and that pawn's bones are not worth reading next.
inline bool ReadFlag(std::uintptr_t base, std::size_t offset, bool& value) {
    const std::uint8_t raw = *reinterpret_cast<const std::uint8_t*>(base + offset);
    if (raw > 1) return false;
    value = raw != 0;
    return true;
}
}
