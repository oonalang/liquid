#include <cstdint>
#include <cmath>
#include <cstddef>

struct Vector3 { float x,y,z; };
struct Transform { Vector3 get_position(); };
struct Pawn { void* get_CurrentWeapon(); };
struct Camera { static Camera* get_main(); };
struct GamePlay { static Pawn* get_LocalPawn(); };
struct Tools { static bool IsPtrValid(void*); };
template<class T> struct List {
    struct Array { int getLength(); };
    Array* items;
    int getSize();
    T* getItems();
};
inline std::uintptr_t m_unity=0;
constexpr std::uintptr_t Class_Gameplay__get_MatchGame=0;
constexpr std::uintptr_t Class_BaseGame__EnemyPawns=0x120;
constexpr std::uintptr_t Class_Pawn__m_HeadBone=0x308;
constexpr std::uintptr_t Class_Pawn__m_Mesh=0x628;
constexpr std::uintptr_t Class_Pawn__m_IsBot=0x5b9;
constexpr std::uintptr_t Pawn_m_IsAlive=0x548;
Vector3 WorldToScreenPoint(Vector3);
int get_width();
int get_height();
#include "../src/main/jni/Features/Triggerbot1657.h"
int main() {return 0;}
