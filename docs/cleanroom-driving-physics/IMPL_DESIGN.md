# Implementation design: driving physics Stage 1

Current behavioral input: `SPEC.md` SHA-256
`61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb`, approved by the
latest gate in `REVIEW.md`. This implementation round used only that approved specification,
`QUESTIONS.md`, `REVIEW.md`, `test-vectors/`, the permitted existing drivetrain module/test,
this design document and the Makefile. That standalone round did not access original executable,
data, analysis, evidence or original source; it did not modify `src/physics.c`.
The later gameplay integration and permitted independent baseline are recorded below.

## Layout and scope

`src/drivetrain.c` / `.h` remain a standalone C99/libm module. `tools/drivetrain_test.c`
loads the approved vector files at run time. `make drivetrain-test` builds and runs the
standalone suite; `VECTORS=directory` selects another approved vector directory.
Gameplay integration is a separate adapter described below.

The engine, clutch, gearbox and wheel states are owned by this module. Normal loads,
contact velocities/flags, signed gripping lateral forces, complete rear-grip multiplier,
lock flags and planar body inputs are supplied by later stages. They are inputs, not
claims about retail extraction or initialization (S1-7.4, S1-8.2, S1-9, S1-11).

## Scheduling and creation (S1-2, S1-3.5, S1-4.4)

- `drv_frame_begin()` shapes pedals and runs the selector once per supplied frame duration.
- `drv_integrate()` collects derivatives, applies explicit Euler, then wheel, engine and
  clutch post-steps. The caller integrates its own body using the previous coupling's
  output forces and refreshes contacts before `drv_couple()`.
- `drv_couple()` prepares torques/tyres, runs five force/correction passes, limits friction
  and finishes coupling. Forces feed both the caller's body and the next wheel derivative.
- `drv_init()` performs creation post-processing without evaluating derivatives: initial
  zero wheel states already satisfy the wheel post-step; engine and clutch post-steps,
  then neutral coupling, produce `mc=0.7`, feedback zero and the cached wheel-side speed.
  A supplied initial engine rpm also passes through engine clamp/limiter hysteresis.
  Calling coupling again with the caller's environment is supported.

## Arithmetic policy (S1-0.3, S1-4.2, U-14)

Persistent numeric state and parameters are binary32. Most intermediates are double.
`Kr`, `Kw`, the subdivision threshold and angle bounds use the specified binary32 values.
`Tp+Ts` is a binary32 sum. Shift-table landing rpm uses ratio-first grouping; the existing
binary32 quotient convention is retained, but exact quotient rounding is not asserted as
retail behavior. Landing-table tests use the approved relative tolerance of 2e-6.

## Decisions traced to the amended specification

