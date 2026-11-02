#include "../src/main/jni/Hacks/Feature/SkinEffectLookup.h"
#include <cassert>
#include <unordered_map>
int main() {
    const std::unordered_map<int,int> effects{{1001,4444},{2002,5555},{3003,0}};
    assert(SkinEffectLookup::Find(effects,{1001,0,0})==4444);
    assert(SkinEffectLookup::Find(effects,{0,2002,0})==5555);
    assert(SkinEffectLookup::Find(effects,{0,0,2002})==5555);
    assert(SkinEffectLookup::Find(effects,{3003,9999,0})==0);
    assert(SkinEffectLookup::Find(effects,{2002,1001,0})==5555);
    assert(SkinEffectLookup::Find(effects,{0,0,0})==0);
}
