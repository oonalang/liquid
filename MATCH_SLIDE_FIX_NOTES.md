# Match transition, smoke workload, and slide regression patch

## Changes

- Triggerbot returns before weapon lookup, aim checks, CanFire, or deferred
  StopFire while the match/pawn is settling or MatchGame is absent. Previously
  only the final firing decision respected the match delay. A separate observed
  match flag prevents match -> null -> match from bypassing the delay.
- Reject null/unreadable controllers before the original update, and revalidate
  afterward. Missing/mismatched local pawns restart both settle timers, including
  when a subsequent spawn reuses the same addresses. Pending releases wait for
  a live, settled pawn. Aim queries require a readable equipped weapon.
- Preserve the ignored-tap decision before clearing pressAmmoBefore so its retry
  backoff actually runs instead of submitting an ignored press every other frame.
- Camera-in-smoke rejection happens once before enemy poses and physics casts.
  Previously it ran per visible sample, after the expensive work. Endpoint and
  segment smoke checks remain in place for cameras outside smoke. This reduces
  work in smoke-heavy fights; it is not a measured Hardpoint FPS fix.
- The slide option forwards the original speed percentage instead of forcing
  an unverified zero. LimitSlideTackle always runs, with the local component's
  two decrease rates (0x48/0x4C in the 1.6.57 dump) temporarily zeroed and restored
  after the call. This preserves initialization/recovery that the old skip could
  suppress. Remote components and the disabled option use original behavior.

## Verification

Eight host executables pass with C++17, -Wall -Wextra -Werror:
gameplay1657, triggerbot1657_native, triggerbot_header_compile, triggerbot1657,
triggerbot_hitboxes, linecast1657_abi, enemy_list_snapshot1657,
triggerbot1657_runtime (all under tests/test_*.cpp).

Regression assertions cover no weapon/aim/CanFire access during a match swap,
no early StopFire, a null-match gap followed by a new match, firing recovery,
one camera smoke query with zero enemy scans/physics casts when inside smoke,
slide routine execution, local-only rate suppression and configuration restoration.
Additional regressions cover controller validation before the original call,
same-address recovery after missing/mismatched pawns, unreadable-weapon aim/fire
suppression and deferred release, and ignored-tap backoff/recovery.

## Device validation still required

No Android SDK/NDK or game device was available. The metadata dump has no method
bodies: the two slide rate fields are identified by name/layout, and their exact
runtime semantics still need verification. Existing accumulated fatigue is not
explicitly reset. Readable pointers and elapsed settle timers do not prove object
liveness; a crash tombstone is needed to confirm the reported crash's cause.

Check consecutive matches including a lobby gap; repeated deaths/respawns;
Hardpoint frame times with smoke both on and off the objective; and repeated
slides with the option off, on, and off again. Compare slide distances on flat
ground with No Crouch disabled, then check interaction with No Crouch separately.

## Phone-only crash capture

On Android 11+ with Wireless debugging available, install Termux's android-tools,
enable Developer options -> Wireless debugging, then pair and connect using the
two ports shown by Android (pairing and connection ports are different):

```sh
pkg install android-tools
adb pair 127.0.0.1:PAIRING_PORT
adb connect 127.0.0.1:DEBUG_PORT
adb logcat -b all -v threadtime > ~/codm-crash.txt
```

Keep the pairing dialog open using split-screen if needed. If localhost does not
connect, use the IP Android displays. Start recording before opening the virtual
space, reproduce the crash, then stop with Ctrl+C. Also try:

```sh
adb logcat -b crash -d -v threadtime > ~/codm-crash-buffer.txt
```

Capture all processes: native faults may appear under the virtual-space host's
package/process, not CODM. A host that catches a fault or kills a guest may leave
no native tombstone; the full log may still contain the exit reason. Plain Termux
logcat generally cannot read other apps' logs. Older Android versions without
wireless pairing require another ADB host or root for comparable access.
