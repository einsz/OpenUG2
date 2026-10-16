# Tire marks review

## Scan result
Starting-point grep over SPEC.md, QUESTIONS.md, test-vectors: no addresses, sub_/FUN_/LAB_/DAT_, mangled names, source paths, register or disassembly fragments. Hex hits are allowed: car-record data offsets (+0x120/+0x134 etc., data-format facts), and the three 32-bit texture name hashes with their asset names (functionally required lookup keys, flagged in TM-1.3). Full read-through done; symbols (k_s, q, kappa, etc.) are analyst's own. No original text/assets reproduced; texture pixels are not included.

## Changes
* SPEC TM-11.3: removed description of how the original's helper call is structured (code-structure detail); kept the behavioural formula and the "no practical fade" conclusion.
* SPEC TM-U6: removed reference to helper argument order; now "single static reading".
* SPEC TM-U11: removed CPU floating-point unit name; now "intermediate precision may exceed binary32".
* test-vectors/README.md: same x87 wording removed.

## Testability / vectors
Requirements are behavioural with [obs]/[inf]/[gue] tags and 12 flagged unknowns (TM-U1..U12; mirrored in QUESTIONS.md). Vectors are declared synthetic consequences of the spec (README + _provenance), none a runtime capture, no copyrighted assets. Reviewer re-checked: cell_keys (all rows recomputed from TM-10.2), render_alpha (TM-11.2), view_metric (TM-11.3, K=355.606; p and F values), and the 240SX quantisation example (w=.1025 -> 6/64). All agree.
Note for implementer: vectors only prove self-consistency with the spec text; the [inf] claims (lay thresholds, 8 kN, 128/64 capacities, alpha unity) are unverified against runtime.

## Contamination note
No contamination reached the implementer from this spec (no implementer had read it before review). PROVENANCE.md mentions the analyst's decompiled-project path as intake provenance only; not spec content.

## Approval
Date (UTC): 2026-10-15
Role: reviewer
SPEC.md sha256: 99b7079559ee599a861ac3baa0a107e2623be9097d1494c06f3605af38b242b5
test-vectors/tire_marks.json sha256: 1f18c4b4b0f299711dc68a291528533b84ab42dee213b10081a6ba14ed2e2633
Any later edit to SPEC.md invalidates approval.

## Amendment review: TM-15 render state (2026-10-16)
Scan: starting-point grep plus targeted grep for effect/technique/shader-language tokens and absolute paths over SPEC.md, QUESTIONS.md, test-vectors: nothing found. Hex hits are only the pre-approved texture name hashes and data offsets. Full read of TM-11.2, TM-14, TM-15.1-15.7, TM-U4: shader behaviour is described as formulas in neutral terms (no effect/technique/pass names, no shader code, no register-level steps, no addresses); fog constants appear only as generic (Fx, Fy, Fz). Texture data is numeric statistics only (alpha min/max/mean, mean RGB), no pixels or palettes.
Changes: none required (no contamination found); no text removed.
Recomputation (independent script): render_state.json effective_alpha (9 rows: vA = max(Ib-F,0)>>1, a = tA/255*vA/255), source_colour (5 rows, min(1, 2*t/255*128/255)), composite (5 rows, 1/2/4 layers over 0.5 grey, src*a + dst*(1-a) repeated) all agree with the file within 1e-6 and with the spec text (0.24804, 0.22297, 0.07422, 0.08203, 0.19922, 0.17621; 0.384 / 0.297 / 0.182). Vertex alpha mapping 255->127, 76->38, 85->42 agrees with TM-11.2.
Testability: TM-15 requirements are formula/state-table based with [obs]/[inf] tags; residuals listed in TM-U4. Vectors declared synthetic (render_state.json _provenance, README); no copyrighted assets.
Notes for coordinator: (1) TM-15 contradicts the existing implementation claim of alpha x2 (VERIFIER_AMENDMENTS V9, QUESTIONS TM-IQ3): spec now says alpha x1 (127 -> 0.498), colour x2 with RGB 128. Implementation needs update; no spec contamination reached the implementer via this amendment as far as the reviewer knows. (2) QUESTIONS.md TM-IQ3 text is stale relative to TM-15. (3) PROVENANCE names the Wine d3dx9 source as an analyst parser reference only; nothing copied.
Approval (amendment)
Date (UTC): 2026-10-16
Role: reviewer
SPEC.md sha256: 1b864fc16a007d8e6d6736bf8615b469ec975abc695620bf7a1bbbfd784ff454
test-vectors/render_state.json sha256: 5aa0eace8e1f55a3509211c3e7486efa81bd121252b181d761a40abc86de7e25
test-vectors/README.md sha256: 4eae6ec4bf1796d279603295afb293ac9d0e6540ce5e798758a074c992a4cb11
Any later edit to these files invalidates approval. Supersedes SPEC hash 99b70795...; hand-off hash 1b864fc1... is the one reviewed.
