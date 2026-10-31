#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include "Triggerbot1657ExportNames.h"
#include "Triggerbot1657Policy.h"

namespace Triggerbot1657 {



inline constexpr const char* kDiagnosticBuildLabel = "1657-r26 / single-trace LOS, instant fire";



enum class DiagnosticStage : unsigned {
    HookNotObserved, Disabled, SnapshotUnavailable, AdsRequired,
    LocalControllerMismatch, LocalPlayerUnavailable, WeaponNotReady,
    NoEnemies, NoDotMatch, WallBlocked, SmokeBlocked,
    EligibilityPassed, FireRequested, FireHeld, FireReleased, FireTapped,
    CameraUnavailable, MatchUnavailable, EnemyListUnavailable,
    RuntimeExportsUnavailable, RuntimeMetadataUnavailable, RuntimeMethodUnavailable,
    RuntimeInvocationException, RuntimeResultUnavailable, RuntimeLibraryUnavailable,

    LinecastPending, SmokePending, LinecastAbiUnavailable, ColliderPending,
    FireModeProbe, FireModeProbed
};
enum class HitKind : unsigned { None, SelectedEnemy, Other, Unknown };
inline std::atomic<unsigned> diagHitKind{0};
inline std::atomic<float> diagHitDistance{-1.f},diagTargetDistance{-1.f};





enum class RayOrigin : unsigned { None, Fire, Eye, Head, Camera };
inline constexpr unsigned kRayOriginCount = 5;
inline const char* RayOriginName(unsigned value) {
    switch (static_cast<RayOrigin>(value)) {
        case RayOrigin::Fire: return "muzzle";
        case RayOrigin::Eye: return "eye";
        case RayOrigin::Head: return "head";
        case RayOrigin::Camera: return "camera";
        default: return "none";
    }
}

inline std::atomic<unsigned> diagHitOrigin{0};



enum class OriginState : unsigned { NotTried, Clear, Blocked, Unavailable };
inline const char* OriginStateName(unsigned value) {
    switch (static_cast<OriginState>(value)) {
        case OriginState::Clear: return "clear";
        case OriginState::Blocked: return "blocked";
        case OriginState::Unavailable: return "unavailable";
        default: return "not tried";
    }
}
inline std::atomic<unsigned> diagOriginState[kRayOriginCount]{};
inline std::atomic<float> diagOriginDistance[kRayOriginCount]{};



inline std::atomic<std::uintptr_t> diagOriginCollider[kRayOriginCount]{};
inline void RecordOrigin(RayOrigin origin,bool clear,float distance,std::uintptr_t collider=0) {
    const auto index=static_cast<unsigned>(origin);
    if (index>=kRayOriginCount) return;
    diagOriginState[index].store(static_cast<unsigned>(
        clear ? OriginState::Clear : OriginState::Blocked),std::memory_order_relaxed);
    diagOriginDistance[index].store(distance,std::memory_order_relaxed);
    diagOriginCollider[index].store(clear ? 0 : collider,std::memory_order_relaxed);
}
inline void MarkOriginUnavailable(RayOrigin origin) {
    const auto index=static_cast<unsigned>(origin);
    if (index>=kRayOriginCount) return;
    diagOriginState[index].store(static_cast<unsigned>(OriginState::Unavailable),
                                 std::memory_order_relaxed);
    diagOriginDistance[index].store(-1.f,std::memory_order_relaxed);
    diagOriginCollider[index].store(0,std::memory_order_relaxed);
}
inline void ResetOriginDiagnostics() {
    for (unsigned i=0;i<kRayOriginCount;++i) {
        diagOriginState[i].store(static_cast<unsigned>(OriginState::NotTried),std::memory_order_relaxed);
        diagOriginDistance[i].store(-1.f,std::memory_order_relaxed);
        diagOriginCollider[i].store(0,std::memory_order_relaxed);
    }
}
inline std::atomic<unsigned> diagSelfSkips{0};


inline std::atomic<unsigned> diagVisibilityProbes{0};


inline std::atomic<unsigned> diagIgnoredTriggerHits{0};


inline std::atomic<unsigned> diagPhysicsCasts{0},diagLegacyPhysicsCasts{0};
inline std::atomic<unsigned> diagPointerValidations{0},diagReusedPointerValidations{0};
inline std::atomic<float> diagLastSampleFraction{-1.f};
inline const char* HitKindName(unsigned value) {
    switch (static_cast<HitKind>(value)) {
        case HitKind::None: return "No collision";
        case HitKind::SelectedEnemy: return "Selected enemy";
        case HitKind::Other: return "Other collider (blocked)";
        default: return "Identity unavailable (blocked)";
    }
}
inline std::atomic<bool> diagFilteredLinecastEnabled{false};
inline std::atomic<unsigned> diagLinecastAbi{0};
inline std::atomic<std::uint32_t> diagLinecastCode[3]{};
inline std::atomic<std::uint32_t> diagMissingExports{0};
inline std::atomic<unsigned> diagStage{static_cast<unsigned>(DiagnosticStage::HookNotObserved)};
inline std::atomic<std::uint64_t> diagUpdateCount{0}, diagFireRequests{0}, diagFireTaps{0},
    diagProbes{0};
// 0 = unclassified/tap, 1 = semi-automatic, 2 = automatic.
inline std::atomic<unsigned> diagFireMode{0};


inline std::atomic<bool> diagProbeSpent{false};
inline const char* FireModeName(unsigned value) {
    switch (static_cast<FireMode>(value)) {
        case FireMode::Single: return "Semi-auto (tap)";
        case FireMode::Auto: return "Automatic (hold)";
        default: return "Unclassified (tap)";
    }
}
inline std::atomic<std::int64_t> diagLastUpdateMs{0};
inline std::atomic<unsigned> diagEnemies{0},diagBots{0},diagDotMatches{0},
    diagWallRejections{0},diagSmokeRejections{0};
inline void Report(DiagnosticStage stage) {
    diagStage.store(static_cast<unsigned>(stage), std::memory_order_relaxed);
}
inline const char* DiagnosticStageName(unsigned value) {
    switch(static_cast<DiagnosticStage>(value)) {
        case DiagnosticStage::HookNotObserved: return "Controller hook has not reported an update";
        case DiagnosticStage::Disabled: return "Triggerbot or dot is disabled";
        case DiagnosticStage::SnapshotUnavailable: return "Dot snapshot / camera / visibility data unavailable";
        case DiagnosticStage::AdsRequired: return "Waiting for ADS (ADS Only mode)";
        case DiagnosticStage::LocalControllerMismatch: return "Controller does not match local pawn";
        case DiagnosticStage::LocalPlayerUnavailable: return "Local pawn not ready or not alive";
        case DiagnosticStage::WeaponNotReady: return "Weapon or required fire API not ready";
        case DiagnosticStage::NoEnemies: return "No valid enemies in current enemy list";
        case DiagnosticStage::NoDotMatch: return "Enemies found, none intersect the firing zone";
        case DiagnosticStage::WallBlocked: return "Candidate rejected by physics linecast";
        case DiagnosticStage::SmokeBlocked: return "Candidate rejected by smoke checks";
        case DiagnosticStage::EligibilityPassed: return "Target passed gating; awaiting fire transition";
        case DiagnosticStage::FireRequested: return "StartFire invoked; live shot is not confirmed";
        case DiagnosticStage::FireHeld: return "StartFire previously invoked; still holding";
        case DiagnosticStage::FireReleased: return "StopFire invoked";
        case DiagnosticStage::FireTapped: return "StopFire invoked for the next semi-auto tap";
        case DiagnosticStage::FireModeProbe: return "Holding fire to probe semi-auto vs automatic";
        case DiagnosticStage::FireModeProbed: return "Fire mode classified from the magazine count";
        case DiagnosticStage::CameraUnavailable: return "Main camera / transform unavailable";
        case DiagnosticStage::MatchUnavailable: return "Current match unavailable";
        case DiagnosticStage::EnemyListUnavailable: return "Current enemy list unavailable or invalid";
        case DiagnosticStage::RuntimeLibraryUnavailable: return "Loaded Unity / IL2CPP library lookup failed; retrying";
        case DiagnosticStage::LinecastAbiUnavailable: return "Linecast wrapper ABI unrecognized; native call blocked";
        case DiagnosticStage::ColliderPending: return "Checking hit collider against the selected enemy";
        case DiagnosticStage::LinecastPending: return "Calling Physics.Linecast for the dot match";
        case DiagnosticStage::SmokePending: return "Line of sight clear; checking smoke volumes";
        case DiagnosticStage::RuntimeExportsUnavailable: return "Required IL2CPP runtime exports unavailable";
        case DiagnosticStage::RuntimeMetadataUnavailable: return "Managed assemblies not available";
        case DiagnosticStage::RuntimeMethodUnavailable: return "Managed collision / controller method unavailable";
        case DiagnosticStage::RuntimeInvocationException: return "Managed collision / controller call raised an exception";
        case DiagnosticStage::RuntimeResultUnavailable: return "Managed collision call returned no valid result";
    }
    return "Unknown diagnostic state";
}
inline std::int64_t NowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
} // namespace Triggerbot1657
