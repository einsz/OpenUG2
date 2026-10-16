# Consumer-domain validation review

UTC date: 2026-10-15T20:18:24Z
Role: reviewer (fresh test350z_validation_review context).
Outcome: **APPROVED** as a bounded new-host validation amendment.

## Scope and contamination scan

Read the entire new SPEC.md and every synthetic vector. Ran the recursive
contamination-pattern scan; it produced no matches. No original executable,
original source, implementation code, decompilation, implementation design or
private executable was read or executed. Reviewed the coordinator-authorized
verifier's synthetic proposal and approved parent behavioral contracts only.
The proposal's implementation-specific observation was not copied into public
inputs. No evidence indicates contamination reached an implementer in this
review. Original runtime behavior is expressly unclaimed.

## Changes by category and location

- Domain completeness, SPEC V1: made the ordinary static-share support interval
  explicit from effective E1 and Stage5 S5-2. Stage4's positive longitudinal
  wheelbase supports rejecting equal/reversed front/rear positions. The support
  check precedes share storage to preserve mathematical nonnegativity at a
  representable boundary. Both endpoint zero-load cases are allowed.
- Semantic units, SPEC V1: predicates apply to metre-valued body coordinates,
  with explicit conversion for alternate host storage. Signed positions and
  coordinate translations are permitted; raw magnitude thresholds are absent.
- Strict predicates, SPEC V1/V3: arithmetic output tolerances cannot excuse
  outside support, negative shares, nonfinite values or zero wheelbase.
- Transaction completeness, SPEC V2/V3: validate final composition and derived
  caches before publishing; preserve both stock and provisional prior states
  and fixed-input outputs on all rejected activation paths.
- Scope preservation, SPEC V2: inherited domains still apply; no new arbitrary
  upgrade/torque/weight bounds, positivity rule for coordinates, coefficient
  restriction or clamp is introduced.
- Vector provenance, test-vectors/validation.json: 31 individually attributed
  invented component cases cover endpoints, adjacent binary32 values, signed
  coordinates, zero/reversed wheelbase, all three nonfinite coordinate classes,
  valid-base composition crossing support, invalid final mass and nonfinite
  derived inertia. No original asset or table is included. Expected values are
  synthetic mathematical derivations from the reviewed contracts.

No sensitive text needed removal from the new documents. The private proposal
has been distilled to behavioral facts and mathematical requirements, without
its implementation identifiers, private file paths or diagnostic observations.

## Remaining unknowns

Original invalid-input handling, original opening-instance identity, complete
runtime trajectories, exact extended-precision arithmetic and inherited unknowns
remain unresolved. This approval does not assert retail parity or exhaustive
new-host testing; the amendment requires all inherited consumer domains.

## Approved hashes

`3793a90991565fafed9ef0e1f92d126617415d6024b2aaaf77a983ef61ffe41c  SPEC.md`

Approved auxiliary implementation input:

`35f398a9fedb8790efae4eb97277c7f618b6b8f35fe997d870d34c9a60e0c7a8  test-vectors/validation.json`

Parent SPEC.md identities were verified unchanged:

- test350z: `9008ae923699cc529220e89a979683af0b5940926d76763cab0565969c6cb28f`
- test350z-effective: `2db456b62e6711a6d4c487688f94430e4c614c8dceebd6bce62ea0b204f8513c`

Any later edit to SPEC.md invalidates this approval. Auxiliary changes also
require review before use. Only the approved SPEC.md and explicitly hashed
auxiliary file are new implementation inputs.
