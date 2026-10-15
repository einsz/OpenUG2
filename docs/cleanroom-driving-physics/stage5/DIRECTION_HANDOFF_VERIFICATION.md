# Host direction handoff verification — 2026-10-15

Bounded PASS for the independent OpenUG2 W/S host policy on `codex/driving-physics-stage5`, baseline `7b7c5b6` plus diagnostic and host-handoff deltas. Fresh verifier used approved specifications, independent implementation/test fixtures and supplied implementation-native traces only. No original executable/source/analyst evidence was accessed. No implementation, approved specification or vector was edited. All five supplied approved SPEC SHA-256 hashes match.

## Project checks

Command (exit 0):

```sh
make drivetrain-test resistance-test braking-test lateral-test contact-test drive-stage1-test drive-stage4-test drive-stage5-test physics-clock-test district-collision-test ug2-track-collision-test ground-motion-test world-collision-test fence-impact-test drive-trace-test
```

Verbatim result lines:

```text
PASS: 3606 checks, 0 failed
Stage 2: 348 checks, 0 failures
Stage 3: 479 checks, 0 failures
Stage4: 208 checks, 0 failures
Stage5: 77 vectors + 29 surfaces, 503 checks, 0 failures
Stage 1 gameplay adapter tests passed
Stage4 gameplay adapter tests passed
Stage5 direction handoff: 123220 substep observations passed
Stage5 adapter: contact lifecycle, force accounting, handbrake/recovery, host pose, surfaces, reset/collision/clock passed
physics_clock_test: PASS (synthetic OpenUG2 contracts; no retail equivalence claim)
district_collision_test: PASS
ug2_track_collision_test: PASS
ground_motion_test: PASS
world_collision_test: PASS
fence_impact_test: PASS (source role, solid/slow contacts, AI finish, visible fall, reload/reset, optional GPU)
drivetrain trace: schema, force/host distinctions and immutable state passed
```

## Independent checks

Private harness/logs: `cleanroom/verification/direction-handoff/` (`check.c`, fixture snapshot `fixture.inc`, `tests.log`, `independent.log`, `sanitizer.log`, `captures.json`). Build products are in `/tmp`. Strict C99 build with `-Wall -Wextra -Werror -Wno-unused-function -Wno-misleading-indentation` passes. ASan+UBSan with `ASAN_OPTIONS=detect_leaks=0` passes; leak detection was deliberately disabled, not verified.

Verbatim independent and sanitizer result:

```text
Independent direction oracle: 2520 cases, 40320 substep queries passed
```

The independent categorical oracle covers actual/pending reverse, neutral and forward gear; all four clutch modes; both retained direction latches; pedals -1, -.3, 0, .3, 1; speeds -.30001, -.3, -.29999, 0, .29999, .3, .30001 m/s. Opposite-direction demand is derived from direction categories, and expected shaping uses the documented dead zone and forward partial-pedal anti-stall floor independently. A cloned frame call additionally cross-checks shaping; it does not supply expected values. This exercises actualForward/requestReverse and actualReverse/requestForward, same-direction/neutral launches, stale targets, coasting and the exact inclusive selector/strict brake boundaries. Supplied focused tests additionally exercise rapid alternating W/S, in-tick Open-clutch transitions and eventual sustained launches in both directions.

Every query observes held shaped throttle, service brake and resistance throttle. Frame/blend refresh counters advance exactly once. First post-Euler velocity matches prior prepared force consumption, and final force/moment sums pass independent accounting. Thus current-tick control changes preserve the incoming-force lifecycle; neither incoming body force nor retained spin/energy is arbitrarily cleared. Source diff inspection confirms only host pedal/selector input policy changed in the simulation module: no traction equations, locks, torque laws, force callbacks, legacy or lower-stage paths were edited. Engine/tyre substeps continue consuming the once-shaped frame input. Component and adapter regressions independently cover lower stages and reset/collision behavior.

## Native trace observations

Completed supplied `/tmp/direction-handoff-native.csv`: 900 finite rows, contiguous ticks 0–899; every row pair has previous prepared force equal to next incoming force. CSV gear labels are HUD values (-1 reverse, 0 neutral, 1 first), not internal indices.

| Trace | W + forward latch + actual reverse rows | Positive throttle among those rows |
|---|---:|---:|
| `/tmp/stationary-repeat.csv` (760 rows) | 25 (ticks 310–333, 474) | 25 |
| `/tmp/direction-handoff-native.csv` (900 rows) | 2 (ticks 346–347) | 0 |

Updated ticks 346–347 each show pedal=1, gear=-1, target_gear=1, reverse=0, throttle=0, service_brake=0. Baseline examples at ticks 310 onward have the same displayed gear/request combination with throttle=1. These are different input trajectories, not a matched performance comparison. They demonstrate the previously identified wrong-gear throttle path and the new hold, not causation or resolution of every reported stationary symptom.

Completed native log reports peak 61.72 km/h, final 4.61 km/h, travel 76.41 m, wall responses 147 and rail responses 0. These are observed report values, not acceptance thresholds. No second native game or expensive full build was run by this verifier; build/run exit status and GLES/debug builds remain implementer-reported.

## Divergences and limits

No outstanding divergence found in this bounded host-policy round; no proposed normative amendment or new reviewer/implementation cycle required. This host device policy is outside original mapping contracts and is not recovered original behavior. Original-runtime differential/parity was not performed or claimed. No ARM64 runtime evidence was obtained. Full build/GLES/debug success is reported separately by the implementer, not independently rerun here.

Persistent stationary wheelspin/bog is not deterministically reproduced or universally fixed by these checks. The bounded fix removes the identified opposite-gear throttle path. Keep manual 240SX W/S/Space CSV capture for any remaining symptom; matched reproduction and further behavioral analysis are required before claiming broader resolution. Existing Stage5 I5-1/U5 producer limits remain unchanged. Any new recovered contract must go through analyst → reviewer → fresh implementer.
