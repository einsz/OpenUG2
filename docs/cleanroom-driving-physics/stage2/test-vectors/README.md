# Stage-2 synthetic vectors

`s2_cases.json` contains 52 synthetic cases derived from SPEC.md. They are not
captured retail outputs and do not establish differential equivalence. Parameters
and starting states were chosen for this exercise; no retail car rows or assets
are included. The private analyst generator is recorded in the stage provenance.
Each case carries its own derivation provenance and section reference.
The JSON contains the complete chosen inputs and expected outputs so reproduction
does not require the generator or the original binary.

Every decimal coefficient prescribed as binary32 is first rounded to binary32.
Some synthetic input literals (such as a chosen momentum product or duration)
are written before a binary32 store. Hosts must store persistent inputs as
binary32 as specified; the stated output tolerance covers that input rounding.
The reference then evaluates formulas in binary64 and rounds scalar/vector output
components to binary32. This convention is reproducible and is not a claim about
retail x87 intermediates. Except for explicitly Boolean/integer outputs, accept
absolute error `2e-6 + 2e-6*abs(expected)`. Predicates are exact for these fixtures;
none relies on a near-roundoff threshold comparison. Output fields called
`threshold_mph` and `measured_mph` are diagnostic rounded values; comparison uses
the formula before those diagnostic stores.

Case groups:

- `drag`: world drag force only, including direction, zero speed, throttle,
  speed scale, reduction, enable and surface factors.
- `vertical_aero`: level identity basis with forward/reverse travel, contact
  fraction, disabled force, zero speed, speed scale and alternate orientation mode.
- `rolling_torque`: one wheel's unsmoothed resistance torque, plus its derivative
  contribution divided by the supplied inertia. There is no driven/contact force
  or gripping slip correction in that output. The airborne case intentionally
  matches the otherwise equal contacting case.
- `limiter_local_force`: supplied **horizontal speed magnitude**, forward-axis
  vertical component and total **body-local force**. It tests the limiter only;
  retained lateral/upward force components are independent supplied values.
  `travel_forward_sign` documents the trajectory direction without changing the
  magnitude predicate. These are not coupled orientation/contact trajectories.
- `stabilization`: supplied momentum/force, z and angular-speed magnitude;
  outputs the derivative and post-Euler nonnegative z. This group does not
  evaluate aerodynamics or wheel coupling.
- `absolute_cap`: momentum and mass at body post-step; outputs capped momentum
  and recomputed velocity. The two masses around 0.7 are adjacent binary32 values
  straddling the binary64 gate. Full-vector magnitude, reverse and equality are covered.
- `prepared_force_euler`: one straight substep with an already prepared wheel
  force, drag and z=0. It checks drag enters momentum once, as required by the
  schedule. It does not pretend to run the five-pass wheel solver.

The force-application caps, full sloped aerodynamic projection, curve-knot
rounding, frame-input lifecycle and real car terminal trajectories have no
original runtime vectors. Their static contract is specified, with remaining
boundaries in U2-1…U2-8. A later verifier must not label success on this file as
retail runtime verification.
