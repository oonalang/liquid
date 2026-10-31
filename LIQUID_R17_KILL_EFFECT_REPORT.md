# LIQUID r17: Cosmetic kill-effect selection correction

Base: LIQUID r16 TLS Fix, not the GitHub main branch.

## Changes
- Prefer the authored `WeaponConfExtraFields::DefaultKillEffectId` for a selected mythic.
- Stop passing the last entry of `KillEffectItemFields::RealAssetIDs` as a kill-effect configuration ID.
- Use the matching `KillEffectItemFields::ID` only when no authored kill-effect ID exists.
- Preserve all triggerbot, firing, UI, and configuration source unchanged.

## Tests
- GCC and Clang: standalone selection, triggerbot policy/native, dot geometry, strict-zone tests pass.
- Clang AddressSanitizer/UndefinedBehaviorSanitizer selection test passes.
- No Android NDK or in-game VFX validation performed.

## Limits
Audio playing does not establish that the required visual-effects asset bundle is loaded or that the game's effect renderer will instantiate the asset. This change corrects one code-level ID mismatch. If the visuals still fail, verify visual resource download, graphic-effect settings, and equip/reload timing before any further runtime changes.

No compiled .so files or IL2CPP dumps included.
