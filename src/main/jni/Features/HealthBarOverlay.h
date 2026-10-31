#pragma once

#include <algorithm>
#include <cmath>

namespace HealthBarOverlay {
enum class Position { Top = 0, Side = 1 };

inline Position position = Position::Top;

struct Rect { float left=0.f, top=0.f, right=0.f, bottom=0.f; };
struct Geometry {
    Rect bounds{}, fill{};
    float fraction=0.f;
    bool valid=false;
};

inline Geometry Calculate(float headX, float headY, float rootY, float density,
                          float health, float maximum, Position placement) {
    if (!std::isfinite(headX) || !std::isfinite(headY) || !std::isfinite(rootY) ||
        !std::isfinite(density) || density<=0.f || !std::isfinite(health) ||
        !std::isfinite(maximum) || maximum<=0.f) return {};
    const float bodyHeight=std::fabs(rootY-headY);
    if (!std::isfinite(bodyHeight) || bodyHeight<=0.f) return {};

    Geometry result;
    result.fraction=std::clamp(health/maximum,0.f,1.f);
    if (placement==Position::Top) {


        const float width=density/3.2f;
        const float thickness=width*.07f;
        result.bounds={headX-width*.5f,headY-thickness*2.1f,
                       headX+width*.5f,headY-thickness*1.1f};
        result.fill=result.bounds;
        result.fill.right=result.bounds.left+width*result.fraction;
    } else if (placement==Position::Side) {

        const float thickness=std::clamp(density/100.f,2.f,6.f);
        const float gap=std::max(2.f,thickness*.75f);
        const float right=headX-bodyHeight*.65f*.5f-gap;
        result.bounds={right-thickness,headY,right,headY+bodyHeight};
        result.fill=result.bounds;
        result.fill.top=result.bounds.bottom-bodyHeight*result.fraction;
    } else {
        return {};
    }
    result.valid=true;
    return result;
}
} // namespace HealthBarOverlay
