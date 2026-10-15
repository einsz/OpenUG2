# Driving with the Stage-1/2/3/4 drivetrain

Build the interactive debug application and opt in explicitly:

```sh
make debug
./nfsu2 '/path/to/your/NFSU2 installation' --car 240SX --drivetrain --hud
```

Omitting `--drivetrain` retains the existing arcade drivetrain. The selector
applies to the player in free roam, circuit, sprint and Street X. Drag and drift
use the existing race-mode physics; the console announces each change between
Stage 1/2/3/4 and legacy race-mode physics. Opponent/traffic dynamics are unchanged.
A car without valid stock Stage-1/2/3/4 parameters is rejected with a diagnostic;
car switching preserves the current car when the replacement lacks those fields.

W accelerates forward with automatic shifts. S brakes forward motion, then
selects reverse below 0.3 m/s; holding S accelerates backward. W brakes reverse
motion and resumes forward automatic selection near rest. Space stops and holds
the car. A/D steer normally. The pedal/direction policy is an OpenUG2 adapter;
it does not resolve the original game's input/menu mapping. Existing reset,
spawn, race restart and car-switch controls also reset the engine/clutch/wheels.
Pausing freezes those states and discards paused wall time on resume.

The console prints that Stage 1/2/3/4 is active. With `--hud`, gear, RPM, shift state,
shift RPM and limiter RPM come from the same drivetrain that drives the car.
Engine audio receives its actual RPM and shaped engine load. Wheel rotation and
wheel RPM diagnostics use the simulated wheel states. Brake lights use the
adapter's service-brake or handbrake state, including braking while reversing.

## What is implemented

The approved [Stage-1 specification](cleanroom-driving-physics/SPEC.md) supplies
stock engine torque and braking curves, clutch, gearbox/automatic shifts, pedal
shaping, axle torque distribution, wheel spin and longitudinal tyre coupling.
The approved [Stage-2 specification](cleanroom-driving-physics/stage2/SPEC.md)
adds quadratic world body drag, vertical aerodynamic force calculations, body
stabilization, a per-car positive-forward-force threshold and the 200 m/s cap.
The approved [Stage-3 specification](cleanroom-driving-physics/stage3/SPEC.md)
adds stock service-brake strengths/share, linear pedal distribution, signed wheel
torque, one-sided slip relief, lock decisions and direction-crossing correction.
Service braking reaches body motion through the final tyre force exactly once.
The adapter reads only the approved stock record fields from the application's
already loaded per-car table. It requires no additional extracted assets.
The persistent state and generated shift tables belong to the selected car.

Each existing 60 Hz gameplay tick supplies 1/60 s to Stage 1, subdivided into
four 1/240 s Euler steps. Forces from the preceding coupling advance body
XY momentum before the next coupling, matching the Stage-1 scheduling
boundary. The host still gathers world contact and resolves suspension and
collisions once per gameplay tick. The existing accumulator gives the same tick
sequence at different render rates, with at most eight catch-up ticks per frame.
This is OpenUG2 timing; the unresolved original residual clock is not emulated.

## Present limits

This mode is a playable Stage-1/2/3/4 planar integration, not a complete recreation of
original vehicle handling. The following inputs are explicit host adapters:

- Normal loads use a static quarter of vehicle weight on each supported wheel,
  gated by the existing suspension contact mask. Weight transfer and contact
  refresh within drivetrain substeps await Stage 5. No contact means no tyre
  thrust, steering or service-brake body force; wheel brake torque still affects
  airborne spin, with zero load. Airborne body momentum follows Stage2 resistance.
- Steering uses the approved Stage4 bounded keyboard filter, front geometry,
  folded contact slip and lateral load curves. Final combined wheel forces
  advance planar body and yaw momentum. The host yaw assignment and lateral
  scrub are replaced. Wall response and suspension remain host approximations.
  The complete rear multiplier is deliberately supplied as 1 for this gameplay
  approximation; it is not asserted as an original general-driving value.
- Existing surface grip is an adapter multiplier on stock tyre grip. Existing
  body surface drag/coast retention and host forward/reverse speed caps are
  replaced by Stage-2 body aerodynamics, stabilization and force/cap rules.
  Engine braking remains Stage 1 and wheel rolling/damage torque is counted once.
  Aerodynamics is prepared before wheel coupling and consumed by the following
  body derivative without an extra resistance pass.
- Service braking uses Stage3 wheel torque, relief and lock state. The old
  nominal 10.5 m/s² host deceleration, direct stop clamp and forced rolling spin
  are removed. Locked supported tyres contribute their final contact force.
  Space retains the existing host 5.5 m/s² stop/hold and wheel-spin constraint;
  while held, tyre forces are excluded from body motion to avoid double braking.
  This is a documented OpenUG2 handbrake adapter. The full nonzero handbrake
  coefficient, rear-grip blend and yaw behavior await Stage5; no retail handbrake
  producer is inferred from Space. The standalone module accepts supplied H or
  converts supplied eta using `H=10*Kh*eta`.
- Stock tuning is used regardless of the performance shop's power/transmission
  settings. Nitrous is disabled while Stage 1 is active. The acceleration slider
  does not rescale shaft torque. The legacy braking slider and vehicle brake
  multiplier do not scale stock Stage3 service torque. Steering controls remain
  effective. The host top-kmh slider no longer limits this mode. Cosmetic wheel scaling does not change stock rolling radii.
- Shift jitter is suppressed. Engine scale and grip factor are 1; bonus and
  location effects are off; initial RPM is idle. The unresolved performance
  class correction is disabled (class 0). Manual assist and its menu return
  path are not selected. These choices do not resolve the spec's unknowns.

