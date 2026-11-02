#include "../src/main/jni/Hacks/Feature/SkinScanSchedule.h"
#include <cassert>
#include <cstddef>

int main() {
    SkinScanSchedule::Gate g;
    assert(g.ShouldBuild(100));
    g.MarkBuilt(100);
    assert(!g.ShouldBuild(101));
    assert(g.ShouldBuild(102));
    g.MarkBuilt(102);
    assert(!g.ShouldBuild(103));
    assert(g.ShouldBuild(104));
    g.MarkBuilt(104);
    assert(!g.ShouldBuild(104));
    assert(!g.ShouldBuild(104));
    assert(g.ShouldBuild(105));
    g.MarkBuilt(105);

    SkinScanSchedule::Gate failed;
    assert(!failed.ShouldBuild(0));
    assert(failed.ShouldBuild(7));
    assert(!failed.ShouldBuild(7));
    assert(failed.ShouldBuild(7));
    failed.MarkBuilt(7);
    assert(!failed.ShouldBuild(7));
    assert(failed.ShouldBuild(8));
}
