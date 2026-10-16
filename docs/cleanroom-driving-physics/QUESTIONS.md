# Implementer questions: driving physics Stage 1

Raised by the implementer while building `src/drivetrain.c` against SPEC.md
(sha256 620bfb86...e73311). Each item says what was done in the meantime. Nothing
here was resolved by looking at anything outside the spec and its test vectors.

## Historical questions: vector/text ambiguity at the earlier approved hash

* **Q-1 (S1-7.2 step 3, bias rule).** Read literally, the two statements "if `f < p` then `f = 1 - 0.175`;
  if `f > 1 - p` then `f = 0.175`" run one after the other, so the first result (0.825) would immediately
  trip the second test when `p = 0.35` (0.825 > 0.65) and become 0.175. The vector row `20,10,1,0` expects
  `share_b = 0.825`, so the second test must be an else-branch. Implemented as `else if`. Spec should say so.
* **Q-2 (S1-4.2, `D[j+1]` arithmetic).** `U[j]*r[j+1]/r[j]` is ambiguous about grouping. In binary32 the
  vectors (`landing_table_D`, exact binary32) match `U[j] * (r[j+1] / r[j])` and not `(U[j]*r[j+1]) / r[j]`
  (they differ in the last bit for 240SX, SKYLINE, SYNTH_RWD). Implemented as the former, in float.
  Spec should state the grouping.

## Historical questions: interim behavior at the earlier approved hash

* **Q-3 (S1-7.4, "and the wheel moves").** What defines "the wheel moves"? Implemented as the wheel's direction state
  (S1-8.1) not being "stopped" (`|omega| > 1e-5`). Alternatives: body speed nonzero, or `vx != 0`. Affects the
  first substeps of a standing start and wheels being dragged while locked.
* **Q-4 (S1-7.5, order of the lock correction).** "In each pass, ... Then, only if M = 3, apply the lock
  correction." Implemented as: inside every one of the 5 passes, after the force update. If it was meant once after
  all passes, the result differs. Also not stated: whether `alpha_w` uses `tau_i - R_i F_i` with the forces updated by
  this pass (implemented: yes).
* **Q-5 (S1-4.5, first-gear down-shift target).** For `g = 2` (first gear) with `t < 0.2` and `rpm <= eps0 + 100`
  the rule gives `g* = g - 1` = neutral. With the automatic rule "if `g <= N`, `g* = 2`" the next frame, this makes a car
  at a standstill with the throttle off hunt between first and neutral (clutch Disengaging/Open/Engaging cycles).
  Implemented literally. Is that retail behaviour or does something else (e.g. the creep logic, a throttle
  threshold) prevent it?
* **Q-6 (S1-4.5, manual-assist).** (a) "when `rpm > eps2 - 300` in a gear above the last one": no gear exists above
  the last; the sub-rule is **not implemented**. (b) When `gs < g*` and `g <= 2` there is no lower forward gear (spec
  would compute `r[g]/r[g-1]` with `r[1] = 0`): implemented as "no change". (c) When `gs > g*` the spec says
  `g* = g + 1`: implemented with an extra guard `g < n + 1`. (d) When the 10 s "without change" timer counts and
  what resets it: implemented as "frame time accumulated while `gs == g*`; reset when `gs` changes or after reporting".
  (e) Spec U-2 already lists the return path as unknown; `report_auto` is exposed as an output flag only.
* **Q-7 (S1-7.1 / S1-7.7 slip-lock, which `omega_w,old`).** For AWD the "wheel-side speed" `omega_w` is the average of
  both axles, so `delta_omega = (omega_e - omega_w,old) / (r[g] F_axle)` is applied to both axles using that combined
  value. Implemented as written (combined value). Also, "neither wheel of that axle touches the ground" is read from the
  `contact` flags. There is no vector for this branch.
* **Q-8 (S1-7.7 first-gear rule vs. conditional).** The text "In any clutch mode, if g = 2" is implemented as independent
  of the `g != N, M = 3, nc > 0` condition above it. Because the performance class `C` is unknown (U-11), `kappa_C` defaults
  to 0 (`DrvTune.perf_class = 0`), which makes the whole rule a no-op by default.
* **Q-9 (S1-7.6, which force is stored and fed to the wheel).** After the friction limit the stored `F_i` is the
  returned force (including the 1.5 factor when scaling happened). The wheel torque term `tau_i - R_i F_i` in
  S1-8.1 and the body use that same final value. Not stated explicitly.
