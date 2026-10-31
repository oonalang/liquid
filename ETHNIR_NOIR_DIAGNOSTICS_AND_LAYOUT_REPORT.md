# ETHNIR NOIR V2: Triggerbot diagnostics, dot UI layout, Long Execute output fix

## Target
- CODM Garena 1.6.57 dump is the method/field reference.
- Input: `ETHNIR_NOIR_V2_DOT_5000_RANGE_FIX.zip`.
- Existing `Main.cpp`, hook registration offsets, memory patches, authentication, and `libV2` naming are unchanged.

## Verified code findings
1. Both bots and real opponents appear in the same `BaseGame.EnemyPawns` list in the existing ESP. Bots are distinguished using `Pawn.m_IsBot`. The Triggerbot uses the same list, so no additional bot-target list is necessary.
2. Previous Triggerbot failed closed on many conditions. It had no in-app failure diagnostics. The actual runtime failure could be hook inactivity, weapon/ADS state, target projection, physics linecast (including enemy collider collision), smoke validation, or the native `StartFire` method's virtual dispatch.
3. The custom dot previously used the grid's tall two-column control cards. Conditional controls made rows of unequal heights and left blank cells.
4. The `CheckTargetIsValid_ManualParameter(Pawn, out float distance)` hook returned true while skipping its output assignment. It cannot safely claim success while leaving the distance undefined.

## Source changes
- `src/main/jni/Features/Triggerbot1657Diagnostics.h` (new): Atomic runtime diagnostic counters and status strings.
- `src/main/jni/Features/Triggerbot1657.h`: Samples controller hook activity, human/bot enumeration, dot candidate overlap, blocked wall/smoke checks, and native StartFire request count. No change to existing hook offsets and no removal of mandatory occlusion checks. Bots are explicitly counted as valid EnemyPawns.
- `src/main/jni/UI/ApplicationUI.h`: Compact one-column native ImGui dot controls and collapsible runtime diagnostics. The new controls no longer enter the tall two-column grid and include ADS Only/Both status.
- `src/main/jni/Preview/PreviewState.h`: Provides diagnostic definitions to the standalone UI preview.
- `src/main/jni/Features/Execution1657Guard.h` (new) and `src/main/jni/hooks.h`: For the existing ManualParameter hook, invoke the original method to fill the out distance before overriding validity. Missing/negative/non-finite out distance no longer receives forced-success and is reset to a defined zero. This preserves the existing hook address and does not alter other execution hook bodies.
- `tests/test_triggerbot1657_native.cpp`: Verifies AI bot is included and diagnostics count a firing request.
- `tests/test_execution1657_guard.cpp` (new): Verifies successful distance, absent/invalid distance, and the normal disabled behavior.

## Diagnostic interpretation
- `Hook updates = 0`: native controller hook has not been observed; no firing attempt is possible.
- `Controller does not match local pawn`: attached player identity condition is preventing execution.
- `Enemies > 0 / Bots > 0 / Dot matches = 0`: enemy entries exist but no screen projection qualified within the dot circle.
- `Dot matches > 0 / Wall rejects > 0`: physics linecast is rejecting candidates; this might include the target's collider. No wall-check bypass was introduced.
- `Smoke rejects > 0`: smoke filters block those candidates.
- `Fire requests > 0` without in-game firing: `StartFire()` was invoked but its actual virtual dispatch / weapon logic / network result remains unverified.

## Tests run
- GCC and Clang C++17 host tests PASSED: native triggerbot simulation, triggerbot policy, execution distance guard, preexisting gameplay policy, dot geometry.
- GCC and Clang C++17 syntax checks PASSED: `ApplicationUI.h` using the standalone preview state plus existing test-only legacy globals (`speed_min`, `speed_max`, `HIDEESP`).
- The new package's ZIP integrity PASSED.

## Unverified and remaining limitations
- Android ARM64 compilation and live in-game Triggerbot firing are NOT VERIFIED; Android NDK and CODM runtime are unavailable here.
- The `PlayerController.StartFire()` virtual call may dispatch differently in game modes. Native offsets and callback ABI require runtime checks.
- An enemy collider might cause `Physics.Linecast` to report an obstruction. Wall/smoke checks deliberately still fail closed.
- The Long Execute change fixes an unassigned `out float distance` source defect. It does NOT establish an extended execution range. Other input, animation, and server-authoritative checks remain unverified.
- No new bypass hooks, anti-cheat patches, offsets or unverified mode-specific overrides were added.
