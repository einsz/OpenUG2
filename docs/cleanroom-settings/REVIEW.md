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

---

# SPEC_GUI review (graphics/display options screens)

## Scan result
Regex scan over SPEC_GUI.md, PROVENANCE_GUI.md and test-vectors-gui/: no code addresses, sub_/FUN_/LAB_/DAT_, mangled names, .pdb/.cpp paths, register names or disassembly/pseudo-C. All 0x hits are 32-bit lookup keys (package, label, message, texture-name hashes), chunk type ids, or the hash seed; the spec flags these as functionally required data keys. Whole spec read manually; vectors scanned and tvg06 read.

## Changes
* SPEC_GUI 1.2: corrected a cross-reference (rebuild is 5.6, not 5.5).
* SPEC_GUI 10: retitled from "proposed" to accepted/applied; stated that SPEC_GUI supersedes SPEC 4.4 where they differ.
* SPEC 4.4: applied the four amendments (re-init serviced at start of next frame; no failure message, frontend list rebuilt; zero-cap toggle rows omitted from Customize; Start on in-game Display resumes without registry write). Each remains tagged [static] and cites SPEC_GUI sections.

## Adjudication of amendments
All four accepted: each is a behavioural, testable statement from static analysis that replaces an earlier looser/incorrect SPEC statement, with a matching vector in tvg06 (change_requires_reinit, ig_start_from_display, toggle_cap_zero_hidden). Amendment 2 is a negative claim (no message); backed by absence of such text in the language file, marked [static; data]. Outcome when every reset attempt fails stays unknown (SPEC_GUI 11.9).

## Allowed with flag
Executable sha256 prefix (identification), key hashes and English UI labels, registry value names (g_...), literal resolution strings (functional), authored object/script/texture names, tuned numeric layout data (pitch, anchors) as behavioural geometry. Pixel/colour measurements are from the user's own screenshot and data, not copied assets.

## Vectors
README states synthetic except tvg01 (observed metadata: names, keys, sizes, hashes, counts; no payload). No retail assets embedded. Each file has provenance/confidence notes.

## Unknowns
Marked inline and in SPEC_GUI 11 (10 items) with capture requests in 12.

## Contamination
None found; none reached an implementer via this review. Note PROVENANCE_GUI records the analyst did not read src/ or impl/.

## Approval
2026-10-14, role: reviewer
* SPEC_GUI.md sha256: 9bcb6c8b6a16fc5cece663f07f6b456e7c6d29f6ecc3a936af6359bcdaf81da8
* SPEC.md sha256 (amended, supersedes eac68834...): e79fe5fadcf5759f3862ce7e3de824b0e708a5c5446ae5a4d816227f5457c8ef
