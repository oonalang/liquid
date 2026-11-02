#pragma once
#include <cstddef>

namespace SkinScanSchedule {

struct Gate {
    std::size_t builtFingerprint = 0;
    unsigned cooldown = 0;

    bool ShouldBuild(std::size_t fingerprint) {
        if (!fingerprint) return false;
        if (cooldown) {
            --cooldown;
            return false;
        }
        if (fingerprint == builtFingerprint) return false;
        cooldown = 1;
        return true;
    }

    void MarkBuilt(std::size_t fingerprint) {
        builtFingerprint = fingerprint;
    }
};

}
