# Stage4 independent implementation

Approved input: SPEC.md SHA-256
`02144d187323c6d74a6fb21e6e0b3e2842097cd0e28c935eea97907c1cbc911a`.
The implementer checked this against REVIEW.md before reading behavioral inputs.
Companion Stage1/2/3 hashes also match their approvals. Behavioral inputs were
only these shared specifications, reviews, questions and vectors. Existing
independent src/, tools/, Makefile and docs/DRIVETRAIN.md were permitted baseline.
No original executable, source, extracted assets, analyst evidence, verification
reports, PROVENANCE.md or HANDOFF.md were read. Native checks execute OpenUG2 and
use its normal application asset loader; they do not execute the original game.

## Components and traces

- **S4-1/2:** `lateral.c` owns small scalar calibration curves, bounded keyboard
  and analog filters, latch, countersteer, keyboard approach rate, and the two
  signed integer truncations. `LatFilter` requires h/e, pedals, previous command
  angle/limit, sigma and speed explicitly. `lat_decode` decodes the held command.
  `lat_body_slip` quantizes the supplied world body-forward and velocity headings,
  subtracts and wraps their difference, with the signed forward <1 zero case.
- **S4-3:** `lat_geometry` implements the rational two-front-angle mapping with
  binary32 unsigned-command conversion and no tangent-based substitution. Angles
  are held through all substeps. Standard libm sine/cosine is a U4-3 adapter.
- **S4-4/5:** `lat_heading` evaluates the mathematical characterization and its
  sparse corrections; it stores no wholesale original heading table. `lat_slip`
  folds reverse travel. `lat_contact` samples the body velocity plus yaw cross
  mount arm, then projects into each wheel frame. `lat_grip` interpolates the
  seven load curves and then angle, applying only the requested lateral multiplier.
  `lat_sliding` exposes the strict bounded branch predicate for vector tests.
- **S4-0/5/6:** the actual Stage1 tyre preparation retains its Stage3 pre-lock slip
  snapshot and newly decided lock. Gameplay supplies the unweighted gripping
  curve, and existing tyre preparation applies wT once. The five coupling passes
  modify longitudinal force only. `DrvBodyPlanar` supplies pure provisional
  acceleration from both wheel components and external force/moment without
  touching persistent body accumulators. `drv_couple` keeps its one combined cap
  and post-cap correction, now with binary32 .15 and explicit Stage4 peak_scale.
  Its Stage4 low-speed sliding normalization uses signed forward speed; magnitude
  remains available for the independent rear player-speed factor and Stage3 frame
  brake distribution. Earlier standalone Stage1 clients retain their established
  supplied-speed boundary unless they select `planar_tyres`.
- **S4-6:** final wheel components rotate to body/world basis and add once to
  `ResBody.force`; contact-arm moments add once to `ResBody.moment[2]`. The final
  force also remains in wheel spin state. There is no second cap, wT factor,
  surface multiplier on the gripping curve, normal-force addition, steering yaw
  assignment, or host lateral scrub. `lat_cap` is the standalone scalar boundary;
  the drivetrain executes the same operation in its existing cap location.

## Gameplay boundaries and independent adapter choices

`stage4_car_read` reads only the S4-1 approved fields through the existing
independent validated GLOBALB framing. Invalid finite/positive domains reject
load/switch; there is no fabricated fallback car tuning. Main requires all four
stages for `--drivetrain`; the default, traffic, drag and drift paths retain their
existing behavior. Car switching validates Stage4 before the transaction commits.

The host uses its existing fixed 60 Hz ticks and Stage1's four 1/240 s substeps.
Frame input updates coefficients/command, leaving the previous prepared world
forces and moments for the first derivative. After each derivative/Euler step,
Stage2 clears/saves force and moment, wheel/shaft post-processing completes,
contact velocity is sampled with the current yaw basis, aero is prepared, then
wheel coupling prepares the next force. The frame's steering angles are fixed;
contact frames refresh after substeps. Position publication remains the host's
once-per-tick final-velocity update. This is an OpenUG2 host approximation.

Yaw uses persistent planar angular momentum, supplied inverse inertia and
`moment - supplied_damping * angular_momentum`, with the same substep h as body
and wheels. Heading advances from the pre-integration yaw rate. Gameplay supplies
stock `m*(dx²+dy²)*b32(1/12)` box inertia, zero centre offset, unadjusted stock
mounts as both velocity arms and contact application arms, and zero additional
angular damping/assistance torque. This is an explicit host choice for U4-5,
not an implementation of the unknown original angular law. Q4-I2 records the
missing law. Existing external moment and aero torque remain in the accumulator;
Stage2 translational stabilization receives actual absolute yaw speed. Collision
position/velocity impulses remain the independent host response and feed the next
contact sample; there is no added collision angular impulse or new Stage5 solver.

The host explicitly supplies static supported-quarter-weight normal loads,
existing contact mask held for the tick, existing surface acceleration as tyre
surface peak/slide, q=1, location response factor=1, complete rear multiplier=1,
peak_scale=1, ordinary mode, no upgrades/damage/nitrous. These values are host
fixtures, including during turns; they are not claimed complete retail producers.
The existing `grip_q*(1-.025*grip_loc_x)` supplies the effective combined frame and
response factor at the Stage1 boundary; callers may provide effective values.
Normal force and vertical suspension are still the host's separate responsibility.
Airborne means zero load and zero prepared body tyre forces/moments; body aero and
yaw momentum persist. Tilted world contacts still use this level-plane reduction.

