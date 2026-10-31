# ADS runtime calls and health-bar position

This update follows the reported crash when scoping directly onto an enemy. It supersedes the triggerbot call details in `FIX_REPORT_2026_10_28.md`; the previous Mythic model corrections are retained.

## Triggerbot changes

- Removed the direct call to the `RaycastHit` value-type getter on a native stack buffer. The code now reads the dump-defined `collider` reference at offset `0x28`, validates it, and obtains that Collider's transform through `UnityEngine.Component.get_transform`.
- `Physics.Linecast(start, end, out hitInfo)` now runs through `il2cpp_runtime_invoke`. The runtime supplies its own generated invocation convention. The code does not guess static receiver arguments or call an RVA with an assumed out-parameter ABI.
- `StartFire` and `StopFire` now resolve the controller's virtual method and invoke it through the same runtime API, supplying the correct method metadata through the runtime.
- Missing runtime exports, missing metadata/methods, managed exceptions, and invalid Boolean results produce explicit diagnostics and deny a firing request. Failed starts do not increment the request counter or mark fire held. Failed releases retain ownership so a subsequent update can retry.
- Corrected the shared Component transform getter from `AtmosphericScatteringSun.get_transform` (`0x87192AC`) to the dump's `UnityEngine.Component.get_transform` (`0x50291B0`). This also corrects existing ADS/aim call paths using that shared wrapper.

Exact method lookup uses the dump's image, namespace, class, method name, argument count, parameter names, and static flag. It distinguishes the three-argument Linecast overload with `hitInfo` from the overload with `layerMask`. Lookup occurs on the game thread after the relevant code is reached, and retries if metadata is not yet available. There is no fallback to the former raw calls.

The metadata dump establishes the declarations and resource layout. It does not establish whether the former value-type getter was a boxed adjustor thunk or identify the crashing instruction. No native backtrace or running game is available, so the original crash location and live resolution remain unconfirmed.

## Health option

Open **Visuals → Health bar position → Side** for a vertical health bar along the left edge of each enemy box. The bar fills from the bottom upward. **Top** remains the default.

The health renderer now rejects zero/invalid maximum health, clamps the displayed fraction, and avoids the previous division by zero in its width calculation. Unused health reads through `PlayerInfo` were removed; health is read from the pawn's `AttackableInfo` in the actual health-rendering block.

Saved binary configuration layouts remain unchanged. Like the existing separate dot settings, the new health position is a runtime setting and resets to Top after restarting.

## Evidence

Reference: [user-provided 1.6.57 metadata dump](https://huggingface.co/datasets/jeudhehs/dump/blob/main/dump%201.6.57.cs), SHA-256 `dd94b4b41a2e5fffaff3d1635d23f9290ab70ca4960d7be853d95381cced758f`.

- `Collider : Component`: dump line 271083.
- `RaycastHit.collider` field: lines 271388–271394.
- `Component.get_transform`: lines 252502–252503; the unrelated AtmosphericScatteringSun getter is at lines 1503041–1503042.
- Linecast overloads: lines 269812–269828.
- PlayerController fire methods: lines 1066277–1066281.
- `AttackableTarget.m_AttackableInfo`: object offset `0x78`, dump line 1068960.

## Validation

- Full production ARM64 compile and link: **PASS**, pinned Android NDK **22.0.7026061**, Android API 21.
- GCC optimized C++17 tests: **PASS** for runtime invocation, native triggerbot simulation, health geometry, resource layout, triggerbot policy, dot geometry, existing gameplay policy, and execution distance guard.
- Clang C++17 tests: **PASS** for runtime invocation, native triggerbot simulation, and health geometry.
- GCC AddressSanitizer + UndefinedBehaviorSanitizer: **PASS** for those three changed test areas.
- Independent code review: **PASS** for invocation signatures, overload selection, out-argument handling, virtual dispatch, diagnostics, and health geometry.

The runtime tests exercise correct overload selection, the actual out-buffer address, Boolean unboxing, void return handling, missing exports/metadata, exceptions, failed start/release handling, and metadata retry. The health tests exercise top/side placement, bottom-up fill, clamping, and invalid health inputs.

Built `libV2.so` SHA-256: `b87700dff3256aec58a87ed80775c186399ff8393a1bc657cf50960d6fdf7ca9`.

Compilation and simulated runtime tests do not demonstrate a successful live-game shot or reproduce the reported crash. Device verification is still required.
