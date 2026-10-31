# LIQUID r19: Triggerbot lifecycle hardening

Baseline: local r18 round-transition source archive. No GitHub content was pulled, and no repository changes were made.

## Reported issue
Intermittent ranked crash after round one while triggerbot (Both), ESP, skins, and weapon-handling options are active.

## Source findings
1. `HookUpdateLocalPawn` reset its global local firing/controller state on any controller pointer change **before** testing whether the controller belonged to the local player. Multiplayer callbacks from other player controllers could therefore reset the local firing state while a firing request was active.
2. Triggerbot accessed enemy-list size and backing entries directly, without the list capacity checks and bounded snapshot already present in the ESP system.
3. The hook dereferenced controller state without checking its pointer for accessibility. This check is an added guard, not a substitute for a managed-object lifetime check.

## Source changes
- Validate the local player and controller before reading its attached pawn.
- Ignore non-local controller callbacks without resetting locally held fire/probe state.
- Reset locally held state only when an eligible local controller changes.
- Read the enemy list through `EnemyListSnapshot1657::Read`, which rejects missing arrays, size/capacity mismatches, and oversized lists before enumerating a fixed snapshot.
- Preserve the existing firing eligibility, line-of-sight, smoke, crosshair, weapon handling, and cosmetics code.
- Add native regression scenarios for non-local controller updates, invalid controller pointers, and inconsistent enemy-list capacity.

## Verification
- 15 standalone test programs passed under GCC and Clang, 30 host executions total.
- The native triggerbot and enemy-list snapshot tests passed AddressSanitizer and UndefinedBehaviorSanitizer under Clang.
- Files in the ZIP passed CRC verification and game dumps were excluded.

## Limits
- This is a source-level correction to observed code defects, not a confirmed repair of the ranked crash.
- No Android NDK build or on-device ranked test was performed.
- `Tools::IsPtrValid` does not prove a managed game object remains alive.
- Snapshot copies do not provide synchronization if the game's enemy list mutates concurrently.
- If crashes remain, record Android's native crash buffer (`adb logcat -b crash -d -v threadtime`) promptly after the reproduction and inspect the signal, fault address, and frames mentioning the native library. A crash trace is needed to attribute the original failure.

No GitHub push was performed.
