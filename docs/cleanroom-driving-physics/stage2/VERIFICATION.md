# Independent Stage-2 verification — 2026-10-15

Fresh verifier: `stage2_resistance_verification`. Result: **PASS for tested
approved standalone behavior and the documented planar host fixture**. No
behavioral amendment or new reviewer cycle is required by these results.
This is specification consistency, not demonstrated retail parity.

Approved hashes independently rechecked unchanged:

- Stage 2: `d3072359aa25bf0ef541504805f70b3579829bb9b1310cda8ba1a568286c6cb6`.
- Stage 1: `61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb`.

## Executed checks

`make resistance-test drivetrain-test drive-stage1-test physics-clock-test district-collision-test`
completed with exit 0. Verbatim result lines:

```
Stage 2: 348 checks, 0 failures
PASS: 3606 checks, 0 failed
Stage 1 gameplay adapter tests passed
physics_clock_test: PASS (synthetic OpenUG2 contracts; no retail equivalence claim)
district_collision_test: PASS
```

The Stage-2 harness explicitly iterates all 52 approved cases and rejects unknown
case families. All seven families execute: drag, vertical aero, wheel rolling
torque, local limiter, stabilization, absolute cap and prepared-force Euler.
Additional harness checks cover point-force/moment caps, slopes, contact-arm
boundary, low-speed fallback, mode retention, strict limiter predicates, curve
knots, scalar state, gravity seeding and accumulator clearing.

Fresh ASan/UBSan standalone and gameplay adapter builds/runs passed, with
`-std=c99 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer` and
`ASAN_OPTIONS=detect_leaks=0`. No sanitizer diagnostic appeared. Leak detection
was disabled. Existing project compiler warnings remain in the build log.

Independent `cleanroom/verification/stage2/check.py` compiles a temporary shared
library and compares its public API against separately calculated approved
formulas. Seed 20261015 produces 10,000 arbitrary signed 3D velocity/state cases,
two adjacent mass-gate cases, and 2,000 sloped aerodynamic cases with independent
per-application force/moment caps. Result:

```
Independent seeded formula/property checks: 163204 checks, 0 failures (10000 cases plus mass boundaries)
Total independent checks including 2000 sloped/capped aerodynamic cases: 177204, 0 failures
```

Coverage includes vector drag and units/signs/scales, limiter force mutation and
retained components, pre-step stabilization momentum, full-vector absolute cap,
cleared accumulators, rotated front/rear loads and force/moment accumulation.
The numerical oracle was corrected during verification to store each accumulator
addition as binary32; the initial higher-precision sum caused cancellation error
outside the output tolerance. This was an oracle issue, not an implementation
finding. Scripts and full logs remain in the ignored verification directory.

## Integration assessment

Read-through and passing gameplay regression tests establish the documented
force lifecycle: creation prepares zero-time aero/coupling before the first
ordinary derivative; the body consumes prepared force once, post-step clears it,
aero is prepared before coupling, and resulting wheel forces remain for the next
derivative. Wheel resistance enters the wheel derivative once and is absent from
the smoothed torque and body force. Body and wheel surface channels are distinct.
The adapter consistently converts metres/tick to m/s using the host tick rate,
uses tonnes/kN and the same mass/basis/throttle/contact snapshot/substep duration.

Gameplay regression includes an independently calculated four-substep airborne
drag trajectory, host drag/top-speed independence, limiter without velocity
clamping, symmetric absolute cap, damage/surface separation and reset retention.
Existing launch/shift/braking/reverse/locking/collision/reset/replacement/pause
and clock checks pass. Source inspection confirms legacy coast/surface drag and
road/reverse caps are conditional on the legacy chassis, and Stage 1/2 requires
explicit `--drivetrain`; drag/drift remain legacy fallbacks. No original runtime
claim follows from retained host steering/braking policies.

The standalone module supports supplied 3D bases and slope rules. The playable
adapter deliberately supplies a level XY basis, zero angular speed, static
contact loads and explicit ordinary fixture inputs. It discards vertical force
before integration and does not feed aero into suspension loads. Full 3D gameplay,
slope-dependent limiter behavior in gameplay, vertical/contact/angular feedback,
unknown mode/surface/damage/reduction producers and complete race families remain
unverified or deferred. Therefore this does **not** establish complete Stage-2
world-motion fidelity. These limitations are already disclosed in IMPL_DESIGN.md.

## Limits and provenance

No implementation or approved spec/vector was edited. No original executable,
supplemental original code, private analyst evidence, or original runtime was
accessed. Safe original runtime comparison was unavailable and remains
**unperformed**; all approved expected outputs are synthetic. Invalid/NaN inputs
are outside the standalone contract; adapter decoder rejection paths pass in
the project suite. No format round-trip is defined for the resistance API.

The implementer's debug/GLES builds and native 480-tick smoke were reported in
IMPL_DESIGN.md and were not rerun by this verifier. Their native rendered/audio
quality and interactive menu lifecycle are not independently certified here.
No retail coast-down or terminal-speed trajectory has been compared.
Legal sufficiency of the clean-room process remains a matter for counsel.
