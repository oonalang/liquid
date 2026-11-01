#pragma once












#include "Triggerbot1657Policy.h"
#include "Triggerbot1657Hitboxes.h"
#include "EnemyListSnapshot1657.h"
#include "CustomDotCrosshair.h"
#include "Triggerbot1657Diagnostics.h"
#include "Triggerbot1657LinecastAbi.h"
#include <link.h>
#include <cstring>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <array>
#include <cstddef>
#include <climits>
#include <type_traits>

namespace Triggerbot1657 {

inline bool uiEnabled=false;
inline Mode uiMode=Mode::AdsOnly;
inline std::atomic<unsigned> selectedMode{0};
inline bool uiRequireWallCheck=true;
inline bool uiRequireSmokeCheck=true;
inline std::atomic<bool> enabled{false};
inline std::atomic<float> crosshairX{0}, crosshairY{0}, radiusPixels{0};
inline std::atomic<float> viewportW{0}, viewportH{0};
inline std::atomic<std::int64_t> lastFrameMs{0};
inline std::atomic<unsigned> configSequence{0};




inline void (*originalUpdateLocalPawn)(void*,float,const void*) = nullptr;

inline void (*startFire)(void*,const void*) = nullptr;

inline void (*stopFire)(void*,bool,const void*) = nullptr;
inline bool (*isAiming)(void*,const void*) = nullptr;
inline bool (*canFire)(void*,bool,const void*) = nullptr;

// Weapon fire-mode metadata (1.6.57 Weapon, RVA from the metadata dump).
// Reading the weapon's own flags classifies a weapon on the frame it is drawn,
// instead of holding the trigger for several hundred milliseconds to guess from
// the magazine count. A semi-auto that waits for that guess never fires: the
// game only accepts one round per press, so holding the button just does nothing.
inline bool (*weaponIsSingleFire)(void*,const void*) = nullptr;   // Weapon.IsSingleFire()
inline bool (*weaponCanAutoFire)(void*,const void*) = nullptr;    // Weapon.get_CanAutoFire()
inline bool (*weaponIsSingleShot)(void*,const void*) = nullptr;   // Weapon.get_IsSingleShot()

inline void* lastController=nullptr;
// MatchGame is a new object for every match. The previous match's pawn, controller
// and weapon are torn down around that swap, so firing across it reaches objects the
// game is already destroying.
inline void* lastMatchGame=nullptr;
inline bool hasObservedMatch=false;
inline std::int64_t matchChangedMs=0;
inline bool matchSettled=true;
inline constexpr std::int64_t kMatchSettleMs=1500;
// A round transition or a respawn replaces the local pawn, and the weapon that was
// pressed belongs to the pawn being torn down. Releasing on it ran the game's fire
// path against a weapon that was already gone, which is what took the game down
// between rounds; the release is remembered and issued on the first settled frame of
// the new pawn instead.
inline void* lastLocalPawn=nullptr;
inline std::int64_t pawnChangedMs=0;
inline bool pawnSettled=true;
inline bool pendingRelease=false;
inline constexpr std::int64_t kPawnSettleMs=600;
// A round transition does not always replace the local pawn: a surviving player keeps
// it while the round itself is rebuilt underneath (weapon, enemy set, physics, effects).
// Only act once the (match, pawn, weapon) triple has held still for a few frames, so no
// fire call and no scan runs against a half-built round. In steady state the window
// re-arms only when one of the three actually changes, so it adds no input latency.
inline void* stableMatch=nullptr;
inline void* stablePawn=nullptr;
inline void* stableWeapon=nullptr;
inline unsigned stableFrames=0;
inline unsigned settleFrames=8;
// The weapon the held press was issued on. A remembered release is only valid on that
// same weapon: the replacement weapon never saw the trigger, and running the game's
// release path on it reaches a fire component that is still being created. That call is
// game code we cannot guard, which is how a round change took the game down.
inline const void* pressWeapon=nullptr;
// Enemy pawns are destroyed and rebuilt at a round change, and (in ranked, where the
// list holds real players) when someone leaves mid-match. BaseGame.EnemyPawns can keep
// pointing at such a pawn for a few frames; reading its ragdoll and calling into an
// animator that is being torn down is game code with no frame of ours to blame. Only
// read a pawn that has stayed in the list for a moment, and only trust a byte as a bool
// when it actually is one -- freed blocks do not read as 0/1.
inline constexpr std::int64_t kEnemySettleMs=300;
inline std::int64_t enemySettleMs=kEnemySettleMs;
struct SettledEnemy { const void* pawn=nullptr; std::int64_t firstMs=0; unsigned epoch=0; };
inline std::array<SettledEnemy,128> settledEnemies{};
inline unsigned settledEnemyCount=0;
inline unsigned settledEnemyEpoch=0;
inline void BeginEnemyEvaluation() {
    if (++settledEnemyEpoch==0) { settledEnemyCount=0; settledEnemyEpoch=1; return; }
    unsigned keep=0;
    for (unsigned i=0;i<settledEnemyCount;++i)
        if (settledEnemies[i].epoch==settledEnemyEpoch-1) settledEnemies[keep++]=settledEnemies[i];
    settledEnemyCount=keep;
}
inline bool EnemyHasSettled(const void* pawn,std::int64_t now) {
    for (unsigned i=0;i<settledEnemyCount;++i) {
        if (settledEnemies[i].pawn!=pawn) continue;
        settledEnemies[i].epoch=settledEnemyEpoch;
        return enemySettleMs<=0 || now-settledEnemies[i].firstMs>=enemySettleMs;
    }
    if (settledEnemyCount<settledEnemies.size())
        settledEnemies[settledEnemyCount++]={pawn,now,settledEnemyEpoch};
    return enemySettleMs<=0;
}
inline bool triggerHeldFire=false;
inline std::int64_t fireTapStartedMs=0;
// Clip count sampled when the current press was issued; -1 when unreadable. A
// single-fire press is held until this drops, so a shot the game delays (the ADS
// aim-to-fire interval) is not thrown away by an early release.
inline std::int32_t pressAmmoBefore=-1;
// Backoff before the next press after the game ignored one, and the single retry
// allowed for a held automatic weapon whose press was dropped by a fire-mode
// transition. Both keep a failed press from re-running the weapon fire path every
// frame for the rest of the match.
inline std::int64_t nextPressMs=0;
inline bool pressReissued=false;
inline bool previousLocalAlive=false;



inline std::int64_t tapHoldMs=60;



inline int (*getClipAmmo)(void*,const void*)=nullptr;






inline Vector3 (*pawnFirePosition)(void*,const void*)=nullptr;
inline Vector3 (*pawnEyePosition)(void*,const void*)=nullptr;



inline unsigned fireOriginReadFailures=0, eyeOriginReadFailures=0;
inline FireMode fireMode=FireMode::Unknown;
inline const void* fireModeWeapon=nullptr;
inline bool probeActive=false;
inline std::int64_t probeStartedMs=0;
inline std::int64_t probeMidMs=200, probeEndMs=350, probeRetryMs=2000;
inline std::int32_t probeBefore=0, probeMid=0;
inline bool probeMidTaken=false;
inline std::int64_t probeRetryAfterMs=0;


inline unsigned probeFailures=0;
inline std::int64_t nextLegacyDiagnosticProbeMs=0;

inline void ResetLifeState() {
    triggerHeldFire=false;
    fireTapStartedMs=0;
    pressAmmoBefore=-1;
    nextPressMs=0;
    pressReissued=false;
    fireMode=FireMode::Unknown;
    fireModeWeapon=nullptr;
    probeActive=false;
    probeStartedMs=0;
    probeBefore=0;
    probeMid=0;
    probeMidTaken=false;
    probeRetryAfterMs=0;
    probeFailures=0;
    fireOriginReadFailures=0;
    eyeOriginReadFailures=0;
    nextLegacyDiagnosticProbeMs=0;
    diagProbeSpent.store(false,std::memory_order_relaxed);
    diagFireMode.store(static_cast<unsigned>(FireMode::Unknown),std::memory_order_relaxed);
    diagUpdateCount.store(0,std::memory_order_relaxed);
    diagLastUpdateMs.store(0,std::memory_order_relaxed);
    diagFireRequests.store(0,std::memory_order_relaxed);
    diagFireTaps.store(0,std::memory_order_relaxed);
    diagProbes.store(0,std::memory_order_relaxed);
}



inline bool ReadClipAmmo(void* weapon, std::int32_t& value) {
    if (!weapon || !getClipAmmo || !Tools::IsPtrValid(weapon)) return false;
    const int ammo=getClipAmmo(weapon,nullptr);
    if (ammo<0 || ammo>2000) return false;
    value=ammo;
    return true;
}

// Authoritative fire-mode read. A weapon that reports itself as single-shot or
// single-fire needs one press per round; a weapon that explicitly cannot
// auto-fire does too (that covers semi-automatic rifles, which are the case the
// magazine probe used to hold down). Only an explicit auto-fire capability keeps
// the trigger held. Anything the metadata cannot answer stays Unknown so the
// bounded magazine probe can still classify it.
inline FireMode IdentifyFireMode(void* weapon) {
    if (!weapon) return FireMode::Unknown;
    const bool singleFire=weaponIsSingleFire && weaponIsSingleFire(weapon,nullptr);
    const bool singleShot=weaponIsSingleShot && weaponIsSingleShot(weapon,nullptr);
    if (singleFire || singleShot) return FireMode::Single;
    if (!weaponCanAutoFire) return FireMode::Unknown;
    return weaponCanAutoFire(weapon,nullptr) ? FireMode::Auto : FireMode::Single;
}





struct RaycastHit {
    Vector3 point,normal;
    std::int32_t faceId;
    float distance;
    float uv[2];
    void* collider;
};
static_assert(sizeof(Vector3)==12 && std::is_trivially_copyable<Vector3>::value,
              "Native Vector3 arguments require three trivially copied floats");
static_assert(offsetof(RaycastHit,collider)==0x28 && sizeof(RaycastHit)==0x30,
              "RaycastHit must match the ARM64 1.6.57 dump");

inline bool (*physicsLinecast)(Vector3,Vector3,RaycastHit*,const void*)=nullptr;



inline bool (*physicsLinecastFiltered)(Vector3,Vector3,RaycastHit*,const void*)=nullptr;
inline bool (*colliderIsTrigger)(void*,const void*)=nullptr;
inline bool (*animatorIsHuman)(void*,const void*)=nullptr;
inline Transform* (*animatorBoneTransform)(void*,int,const void*)=nullptr;





inline constexpr std::int64_t kLegacyProbeIntervalMs=1000;
inline constexpr unsigned kMaxLinecastsPerEvaluation=24;
inline Transform* (*componentTransform)(void*,const void*)=nullptr;

inline bool (*transformIsChildOf)(Transform*,Transform*,const void*)=nullptr;
inline bool (*smokeBetween)(void*,Vector3,Vector3,const void*)=nullptr;
inline bool (*inSmoke)(void*,Vector3,const void*)=nullptr;

inline std::uintptr_t nativeLinecastAddress=0;
inline std::uintptr_t nativeMaskedLinecastAddress=0;
inline LinecastAbi nativeLinecastAbi=LinecastAbi::Unverified;
inline bool NativeLinecast(Vector3 from,Vector3 to,RaycastHit* hit,const void* method) {
    if (nativeLinecastAbi==LinecastAbi::Static)
        return reinterpret_cast<bool (*)(Vector3,Vector3,RaycastHit*,const void*)>(nativeLinecastAddress)(from,to,hit,method);
    if (nativeLinecastAbi==LinecastAbi::StaticWithReceiver)
        return reinterpret_cast<bool (*)(void*,Vector3,Vector3,RaycastHit*,const void*)>(nativeLinecastAddress)(nullptr,from,to,hit,method);
    return false; // Unverified binding is never published to physicsLinecast.
}

// layerMask=-5 and queryTriggerInteraction=UseGlobal(0). Preserve the verified


inline bool NativeLinecastFiltered(Vector3 from,Vector3 to,RaycastHit* hit,const void* method) {
    constexpr std::int64_t defaultLayers=-5;
    constexpr int ignoreTriggers=1;
    if (nativeLinecastAbi==LinecastAbi::Static)
        return reinterpret_cast<bool (*)(Vector3,Vector3,RaycastHit*,std::int64_t,int,const void*)>(
            nativeMaskedLinecastAddress)(from,to,hit,defaultLayers,ignoreTriggers,method);
    if (nativeLinecastAbi==LinecastAbi::StaticWithReceiver)
        return reinterpret_cast<bool (*)(void*,Vector3,Vector3,RaycastHit*,std::int64_t,int,const void*)>(
            nativeMaskedLinecastAddress)(nullptr,from,to,hit,defaultLayers,ignoreTriggers,method);
    return false;
}
inline bool ReadLinecastWrapper(std::uintptr_t base,std::array<std::uint32_t,3>& code) {
    struct Probe { std::uintptr_t base; std::array<std::uint32_t,3>* code; bool read=false; } probe{base,&code};
    dl_iterate_phdr([](dl_phdr_info* info,std::size_t,void* context) {
        auto& p=*static_cast<Probe*>(context);
        if (info->dlpi_addr!=p.base) return 0;
        const auto address=p.base+0x506D104;
        for (unsigned i=0;i<info->dlpi_phnum;++i) {
            const auto& segment=info->dlpi_phdr[i];
            if (segment.p_type!=PT_LOAD || (segment.p_flags&(PF_R|PF_X))!=(PF_R|PF_X)) continue;
            const auto start=p.base+segment.p_vaddr;
            if (address<start || address-start>segment.p_memsz ||
                sizeof(*p.code)>segment.p_memsz-(address-start)) continue;
            std::memcpy(p.code->data(),reinterpret_cast<const void*>(address),sizeof(*p.code));
            p.read=true;
            break;
        }
        return 1;
    },&probe);
    return probe.read;
}
inline void Initialize(std::uintptr_t unityBase) {
    if (!unityBase) return;
    startFire=reinterpret_cast<void (*)(void*,const void*)>(unityBase+0x66704C4);
    stopFire=reinterpret_cast<void (*)(void*,bool,const void*)>(unityBase+0x667062C);

    getClipAmmo=reinterpret_cast<int (*)(void*,const void*)>(unityBase+0x50EC614);

    pawnFirePosition=reinterpret_cast<Vector3 (*)(void*,const void*)>(unityBase+0x51B8900);
    pawnEyePosition=reinterpret_cast<Vector3 (*)(void*,const void*)>(unityBase+0x51B87B8);
    fireOriginReadFailures=0;
    eyeOriginReadFailures=0;
    nextLegacyDiagnosticProbeMs=0;
    diagIgnoredTriggerHits.store(0,std::memory_order_relaxed);
    isAiming=reinterpret_cast<bool (*)(void*,const void*)>(unityBase+0x519BB10);
    canFire=reinterpret_cast<bool (*)(void*,bool,const void*)>(unityBase+0x5260EC0);
    weaponIsSingleFire=reinterpret_cast<bool (*)(void*,const void*)>(unityBase+0x50EC158);
    weaponCanAutoFire=reinterpret_cast<bool (*)(void*,const void*)>(unityBase+0x512A0E8);
    weaponIsSingleShot=reinterpret_cast<bool (*)(void*,const void*)>(unityBase+0x50EBA5C);
    nativeLinecastAddress=unityBase+0x506D104;

    nativeMaskedLinecastAddress=unityBase+0x506D07C;
    std::array<std::uint32_t,3> code{};
    nativeLinecastAbi=ReadLinecastWrapper(unityBase,code) ?
        DecodeLinecastAbi(code,nativeLinecastAddress,unityBase+0x506D07C) : LinecastAbi::Unverified;
    for (unsigned i=0;i<3;++i) diagLinecastCode[i].store(code[i],std::memory_order_relaxed);
    diagLinecastAbi.store(static_cast<unsigned>(nativeLinecastAbi),std::memory_order_relaxed);
    physicsLinecast=nativeLinecastAbi==LinecastAbi::Unverified ? nullptr : &NativeLinecast;
    physicsLinecastFiltered=nativeLinecastAbi==LinecastAbi::Unverified ? nullptr : &NativeLinecastFiltered;
    diagFilteredLinecastEnabled.store(physicsLinecastFiltered!=nullptr,std::memory_order_relaxed);

    colliderIsTrigger=reinterpret_cast<bool (*)(void*,const void*)>(unityBase+0x50279D4);
    animatorIsHuman=reinterpret_cast<bool (*)(void*,const void*)>(unityBase+0x5014CE8);
    animatorBoneTransform=reinterpret_cast<Transform* (*)(void*,int,const void*)>(unityBase+0x50182B8);

    componentTransform=reinterpret_cast<Transform* (*)(void*,const void*)>(unityBase+0x50291B0);
    transformIsChildOf=reinterpret_cast<bool (*)(Transform*,Transform*,const void*)>(unityBase+0x509DF80);
    smokeBetween=reinterpret_cast<bool (*)(void*,Vector3,Vector3,const void*)>(unityBase+0x9CC36E0);
    inSmoke=reinterpret_cast<bool (*)(void*,Vector3,const void*)>(unityBase+0x9CC3518);
}

inline void PublishUi(const ImVec2& screen) {
    if (!uiEnabled || !CustomDotCrosshair::settings.enabled ||
        !std::isfinite(screen.x) || !std::isfinite(screen.y) ||
        screen.x < 100.f || screen.y < 100.f) {
        enabled.store(false,std::memory_order_release);
        Report(DiagnosticStage::Disabled);
        return;
    }
    const auto g=CustomDotCrosshair::Calculate(screen,CustomDotCrosshair::settings);
    if (!std::isfinite(g.center.x) || !std::isfinite(g.center.y)) {
        enabled.store(false,std::memory_order_release);
        return;
    }
    configSequence.fetch_add(1,std::memory_order_acq_rel);
    selectedMode.store(static_cast<unsigned>(uiMode),std::memory_order_relaxed);
    crosshairX.store(g.center.x,std::memory_order_relaxed);
    crosshairY.store(g.center.y,std::memory_order_relaxed);
    radiusPixels.store(g.firingRadius,std::memory_order_relaxed);
    viewportW.store(screen.x,std::memory_order_relaxed);
    viewportH.store(screen.y,std::memory_order_relaxed);
    lastFrameMs.store(NowMs(),std::memory_order_relaxed);
    configSequence.fetch_add(1,std::memory_order_release);
    enabled.store(true,std::memory_order_release);
}
inline bool ReadSnapshot(Point& center,float& radius,float& renderW,float& renderH,std::int64_t& frame,unsigned& mode) {
    if (!enabled.load(std::memory_order_acquire)) return false;
    for (unsigned attempt=0; attempt<3; ++attempt) {
        const auto a=configSequence.load(std::memory_order_acquire);
        if (a&1) continue;
        center={crosshairX.load(std::memory_order_relaxed),crosshairY.load(std::memory_order_relaxed)};
        radius=radiusPixels.load(std::memory_order_relaxed);
        renderW=viewportW.load(std::memory_order_relaxed);
        renderH=viewportH.load(std::memory_order_relaxed);
        frame=lastFrameMs.load(std::memory_order_relaxed);
        mode=selectedMode.load(std::memory_order_relaxed);
        const auto b=configSequence.load(std::memory_order_acquire);
        if (a==b && !(b&1)) return enabled.load(std::memory_order_acquire);
    }
    return false;
}
inline bool Finite3(Vector3 p) { return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z); }






