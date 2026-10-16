# Independent Stage3 implementation design

Behavioral inputs were the approved Stage3 `SPEC.md` SHA-256
`d4689a16861df810159760a6a668ef3714966fd6c20a30e3b2b54f355c41c6f7`,
Stage1 `../SPEC.md` SHA-256
`61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb`,
and Stage2 `../stage2/SPEC.md` SHA-256
`d3072359aa25bf0ef541504805f70b3579829bb9b1310cda8ba1a568286c6cb6`.
All matched their approvals before implementation. Only approved shared
specifications/reviews/questions/vectors, the clean-room skill and permitted
independent OpenUG2 `src/`, `tools/`, `Makefile`, `DRIVETRAIN.md` and earlier
implementation designs were used. No original executable, direct original-data
inspection, analysis, evidence, original source, provenance, handoff or verification
report was accessed. Native smoke uses normal OpenUG2 asset loading only.

## Architecture and requirements

`src/braking.c/.h` is an independent pure C99/libm component. `BrkCar` owns three
supplied effective parameters; `BrkPedals` owns the entry pedal snapshot;
`BrkFrame` holds frame coefficients and requested torque; `BrkInput/BrkResult`
express one wheel preparation and its diagnostics. Temporary combined grip and
comparison diagnostics use double; persistent numeric values and supplied scalar
parameters use float. No original intermediate-precision claim is made (U3-4).

The existing independent drivetrain composes Stage3 explicitly through
`drv_brakes_supply`. Without this opt-in the approved Stage1 brake-free fixture
boundary remains available. Once enabled, `d.brakes.locked[]` owns persistent
locks; `d.env.wheel[].locked` mirrors the current evaluated state. Incoming
external environment lock fields no longer overwrite these persistent states.
The supplying call requires an initial four-wheel mask, relief-disable flag and
exclusivity switch. A caller entering an arbitrary saved state must also supply
wheel direction and the other drivetrain/wheel fields. `drv_init` still creates
direction stopped. Reconfiguration is an explicit boundary, not a per-frame reset.

| Specification | Independent implementation |
| --- | --- |
| S3-0/S3-1 | `BrkCar` strength units are kN m; rear share is dimensionless. `stage3_car_read` uses the permitted existing parser's car-row framing and reads only +0x384/+0x388/+0x38c. It rejects missing/truncated/nonfinite/negative strengths and invalid share; +0x380 is never substituted. Main startup and car replacement require all three valid fields. |
| S3-2 | `brk_pedals` is the single shared exclusivity operation used by `drv_frame_begin`. Engine deadzone/anti-stall follows it; service distribution uses the pre-engine-shaping throttle. `brk_distribute` computes linear front/rear coefficients and the strict low-speed simultaneous-pedal exception, then stores requested B. Full body-speed magnitude times the specified binary32 mph factor is sampled once at frame setup; coefficients remain fixed across coupling/substeps. |
| S3-3 | After initial drive distribution, `brk_prepare` skips signed B/H additions for an incoming lock, otherwise uses spin sign, including positive addition at zero. The combined torque gets the one-sided negative-slip relief only under the stated supplied flag/H/sign predicates. Requested B/H remain independent of relief and torque omission. No ABS timer, curve or mirrored reverse relief exists. |
| S3-4 | A motion predicate gates Q/A/C evaluation; bypass retains incoming omega/lock. `brk_compare` implements the exact A/C and A>1 predicates. Slip and combined magnitude are sampled before omega can snap; their double calculation remains the current evaluation's sample and float slip is stored for later consumers. Complete sliding mu is sampled with incoming lock/spin and reused through that evaluation. Zero load can still brake/lock spin without body force. |
| S3-5 | `drv_prepare_tyre` composes brake preparation with the independent existing tyre branch. Five gripping/unlocked force passes and clutch corrections remain in their prior order; they see updated locks. Final caps recompute player rear-speed multiplier from current lock/spin rather than reusing the pre-lock multiplier. Final force is shared by wheel derivative and body. `drv_wheel_derivative` updates smoothed net torque before locked torque/drag zeroing, then independently applies the gripping correction when h>0.001. Engine feedback, idle/limiter/clutch logic remain coupled under Stage1 rules. |
| S3-6 | `brk_direction_post` applies unconditional old-direction crossing correction, then strict binary32 epsilon classification. `drv_post` clears torque/forces/slip, retains smoothed torque, preserves angle presentation handling and runs engine/clutch post-processing. It can be used for a correction sweep without derivatives or Euler integration. |
| S3-7 | Frame setup does not rebuild already prepared torque or tyre force. The following derivative consumes that preceding coupling; the next coupling uses new frame coefficients. Creation completes zero-pedal preparation using the first host contact snapshot before this frame's new pedal distribution. Each substep prepares aero before initial drive/brakes/tyres/five passes/final cap/finishing, then accumulates tyre contact force once for the next body derivative. |
| S3-8 | Service-only mode supplies H=0. `brk_handbrake` converts later-stage supplied eta to H=10*Kh*eta; alternatively callers can inject per-wheel H directly into the explicit shared boundary. No eta, nonlinear boost, eligibility, grip blend, yaw, recovery or surface producer is inferred. |

