# Clean-room review gate

Date: 2026-10-15 21:26:55 UTC
Role: reviewer
Decision: approved for the bounded supplied-input component contract only.

## Contamination scan

Read the complete SPEC.md, QUESTIONS.md, PROVENANCE.md, vector README and all 56 vectors. Address/symbol/source-path/register scan and manual content review found no original code, recovered original identifiers, disassembly, decompiler fragments, long original strings or copied assets/tables. Scan matches were fixture names containing the word zero and the stated floating-point precision unknown. No contamination requiring removal was found; no evidence of implementer exposure was supplied. Analyst evidence remained private.

## Changes by category and location

- Scope clarity, SPEC.md introduction and HR-3; QUESTIONS.md: corrected the opening HB-Q2 implication, explicitly limited host activation to supplied inputs, and excluded the separate stuck/reset-request timer activation, expiry, clock wrapping and downstream callback effects. This is not a claim fixing HB-Q2/Q3 or establishing a recovery time.
- Domain/testability, HR-2/3: aligned c with the companion's nonnegative selector domain, declared four present wheels, and made restored gravity explicit for covered state codes. No inferred contact-count or ordinary race selector mapping was added.
- Vector coverage/provenance, release.json and README: added 18 synthetic cases for lift input/state distinctions, pending reset, exact tilt/angular thresholds, independent c=0 inputs, stabilization contact/state gates, publication order, repeated per-completion slowing and equality-triggered z decay. Renamed the c=0 retained-spin fixture to avoid implying grounding identity. Every vector states its synthetic/static-inference provenance; none is an original execution capture or asset.
- Review provenance, PROVENANCE.md: recorded the fresh reviewer and preserved role separation. Companion specifications were read only and not edited.

## Behavioral and validation gate

HR-1 preserves the evaluator call predicate, cached slip, incoming versus updated lock timing, separate gripping correction and lock retention. HR-2 preserves strict thresholds and separate c/d/lift inputs. HR-3 places classification after ordinary post-step and the nonrecursive zero-duration sweep before controller force preparation; no extra Euler integration is authorized. HR-4 preserves pre-integration z damping, unconditional z derivative, negative-only clamp, construction versus existing-body import, and the already approved frame gate. Finishing/clutch coefficients remain companion behavior.

56 unique vector records have provenance. Independently checked 47 arithmetic/predicate vectors, including all classification, lock evaluation, gate, isolated damping, history checkpoints, repeated slowing and equality-decay calculations. The remaining 9 rest-write, lock-derivative, initialization, ordering and eligibility assertions were reviewed against requirements and traceability. No implementation tests or original host execution were performed.

## Remaining unknowns

Ordinary c/d/race/state producers and host activation; full timer/reset dispatch and clearing; lock writers beyond the bounded paths; exact-rest trajectory persistence; runtime 240SX/350Z acceleration/recovery histories; exact extended intermediates and excluded degenerate inputs. These unknowns do not license perpetual unlock, z reset, arbitrary launch assistance, or undocumented selector defaults. Any later SPEC.md edit invalidates this approval.

## Approved hashes

- SPEC.md SHA-256: `48c2c9c15e62dc9458eeee3ac753500e73ae34a88a0169ab78075e99994d52be`
- test-vectors/release.json SHA-256: `3af7ea4f725c333c621dbb932ced4612a148ac5c606e8af8725667d85cfa6ac5`
- test-vectors/README.md SHA-256: `f3eb868d7ef3849d8acbe354c2684e6459dd86a87ec45d3f0cd6cc8b847f9954`
- QUESTIONS.md SHA-256: `46d03c738f63cd63649caef26e119280154d5871fd56c2a238903a6c01f403cd`
- PROVENANCE.md SHA-256: `e48aa1a95e9b951d084352556997b67dfa8b1dc0c7e8e0a9e5e28a480b6c8360`
