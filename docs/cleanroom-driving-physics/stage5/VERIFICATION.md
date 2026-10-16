# Stage5 independent verification — 2026-10-15

Final result: PASS for the tested bounded component and prescribed-pose host contracts.
This does not establish full dynamic 3D gameplay or original-runtime parity.
Fresh verifier: `stage5_contact_verification`; fresh implementer:
`stage5_contact_implementation`. No implementation, spec or shared vector edits
were made by this verifier. All five approved spec SHA-256 values independently
match the approvals; Stage5 is
`131e2c9e26091c647fb96bdb9ac0891e43c309487abe1e984b4408b711eec16e`.

## Independently executed checks

Project command:
`make contact-test drive-stage5-test drivetrain-test resistance-test braking-test lateral-test drive-stage1-test drive-stage4-test physics-clock-test district-collision-test ug2-track-collision-test`.
Exit 0. Relevant output verbatim:

```
Stage5: 77 vectors + 29 surfaces, 503 checks, 0 failures
Stage5 moving hb: coast 11.96928 hb 10.23541 blend 0.50000
Stage5 adapter: contact lifecycle, force accounting, handbrake/recovery, host pose, surfaces, reset/collision/clock passed
PASS: 3606 checks, 0 failed
Stage 2: 348 checks, 0 failures
Stage 3: 479 checks, 0 failures
Stage4: 208 checks, 0 failures
physics_clock_test: PASS (synthetic OpenUG2 contracts; no retail equivalence claim)
district_collision_test: PASS
ug2_track_collision_test: PASS
```

All 77 approved cases execute, including tilted four-contact geometry, sequential
pair damping/progression, preload/aero snapshots, strict geometric/rate and
handbrake/grip boundaries, corrected signed service conversion and reverse
coefficients. All 174 numeric surface fields compare exactly as binary32.
Some component scheduling assertions use explicit fixture bookkeeping; the
composed adapter separately observes actual post-Euler velocity and refresh counts.

The adapter tests exercise force and full moment accounting against once-only
gravity/aero plus four final wheel contributions, all force selectors, inertia
and centre shifts, bottoming caps/suppression and extra zero-time sweeps,
missing-contact/recontact, immediate wheel versus delayed body surface channels,
handbrake braking/recovery, mirrored steering and prescribed load consequences,
creation/reset/replacement, collision velocity and paused clock. At 30,60,90,144,
240 render Hz, 180 identical physics ticks produce byte-identical adapter state.
The mirrored pitch fixtures prescribe their pose; they do not demonstrate a
brake pedal causing dynamic pitch.

An independently written seeded Python/ctypes mathematical oracle compares the
new component library with formulas from the approved spec. It passes 240,008
checks: spring/contact/rate, negative-coordinate bilinear bump, blend, eta,
signed mph, alignment, rear grip (including unclamped interpolation and negative
bonuses), plus exact adjacent-binary32 eta threshold checks. This is synthetic
formula differential testing, not a comparison to original execution.
Private script and logs: `cleanroom/verification/stage5/oracle.py`, `oracle.log`,
`project-tests.log`. Binaries were independently built under /tmp.

A fresh composed adapter compiled with AddressSanitizer and UndefinedBehaviorSanitizer
passes with `ASAN_OPTIONS=detect_leaks=0`, exit 0. Default LeakSanitizer failed
because this environment uses ptrace; its fatal diagnostic is preserved in
`sanitizer.log`. No leak check is claimed. Successful run is in
`sanitizer-noleaks.log`. Existing baseline compiler indentation warnings remain.

## Composition inspection and boundaries

Inspection of `stage5_drive_step`, `sample`, `prepare`, `ct_post` and main's
selected adapter confirms held frame preload, grip/blend/pedals and exported
snapshots; first derivative retains old preparation. Post-step wheels use
ordered pair travel. Current scalar/aligned loads and four distinct surfaces
enter the tyre/brake path. Final contact point/basis forces and moments enter
once, after coupling. Stage5 returns before the old quarter-weight adapter or
arcade chassis handbrake path. Main forwards Space as the handbrake input.

The component/body interface exposes full forces/moments and both inertia
choices. The host integrates XY/yaw while vertical/pitch/roll come from the
independent ride trajectory. Main still steps that independent ride; its
vertical response is not a second Stage5 body-force integration. Cosmetic lateral
lean is distinct from the contact pose producer. Every reachable world triangle
uses supplied ID1; no retail world-material mapping was verified. Bottoming
requests and zero sweeps are present, but the host suppresses impulse application
because cap inputs remain unknown.

