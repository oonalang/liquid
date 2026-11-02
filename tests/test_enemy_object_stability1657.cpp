#include "../src/main/jni/Features/EnemyObjectStability1657.h"
#include <cassert>
#include <iostream>

int main() {
    EnemyObjectStability1657::Tracker<2> g;
    assert(!g.Stable(1,10,11,1000));
    assert(!g.Stable(1,10,11,1031));
    assert(g.Stable(1,10,11,1032));
    assert(g.Stable(1,10,11,1100));
    assert(!g.Stable(1,20,11,1101));
    assert(!g.Stable(1,20,11,1132));
    assert(g.Stable(1,20,11,1133));
    assert(!g.Stable(1,20,11,2200));
    assert(g.Stable(1,20,11,2232));
    assert(!g.Stable(0,20,11,2300));
    assert(!g.Stable(2,30,0,2300));
    assert(g.Stable(2,30,0,2332));
    assert(!g.Stable(3,40,0,2300));
    assert(!g.Stable(4,50,0,2300));
    g.Reset();
    assert(!g.Stable(1,20,11,3000));
    std::cout << "object stability: ok\n";
}
