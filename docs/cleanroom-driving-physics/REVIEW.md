# Driving physics Stage 1 review

## Scan result
Starting-point grep run over SPEC.md and test-vectors. No executable addresses, sub_/FUN_/LAB_/DAT_ names, mangled names, source paths, disassembly, pseudo-C, register sequences or original identifiers found. Remaining hex hits are allowed: binary32 bit patterns of numeric constants (Kr, Kw, 0.0044), the chunk id 0x00034600 and data-record offsets (+0x110, +0x220 ... +0x358 are offsets inside the game-data car record, cross-checked as data-format facts, not executable layout). Full read-through done; symbols (rho_S, zeta, etc.) are the analyst's own names.

## Changes
* SPEC S1-0.3 / U-14: removed reference to the CPU floating-point unit by name; now "higher than binary32 precision".
* SPEC S1-1.1: clarified 0x00034600 is a chunk id, not a file offset.
* SPEC S1-4.2: removed description of a disabled design-time switch in the original (original control-flow layout); kept the behavioural statement (do not implement as default).
* SPEC U-10: removed description of an original global sentinel value; now "unidentified global condition".
* test-vectors/s1_params.json: removed row_file_offset fields (locations in the owner's file; same call as docs/cleanroom-camera).
* test-vectors/s1_axle_split.csv: removed 2 rows for override modes 1/2 (deferred drag aid, not specified in SPEC, so untestable).
* test-vectors/README.md: removed pointer to analyst-only generator script and evidence hash file; added provenance paragraph; updated axle-split note.

## Testability / arithmetic check (recomputed by reviewer)
substeps (10, 60, 227.27, 240 Hz), all axle-split rows, all clutch rows incl. the 0.5 halving case and 300 clamp, SYNTH_RWD torque rows (u=1, 0.5, 0 gear, neutral x0.65, limiter x2) and the first two freerev substeps (omega 106.1086, 106.87; engagement 0.015625 steps) all match the SPEC formulas.
Limits: all vectors are derived from the spec, not captured (U-16), so they check implementation consistency, not retail fidelity. The [obs] tags mean observed in static analysis/data, not at runtime. Several mechanisms (S1-7.5 literal update, S1-7.7 slip-lock branch) have no vectors. Remaining unknowns U-1..U-16 are explicitly marked and acceptable.
SYNTH_RWD vectors (params, torque, clutch, freerev, axle, substeps) are self-sufficient for an implementer.

## Retail-derived vectors: recommendation (no deletion made)
PEUGOT/240SX/SKYLINE rows in s1_params.json and s1_engine_torque_* are small per-car derived facts (about 40 numbers/car), comparable to the camera spec's effective tuning tables: acceptable in my view. The s1_freerev_neutral_{PEUGOT,240SX,SKYLINE}.csv files (about 26 KB each) are computed outputs, not data dumps, but are the bulkiest retail-linked items; if the user wants a conservative stance, delete those three and keep SYNTH_RWD. Final call is the user's under CONTRIBUTING.md.

## Deviations / contamination notice
* Analyst ran the game under Wine once, not in a VM and with network not disabled (logged in PROVENANCE.md). No data from it was used. Not a contamination of the spec.
* Public notes (troikoss/nfsu2-re) were used as address leads only; no addresses reached the spec.
* No contamination has reached an implementer through this spec as reviewed.

## Approval
Date (UTC): 2026-10-15
Role: reviewer
SPEC.md sha256: 620bfb86586a98a62714be10c20fb4ca3f300bcbf2b6623b45700065e8e73311
(Any later edit to SPEC.md invalidates this approval.)


## Fresh question-amendment gate — 2026-10-15T13:35:25.760343+00:00

Role: reviewer (fresh cleanroom-reviewer context, driving_questions_review).
Scope: SPEC.md, QUESTIONS.md, test-vectors, REVIEW.md, PROVENANCE.md and HANDOFF.md.
Implementation, implementation design, and original executable were not accessed or run.
Analyst evidence for Q-1–Q-16 was consulted for traceability only.

### Scan and change categories
- Full specification and question read-through plus recursive contamination scan: no original
  executable addresses, recovered symbols, source paths, decompilation, disassembly, original comments,
  long original strings, assets or wholesale executable-table dumps found in the reviewed inputs.
  Bit patterns and car-data offsets/chunk ID are numeric/data-format facts; planar acceleration labels
  are mathematical notation. REVIEW.md's scanner vocabulary is historical review terminology.
- SPEC S1-3.3: replaced storage-oriented nitrous-scale notation with an independent descriptive symbol
  as a precaution. This is naming hygiene, not evidence of a recovered original identifier or code exposure.
- SPEC S1-4.5: clarified accepted-selection wording and explicitly left initial assist timer/remembered
  gear as supplied state with retail initialization unresolved under U-2.
- QUESTIONS.md: marked interim implementation choices as historical and published reviewed disposition.
- test-vectors/README.md: made landing-table tolerance consistent with U-14 and documented isolated-case
  entry conditions, assist timing and supplied branch state; omitted inputs are not retail defaults.
- HANDOFF.md: added current gate status and marked old implementation/testing and open-item notes historical.

### Testability and provenance
All 18 added synthetic cases were independently checked against the specification arithmetic or
predicates. CSVs were read in full for schema and finite numeric data (2,580 rows total); parameter JSON
was inspected. These checks are not implementation tests, runtime observations or differential verification.
The Q-4 example tests current-force acceleration and one correction; it does not test five-pass sequencing.
Q-14 has no dedicated lateral-curve vector because that curve is outside Stage 1. Q-2 uses the stated
ratio-first convention with intermediate rounding unresolved; exact comparison is not a retail oracle.
All vector families have provenance in test-vectors/README.md: retail data-field observations plus derived
formulas, or invented synthetic inputs plus derived formulas. No runtime-captured vector is claimed.
Small selected car-data parameter facts and computed outputs remain; no original asset is included.

Q-1, Q-3–Q-10, Q-12–Q-13 resolved within the Stage-1 valid input contract; Q-14–Q-15 resolved as supplied
later-stage boundaries. Q-2, Q-11 and Q-16 remain partial. U-1–U-16 remain explicit, including U-2 assist
initialization/menu consumption, U-14 precision and U-16 absence of runtime proof. Approval is a clean
behavioral-spec gate, not a claim of complete retail fidelity. No confirmed contamination exposure was
identified; no implementation quarantine is requested on the evidence reviewed.

### Approval (supersedes earlier hash)
Date (UTC): 2026-10-15T13:35:25.760343+00:00
Role: reviewer
Outcome: APPROVED for the stated Stage-1 boundaries and explicit unknowns.
SPEC.md sha256: 61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb
Any later edit to SPEC.md invalidates this approval.
