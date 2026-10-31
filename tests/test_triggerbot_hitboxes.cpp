#include "../src/main/jni/Features/Triggerbot1657Hitboxes.h"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace Triggerbot1657;
using namespace Triggerbot1657::Hitboxes;

static void Set(Pose& pose,Joint part,float sx,float sy,float wx,float wy) {
    pose[static_cast<unsigned>(part)]={{wx,wy,0.f},{sx,sy},true};
}
int main() {
    Pose pose{};
    const Point center{100.f,100.f};
    assert(Collect(pose,center,10.f).count==0);
    Set(pose,Joint::Head,130.f,40.f,0.f,2.f);
    Set(pose,Joint::Neck,130.f,65.f,0.f,1.7f);
    Set(pose,Joint::Chest,130.f,100.f,0.f,1.4f);
    Set(pose,Joint::Hips,130.f,140.f,0.f,0.8f);
    assert(Collect(pose,center,10.f).count==0);
    Set(pose,Joint::LeftShoulder,115.f,72.f,-0.2f,1.7f);
    Set(pose,Joint::LeftUpperArm,102.f,90.f,-0.45f,1.5f);
    Set(pose,Joint::LeftLowerArm,98.f,110.f,-0.6f,1.3f);
    auto arm=Collect(pose,center,10.f);
    assert(arm.count>0);
    assert(arm.samples[0].distanceSquared<=100.f);
    bool onLeftArm=false;
    for (unsigned i=0;i<arm.count;++i)
        if (arm.samples[i].world.x < -0.4f) onLeftArm=true;
    assert(onLeftArm);
    assert(Collect(pose,{100.f,118.f},1.f).count==0);
    Set(pose,Joint::RightUpperLeg,120.f,140.f,0.2f,0.8f);
    Set(pose,Joint::RightLowerLeg,100.f,100.f,0.3f,0.4f);
    auto leg=Collect(pose,center,10.f);
    assert(leg.count>0);
    bool legFound=false;
    for (unsigned i=0;i<leg.count;++i)
        if (leg.samples[i].world.x>0.1f) legFound=true;
    assert(legFound);
    assert(!RoughlyNear({600.f,600.f},10.f,{100.f,70.f},{100.f,140.f}));
    assert(RoughlyNear({120.f,100.f},10.f,{100.f,70.f},{100.f,140.f}));
    assert(RoughlyNear({150.f,200.f},7.f,{180.f,196.f},{180.f,206.f}));
    assert(RoughlyNear({120.f,100.f},10.f,{NAN,70.f},{100.f,140.f}));
    assert(Collect(pose,center,NAN).count==0);
    assert(Collect(pose,center,0.f).count==0);
    Pose independent{};
    Set(independent,Joint::RightShoulder,130.f,75.f,0.2f,1.9f);
    Set(independent,Joint::RightUpperArm,112.f,92.f,0.4f,1.6f);
    Set(independent,Joint::RightLowerArm,100.f,108.f,0.7f,1.3f);
    assert(Collect(independent,center,10.f).count>0);
    assert(Collect(independent,{300.f,300.f},10.f).count==0);
    Pose bothLegs{};
    Set(bothLegs,Joint::LeftUpperLeg,95.f,90.f,-0.25f,1.0f);
    Set(bothLegs,Joint::LeftLowerLeg,95.f,110.f,-0.30f,0.4f);
    Set(bothLegs,Joint::RightUpperLeg,105.f,90.f,0.25f,1.0f);
    Set(bothLegs,Joint::RightLowerLeg,105.f,110.f,0.30f,0.4f);
    assert(Collect(bothLegs,center,10.f).count>0);
    auto exact=Collect(bothLegs,{95.f,100.f},1.f);
    assert(exact.count>0);
    auto outside=Collect(bothLegs,{115.f,100.f},1.f);
    assert(outside.count==0);
    Pose duplicate{};
    Set(duplicate,Joint::Head,100.f,100.f,0.f,1.f);
    Set(duplicate,Joint::Neck,100.f,100.f,0.f,1.f);
    assert(Collect(duplicate,center,10.f).count==1);
    for (auto& element : duplicate) element={};
    for (unsigned i=0;i<kJointCount;++i) {
        const float delta=static_cast<float>(i)/50.f;
        duplicate[i]={{delta,1.f,0.f},{100.f+delta,100.f},true};
    }
    const auto limited=Collect(duplicate,center,10.f);
    assert(limited.count==kMaxCandidates);
    for (unsigned i=1;i<limited.count;++i)
        assert(limited.samples[i-1].distanceSquared<=limited.samples[i].distanceSquared);
    std::cout << "hitbox zone tests passed\n";
}
