# LIQUID r22: Triggerbot death and respawn lifecycle

Baseline: local r21 source ZIP. No GitHub update.

## Findings

- `diagUpdateCount` was incremented at the top of the pawn-update hook, including for non-local controllers and dead players. This counter occupies a fixed 64-bit atomic value and does not grow memory usage; the increase itself is not evidence of a leak.
- A change in equipped weapon or controller cleared parts of the triggerbot state, but a death followed by a respawn with the same controller and weapon did not explicitly reset all the firing-mode probe state.
- During a transition where the local player was temporarily missing or the old controller was detached, the hook returned without clearing internal state.

## Changes

- Only count updates after verifying the local attached pawn is alive.
- When local death is detected, issue one `StopFire` for an active held request on the verified current controller, then clear the held flag, timer, weapon identity, fire-mode classification, ammo probe state, retry budget, origin read failures, and per-life diagnostic counters.
- While dead, skip target evaluation, physics calls, weapon queries, fire requests, and repeated stop requests.
- On respawn, initialize the session again even if the game reuses the controller pointer and weapon pointer. This restores the existing eligibility checks and selected firing mode without having to retoggle the UI.
- If the previously tracked controller is no longer attached to the current local pawn, or the local pawn disappears, clear internal state without invoking a stale controller pointer. The game must manage actual firing on destroyed/detached objects.
- Preserve other-player isolation: their pawn updates cannot reset the active local controller.

## Verification

- 19 test programs passed under GCC, and 19 under Clang, 38 host executions total.
- The native triggerbot test passed AddressSanitizer and UndefinedBehaviorSanitizer.
- New cases: death with an active fire request, repeated dead frames, respawn using the same weapon/controller, per-life counter reset, round-local pawn disappearance, and non-local updates not increasing the counter.
- ZIP sources are a delta of only two modified files from the r21 source, plus this report. Full ZIP retains the ten required static build archives and excludes game dumps.

## Limitations

- Android NDK compilation and in-game ranked round testing have not been performed.
- The growing counter was not a memory leak; the device crash, lag, and intermittent firing issues are not independently confirmed fixed by this change.
- When a local pawn or controller disappears before the death flag is readable, the hook clears tracking state but intentionally avoids calling into an unverified, potentially destroyed game controller.