| Sections | Independent implementation decision |
|---|---|
| S1-1, S1-4.2 | Finalization composes torque curves, drive class/launch scale and shift tables. The inactive optimal-shift search is omitted. |
| S1-3 | Curve lookup, shaft torque, Euler derivative and engine post-step remain separate operations. Deferred nitrous/over-rev/presentation state is omitted. |
| S1-4.3 | Optional caller random source supplies jitter; a null source explicitly suppresses jitter for deterministic tests. |
| S1-4.5, S1-11/Q-6 | Assist elapsed time advances on every `-2` invocation, including non-Locked frames. A new request remembers current gear and resets elapsed time. Reporting automatic preserves elapsed time and takes priority over the low-rpm fallback. High-rpm reporting compares against the remembered pre-change gear. No extra top-gear guard is added to valid manual requests. |
| S1-4.5/U-2 | `drv_assist_supply()` requires the caller's initial selected gear, elapsed time and remembered gear. Assist decisions remain inactive until that state is supplied. `drv_assist_select()` accepts only forward selections and resets elapsed time when the previous selection was neutral. API input rejection is local validation, not a claim about invalid retail states. Menu consumption of `report_auto` remains external. |
| S1-5 | Raw pedals undergo exclusivity, dead zone and forward-gear anti-stall floor before selector decisions. |
| S1-7.1/7.2 | A shared axle-combination helper and additive distribution implement contact-weighted reaction and mutually exclusive axle bias. Locked wheels reject drive torque. |
| S1-7.3/Q-12 | Initial slip coupling reads the speed cached before clutch mode logic changes gear. Coupling does not refresh that snapshot. Finishing resamples independently and only slip sync overwrites the cached value. |
| S1-7.4/Q-3/Q-14 | The sliding eligibility predicate tests contact-frame translation (`vx` or `vy` nonzero), independently of wheel spin/direction. The gripping lateral input is already signed and only multiplied by `wT`. |
| S1-7.5/Q-4 | Every pass samples body acceleration once, updates every eligible force, then distributes a lock correction using updated forces and accumulated reaction/drive torque. `drv_lock_correction()` exposes this current-state calculation for isolated tests without mutating state. |
| S1-7.6/Q-9 | Friction scaling retains the 1.5 longitudinal factor only when saturation occurs. The final stored force is shared by body output and wheel derivative. |
| S1-7.7/Q-7/Q-8 | AWD slip sync uses one combined pre-adjustment speed for both axles. First-gear feedback is independent of Locked/contact eligibility. Performance class remains a supplied tuning value. |
| S1-7.8/S1-11/Q-11 | `DrvEnv.rear_grip_multiplier` supplies the complete rear multiplier. The module derives no general rear term from orientation. `drv_env_default()` is documented strictly as a level, straight, no-handbrake synthetic adapter with multiplier 1; actual callers must supply the full multiplier. Player rear speed scaling remains separate. |
| S1-8.1/Q-10 | Wheel angle applies the specified one-step large-range subtraction/addition and conditional zero reset, with strict comparisons; it is not modulo a turn. |
| S1-8.2/Q-15 | Planar body moment uses `x*cy-y*cx`. Mass, inverse yaw inertia, arms and heading/lateral axes are supplied physical inputs. |
| U-5/S1-11/Q-16 | `drv_quantize_frame_dt_test_adapter()` is explicitly a stateless testing adapter, with optional supplied clamp before independent rounding. It carries no residual clock and is not the retail timing model. Subdivision consumes supplied durations. |

## Explicit unknowns and adapters

Existing unknowns remain exposed: engine scale/bonus, grip factor/location factor,
performance class, random stream, initial engine rpm and decision-rpm override. The tuning
convenience initializer uses known initial values where specified and documents test choices
(e.g. idle when no initial rpm is supplied); it does not resolve their retail sources.

Q-2/U-14 (intermediate rounding), Q-6/U-2 (initial assist state and menu consumption),
Q-11 (rear mode/blend context) and Q-16/U-5 (end-to-end residual timing/clamps) remain open
within the existing question record. No new unanswered behavioral question was needed.
U-1 through U-16 and later stages remain the limits of this module.

## Validation

The new question vectors were encoded before behavioral changes. With the existing
calculation extracted as a non-mutating helper, the baseline failed 11 of the new checks;
existing vector families passed. All 18 JSON cases are now exercised, as are all existing
parameter/torque/subdivision/clutch/axle and 2,400 free-rev trajectory rows. Free-rev cases
explicitly override creation `mc` to their prescribed starting value 1; they are not creation
traces. Failure output is capped without skipping remaining trajectory rows.

Additional deterministic checks exercise elapsed timer strictness and retention, non-Locked
assist time, remembered gear, request acceptance/cancellation, signed lateral input, supplied
rear multipliers, angle equality/reset boundaries and high-rpm creation. A five-pass oracle
observes every pass's incoming forces/torques/reaction and independently predicts the updated
forces and distributed correction, including the fifth pass. The inherited assumed-load
launch remains a smoke test only.

Current `make drivetrain-test`: **3,606 checks passed, 0 failures**. The maximum free-rev
speed error is below 0.00012 rad/s, with zero limiter mismatches. These vectors are derived
from the approved formulas; passing them is specification consistency, not runtime fidelity
(U-16).

Standalone strict C99 syntax checks (`-Wall -Wextra -Werror -pedantic`) also pass.
The AddressSanitizer/UndefinedBehaviorSanitizer build runs all 3,606 checks successfully
with leak detection disabled. The initial run with LeakSanitizer enabled could not run
under this environment's ptrace constraints; leak checking is therefore unverified.


## Gameplay integration (approved Stage-1 hash unchanged)

The fresh integration role used the approved spec, review, questions and vectors,
plus the explicitly permitted independent OpenUG2 baseline (`physics`, `main`,
parser/framing, audio/HUD, tests and build files). It did not inspect original
executables/data, analyst material or evidence. Native OpenUG2 smoke runs load
ordinary assets through the existing independent parsers; this is runtime
integration testing, not original-game observation or data extraction.

