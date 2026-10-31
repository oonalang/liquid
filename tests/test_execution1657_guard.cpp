#include <cassert>
#include <cmath>
#include <cstdint>
#include "Features/Execution1657Guard.h"
static int calls=0;
static bool validWithDistance(void*,void*,float* output) { ++calls; *output=55.f; return false; }
static bool invalidWithoutDistance(void*,void*,float*) { ++calls; return false; }
static bool rejectsWithDistance(void*,void*,float* output) { ++calls; *output=-1.f; return false; }
static bool normal(void*,void*,float* output) { ++calls; *output=7.f; return true; }
int main() {
    int object=1;float dist=123.f;
    assert(Execution1657::ValidateWithDistance(&object,&object,&dist,true,validWithDistance));
    assert(calls==1 && dist==55.f);
    assert(!Execution1657::ValidateWithDistance(&object,&object,&dist,true,invalidWithoutDistance));
    assert(calls==2 && dist==0.f);
    assert(!Execution1657::ValidateWithDistance(&object,&object,&dist,true,rejectsWithDistance));
    assert(dist==0.f);
    assert(!Execution1657::ValidateWithDistance(&object,&object,&dist,false,validWithDistance));
    assert(dist==55.f);
    assert(Execution1657::ValidateWithDistance(&object,&object,&dist,false,normal));
    assert(!Execution1657::ValidateWithDistance(&object,&object,&dist,true,nullptr));
}
