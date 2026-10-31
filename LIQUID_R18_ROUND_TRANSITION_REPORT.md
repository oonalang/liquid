# LIQUID r18: Round-transition crash hardening

Source baseline: local r17 kill-effect-selection archive. GitHub main was not used or modified.

## Reported symptom
The game exits after ranked round one while the following are active: one legendary and one mythic skin, triggerbot Both, ESP box and health, no slide fatigue, no crouch, no aim drift and no hit flinch.

## Verified source risks
- `DrawESP` used the game enemy-list size and raw array pointer repeatedly, without bounding the list or validating its backing array.
- `DrawESP` read enemy transforms before testing alive state; it also read health data through an unvalidated `AttackableInfo` pointer.
- Skin effect resolution forwarded to the original getter while `gEffectMutex` was held. A re-entrant path could deadlock.
- The skin collector retains raw object pointers across object lifetimes. Page accessibility does not establish managed object validity. That risk remains and requires a crash trace/lifecycle-specific investigation.

## Changes
- Snapshot enemy-list contents, reject malformed negative, oversized or larger-than-capacity lists.
- Check relevant pointers and enemy alive state before transform and health access.
- Validate local pawn and weapon pointers before requesting the selected weapon ID in skin-effect callbacks.
- Release `gEffectMutex` before forwarding to the original effect getter.
- Fix division by zero in legacy name/distance placement calculations.
- No change to triggerbot firing, skin ID selection, visibility, feature toggles or release UI.

## Tests
- 10 targeted standalone tests passed under GCC and Clang (20 executions): gameplay, health overlay, skin selection, triggerbot policy, dot geometry, triggerbot runtime, linecast ABI, native triggerbot, bounded enemy-list snapshot, and skin-effect fallback.
- New enemy-list test passed with AddressSanitizer and UndefinedBehaviorSanitizer.
- Production Android native build and live ranked-round transition were not run here; a crash fix is not confirmed.

## Remaining limitations and next evidence
- Existing skin collectors preserve raw pointers in global vectors. Avoid assuming `Tools::IsPtrValid` verifies object lifetime; it only tests memory accessibility.
- A native crash log showing signal, fault address and first application/native library frames is required to attribute the termination to skins, ESP, triggerbot or gameplay hooks.
- Check whether there is a native SIGSEGV/SIGABRT tombstone versus a normal process termination. This distinguishes a code crash from an external process kill; do not speculate based on timing alone.
- Use an ADB-authorized device to capture `adb logcat -b crash -d -v threadtime` immediately after reproducing. Some Android devices restrict other apps from reading game logs.

No IL2CPP dumps or new compiled `.so` files are included. No GitHub push was performed.
