#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace EnemyObjectStability1657 {
struct Entry {
    std::uintptr_t owner = 0;
    std::uintptr_t primary = 0;
    std::uintptr_t secondary = 0;
    std::int64_t firstMs = 0;
    std::int64_t lastMs = 0;
};

template<std::size_t Capacity=128>
struct Tracker {
    static_assert(Capacity>0,"Capacity must be nonzero");
    std::array<Entry, Capacity> entries{};
    std::size_t count = 0;
    std::int64_t settleMs = 32;
    std::int64_t forgetMs = 1000;

    void Reset() { entries = {}; count = 0; }
    bool Stable(std::uintptr_t owner,std::uintptr_t primary,std::uintptr_t secondary,
                std::int64_t nowMs) {
        if (!owner || !primary || nowMs < 0) return false;
        for (std::size_t i=0;i<count;++i) {
            Entry& entry=entries[i];
            if (entry.owner!=owner) continue;
            if (nowMs<entry.lastMs || nowMs-entry.lastMs>forgetMs ||
                entry.primary!=primary || entry.secondary!=secondary) {
                entry={owner,primary,secondary,nowMs,nowMs};
                return false;
            }
            entry.lastMs=nowMs;
            return nowMs-entry.firstMs>=settleMs;
        }
        std::size_t keep=0;
        for (std::size_t i=0;i<count;++i) {
            if (nowMs>=entries[i].lastMs && nowMs-entries[i].lastMs<=forgetMs)
                entries[keep++]=entries[i];
        }
        count=keep;
        if (count==Capacity) return false;
        entries[count++]={owner,primary,secondary,nowMs,nowMs};
        return false;
    }
};
}