## Playable host boundary

`--drivetrain` now loads stock Stage1/2/3 for its existing ordinary race family.
Default legacy dynamics, drag/drift fallback, input direction selection, HUD,
audio, wheel display, resets, pause clock, collision handling and steering paths
remain the existing host integration. W accelerates forward or brakes reverse;
S brakes forward then selects reverse near rest; Space retains its host hold.
The additional `STAGE3` drive-audit row reports requested pedal/strength/share,
relief flag, lock state, per-axle requested torque, force and spin.

Service braking has no host 10.5 m/s² deceleration, equivalent extra body force,
velocity stop clamp, brake slider multiplier, vehicle brake multiplier or forced
rolling-speed overwrite. Supported locked tyres now contribute their force to
body motion, exactly like unlocked tyres. This is covered by a force-accounting
fixture. Unsupported contacts have zero supplied load and no body force, while
wheel brake torque and lock rules continue to run.

The host explicitly supplies initial lock mask 0 on creation/reset and
`relief_disabled=false`, with exclusivity enabled. These are fixture choices,
not retail defaults (U3-2/U3-3). Reset keeps the configured strengths, initial
mask, relief switch and Stage2 environmental channels; it clears dynamic state,
seeds wheel spin from current host velocity and supplies matching direction.
New valid cars supply a new fixture; invalid Stage3 fields preserve the current
car. Pause calls no physics adapter; current host velocity imports collision
impulses at the next tick rather than restoring stale body motion.

Space remains an explicitly separate OpenUG2 handbrake adapter: engine throttle
is zero, spin is constrained to zero within host substeps, tyre body forces are
excluded while held, and the shared chassis applies its existing single 5.5 m/s²
stop/hold and lateral behavior. It does not assign persistent Stage3 service locks
or invent nonzero eta/H. This preserved gameplay control is outside the specified
nonzero-handbrake producer. No service-only fallback uses that direct body stop.

The same stock mass/radii/inertia, frame pedals/body speed, quarter-weight
contact-gated normal loads and unit rear multiplier feed the composed solver.
Loads remain the documented static host approximation; full rear grip, load
transfer, contact refresh, suspension and surfaces are later-stage supplied
boundaries. Stage2 aero is retained once; its vertical load is diagnostic in the
level-body XY adapter and does not feed normal loads. Steering/lateral scrub
remain the host approximation, with no claimed Stage4 curve or yaw inertia.
The host still uses 60 Hz ticks subdivided into four substeps, imports contact
once per host tick, and removes stale prepared tyre force on support loss or
Space hold. The upstream original clock remains unresolved. No stopping-distance
or original-game perceptual/runtime parity is claimed.

## Tests and results

Every one of the 54 approved Stage3 vector cases was encoded before implementing
behavior. The first compile failed on missing Stage3 drivetrain interfaces;
`/tmp/braking_first.log` retains that expected failure. `tools/braking_test.c`
checks every component output, exact Boolean/enum/diagnostic predicates and
preparation force at the fixture boundary. Numeric tolerance is the vector
README's tolerance; branch predicates do not use tolerance.

