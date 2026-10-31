#pragma once


#include "../ImGui/imgui.h"
#include <algorithm>
#include <cmath>

namespace CustomDotCrosshair {
struct Settings {
    bool enabled = false;
    float dotSize = 3.5f;
    float color[3] = {0.72f, 0.88f, 1.0f};
    float opacity = 1.0f;
    bool outline = false;
    float outlineThickness = 1.5f;
    float centerOffsetX = 0.0f;
    float centerOffsetY = 0.0f;
    float firingRadius = 14.0f;
    bool showFiringZoneGuide = false;
};


inline Settings settings{};

struct Geometry {
    ImVec2 center;
    float dotRadius;
    float firingRadius;
};

inline Geometry Calculate(const ImVec2& displaySize, const Settings& s) {


    const float minSide = std::max(1.0f, std::min(displaySize.x, displaySize.y));
    const float scale = std::max(0.5f, std::min(3.0f, minSide / 720.0f));
    const float ox = std::max(-60.0f, std::min(60.0f, s.centerOffsetX));
    const float oy = std::max(-60.0f, std::min(60.0f, s.centerOffsetY));
    return {
        ImVec2(displaySize.x * 0.5f + ox * scale,
               displaySize.y * 0.5f + oy * scale),
        std::max(0.5f, std::min(20.0f, s.dotSize)) * scale,
        std::max(1.0f, std::min(160.0f, s.firingRadius)) * scale
    };
}



inline bool IntersectsFiringZone(const Geometry& g, const ImVec2& minimum,
                                 const ImVec2& maximum) {
    if (!std::isfinite(g.center.x) || !std::isfinite(g.center.y) ||
        !std::isfinite(g.firingRadius) || g.firingRadius < 0.0f ||
        !std::isfinite(minimum.x) || !std::isfinite(minimum.y) ||
        !std::isfinite(maximum.x) || !std::isfinite(maximum.y) ||
        minimum.x > maximum.x || minimum.y > maximum.y) return false;
    const float x = std::max(minimum.x, std::min(maximum.x, g.center.x));
    const float y = std::max(minimum.y, std::min(maximum.y, g.center.y));
    const float dx = g.center.x - x;
    const float dy = g.center.y - y;
    return dx * dx + dy * dy <= g.firingRadius * g.firingRadius;
}

inline void Draw(ImDrawList* drawing, const ImVec2& displaySize,
                 const Settings& s = settings) {
    if (!s.enabled || drawing == nullptr || !std::isfinite(displaySize.x) ||
        !std::isfinite(displaySize.y) || displaySize.x <= 0 || displaySize.y <= 0)
        return;

    const Geometry geometry = Calculate(displaySize, s);
    const float alpha = std::max(0.0f, std::min(1.0f, s.opacity));
    const ImU32 fill = ImGui::ColorConvertFloat4ToU32(ImVec4(
        std::max(0.f, std::min(1.f, s.color[0])),
        std::max(0.f, std::min(1.f, s.color[1])),
        std::max(0.f, std::min(1.f, s.color[2])), alpha));

    if (s.showFiringZoneGuide) {
        const ImU32 guide = ImGui::ColorConvertFloat4ToU32(
            ImVec4(s.color[0], s.color[1], s.color[2], alpha * .45f));
        drawing->AddCircle(geometry.center, geometry.firingRadius, guide, 72, 1.0f);
    }
    if (s.outline) {
        const float thickness = std::max(.5f, std::min(8.f, s.outlineThickness));
        drawing->AddCircleFilled(geometry.center, geometry.dotRadius + thickness,
                                 IM_COL32(0, 0, 0, 255), 32);
    }
    drawing->AddCircleFilled(geometry.center, geometry.dotRadius, fill, 32);
}
}  // namespace CustomDotCrosshair
