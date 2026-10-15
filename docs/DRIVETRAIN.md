# Driving with the Stage-1 drivetrain

Build the interactive debug application and opt in explicitly:

```sh
make debug
./nfsu2 '/path/to/your/NFSU2 installation' --car 240SX --drivetrain --hud
```

Omitting `--drivetrain` retains the existing arcade drivetrain. The selector
applies to the player in free roam, circuit, sprint and Street X. Drag and drift
use the existing race-mode physics; the console announces each change between
Stage 1 and legacy race-mode physics. Opponent/traffic dynamics are unchanged.
A car without valid stock Stage-1 parameters is rejected with a diagnostic;
car switching preserves the current car when the replacement lacks those fields.

W accelerates forward with automatic shifts. S brakes forward motion, then
selects reverse below 0.3 m/s; holding S accelerates backward. W brakes reverse
motion and resumes forward automatic selection near rest. Space stops and holds
the car. A/D steer normally. The pedal/direction policy is an OpenUG2 adapter;
it does not resolve the original game's input/menu mapping. Existing reset,
spawn, race restart and car-switch controls also reset the engine/clutch/wheels.
Pausing freezes those states and discards paused wall time on resume.

The console prints that Stage 1 is active. With `--hud`, gear, RPM, shift state,
shift RPM and limiter RPM come from the same drivetrain that drives the car.
Engine audio receives its actual RPM and shaped engine load. Wheel rotation and
wheel RPM diagnostics use the simulated wheel states. Brake lights use the
adapter's service-brake or handbrake state, including braking while reversing.

## What is implemented

The approved [Stage-1 specification](cleanroom-driving-physics/SPEC.md) supplies
stock engine torque and braking curves, clutch, gearbox/automatic shifts, pedal
shaping, axle torque distribution, wheel spin and longitudinal tyre coupling.
The adapter reads only the approved stock record fields from the application's
already loaded per-car table. It requires no additional extracted assets.
The persistent state and generated shift tables belong to the selected car.

Each existing 60 Hz gameplay tick supplies 1/60 s to Stage 1, subdivided into
four 1/240 s Euler steps. Forces from the preceding coupling advance body
forward velocity before the next coupling, matching the Stage-1 scheduling
boundary. The host still gathers world contact and resolves suspension and
collisions once per gameplay tick. The existing accumulator gives the same tick
sequence at different render rates, with at most eight catch-up ticks per frame.
This is OpenUG2 timing; the unresolved original residual clock is not emulated.

## Present limits

This mode is a playable Stage-1 integration, not a complete recreation of
original vehicle handling. The following inputs are explicit host adapters:

- Normal loads use a static quarter of vehicle weight on each supported wheel,
  gated by the existing suspension contact mask. Weight transfer and contact
  refresh within drivetrain substeps await Stage 5. No contact means no tyre
  thrust, steering or brake authority; airborne body momentum follows the host.
- Longitudinal coupling uses a symmetric straight-line contact slice. The
  chassis retains existing steering, lateral scrub and wall response. No
  original lateral tyre curve, yaw inertia or suspension/body solver is supplied.
  The complete rear multiplier is deliberately supplied as 1 for this gameplay
  approximation; it is not asserted as an original general-driving value.
- Existing surface grip is an adapter multiplier on stock tyre grip. Existing
  body drag is applied once across the substeps, separately from the specified
  wheel rolling term. Legacy propulsion and legacy off-throttle body braking
  are omitted; engine braking comes from Stage 1. Existing speed caps remain.
- Service braking retains the host's nominal 10.5 m/s² deceleration and its
  brake multiplier; supported wheels are kept at rolling speed during braking.
  Handbrake deceleration is 5.5 m/s² with locked wheels, applied once by the
  shared chassis; locked-wheel forces are not also added to the body. These are gameplay
  approximations, not the deferred original brake/ABS rules.
- Stock tuning is used regardless of the performance shop's power/transmission
  settings. Nitrous is disabled while Stage 1 is active. The acceleration slider
  does not rescale shaft torque; steering, brake and host speed-cap controls
  remain effective. Cosmetic wheel scaling does not change stock rolling radii.
- Shift jitter is suppressed. Engine scale and grip factor are 1; bonus and
  location effects are off; initial RPM is idle. The unresolved performance
  class correction is disabled (class 0). Manual assist and its menu return
  path are not selected. These choices do not resolve the spec's unknowns.

All approved trajectory vectors are mathematically derived. Neither the vector
suite nor gameplay captures establish original-game runtime fidelity.

## Repeatable checks

```sh
make drivetrain-test drive-stage1-test physics-clock-test
make debug
SDL_AUDIODRIVER=dummy ./nfsu2 '/path/to/your/NFSU2 installation' \
  --car 240SX --drivetrain --track STREAML4RA --resolution 640x360 \
  --drive-audit /tmp/stage1 --frames 480
```

The second command runs a native, scripted world drive and exits with captures
under `/tmp`; no original executable is launched. Omit the dummy audio driver
for normal interactive audio. `--drive-audit` now respects an explicit `--frames`
bound (its default remains 2,220). It prints `STAGE1` rows with actual gear,
target, RPM, clutch state, contact mask and signed body speed alongside the
existing movement/contact trace. Road obstacles can stop this scripted straight
line; the headless synthetic tests isolate launch/coast/brake/reverse behavior.

The synthetic integration suite covers stock-field mapping and validation,
FWD/AWD/RWD launch and automatic shifts, coast, braking, direction changes,
handbrake hold, steering, airborne authority, velocity impulses, reset/car
replacement, paused state and equal results at 30/60/90/144/240 render Hz.
It also runs the existing chassis self-test. No game assets are used by that
suite.
