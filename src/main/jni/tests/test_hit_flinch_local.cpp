#include <cstdint>
#include <cstring>
struct Vector2 {float x,y; Vector2(float xx,float yy):x(xx),y(yy) {}};
struct Pawn {void* weapon=nullptr;void* get_CurrentWeapon() {return weapon;}};
uintptr_t m_unity=1;
Pawn ownPawn, otherPawn;
namespace GamePlay {Pawn* get_LocalPawn(){return &ownPawn;}}
#include "../Features/Gameplay1657.h"
#include <cassert>
#include <iostream>
int calledWeapon=0, calledFire=0, calledInstant=0,calledProjectile=0;
void fallbackWeapon(void*,float){++calledWeapon;}
void fallbackFire(void*,float){++calledFire;}
void fallbackInstant(void*,float){++calledInstant;}
void fallbackProjectile(void*,float){++calledProjectile;}
int main() {
    using namespace Gameplay1657;
    char weapon[96]{};
    char opponentWeapon[96]{};
    ownPawn.weapon=weapon;
    otherPawn.weapon=opponentWeapon;
    alignas(void*) char component[96]{};
    alignas(void*) char opponentComponent[96]{};
    void* own=&ownPawn,*enemy=&otherPawn;
    std::memcpy(component+0x28,&own,sizeof(void*));
    std::memcpy(opponentComponent+0x28,&enemy,sizeof(void*));
    origWeaponHitFlinch=fallbackWeapon;
    origFireComponentHitFlinch=fallbackFire;
    origInstantHitFlinch=fallbackInstant;
    origProjectileHitFlinch=fallbackProjectile;
    uiNoHitFlinch=true;
    SyncFromUi();
    HookWeaponHitFlinch(weapon,1.f);
    HookFireComponentHitFlinch(component,1.f);
    HookInstantHitFlinch(component,1.f);
    HookProjectileHitFlinch(component,1.f);
    assert(calledWeapon==0 && calledFire==0 && calledInstant==0 && calledProjectile==0);
    HookWeaponHitFlinch(opponentWeapon,1.f);
    HookFireComponentHitFlinch(opponentComponent,1.f);
    HookInstantHitFlinch(opponentComponent,1.f);
    HookProjectileHitFlinch(opponentComponent,1.f);
    assert(calledWeapon==1 && calledFire==1 && calledInstant==1 && calledProjectile==1);
    uiNoHitFlinch=false;
    SyncFromUi();
    HookWeaponHitFlinch(weapon,1.f);
    HookFireComponentHitFlinch(component,1.f);
    HookInstantHitFlinch(component,1.f);
    HookProjectileHitFlinch(component,1.f);
    assert(calledWeapon==2 && calledFire==2 && calledInstant==2 && calledProjectile==2);
    std::cout << "hit flinch local scope tests passed\n";
}
