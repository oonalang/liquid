#pragma once

#include "Triggerbot1657Policy.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace Triggerbot1657 {
namespace Hitboxes {

struct World { float x=0.f, y=0.f, z=0.f; };
enum class Joint : std::uint8_t {
    Head, Neck, Chest, Hips,
    LeftShoulder, LeftUpperArm, LeftLowerArm, LeftHand,
    RightShoulder, RightUpperArm, RightLowerArm, RightHand,
    LeftUpperLeg, LeftLowerLeg, LeftFoot,
    RightUpperLeg, RightLowerLeg, RightFoot,
    Count
};

constexpr unsigned kJointCount=static_cast<unsigned>(Joint::Count);
constexpr unsigned kMaxCandidates=8;

struct JointPoint {
    World world{};
    Point screen{};
    bool valid=false;
};

using Pose=std::array<JointPoint,kJointCount>;

struct Candidate {
    World world{};
    Point screen{};
    float distanceSquared=std::numeric_limits<float>::infinity();
    float fraction=0.f;
    bool valid=false;
};

struct Results {
    std::array<Candidate,kMaxCandidates> samples{};
    unsigned count=0;
};

struct Segment { Joint from; Joint to; };
constexpr std::array<Segment,17> kSegments{{
    {Joint::Head, Joint::Neck},
    {Joint::Neck, Joint::Chest},
    {Joint::Chest, Joint::Hips},
    {Joint::Chest, Joint::LeftShoulder},
    {Joint::LeftShoulder, Joint::LeftUpperArm},
    {Joint::LeftUpperArm, Joint::LeftLowerArm},
    {Joint::LeftLowerArm, Joint::LeftHand},
    {Joint::Chest, Joint::RightShoulder},
    {Joint::RightShoulder, Joint::RightUpperArm},
    {Joint::RightUpperArm, Joint::RightLowerArm},
    {Joint::RightLowerArm, Joint::RightHand},
    {Joint::Hips, Joint::LeftUpperLeg},
    {Joint::LeftUpperLeg, Joint::LeftLowerLeg},
    {Joint::LeftLowerLeg, Joint::LeftFoot},
    {Joint::Hips, Joint::RightUpperLeg},
    {Joint::RightUpperLeg, Joint::RightLowerLeg},
    {Joint::RightLowerLeg, Joint::RightFoot}
}};

inline bool Finite(World p) {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}

inline void Add(Results& out, const Candidate& sample, float radiusSquared) {
    if (!sample.valid || !Finite(sample.world) || !std::isfinite(sample.screen.x) ||
        !std::isfinite(sample.screen.y) || !std::isfinite(sample.distanceSquared) ||
        sample.distanceSquared > radiusSquared) return;
    for (unsigned i=0;i<out.count;++i) {
        const auto& previous=out.samples[i];
        const float dx=previous.world.x-sample.world.x;
        const float dy=previous.world.y-sample.world.y;
        const float dz=previous.world.z-sample.world.z;
        if (dx*dx+dy*dy+dz*dz<0.0004f) return;
    }
    unsigned position=0;
    while (position<out.count && out.samples[position].distanceSquared<=sample.distanceSquared)
        ++position;
    if (position>=kMaxCandidates) return;
    const unsigned length=std::min(out.count,kMaxCandidates-1);
    for (unsigned i=length;i>position;--i) out.samples[i]=out.samples[i-1];
    out.samples[position]=sample;
    if (out.count<kMaxCandidates) ++out.count;
}

inline Results Collect(const Pose& pose, Point crosshair, float radius) {
    Results results;
    if (!std::isfinite(crosshair.x) || !std::isfinite(crosshair.y) ||
        !std::isfinite(radius) || radius<1.f || radius>600.f) return results;
    const float radiusSquared=radius*radius;
    for (const auto& joint : pose) {
        if (!joint.valid) continue;
        const float dx=joint.screen.x-crosshair.x,dy=joint.screen.y-crosshair.y;
        Add(results,{joint.world,joint.screen,dx*dx+dy*dy,0.f,true},radiusSquared);
    }
    for (const auto& edge : kSegments) {
        const auto& from=pose[static_cast<unsigned>(edge.from)];
        const auto& to=pose[static_cast<unsigned>(edge.to)];
        if (!from.valid || !to.valid) continue;
        const auto closest=ClosestOnBodySegment(crosshair,from.screen,to.screen);
        if (!closest.valid) continue;
        const float segmentX=to.screen.x-from.screen.x;
        const float segmentY=to.screen.y-from.screen.y;
        const float length=std::hypot(segmentX,segmentY);
        const auto addFraction=[&](float fraction) {
            const Point screen{
                from.screen.x+segmentX*fraction,
                from.screen.y+segmentY*fraction
            };
            const World world{
                from.world.x+(to.world.x-from.world.x)*fraction,
                from.world.y+(to.world.y-from.world.y)*fraction,
                from.world.z+(to.world.z-from.world.z)*fraction
            };
            const float dx=screen.x-crosshair.x,dy=screen.y-crosshair.y;
            Add(results,{world,screen,dx*dx+dy*dy,fraction,true},radiusSquared);
        };
        addFraction(closest.fraction);
        if (length>0.01f) {
            const float delta=std::min(1.f,radius*0.75f/length);
            addFraction(std::max(0.f,closest.fraction-delta));
            addFraction(std::min(1.f,closest.fraction+delta));
        }
    }
    return results;
}

inline bool RoughlyNear(Point crosshair,float radius,Point head,Point root) {
    if (!std::isfinite(crosshair.x) || !std::isfinite(crosshair.y) ||
        !std::isfinite(head.x) || !std::isfinite(head.y) ||
        !std::isfinite(root.x) || !std::isfinite(root.y) ||
        !std::isfinite(radius)) return true;
    const float height=std::hypot(head.x-root.x,head.y-root.y);
    if (height<1.f || height>2000.f) return true;
    const float margin=std::max(48.f,height*1.3f)+radius;
    return crosshair.x>=std::min(head.x,root.x)-margin &&
        crosshair.x<=std::max(head.x,root.x)+margin &&
        crosshair.y>=std::min(head.y,root.y)-margin &&
        crosshair.y<=std::max(head.y,root.y)+margin;
}

}
}
