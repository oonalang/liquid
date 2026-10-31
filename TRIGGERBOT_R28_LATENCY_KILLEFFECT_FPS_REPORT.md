# LIQUID r28: triggerbot latency, mythic kill effect, long-session cost

Baseline: repository `main` at r27 (`63586e4`). Changes are in the working tree, not pushed.

## Reported symptoms

1. Still shooting delays and pauses, especially against an enemy partially covered by a wall.
2. Shooting after scoping is delayed; it should be almost immediate.
3. The same in Both mode.
4. A mythic kill effect and its kill sound do not play, while the weapon's firing sound does.
5. The longer the triggerbot is used, the worse the frame rate gets.

## 1. Scope / fire-mode transition delay (ADS Only and Both)

`PlayerController.CanFire` in 1.6.57 is
`public virtual bool CanFire(bool ignoreChangingFireModeTransition = false)` at RVA `0x5260EC0`
(dump line 826339). The triggerbot called it with `false`, so while the weapon was in the
hip-fire <-> ADS fire-mode transition after scoping the answer was "not yet" and the bot waited
out the entire transition before it even evaluated the target.

The readiness gate now calls `canFire(local, true)`, which ignores only that transition. Reload,
empty magazine, equip time, and the shot cooldown still block firing, and the game still refuses
a round it cannot deliver. Both modes share this gate, so Both mode gains the same reduction.

A press dropped by the transition is also recovered: for an automatic weapon the held request is
re-issued once when the weapon reports ready and the magazine has not moved, and a single-fire
press the magazine proves was ignored is retried after `kIgnoredPressBackoffMs` (120 ms) instead
of on the next frame.

## 2. Pauses against a partially covered enemy

The line-of-sight budget is 24 casts per evaluation. One candidate can hold up to 8 sample
points, each tried against up to 4 origins, and each ray can consume several casts while it
steps past the player's own view-model colliders (up to 6 self-hit skips). A partially covered
enemy spends the budget on its hidden side; because the sample order is deterministic, the
visible side was never reached - not for one frame, but for as long as the pose stayed the same.

- The remaining budget is now shared fairly between the samples still to be tested
  (`SampleCastAllowance`), so every sample gets its turn.
- In ADS the camera origin is tried first. The game's bullet ray is spawned from the camera
  (r26 finding) and the muzzle ray starts inside the player's own view model, so this is both
  the authoritative path and the cheapest one. A clear ADS target now costs one cast.
- The per-frame pointer-validation cache grew from 48 to 96 entries, cutting the number of
  `/dev/random` validation syscalls on large rigs.

## 3. Long-session frame-rate loss

Two bounded-cost fixes and one memory fix:

- An empty magazine is no longer pressed. The game answers every press with a dry-fire sound and
  animation; repeating that at the tap rate for as long as the dot rests on a target is steady
  game work with no shot behind it.
- The ADS camera-first order and the fair-share cap cut the number of physics casts per frame.
- The captured config lists (`SkinHaxSystem::Capture`, `CollectWeaponSkinConfig`) were unbounded
  for the whole process lifetime. The game keeps creating config instances across matches, so
  every database rebuild and every one-second fingerprint pass got slower the longer a session
  ran. They are now capped (2048 / 512) and the fingerprint reads the count instead of copying
  the list.

## 4. Mythic kill effect and kill sound

Kill effect resolution read only the selected skin's `WeaponConfExtraConfig.DefaultKillEffectId`,
then the compatibility table keyed by the inventory icon. A mythic skin normally carries its kill
effect on the **base weapon's** extra config, and the compatibility table (`KillEffectItemFields.
WeaponCanEquip`, 0x90) lists weapon ids, not icon ids. Both lookups therefore missed, the id
stayed 0, and `ApplyEffectAliases` erased the runtime entry. `ApplyToCurrentInstances` then wrote
that same 0 over `DefaultKillEffectId`, removing the kill visual and the kill sound with it. The
firing effect is a separate field and still worked.

- New `ResolveKillEffectId(authored, baseAuthored, matchingItem)`: skin config, then base weapon
  config, then the compatibility item.
- The compatibility table is queried by skin id, base weapon id, then inventory icon.
- The lookup no longer requires `ColorID == 5`.
- A resolved id of 0 no longer overwrites a kill effect the weapon already carries.

## Tests

`g++ -std=c++17 -Wall -Wextra -Werror`:

- triggerbot native simulation (new cases: press during the fire-mode transition, empty magazine
  never pressed, ADS camera-first single cast), policy, header compile, hitboxes, strict zone,
  linecast ABI, dot geometry, enemy list snapshot, managed runtime: PASS.
- Same native simulation under AddressSanitizer/UBSan: PASS.
- Kill-effect selection (new `ResolveKillEffectId` cases), kill-effect compatibility, effect
  aliases, effect fallback lifecycle: PASS.

## Not verified

- No Android NDK build and no live match. `SkinHaxSystem.h` and `MythicAttachmentSystem.h` are not
  covered by any host test, so their edits were reviewed but not compiled here.
- The frame-rate improvement is expected from fewer physics casts and bounded lists; it was not
  measured on a device.
- Whether the game accepts a press issued during its fire-mode transition is unverified. If it
  ignores it, the automatic retry and the 120 ms single-fire backoff recover and the result is no
  worse than before; if it accepts it, the shot leaves when the transition ends.
- The kill-effect fix depends on the mythic skin's base-weapon config actually carrying the
  authored id. If it carries none, the compatibility table is the only remaining source.
