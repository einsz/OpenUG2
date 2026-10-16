# Stage4 component vectors

`components.json` contains 72 **synthetic derived** input/output examples. None
was obtained by running the retail executable or by a differential comparison.
The analyst calculated 71 expectations; the reviewer added the complementary
`locked_with_motion` case from the same equations. Expectations were calculated
from S4-1…S4-6 using invented inputs;
the static constants were cross-checked against the identified executable.
The analyst-only derivation script and original-byte evidence stay outside
this directory. This directory contains no binary, disassembly, original code
or copied game assets.

Each case names its component entry boundary. It is not an end-to-end vehicle
run. The component names are descriptive test vocabulary, not required module
names. Use the case's supplied load, grip limit, state/history, contact arms,
mass/inertia and timing exactly. Where a case supplies a folded angle directly,
it tests the lateral function or final force correction independently of the
heading producer. A kernel fixture supplies history averages; it does not
validate a guessed history sampler. vctl is signed forward speed in m/s.

For real-valued outputs accept `abs(actual-expected) <= absolute_tolerance +
relative_tolerance*abs(expected)`. Integer commands/headings and Boolean outputs
must match exactly; their numeric tolerances do not apply. Arrays use the same
rule per element. Exact branch fixtures use supplied exactly representable
values where possible. Tolerance never changes a strict comparison in SPEC.md.

The planar contact-velocity cases use ideal sine/cosine and allow 0.003 m/s
absolute difference; force-correction cases likewise use ideal sine with their
stated force tolerance. This accommodates U4-3 rather than asserting original
trig bit patterns. The front geometry cases test angles before that primitive.

Coverage includes:

- Both device mappings, zero/release, mirrored step, high/reverse speed,
  countersteer latch boundaries, history floor, command truncation.
- Front angle mirror/zero/large steer, rear zero angles.
- Heading axes/diagonals, reverse folding, small positive/negative lateral
  velocity, two exceptional quantizer samples.
- Zero/mirrored/reverse gripping forces, load/angle clamp, zero load and wT.
- Combined-force equality/saturation, positive-drive test scaling, locked
  exception, post-cap correction and its abs(vx)=1 boundary, tiny-force bypass.
- Steering/yaw contact velocity, force/moment mirror/cancellation, airborne
  zero prepared tyre forces, fixed-substep tyre-only yaw/velocity increments.
- Slip/angle equality versus strict threshold, lock with and without the
  nonzero-slip predicate, and spinning-at-rest bypass.

Not covered as numeric retail captures: entire input history, suspension/contact
production, 3D attitude dynamics, extra chassis yaw torque, complete rear-grip
production, original-frame timing, stopping radius, or perceived handling.
These remain the explicit SPEC.md unknowns and Stage5 boundaries.
