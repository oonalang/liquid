#include <cassert>
#include <cmath>
#include "Features/HealthBarOverlay.h"

using namespace HealthBarOverlay;
static bool Near(float a,float b) { return std::fabs(a-b)<.001f; }

int main() {
    assert(position==Position::Top);
    const auto top=Calculate(200.f,100.f,300.f,320.f,50.f,100.f,Position::Top);
    assert(top.valid && Near(top.fraction,.5f));
    assert(Near(top.bounds.left,150.f) && Near(top.bounds.right,250.f));
    assert(top.bounds.bottom<100.f && Near(top.fill.right,200.f));

    const auto side=Calculate(200.f,100.f,300.f,320.f,50.f,100.f,Position::Side);
    assert(side.valid && Near(side.bounds.top,100.f) && Near(side.bounds.bottom,300.f));
    assert(side.bounds.right<135.f); // Left of the projected box, including a gap.
    assert(Near(side.fill.top,200.f) && Near(side.fill.bottom,300.f));
    assert(Near(side.fill.left,side.bounds.left) && Near(side.fill.right,side.bounds.right));
    const auto full=Calculate(200,100,300,320,150,100,Position::Side);
    assert(full.valid && full.fraction==1.f && Near(full.fill.top,full.bounds.top));
    const auto empty=Calculate(200,100,300,320,-10,100,Position::Side);
    assert(empty.valid && empty.fraction==0.f && Near(empty.fill.top,empty.fill.bottom));
    assert(!Calculate(200,100,300,320,50,0,Position::Side).valid);
    assert(!Calculate(200,100,300,320,NAN,100,Position::Top).valid);
    assert(!Calculate(200,100,300,0,50,100,Position::Top).valid);
    assert(!Calculate(200,100,100,320,50,100,Position::Side).valid);
    assert(!Calculate(200,100,300,320,50,100,static_cast<Position>(2)).valid);
}
