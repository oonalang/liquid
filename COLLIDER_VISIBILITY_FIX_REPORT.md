# Collider visibility classification

Base: `oonalang/liquid` main at `dd2e87aa9cb702bb68a4fc706354d44a082a766d`.

The screenshot confirms that the checked static-receiver Linecast call returns, detects a collision, and rejects the candidate. The user reports an exposed enemy under the crosshair. The old diagnostic does not identify the collider, so it cannot establish whether that specific collision was the enemy, local equipment, or other geometry.

The previous classifier used `hitDistance + 0.5 >= targetDistance` to call a collision the target. Distance cannot establish identity: a target surface can be farther than 0.5 units ahead of its bone point, and unrelated cover can be closer than 0.5 units to it.

This update accepts a first collision only when the returned collider's Transform is the selected enemy's root Transform or its child. Other and unidentified colliders remain blocked. No distance tolerance is used to grant visibility. Smoke checks still run for an identified target. The already verified Linecast ABI adapter is unchanged.

The code uses the dump's Collider reference at RaycastHit offset `0x28`, `UnityEngine.Component.get_transform` at `0x50291B0`, and the reference-type `Transform.IsChildOf` method at `0x509DF80`. It does not call a value-type RaycastHit getter. Returned pointers are checked before use, remain local to the game-thread update, and are not retained across updates. Pointer readability checks do not guarantee native object lifetime; live behavior still requires device validation.

Diagnostics now report the last dot candidate's hit classification and hit/target distances. This distinguishes a selected-enemy hit from an unrelated collider or missing identity without increasing the tolerance or bypassing obstruction checks.

Validation:

- Production ARM64 compile/link: PASS, NDK 22.0.7026061.
- Native regression under GCC, Clang and GCC ASan/UBSan: PASS.
- Regression cases cover a target body well ahead of its bone, target root/child identity, unrelated cover close to the target, missing/invalid collider, missing transform or hierarchy callback, and smoke.
- Controller regression verifies that a selected-enemy collision requests fire and a subsequent unrelated collision releases it.
- Existing ABI decoder, triggerbot policy, health geometry, and resource-layout regressions: PASS.

The current screenshot does not prove which collider caused the rejection. On-device firing and the new classification remain unverified here; no game runtime is attached. If the new diagnostic says Other collider, its hit remains blocked rather than being assumed to be the enemy.
