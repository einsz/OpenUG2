# Stage4 verification — 2026-10-15

PASS for the exercised approved component contracts and documented independent
level-planar gameplay adapter. No behavioral divergence was found. This does not
establish complete retail vehicle behavior or original-runtime parity.

Fresh verifier: `stage4_lateral_verification`; fresh implementer:
`stage4_lateral_implementation`. The verifier changed no implementation,
approved specification or shared test vector, and did not launch/read original
executable code or analyst evidence. Private synthetic checks and full logs are
ignored under `cleanroom/verification/stage4/`.

## Approved inputs

Hashes independently confirmed before verification:

| Specification | SHA-256 |
| --- | --- |
| Stage1 | `61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb` |
| Stage2 | `d3072359aa25bf0ef541504805f70b3579829bb9b1310cda8ba1a568286c6cb6` |
| Stage3 | `d4689a16861df810159760a6a668ef3714966fd6c20a30e3b2b54f355c41c6f7` |
| Stage4 | `02144d187323c6d74a6fb21e6e0b3e2842097cd0e28c935eea97907c1cbc911a` |

Stage4 `test-vectors/components.json` and README define 72 synthetic derived
cases, including exact integer/Boolean comparisons and individual float
 tolerances. These are reference outputs from reviewed mathematical requirements,
not retail runtime captures. No new original differential test was performed.

## Independently executed checks

Command (safe targets only; no main executable rebuild):

```sh
make lateral-test drivetrain-test resistance-test braking-test drive-stage1-test drive-stage4-test physics-clock-test district-collision-test ug2-track-collision-test race-ai-test
```

Exit status 0. Verbatim principal results:

```text
Stage4: 208 checks, 0 failures
PASS: 3606 checks, 0 failed
Stage 2: 348 checks, 0 failures
Stage 3: 479 checks, 0 failures
Stage 1 gameplay adapter tests passed
Stage4 mirror: h 0.28064 / -0.28064 vy 2.98894 / -2.98894 yaw 0.28858 / -0.28858
Stage4 gameplay adapter tests passed
physics_clock_test: PASS (synthetic OpenUG2 contracts; no retail equivalence claim)
district_collision_test: PASS
ug2_track_collision_test: PASS
render interpolation: timing, heading wrap, fast wheel phases, contact travel, reset and unchanged simulation PASS
```

The full race-AI suite also passes; its long final coverage statement is retained
verbatim in `project-tests.log`. Compilation emits existing indentation warnings.
A separate Stage4 component build with C99, Wall/Wextra/Werror and the project's
`-Wno-unused-function` passes all 208 checks. An initial attempt without that
suppression failed solely on unused shared JSON helper `hexkey`; it was rerun
with the project setting. Fresh ASan/UBSan build of the Stage4 adapter suite,
with leak detection disabled, exits 0 without sanitizer diagnostics. Executables
for extra verification were built in `/tmp`, avoiding debug/GLES build races.

Independent Python/ctypes mathematical oracle, deterministic seed 41015:

```text
Independent Stage4 mathematical/property oracle: 64362 checks passed
Independent Stage4 oracle total: 366506 checks passed (including 10,000 supplied-state filters and all 65,536 commands)
```

The oracle uses the approved equations, no original material. It compares all
256 heading bins across eight octants plus axes/diagonals/origin, folded reverse
slip, every scalar curve across knots/clamps, 10,000 randomized supplied-state
keyboard/analog filters with exact integer command checks, all unsigned steering
commands through rational geometry, 10,000 load/angle interpolations and combined
caps/post-cap corrections and transformed force/moments, plus strict sliding
thresholds, lock states, zero-speed and reverse cases. Float comparisons use
2e-5 absolute plus 3e-6 relative tolerance; integer outputs are exact. Shared
specification mathematics cannot independently validate the analyst's original
observations.

## Integration and lifecycle assessment

Executed adapter fixtures cover mirrored/zero turns, reverse yaw, nonzero service
brake plus turn, final force/moment sum once, lateral multiplier and peak scaling,
signed reverse normalization, provisional external moment, zero supplied airborne
loads and removal of carried tyre force/moment with yaw persistence, actual wall
velocity response entering the next contact sample, reset and configured car
replacement, bypass of old host steer/lateral tuning, and identical complete
adapter state across 30/60/90/144/240 Hz rendering schedules. Parser cases include
synthetic approved fields, truncated record, missing car and nonfinite reference.
Race fixtures check front-wheel presentation interpolation and existing actors.

Source conformance inspection confirms a single drivetrain combined cap after
five longitudinal passes, post-cap correction retained for wheel/body, pure
provisional acceleration, final force/moment accumulation once, no extra normal
force, no host lateral scrub or direct steering-to-yaw in Stage4, and actual yaw
speed supplied to translational stabilization. Command refresh does not replace
carried first-derivative forces; post-step contact and force preparation produce
the next derivative. Lost support explicitly removes prepared tyre contributions.
Main validates all four stages before stock car/switch configuration commits,
uses fixed simulation ticks, bypasses the old steer smoother in this opt-in path,
and propagates individual front angles to render poses. Transactional UI switching
and interactive lifecycle were inspected, not exercised end to end by this verifier.

## Native evidence inspected

Implementer reports final GLES/debug builds and completed 275-tick OpenUG2 run
using the existing legal asset loader, DISPLAY=:0 and dummy audio. Those build
and process exit results were not independently rerun. Verifier independently
read the completed `/tmp/stage4-steering.log`, inspected its end PNG and recorded
artifact hashes. The image shows the player car on the rendered roadway.

Observed telemetry: tick210 positive front angles .057489/.059538 rad, contact
forward 3.8734 m/s, lateral force +2.176638 kN and yaw +.054193 rad/s;
tick270 negative angles -.055703/-.053905 rad with yaw -.214906 rad/s.
Summary is 34.91 km/h final/peak, 10.38 m travel, wall-response=0,
rail-response=0, nan=0, oob_z=0, oob_xy=0. This short run ends before the
300–344 brake pulse; brake+turn is established synthetically, not by this native
run. Native captures/logs remain outside tracked artifacts; copied log/hash
records are private verification evidence.

## Boundaries and next gate

Q4-I1 remains U4-2: the 33-input/nine-rate 60 Hz FIFO with zero/one seeds is a
documented independent host producer, not the unknown original timed history.
Q4-I2 remains U2-8/U4-5: supplied inertia and damping compose the planar tyre
contribution; gameplay uses stock box inertia, persistent angular momentum and
zero additional damping. No missing original angular adjustment/assist law is
claimed implemented. These explicit choices are within the bounded adapter
scope; they do not resolve or silently redefine approved unknowns.

Complete contacts/suspension/load transfer, centre/contact arms, nonlevel force
basis, grip and handbrake producers, alternate modes, exact x87/trig precision,
original input cadence, angular assistance and perceptual handling remain open.
Space is an existing independent stop/hold adapter. Standalone bounded Stage4
components and this host fixture pass; complete retail steering/chassis fidelity
is unverified. Original execution/differential testing requires the prescribed
isolated VM/container, no network and snapshot; none was launched here.

No new reviewer cycle is required for observed conformance. Resolving unknown
producers or broadening parity claims requires fresh analyst specification work
and reviewer approval before a fresh implementer receives it. Technical clean-room
records do not establish legal sufficiency; counsel determines that separately.
