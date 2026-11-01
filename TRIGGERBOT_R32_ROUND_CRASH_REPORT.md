# Triggerbot r32 — round-transition crash

Reported: the game crashes at the end of round 1 / start of round 2 with the triggerbot enabled.

## What the supplied log actually contains

`codm-crash-part01..03.txt` (269,175 lines, 20:27–22:19) hold only three fatal signals:

| time | process | signal |
|---|---|---|
| 20:27:29 | `lelspace:lbcore` | SIGBUS, unrelated app |
| 22:16:07.630 | `com.garena.game.codm` | SIGSEGV, `pid 30949` |
| 22:16:15.206 | `com.garena.game.codm` | SIGSEGV, `pid 31579` |

Both game crashes are identical, five frames deep, and happen **1.7 s after `CODM: onCreate start`**:

```
#00 libanort.so+0x1336e8
#01 libanort.so+0x11b7e4
#02 libanort.so+0x11d0f8
#03 libanort.so+0x032d20
#04 libunity.so+0xe742170
```

`x2 = 0xffffffffffffffc0` is a negative `memcpy`/loop length, and `x1` points one page before the
fault address — a corrupt length/pointer inside the game's protected library, not a bad argument
from this mod. No frame is in the mod, there is no tbot code in either stack, and the mod emits no
log lines at all in the dump. The third launch (`pid 31882`, 22:16:19) survived and the log ends
normally at 22:19:11 with no further crash.

So this dump does **not** contain a crash during a match or at a round change. It contains two
launch-time crashes inside the game's own ACE-side library. They are still worth a separate
investigation (the mod patches `libanogs` and ~50 `libunity` offsets blindly), but they are not the
event described.

The round-transition crash was therefore diagnosed from the code path rather than from a tombstone.

## Cause

r30 covered the case where the round *replaces the pawn*: it waits out `kPawnSettleMs` and defers the
release. But a player who survives into the next round **keeps the same pawn**. The game rebuilds the
round underneath it — new weapon, new enemy set, new physics/effects — while `match`, `local` and the
controller pointer all stay the same, so none of the existing lifecycle gates fire.

In that window the triggerbot kept:

1. **Releasing on the replacement weapon.** `pendingRelease` was remembered from the old press and
   then issued with `stopFire(self, …)` on whatever weapon the new round had built, even though that
   weapon had never seen our trigger. `PlayerController.StopFire` walks the weapon/fire-component
   state; on a weapon that is still being constructed that is game code we cannot guard, and it is
   called from our hook. This is the crash that has no mod frame.
2. **Querying the half-built round every frame.** `isAiming`, `canFire`, the clip count and the
   bone/physics scan only waited on the pawn/match settle windows, not on the weapon.

## Fix (`Features/Triggerbot1657.h`)

- New stability window on the **(match, pawn, weapon)** triple: any change re-arms `settleFrames`
  (8 frames by default) and the hook returns before any aim/fire/scan call. In steady state the
  window only re-arms on a real change, so it adds no input latency.
- A held press is remembered (`pendingRelease`) as soon as that triple changes, never released into
  the new objects.
- The remembered release is only issued on the **same weapon instance** that was pressed
  (`pressWeapon`). If the round replaced the weapon, the press is dropped: the trigger state was
  reset with the round and the new weapon never fired, so there is nothing to release.
- `pressWeapon` is cleared everywhere a release is actually performed.

## Tests

`tests/test_triggerbot1657_native.cpp`, new case: press on an automatic weapon, replace the weapon
while the pawn survives (exactly a surviving player entering round 2), then assert

- no `StopFire` and no `StartFire` during the stability window and while the triple is unresolved;
- after it settles the stale press is dropped, not released, and the new weapon is pressed.

Existing r26–r31 coverage (low-cover line of sight, embedded hits, single-fire taps, probe budget,
match/pawn transitions, controller mismatch, deferred release on a replaced pawn) still passes, as do
`test_triggerbot1657.cpp`, `test_triggerbot_hitboxes.cpp`, `test_triggerbot_strict_zone.cpp`,
`test_triggerbot_header_compile.cpp` and `test_gameplay1657.cpp`, plus ASan/UBSan on the native test.

## Not verified

No NDK build and no device. The round-transition fix is a code-level invariant, not a reproduced fix:
it removes the calls that can reach a half-built weapon, but if the crash survives it, the next step
is a tombstone from the actual crash (the two in this dump are launch-time and unrelated).

## Update: captured SIGSEGV (2026-11-02 04:56:54.808)

The runtime capture finally contained a crash in a match, not at launch:

```
pid 9922, tid 11393, name UnityMain  >>> com.garena.game.codm <<<
signal 11 (SIGSEGV), code -6, fault addr 000028ee000026c2
pc 00000073e7780d9c   x30 00000073e7780d8c
at libunity.(:0)
at libunity.(:0)
at libsigchain._ZN3art11SignalChain7HandlerE...
```

The fault address is not a heap pointer: `0x28ee000026c2 == (10478 << 32) | 9922`,
i.e. the game's uid (10478, the uid named in the Gspace SecurityException storm in
the same log) packed in front of the crashing pid. The faulting access used an
Android process-identity key as a pointer, which no fire path and no enemy scan in
this mod produces; both of those fault at heap addresses or at zero.

Consequences:

- The two mod-side hazards fixed above stay, but neither matches this signature.
- The crash report carries absolute PCs and no memory map, so the frame cannot be
  named. `Main.cpp` now logs `LIQUID: libunity base 0x...` once at startup; RVA =
  `pc - base` against the 1.6.57 offset table will identify the function.
- Verification is pending on device, without the Gspace container, with the
  triggerbot and the sprint option off, to separate mod calls from the game's own
  identity handling.

## Update: ESP-only reproduction (2026-11-02 06:03)

ESP with only boxes enabled crashed every match at the round flip, while the same
session with the triggerbot alone did not. The ESP walked `EnemyPawns` with nothing
but `Tools::IsPtrValid` between the list and the bone read - the same hole the
triggerbot had: a mapped page belonging to a pawn the game has already destroyed.

`Features/EnemyReadGuard1657.h` now gates every ESP bone read (closest target, FOV
target, the draw loop and the count panel) on a 300 ms settle window per pawn address,
and on `IsAlive` / `IsBot` reading exactly 0 or 1. A pawn that reappears after more
than a second starts a fresh window, so a reused address is never trusted.

Not yet verified on device.
