# Settings spec review

## Scan result
Regex scan (addresses, sub_/FUN_/LAB_/DAT_, mangled names, .pdb/.cpp, register names) over SPEC.md, QUESTIONS.md and test-vectors/: no code addresses, disassembly, pseudo-C, mangled or source-file names. Hex hits are language-file label keys, CRC polynomial/init constants, checksum values in vectors and one PCI device id. Whole spec read manually.

## Changes
* SPEC 3.3: removed an implementation-internals remark (pointer null-check quirk); restated behaviourally.
* SPEC 4.5: removed specific PCI device id and per-device override detail (irrelevant legacy hardware); kept the behavioural override summary.
* SPEC header: added explicit flag that registry value names (incl. `g_...` names), label keys and English labels are functional interoperability data, not identifiers to copy.
* test-vectors/README: reworded generator provenance ("written for this project").

## Allowed with flag
* Registry value names and key paths, container magic "20CM", label hash keys with short English UI labels, language strings, CRC constants, executable sha256 prefix/size (identification only).

## Vectors
All synthetic (README states so); recomputed all checksums in tv02, tv03 and tv03b independently: they match, and tv03b fails only block B as documented. Profile name is TESTPROFILE, input-device block and career region are zero: no real user data or retail assets. Placeholders for unknown fields are labelled as guesses.

## Unknowns
Marked inline and listed in SPEC 11 and QUESTIONS.md; capture requests in SPEC 12. Facts are [static] only (no runtime capture).

## Contamination
None reached an implementer via this review.

## Approval
2026-10-13, role: reviewer, SPEC.md sha256: eac68834981e47fb835f0caa78097a2c1081c68304ba8e58c655a818a185d422
