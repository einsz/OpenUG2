# Independent Stage-2 implementation design

Behavioral input: Stage-2 `SPEC.md` SHA-256
`d3072359aa25bf0ef541504805f70b3579829bb9b1310cda8ba1a568286c6cb6`
and Stage-1 `../SPEC.md` SHA-256
`61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb`,
checked against both approved reviews before implementation. Only the approved
specifications, reviews, questions and vectors, the clean-room skill, and the
coordinator-permitted independent `src/`, `tools/`, `Makefile`, `DRIVETRAIN.md`
and Stage-1 implementation design were used. No original executable/data,
analysis, evidence, provenance, handoff or verification reports were inspected.
Native OpenUG2 smoke uses ordinary asset loading through its existing independent
parsers; it is not original-game execution or direct data extraction.

## Architecture and traceability

`src/resistance.c/.h` is a standalone C99/libm module without IO, SDL or GL.
`ResCar` holds supplied effective coefficients; `ResInput` holds explicit mode,
scale, reduction, throttle and contact inputs. `ResBasis` holds orthonormal world
axes. `ResBody` owns binary32 momentum, velocity, force/moment accumulators and
stabilization state. Callers must keep momentum and velocity consistent.
`ResAero`, `ResLimit` and `ResDerivative` expose derived diagnostics.

| Specification | Independent design |
| --- | --- |
| S2-0/S2-1 | Finite valid supplied-input contract. `stage2_car_read` reads only four approved stock row fields through existing independent framing; mass remains the Stage-1 mass. Invalid/missing stock coefficients reject creation and car replacement. No whole-car table or new extracted asset is embedded. |
| S2-1 | `res_frame_inputs` maps only established state codes; other codes retain supplied mode values. Body surface factors are averaged only across the supplied contact mask. Wheel rolling surfaces and damage mask are separate adapter channels. Initial reduction is a mandatory initializer argument. |
| S2-2 | `res_drag` stores raw world components once as binary32. `res_prepare_aero` applies drag once at its world-relative arm, then front and rear aerodynamic forces in order. It has no wheel-contact gate on drag. |
| S2-3 | Full world axes, low-speed fallback, alternate orientation curve, low-contact upright factor, airborne front/rear fractions and reported load before enable are implemented. A private point-force helper caps each force at 1200 and its moment at 100, independently; summed forces are not capped. `ResAero.drag` reports raw drag while front/rear report enabled capped applied vectors. |
| S2-4/S1-8.1 | `drv_wheel_resistance` centralizes unsmoothed rolling/damage torque with the prescribed binary32 constants. The existing single wheel derivative consumes it, before the existing gripping slip correction. There is no additional body rolling force. |
| S2-5 | Local diagnostic helper and world derivative share the threshold arithmetic. World application replaces only the forward projection of the stored force. Horizontal speed and positive-forward force remain independent strict predicates. No velocity clamp is derived from L or gearing. |
| S2-6 | Curve samples are binary32; knots are equally spaced mathematically at i/7. The derivative uses pre-integration z and momentum. z evolves even when translational stabilization is disabled; post-step clamps only negative z. |
| S2-7 | Separate derivative/integrate/post operations allow all blocks' derivatives to use old state. Post-step applies the full-vector 200 m/s cap with mass compared to binary64 0.7 and a binary32 speed divisor, clamps z, saves/clears forces and moments, then optionally seeds gravity. Repeating a post sweep performs no integration. |
| S2-7/S2-8 | Gameplay post-step prepares aero before five-pass Stage-1 coupling. The coupling callback receives that external force; wheel forces join the same accumulator afterward. The next derivative consumes the prepared force once. No new terminal-speed equilibrium or gearing clamp is imposed. |

## Gameplay boundary and explicit supplied choices

The existing `--drivetrain` mode now uses Stage 1 plus Stage 2 for its supported
ordinary race family. The default legacy mode and drag/drift fallback keep their
existing physics. The adapter retains the existing 60 Hz host clock with four
Euler substeps per tick, subject to original-clock unknown S1 U-5.

The host remains a **level-body XY longitudinal slice**: forward/lateral axes
come from heading, up is world z, angular speed supplied to stabilization is 0,
and XY metres/tick are converted to m/s and momentum using the stock mass.
This same basis, mass, speed, contact snapshot, shaped throttle and h feed both
stages. Existing host suspension owns vertical motion, pitch and roll; its
orientation/rates are not mixed into a planar tyre acceleration callback.
The adapter computes/reports vertical aero load and moments, but constrains the
Stage-2 body to its XY slice, discarding vertical force before integration.
It does not feed aerodynamic loads into normal-load/suspension feedback. Thus
full sloped 3D aero, slope-dependent L and vertical components of the global cap
are implemented/tested in the standalone module, **not represented by this
playable planar adapter**. This is not complete Stage-2 world-motion fidelity.

Creation explicitly supplies ordinary state code 0; G=1; a=0; no instance bonus,
no damage, no alternate movement/aero/global override; unit body-drag surface and
unit rolling-drag surface factors, independently. The known state mapping yields
e=1 and stabilization enabled. z starts at 0. G/bonus are copied consistently to
the drivetrain; there is no exceptional engine-only multiplier in this fixture.
Reduction/modes/surface arrays/damage remain caller-supplied fields. These are
host fixture choices, not discoveries of unknown original initialization or
surface/mode/damage producers. Existing grip surface multipliers remain supplied
separately through the host. Per-wheel loads remain static quarter-weight,
contact-gated; rear multiplier remains the documented Stage-1 synthetic value 1.