Additional synthetic boundaries cover frame-input latency, coefficients held
while speed changes, pre-engine-shaping pedals, supplied nonzero eta conversion,
pre-decision versus final-cap rear grip, engine reaction with incoming locked
wheels, derivative-free correction sweep and invalid explicit configuration.
The Stage3 suite has 479 checks. Existing Stage1 vectors/tests remain unchanged.
The gameplay suite now supplies synthetic Stage3 strengths/share/initial state
and adds approved-field mapping and rejection, legacy brake slider independence,
zero strength, airborne spin braking without body braking, locked contact-force
accounting and reset configuration. Existing launch/shift/coast/brake/reverse,
Space hold, steering, airborne, velocity-impulse, replacement, pause and equal
30/60/90/144/240-Hz render clock assertions remain unchanged and pass.

Final synthetic suite:

```sh
make braking-test drivetrain-test resistance-test drive-stage1-test \
  physics-clock-test district-collision-test
```

Results: Stage3 **479 checks, 0 failures**; Stage1 **3,606 checks, 0 failures**;
Stage2 **348 checks, 0 failures**; gameplay adapter, physics clock and district
collision all pass. Output: `/tmp/stage3_final_tests.log`.

The strict standalone command is:

```sh
cc -std=c99 -Wall -Wextra -Werror -pedantic -Isrc \
  tools/braking_test.c src/braking.c src/drivetrain.c -lm \
  -o /tmp/stage3_braking_strict
/tmp/stage3_braking_strict
```

The changed adapter/test/core source also passes strict C99 syntax checking with
`-D_DEFAULT_SOURCE -Wall -Wextra -Werror -pedantic -Wno-unused-function
-Wno-misleading-indentation -Isrc -fsyntax-only`. Suppressing inherited independent
parser indentation warnings is needed. An attempted full `-pedantic -Werror`
adapter build failed on existing pre-C23 array-qualifier conversions in
`src/physics.c`; that untouched baseline is outside this change. Full adapter
compilation/execution passes with the same flags except `-pedantic` omitted.

Both standalone and gameplay suites pass AddressSanitizer/UndefinedBehaviorSanitizer
with `-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer` and
`ASAN_OPTIONS=detect_leaks=0`. Leak checking was first attempted but the environment
reports LeakSanitizer cannot run under ptrace; it is not a passed leak check.
The binaries are `/tmp/stage3_braking_san` and `/tmp/stage3_drive_san`.

No new specification question was necessary. U3-1 through U3-7 and companion
unknowns remain explicit. Specifications, shared vectors and QUESTIONS.md were
not edited. No staging, commits or pushes were performed by this role.

Both `make gles` and `make debug` passed on the final source. Build logs are
`/tmp/stage3_gles.log` and `/tmp/stage3_debug.log`; `nfsu2` is left as the debug
build. Existing unrelated compiler warnings remain. `git diff --check` passed
for this implementation's changed files.

The initial native run used a 60-second timeout and `--frames 2100`. It timed
out with exit 124 around tick 1020 on this machine, before the audit's brake
phase; `/tmp/openug2_stage3.log` records that incomplete run. Its spawn capture
was inspected: car, world and HUD are visible. A longer bounded run uses the
same existing script with a 180-second timeout:

```sh
timeout 180 env DISPLAY=:0 SDL_AUDIODRIVER=dummy ./nfsu2 \
  '/home/dq/Games/Need for Speed Underground 2' \
  --car 240SX --drivetrain --hud --track STREAML4RA --resolution 640x360 \
  --drive-audit /tmp/openug2_stage3_final --frames 2100 \
  >/tmp/openug2_stage3_final.log 2>&1
```

The 2100-tick native run exited 0. Summary: peak 69.62 km/h, final 0.15 km/h,
193.13 m traveled, 1,176 wall responses, and `nan=0 oob_z=0 oob_xy=0`.
The end capture was visually inspected. However, its scripted turns stalled
against a wall before tick 1920; S then selected reverse instead of service
braking. Its Stage3 rows correctly reported brake=0. This run validates the
running integration and diagnostics but does not by itself exercise native
service braking. The synthetic launch-then-brake/body-wheel-lock fixtures above
remain the direct braking evidence. Native service braking was not exercised
in this run; no native body/wheel/lock response to service input is claimed.

Existing HUD texture decoding reports unsupported/missing artwork; complete HUD
art is not verified. Dummy audio exercises the audio-input path but does not
validate audible output. Pause and car-switch menus were not automated; their
pure reset/replacement/clock boundaries are covered synthetically.
