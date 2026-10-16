# Stage 1 verification — 2026-10-15

Result: PASS against the approved specification and supplied derived vectors. This does not establish retail runtime parity.

Approved SPEC.md SHA-256: `61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb` (independently checked).
Fresh cleanroom-verifier: driving_amendment_verification. Implementation and approved specification were not edited.

## Results

- `make drivetrain-test`: `PASS: 3606 checks, 0 failed`.
- Independent strict build: `cc -std=c99 -Wall -Wextra -Wpedantic -Werror -Isrc tools/drivetrain_test.c src/drivetrain.c -o /tmp/drivetrain-verify-strict -lm`; execution: `PASS: 3606 checks, 0 failed`.
- AddressSanitizer and UndefinedBehaviorSanitizer build (`-g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer`): execution with `ASAN_OPTIONS=detect_leaks=0`: `PASS: 3606 checks, 0 failed`; no sanitizer diagnostic. Leak detection was disabled, so leaks were not verified.
- Independent deterministic property script: `PASS: 40011 independent deterministic property cases; seed 20261015`. It compares axle splits with the specified piecewise formula, checks share bounds and conservation, compares curve interpolation and clamping on signed samples, and checks binary32 duration subdivision/count/maximum-step properties. Inputs remain within meaningful defined numeric domains.
- Compiler: `cc (Ubuntu 15.2.0-16ubuntu1) 15.2.0`.

## Coverage assessment

Every supplied vector family ran: parameters (159 cumulative checks), engine torque (861), substeps (887), clutch torque (960), axle split (973), and all four neutral free-rev files (3389). Free-rev maximum angular-speed differences were 0.00011 rad/s (PEUGOT, 240SX, SYNTH_RWD) and 0.00005 rad/s (SKYLINE), with zero limiter mismatches.

All 18 question cases ran (3477 cumulative checks). They exercise axle bias; tyre branch selection; first-gear automatic selection; assist reporting; AWD shared synchronization snapshot; first-gear feedback; saturated/unsaturated stored force, including subsequent wheel use; angle adjustment; cached speed across a shift; creation processing; planar moment sign; and current-force lock correction. Some expected intermediate quantities (automatic thresholds, lock denominator/accelerations) are not separately exposed/asserted; the resulting selector decision or correction is asserted.

The additional amendment boundary tests reach 3561 cumulative checks. They test assist state supplied by the caller, timer advancement outside Locked, strict 10-second boundary and persistent reporting, neutral selection timer reset, remembered pre-change gear, high-rpm equality, downshift ceiling equality/cancellation, low-rpm fallback, invalid manual selections preserving state, full supplied rear multiplier above/below one, signed lateral input, five-pass force/reaction accumulation including the final correction, cached speed preservation, angle equality/one-adjustment/reset, and high-rpm creation post-processing. Remaining behavior tests reach 3606 checks. The launch smoke result uses invented loads/drag and is not a retail launch vector.

## Limitations and differential status

The supplied vectors are mathematical references derived from the specification, not recorded retail outputs. Their comparison verifies specification consistency only. No retail executable was launched. The reference file hash independently matches the recorded intake. A harmless bubblewrap namespace probe failed because user namespaces are not permitted. No configured isolated VM/snapshot runtime capture setup was established; presence of VM/container executables alone is insufficient. Runtime differential testing is therefore UNPERFORMED.

No full in-gear retail launch reference exists. Later-stage body integration, normal loads, braking and lateral curves are outside this module verification. Q-2 intermediate rounding, Q-11 general rear context, Q-16 carried clock/clamp and U-1–U-16 remain unresolved as stated in the approved spec. Round-trip serialization is not an applicable Stage 1 contract. Invalid API-selection checks ran; unspecified malformed/NaN/infinite state behavior was not treated as a retail requirement.

No observable divergence was found in the executed checks. No new analyst/reviewer cycle is required by these results. Retail parity requires future isolated runtime observations and the usual reviewer gate for any resulting behavioral amendments.

Verbatim command outputs and independent property script are retained in analyst-side `cleanroom/verification/`: `make-drivetrain-test.log`, `strict.log`, `sanitizers.log`, `properties.py`, `properties.log`. These contain no retail code/disassembly.