All approved trajectory vectors are mathematically derived. Neither the vector
suite nor gameplay captures establish original-game runtime fidelity.

## Repeatable checks

```sh
make lateral-test drive-stage4-test braking-test resistance-test drivetrain-test drive-stage1-test physics-clock-test district-collision-test
make debug
SDL_AUDIODRIVER=dummy ./nfsu2 '/path/to/your/NFSU2 installation' \
  --car 240SX --drivetrain --track STREAML4RA --resolution 640x360 \
  --drive-audit /tmp/stage3 --frames 2100
```

The application command runs a native, scripted world drive and exits with captures
under `/tmp`; no original executable is launched. Omit the dummy audio driver
for normal interactive audio. `--drive-audit` now respects an explicit `--frames`
bound (its default remains 2,220). It prints `STAGE1` rows with actual gear,
target, RPM, clutch state, contact mask and signed body speed alongside the
existing movement/contact trace. `STAGE2` rows expose prepared drag/force, z,
vertical aero load, threshold parameter, scale and reduction. `STAGE3` rows expose pedal, strength, rear share, relief
switch, persistent locks, requested brake torque, tyre force and spin. The scripted
audit with `--drivetrain` also requests straight-line S during ticks 300–344,
then resumes acceleration; `--frames 420` includes braking and its release.
The later audit requests S at tick 1920; a 2100-tick bound reaches that section. If the
car has stalled against an obstacle, S selects reverse instead of service braking.
Road obstacles can stop this scripted path; the headless synthetic tests isolate launch/coast/brake/reverse behavior.

The synthetic integration suite covers stock-field mapping and validation,
FWD/AWD/RWD launch and automatic shifts, coast, braking, direction changes,
handbrake hold, steering, airborne authority, velocity impulses, reset/car
replacement, paused state and equal results at 30/60/90/144/240 render Hz.
It also runs the existing chassis self-test. No game assets are used by that
suite.


Stage 2 uses an explicit host fixture: G=1, a=0, ordinary state 0, unit body
and rolling surface factors as separate channels, no bonus/damage/alternate mode,
and a supplied zero additional angular damping coefficient. Stage4 supplies actual
yaw speed to the Stage2 stabilization curve. These choices do not establish the unknown original input
producers or reduction initialization. Stock cD/cF/cR/L are decoded from the same
validated car row; invalid fields reject loading/switching rather than substituting
coefficients. Reset preserves supplied tuning/environment and resets z to zero.

The playable adapter supplies a level body basis from heading and integrates XY
momentum. Existing suspension still owns vertical motion, pitch and roll. Vertical
aero load/moments are calculated for diagnostics but do not feed back into loads
or suspension, and the planar constraint removes their direct vertical motion.
The standalone module supports and tests full 3D/sloped aero, the slope-dependent
threshold and full-vector cap; the playable adapter does not reproduce those
vertical/slope effects. It makes no complete world-motion or runtime fidelity
claim. See [Stage-2 design](cleanroom-driving-physics/stage2/IMPL_DESIGN.md) for
precise scheduling, supplied-input choices and validation.


Stage3 creation and reset explicitly supply initial lock mask 0 and
`relief_disabled=false`; exclusivity is enabled. These are host fixture choices,
not established original initialization or flag producers (U3-2/U3-3). The
standalone configuration requires those inputs explicitly and retains lock state
across pedal release until the specified motion predicate reevaluates it. Reset
restores the configured initial mask and retains the strengths and relief switch.
The once-per-frame speed snapshot uses the full planar magnitude, while wheel
brake sign uses spin. Frame input changes leave the preceding coupling's torque
and force prepared for the first derivative; the following coupling uses the new
coefficients. Detailed architecture and validation are in
[Stage3 design](cleanroom-driving-physics/stage3/IMPL_DESIGN.md).


The approved [Stage4 specification](cleanroom-driving-physics/stage4/SPEC.md)
adds ordinary keyboard/analog filter kernels, exact unsigned steering commands,
front wheel geometry, wheel-frame velocities, folded slip, gripping lateral load
curves, combined grip and planar yaw moments. The interactive host uses its raw
keyboard-equivalent axis, with independent 60 Hz box histories (33 prior input
samples and nine rate samples including current, initialized to zero/one).
The original timed-history producer remains unknown. Command and wheel geometry
update once per tick, without applying the old steering smoother a second time.
Actual per-wheel front angles also drive the interpolated wheel/brake-hub meshes.

The yaw adapter uses stock body dimensions for box inertia, unadjusted stock mounts
as contact arms, zero centre offset, and supplied zero extra angular damping or
yaw assistance. Loads remain static quarter weights, and complete rear grip and
peak scales are explicitly 1 even while turning; these are documented independent
host choices. Full angular damping, suspension/load/centre/contact producers and
3D force bases await later specifications. Airborne tyre forces/moments are zero,
while yaw momentum and body aero persist. Space additionally constrains yaw spin
as part of the existing host hold. Reset/car switching clears command, histories,
latch, yaw state and prepared forces while retaining selected stock tuning.

`--drive-audit` adds moving steer pulses at ticks 210–239 (+.35) and 255–284
(-.35), before the service-brake pulse at 300–344. `STAGE4` rows report command,
front angles, contact speed, folded slip, lateral forces, yaw rate/moment, heading,
world velocity and wall/rail response counts. A 300-frame run reaches both steer
pulses; 420 frames also covers service braking. See
[Stage4 design](cleanroom-driving-physics/stage4/IMPL_DESIGN.md) for composition,
exact validation commands, results, unknowns and adapter limitations.
