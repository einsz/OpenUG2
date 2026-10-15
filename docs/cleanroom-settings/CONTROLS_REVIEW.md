# CONTROLS_SPEC review log

Scope: CONTROLS_SPEC.md, test-vectors/controls/*, "Controls screen" section of QUESTIONS.md.
(REVIEW.md untouched.)

## Contamination scan
* Regex scan (addresses, sub_/FUN_/LAB_/DAT_, mangled names, .pdb/.cpp, registers): only hits
  were 32-bit hash keys, chunk ids and a partial executable sha256, all data lookup keys or
  identification hashes. Accepted as functionally required (flagged as such in the spec).
* Cross-check: all 12 original code addresses present in cleanroom/analysis/controls were
  searched for in the spec, vectors and QUESTIONS.md: none found.
* Full read: no pseudo-code, decompiler fragments, original function/variable/struct names or
  long verbatim strings. Quoted display strings are short and functionally required. Vectors
  contain no retail asset bytes (package sha256 is an identifier only). Known-device default
  tables (tvc05) are small functional configuration data, accepted.
* External URLs (public forum posts) are citations only.

## Changes
* Removed (category: original identifiers from executable) three per-device-class package file
  names the executable carries, in section 2.3; replaced by a generic statement.
* Added (category: resource policy) "Normative resource rule" R1-R4 after the Identifiers/
  Hashes preamble: no new art/font/layout/string/sound; only original retail resources by
  key/name; feature unavailable if missing; no asset bytes copied to the repo.

## Testability / unknowns
Requirements are behavioural; vectors tvc01-tvc09 attached and described in
test-vectors/controls/README.md as synthetic/observed from data files (provenance stated).
Unknowns are flagged U1-U10 and D1-D6 (runtime verification absent, nothing was run).

## Approval
Date (UTC): 2026-10-14
Role: reviewer
Status: APPROVED (any later edit to CONTROLS_SPEC.md invalidates this)
sha256 CONTROLS_SPEC.md: 3295a4d436958c6c784ec752db9c2e64cb976a9802eb17a91f2147a035639c18
