# Camera spec review

## Scan result
No executable addresses, sub_/FUN_ names, mangled names, disassembly, pseudo-C or register sequences found. Hex hits in SPEC.md are string-hash keys, chunk id 0x00034600 and noise constants (functional data keys, allowed).

## Changes
* Tuning (SPEC 4): removed raw "record pair" dump of executable-embedded values; kept only effective per-view table with meaning descriptions; reworded storage description.
* SPEC 12: removed pointer to analysis/ file; replaced with inline list.
* tv01: removed file-offset column (location in user's file, not needed).
* test-vectors/README: removed reference to analyst-only script path; added provenance note.

## Allowed with flag
* Hash keys for UI labels, chunk id and record offsets (data format), surface-type names (engine data keys), lattice-noise constants, public Nikki library citation (public source).
* Vectors: "data" from user's retail files (numeric values only); "derived" from analyst model, not runtime captures. Unknowns are listed in SPEC 13 and tagged confidence levels.

## Approval
2026-10-13, role: reviewer, SPEC.md sha256: 053d727efc0598d0050503097fd5d836bae7c555bced242a59f9e97a98b6198a

## Amendments 1 review (2026-10-13)
Scan of AMENDMENTS-1.md: no addresses, disassembly, or original identifiers. Removed from consideration (not copied into SPEC): implementation-divergence list, implementation constants and doc references (implementation-side, not spec), the view-id 9/10 override aside (irrelevant), and "static inspection" process narrative beyond the [static] tag.
Applied: A1 (SPEC 4 look z -> +-1, data-backed, matches tv02); A2 (SPEC 7 integer-angle truncation rule, matches tv09); A3-A6 recorded as open unknowns (SPEC 13 items 15-18, SPEC 8 note). A7 no change. QUESTIONS.md updated.
Caveat: A2 rests on static inspection only; no runtime capture. tv03-tv10 remain derived, not game captures.
No contamination reached an implementer via this review (amendments contained none).

## Approval (supersedes earlier hash)
2026-10-13, role: reviewer, SPEC.md sha256: ba92a5f3ce4110502fb09a5445739a53f2323d3363505125fe3e8ea460a78eb1

## Amendments 2 review (2026-10-13)
Scan of whole directory (SPEC, QUESTIONS, README, all vectors): no code addresses, disassembly, mangled or original symbol names, analyst paths or pseudo-C. Hex hits are the previously allowed hash keys, chunk id and noise constants. No contamination reached an implementer.
* P1 (project policy, not original behaviour): added SPEC 7.1 "widescreen projection (OpenUG2 addition)", clearly labelled. Clarified an ambiguity in the amendment: the horizontal SCALE is multiplied by R/A (not the angle factor s0), which is what the stated consequences (0.75 at 16:9) require; no re-truncation. New vector tv11_projection_policy.csv (derived; 640x480 rows match tv09).
* P2: tv01 trimmed from 46/47 cars to the 3 referenced by other vectors; added synthetic tv01b_synthetic_car_records.csv (marked synthetic, with entry byte encodings); README and SPEC 3.3 updated, README documents the optional local-only full table and structural-only fallback. The full original table is kept local-only, outside the repository.
* Deleted AMENDMENTS-2.md after folding it in.
* Paths in docs already referred to no analyst locations; none changed.
* SPEC 3.3 still gives per-view value ranges (summary, not a table dump); judged acceptable.

## Approval (supersedes earlier hash)
2026-10-13, role: reviewer, SPEC.md sha256: d4e52dd2e801db41e40742187a8f1e0687aa6a0240604e3afaca845d4dbaa3da