struct FramePointerValidation {
    struct Entry { const void* ptr; bool valid; };
    std::array<Entry,96> entries{};
    unsigned size=0, checks=0, reused=0;
    bool IsValid(const void* ptr) {
        if (!ptr) return false;
        for (unsigned i=0;i<size;++i) {
            if (entries[i].ptr==ptr) {++reused; return entries[i].valid;}
        }
        ++checks;
        const bool valid=Tools::IsPtrValid(const_cast<void*>(ptr));
        if (size<entries.size()) entries[size++]={ptr,valid};
        return valid;
    }
    ~FramePointerValidation() {
        diagPointerValidations.store(checks,std::memory_order_relaxed);
        diagReusedPointerValidations.store(reused,std::memory_order_relaxed);
    }
};




inline bool IsPawnPart(Transform* part, void* pawn,FramePointerValidation& validity) {
    if (!part || !pawn || !componentTransform) return false;
    auto* root=componentTransform(pawn,nullptr);
    if (!validity.IsValid(root)) return false;
    if (part==root) return true;
    return transformIsChildOf && transformIsChildOf(part,root,nullptr);
}





struct LocalAnchors {
    Transform* mesh=nullptr;   // Pawn.m_Mesh: the model root, owns the body colliders
    Transform* head=nullptr;   // Pawn.m_HeadBone
    Transform* root=nullptr;   // Component.get_transform(Pawn)
    Transform* camera=nullptr; // first-person view model hangs off the camera
};
inline bool SameOrChildOf(Transform* part,Transform* anchor) {
    if (!part || !anchor) return false;
    if (part==anchor) return true;
    return transformIsChildOf && transformIsChildOf(part,anchor,nullptr);
}
inline bool IsLocalPart(Transform* part,const LocalAnchors& anchors) {
    if (!part) return false;
    if (SameOrChildOf(part,anchors.mesh) || SameOrChildOf(part,anchors.head) ||
        SameOrChildOf(part,anchors.root) || SameOrChildOf(part,anchors.camera)) return true;



    return transformIsChildOf &&
        ((anchors.mesh && transformIsChildOf(anchors.mesh,part,nullptr)) ||
         (anchors.root && transformIsChildOf(anchors.root,part,nullptr)));
}