* **Q-10 (S1-8.1, `phi` wrap range).** "wrap `phi_i` into range": implemented as `[0, 2*pi)`. No effect on dynamics.
* **Q-11 (S1-7.8, rear yaw term).** The rear handbrake/yaw term "= `1 + |forward-axis vertical component|` when driving
  straight" is taken as an input (`DrvEnv.fwd_axis_vertical`, default 0 so the term is 1). The conditions under which it
  applies other than "straight" (handbrake? speed?) are not stated; applied to rear wheels always.
* **Q-12 (S1-7.3 / S1-7.1, when `omega_w` is sampled).** The clutch post-step computes `omega_w`; the coupling later in the
  same substep re-computes it from the wheel speeds (gear may have changed in an Open to Engaging step). Spec does not say
  whether the coupling reads the post-step value. Only matters for one substep after a gear swap, when `c = 0`.
* **Q-13 (S1-2.4 / S1-3.5, `Phi` and coupling at creation).** The caller must run `drv_couple()` once after
  `drv_init()` (S1-2.5); the first derivative sees `Phi = 0`. Fine for the free-rev vectors; confirm intended.
* **Q-14 (S1-7.4, lateral gripping force).** The lateral curve is Stage 4. The gripping-branch lateral force is an input
  (`DrvWheelEnv.lateral_grip_force`, default 0) multiplied by `wT`.
* **Q-15 (S1-8.2, planar body).** `Iz` and wheel arms are not in Stage 1. `drv_body_accel_planar()` takes `1/Iz`,
  arms and unit vectors from the caller; the moment sign convention (`x*cy - y*cx`) is the standard one implied by the
  `A_i` expression, not stated.
* **Q-16 (S1-2.1 frame quantiser).** `drv_quantize_frame_dt()` rounds to nearest 1/1800 s and applies optional
  clamp bounds that are 0 (off) by default (U-5).

## Historical parameter boundaries at the earlier approved hash

`G` (U-7), bonus flag (U-6), `q` and `X` (U-4), performance class `C` (U-11), jitter stream (U-3, default suppressed =
offsets 0), initial engine speed (U-15, default idle), shift-decision rpm override (U-10, argument of `drv_frame_begin`),
frame-time bounds (U-5): all are explicit fields of `DrvTune` / arguments and documented in `src/drivetrain.h`.


## Reviewed disposition — 2026-10-15

The preceding entries are retained as the question record; their interim choices are not approvals.
Answers amend SPEC.md; the current reviewer hash gate is recorded in REVIEW.md. All new executable findings are static
inferences; none is runtime-validated.

| Questions | Status | Specification answer |
|---|---|---|
| Q-1 | Resolved | S1-7.2: mutually exclusive conditions |
| Q-2 | Partial | S1-4.2: ratio first; binary32 intermediate rounding remains U-14 |
| Q-3 | Resolved | S1-7.4: either contact-frame velocity component nonzero |
| Q-4 | Resolved | S1-7.5: correction every pass, using updated forces |
| Q-5 | Resolved | S1-4.5: second threshold 0 prevents positive-idle down-shift |
| Q-6 | Resolved for valid Stage-1 inputs | S1-4.5: remembered gear and timer clarified; U-2 menu mapping and initial assist state remain |
| Q-7 | Resolved | S1-7.7: one combined snapshot for both axles |
| Q-8 | Resolved | S1-7.7: independent condition; class source remains U-11 |
| Q-9 | Resolved | S1-7.6: final force shared by body and wheel derivative |
| Q-10 | Resolved | S1-8.1: large-range one-step angle adjustment |
| Q-11 | Partial | S1-11: full rear multiplier is input until mode/blend context is resolved |
| Q-12 | Resolved | S1-7.3: cached clutch-post-step wheel-side speed |
| Q-13 | Resolved | S1-2.5: creation post-processing; feedback 0 and mc=0.7 in neutral |
| Q-14 | Resolved as Stage-1 boundary | S1-7.4: already-signed lateral force supplied; curve deferred |
| Q-15 | Resolved as Stage-1 boundary | S1-8.2: positive moment x*cy-y*cx; parameters supplied |
| Q-16 | Partial | U-5/S1-11: residual clock and clamp unresolved end to end |

New derived regression examples: `test-vectors/s1_question_cases.json`; provenance and tolerances
in `test-vectors/README.md`. Runtime differential verification remains outstanding (U-16).
