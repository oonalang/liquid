#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <limits>
struct Vector3 { float x,y,z; };
struct Transform { Vector3 pos; Transform* parent=nullptr; Vector3 get_position(){return pos;} };
static int legacyTransformCalls=0;
struct Component { Transform* transform; Transform* get_transform(){++legacyTransformCalls;return transform;} };
struct Collider { Transform* transform; };
struct Camera:Component { static Camera* main; static Camera* get_main(){return main;} };
Camera* Camera::main=nullptr;
struct Pawn { static void* equipped; void* get_CurrentWeapon(){return equipped;} };
void* Pawn::equipped=nullptr;
struct GamePlay { static Pawn* local; static Pawn* get_LocalPawn(){return local;} };
Pawn* GamePlay::local=nullptr;
template <typename T> struct ManagedArray { int length=0; T* data=nullptr; int getLength() const {return length;} };
template <typename T> struct List { ManagedArray<T>* items=nullptr;int count=0; T* getItems(){return items ? items->data : nullptr;}int getSize(){return count;} };
namespace Tools {
inline const void* invalidPointer=nullptr;
inline bool IsPtrValid(const void* p){return p && p!=invalidPointer;}
}
static uintptr_t m_unity=1;
static constexpr size_t Pawn_m_IsAlive=0x548;
static constexpr size_t Class_Pawn__m_IsBot=0x5B9;
static constexpr size_t Class_Pawn__m_HeadBone=0x308;
static constexpr size_t Class_Pawn__m_Mesh=0x628;
static constexpr size_t Class_BaseGame__EnemyPawns=0x178;
static uintptr_t gameAddress=0;
static uintptr_t MatchFn(){return gameAddress;}
#define Class_Gameplay__get_MatchGame (reinterpret_cast<uintptr_t>(&MatchFn))
static int get_width(){return 300;}
static int get_height(){return 400;}
static float projectionScale=1.f;
static Vector3 WorldToScreenPoint(Vector3 p){return {p.x*projectionScale,p.y*projectionScale,p.z};}
#include "Features/Triggerbot1657.h"
static bool physicsBlocked=false,smokeBlocked=false,endpointSmoke=false,ready=true,ads=false;
static int probeAmmo=30;
static int GetClipAmmo(void*,const void*) { return probeAmmo; }
static void* enemyObject=nullptr;
static Transform* enemyObjectTransform=nullptr;
static Collider* hitCollider=nullptr;
static unsigned hitTransformCalls=0;
static float hitDistance=-1.f; // <0 selects the blocked default of 0 units.
static int startCalls=0,stopCalls=0,updateCalls=0;
static void StartFire(void*,const void*) { ++startCalls; }
static void StopFire(void*,bool,const void*) { ++stopCalls; }
static void UpdatePawn(void*,float,const void*) { ++updateCalls; }
static bool Aim(void*,const void*) { return ads; }
static bool CanFire(void*,bool,const void*) { return ready; }
static Vector3 lastFrom{};
static Vector3 firePoint{5,5,6},eyePoint{4.5f,5,6};
static bool fireOriginBlocked=false,eyeOriginBlocked=false;
static float partialCoverY=-1.f;
struct MockAnimator {
    std::array<Transform*,56> bones{};
    bool human=true;
};
static int humanoidBoneReads=0;
static bool MockIsHuman(void* animator,const void*) {
    return static_cast<MockAnimator*>(animator)->human;
}
static Transform* MockBoneTransform(void* animator,int bone,const void*) {
    ++humanoidBoneReads;
    if (bone<0 || bone>=56) return nullptr;
    return static_cast<MockAnimator*>(animator)->bones[static_cast<unsigned>(bone)];
}
static int fireCalls=0;
static Vector3 FirePosition(void*,const void*) { ++fireCalls; return firePoint; }
static Vector3 EyePosition(void*,const void*) { return eyePoint; }
static Transform coverRoot{{0,0,0}};
static Collider coverCollider{&coverRoot};
static int selfHitsToServe=0;
static Collider* selfCollider=nullptr;
static int embeddedHitsToServe=0;
static Collider* embeddedCollider=nullptr;
static bool headOriginBlocked=false,cameraOriginBlocked=false;
static bool triggerVolumeBlocking=false;
static bool Linecast(Vector3 from,Vector3 to,Triggerbot1657::RaycastHit* hit,const void*) {
    lastFrom=from;
    if (triggerVolumeBlocking) {
        hit->collider=&coverCollider;
        hit->distance=7.89f;
        return true;
    }
    if (embeddedHitsToServe>0) {
        --embeddedHitsToServe;
        hit->collider=embeddedCollider;
        hit->distance=0.f; // the cast started on/inside the obstacle
        return true;
    }
    if (selfHitsToServe>0) {
        --selfHitsToServe;
        hit->collider=selfCollider;
        hit->distance=.66f;
        return true;
    }
    if (fireOriginBlocked && from.x==5.f && from.y==5.f && from.z==6.f) {
        hit->collider=&coverCollider;
        hit->distance=.3f;
        return true;
    }
    if (eyeOriginBlocked && from.x==4.5f && from.y==5.f && from.z==6.f) {
        hit->collider=&coverCollider;
        hit->distance=.35f;
        return true;
    }
    if (headOriginBlocked && from.x==4.f && from.y==5.f && from.z==6.f) {
        hit->collider=&coverCollider;
        hit->distance=.5f;
        return true;
    }
    if (cameraOriginBlocked && from.x==0.f && from.y==0.f && from.z==0.f) {
        hit->collider=&coverCollider;
        hit->distance=.5f;
        return true;
    }
    if (partialCoverY>=0.f) {

        if (to.y>partialCoverY) return false;
        hit->collider=&coverCollider;
        hit->distance=7.49f;
        return true;
    }
    hit->collider=hitCollider;
    hit->distance=hitDistance>=0.f ? hitDistance : 0.f;
    return physicsBlocked;
}
static bool LinecastIgnoreTriggers(Vector3 from,Vector3 to,
                                   Triggerbot1657::RaycastHit* hit,const void* method) {
    if (triggerVolumeBlocking) {


        if (!physicsBlocked) return false;
        hit->collider=hitCollider;
        hit->distance=hitDistance>=0.f ? hitDistance : 8.5f;
        return true;
    }
    return Linecast(from,to,hit,method);
}
static bool ColliderIsTrigger(void* collider,const void*) {
    return triggerVolumeBlocking && collider==&coverCollider;
}
static Transform localPawnRoot{{0,0,0}};
static Transform* localPawnTransform=&localPawnRoot;
static Transform* ComponentTransform(void* object,const void*) {
    if (object==Camera::main) return Camera::main->transform;
    if (object==GamePlay::local) return localPawnTransform;
    if (object==enemyObject) return enemyObjectTransform;
    if (object==hitCollider) { ++hitTransformCalls; return hitCollider->transform; }
    if (selfCollider && object==selfCollider) { ++hitTransformCalls; return selfCollider->transform; }
    if (object==&coverCollider) return coverCollider.transform;
    assert(false && "Component getter received an unexpected object");
    return nullptr;
}
static bool IsChildOf(Transform* child,Transform* parent,const void*) {
    for (auto* p=child->parent;p;p=p->parent) if (p==parent) return true;
    return false;
}
static bool SmokeBetween(void*,Vector3,Vector3,const void*){return smokeBlocked;}
static bool InSmoke(void*,Vector3,const void*){return endpointSmoke;}
static Triggerbot1657::RaycastHit* expectedHit=nullptr;
static bool NormalLinecast(Vector3,Vector3,Triggerbot1657::RaycastHit* hit,const void* method) {
    assert(hit==expectedHit && method==nullptr);
    hit->distance=12.f;
    return true;
}
static bool ReceiverLinecast(void* receiver,Vector3,Vector3,Triggerbot1657::RaycastHit* hit,const void* method) {
    assert(receiver==nullptr && hit==expectedHit && method==nullptr);
    hit->distance=13.f;
    return true;
}
static bool MaskedLinecast(Vector3,Vector3,Triggerbot1657::RaycastHit* hit,
                           std::int64_t layers,int triggerMode,const void* method) {
    assert(layers==-5 && triggerMode==1 && hit==expectedHit && method==nullptr);
    hit->distance=14.f;
    return true;
}
static bool MaskedReceiverLinecast(void* receiver,Vector3,Vector3,
                                   Triggerbot1657::RaycastHit* hit,
                                   std::int64_t layers,int triggerMode,const void* method) {
    assert(receiver==nullptr && layers==-5 && triggerMode==1 &&
        hit==expectedHit && method==nullptr);
    hit->distance=15.f;
    return true;
}
int main() {
    std::array<std::uint32_t,3> unreadable{};
    assert(!Triggerbot1657::ReadLinecastWrapper(1,unreadable));

    Triggerbot1657::RaycastHit abiHit{};
    expectedHit=&abiHit;
    Triggerbot1657::nativeLinecastAddress=reinterpret_cast<uintptr_t>(&NormalLinecast);
    Triggerbot1657::nativeLinecastAbi=Triggerbot1657::LinecastAbi::Static;
    assert(Triggerbot1657::NativeLinecast({}, {}, &abiHit,nullptr) && abiHit.distance==12.f);
    Triggerbot1657::nativeLinecastAddress=reinterpret_cast<uintptr_t>(&ReceiverLinecast);
    Triggerbot1657::nativeLinecastAbi=Triggerbot1657::LinecastAbi::StaticWithReceiver;
    assert(Triggerbot1657::NativeLinecast({}, {}, &abiHit,nullptr) && abiHit.distance==13.f);
    Triggerbot1657::nativeLinecastAddress=0;


    Triggerbot1657::nativeMaskedLinecastAddress=reinterpret_cast<uintptr_t>(&MaskedLinecast);
    Triggerbot1657::nativeLinecastAbi=Triggerbot1657::LinecastAbi::Static;
    assert(Triggerbot1657::NativeLinecastFiltered({}, {}, &abiHit,nullptr) && abiHit.distance==14.f);
    Triggerbot1657::nativeMaskedLinecastAddress=reinterpret_cast<uintptr_t>(&MaskedReceiverLinecast);
    Triggerbot1657::nativeLinecastAbi=Triggerbot1657::LinecastAbi::StaticWithReceiver;
    assert(Triggerbot1657::NativeLinecastFiltered({}, {}, &abiHit,nullptr) && abiHit.distance==15.f);
    Triggerbot1657::nativeMaskedLinecastAddress=0;
    Triggerbot1657::nativeLinecastAbi=Triggerbot1657::LinecastAbi::Unverified;

    alignas(16) std::array<unsigned char,0x700> player{};
    alignas(16) std::array<unsigned char,0x700> enemy{};
    alignas(16) std::array<unsigned char,0x300> match{};
    alignas(16) std::array<unsigned char,0x200> controller{};
    int weapon=99;
    Pawn::equipped=&weapon;
    GamePlay::local=reinterpret_cast<Pawn*>(player.data());
    player[Pawn_m_IsAlive]=1; enemy[Pawn_m_IsAlive]=1;
    Transform localHead{{4,5,6}};
    Transform* localHeadPtr=&localHead;
    std::memcpy(player.data()+Class_Pawn__m_HeadBone,&localHeadPtr,sizeof(localHeadPtr));
    Pawn* attached=GamePlay::local;
    std::memcpy(controller.data()+0x78,&attached,sizeof(attached));
    Transform enemyHead{{150,200,1}};
    Transform enemyRoot{{150,170,1}};
    Transform* headPtr=&enemyHead;
    std::memcpy(enemy.data()+Class_Pawn__m_HeadBone,&headPtr,sizeof(headPtr));
    Transform* rootPtr=&enemyRoot;
    std::memcpy(enemy.data()+Class_Pawn__m_Mesh,&rootPtr,sizeof(rootPtr));
    uintptr_t enemyAddress=reinterpret_cast<uintptr_t>(enemy.data());
    enemyObject=enemy.data();
    Transform enemyObjectRoot{{0,0,0}};
    enemyObjectTransform=&enemyObjectRoot;
    ManagedArray<uintptr_t> enemyArray{1,&enemyAddress};
    List<uintptr_t> enemies{&enemyArray,1};
    auto* listPtr=&enemies;
    std::memcpy(match.data()+Class_BaseGame__EnemyPawns,&listPtr,sizeof(listPtr));
    gameAddress=reinterpret_cast<uintptr_t>(match.data());
    Transform cameraTF{{0,0,0}};
    Camera camera{};camera.transform=&cameraTF;
    Camera::main=&camera;
    Triggerbot1657::originalUpdateLocalPawn=&UpdatePawn;
    Triggerbot1657::startFire=&StartFire;
    Triggerbot1657::stopFire=&StopFire;
    Triggerbot1657::isAiming=&Aim;
    Triggerbot1657::canFire=&CanFire;
    Triggerbot1657::physicsLinecast=&Linecast;
    Triggerbot1657::componentTransform=&ComponentTransform;
    Triggerbot1657::transformIsChildOf=&IsChildOf;
    Triggerbot1657::smokeBetween=&SmokeBetween;
    Triggerbot1657::inSmoke=&InSmoke;

    Triggerbot1657::tapHoldMs=1000;
    auto tick=[&](){ Triggerbot1657::HookUpdateLocalPawn(controller.data(),.016f,nullptr); };
    tick();assert(startCalls==0 && stopCalls==0);
    CustomDotCrosshair::settings.enabled=true;
    Triggerbot1657::uiEnabled=true;
    Triggerbot1657::uiMode=Triggerbot1657::Mode::AdsOnly;
    Triggerbot1657::PublishUi(ImVec2{300,400});
    tick();assert(startCalls==0); // ADS only while unscoped
    assert(Triggerbot1657::diagUpdateCount.load()>0);
    assert(Triggerbot1657::diagStage.load()==static_cast<unsigned>(Triggerbot1657::DiagnosticStage::AdsRequired));
    ads=true;enemy[Class_Pawn__m_IsBot]=1;tick();assert(startCalls==1 && stopCalls==0);
    assert(Triggerbot1657::diagBots.load()==1 && Triggerbot1657::diagEnemies.load()==1);
    assert(Triggerbot1657::diagFireRequests.load()==1);
    tick();assert(startCalls==1); // no duplicate starts
    physicsBlocked=true;tick();assert(stopCalls==1);physicsBlocked=false;
    ads=false;tick();assert(startCalls==1);
    Triggerbot1657::uiMode=Triggerbot1657::Mode::Both;
    Triggerbot1657::PublishUi(ImVec2{300,400});
    tick();assert(startCalls==2 && stopCalls==1); // hip-fire allowed
    smokeBlocked=true;tick();assert(stopCalls==2);smokeBlocked=false;
    endpointSmoke=true;tick();assert(startCalls==2);endpointSmoke=false;
    enemyHead.pos.x=250;enemyRoot.pos.x=250;tick();assert(startCalls==2);enemyHead.pos.x=150;enemyRoot.pos.x=150;
    ready=false;tick();assert(startCalls==2);ready=true;
    Triggerbot1657::physicsLinecast=nullptr;tick();assert(startCalls==2);
    Triggerbot1657::physicsLinecast=&Linecast;

    enemyHead.pos.y=290; enemyRoot.pos.y=110;
    tick();assert(startCalls==3);


    enemyHead.pos.z=5001;enemyRoot.pos.z=5001;
    assert(Triggerbot1657::EligibleFromLocalView(&weapon,false,true));
    assert(Triggerbot1657::diagTargetDistance.load()>5000.f);
    physicsBlocked=true;tick();assert(stopCalls==3); // occlusion, not distance
    assert(Triggerbot1657::WithinRange(5001.f));
    assert(Triggerbot1657::WithinRange(1.0e9f));
    physicsBlocked=false;
    enemyHead.pos.z=1;enemyRoot.pos.z=1;
    tick();assert(startCalls==4);


    enemyHead.pos.z=-1;enemyRoot.pos.z=2;
    tick();assert(startCalls==4);
    enemyHead.pos.z=1;enemyRoot.pos.z=1;
    CustomDotCrosshair::settings.enabled=false;
    Triggerbot1657::PublishUi(ImVec2{300,400});
    tick();assert(stopCalls==4);
    Triggerbot1657::uiEnabled=false;
    Triggerbot1657::PublishUi(ImVec2{300,400});
    tick();assert(stopCalls==4 && updateCalls>10);

    attached=nullptr;
    std::memcpy(controller.data()+0x78,&attached,sizeof(attached));
    Triggerbot1657::uiEnabled=true;
    CustomDotCrosshair::settings.enabled=true;
    Triggerbot1657::PublishUi(ImVec2{300,400});
    tick();assert(startCalls==4);

    attached=GamePlay::local;
    std::memcpy(controller.data()+0x78,&attached,sizeof(attached));
    Triggerbot1657::uiMode=Triggerbot1657::Mode::Both;
    Triggerbot1657::PublishUi(ImVec2{300,400});
    auto eligible=[&](){return Triggerbot1657::EligibleFromLocalView(&weapon,false,true);};


    Transform bodyPart{{0,0,0},&enemyObjectRoot};
    Collider selectedCollider{&bodyPart};
    physicsBlocked=true;hitCollider=&selectedCollider;hitDistance=1.f;
    assert(eligible());
    assert(Triggerbot1657::diagHitKind.load()==static_cast<unsigned>(Triggerbot1657::HitKind::SelectedEnemy));
    assert(Triggerbot1657::diagWallRejections.load()==0);
    selectedCollider.transform=&enemyObjectRoot;assert(eligible());
    selectedCollider.transform=&bodyPart;
    Transform otherRoot{{0,0,0}};
    Collider otherCollider{&otherRoot};
    hitCollider=&otherCollider;
    hitDistance=Triggerbot1657::diagTargetDistance.load()-.1f;
    assert(!eligible()); // Cover close to the target is still cover.
    assert(Triggerbot1657::diagHitKind.load()==static_cast<unsigned>(Triggerbot1657::HitKind::Other));
    assert(Triggerbot1657::diagWallRejections.load()==1);
    hitDistance=10000.f;assert(!eligible()); // No distance can grant identity.
    hitCollider=nullptr;assert(!eligible());
    assert(Triggerbot1657::diagHitKind.load()==static_cast<unsigned>(Triggerbot1657::HitKind::Unknown));
    hitCollider=&selectedCollider;
    const auto callsBeforeInvalid=hitTransformCalls;
    Tools::invalidPointer=hitCollider;assert(!eligible());
    assert(hitTransformCalls==callsBeforeInvalid);
    Tools::invalidPointer=nullptr;
    selectedCollider.transform=nullptr;assert(!eligible());
    selectedCollider.transform=&bodyPart;
    enemyObjectTransform=nullptr;assert(!eligible());
    enemyObjectTransform=&enemyObjectRoot;
    Triggerbot1657::transformIsChildOf=nullptr;assert(!eligible());
    Triggerbot1657::transformIsChildOf=&IsChildOf;
    smokeBlocked=true;assert(!eligible()); // Body identity never bypasses smoke.
    smokeBlocked=false;
    physicsBlocked=false;hitCollider=nullptr;hitDistance=-1.f;
    assert(eligible());
    assert(legacyTransformCalls==0); // camera uses the scoped Unity Component getter

    assert(lastFrom.x==4.f && lastFrom.y==5.f && lastFrom.z==6.f); // head origin, one cast




    enemyHead.pos.y=230.f; enemyRoot.pos.y=170.f;
    partialCoverY=203.f;
    assert(eligible());
    assert(Triggerbot1657::diagVisibilityProbes.load()>1);
    assert(Triggerbot1657::diagLastSampleFraction.load()<0.5f);
    assert(Triggerbot1657::diagWallRejections.load()==0);
    smokeBlocked=true;
    assert(!eligible()); // A clear alternate point never bypasses smoke.
    assert(Triggerbot1657::diagSmokeRejections.load()==1);
    smokeBlocked=false;

    partialCoverY=225.f;
    assert(!eligible());
    assert(Triggerbot1657::diagWallRejections.load()==1);

    partialCoverY=260.f;
    assert(!eligible());
    assert(Triggerbot1657::diagHitKind.load()==static_cast<unsigned>(Triggerbot1657::HitKind::Other));

    const auto headRay=static_cast<unsigned>(Triggerbot1657::RayOrigin::Head);
    const auto cameraRay=static_cast<unsigned>(Triggerbot1657::RayOrigin::Camera);
    assert(Triggerbot1657::diagOriginState[headRay].load()==
        static_cast<unsigned>(Triggerbot1657::OriginState::Blocked));
    assert(Triggerbot1657::diagOriginCollider[headRay].load()==
        reinterpret_cast<std::uintptr_t>(&coverCollider));
    assert(Triggerbot1657::diagOriginState[cameraRay].load()==
        static_cast<unsigned>(Triggerbot1657::OriginState::Blocked)); // cover blocks every origin
    partialCoverY=-1.f;
    enemyHead.pos.y=200.f; enemyRoot.pos.y=170.f;
    assert(eligible()); // Restore the clear diagnostic baseline.
    assert(Triggerbot1657::diagOriginCollider[static_cast<unsigned>(
        Triggerbot1657::RayOrigin::Head)].load()==0); // stale blockers cleared




    Triggerbot1657::physicsLinecastFiltered=&LinecastIgnoreTriggers;
    Triggerbot1657::colliderIsTrigger=&ColliderIsTrigger;
    triggerVolumeBlocking=true;
    const auto triggersBefore=Triggerbot1657::diagIgnoredTriggerHits.load();
    assert(eligible());
    assert(Triggerbot1657::diagIgnoredTriggerHits.load()==triggersBefore);
    assert(Triggerbot1657::diagLegacyPhysicsCasts.load()==0);

    assert(eligible());
    assert(Triggerbot1657::diagLegacyPhysicsCasts.load()==0);
    assert(Triggerbot1657::diagIgnoredTriggerHits.load()==triggersBefore);
    assert(Triggerbot1657::diagWallRejections.load()==0);
    physicsBlocked=true;hitCollider=&otherCollider;hitDistance=8.5f;
    assert(!eligible()); // Masking a trigger must not mask solid cover behind it.
    assert(Triggerbot1657::diagWallRejections.load()==1);
    physicsBlocked=false;hitCollider=nullptr;hitDistance=-1.f;
    smokeBlocked=true;
    assert(!eligible()); // Filtering triggers cannot disable smoke checks.
    smokeBlocked=false;
    triggerVolumeBlocking=false;
    physicsBlocked=true;hitCollider=&otherCollider;hitDistance=7.89f;
    assert(!eligible()); // Solid cover still blocks with the filtered API.
    assert(Triggerbot1657::diagWallRejections.load()==1);
    assert(Triggerbot1657::diagPhysicsCasts.load()>0);
    assert(Triggerbot1657::diagReusedPointerValidations.load()>0);
    assert(Triggerbot1657::diagPointerValidations.load()>0);
    physicsBlocked=false;hitCollider=nullptr;hitDistance=-1.f;
    assert(eligible());

    assert(Triggerbot1657::diagIgnoredTriggerHits.load()==triggersBefore);
    Triggerbot1657::physicsLinecastFiltered=nullptr;
    Triggerbot1657::colliderIsTrigger=nullptr;

    std::array<std::uintptr_t,24> crowded{};
    crowded.fill(enemyAddress);
    enemyArray.data=crowded.data();
    enemyArray.length=static_cast<int>(crowded.size());
    enemies.count=static_cast<int>(crowded.size());
    physicsBlocked=true;
    hitCollider=&otherCollider;
    hitDistance=1.f;
    assert(!eligible());
    assert(Triggerbot1657::diagPhysicsCasts.load()==
        Triggerbot1657::kMaxLinecastsPerEvaluation);
    enemies.count=1;
    enemyArray.data=&enemyAddress;
    enemyArray.length=1;
    physicsBlocked=false;
    hitCollider=nullptr;
    hitDistance=-1.f;
    assert(eligible());
    // A clear line of sight now costs exactly one trace (it used to require two).
    assert(Triggerbot1657::diagPhysicsCasts.load()==1);

    // Standing flush against low cover: the trace starts inside the obstacle and reports
    // a zero-distance hit. That must be stepped out of, not treated as a wall.
    embeddedCollider=&coverCollider;
    embeddedHitsToServe=1;
    assert(eligible());
    assert(embeddedHitsToServe==0);
    embeddedHitsToServe=99; // a real wall that keeps returning zero-distance hits
    assert(!eligible());
    assert(Triggerbot1657::kMaxEmbeddedSkips<=Triggerbot1657::kMaxLinecastsPerEvaluation);
    embeddedHitsToServe=0;
    embeddedCollider=nullptr;
    assert(eligible());





    Transform ownLimb{{4,5,6},&localPawnRoot};
    Collider ownCollider{&ownLimb};
    selfCollider=&ownCollider;
    const auto rejectsBeforeSelf=Triggerbot1657::diagWallRejections.load();
    selfHitsToServe=1;physicsBlocked=false;hitCollider=nullptr;hitDistance=-1.f;
    assert(eligible());
    assert(Triggerbot1657::diagSelfSkips.load()==1);
    assert(Triggerbot1657::diagHitOrigin.load()==static_cast<unsigned>(Triggerbot1657::RayOrigin::Head));
    assert(Triggerbot1657::diagWallRejections.load()==rejectsBeforeSelf);

    selfHitsToServe=1;physicsBlocked=true;hitCollider=&otherCollider;
    hitDistance=Triggerbot1657::diagTargetDistance.load()-.1f;
    assert(!eligible());
    assert(Triggerbot1657::diagHitKind.load()==static_cast<unsigned>(Triggerbot1657::HitKind::Other));

    selfHitsToServe=99;physicsBlocked=false;hitCollider=nullptr;hitDistance=-1.f;
    assert(!eligible());
    // The self-skip budget or the per-evaluation cast budget must stop it; either way
    // the shot is refused instead of firing through the player's own model.
    assert(Triggerbot1657::diagSelfSkips.load()==Triggerbot1657::kMaxSelfHitSkips);
    selfHitsToServe=0;



    // Any single clear origin is enough: the muzzle being buried in low cover while the
    // camera sees the enemy must still fire (the game spawns its bullet ray at the camera).
    headOriginBlocked=true;physicsBlocked=false;hitCollider=nullptr;hitDistance=-1.f;
    assert(eligible());
    assert(Triggerbot1657::diagHitOrigin.load()==static_cast<unsigned>(Triggerbot1657::RayOrigin::Camera));
    headOriginBlocked=false;cameraOriginBlocked=true;
    assert(eligible());
    assert(Triggerbot1657::diagHitOrigin.load()==static_cast<unsigned>(Triggerbot1657::RayOrigin::Head));
    cameraOriginBlocked=false;headOriginBlocked=true;cameraOriginBlocked=true;
    assert(!eligible()); // every origin blocked by solid cover
    assert(Triggerbot1657::diagHitOrigin.load()==static_cast<unsigned>(Triggerbot1657::RayOrigin::None));
    headOriginBlocked=false;cameraOriginBlocked=false;selfCollider=nullptr;
    assert(eligible());



    Triggerbot1657::uiMode=Triggerbot1657::Mode::AdsOnly;
    Triggerbot1657::PublishUi(ImVec2{300,400});
    assert(!eligible());
    assert(Triggerbot1657::diagStage.load()==static_cast<unsigned>(Triggerbot1657::DiagnosticStage::AdsRequired));
    Triggerbot1657::uiMode=Triggerbot1657::Mode::Both;
    Triggerbot1657::PublishUi(ImVec2{300,400});
    Camera::main=nullptr;
    assert(!eligible());
    assert(Triggerbot1657::diagStage.load()==static_cast<unsigned>(Triggerbot1657::DiagnosticStage::CameraUnavailable));
    Camera::main=&camera;
    camera.transform=nullptr;
    assert(!eligible());
    assert(Triggerbot1657::diagStage.load()==static_cast<unsigned>(Triggerbot1657::DiagnosticStage::CameraUnavailable));
    camera.transform=&cameraTF;
    const auto savedGame=gameAddress;
    gameAddress=0;
    assert(!eligible());
    assert(Triggerbot1657::diagStage.load()==static_cast<unsigned>(Triggerbot1657::DiagnosticStage::MatchUnavailable));
    gameAddress=savedGame;
    enemies.count=-1;
    assert(!eligible());
    assert(Triggerbot1657::diagStage.load()==static_cast<unsigned>(Triggerbot1657::DiagnosticStage::EnemyListUnavailable));
    enemies.count=0;enemies.items=nullptr;
    assert(!eligible());
    assert(Triggerbot1657::diagStage.load()==static_cast<unsigned>(Triggerbot1657::DiagnosticStage::NoEnemies));
    enemies.count=1;enemies.items=&enemyArray;
    Triggerbot1657::componentTransform=nullptr;assert(!eligible());
    assert(Triggerbot1657::diagStage.load()==static_cast<unsigned>(Triggerbot1657::DiagnosticStage::SnapshotUnavailable));
    Triggerbot1657::componentTransform=&ComponentTransform;
    assert(eligible());


    const auto requestsBeforeStart=Triggerbot1657::diagFireRequests.load();
    tick();
    assert(Triggerbot1657::triggerHeldFire);
    assert(Triggerbot1657::diagFireRequests.load()==requestsBeforeStart+1);
    assert(startCalls==5);
    physicsBlocked=true;tick();
    assert(!Triggerbot1657::triggerHeldFire);
    assert(stopCalls==5);

    hitCollider=&selectedCollider;hitDistance=1.f;
    tick();assert(startCalls==6 && Triggerbot1657::triggerHeldFire);
    assert(Triggerbot1657::diagFireRequests.load()==requestsBeforeStart+2);
    hitCollider=&otherCollider;
    tick();assert(stopCalls==6 && !Triggerbot1657::triggerHeldFire);

    Triggerbot1657::nativeLinecastAddress=1;
    Triggerbot1657::physicsLinecast=nullptr;
    assert(!eligible());
    assert(Triggerbot1657::diagStage.load()==static_cast<unsigned>(Triggerbot1657::DiagnosticStage::LinecastAbiUnavailable));



    Triggerbot1657::nativeLinecastAddress=0;
    Triggerbot1657::physicsLinecast=&Linecast;
    hitCollider=nullptr;hitDistance=-1.f;physicsBlocked=false;
    Triggerbot1657::tapHoldMs=0;
    // An unclassified weapon is held (no artificial burst/release cycle).
    Triggerbot1657::fireMode=Triggerbot1657::FireMode::Unknown;
    const auto startsBeforeHold=startCalls,stopsBeforeHold=stopCalls;
    for (int i=0;i<4;++i) tick();
    assert(startCalls==startsBeforeHold+1 && Triggerbot1657::triggerHeldFire);
    assert(stopCalls==stopsBeforeHold);
    assert(Triggerbot1657::diagStage.load()==static_cast<unsigned>(Triggerbot1657::DiagnosticStage::FireHeld));
    physicsBlocked=true;tick();physicsBlocked=false;
    assert(stopCalls==stopsBeforeHold+1 && !Triggerbot1657::triggerHeldFire);

    // Verified single-fire weapons still tap so the next round can be requested.
    Triggerbot1657::fireMode=Triggerbot1657::FireMode::Single;
    const auto startsBeforeTaps=startCalls,stopsBeforeTaps=stopCalls;
    tick();
    assert(startCalls==startsBeforeTaps+1 && Triggerbot1657::triggerHeldFire);
    tick();
    assert(stopCalls==stopsBeforeTaps+1 && !Triggerbot1657::triggerHeldFire);
    assert(Triggerbot1657::diagFireTaps.load()==1);
    assert(Triggerbot1657::diagStage.load()==static_cast<unsigned>(Triggerbot1657::DiagnosticStage::FireTapped));
    tick();
    assert(startCalls==startsBeforeTaps+2);
    tick();
    assert(stopCalls==stopsBeforeTaps+2);

    tick();
    assert(startCalls==startsBeforeTaps+3);
    physicsBlocked=true;tick();
    assert(stopCalls==stopsBeforeTaps+3 && !Triggerbot1657::triggerHeldFire);
    assert(Triggerbot1657::diagStage.load()==static_cast<unsigned>(Triggerbot1657::DiagnosticStage::FireReleased));


    Triggerbot1657::getClipAmmo=&GetClipAmmo;
    Triggerbot1657::fireMode=Triggerbot1657::FireMode::Unknown;
    Triggerbot1657::tapHoldMs=0;
    Triggerbot1657::probeMidMs=0;
    Triggerbot1657::probeEndMs=1000000;
    Triggerbot1657::probeRetryMs=0;
    probeAmmo=30;

    const auto idleProbes=Triggerbot1657::diagProbes.load();
    const auto idleRequests=Triggerbot1657::diagFireRequests.load();
    tick();tick();
    assert(Triggerbot1657::diagProbes.load()==idleProbes);
    assert(Triggerbot1657::diagFireRequests.load()==idleRequests);
    physicsBlocked=false;


    tick(); // press + probe start
    assert(Triggerbot1657::triggerHeldFire && Triggerbot1657::probeActive);
    assert(Triggerbot1657::diagFireMode.load()==0);
    probeAmmo=29; // exactly one round for the held press
    tick();       // mid sample
    assert(Triggerbot1657::probeMidTaken && Triggerbot1657::probeMid==29);
    Triggerbot1657::probeEndMs=0;
    tick(); // classify and release
    assert(!Triggerbot1657::triggerHeldFire && !Triggerbot1657::probeActive);
    assert(Triggerbot1657::fireMode==Triggerbot1657::FireMode::Single);
    assert(Triggerbot1657::diagFireMode.load()==1);
    const auto probesAfterSingle=Triggerbot1657::diagProbes.load();

    tick();assert(Triggerbot1657::triggerHeldFire && !Triggerbot1657::probeActive);
    tick();assert(!Triggerbot1657::triggerHeldFire);
    assert(Triggerbot1657::diagProbes.load()==probesAfterSingle);


    int secondWeapon=1234;
    Pawn::equipped=&secondWeapon;
    probeAmmo=30;
    Triggerbot1657::probeEndMs=1000000;
    tick();
    assert(Triggerbot1657::fireMode==Triggerbot1657::FireMode::Unknown);
    assert(Triggerbot1657::triggerHeldFire && Triggerbot1657::probeActive);
    probeAmmo=26;
    tick(); // mid sample
    probeAmmo=22; // still consuming late in the hold -> automatic
    Triggerbot1657::probeEndMs=0;
    tick();
    assert(Triggerbot1657::triggerHeldFire);
    assert(Triggerbot1657::fireMode==Triggerbot1657::FireMode::Auto);
    assert(Triggerbot1657::diagFireMode.load()==2);
    const auto probesAfterAuto=Triggerbot1657::diagProbes.load();

    tick();assert(Triggerbot1657::triggerHeldFire);
    tick();assert(Triggerbot1657::triggerHeldFire);
    assert(Triggerbot1657::diagProbes.load()==probesAfterAuto);


    int thirdWeapon=777;
    Pawn::equipped=&thirdWeapon;
    Triggerbot1657::getClipAmmo=nullptr;
    tick();
    assert(Triggerbot1657::fireMode==Triggerbot1657::FireMode::Unknown);
    assert(Triggerbot1657::triggerHeldFire && !Triggerbot1657::probeActive);
    tick();
    assert(Triggerbot1657::triggerHeldFire); // unclassifiable weapon stays held, no gap
    assert(Triggerbot1657::diagProbes.load()==probesAfterAuto);







    physicsBlocked=false;hitCollider=nullptr;hitDistance=-1.f;
    selfHitsToServe=0;smokeBlocked=false;endpointSmoke=false;selfCollider=nullptr;
    Transform localMeshRoot{{4,1.5f,6}};
    Transform* localMeshPtr=&localMeshRoot;
    std::memcpy(player.data()+Class_Pawn__m_Mesh,&localMeshPtr,sizeof(localMeshPtr));
    auto eligibleLocal=[&](){return Triggerbot1657::EligibleFromLocalView(&thirdWeapon,false,true);};
    auto originState=[](Triggerbot1657::RayOrigin origin) {
        return Triggerbot1657::diagOriginState[static_cast<unsigned>(origin)].load();
    };
    auto stateName=[](Triggerbot1657::OriginState state) {return static_cast<unsigned>(state);};

    assert(eligibleLocal());
    assert(Triggerbot1657::diagHitOrigin.load()==static_cast<unsigned>(Triggerbot1657::RayOrigin::Head));
    assert(originState(Triggerbot1657::RayOrigin::Fire)==stateName(Triggerbot1657::OriginState::Unavailable));
    Triggerbot1657::pawnFirePosition=&FirePosition;
    Triggerbot1657::pawnEyePosition=&EyePosition;
    assert(eligibleLocal());
    assert(Triggerbot1657::diagHitOrigin.load()==static_cast<unsigned>(Triggerbot1657::RayOrigin::Fire));
    assert(originState(Triggerbot1657::RayOrigin::Eye)==stateName(Triggerbot1657::OriginState::NotTried)); // eye not needed
    assert(lastFrom.x==5.f && lastFrom.y==5.f && lastFrom.z==6.f); // single cast from the muzzle

    // A blocked muzzle falls through to the next usable origin instead of refusing.
    fireOriginBlocked=true;
    assert(eligibleLocal());
    assert(Triggerbot1657::diagHitOrigin.load()==static_cast<unsigned>(Triggerbot1657::RayOrigin::Eye));
    assert(originState(Triggerbot1657::RayOrigin::Fire)==stateName(Triggerbot1657::OriginState::Blocked));
    assert(Triggerbot1657::diagOriginDistance[static_cast<unsigned>(Triggerbot1657::RayOrigin::Fire)].load()>0.f);

    fireOriginBlocked=false;cameraOriginBlocked=true;
    assert(eligibleLocal());
    assert(originState(Triggerbot1657::RayOrigin::Fire)==stateName(Triggerbot1657::OriginState::Clear));
    assert(originState(Triggerbot1657::RayOrigin::Camera)==stateName(Triggerbot1657::OriginState::NotTried));

    // The reported case: low cover swallows muzzle, eye and head (a player standing
    // flush against it) while the camera still sees the enemy. It must fire.
    cameraOriginBlocked=false;fireOriginBlocked=true;eyeOriginBlocked=true;headOriginBlocked=true;
    assert(eligibleLocal());
    assert(Triggerbot1657::diagHitOrigin.load()==static_cast<unsigned>(Triggerbot1657::RayOrigin::Camera));
    assert(originState(Triggerbot1657::RayOrigin::Fire)==stateName(Triggerbot1657::OriginState::Blocked));
    assert(originState(Triggerbot1657::RayOrigin::Head)==stateName(Triggerbot1657::OriginState::Blocked));
    assert(originState(Triggerbot1657::RayOrigin::Camera)==stateName(Triggerbot1657::OriginState::Clear));

    cameraOriginBlocked=true; // solid cover across every origin
    assert(!eligibleLocal());
    assert(Triggerbot1657::diagHitOrigin.load()==static_cast<unsigned>(Triggerbot1657::RayOrigin::None));
    assert(originState(Triggerbot1657::RayOrigin::Camera)==stateName(Triggerbot1657::OriginState::Blocked));
    fireOriginBlocked=false;eyeOriginBlocked=false;headOriginBlocked=false;cameraOriginBlocked=false;
    assert(eligibleLocal());
    assert(Triggerbot1657::diagHitOrigin.load()==static_cast<unsigned>(Triggerbot1657::RayOrigin::Fire));

    firePoint={150,200,1};
    assert(eligibleLocal());
    assert(Triggerbot1657::diagHitOrigin.load()==static_cast<unsigned>(Triggerbot1657::RayOrigin::Eye));
    assert(originState(Triggerbot1657::RayOrigin::Fire)==stateName(Triggerbot1657::OriginState::Unavailable));
    Triggerbot1657::fireOriginReadFailures=0; // isolate from the sticky gate below
    firePoint={std::numeric_limits<float>::quiet_NaN(),5,6};
    assert(eligibleLocal());
    assert(originState(Triggerbot1657::RayOrigin::Fire)==stateName(Triggerbot1657::OriginState::Unavailable));
    Triggerbot1657::fireOriginReadFailures=0;
    firePoint={5,5,6};
    assert(eligibleLocal());
    assert(Triggerbot1657::diagHitOrigin.load()==static_cast<unsigned>(Triggerbot1657::RayOrigin::Fire));
    assert(Triggerbot1657::diagHitOrigin.load()==static_cast<unsigned>(Triggerbot1657::RayOrigin::Fire));



    std::memset(player.data()+Class_Pawn__m_Mesh,0,sizeof(void*));
    localPawnTransform=nullptr;
    assert(eligibleLocal());
    assert(originState(Triggerbot1657::RayOrigin::Fire)==stateName(Triggerbot1657::OriginState::Clear));
    assert(Triggerbot1657::diagHitOrigin.load()==static_cast<unsigned>(Triggerbot1657::RayOrigin::Fire));


    std::memset(player.data()+Class_Pawn__m_HeadBone,0,sizeof(void*));
    assert(eligibleLocal()); // the camera path is still a valid line of sight
    assert(originState(Triggerbot1657::RayOrigin::Fire)==stateName(Triggerbot1657::OriginState::Unavailable));
    assert(originState(Triggerbot1657::RayOrigin::Eye)==stateName(Triggerbot1657::OriginState::Unavailable));
    assert(Triggerbot1657::diagHitOrigin.load()==static_cast<unsigned>(Triggerbot1657::RayOrigin::Camera));
    std::memcpy(player.data()+Class_Pawn__m_Mesh,&localMeshPtr,sizeof(localMeshPtr));
    std::memcpy(player.data()+Class_Pawn__m_HeadBone,&localHeadPtr,sizeof(localHeadPtr));
    localPawnTransform=&localPawnRoot;
    assert(eligibleLocal());
    assert(Triggerbot1657::diagHitOrigin.load()==static_cast<unsigned>(Triggerbot1657::RayOrigin::Fire));

    Transform muzzleLimb{{5,5,6},&localPawnRoot};
    Collider muzzleCollider{&muzzleLimb};
    selfCollider=&muzzleCollider;
    selfHitsToServe=1;
    assert(eligibleLocal());
    assert(Triggerbot1657::diagSelfSkips.load()==1);
    assert(Triggerbot1657::diagHitOrigin.load()==static_cast<unsigned>(Triggerbot1657::RayOrigin::Fire));
    selfHitsToServe=0;selfCollider=nullptr;
    Triggerbot1657::pawnFirePosition=nullptr;
    Triggerbot1657::pawnEyePosition=nullptr;
    assert(eligibleLocal());
    assert(Triggerbot1657::diagHitOrigin.load()==static_cast<unsigned>(Triggerbot1657::RayOrigin::Head));




    Transform meshLimb{{4,5,6},&localMeshRoot};
    Collider meshCollider{&meshLimb};
    selfCollider=&meshCollider;selfHitsToServe=1;
    assert(eligibleLocal());
    assert(Triggerbot1657::diagSelfSkips.load()==1);


    Transform playerObject{{0,0,0}};
    localPawnRoot.parent=&playerObject;
    Collider objectCollider{&playerObject};
    selfCollider=&objectCollider;selfHitsToServe=1;
    assert(eligibleLocal());
    assert(Triggerbot1657::diagSelfSkips.load()==1);
    localPawnRoot.parent=nullptr;

    Transform viewModel{{4,5,6},&cameraTF};
    Collider viewModelCollider{&viewModel};
    selfCollider=&viewModelCollider;selfHitsToServe=1;
    assert(eligibleLocal());
    assert(Triggerbot1657::diagSelfSkips.load()==1);

    physicsBlocked=true;hitCollider=&otherCollider;hitDistance=1.f;
    selfCollider=&coverCollider;selfHitsToServe=1;
    assert(!eligibleLocal());
    assert(Triggerbot1657::diagSelfSkips.load()==0);
    assert(Triggerbot1657::diagHitKind.load()==static_cast<unsigned>(Triggerbot1657::HitKind::Other));
    selfHitsToServe=0;selfCollider=nullptr;physicsBlocked=false;hitCollider=nullptr;hitDistance=-1.f;




    Triggerbot1657::pawnFirePosition=&FirePosition;
    Triggerbot1657::pawnEyePosition=&EyePosition;
    firePoint={5,5,6};
    Triggerbot1657::fireOriginReadFailures=0;
    assert(eligibleLocal());
    assert(Triggerbot1657::diagHitOrigin.load()==static_cast<unsigned>(Triggerbot1657::RayOrigin::Fire));
    firePoint={150,200,1};
    for (unsigned i=0;i<Triggerbot1657::kMaxOriginReadFailures;++i) (void)eligibleLocal();
    const auto gatedCalls=fireCalls;
    firePoint={5,5,6};
    assert(eligibleLocal());
    assert(fireCalls==gatedCalls);
    assert(originState(Triggerbot1657::RayOrigin::Fire)==stateName(Triggerbot1657::OriginState::Unavailable));
    assert(Triggerbot1657::diagHitOrigin.load()==static_cast<unsigned>(Triggerbot1657::RayOrigin::Eye));
    bool recovered=false;
    for (int i=0;i<70;++i) {
        assert(eligibleLocal());
        if (fireCalls>gatedCalls) { recovered=true; break; }
    }
    assert(recovered);
    assert(originState(Triggerbot1657::RayOrigin::Fire)==stateName(Triggerbot1657::OriginState::Clear));





    int fourthWeapon=4242;
    Pawn::equipped=&fourthWeapon;
    Triggerbot1657::getClipAmmo=&GetClipAmmo;
    Triggerbot1657::tapHoldMs=0;
    Triggerbot1657::probeMidMs=0;
    Triggerbot1657::probeEndMs=0;
    Triggerbot1657::probeRetryMs=0;
    Triggerbot1657::pawnFirePosition=nullptr;
    Triggerbot1657::pawnEyePosition=nullptr;
    probeAmmo=30;
    const auto probesBeforeBudget=Triggerbot1657::diagProbes.load();
    tick(); // weapon switch resets the budget and starts the first probe
    assert(Triggerbot1657::triggerHeldFire && Triggerbot1657::probeActive);
    assert(Triggerbot1657::diagProbes.load()==probesBeforeBudget+1);
    tick(); // magazine never moves: failed probe #1
    assert(Triggerbot1657::triggerHeldFire && !Triggerbot1657::probeActive); // stays held
    assert(Triggerbot1657::fireMode==Triggerbot1657::FireMode::Unknown);
    assert(Triggerbot1657::probeFailures==1);
    assert(!Triggerbot1657::diagProbeSpent.load());
    tick(); // one more attempt is allowed
    assert(Triggerbot1657::triggerHeldFire && Triggerbot1657::probeActive);
    tick(); // failed probe #2: budget spent
    assert(Triggerbot1657::triggerHeldFire);
    assert(Triggerbot1657::probeFailures==2);
    assert(Triggerbot1657::diagProbeSpent.load());
    const auto probesSpent=Triggerbot1657::diagProbes.load();
    assert(probesSpent==probesBeforeBudget+2);
    for (int i=0;i<8;++i) tick();
    assert(Triggerbot1657::diagProbes.load()==probesSpent); // held, no further probes
    assert(Triggerbot1657::fireMode==Triggerbot1657::FireMode::Unknown);

    int fifthWeapon=5151;
    Pawn::equipped=&fifthWeapon;
    tick();
    assert(Triggerbot1657::diagProbes.load()==probesSpent+1);
    assert(!Triggerbot1657::diagProbeSpent.load());

    alignas(16) std::array<unsigned char,0x200> otherController{};
    void* remotePawn=reinterpret_cast<void*>(enemy.data());
    std::memcpy(otherController.data()+0x78,&remotePawn,sizeof(remotePawn));
    Triggerbot1657::lastController=controller.data();
    Triggerbot1657::triggerHeldFire=true;
    Triggerbot1657::probeActive=true;
    const int callsBeforeOther=updateCalls;
    const auto updatesBeforeOther=Triggerbot1657::diagUpdateCount.load();
    Triggerbot1657::HookUpdateLocalPawn(otherController.data(),.016f,nullptr);
    assert(updateCalls==callsBeforeOther+1);
    assert(Triggerbot1657::diagUpdateCount.load()==updatesBeforeOther);
    assert(Triggerbot1657::lastController==controller.data());
    assert(Triggerbot1657::triggerHeldFire && Triggerbot1657::probeActive);

    Tools::invalidPointer=reinterpret_cast<void*>(0x1);
    Triggerbot1657::HookUpdateLocalPawn(const_cast<void*>(Tools::invalidPointer),.016f,nullptr);
    assert(Triggerbot1657::lastController==controller.data());
    Tools::invalidPointer=nullptr;

    Triggerbot1657::fireMode=Triggerbot1657::FireMode::Auto;
    Triggerbot1657::probeFailures=2;
    Triggerbot1657::fireOriginReadFailures=8;
    Triggerbot1657::eyeOriginReadFailures=8;
    Triggerbot1657::diagProbeSpent.store(true);
    const auto stopsBeforeDeath=stopCalls;
    const auto startsBeforeDeath=startCalls;
    player[Pawn_m_IsAlive]=0;
    tick();
    assert(stopCalls==stopsBeforeDeath+1);
    assert(startCalls==startsBeforeDeath);
    assert(!Triggerbot1657::triggerHeldFire);
    assert(!Triggerbot1657::probeActive);
    assert(!Triggerbot1657::previousLocalAlive);
    assert(Triggerbot1657::fireMode==Triggerbot1657::FireMode::Unknown);
    assert(Triggerbot1657::fireModeWeapon==nullptr);
    assert(Triggerbot1657::probeFailures==0);
    assert(Triggerbot1657::fireOriginReadFailures==0);
    assert(Triggerbot1657::eyeOriginReadFailures==0);
    assert(!Triggerbot1657::diagProbeSpent.load());
    assert(Triggerbot1657::diagUpdateCount.load()==0);
    assert(Triggerbot1657::diagFireRequests.load()==0);
    assert(Triggerbot1657::diagProbes.load()==0);
    for (int i=0;i<8;++i) tick();
    assert(stopCalls==stopsBeforeDeath+1);
    assert(startCalls==startsBeforeDeath);
    assert(Triggerbot1657::diagUpdateCount.load()==0);

    player[Pawn_m_IsAlive]=1;
    Triggerbot1657::probeEndMs=1000000;
    tick();
    assert(Triggerbot1657::previousLocalAlive);
    assert(Triggerbot1657::lastController==controller.data());
    assert(startCalls==startsBeforeDeath+1);
    assert(Triggerbot1657::triggerHeldFire);
    assert(Triggerbot1657::probeActive);
    assert(Triggerbot1657::diagProbes.load()==1);
    assert(Triggerbot1657::diagUpdateCount.load()==1);
    tick();
    assert(Triggerbot1657::diagUpdateCount.load()==2);
    assert(startCalls==startsBeforeDeath+1);
    player[Pawn_m_IsAlive]=0;
    tick();
    assert(stopCalls==stopsBeforeDeath+2);
    assert(!Triggerbot1657::triggerHeldFire);
    assert(!Triggerbot1657::probeActive);
    tick();
    assert(stopCalls==stopsBeforeDeath+2);

    player[Pawn_m_IsAlive]=1;
    tick();
    assert(startCalls==startsBeforeDeath+2);
    assert(Triggerbot1657::diagProbes.load()==1);

    GamePlay::local=nullptr;
    tick();
    assert(Triggerbot1657::lastController==nullptr);
    assert(!Triggerbot1657::triggerHeldFire);
    assert(!Triggerbot1657::probeActive);
    GamePlay::local=reinterpret_cast<Pawn*>(player.data());
    tick();
    assert(Triggerbot1657::lastController==controller.data());
    assert(startCalls==startsBeforeDeath+3);

    {
        projectionScale=20.f;
        auto eligibleRig=[&](){return Triggerbot1657::EligibleFromLocalView(Pawn::equipped,false,true);};
        enemyHead.pos={9.f,10.2f,1.f};
        enemyRoot.pos={9.f,9.7f,1.f};
        Transform rigHead{{9.f,10.2f,1.f}};
        Transform rigChest{{9.f,10.f,1.f}};
        Transform rigHips{{9.f,9.7f,1.f}};
        Transform rigLeftShoulder{{8.6f,10.1f,1.f}};
        Transform rigLeftUpperArm{{7.6f,10.0f,1.f}};
        Transform rigLeftLowerArm{{7.5f,10.0f,1.f}};
        Transform rigRightUpperLeg{{9.1f,9.5f,1.f}};
        Transform rigRightLowerLeg{{9.1f,9.3f,1.f}};
        MockAnimator rig{};
        rig.bones[10]=&rigHead;
        rig.bones[8]=&rigChest;
        rig.bones[0]=&rigHips;
        rig.bones[11]=&rigLeftShoulder;
        rig.bones[13]=&rigLeftUpperArm;
        rig.bones[15]=&rigLeftLowerArm;
        rig.bones[2]=&rigRightUpperLeg;
        rig.bones[4]=&rigRightLowerLeg;
        alignas(16) std::array<unsigned char,0x100> animation{};
        void* rigPointer=&rig;
        void* animationPointer=animation.data();
        std::memcpy(animation.data()+0x20,&rigPointer,sizeof(rigPointer));
        std::memcpy(enemy.data()+0x538,&animationPointer,sizeof(animationPointer));
        Triggerbot1657::animatorIsHuman=&MockIsHuman;
        Triggerbot1657::animatorBoneTransform=&MockBoneTransform;
        physicsBlocked=false;
        smokeBlocked=false;
        endpointSmoke=false;
        assert(eligibleRig());
        assert(humanoidBoneReads>0);
        rigLeftUpperArm.pos={9.f,10.f,1.f};
        rigLeftLowerArm.pos={9.f,10.f,1.f};
        assert(!eligibleRig());
        rigLeftUpperArm.pos={7.6f,10.f,1.f};
        assert(eligibleRig());
        cameraOriginBlocked=true;
        assert(eligibleRig()); // the head origin is still clear
        headOriginBlocked=true;
        assert(!eligibleRig()); // every usable origin blocked
        headOriginBlocked=false;
        cameraOriginBlocked=false;
        smokeBlocked=true;
        assert(!eligibleRig());
        smokeBlocked=false;
        rig.human=false;
        assert(!eligibleRig());
        rig.human=true;
        assert(eligibleRig());
        Triggerbot1657::animatorIsHuman=nullptr;
        Triggerbot1657::animatorBoneTransform=nullptr;
        animationPointer=nullptr;
        std::memcpy(enemy.data()+0x538,&animationPointer,sizeof(animationPointer));
        projectionScale=1.f;
    }

    EnemyListSnapshot1657::Snapshot snapshot{};
    auto validListPointer=[](const void* p){ return p!=nullptr; };
    enemyArray.length=0;
    assert(!EnemyListSnapshot1657::Read(&enemies,validListPointer,snapshot));
    enemyArray.length=1;
    assert(EnemyListSnapshot1657::Read(&enemies,validListPointer,snapshot));
}
