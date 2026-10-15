# Stage-2 specification review

- Decision: approved for independent implementation within the stated supplied-input boundaries.
- UTC date: 2026-10-15 14:20:23 UTC
- Role: reviewer (fresh `stage2_resistance_review` context).
- Approved `SPEC.md` SHA-256: `d3072359aa25bf0ef541504805f70b3579829bb9b1310cda8ba1a568286c6cb6`.
- Reviewed vector SHA-256: `fdb4caac8fd4590833e164ff1cd4aa83ea72fa0489674476fe231b55de82fc0e`.
- Any later change to SPEC.md invalidates this approval. Changes to other shared
  requirements or vectors require review before use.

## Gate and change categories

- Contamination scan: read all shared Stage-2 specification, questions, provenance,
  vector notes and all 52 cases. No executable addresses, recovered original
  identifiers, code fragments, original strings or assets were found. Scan matches
  were architecture terminology, mathematical component notation and an independently
  named synthetic case. Car-row offsets describe the input data format, not code.
  The eight scalar curve samples are required behavioral constants; no equivalent
  formula is established, and no wholesale asset/table dump is included.
- Traceability / unsupported default: SPEC.md S2-1/S2-7/U2-1 no longer asserts a
  reduction-state creation default. The reviewed private capture labelled as its
  constructor did not establish that claim. The value is now an explicit supplied
  input and initialization remains unknown. No removed original material is copied
  into this log. Other section claims map to private findings and fingerprinted
  evidence; these are static inferences, not original runtime observations.
- Behavioral clarity: S2-3 distinguishes world-coordinate slope effects from the
  zero direct projection of body-up force onto the orthogonal body-forward axis.
- Integration scope: SPEC.md S2-7 now states host unit/state/input obligations,
  the single prepared-force contribution, Stage-1 wheel-resistance retention and
  unresolved upstream clock explicitly. This describes required behavior and
  scheduling, not original architecture or module decomposition.
- Vector provenance: each case in test-vectors/s2_cases.json names its specification
  derivation and invented input origin. README explains input-storage rounding.
  These examples are derived from statically inferred behavior, not observations
  or public-source captures; they make no independent runtime-equivalence claim.
- Status consistency: QUESTIONS.md and PROVENANCE.md distinguish analyst hand-back
  from this review and retain all open/deferred boundaries.

## Validation and retained limits

All 52 cases were independently recalculated from the mathematical requirements
with a reviewer-only temporary script; every expected scalar/vector matched the
README absolute/relative tolerance and every Boolean predicate matched exactly.
Case identifiers are unique; numeric inputs/outputs are finite. The approved
Stage-1 dependency hash was independently rechecked and is unchanged.

U2-1 through U2-8 remain explicit. S2-Q6 and S2-Q8 remain open; S2-Q7 remains
deferred. Force/moment caps, sloped projection, curve-knot rounding, complete
input lifecycles and terminal trajectories lack original runtime vectors. The
existing fixtures validate local arithmetic and prepared-force accounting; they
do not validate the full coupled solver, host adapter, contact/load feedback or
retail timing. Exact intermediate FPU rounding remains outside this contract.

No implementation code or implementation reports were read or edited. No original
executable was run. No contamination incident or evidence of prior implementer
exposure was identified; analyst hand-back states no implementer had this stage.

Approval: 2026-10-15 14:20:23 UTC; role `reviewer`; `sha256sum SPEC.md` = `d3072359aa25bf0ef541504805f70b3579829bb9b1310cda8ba1a568286c6cb6`.