I5-1 remains OPEN and accurately describes this limit. S5-8 explicitly permits
prescribed planar fixtures and forbids claiming dynamic pitch/roll transfer.
Therefore no spec divergence was found, but dynamic brake/throttle pitch, full
roll/heave feedback, stock angular selection/damping, world ID mapping and
unknown impulse caps remain future reviewed producer work. ID1 is a gameplay
fixture, not a recovered mapping. No full Stage5 gameplay fidelity claim follows.

## Unverified behavior

No original executable was run, no original source/decompilation or private
analyst evidence was read, and no isolated original VM with no-network snapshot
was available. Retail differential, trajectories, stopping distance, slope/
landing parity, exact x87/trig/extreme-coordinate behavior and U5-1…U5-7 remain
unverified. No new amendment/reviewer cycle is required for the tested bounded
contract; future producer answers must pass a fresh reviewer before implementation.

## Final-sweep correction checkpoint

The implementer reopened the source freeze after its own audit found that the
upper-limit extra sweep refreshed wheel/contact geometry but did not repeat the
body post-step publication/clear/gravity work. The initial numeric tests above
do not establish that missing lifecycle behavior. The final-source lifecycle
result is recorded below after re-freeze and an independent affected rerun.
The checkpoint/final contact.c hash is
`9f235f741e18e63927c5e04ca2d61d96189a704d18e611bc1edde6c3b234565a`;
checkpoint/final drive_contact.c hash is
`4a4a3639846affff7521f5f31b62352338a68b705f362ddff80a179630788dd9`.

Additional independent project command
`make ground-motion-test world-collision-test fence-impact-test` exits 0:

```
ground_motion_test: PASS
world_collision_test: PASS
fence_impact_test: PASS (source role, solid/slow contacts, AI finish, visible fall, reload/reset, optional GPU)
```

Log: `cleanroom/verification/stage5/gameplay-tests.log`.

## Final-source lifecycle verification

The re-frozen source now performs body `res_post`, drivetrain `drv_post`, then
ordered zero-duration geometry at an upper-limit event. Independently reran
`make contact-test drive-stage5-test drive-stage1-test drive-stage4-test`, exit 0
(`final-tests.log`), and rebuilt/reran the final composed adapter with ASan/UBSan
and leak detection disabled, exit 0 (`sanitizer-final.log`). Numeric contact.c
remains unchanged, so the independently compiled numerical oracle remains valid.

An independent probe reuses only synthetic fixture setup, then observes the
first upper-limit zero sweep: body moment is zero and force is gravity alone,
selected clutch gear is 2, and momentum, pose and wheel spin equal the preceding
geometry sweep (no Euler). Final full force/moment accounting also passes. Output:

```
Independent extra sweep: body gravity/clear, clutch post, retained momentum/pose/spin and once-only final forces PASS
```

Private `post_sweep.c`, `post-sweep.log` and `final-source-hashes.txt` record the
probe and source identity. Its first harness compilation encountered an entry
point macro conflict, corrected only in the private harness; this was not a
product failure. Inspected `/tmp/stage5-zero-sweep-red.log` records the
implementer's failing pre-correction clutch assertion. This resolved lifecycle
implementation gap required no spec/vector amendment; no outstanding divergence
remains in the tested contract.

## Final native observation

Independently inspected completed final-source `/tmp/stage5-accepted.log` and
`/tmp/stage5-accepted_end.png`; verifier did not launch a second game. The
implementer reports strict, sanitizer, GLES and restored debug builds passing
and native debug exit 0. Those builds/exit are reported evidence, distinct from
the verifier's direct tests and inspection of completed telemetry/image.

A private independent parser (`native_check.py`, `native-check.log`) validates
78 finite Stage5 rows, nonnegative loads and completion summary/image. At
tick169 signed speed is 3.21296 m/s; rear-only H=5 kN·m is present for ticks
170…184, with front H=0 and speed falling to 1.49768 m/s. Blend falls from 1
to .666667 then .5. Tick185 has H=0 and blend .516667; subsequent .533333/.55
values establish recovery. This early segment has no wall/rail response.
Moving service torque at tick300 is nonzero and speed falls 8.95450→5.22972
→1.65270 m/s across ticks300/315/330; tick330 reports all wheel locks.
These are observed host trajectories, not measured retail curves.

Completed 900-tick summary: peak 61.67 km/h, final 20.16 km/h, travel 80.43 m,
106 wall responses, 0 rail responses, and all nan/oob flags zero. The final
image shows the vehicle beside the wall under an overpass; it renders normally
but establishes no handling or contact-accuracy parity. All sampled masks are
`f`; native liftoff/recontact and alternate material IDs were not demonstrated.
Synthetic composed fixtures provide their bounded evidence. The later wall
contacts preclude calling this a collision-free drive. Source hashes match
`final-source-hashes.txt`; no implementation changes followed the final checks.
