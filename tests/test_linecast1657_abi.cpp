#include <cassert>
#include "Features/Triggerbot1657LinecastAbi.h"
using namespace Triggerbot1657;
int main() {

    constexpr std::uintptr_t entry=0x506D104,callee=0x506D07C;
    std::array<std::uint32_t,3> normal{0x92800081,0x52800002,0x17ffffdc};
    std::array<std::uint32_t,3> receiver{0x92800082,0x52800003,0x17ffffdc};
    assert(DecodeLinecastAbi(normal,entry,callee)==LinecastAbi::Static);
    assert(DecodeLinecastAbi(receiver,entry,callee)==LinecastAbi::StaticWithReceiver);
    normal[1]=0x2a1f03e2;
    assert(DecodeLinecastAbi(normal,entry,callee)==LinecastAbi::Static);
    std::swap(receiver[0],receiver[1]);
    assert(DecodeLinecastAbi(receiver,entry,callee)==LinecastAbi::StaticWithReceiver);
    normal[2]=0x97ffffdc; // BL is not the forwarding tail branch.
    assert(DecodeLinecastAbi(normal,entry,callee)==LinecastAbi::Unverified);
    normal[2]=0x17ffffdd; // Wrong callee.
    assert(DecodeLinecastAbi(normal,entry,callee)==LinecastAbi::Unverified);
    normal={0x92800080,0x52800002,0x17ffffdc}; // Mask overwrites out register.
    assert(DecodeLinecastAbi(normal,entry,callee)==LinecastAbi::Unverified);
    normal={0x92800081,0x52800003,0x17ffffdc}; // Unexpected query register.
    assert(DecodeLinecastAbi(normal,entry,callee)==LinecastAbi::Unverified);
    assert(DecodeLinecastAbi({},entry,callee)==LinecastAbi::Unverified);
}
