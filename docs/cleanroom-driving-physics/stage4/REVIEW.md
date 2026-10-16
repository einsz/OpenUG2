# Stage4 clean-room review

## Scope and outcome

Approved for the bounded ordinary steering and level-planar tyre contribution.
Role: reviewer, fresh session. Reviewed all Stage4 shared artifacts and the
three named approved companion specifications. Consulted private static findings,
numeric manifest facts and validation record for traceability only. No implementation,
implementation/design/audit tree, verification reports, or original executable
execution was accessed or performed. Prior companion hashes remain unchanged.

## Contamination gate

Read the entire shared specification, questions, provenance and vector README,
and every vector's inputs, expected outputs and provenance. Pattern scan found
only benign dimension names, descriptive vector vocabulary and floating-point
precision terminology. No executable addresses, recovered symbols, source paths,
disassembly, decompiler fragments, recovered identifiers, verbatim original prose,
assets or original control-flow design were identified. No implementer exposure
or contamination incident was identified in the material reviewed.

The heading quantizer is a mathematical input/output characterization with 22
sparse numeric corrections. It contains no original table or code. Those corrections
are necessary for the stated exact integer headings; two vectors exercise positive
and negative corrections. Private static validation covers all 256 indices.
The retained input and load curves are small calibration facts needed to determine
behavior, independently checked against static numeric evidence; no copied asset
or expressive table dump is included.

## Changes by category and location

- Composition/testability, S4-0: stated the once-only substitution for S1-7.4's
  lateral boundary and refinement of S1-7.6; retained Stage2/3 state/force lifecycle,
  external torques and stabilization; separated provisional pass evaluation from
  persistent accumulation and stated host replacement obligations.
- Sign/domain clarity, S4-0/2/3/5: bounded effective filter inputs; tied positive
  steering to a symmetric straight fixture; qualified mirror symmetry for
  binary32 conversion; replaced conditional-expression sign notation with a
  behavioral sign condition. Explicitly restated the complete motion predicate.
- Numeric characterization, S4-0/4: specified binary32 nearest rounding and
  mathematical pi/atan for the compact quantizer; explained sparse correction
  necessity and static provenance without retaining a wholesale original table.
- Basis/normal-force clarity, S4-6: specified body-basis force/arm inputs and world
  conversion, with existing normal-force application counted once.
- Vector coverage/provenance, test-vectors/: retained all 71 analyst cases and
  clarified the existing lock bypass case; added synthetic `locked_with_motion`
  to exercise the enabled predicate with a new lock. Updated the README count
  and reviewer derivation provenance. No original assets were used.

## Checks and limits

Independently recalculated all 71 original vectors using the shared formulas;
all meet their exact integer/Boolean requirements and declared numeric tolerances.
Independently checked the added lock vector and all seven branch cases. All 72
cases have unique IDs and explicit derivation provenance. Nine scalar calibration
curves, six nonzero lateral curves and sparse angle corrections match private
static numeric facts; the zero lateral curve is supported by the static findings.
The earlier analyst validation record predates reviewer edits and is not a hash
approval of these final shared files.

Unknowns U4-1 through U4-7 are preserved: complete device/history producer and
cadence; exact floating-point/trig and marginal ratio rounding; effective grip
producers; full 3D inertia/contact/suspension/extra yaw and nonordinary modes;
and absent retail runtime/differential or perceptual validation. These limit
coverage and do not prevent implementation at the explicit supplied boundaries.
Synthetic checks validate internal consistency, not retail execution parity.

## Approval

UTC: 2026-10-15T15:31:32Z
Role: reviewer
Outcome: approved
SPEC.md SHA-256: `02144d187323c6d74a6fb21e6e0b3e2842097cd0e28c935eea97907c1cbc911a`
Reviewed components.json SHA-256: `e7b9383ca0cf193bec93628241c5e28a511fc4216792ae26db6fe8e90b229c51`
Any later SPEC.md edit invalidates this approval. Vector amendments require
review before they are used as an extension of this approved contract.