inline bool LineToTarget(Vector3 from,Vector3 target,const LocalAnchors& own,void* enemyPawn,
                         FramePointerValidation& validity,HitKind& kind,float& hitDistance,unsigned& selfSkips,
                         unsigned& remainingCasts,std::uintptr_t* blockedCollider=nullptr) {
    kind=HitKind::None;
    hitDistance=-1.f;
    selfSkips=0;
    if (blockedCollider) *blockedCollider=0;
    const float dx=target.x-from.x, dy=target.y-from.y, dz=target.z-from.z;
    const float length=std::sqrt(dx*dx+dy*dy+dz*dz);
    if (!std::isfinite(length) || length<=0.001f) {
        kind=HitKind::None;
        return true;
    }
    const Vector3 direction{dx/length,dy/length,dz/length};
    float advanced=0.f;
    unsigned embeddedSkips=0;
    auto advance=[&](float step) {
        if (!std::isfinite(step) || step<=0.f || advanced+step>=length) return false;
        advanced+=step;
        return true;
    };
    auto pointAt=[&](float distance) {
        return Vector3{from.x+direction.x*distance,
                       from.y+direction.y*distance,
                       from.z+direction.z*distance};
    };
    for (;;) {
        RaycastHit hit{};


        if (!remainingCasts) {
            kind=HitKind::Unknown;
            return false;
        }
        --remainingCasts;
        const auto cast=physicsLinecastFiltered ? physicsLinecastFiltered : physicsLinecast;
        diagPhysicsCasts.fetch_add(1,std::memory_order_relaxed);
        const Vector3 cursor=pointAt(advanced);
        if (!cast(cursor,target,&hit,nullptr)) {




            kind=HitKind::None;
            hitDistance=-1.f;
            return true;
        }
        hitDistance=advanced+std::max(hit.distance,0.f);
        Transform* hitTransform=nullptr;
        if (validity.IsValid(hit.collider) && componentTransform)
            hitTransform=componentTransform(hit.collider,nullptr);
        const bool transformValid=validity.IsValid(hitTransform);
        if (transformValid) {
            if (enemyPawn && IsPawnPart(hitTransform,enemyPawn,validity)) {
                kind=HitKind::SelectedEnemy;
                return true;
            }
            if (IsLocalPart(hitTransform,own)) {
                if (!SkipSelfHit(selfSkips,advanced,std::max(hit.distance,0.f))) {
                    if (blockedCollider) *blockedCollider=reinterpret_cast<std::uintptr_t>(hit.collider);
                    kind=HitKind::Other;
                    return false;
                }
                if (!advance(std::max(hit.distance,0.f)+kSelfHitStep)) {
                    if (blockedCollider) *blockedCollider=reinterpret_cast<std::uintptr_t>(hit.collider);
                    kind=HitKind::Other; return false;
                }
                ++selfSkips;
                continue;
            }
        }
        // The ray began on or inside the obstacle (the player is standing flush
        // against low cover): step out of it instead of reporting an immediate block.
        if (EmbeddedHit(hit.distance) && advanced<=0.f && embeddedSkips<kMaxEmbeddedSkips) {
            ++embeddedSkips;
            if (!advance(kEmbeddedHitStep)) {
                if (blockedCollider) *blockedCollider=reinterpret_cast<std::uintptr_t>(hit.collider);
                kind=transformValid ? HitKind::Other : HitKind::Unknown;
                return false;
            }
            continue;
        }
        if (blockedCollider) *blockedCollider=reinterpret_cast<std::uintptr_t>(hit.collider);
        kind=transformValid ? HitKind::Other : HitKind::Unknown;
        return false;
    }
}


