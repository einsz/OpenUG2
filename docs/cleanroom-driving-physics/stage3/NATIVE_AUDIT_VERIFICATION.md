# Supplemental native audit verification — 2026-10-15

Result: PASS for the narrow diagnostic input enhancement. Fresh verifier
`stage3_audit_verification` read independent OpenUG2 source, approved contracts,
design/report documents and recorded native logs. No original executable,
original instructions, analyst notes, disassembly or decompilation was accessed.
No implementation, approved specification or shared vector was edited.

The Stage1, Stage2 and Stage3 SPEC.md hashes independently match respectively
`61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb`,
`d3072359aa25bf0ef541504805f70b3579829bb9b1310cda8ba1a568286c6cb6`, and
`d4689a16861df810159760a6a668ef3714966fd6c20a30e3b2b54f355c41c6f7`.

## Source and targeted execution

The added interval is nested within explicit `daudit`, requires `stage1_active`,
and assigns only throttle/steer. It requests S at ticks 300–344 and falls back
to normal acceleration at 345. The active predicate requires the drivetrain
option and excludes active drag/drift races. No downstream physics change is
part of this interval. No interval override runs for ordinary interactive input
or legacy audit input. This conclusion is source-level plus extracted-block
execution, not a full interactive keyboard/legacy-race runtime exercise.

The verifier extracted the actual audit input block into an isolated C harness,
compiled with `cc -std=c99 -Wall -Wextra -Werror`, and compared against the
specified schedule across ticks 0–2499 for every audit/active flag combination,
including all interval boundaries. Existing steering, late S and coast phases
were included. Recorded results, verbatim:

```text
Audit schedule: 10000 cases, 0 failures
Pre-interval differential: 40 identical telemetry rows, 0 failures
Native brake/release/contact/summary checks: PASS
```

The second check compares all four telemetry row families at ten sampled ticks
0–270 between `/tmp/openug2_stage3_brake_audit_baseline.log` and
`/tmp/openug2_stage3_brake_audit.log`. The baseline continues acceleration at 300
(11.752 m/s), while the new log requests braking (11.505 m/s).

## Recorded native trajectory

The native log records stock 240SX on STREAML4RA, 420 frames, using the native
OpenUG2 loader. Full command is in NATIVE_AUDIT_DESIGN.md. Its zero process exit
is implementer-reported; this verifier inspected the completed log/capture and
did not launch another game or repeat the build. The log ends with the final
capture write. The unchanged numerical suites and sanitizers remain covered by
the independent core VERIFICATION.md; they were not repeated for this delta.

| Tick | Pedal | Requested front/rear torque (kN·m per wheel) | Speed (m/s) | Front/rear spin (rad/s) |
|---|---|---|---|---|
| 270 | 0 | 0 / 0 | 8.702 | 27.712 / 27.568 |
| 300 | 1 | 1.804200 / 2.075800 | 11.505 | 36.663 / 36.480 |
| 330 | 1 | 1.804200 / 2.075800 | 4.693 | 14.919 / 14.747 |
| 360 | 0 | 0 / 0 | 1.949 | 6.219 / 6.219 |
| 390 | 0 | 0 / 0 | 3.288 | 10.481 / 10.457 |

All five rows report four contacts (`f`), gear 1 and locks `0000`. At ticks 300
and 330 both reported tyre forces are negative; at 360/390 the rear force is
positive again. DA rows confirm S/zero steer at 300/330 and acceleration/zero
steer at 360/390. The summary reports wall-response=0, rail-response=0, nan=0,
oob_z=0 and oob_xy=0. These samples demonstrate moving service input, wheel/body
response, release and resumed acceleration in the independent native simulation.
They do not establish a stopping distance or a fully stopped interval.

Private reproducible checks and output are under
`cleanroom/verification/stage3/native-audit/` (`check.py`, extracted `schedule.c`,
`results.log`). Log SHA-256 values:

- New native log: `25f70724e49892031cd32528a4d7121b2c74eabb36ddf20fb22af38e319fbb06`.
- Baseline log: `ae72f9ba7c10733a32738334e516fb4fcdea670e7b8bc95a2ec0fe6230ad38c0`.
- Observed src/main.c: `0460ff687c93bd467e443b3667ca4db8684a8391ce2b5655008978918aa6eb84`.

## Divergences and limits

No divergence or proposed specification amendment was found. No further
reviewer/implementer round is needed for this diagnostic enhancement. This
supplement closes the old audit's missing native nonzero-service-pedal evidence;
it preserves the historical finding about that old run. Lock transitions are
not exercised here. No original-runtime differential, retail parity, retail
stopping curve, visual/audio quality, exhaustive interaction or Stage4/5 fidelity
is established. Unknowns and host fixture limitations in the core report remain.
Legal sufficiency remains a question for counsel.
