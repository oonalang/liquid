# LIQUID r23: Crosshair-to-body-part visibility

Base: local r22 death/respawn source. No GitHub checkout or push.

## Implemented

- Replaced the head-to-mesh centerline as the primary target selector with individually projected humanoid head, torso, shoulder, arm, hand, hip, thigh, knee, and foot points.
- Uses verified Unity `Animator.GetBoneTransform(HumanBodyBones)` and `Animator.get_isHuman()` methods from the supplied 1.6.57 metadata, accessed through `Pawn.m_AnimationComponent` and `AnimationComponent.m_PlayableAnimator`.
- Projects the segments between related bones and retains only sample points inside the configured firing circle. Samples are ordered by crosshair proximity and capped at eight per enemy.
- Tries multiple points *inside the same firing circle* to handle partially visible hitboxes, with a global 24-physics-cast budget.
- Preserves solid-only Linecast filtering, camera and firing-origin checks, smoke rejection, ADS/Both modes, and the local-player death/respawn reset from r22.
- Keeps a head/torso fallback when a usable humanoid rig is absent. It does not claim limb precision for non-humanoid rigs.
- Adds an inexpensive 2D prefilter before per-bone lookup; its margin accounts for outstretched limbs.
- Does not install any new reporting or bypass hooks.

## Files changed

- `src/main/jni/Features/Triggerbot1657.h`
- `src/main/jni/Features/Triggerbot1657Hitboxes.h` (new)
- `tests/test_triggerbot1657_native.cpp`
- `tests/test_triggerbot_hitboxes.cpp` (new)
- `tests/test_triggerbot_header_compile.cpp` (new)

## Verification

- 42 host test runs passed across GCC and Clang (21 test programs each).
- AddressSanitizer and UndefinedBehaviorSanitizer passed for geometry and native triggerbot regression tests.
- Stub-header compilation passed with GCC and Clang.
- Native tests check an arm entering the circle while the torso remains outside it, loss of overlap, wall and smoke rejection, partially visible cover, and existing death/respawn behavior.
- ZIP integrity verified; game dumps and generated shared libraries excluded. Required static build dependencies (`.a`) are retained.

## Limits

Android build and live CODM 1.6.57 playtesting were not available. The exact availability of humanoid Animator bones in each model/skin and the runtime frame-time cost still require device verification. This package must not be described as an in-game confirmed fix.