struct OriginCandidate {
    RayOrigin id=RayOrigin::None;
    Vector3 position{};
    bool available=false;
};









inline bool ReadPawnOrigin(Vector3 (*getter)(void*,const void*),void* pawn,
                           bool haveReference,Vector3 reference,
                           unsigned& failures,Vector3& out) {
    if (!getter || !pawn || !haveReference) return false;
    if (failures>=kMaxOriginReadFailures && failures%64u!=0u) {
        if (failures!=UINT_MAX) ++failures;
        return false;
    }
    const Vector3 value=getter(pawn,nullptr);
    bool sane=Finite3(value);
    if (sane) {
        const float dx=value.x-reference.x, dy=value.y-reference.y, dz=value.z-reference.z;
        sane=OriginOffsetSane(dx*dx+dy*dy+dz*dz);
    }
    if (!sane) {
        if (failures!=UINT_MAX) ++failures;
        return false;
    }
    failures=0;
    out=value;
    return true;
}

inline bool EligibleFromLocalView(void* weapon, bool ads, bool weaponReady) {
    FramePointerValidation validity;
    Point center{}; float radius=0,renderW=0,renderH=0; std::int64_t frame=0;
    unsigned modeValue=0;

    Report(DiagnosticStage::SnapshotUnavailable);
    diagEnemies.store(0,std::memory_order_relaxed);
    diagBots.store(0,std::memory_order_relaxed);
    diagDotMatches.store(0,std::memory_order_relaxed);
    diagWallRejections.store(0,std::memory_order_relaxed);
    diagSmokeRejections.store(0,std::memory_order_relaxed);
    diagHitKind.store(static_cast<unsigned>(HitKind::None),std::memory_order_relaxed);
    diagHitDistance.store(-1.f,std::memory_order_relaxed);
    diagTargetDistance.store(-1.f,std::memory_order_relaxed);
    diagHitOrigin.store(static_cast<unsigned>(RayOrigin::None),std::memory_order_relaxed);
    diagSelfSkips.store(0,std::memory_order_relaxed);
    diagVisibilityProbes.store(0,std::memory_order_relaxed);
    diagPhysicsCasts.store(0,std::memory_order_relaxed);
    diagLegacyPhysicsCasts.store(0,std::memory_order_relaxed);
    diagLastSampleFraction.store(-1.f,std::memory_order_relaxed);
    ResetOriginDiagnostics();
    if (!physicsLinecast && nativeLinecastAddress) {
        Report(DiagnosticStage::LinecastAbiUnavailable); return false;
    }
    if (!ReadSnapshot(center,radius,renderW,renderH,frame,modeValue) || !m_unity || !physicsLinecast ||
        !componentTransform || !smokeBetween || !inSmoke) {
        Report(DiagnosticStage::SnapshotUnavailable); return false;
    }
    if (!weapon || !weaponReady) {
        Report(DiagnosticStage::WeaponNotReady); return false;
    }
    // The ADS requirement gates the shot, not the work that finds the target. Bailing
    // out here made the first scoped frame of an ADS-only engagement pay for the whole
    // scan - snapshot, bone samples and the raycast batch - on top of the scope
    // transition, which is the one frame that has no room for it. Both mode never
    // showed the hitch because it spreads that work over every frame.
    if (modeValue>1) { Report(DiagnosticStage::AdsRequired); return false; }
    const bool modeAllows=ModeAllows(static_cast<Mode>(modeValue),ads);
    auto* pawn=GamePlay::get_LocalPawn();
    if (!pawn || !validity.IsValid(pawn) || pawn->get_CurrentWeapon()!=weapon) {
        Report(DiagnosticStage::LocalPlayerUnavailable); return false;
    }
    const auto aliveByte=*reinterpret_cast<const std::uint8_t*>(
        reinterpret_cast<std::uintptr_t>(pawn)+Pawn_m_IsAlive);
    const bool alive=aliveByte==1;
    if (!alive) { Report(DiagnosticStage::LocalPlayerUnavailable); return false; }
    auto* camera=Camera::get_main();
    if (!camera || !validity.IsValid(camera)) { Report(DiagnosticStage::CameraUnavailable); return false; }
    auto* cameraTf=componentTransform(camera,nullptr);
    if (!cameraTf || !validity.IsValid(cameraTf)) { Report(DiagnosticStage::CameraUnavailable); return false; }
    const Vector3 cameraPos=cameraTf->get_position();
    if (!Finite3(cameraPos)) { Report(DiagnosticStage::CameraUnavailable); return false; }










    OriginCandidate origins[]={
        {RayOrigin::Fire, {}, false},
        {RayOrigin::Eye, {}, false},
        {RayOrigin::Head, {}, false},
        {RayOrigin::Camera, cameraPos, true},
    };


    auto setOrigin=[&origins](RayOrigin id,Vector3 position) {
        for (auto& candidate : origins) {
            if (candidate.id!=id) continue;
            candidate.position=position;
            candidate.available=true;
            return;
        }
    };


    Transform* ownRoot=componentTransform(pawn,nullptr);
    if (ownRoot && !validity.IsValid(ownRoot)) ownRoot=nullptr;
    const auto localHead=*reinterpret_cast<Transform**>(
        reinterpret_cast<std::uintptr_t>(pawn)+Class_Pawn__m_HeadBone);
    bool haveHead=false;
    Vector3 headPos{};
    if (localHead && validity.IsValid(localHead)) {
        const Vector3 localHeadPos=localHead->get_position();
        if (Finite3(localHeadPos)) { headPos=localHeadPos; haveHead=true; }
    }




    bool haveReference=false;
    Vector3 reference{};
    Transform* meshTransform=nullptr;
    const auto localMesh=*reinterpret_cast<Transform**>(
        reinterpret_cast<std::uintptr_t>(pawn)+Class_Pawn__m_Mesh);
    if (localMesh && validity.IsValid(localMesh)) {
        meshTransform=localMesh;
        const Vector3 meshPos=localMesh->get_position();
        if (Finite3(meshPos)) { reference=meshPos; haveReference=true; }
    }
    if (!haveReference && ownRoot) {
        const Vector3 rootPos=ownRoot->get_position();
        if (Finite3(rootPos)) { reference=rootPos; haveReference=true; }
    }
    if (!haveReference && haveHead) { reference=headPos; haveReference=true; }
    LocalAnchors own;
    own.mesh=meshTransform;
    own.head=haveHead ? localHead : nullptr;
    own.root=ownRoot;
    own.camera=cameraTf;
    Vector3 firePos{}, eyePos{};
    if (ReadPawnOrigin(pawnFirePosition,pawn,haveReference,reference,
                       fireOriginReadFailures,firePos)) setOrigin(RayOrigin::Fire,firePos);
    if (ReadPawnOrigin(pawnEyePosition,pawn,haveReference,reference,
                       eyeOriginReadFailures,eyePos)) setOrigin(RayOrigin::Eye,eyePos);
    if (haveHead) setOrigin(RayOrigin::Head,headPos);
    const float gameW=static_cast<float>(get_width());
    const float gameH=static_cast<float>(get_height());
    if (!std::isfinite(gameW) || !std::isfinite(gameH) || gameW<100.f ||
        gameH<100.f || renderW<100.f || renderH<100.f) return false;

    const auto gameFn=reinterpret_cast<std::uintptr_t(*)()>(Class_Gameplay__get_MatchGame);
    const std::uintptr_t game=gameFn();
    if (!game || !validity.IsValid(reinterpret_cast<void*>(game))) {
        Report(DiagnosticStage::MatchUnavailable); return false;
    }
    // Every target is rejected while the camera is inside smoke. Query once,
    // before building poses and casting up to 24 rays through a busy objective.
    if (inSmoke(reinterpret_cast<void*>(game),cameraPos,nullptr)) {
        diagSmokeRejections.fetch_add(1,std::memory_order_relaxed);
        Report(DiagnosticStage::SmokeBlocked);
        return false;
    }
    const auto enemyList=*reinterpret_cast<List<std::uintptr_t>**>(game+Class_BaseGame__EnemyPawns);
    if (!enemyList || !validity.IsValid(enemyList)) {
        Report(DiagnosticStage::EnemyListUnavailable); return false;
    }
    unsigned castsRemaining=kMaxLinecastsPerEvaluation;
    EnemyListSnapshot1657::Snapshot enemySnapshot;
    if (!EnemyListSnapshot1657::Read(enemyList,
            [&validity](const void* pointer) { return validity.IsValid(const_cast<void*>(pointer)); },
            enemySnapshot)) {
        Report(DiagnosticStage::EnemyListUnavailable);
        return false;
    }

    // Preferred order: firing position, eye, head, then camera. A blocked muzzle no
    // longer vetoes the shot - in 1.6.57 the bullet ray is spawned from the camera, so
    // a clear camera path is what the game itself will hit. Solid cover still blocks
    // every origin, and the first clear origin wins so the common case costs one cast.
    const OriginCandidate* usable[4]{};
    unsigned usableCount=0;
    for (const auto& candidate : origins) {
        if (candidate.available) usable[usableCount++]=&candidate;
        else MarkOriginUnavailable(candidate.id);
    }
    // In ADS the game fires from the camera, and the muzzle ray starts inside the
    // player's own view model, so the camera test is both authoritative and the
    // cheapest. Trying the muzzle first spent several casts before reaching it, which
    // is latency on every shot and physics work on every frame.
    if (ads) {
        for (unsigned i=1;i<usableCount;++i) {
            if (usable[i]->id!=RayOrigin::Camera) continue;
            std::swap(usable[0],usable[i]);
            break;
        }
    }

    BeginEnemyEvaluation();
    for (int i=0;i<enemySnapshot.count;++i) {
        const std::uintptr_t enemy=enemySnapshot.pawns[i];
        if (!enemy || enemy==reinterpret_cast<std::uintptr_t>(pawn) ||
            !validity.IsValid(reinterpret_cast<void*>(enemy))) continue;
        if (*reinterpret_cast<const std::uint8_t*>(enemy+Pawn_m_IsAlive)!=1) continue;
        if (!EnemyHasSettled(reinterpret_cast<const void*>(enemy),NowMs())) continue;

        diagEnemies.fetch_add(1,std::memory_order_relaxed);
        if (*reinterpret_cast<const std::uint8_t*>(enemy+Class_Pawn__m_IsBot)==1)
            diagBots.fetch_add(1,std::memory_order_relaxed);
        const auto head=*reinterpret_cast<Transform**>(enemy+Class_Pawn__m_HeadBone);
        const auto root=*reinterpret_cast<Transform**>(enemy+Class_Pawn__m_Mesh);
        if (!head || !root || !validity.IsValid(head) || !validity.IsValid(root)) continue;
        const Vector3 headPos=head->get_position(), rootPos=root->get_position();
        if (!Finite3(headPos) || !Finite3(rootPos)) continue;
        const Vector3 headScreen=WorldToScreenPoint(headPos);
        const Vector3 rootScreen=WorldToScreenPoint(rootPos);
        const auto projected=[](Vector3 point) {
            return Finite3(point) && point.z>0.f;
        };
        const auto screenPoint=[&](Vector3 point) -> Point {
            return {point.x*(renderW/gameW),renderH-point.y*(renderH/gameH)};
        };
        const bool headVisible=projected(headScreen),rootVisible=projected(rootScreen);
        if (headVisible && rootVisible &&
            !Hitboxes::RoughlyNear(center,radius,screenPoint(headScreen),screenPoint(rootScreen)))
            continue;

        Hitboxes::Pose pose{};
        auto setJoint=[&](Hitboxes::Joint joint,Vector3 position) {
            if (!Finite3(position)) return;
            const float dx=position.x-rootPos.x,dy=position.y-rootPos.y,dz=position.z-rootPos.z;
            if (joint!=Hitboxes::Joint::Head && dx*dx+dy*dy+dz*dz>36.f) return;
            const Vector3 screen=WorldToScreenPoint(position);
            if (!projected(screen)) return;
            auto& point=pose[static_cast<unsigned>(joint)];
            point.world={position.x,position.y,position.z};
            point.screen=screenPoint(screen);
            point.valid=true;
        };
        setJoint(Hitboxes::Joint::Head,headPos);

        const auto* animationComponent=*reinterpret_cast<void* const*>(enemy+0x538);
        void* animator=nullptr;
        if (validity.IsValid(animationComponent)) {
            animator=*reinterpret_cast<void* const*>(
                reinterpret_cast<std::uintptr_t>(animationComponent)+0x20);
        }
        bool humanoid=false;
        if (animator && validity.IsValid(animator) && animatorIsHuman && animatorBoneTransform)
            humanoid=animatorIsHuman(animator,nullptr);
        if (humanoid) {
            struct Bone { Hitboxes::Joint joint; int humanBone; };
            static constexpr std::array<Bone,18> bones{{
                {Hitboxes::Joint::Head,10}, {Hitboxes::Joint::Neck,9},
                {Hitboxes::Joint::Chest,8}, {Hitboxes::Joint::Hips,0},
                {Hitboxes::Joint::LeftShoulder,11}, {Hitboxes::Joint::LeftUpperArm,13},
                {Hitboxes::Joint::LeftLowerArm,15}, {Hitboxes::Joint::LeftHand,17},
                {Hitboxes::Joint::RightShoulder,12}, {Hitboxes::Joint::RightUpperArm,14},
                {Hitboxes::Joint::RightLowerArm,16}, {Hitboxes::Joint::RightHand,18},
                {Hitboxes::Joint::LeftUpperLeg,1}, {Hitboxes::Joint::LeftLowerLeg,3},
                {Hitboxes::Joint::LeftFoot,5}, {Hitboxes::Joint::RightUpperLeg,2},
                {Hitboxes::Joint::RightLowerLeg,4}, {Hitboxes::Joint::RightFoot,6}
            }};
            for (const auto& bone : bones) {
                auto* transform=animatorBoneTransform(animator,bone.humanBone,nullptr);
                if (transform && validity.IsValid(transform))
                    setJoint(bone.joint,transform->get_position());
            }
        }
        if (!pose[static_cast<unsigned>(Hitboxes::Joint::Chest)].valid) {
            auto* upper=*reinterpret_cast<Transform* const*>(enemy+0x2C8);
            if (upper && validity.IsValid(upper))
                setJoint(Hitboxes::Joint::Chest,upper->get_position());
        }
        if (!pose[static_cast<unsigned>(Hitboxes::Joint::Hips)].valid)
            setJoint(Hitboxes::Joint::Hips,rootPos);
        if (!humanoid && !pose[static_cast<unsigned>(Hitboxes::Joint::Chest)].valid)
            setJoint(Hitboxes::Joint::Neck,rootPos);

        const auto eligibleParts=Hitboxes::Collect(pose,center,radius);
        if (!eligibleParts.count) continue;
        diagDotMatches.fetch_add(1,std::memory_order_relaxed);

        bool wallRejected=false,smokeRejected=false;
        if (!usableCount) {
            diagWallRejections.fetch_add(1,std::memory_order_relaxed);
            continue;
        }
        for (unsigned sampleIndex=0;sampleIndex<eligibleParts.count;++sampleIndex) {
            const auto& sample=eligibleParts.samples[sampleIndex];
            const Vector3 sampleTarget{sample.world.x,sample.world.y,sample.world.z};
            const float dx=sampleTarget.x-cameraPos.x;
            const float dy=sampleTarget.y-cameraPos.y;
            const float dz=sampleTarget.z-cameraPos.z;
            const float distance=std::sqrt(dx*dx+dy*dy+dz*dz);
            Evidence e{};
            e.enabled=true;
            e.mode=static_cast<Mode>(modeValue);
            e.isAds=ads;
            e.weaponReady=weaponReady;
            e.localPlayerAlive=alive;
            e.validEnemy=true;
            e.depthValid=true;
            e.pointValid=sample.valid;
            e.crosshair=center;
            e.target=sample.screen;
            e.radius=radius;
            e.targetHalfWidth=0.f;
            e.worldDistance=distance;
            e.ageMs=NowMs()-frame;
            e.wall=Visibility::Clear;
            e.smoke=Visibility::Clear;
            if (!Allowed(e)) continue;
            diagVisibilityProbes.fetch_add(1,std::memory_order_relaxed);
            diagLastSampleFraction.store(sample.fraction,std::memory_order_relaxed);
            Report(DiagnosticStage::LinecastPending);
            HitKind hitKind=HitKind::Unknown;
            float hitDistance=-1.f;                unsigned selfSkips=0;
                RayOrigin usedOrigin=RayOrigin::None;
            // Share the remaining budget between the samples still to test, so the
            // hidden side of a partially covered enemy cannot spend every cast and
            // leave its visible side untested for as long as the pose is the same.
            const unsigned allowance=SampleCastAllowance(castsRemaining,eligibleParts.count-sampleIndex);
            const unsigned castsAtSampleStart=castsRemaining;
            for (unsigned originIndex=0;originIndex<usableCount;++originIndex) {
                if (!castsRemaining) break;
                if (castsAtSampleStart-castsRemaining>=allowance) break;
                HitKind kind=HitKind::Unknown;
                float hit=-1.f;
                unsigned skips=0;
                std::uintptr_t blocker=0;
                const bool clear=LineToTarget(usable[originIndex]->position,sampleTarget,own,
                    reinterpret_cast<void*>(enemy),validity,kind,hit,skips,
                    castsRemaining,&blocker);
                RecordOrigin(usable[originIndex]->id,clear,hit,blocker);
                hitKind=kind;
                hitDistance=hit;
                selfSkips=std::max(selfSkips,skips);
                if (clear) { usedOrigin=usable[originIndex]->id; break; }
            }
            const bool lineClear=usedOrigin!=RayOrigin::None;
            diagHitKind.store(static_cast<unsigned>(hitKind),std::memory_order_relaxed);
            diagHitDistance.store(hitDistance,std::memory_order_relaxed);
            diagHitOrigin.store(static_cast<unsigned>(usedOrigin),std::memory_order_relaxed);
            diagSelfSkips.store(selfSkips,std::memory_order_relaxed);
            diagTargetDistance.store(e.worldDistance,std::memory_order_relaxed);
            if (!lineClear) {
                wallRejected=true;
                if (!castsRemaining) break;
                continue;
            }
            Report(DiagnosticStage::SmokePending);
            const bool concealed=inSmoke(reinterpret_cast<void*>(game),sampleTarget,nullptr) ||
                smokeBetween(reinterpret_cast<void*>(game),cameraPos,sampleTarget,nullptr);
            if (concealed) {
                smokeRejected=true;
                continue;
            }
            if (!modeAllows) { Report(DiagnosticStage::AdsRequired); return false; }
            Report(DiagnosticStage::EligibilityPassed);
            return true;
        }
        if (wallRejected) diagWallRejections.fetch_add(1,std::memory_order_relaxed);
        if (smokeRejected) diagSmokeRejections.fetch_add(1,std::memory_order_relaxed);
        if (!castsRemaining) break;
    }
    // The scan ran but nothing was usable; the reason the shot is refused is still the
    // mode, so that is what the diagnostic should say.
    if (!modeAllows) { Report(DiagnosticStage::AdsRequired); return false; }
    if (diagSmokeRejections.load(std::memory_order_relaxed)) Report(DiagnosticStage::SmokeBlocked);
    else if (diagWallRejections.load(std::memory_order_relaxed)) Report(DiagnosticStage::WallBlocked);
    else if (diagDotMatches.load(std::memory_order_relaxed)) Report(DiagnosticStage::SnapshotUnavailable);
    else if (diagEnemies.load(std::memory_order_relaxed)) Report(DiagnosticStage::NoDotMatch);
    else Report(DiagnosticStage::NoEnemies);
    return false;
}