`stage1_drive_init` requires valid Stage-1 stock parameters, Stage-2 coefficients
and an explicit initial reduction. It initializes z=0 and performs zero-time
body/neutral drivetrain post-processing. The first available host contact
snapshot completes zero-time aero/coupling before the first ordinary derivative.
Reset preserves supplied coefficient/environment settings but clears transient
shaft/clutch, forces, prepared-force flags and z; wheels seed from host speed.
A valid replacement car starts a fresh stock fixture and invalid fields preserve
the current car. Pause does not call the adapter. Collision/steering impulses
are imported from current host velocity at the next tick, avoiding stale
momentum restoring pre-collision motion. Newly unsupported or locked wheel
thrust is removed from the prepared accumulator as an explicit host authority
policy, without adding an extra resistance pass.

Service brakes retain the host deceleration and supported-wheel rolling policy;
handbrake retains its single shared-chassis stop. Steering, lateral scrub,
contact refresh and wall response remain independent approximations. Those
retained lateral/brake effects are distinct from removed longitudinal host
surface retention/coast drag. The opt-in chassis bypasses all host road/top-kmh
and reverse caps. Its speed rules are now S2-5/S2-7; the default chassis path is
unchanged. Nitrous and upgrades remain disabled in this mode. Reset, presentation,
audio inputs, wheel animation and lights retain the existing Stage-1 state path.

## Validation

Every one of the 52 approved cases was encoded before implementation; the first
compile failed because `resistance.h` did not exist. `make resistance-test` now
checks all cases plus independent caps, sloped forces/projections, mode retention,
contact-arm thresholds, low-speed fallback strictness, world-force replacement,
curve knots, scalar-state behavior, accumulator clearing and gravity seeding.
The vectors demonstrate formula consistency, not original runtime equivalence.

The gameplay suite adds an independently calculated four-substep airborne drag
trajectory, host drag/cap independence, positive-force replacement without speed
clamping, symmetric absolute cap, distinct surface/damage channels, decoder
validation and reset retention. Its former no-change airborne lateral expectation
now expects the approved aerodynamic slowing. Existing launch/shift, coast,
brake/reverse, lock/hold, steering, collision impulse, reset, replacement, pause
and render-rate timing tests remain and pass. All existing Stage-1 vectors are
unchanged. No new unanswered question was necessary; S2-Q6/Q8 remain open and
S2-Q7 remains deferred, as do the Stage-1 unknowns.

Validation commands/results and native smoke are recorded below after completion.

Completed checks (2026-10-15):

- `make resistance-test`: **348 checks, 0 failures**, covering all 52 cases.
- `make drivetrain-test`: **3,606 checks, 0 failures**, unchanged vector inputs.
- `make drive-stage1-test physics-clock-test district-collision-test`: passed.
- `cc -std=c99 -Wall -Wextra -Werror -pedantic -Isrc tools/resistance_test.c src/resistance.c src/drivetrain.c -lm -o /tmp/resistance_strict` and execution: passed.
- Strict syntax check of adapter/integration test initially failed on inherited
  misleading-indentation warnings in `src/nfsu2.h`; repeating with only
  `-Wno-misleading-indentation` added passed. No unrelated parser style edits made.
- Standalone Stage-2 and gameplay integration AddressSanitizer plus
  UndefinedBehaviorSanitizer (`-O1 -g -fsanitize=address,undefined
  -fno-omit-frame-pointer`, `ASAN_OPTIONS=detect_leaks=0`): passed. Leak checking
  was not enabled. Executables are `/tmp/resistance_san` and `/tmp/drive_stage2_san`.
- `make gles`: passed. `make debug`: passed afterward, including final audit
  diagnostics; working `nfsu2` was left as the debug build. Existing unrelated
  compiler warnings remain. Build logs: `/tmp/stage2_gles.log`,
  `/tmp/stage2_debug.log`.
- Native smoke exited 0 with this exact command:

```sh
timeout 60 env DISPLAY=:0 SDL_AUDIODRIVER=dummy ./nfsu2 \
  '/home/dq/Games/Need for Speed Underground 2' \
  --car 240SX --drivetrain --hud --track STREAML4RA --resolution 640x360 \
  --drive-audit /tmp/openug2_stage2 --frames 480 \
  >/tmp/openug2_stage2.log 2>&1
```

The 480-tick run used ordinary app asset loading only, logged the explicit
fixture and both Stage-1/2 states, launched/upshifted to second, reached
69.62 km/h, ended at 47.75 km/h and traveled 62.59 m. Its straight scripted
route exercised 49 wall responses; summary reported `nan=0 oob_z=0 oob_xy=0`.
`/tmp/openug2_stage2_spawn.png` and `/tmp/openug2_stage2_end.png` contain the
captures; the end capture was visually inspected. Car/world and HUD are visible.
The existing HUD texture decoder reported unsupported/missing art, so complete
HUD artwork is not verified. Dummy audio verifies the running audio-input path,
not audible output. Interactive pause/menu/car-switch UI was not automated;
synthetic reset/replacement/pause/clock and field-validation boundaries pass.
No original-game runtime equivalence is established.

Both approved spec hashes and the reviewed Stage-2 vector hash
`fdb4caac8fd4590833e164ff1cd4aa83ea72fa0489674476fe231b55de82fc0e`
were rechecked unchanged. No spec/vector source files, questions or original
assets were modified, and no staging or commits were performed by this role.
