# Manual drivetrain trace verification — 2026-10-15

Scope: independent OpenUG2 diagnostic delta on `codex/driving-physics-stage5`, baseline `7b7c5b6`. Verifier changed no implementation or approved specification. This is diagnostic acceptance, not original-runtime parity or a physics correction.

## Results

PASS, project command `make drive-trace-test drive-stage5-test physics-clock-test district-collision-test`, exit 0. Verbatim result lines:

```text
drivetrain trace: schema, force/host distinctions and immutable state passed
Stage5 adapter: contact lifecycle, force accounting, handbrake/recovery, host pose, surfaces, reset/collision/clock passed
physics_clock_test: PASS (synthetic OpenUG2 contracts; no retail equivalence claim)
district_collision_test: PASS
```

PASS strict C99 trace compilation with `-Wall -Wextra -Werror -Wno-unused-function -Wno-misleading-indentation`; PASS ASan+UBSan trace test (`ASAN_OPTIONS=detect_leaks=0`). PASS direct unbuffered `/dev/full` writer error checks: header and row each report failure. PASS missing CLI path: exit 2 and `--drivetrain-trace requires a CSV file path`. Private check/log locations: `cleanroom/verification/stall-trace/tests.log`, `error_check.c`, `cli-missing.log`, `captures.json`. The provided trace test checks bytewise nonmutation of both supplied structures and synthetic positive blocker values.

Independently parsed completed implementation-native captures:

| Capture | Rows | Columns | Finite | Contiguous ticks | Previous prepared = incoming |
|---|---:|---:|---|---|---|
| `/tmp/stationary-trace-smoke.csv` | 420 | 133 | yes | 0–419 | every row pair |
| `/tmp/stationary-idle-smoke.csv` | 30 | 133 | yes | 0–29 | every row pair |
| `/tmp/stationary-repeat.csv` | 760 | 133 | yes | 0–759 | every row pair |

All headers have 133 unique fields; rows match header width. The no-audit capture contains only zero pedal, zero service brake, and no injected acceleration request; its final exported speed is 0.502694 m/s, so it is not evidence of exact stationary equilibrium. The 420-tick audit capture contains pedal -1/0/1 and service braking; it ends at exported speed 4.767568 m/s. The temporary repeated-input native harness contains four S→W transitions; subsequent W intervals end at exported speed 8.337, 3.569, 5.668, and 7.022 m/s. These captures are evidence supplied by the independent implementation; verifier did not launch a second native game or an original executable.

Source-boundary inspection confirms the CLI enables drivetrain mode; trace writes supplied state without generating controls or stepping physics. With tracing absent, no stream is opened and no row is written. Incoming forces are copied before derivatives, next-prepared forces come from tick-exit body state; host velocity is sampled after host/vehicle responses and converted to m/s, while body velocity retains the drivetrain phase. Ride pose is updated host state and contact preparation belongs to the preceding drivetrain preparation. Wall count, rail response, ground limiting fraction, and player vehicle-impact magnitude are carried from their host observations. `vehicle_contact` is an impact magnitude, not a count of all overlapping vehicles.

Error handling inspection: open/header failure exits 1; detected row/periodic flush failure reports and disables recording; final close failure reports. Runtime open failure and periodic/final buffered error integration were not exercised because another native harness owned the game session. CLI normal missing-path handling and writer failure signals were exercised as above.

## Coverage limits and bug status

No diagnostic defect requiring a spec amendment was found in this bounded verification. All three saved captures have zero positive wall, rail, or ground-blocker events and no host/body velocity difference above 1e-5 m/s. Positive blocker telemetry and host/body distinction pass synthetic writer tests and boundary inspection only; actual blocker-positive native trace remains unobserved. Pause/no-rows and inactive legacy-mode runtime behavior were inspected, not driven interactively. Complete enabled-versus-disabled native state equivalence was not measured. No ARM64 runtime test was performed; prior implementer cross-compilation is not runtime evidence.

All five supplied approved SPEC SHA-256 hashes match. Diff scope at verification was Makefile, main.c, DRIVETRAIN.md, Stage5 IMPL_DESIGN.md, plus new drive_trace.h and drive_trace_test.c; simulation modules were unchanged. Prior numerical oracles were not repeated for this observational delta. No original executable or original-runtime reference outputs were available or used, so differential parity is unverified.

The reported persistent braking stall is not yet deterministically reproduced or fixed. Coordinator separately reports that the temporary repeated-input harness exposes a host control handoff where W requests forward while actual gear remains reverse; this is a separate pending implementation delta and is not a trace acceptance claim. No traction, lock-clear, clutch, or force hack was introduced by this diagnostic delta.

User capture command (normal manual controls):

```sh
./nfsu2 '/path/to/your/NFSU2 installation' --car 240SX --drivetrain-trace /tmp/240sx-driving.csv
```

Reproduce with W/S/Space normally, allow a few seconds after the symptom, then quit normally. The named file is replaced. No commit/push performed.