inline void* CurrentMatchGame() {
    if (!m_unity) return nullptr;
    const auto gameFn=reinterpret_cast<std::uintptr_t(*)()>(Class_Gameplay__get_MatchGame);
    return reinterpret_cast<void*>(gameFn());
}

inline void SuspendTrackedController(void* self) {
    if (!self || self!=lastController) return;
    // Losing the local pawn is a lifecycle boundary even if the allocator later
    // reuses the exact same pawn/match addresses. Don't resume on that first tick.
    if (triggerHeldFire) pendingRelease=true;
    ResetLifeState();
    previousLocalAlive=false;
    lastController=nullptr;
    pawnSettled=matchSettled=false;
    pawnChangedMs=matchChangedMs=NowMs();
}

inline void HookUpdateLocalPawn(void* self, float dt, const void* methodInfo) {
    if (!self || !originalUpdateLocalPawn || !Tools::IsPtrValid(self)) {
        SuspendTrackedController(self);
        Report(DiagnosticStage::LocalControllerMismatch);
        return;
    }
    originalUpdateLocalPawn(self,dt,methodInfo);
    if (!self || !originalUpdateLocalPawn || !Tools::IsPtrValid(self)) {
        SuspendTrackedController(self);
        Report(DiagnosticStage::LocalControllerMismatch);
        return;
    }

    Pawn* local=m_unity ? GamePlay::get_LocalPawn() : nullptr;
    if (!local || !Tools::IsPtrValid(local)) {
        SuspendTrackedController(self);
        Report(DiagnosticStage::LocalControllerMismatch);
        return;
    }
    const auto attachedPawn=*reinterpret_cast<void**>(reinterpret_cast<std::uint8_t*>(self)+0x78);
    if (attachedPawn!=local) {
        SuspendTrackedController(self);
        Report(DiagnosticStage::LocalControllerMismatch);
        return;
    }

    void* match=CurrentMatchGame();
    if (match!=lastMatchGame) {
        // A press issued in the old match must not stay held on a controller that is
        // being destroyed with it. As at a round change, the release is deferred to the
        // first settled frame instead of running against the dying objects.
        if (triggerHeldFire) pendingRelease=true;
        ResetLifeState();
        previousLocalAlive=false;
        lastController=self;
        matchSettled=match && !hasObservedMatch;
        if (match) hasObservedMatch=true;
        lastMatchGame=match;
        matchChangedMs=NowMs();
    }
    if (match && !matchSettled && NowMs()-matchChangedMs>=kMatchSettleMs) matchSettled=true;

    if (local!=lastLocalPawn) {
        // Round change or respawn: drop the press without touching the game, and do
        // nothing else until the new pawn has had a moment to finish initialising.
        if (triggerHeldFire) pendingRelease=true;
        ResetLifeState();
        // The first pawn seen after injection is not a transition, so it is settled at
        // once; only a replacement pawn has to wait out the window.
        pawnSettled=lastLocalPawn==nullptr;
        lastLocalPawn=local;
        pawnChangedMs=NowMs();
        previousLocalAlive=false;
    }
    if (!pawnSettled && NowMs()-pawnChangedMs>=kPawnSettleMs) pawnSettled=true;

    if (self!=lastController) {
        ResetLifeState();
        lastController=self;
        previousLocalAlive=false;
    }

    // Guard every weapon/aim/release call, not just the final firing decision.
    // A readable pointer can still refer to a scene object being torn down.
    if (!match || !matchSettled || !pawnSettled) {
        Report(DiagnosticStage::MatchUnavailable);
        return;
    }

    const bool healthy=*reinterpret_cast<const bool*>(
        reinterpret_cast<std::uintptr_t>(local)+Pawn_m_IsAlive);
    if (!healthy) {
        // The pawn is dead or being destroyed. Releasing the press here would run the
        // game's fire path on the weapon that is going away, so the release is only
        // remembered; it is issued once there is a live pawn to release it on.
        if (triggerHeldFire) pendingRelease=true;
        if (previousLocalAlive || triggerHeldFire || probeActive || fireModeWeapon) {
            ResetLifeState();
        }
        previousLocalAlive=false;
        Report(DiagnosticStage::LocalPlayerUnavailable);
        return;
    }
    if (!previousLocalAlive) {
        if (triggerHeldFire) pendingRelease=true;
        ResetLifeState();
        previousLocalAlive=true;
    }
    diagUpdateCount.fetch_add(1,std::memory_order_relaxed);
    diagLastUpdateMs.store(NowMs(),std::memory_order_relaxed);

    const bool enabledNow=enabled.load(std::memory_order_acquire);
    const void* currentWeapon=local->get_CurrentWeapon();
    const bool weaponHandleOk=currentWeapon &&
        Tools::IsPtrValid(const_cast<void*>(currentWeapon));

    // Round/weapon stability window. Nothing below this point is allowed to call into
    // the game while the triple is still moving.
    if (match!=stableMatch || static_cast<void*>(local)!=stablePawn ||
        const_cast<void*>(currentWeapon)!=stableWeapon) {
        // The round (or the weapon, or the pawn) is changing under a press that is
        // still held. Do not release into the new objects: remember the press and
        // drop the per-weapon state. It is resolved below once the triple settles.
        if (triggerHeldFire) {
            // Never release into the objects that are replacing the ones we pressed:
            // remember the press and clear only the press state. The per-weapon state
            // (fire mode, probe, diagnostics) is reset by the weapon-change path below
            // once the new objects are readable.
            pendingRelease=true;
            triggerHeldFire=false;
            pressAmmoBefore=-1;
            fireTapStartedMs=0;
        }
        stableMatch=match;
        stablePawn=local;
        stableWeapon=const_cast<void*>(currentWeapon);
        stableFrames=0;
    } else if (stableFrames<settleFrames) {
        ++stableFrames;
    }
    if (stableFrames<settleFrames) {
        // The round is still assembling. Keep the remembered release (it is resolved
        // below once the triple settles) and touch nothing else.
        Report(DiagnosticStage::MatchUnavailable);
        return;
    }

    if (pendingRelease) {
        if (!pressWeapon || currentWeapon!=pressWeapon) {
            // A different weapon is held now, so this one never saw the press: the game
            // reset the trigger together with the round. Nothing to release.
            pendingRelease=false;
            pressWeapon=nullptr;
        } else if (weaponHandleOk && stopFire) {
            // The press from the previous pawn is still held in the controller's input
            // state. Release it on the very weapon that was pressed.
            stopFire(self,true,nullptr);
            pendingRelease=false;
            pressWeapon=nullptr;
            Report(DiagnosticStage::FireReleased);
        }
    }

    if (fireModeWeapon!=currentWeapon) {
        fireModeWeapon=currentWeapon;
        // The weapon metadata is read on the first accepted press below, when the
        // weapon is fully active, rather than here where a weapon that has only just
        // been switched to may not be initialised yet. Without a readable clip count
        // there is nothing to probe at all, so spend the budget and go to tapping.
        fireMode=FireMode::Unknown;
        probeActive=false;
        probeRetryAfterMs=0;
        nextPressMs=0;
        pressReissued=false;
        probeFailures=!getClipAmmo ? kMaxProbeAttempts : 0;
        diagProbeSpent.store(probeFailures>=kMaxProbeAttempts,std::memory_order_relaxed);
        if (triggerHeldFire) {
            // Same rule as a round change: never release against a handle that is not
            // readable, keep the release for the first frame that has one.
            if (weaponHandleOk && stopFire) {
                stopFire(self,true,nullptr);
                pressWeapon=nullptr;
            } else {
                // The deferred release is dropped later: it belongs to the old weapon.
                pendingRelease=true;
            }
            triggerHeldFire=false;
            Report(DiagnosticStage::FireReleased);
        }
        diagFireMode.store(static_cast<unsigned>(FireMode::Unknown),std::memory_order_relaxed);
    }
    const bool validApis=startFire && stopFire && isAiming && canFire;
    const bool ads=validApis && weaponHandleOk && healthy ? isAiming(local,nullptr) : false;


    // CanFire(bool ignoreChangingFireModeTransition): scoping in and switching fire
    // mode put the weapon into a short transition during which the default query
    // answers "not yet". Ignoring that single gate lets the press go out on the frame
    // the scope settles instead of one whole transition later; reload, empty
    // magazine, equip time and the shot cooldown still block it, and the game still
    // refuses a round it cannot deliver.
    const bool weaponReady=validApis && weaponHandleOk && healthy &&
        (triggerHeldFire || canFire(local,true,nullptr));
    if (!enabledNow) Report(DiagnosticStage::Disabled);
    else if (!matchSettled) Report(DiagnosticStage::MatchUnavailable);
    else if (!healthy) Report(DiagnosticStage::LocalPlayerUnavailable);
    else if (!validApis || !weaponReady) Report(DiagnosticStage::WeaponNotReady);
    const bool shouldFire=enabledNow && validApis && weaponReady && matchSettled &&
        pawnSettled && !pendingRelease &&
        EligibleFromLocalView(const_cast<void*>(currentWeapon),ads,weaponReady);

    const auto now=NowMs();
    std::int32_t ammoForProbe=0;
    auto probeWanted=[&]() {
        return fireMode==FireMode::Unknown &&
            probeFailures<kMaxProbeAttempts && now>=probeRetryAfterMs;
    };
    if (shouldFire && !triggerHeldFire) {
        // Classify from the weapon itself on the first accepted press, so a
        // semi-automatic taps from the start instead of holding fire to probe the
        // magazine. If the metadata cannot answer, the bounded probe below decides.
        if (fireMode==FireMode::Unknown) {
            const FireMode identified=IdentifyFireMode(const_cast<void*>(currentWeapon));
            if (identified!=FireMode::Unknown) {
                fireMode=identified;
                probeActive=false;
                diagFireMode.store(static_cast<unsigned>(identified),std::memory_order_relaxed);
            }
        }
        const bool ammoAtPress=ReadClipAmmo(const_cast<void*>(currentWeapon),ammoForProbe);
        // An empty magazine cannot produce a round. Pressing it only runs the game's
        // dry-fire path (sound, animation, effects); repeating that at the tap rate for
        // as long as the dot rests on a target is game work that piles up over a long
        // session. Wait out the backoff instead of turning the trigger into a metronome.
        const bool emptyClip=ammoAtPress && ammoForProbe==0 && !canFire(local,false,nullptr);
        if (now<nextPressMs || emptyClip) {
            if (emptyClip) nextPressMs=now+kIgnoredPressBackoffMs;
            Report(DiagnosticStage::WeaponNotReady);
        } else {
        const bool canProbe=probeWanted() && ammoAtPress;

        pressAmmoBefore=ammoAtPress ? ammoForProbe : -1;
        pressReissued=false;
        startFire(self,nullptr);
        triggerHeldFire=true;
        pressWeapon=currentWeapon;
        fireTapStartedMs=now;
        diagFireRequests.fetch_add(1,std::memory_order_relaxed);
        if (canProbe) {


            probeActive=true;
            probeStartedMs=now;
            probeBefore=ammoForProbe;
            probeMid=ammoForProbe;
            probeMidTaken=false;
            diagProbes.fetch_add(1,std::memory_order_relaxed);
        } else if (probeWanted()) {
            // No readable clip count: spend the budget so the tap fallback owns the
            // trigger instead of holding fire forever on an unclassified weapon.
            probeFailures=kMaxProbeAttempts;
            diagProbeSpent.store(true,std::memory_order_relaxed);
        }
        Report(DiagnosticStage::FireRequested);
        }
    } else if (!shouldFire && triggerHeldFire) {
        // isImmidiately=true: a gradual release runs the weapon-lower transition, which
        // the game has to finish before the next StartFire is accepted. That transition
        // is what made the next shot feel delayed, so release instantly instead. If the
        // weapon handle is not readable, defer to the pending release instead of
        // running the game's fire path on it.
        if (weaponHandleOk && stopFire) {
            stopFire(self,true,nullptr);
            pressWeapon=nullptr;
        } else {
            // Resolved against the pressed weapon once the round settles.
            pendingRelease=true;
        }
        triggerHeldFire=false;
        pressAmmoBefore=-1;
        if (probeActive) {

            probeActive=false;
            probeRetryAfterMs=now+probeRetryMs;
        }
        Report(DiagnosticStage::FireReleased);
    } else if (shouldFire && triggerHeldFire) {
        if (probeActive) {
            std::int32_t ammo=0;
            if (!probeMidTaken && now-probeStartedMs>=probeMidMs &&
                ReadClipAmmo(const_cast<void*>(currentWeapon),ammo)) {
                probeMid=ammo;
                probeMidTaken=true;
            }
            if (now-probeStartedMs>=probeEndMs) {
                const bool read=ReadClipAmmo(const_cast<void*>(currentWeapon),ammo);
                const FireMode verdict=read ?
                    ClassifyFireMode(probeBefore,probeMid,ammo) : FireMode::Unknown;
                probeActive=false;
                if (verdict==FireMode::Single) {
                    stopFire(self,true,nullptr);
                    triggerHeldFire=false;
                    pressWeapon=nullptr;
                    pressAmmoBefore=-1;
                    diagFireTaps.fetch_add(1,std::memory_order_relaxed);
                }
                if (verdict==FireMode::Unknown) {
                    if (probeFailures<kMaxProbeAttempts) ++probeFailures;
                    diagProbeSpent.store(probeFailures>=kMaxProbeAttempts,
                                         std::memory_order_relaxed);
                    probeRetryAfterMs=now+probeRetryMs;
                } else {
                    fireMode=verdict;
                    diagFireMode.store(static_cast<unsigned>(verdict),std::memory_order_relaxed);
                }
                Report(DiagnosticStage::FireModeProbed);
            } else {
                Report(DiagnosticStage::FireModeProbe);
            }
        } else if (probeWanted()) {
            // While the trigger stays held there is no new StartFire to hang a probe on,
            // so a retry starts on the held trigger instead of releasing the weapon.
            if (ReadClipAmmo(const_cast<void*>(currentWeapon),ammoForProbe)) {
                probeActive=true;
                probeStartedMs=now;
                probeBefore=ammoForProbe;
                probeMid=ammoForProbe;
                probeMidTaken=false;
                diagProbes.fetch_add(1,std::memory_order_relaxed);
            } else {
                // The magazine cannot be read, so no probe can ever succeed. Fall back
                // to tapping rather than holding a trigger that will never shoot.
                probeFailures=kMaxProbeAttempts;
                probeRetryAfterMs=now+probeRetryMs;
                diagProbeSpent.store(true,std::memory_order_relaxed);
            }
            Report(DiagnosticStage::FireModeProbe);
        } else if (fireMode==FireMode::Auto ||
                   (fireMode==FireMode::Unknown && probeFailures<kMaxProbeAttempts)) {
            // A press issued just before the game entered its fire-mode transition can
            // be dropped. The trigger is already held, so nothing else will ask the
            // weapon to fire again; retry the request once when the weapon says it is
            // ready and the magazine has not moved.
            if (fireMode==FireMode::Auto && !pressReissued &&
                now-fireTapStartedMs>=kIgnoredPressBackoffMs) {
                std::int32_t ammoNow=0;
                const bool haveAmmo=ReadClipAmmo(const_cast<void*>(currentWeapon),ammoNow);
                const bool delivered=pressAmmoBefore>=0 && haveAmmo && ammoNow<pressAmmoBefore;
                if (!delivered && canFire(local,true,nullptr)) {
                    startFire(self,nullptr);
                    fireTapStartedMs=now;
                    if (haveAmmo) pressAmmoBefore=ammoNow;
                    pressReissued=true;
                    diagFireRequests.fetch_add(1,std::memory_order_relaxed);
                }
            }
            // Automatic - and still being classified: keep the trigger held. Chopping
            // every hold into a tapHoldMs burst released and re-pressed the game's fire
            // state machine dozens of times per second, which delayed the next shot (the
            // trigger has to re-arm) and multiplied the game's per-press work (fire
            // requests, effects, audio) over a long session. Verified single-fire weapons
            // still tap below; a weapon that never classifies falls back to tapping so it
            // keeps shooting at all.
            Report(DiagnosticStage::FireHeld);
        } else {
            // Single-fire weapon: one round per press, so the press has to be released
            // before the next one. Release as soon as the round has left the weapon
            // (clip count dropped), not after a fixed hold: during the ADS aim-to-fire
            // interval the game delivers the shot later, and releasing first threw the
            // shot away - which is what made ADS firing feel delayed. If the clip cannot
            // be read, or the weapon reports it is ready to fire again (the press did
            // nothing), release at the old minimum hold; kMaxTapHoldMs bounds a press
            // the game never answers so the trigger cannot stay stuck down.
            const std::int64_t held=now-fireTapStartedMs;
            std::int32_t ammoNow=0;
            const bool delivered=pressAmmoBefore>=0 &&
                ReadClipAmmo(const_cast<void*>(currentWeapon),ammoNow) &&
                ammoNow<pressAmmoBefore;
            const bool readyAgain=!delivered && held>=tapHoldMs &&
                (pressAmmoBefore<0 || canFire(local,true,nullptr));
            if (delivered || readyAgain || held>=kMaxTapHoldMs) {
                const bool ignoredPress=!delivered && pressAmmoBefore>=0;
                stopFire(self,true,nullptr);
                triggerHeldFire=false;
                pressAmmoBefore=-1;
                pressReissued=false;
                // Only a press the magazine proves was ignored (it arrived during a
                // fire-mode transition) earns a backoff; the retry runs the weapon fire
                // path even though no round can come out yet. With an unreadable clip
                // there is no evidence either way, so the tap cadence is kept.
                if (ignoredPress) nextPressMs=now+kIgnoredPressBackoffMs;
                diagFireTaps.fetch_add(1,std::memory_order_relaxed);
                Report(DiagnosticStage::FireTapped);
            } else {
                Report(DiagnosticStage::FireHeld);
            }
        }
    }
}
} // namespace Triggerbot1657
