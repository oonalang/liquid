# LIQUID r20: Mythic kill-effect mapping

Baseline: local LIQUID r19 archive. GitHub main was not read or changed.

## Reported behavior
- Legendary kill-effect visuals display.
- Mythic kill-effect audio plays, but the visual is missing.
- The user has applied one legendary and one mythic weapon skin.

## Verified source issues
1. `ApplyToCurrentInstances` erased all four runtime effect maps on every selected-skin application. Applying one weapon after another discarded the first weapon's kill-effect override.
2. The kill-effect compatibility index used only the last entry of each `WeaponCanEquip` array. Multi-weapon entries were ignored.
3. The worker remembered only the last applied skin for rescan/reapplication, even though multiple weapons can be configured.

## Changes
- Update kill, firing, bullet, and broadcast effect mappings only for the selected weapon's aliases. Unrelated weapons retain their mappings.
- Record applied selections per base weapon ID. On a database rebuild, reapply all recorded selections rather than the last one only.
- Index every validated `WeaponCanEquip` entry, with a maximum of 256 entries per effect, instead of indexing only the last one.
- Keep the r17 preference for an authored kill-effect ID. Do not replace it with a raw visual resource asset ID.
- No modifications to triggerbot, ESP, mechanics, authentication, diagnostic UI, or game dumps.

## Verification
- 19 standalone host programs passed with GCC and Clang (38 executable runs). The certificate test was supplied the local CA store, and the export test was supplied built partial/full runtime fixtures.
- New effect-alias preservation and multi-weapon compatibility tests passed with Clang ASan/UBSan.
- No Android NDK or live game test. A complete native build was not performed.

## Limitations
- A missing/unloaded mythic VFX bundle, unsupported graphics setting, evolution/kill-effect progression setting, or game-side effect renderer can still prevent visuals.
- This fix addresses verified mapping losses. It does not establish that the desired mythic VFX asset exists on the device.
- The skin system retains raw captured instance pointers across managed object lifetimes; this release does not eliminate that separate round-transition crash risk.
- No source pushed to GitHub.
