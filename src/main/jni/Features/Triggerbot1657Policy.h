#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Triggerbot1657 {
struct Point { float x=0.f, y=0.f; };
enum class Visibility : std::uint8_t { Unknown, Clear, Blocked };
enum class Mode : std::uint8_t { AdsOnly = 0, Both = 1 };




enum class FireMode : std::uint8_t { Unknown, Single, Auto };
inline FireMode ClassifyFireMode(int before, int mid, int after) {
    if (before < 0 || mid < 0 || after < 0) return FireMode::Unknown;

    if (mid > after) return FireMode::Auto;
    const int consumed = before - after;



    if (consumed >= 1) return FireMode::Single;


    return FireMode::Unknown;
}





constexpr unsigned kMaxSelfHitSkips = 2;
constexpr float kMaxSelfHitAdvance = 2.5f;
constexpr float kSelfHitStep = 0.05f;
inline bool SkipSelfHit(unsigned skipped, float advanced, float hitDistance) {
    if (!std::isfinite(advanced) || !std::isfinite(hitDistance) || hitDistance < 0.f) return false;
    if (skipped >= kMaxSelfHitSkips) return false;
    return advanced + hitDistance + kSelfHitStep <= kMaxSelfHitAdvance;
}



constexpr float kMaxOriginOffsetFromPawn = 4.f;
inline bool OriginOffsetSane(float offsetSquared) {
    return std::isfinite(offsetSquared) && offsetSquared >= 0.f &&
        offsetSquared <= kMaxOriginOffsetFromPawn * kMaxOriginOffsetFromPawn;
}




constexpr unsigned kMaxOriginReadFailures = 8;




constexpr unsigned kMaxProbeAttempts = 2;
constexpr std::int64_t kMaximumUiAgeMs = 350;



inline bool WithinRange(float worldDistance) {
    return std::isfinite(worldDistance) && worldDistance >= 0.f;
}
inline bool ModeAllows(Mode mode, bool isAds) {
    return mode == Mode::Both || (mode == Mode::AdsOnly && isAds);
}


struct SegmentHit {
    Point point{};
    float fraction=0.f;
    float halfWidth=0.f;
    bool valid=false;
};
inline SegmentHit ClosestOnBodySegment(Point crosshair, Point head, Point root) {
    if (!std::isfinite(crosshair.x) || !std::isfinite(crosshair.y) ||
        !std::isfinite(head.x) || !std::isfinite(head.y) ||
        !std::isfinite(root.x) || !std::isfinite(root.y)) return {};
    const float dx=root.x-head.x, dy=root.y-head.y;
    const float len2=dx*dx+dy*dy;
    if (!std::isfinite(len2)) return {};
    if (len2 < 0.0001f) return {head,0.5f,0.75f,true};
    const float t=std::clamp(((crosshair.x-head.x)*dx+(crosshair.y-head.y)*dy)/len2,0.f,1.f);
    SegmentHit result;
    result.point={head.x+t*dx,head.y+t*dy};
    result.fraction=t;


    result.halfWidth=std::clamp(std::sqrt(len2)*0.13f,0.75f,90.f);
    result.valid=std::isfinite(result.point.x) && std::isfinite(result.point.y);
    return result;
}
struct Evidence {
    bool enabled=false;
    Mode mode=Mode::AdsOnly;
    bool isAds=false;
    bool weaponReady=false;
    bool localPlayerAlive=false;
    bool validEnemy=false;
    bool depthValid=false;
    bool pointValid=false;
    Visibility wall=Visibility::Unknown;
    Visibility smoke=Visibility::Unknown;
    Point crosshair{};
    Point target{};
    float radius=0.f;
    float targetHalfWidth=0.f;
    float worldDistance=0.f;
    std::int64_t ageMs=INT64_MAX;
};

inline bool Allowed(const Evidence& e) {
    if (!e.enabled || !ModeAllows(e.mode,e.isAds) || !e.weaponReady || !e.localPlayerAlive || !e.validEnemy ||
        !e.depthValid || !e.pointValid || e.wall != Visibility::Clear ||
        e.smoke != Visibility::Clear || e.ageMs < 0 || e.ageMs > kMaximumUiAgeMs || !WithinRange(e.worldDistance) ||
        !std::isfinite(e.crosshair.x) || !std::isfinite(e.crosshair.y) ||
        !std::isfinite(e.target.x) || !std::isfinite(e.target.y) ||
        !std::isfinite(e.radius) || e.radius < 1.f || e.radius > 600.f ||
        !std::isfinite(e.targetHalfWidth) || e.targetHalfWidth < 0.f || e.targetHalfWidth > 90.f)
        return false;
    const float dx=e.target.x-e.crosshair.x, dy=e.target.y-e.crosshair.y;
    return dx*dx + dy*dy <= e.radius*e.radius;
}
} // namespace Triggerbot1657
