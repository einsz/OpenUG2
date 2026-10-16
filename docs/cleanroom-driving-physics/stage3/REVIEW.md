# Stage3 clean-room spec review

## Scope and method

Fresh reviewer context; Stage3 shared SPEC.md, QUESTIONS.md, PROVENANCE.md,
test-vectors/README.md and every field of test-vectors/braking.json reviewed.
The approved Stage1/Stage2 behavioral companions were consulted and their
supplied hashes independently matched. Private Stage3 evidence was consulted
only for traceability. No implementation, implementation design or verification
report was read; no original executable or original instruction stream was run.

A broad contamination-pattern scan and full document/vector reading found no
executable addresses, recovered function/type/variable names, original source
paths, decompiler/disassembly fragments, original control-flow transcription,
long original strings, copied assets or wholesale original table dumps in the
shared material. Car-row field offsets describe an input data format and are
allowed. Scan matches in the synthetic zero-case identifier and processor
precision terminology are harmless, not recovered symbols or instructions.
No contamination reaching an implementer was identified; this review does not
audit implementer access history.

## Changes by category and location

- Companion-contract composition — SPEC.md opening, QUESTIONS.md Q3-9:
  specified one shared pedal-exclusivity operation, distinguished the Stage1
  brake-free fixture shorthand from persistent Stage3 lock state, scoped the
  locked derivative before its independent gripping correction, and made the
  Stage2 prepared contact-force lifecycle and once-only host brake contribution
  explicit. Earlier approved companion files were not changed.
- Evidence confidence — SPEC.md S3-6/S3-8: replaced ambiguous observation
  wording with explicit static-inference wording. No runtime claim is approved.
- Test scope — SPEC.md S3-10: component fixtures are distinguished from the
  separately stated scheduling contract; no numeric lifecycle capture or
  stopping-distance validation is claimed.
- Vector provenance — test-vectors/braking.json and README.md: each case now
  identifies synthetic calculation, relevant requirements/evidence IDs, chosen
  fixture inputs and absence of original assets. These are derived consistency
  vectors, not observations or public-source captures.
- Gate/provenance — SPEC.md evidence notice and PROVENANCE.md reviewer entry:
  directed approval to this exact hash and recorded reviewer separation/limits.
- Contamination removal — none required; no prohibited original material found.

## Behavioral and numeric checks

All 54 unique synthetic cases were independently recomputed from the equations:
pedals, distribution, preparation/relief/lock/force, direction, isolated lock
comparisons and wheel derivatives. Every expected record matched exactly under
the README's binary32-input/state-store and double-intermediate convention.
Exact branch/Boolean/enum expectations remain distinct from numeric tolerances.
The comparison-only lock fixtures deliberately isolate equality without claiming
all combinations occur in a retail state. Per-case provenance contains no assets.
The private evidence ledger provides E3-1 through E3-7 for the normative inferred
rules; the derivative and lock rules were additionally checked against their
referenced private extracts. No original material is reproduced here.

Requirements are expressed as mathematical outputs, predicates, state retention
and accumulation boundaries; the implementation architecture remains independent.
Stage3 supplies service brakes and a bounded injected handbrake interface only.
The Stage1 and Stage2 companion hashes remain respectively
`61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb` and
`d3072359aa25bf0ef541504805f70b3579829bb9b1310cda8ba1a568286c6cb6`.

## Remaining unknowns and limitations

U3-1–U3-7 remain: upstream device/reverse mapping; lock initialization/reset;
relief-disable producer; extended intermediate precision; runtime/differential
and stopping-distance validation; upgrades/tuning/mode composition; complete
handbrake/contact/load/lateral/surface producers. Supplied states and fixture
choices do not establish retail defaults. The inherited upstream clock and
companion unknowns remain unresolved. Approval establishes a clean behavioral
handoff and mathematical consistency, not measured retail/perceptual parity.

## Approval

- Outcome: approved for the stated inferred behavior and bounded scope.
- UTC date: 2026-10-15T14:54:51Z.
- Role: reviewer (fresh cleanroom-reviewer context).
- SHA-256 SPEC.md: `d4689a16861df810159760a6a668ef3714966fd6c20a30e3b2b54f355c41c6f7`.
- Any later edit to SPEC.md invalidates this approval and requires a fresh gate.
