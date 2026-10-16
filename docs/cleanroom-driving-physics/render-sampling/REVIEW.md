# Render-sampling clean-room review gate

UTC date: 2026-10-15. Role: reviewer. Decision: **APPROVED** for the exact inputs below and the independent host-presentation domain of SPEC.md.

## Scope and contamination gate

Read the complete original proposal, complete final specification and all vectors. Consulted the baseline body-response/JITTER_VERIFICATION.md and synthetic diagnostic metrics for traceability, and relevant approved companion contracts. No implementation source, original binary, recovered original presentation algorithm, original code or original assets were read or edited. Diagnostic fixture implementation was not used as a design input. No companion approved files were changed.

Pattern scanning and whole-document review found no executable addresses, recovered original identifiers, original source paths, disassembly/decompiler fragments, expressive original strings/assets or wholesale proprietary tables. No contamination removal was necessary and no evidence of exposure to forbidden material was found. The baseline report's independently observed mixed-time channels motivate an independent requirement; they do not establish retail presentation behavior or prove a native pixel symptom fixed.

## Changes by category and location

- Provenance/gate, SPEC preamble and RS-0: created a standalone approved candidate from the proposal while preserving PROPOSAL.md unchanged. Clearly label independent host policy and exclude proposal/diagnostic material from implementation inputs.
- Clock/testability, RS-0/1/4: define adjacent completed fixed-tick exports, residual alpha time, zero/multiple-tick selection, initialization, pause/snap, reset/teleport/car/geometry/actor-generation and dynamic/legacy boundaries. Distinguish geometric origin from centre and completed ticks from interior physics substeps.
- Channel consistency, RS-1/2: require one sampled actor for chassis, orientation, individual hubs/travel/steer/spin, lamps and camera actor target. Preserve independently defined camera response and coherent follow-other-actor selection.
- Wheel identity, RS-0/2: define stable mount/side assignment, outward-face sign, per-wheel physical spin/steering sense after fixed geometry conversion, and no double application of hub travel. No mesh architecture or recovered original transform procedure is prescribed.
- Orientation/math, RS-2/3: explicit unwrapped spin interpolation followed by optional phase reduction; pure small-yaw and label-wrap arithmetic; continuous proper 3D basis and deterministic half-turn branch without prescribing arbitrary 3D intermediate values or physics integration changes.
- Dynamics boundary, RS-5: preserve simulation state bitwise, contacts/queries and preparation/derivative timing; prohibit display-driven physics tuning. Require varying-alpha verification and separate native visual confirmation before a user-visible fix claim.
- Vector provenance/coverage, test-vectors/render-sampling.json: retain and clarify nine proposal cases, attach individual synthetic derivation provenance, specify fixture defaults/units/order, and add six cases for transformed hubs/lamps/target, physical wheel side/sense, tilted/half-turn bases, pause/snap, render-cadence invariance and yaw-label wrap.

## Validation and limitations

JSON parses with 15 unique vector IDs. Independently evaluated scalar wheel interpolation, positive/negative multi-turn spin, phase reduction, linear motion and zero/multiple-tick schedule arithmetic; checked hub/lamp transforms, yaw outputs, stationary/current selection expectations and all supplied 3D endpoint basis orthonormality/handedness. Property cases state explicit invariants rather than inventing a unique 3D interpolation. Each vector is an invented mathematical host fixture, not an observation of the original program or a public original asset; provenance explicitly records this independent origin. No original assets or copied tables are present.

This review approves a testable contract, not implementation correctness. Fractional-alpha varying-cadence host checks, simulation-output preservation and native visual confirmation remain verification obligations. Retail sampling, camera latency, original trajectories, active mesh conversion and other causes of the reported symptom remain unknown as enumerated in SPEC.md. Existing physics/configuration companion unknowns remain unchanged. No new physical law, tuning or retail parity is approved.

## Approved implementation inputs

Only the following files from this directory may cross the implementation wall. REVIEW.md is gate metadata. PROPOSAL.md is preserved proposal provenance and is not an approved implementation input; preserved SHA-256: `acdb52318db637a02aa4258cca1f9a9ca53cbe51b68aef4a1c33a145206d1df9`.

| File | SHA-256 |
| --- | --- |
| SPEC.md | `ae4830e01a38054b37fc4aede4545bab80eb602450ffdfa63eb4f7d8bb259825` |
| test-vectors/render-sampling.json | `976dc0dc16c2b3763acbcae261e58225251c5569ac7bf9d92d84db858dcc841c` |

Any later edit to SPEC.md invalidates approval; auxiliary changes require re-review. Separately approved physical/configuration inputs retain their own gates and are not replaced here.

UTC date: 2026-10-15. Role: reviewer. Approved SHA-256 of SPEC.md: `ae4830e01a38054b37fc4aede4545bab80eb602450ffdfa63eb4f7d8bb259825`.
