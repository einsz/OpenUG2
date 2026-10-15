# Stage3 derived brake fixtures

`braking.json` contains 54 synthetic input/output cases calculated from S3-2–S3-6
and the inherited Stage1 wheel derivative. They were generated on 2026-10-15
by the analyst from fresh mathematical formulas. No target execution, runtime
capture, copied original code, car table or original asset is embedded here.
The analyst-only generator and static evidence manifest are retained separately.
Each case carries its own provenance and behavioral/evidence references. The
source is a synthetic calculation from statically inferred requirements, not
runtime observation or a public-source capture. The reviewer independently
recomputed all 54 expected records under the stated numeric convention.

The component discriminator states the entry boundary:

- `pedals`: enter before exclusivity, exit before engine throttle shaping.
- `distribution`: u/b have already passed exclusivity. vmph is the frame's
  already sampled unsigned mph scalar; this isolates exact 10 mph equality
  from rounding of a constructed world velocity. Strength Kb is in kN·m.
- `preparation`: enter after initial drive distribution with tau_initial.
  B/H are the already calculated positive torque magnitudes in kN·m; they are
  supplied directly to isolate the wheel rule. `relief_disabled` and lock
  are explicit inputs. R, omega, velocities, load and mu are supplied, not
  calculated from body speed or contact count. mu is the complete pre-decision
  sliding coefficient. All these fixtures use longitudinal wheel multiplier
  1, no slip-angle-threshold exceedance (a supplied threshold 12 radians),
  body-speed snapshot 20 m/s (so no low-speed force normalization), no nitrous,
  no further grip modifier, and no later five-pass iteration or final cap.
  initial_longitudinal_force is precisely the preparation output, not final
  contact force or a guaranteed body deceleration. Q/A/C are diagnostic values;
  null means the lock decision was bypassed. Lock and sliding outputs are exact.
  The locked-release and stationary cases specifically check retained state.
- `lock_comparison`: enter at the comparison table of S3-4 with A/C already
  supplied. These mathematical branch fixtures isolate equality at 1 and C;
  they do not claim that every A/C pair is exactly constructible through a
  binary32 car row and pedal product.
- `direction_post`: enter after Euler integration, before direction-crossing
  correction. No torque, pedal, contact, gear or lock input gates this rule.
- `wheel_derivative`: enter with the saved torque/force/smoothed state from a
  prior coupling. Test state update and acceleration before Euler integration;
  this is not a request to redo brake preparation during the derivative.
  The locked/gripping case exercises the later slip correction separately.

Scalar input numbers denote their binary32 values. The reference calculation
uses ordinary double arithmetic between documented binary32 state stores;
intermediate original x87 precision is not asserted. Compare numeric results
with `abs(actual-expected) <= max(0.00001, 0.00001*abs(expected))`; for
angular_acceleration use `max(0.0001,0.00001*abs(expected))`. Booleans,
enums, null diagnostics and branch selections must match exactly. A/C equality
and strict slip/speed/pedal thresholds are deliberate; tolerance is for numeric
outputs, never for changing a branch predicate. Use b32 constants in the spec.

Coverage includes zero, partial and full pedal; axle-share endpoints; simultaneous
pedal exception and equality boundaries; forward/reverse/zero spin; engine/drive
and brake combined torque; signed slip-relief boundary and disable flag; injected
handbrake suppression; new lock and zero-spin-without-lock; release retention;
zero load and high spin; mixed longitudinal/lateral velocity; stale pre-snap slip;
direction thresholds; smoothed torque and the locked/gripping derivative case.
These are bounded component fixtures; no end-to-end stopping-distance or retail
frame-cadence assertion is made. The full prepared-force lifecycle is normative
in S3-7 and the approved companion stages.
