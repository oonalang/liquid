# Linecast call correction

Base: `oonalang/liquid`, branch `main`, commit `7aaa2692659dfc319e5cd09e2ac021ad624b3459` (latest checked before packaging).

The screenshot's last stage is immediately before the direct `Physics.Linecast` call. This commit supplies its out-hit pointer as the first integer argument and null as the second. That is correct only for a generated static method without a hidden receiver. If the generated method takes a static receiver, its out-hit pointer belongs in the second integer argument instead. The metadata declaration alone does not distinguish those native conventions, and there is no device backtrace to establish the exact faulting instruction.

## Change

Before binding the method, inspect its loaded ARM64 forwarding wrapper. The 1.6.57 dump places this 12-byte wrapper at `0x506D104` and its default-mask callee at `0x506D07C`. The decoder accepts only the exact forwarding form that loads DefaultRaycastLayers (-5), loads UseGlobal (0), and tail-branches to that callee. Both the mask/query registers and the branch destination must match.

The integer register used for the mask establishes the preceding output-pointer position: mask in x1 means out-hit in x0; mask in x2 means out-hit in x1 after a receiver. Vector3 arguments occupy the independent ARM64 floating-point argument registers. The adapter supplies the output pointer in the verified position and supplies a null receiver only for the receiver form.

The reader checks that the wrapper is inside a readable, executable load segment of the selected module before copying instructions. Unrecognized or inaccessible code leaves Linecast unbound; eligibility rejects it with a specific diagnostic. It must never be treated as an unobstructed line of sight. Diagnostics display the three instruction words for an unsupported wrapper.

No collision distance, fire method, smoke method, skin, health, authentication, or build configuration changes were made. The latest commit's existing 0.5-unit target-distance policy remains unchanged; this patch does not validate that policy as collider identity.

## Validation

- Full ARM64 production compile/link: PASS with pinned NDK 22.0.7026061, Android API 21.
- NDK ARM64 assembler and disassembler independently confirm the mask, query, and branch instruction encodings used by the decoder tests.
- Production adapter disassembly confirms the output pointer remains in x0 for the static form, or moves to x1 while x0 is cleared for the receiver form; floating-point arguments are preserved.
- GCC and Clang decoder/native integration tests: PASS.
- GCC AddressSanitizer + UndefinedBehaviorSanitizer decoder/native integration tests: PASS.
- Existing triggerbot policy, health geometry, and resource layout regressions: PASS.
- Unknown wrapper code and an unmapped module base are rejected in regression tests.

The game binary and running device are unavailable here. The wrapper forms are verified using instruction fixtures and compiler output; which form the installed game contains is determined by the new on-device check. This build must still be tested on-device. If diagnostics report an unrecognized wrapper, its instruction words or the matching libunity.so are needed before extending the decoder. No unsupported convention is attempted.
