#pragma once
#include <array>
#include <cstdint>

namespace EspBoxGuard1657 {
struct Scene {
    std::uintptr_t match=0, local=0, list=0, camera=0;
    bool operator==(const Scene& other) const {
        return match==other.match && local==other.local &&
               list==other.list && camera==other.camera;
    }
};
struct TrackedBones {
    std::uintptr_t pawn=0, head=0, mesh=0;
    std::int64_t firstMs=0, lastMs=0;
};
struct Tracker {
    static constexpr unsigned kCapacity=128;
    Scene scene{};
    bool hasScene=false;
    std::array<TrackedBones,kCapacity> entries{};
    unsigned count=0;
    std::int64_t geometrySettleMs=32;
    std::int64_t forgetMs=1000;

    void Reset() {
        scene={}; hasScene=false; entries={}; count=0;
    }
    // The first frame of a new scene is never safe for a pawn read.
    bool Observe(Scene candidate) {
        if (!candidate.match || !candidate.local || !candidate.list || !candidate.camera) {
            Reset(); return false;
        }
        if (!hasScene || !(scene==candidate)) {
            scene=candidate; hasScene=true; entries={}; count=0; return false;
        }
        return true;
    }
    // A pawn may keep its address while the head and mesh objects are rebuilt.
    bool GeometryStable(std::uintptr_t pawn,std::uintptr_t head,std::uintptr_t mesh,
                        std::int64_t nowMs) {
        if (!hasScene || !pawn || !head || !mesh) return false;
        for (unsigned i=0;i<count;++i) {
            auto& entry=entries[i];
            if (entry.pawn!=pawn) continue;
            if (nowMs<entry.lastMs || nowMs-entry.lastMs>forgetMs ||
                entry.head!=head || entry.mesh!=mesh) {
                entry={pawn,head,mesh,nowMs,nowMs};
                return false;
            }
            entry.lastMs=nowMs;
            return nowMs>=entry.firstMs && nowMs-entry.firstMs>=geometrySettleMs;
        }
        unsigned keep=0;
        for (unsigned i=0;i<count;++i) {
            if (nowMs>=entries[i].lastMs && nowMs-entries[i].lastMs<=forgetMs)
                entries[keep++]=entries[i];
        }
        count=keep;
        if (count>=kCapacity) return false;
        entries[count++]={pawn,head,mesh,nowMs,nowMs};
        return false;
    }
};
}
