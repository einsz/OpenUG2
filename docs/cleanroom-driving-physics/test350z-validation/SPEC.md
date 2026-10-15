# Temporary 350Z consumer-domain validation amendment

2026-10-15. New-host validation only. This amendment extends the approved
`../test350z/SPEC.md` and `../test350z-effective/SPEC.md`; their selection,
composition, scope and unknowns remain in force. It clarifies effective E1 using
Stage4 steering geometry and Stage5 S5-0/S5-2. It claims no original runtime
malformed-input behavior. Its motivation is a verifier's synthetic host test;
its normative requirements derive from those approved contracts, not from
implementation code or original assets.

## V1 Semantic coordinates and ordinary static shares

Validate the final composed configuration before making it active. Interpret
centre and mount positions as body-local coordinates in metres, x forward.
If a host stores different units or axes, perform its explicit coordinate
conversion before applying these predicates. Apply no extra magnitude bound,
absolute-value operation or positivity requirement to a position coordinate.
A negative centre, height or mount coordinate alone is valid. A signed travel
limit is similarly permitted when its inherited range constraints hold.

Let F be final wheel-0 mount x, R final wheel-3 mount x, and C final centre x.
The enabled-centre ordinary path requires finite F,R,C, positive wheelbase
L=F-R, and R<=C<=F. The Stage5 share ratio before storage is
q=.5*(F-C)/L. Require finite q in [0,.5]; its stored rear per-wheel share is
sr=b32(q), with front per-wheel share sf=.5-sr. Both stored shares must be
finite and nonnegative. They are per-wheel shares: sf+sr=.5. Equal centre and
rear mount permits (sf,sr)=(0,.5); equal centre and front mount permits
(.5,0). Zero shares, including negative floating-point zero, are nonnegative.
Do not require both shares to be strictly positive. Do not replace these shares
with an axle sum, a mass-dependent share, or a guessed percentage range.

The closed support-interval check is applied to the final finite semantic
coordinates before rounding the share. It prevents a slightly outside centre
from passing because division/storage rounds its share to an endpoint. Use
strict ordered comparisons, with no epsilon expansion of the interval. Numeric
output tolerances in the parent vectors do not turn a negative quantity into a
valid nonnegative one, or zero wheelbase into a valid positive one. The approved
composition rounding is unchanged; the predicates inspect its final values.

This is the enabled-centre ordinary configuration selected for this experiment.
The Stage5 externally disabled-centre switch, alternate centre producers and
other families retain their supplied-input boundaries; no new switch behavior
or original default is inferred here.

## V2 Full validation and transactional failure

V1 is one consumer check, not an exhaustive substitute for effective E1.
Continue enforcing every inherited consumed-parameter domain, including finite
consumed values and finite derived caches, positive mass/dimensions/inertias/
radii/final drives, positive declared forward ratios, valid integer gear count,
nondegenerate engine/scoring domains, valid travel ranges and documented
coefficient/share constraints. Validate the final values after all composition
and cache derivation; a valid base record does not guarantee a valid result.
Reject a nonfinite required input or nonfinite consumed/derived result, even
when it arose by overflow from finite operands. Do not reinterpret opaque
fields as floats. Used-field and sentinel rules remain those of the parent.

On any validation failure, activation reports a consumer-domain failure and
leaves the previous active profile identity, configuration, derived caches and
driving outputs unchanged. This applies whether the previous profile is stock
or provisional, and whether failure occurs during parsing, composition or
final cache validation. Failure diagnostics may change. No partially composed
record becomes visible as an active profile; a rejected attempt does not emit a
successful activation trace or a ready/active indication. Exact error prose is
not prescribed. Successful switching still follows the parent's reset-instance
contract. Do not clamp an invalid centre/share, substitute stock, or modify
upgrade weights to make a failed candidate pass.

No new interval for upgrade weights, target scalars, torque samples or signed
positions is introduced. In particular the parent's conditional weight
clamping remains intact; a weight outside [0,1] can remain valid when its mean
and final consumers meet that contract. Existing signed coefficient allowances
remain intact. Never reject an otherwise valid profile merely for differing
from one inspected car's magnitudes.

## V3 Synthetic acceptance and provenance

`test-vectors/validation.json` is an approved component-boundary test set.
Each case declares reviewer-authored synthetic mathematical derivation from
V1/V2 and the approved parent formulas; no original source, asset, observed
runtime output or recovered identifier is included. Inputs enter after final
composition except the explicitly named category-0 centre-addition cases.
Unspecified consumer fields are assumed valid; these are not complete car rows.
Nonfinite labels are test-harness tokens for numeric nonfinite values, not
strings accepted in an asset. A rejected case must exercise V2 against a prior
valid stock state and again against a prior valid provisional state. Compare
all active configuration/caches and fixed-input outputs with their snapshots;
error diagnostics are excluded from that equality. Accepted cases must remain
eligible when all inherited consumer checks also pass.

Acceptance/rejection, endpoints, sign predicates and snapshot preservation are
exact. Expected finite shares permit absolute error 2e-7 for arithmetic display
comparison only. That tolerance never applies to domain decisions. The supplied
near-boundary coordinates are distinct binary32 values on opposite sides of an
endpoint; do not collapse them during fixture construction.

## Unknowns and limits

Original invalid-input handling, complete opening-instance identity, original
runtime trajectories, extended-precision equivalence and all parent unknowns
remain unresolved. This amendment does not supply new upgrade physics or
retail runtime evidence. Only this approved SPEC.md and auxiliary files hashed
in REVIEW.md cross the implementation gate; the verifier report remains private.