`src/drive_stage1.c/.h` owns stock-field decoding/validation and the pure player
adapter. `src/main.c` owns the opt-in `--drivetrain` selector, transitions,
reset/swap lifecycle and presentation. A small chassis helper shares the
existing steering/lateral/cap/position code without legacy propulsion or drag;
the default `phys_car_step` executes its previous path. Tests link the adapter
explicitly so existing GL-free physics test targets need no new dependencies.

| Spec boundary | Integration decision |
|---|---|
| S1-1 | Read stock mass, wheel radii/indexes, axle grip, engine, clutch, both final drives, split and ratios by the approved offsets from existing framed car records. Validate finite usable parameters and reject invalid cars; no invented tuning fallback. Primary/secondary composition remains in `drv_car_finalize`. |
| S1-2/U-5/Q-16 | The existing fixed 60 Hz host clock supplies durations; each tick gets four substeps. Input shaping/selection runs once per host tick, all wheel/engine/clutch states integrate, then coupling produces next-step forces. The original residual clock is not claimed. |
| S1-3/S1-4 | Stock engine and automatic selector own actual RPM/gear. Host W/S direction policy requests direct reverse or auto and uses service braking while changing direction. This is a documented adapter for unresolved original menu mapping, not a specification amendment. |
| S1-7/S1-8/S1-9 | Host ride contact gates static quarter-weight loads. A symmetric longitudinal body callback uses actual stock mass and host drag/braking as external force. Host steering/lateral/caps/contact/suspension/collisions remain independent approximations. Body propulsion comes only from Stage-1 forces. |
| S1-7.8/S1-11 | Human-player rear speed factor is enabled. The complete rear multiplier is explicitly 1 for the host's synthetic longitudinal contact slice, not a general original-game formula. No lateral curve or yaw-inertia extraction is invented. |
| S1-9 | Host surface drag acts once across substeps; legacy propulsion and legacy coast drag are bypassed. Service brakes synchronize supported wheel spin to body rolling speed; handbrake locks/holds. Nitrous/upgrades are disabled while active; drag/drift explicitly announce legacy fallback. |
| S1-2.5/S1-4.1 | Spawn/reset/restart/car replacement clear transient engine/clutch/wheel state; wheel speed seeds from host velocity on a mode transition. Pause changes no drivetrain states and discards paused elapsed time. |
| S1-3/S1-4/S1-8.1 | Audio/HUD use actual engine RPM, gear and shift state. Wheel display/diagnostics use integrated spin. Speed remains the collision-resolved host body's speed. |

`tools/drive_stage1_test.c` was initially encoded before adapter implementation
and failed because the adapter interface did not yet exist. It exercises
synthetic gameplay invariants and independent serialized record fields; it is
not an original-game trajectory oracle. The existing approved vector suite is
retained unchanged. Full user-facing adapter boundaries and commands are in
`docs/DRIVETRAIN.md`. No new behavioral question or spec amendment was required.

Integration validation (2026-10-15): `make drivetrain-test` passed all 3,606
checks; `make drive-stage1-test`, `make physics-clock-test`, and
`make district-collision-test` passed. The synthetic gameplay suite also passed
AddressSanitizer/UndefinedBehaviorSanitizer with `ASAN_OPTIONS=detect_leaks=0`.
Both `make gles` and `make debug` built successfully; the working executable
was left as the debug build. Existing project compiler warnings remain.

Native debug smoke used the coordinator-provided local asset root through the
normal OpenUG2 application, with dummy audio for the restricted audio backend.
The final `--drivetrain --hud --car 240SX --track STREAML4RA --resolution 640x360
--drive-audit /tmp/openug2_stage1_final --frames 360` run exited 0. It logged
Stage 1 active, launched, upshifted to second, reached 60.01 km/h and traveled
37.74 m, with no NaN or world-bound failure flags. A prior 480-tick run also
exercised wall contact and slowdown. Captures/logs stayed under `/tmp`.
HUD activation succeeded but the existing asset decoder reported an unresolved
package image/unsupported texture; complete HUD art is not validated. Audible
output was not validated with the dummy backend. Interactive car-switch and
pause menus were not automated; their pure reset/swap/clock boundaries were
covered synthetically. No claim of original-game runtime fidelity is made.
