#include "../src/main/jni/Features/EspBoxGuard1657.h"
#include <cassert>
int main() {
    EspBoxGuard1657::Tracker g;
    const EspBoxGuard1657::Scene scene{1,2,3,4};
    assert(!g.Observe(scene));
    assert(g.Observe(scene));
    assert(!g.GeometryStable(100,200,300,1000));
    assert(!g.GeometryStable(100,200,300,1031));
    assert(g.GeometryStable(100,200,300,1032));
    assert(g.GeometryStable(100,200,300,1040));
    assert(!g.GeometryStable(100,201,300,1041));
    assert(!g.GeometryStable(100,201,300,1072));
    assert(g.GeometryStable(100,201,300,1073));
    assert(!g.GeometryStable(100,201,300,2200));
    assert(g.GeometryStable(100,201,300,2233));
    assert(!g.Observe({1,2,5,4}));
    assert(!g.GeometryStable(100,201,300,2234));
    assert(g.GeometryStable(100,201,300,2270));
    assert(!g.Observe({0,0,0,0}));
    assert(!g.GeometryStable(100,201,300,2300));
    assert(!g.Observe(scene));
    assert(!g.GeometryStable(100,201,300,2400));
}
