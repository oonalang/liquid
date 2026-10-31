#pragma once
#include <array>
#include <cstdint>

namespace Triggerbot1657 {
enum class LinecastAbi : unsigned { Unverified, Static, StaticWithReceiver };





inline LinecastAbi DecodeLinecastAbi(const std::array<std::uint32_t,3>& code,
                                    std::uintptr_t address,std::uintptr_t callee) {
    if ((code[2]&0xfc000000u)!=0x14000000u) return LinecastAbi::Unverified;
    const std::int64_t displacement=(static_cast<std::int64_t>(code[2]&0x03ffffffu)^0x02000000ll)-0x02000000ll;
    if (static_cast<std::int64_t>(address)+8+displacement*4!=static_cast<std::int64_t>(callee))
        return LinecastAbi::Unverified;
    for (unsigned maskRegister : {1u,2u}) {
        const auto mask=0x92800080u|maskRegister; // mov xN, #-5 (DefaultRaycastLayers)
        const auto query=maskRegister+1;
        auto zero=[&](std::uint32_t instruction) {
            return instruction==(0x52800000u|query) || // mov wN, #0 (UseGlobal)
                   instruction==(0x2a1f03e0u|query);   // mov wN, wzr
        };
        if ((code[0]==mask && zero(code[1])) || (code[1]==mask && zero(code[0])))
            return maskRegister==1 ? LinecastAbi::Static : LinecastAbi::StaticWithReceiver;
    }
    return LinecastAbi::Unverified;
}
}