For U4-1/2 the host passes raw keyboard-equivalent axis to the bounded keyboard
kernel without the old exponential steer smoother. It supplies a fixed-tick FIFO
mean of 33 previous input samples and nine rate samples (including current),
seeded with input zeros and rate ones. This is a documented independent producer,
not an inferred original expiry/startup rule. Q4-I1 leaves that original producer
unimplemented. The previous frame's exported signed speed is held as vctl; reset
explicitly publishes host velocity. h/e producers and command/geometry refresh
only on simulation ticks; pausing and render cadence do not mutate them.

Reset retains stock/tunable inputs and reinitializes command, latch, previous
limit=45, histories, yaw state and prepared state, then seeds rolling speed from
host velocity. Car replacement creates independent clean state. Space remains
outside ordinary Stage4: the host excludes body tyre forces and moments, constrains
wheel/yaw spin, and retains its existing linear stop/hold deceleration. Service
brakes remain Stage3. No original handbrake/rear-grip blend is invented.

The player pose carries the actual two front angles through `ai_render_pose`.
Both wheel and brake-hub presentation use those per-wheel angles (rear zero),
interpolated only for drawing. Debug demo steering remains a rendering override.

## Validation and exact commands

All 72 approved vectors were encoded before implementation: initial build failed
for missing lateral.h; final `lateral-test` passes 208 checks with each vector's
own tolerances and exact integer/Boolean checks. No vectors/spec were edited.
`drive-stage4-test` covers mirror/zero turns, brake+turn, reverse, once-only final
force/moment sums, actual wT/peak scaling, signed reverse sliding normalization,
external provisional moment, body-slip heading convention, host steer/scrub bypass,
zero-load air with retained yaw, collision velocity impulse, reset/replacement and
identical full adapter state at 30/60/90/144/240 Hz. `race-ai-test` covers actual
front-angle render interpolation and existing AI/animation behavior.

Commands run from the repository root:

```sh
make lateral-test drivetrain-test resistance-test braking-test drive-stage1-test
make drive-stage4-test race-ai-test physics-clock-test district-collision-test ug2-track-collision-test
make lateral-test drivetrain-test resistance-test braking-test drive-stage1-test drive-stage4-test CFLAGS='-O2 -std=c99 -Wall -Wextra -Werror -Wno-unused-function -Wno-misleading-indentation'
make debug
env DISPLAY=:0 SDL_AUDIODRIVER=dummy ./nfsu2 '/home/dq/Games/Need for Speed Underground 2' --car 240SX --drivetrain --track STREAML4RA --resolution 640x360 --drive-audit /tmp/stage4-native --frames 300
```

The bounded native run exercised only scripted input through production physics:
ticks 210–239 steer +.35, ticks 255–284 steer -.35, separate from the existing
300–344 service-brake pulse. No physical state was forced. `/tmp/stage4-native.log`
and spawn/end PNGs retain results. Summary: 14.96 m travel, 43.90 km/h final,
wall-response=0, rail-response=0, nan/oob flags zero. Tick210 at 3.91 m/s had
front angles .05749/.05954 rad, front slip -.02148 rad, front Fy +2.17664 kN,
yaw +.05419 rad/s. Tick240 heading was 1.96989 from initial 1.90725. Tick270
negative steer had angles -.05570/-.05391 and yaw -.21491 rad/s at 9.3 m/s.
Small asymmetries/straight yaw can arise from stock mount/load fixture and binary32
quantization; the separate symmetric fixture mirrors within explicit tolerances.
These native checks validate this independent host integration, not retail parity.

Additional build/sanitizer results are recorded below after completion.

Final validation: all component and adapter strict checks pass. The full
ASan+UBSan run passes with no reported memory/undefined-behavior errors:

```sh
ASAN_OPTIONS=detect_leaks=0 make lateral-test drivetrain-test resistance-test braking-test drive-stage1-test drive-stage4-test CFLAGS='-O1 -g -std=c99 -Wall -Wextra -Wno-unused-function -Wno-misleading-indentation -fsanitize=address,undefined -fno-omit-frame-pointer'
make gles
make debug
```

The initial sanitizer attempt stopped because LeakSanitizer reports that it
cannot work under process tracing in this sandbox; leak checking is therefore
not claimed. The repeated run explicitly disables that unsupported feature.
Logs: `/tmp/stage4-sanitize.log`, `/tmp/stage4-strict.log`,
`/tmp/stage4-final-components.log`, `/tmp/stage4-extra.log`,
`/tmp/stage4-collision-clock.log`, `/tmp/stage4-gles.log`,
`/tmp/stage4-debug-final.log`. Existing independent parser/physics indentation
warnings occur in ordinary builds; the strict commands suppress that known
baseline warning class. New lateral/adapter code emits no such warnings.
Stage1: 3,606 checks; Stage2: 348; Stage3: 479; Stage4: 208, all passing.
Both gameplay adapter suites and clock, district/track collision and AI/animation
regression suites pass. GLES completed successfully; debug is rebuilt last.
All four approved specification hashes and the approved Stage4 vector hash
remain unchanged. Added questions: Q4-I1 and Q4-I2; no specification amendments.

After final debug restoration, a second input-only native capture completed:

```sh
env DISPLAY=:0 SDL_AUDIODRIVER=dummy ./nfsu2 '/home/dq/Games/Need for Speed Underground 2' --car 240SX --drivetrain --track STREAML4RA --resolution 640x360 --drive-audit /tmp/stage4-steering --frames 275
```

`/tmp/stage4-steering.log` repeats the moving tyre/yaw measurements above with
zero wall/rail responses and zero nan/oob flags. Final speed 34.91 km/h, travel
10.38 m, actual wheel presentation -3.1 degrees. The final PNG was visually
inspected: the car is supported on the road, with no visible wall contact.
Captures: `/tmp/stage4-steering_spawn.png`, `/tmp/stage4-steering_end.png`.
The restored `nfsu2` remains the desktop debug build. No staging, commit or push
was performed by this implementation session.
