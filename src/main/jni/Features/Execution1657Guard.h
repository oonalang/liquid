#pragma once


#include <cmath>
#include <limits>

namespace Execution1657 {
using OriginalCheck = bool(*)(void*,void*,float*);
inline bool ValidateWithDistance(void* owner,void* target,float* distance,
                                 bool extended,OriginalCheck original) {
    if (!original) return false;
    if (!extended || !owner || !target || !distance)
        return original(owner,target,distance);


    *distance=std::numeric_limits<float>::quiet_NaN();
    const bool originalResult=original(owner,target,distance);
    if (!std::isfinite(*distance) || *distance<0.f) {
        *distance=0.f;
        return originalResult;
    }
    return true;
}
} // namespace Execution1657
