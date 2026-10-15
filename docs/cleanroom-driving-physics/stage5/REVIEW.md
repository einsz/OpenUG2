# Stage5 clean-room review

Date: 2026-10-15 16:02:57 UTC. Role: reviewer. Outcome: **approved** for the bounded
static-inferred component/composition contract. No retail runtime parity is
asserted. Approval applies to the exact SPEC.md digest below; any later edit
to that file invalidates approval.

## Scope and contamination gate

Read SPEC.md, QUESTIONS.md, PROVENANCE.md, test-vectors/README.md and both JSON
files in full. Checked E5-1 through E5-8 against the private evidence register,
selected captured instruction ranges and direct numeric-data provenance.
Implementation trees, design documents and implementation verification outputs
were not read. No executable was launched. No implementation exposure or
contamination incident was found.

The automatic scan plus manual review found no executable locations, recovered
original identifiers, original source paths, code fragments, original comments,
long required strings or expressive assets in shared artifacts. Scan matches
are fresh descriptive fixture identifiers, dimensional symbols and the stated
precision unknown. S5-1 offsets describe an external data format; they are
permitted interoperability facts, not executable locations. Mathematical
expressions specify outputs; ordered callbacks/pair reads and prepared-force
latency are observable scheduling requirements, not implementation design.

## Change categories and locations

- **Numeric geometry correction:** test-vectors/components.json, the three tilted
  prescribed_four_contact cases. Recomputed body-up distance with the full
  projection divisor and centre-height/radius term, then sequential pair loads
  and alignment. The level control remains equal. All four cases now provide
  explicit body_axes; README records sign convention, zero steering/ride/bump
  and damping/progression choices, and explains that pedals are labels.
- **Strict boundary correction:** components.json, grip_threshold_bonus, plus
  new grip_threshold_below; SPEC S5-6 and README. The exact angle-constant
  product is not a binary32 input. Replaced the nonrepresentable input with
  the adjacent representable value above and added the adjacent value below.
  The strict comparison uses the stated product without an extra rounding.
- **Units and lifecycle clarity:** SPEC S5-0 gives coefficient dimensions;
  S5-8 distinguishes zero initial travel/rate from geometric changes during
  zero-duration creation preparation. No new retail producer was inferred.
- **Provenance and minimal-data justification:** each component case carries
  synthetic provenance; README and PROVENANCE describe reviewer calculations.
  README states per-row surface provenance and why all 29 IDs/six channels are
  needed. No contamination removal was required.

## Testability, consistency and provenance

Independent arithmetic recomputation covers every one of the original 76 cases
and the added angle-boundary case (77 total), including another complete pass
with every decimal input first rounded to binary32. All final scalar/vector
results meet absolute 0.00002 plus relative 0.00002 tolerance; discrete flags
and IDs match. Branch checks include signed reverse service distribution,
strict speed/throttle/handbrake/slip thresholds, contact endpoints, normal-dot
threshold, retained rates, ordered stale/fresh pair sampling, the extra zero
sweep, tilt geometry, force cross products, preload aero latency, negative
bump cells and distinct surface channels. Scheduling counts are requirements
checked against the documented lifecycle, not measured execution counts.

All 29 surface rows exactly match private direct numeric extraction. The six
channels affect independent specified outputs. No evidenced formula replaces
that ID mapping; repeated entries remain facts rather than an invented pattern.
The tiny required bump sample sequence remains numeric compatibility data,
while its index and interpolation are given by a formula. No names, assets,
presentation fields or complete original records are exported. Vectors are
synthetic derivations of static findings, not runtime observations or public
source captures; this limitation is explicit per case and in README.

Composition is once-only: preload/geometric spring replaces supplied load only
when selected; aligned N supplies all tyre/lock/cap consumers; surface channels
remain distinct; handbrake distribution replaces its placeholder; final body
force and moment, gravity and aero are each accumulated at the stated site.
S5-0 explicitly supersedes only Stage3 composed distribution speed/conversion
and signed reverse B semantics; direct Stage3 component inputs remain supplied.
Prior approved companion files were not edited and their hashes still match.

## Remaining unknowns and acceptance limits

U5-1 through U5-7 remain: original-runtime/differential parity; complete world
query/material mapping/collision; base/drafting/event/other multiplier writers;
upgrades/input mapping; exact intermediate/trig/extreme-cell precision; full
angular/momentum/mode/rest/lift/cap/switch producers; upstream frame cadence.
Explicit supplied state/trajectory, valid surface/normal and impulse-cap inputs
make the bounded component contract testable without guessing these systems.
No further analyst follow-up is required for this approval. Full handling/world
or dynamic pitch/roll parity requires a separately scoped analysis and review.

## Approval

UTC date: 2026-10-15 16:02:57 UTC

Role: reviewer

sha256sum SPEC.md:

`131e2c9e26091c647fb96bdb9ac0891e43c309487abe1e984b4408b711eec16e  SPEC.md`

Reviewed companion-artifact SHA-256 values (paths relative to Stage5):

- `1e86f6198ff82d1b1f1ba264e9c5f4728503ff170e0b330fd59b47e6f16efcd9` — QUESTIONS.md
- `da1c12d6a003e33c3305a3bc3a470a425e17cfcb1ab9f28bd300193e56d1cc3d` — PROVENANCE.md
- `f29c04dce747c0f71fa53c0f291c05994489ac4af206ba9c7d8d5b359ab771ae` — test-vectors/README.md
- `4541fa4b4782a76b8efb6d11e8b1820d4d95ca6e4345a484e4e71262cdd5203e` — test-vectors/components.json
- `eaff75a3b963042083034016561809f5d719ed6f655d10ff1660bfc2be2b446c` — test-vectors/surfaces.json
