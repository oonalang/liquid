#include <cassert>
#include <cmath>
#include "Features/CustomDotCrosshair.h"
using namespace CustomDotCrosshair;
int main() {
    Settings s;
    assert(!s.enabled && !s.showFiringZoneGuide);
    for (ImVec2 sz : {ImVec2(1920,1080), ImVec2(1080,1920), ImVec2(2340,1080), ImVec2(1080,2340), ImVec2(720,1600)}) {
        auto g=Calculate(sz,s);
        assert(std::abs(g.center.x-sz.x*.5f)<.01f);
        assert(std::abs(g.center.y-sz.y*.5f)<.01f);
        assert(g.firingRadius>g.dotRadius);
        assert(IntersectsFiringZone(g, ImVec2(g.center.x-1,g.center.y-1), ImVec2(g.center.x+1,g.center.y+1)));
        assert(!IntersectsFiringZone(g, ImVec2(0,0), ImVec2(1,1)));
        assert(!IntersectsFiringZone(g, ImVec2(10,10), ImVec2(9,9)));
    }
    s.centerOffsetX=10; s.centerOffsetY=-9;
    auto off=Calculate(ImVec2(1080,1920),s);
    assert(off.center.x>540 && off.center.y<960);
    assert(!IntersectsFiringZone(off, ImVec2(NAN,0), ImVec2(10,10)));
    return 0;
}
