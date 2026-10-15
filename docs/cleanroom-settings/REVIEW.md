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

---
# SPEC_GUI_ART review (authored-art supplement, tvg07-tvg11)

## Scan result
Regex scan plus full read of SPEC_GUI_ART.md, tvg07-tvg11 (valid JSON, header/body read), README and PROVENANCE_GUI.md additions: no code addresses, sub_/FUN_/LAB_/DAT_, mangled names, source paths, register names, disassembly or pseudo-C. Hex literals are lookup keys (package, object, message, label, texture hashes), flag values and game-mode constants, flagged as functionally required data keys. "Static"/"data" tags are provenance labels, not code. analysis/settings-gui/notes/evidence-index-art.md and evidence/decomp were not copied from; they stay analyst-only and uncommitted.

## Changes
* SPEC_GUI_ART header: status changed from "draft" to approved; section 9 retitled (corrections accepted).
* SPEC_GUI 6: Default Settings initial focus corrected to second button (No), no wrap, Back = No; supersession note added. Reference to an external pause-menu QUIT_DIALOG work replaced by pointer to SPEC_GUI_ART 6.
* SPEC_GUI 11 items 4, 5 and capture request 3: marked resolved / updated expectations.
* tvg06 defaults_yes: initial_dialog_focus corrected; README correction notice rewritten as "applied".

## Adjudication
1. Pause quit confirmation = UI_InGameDialog, OK/Cancel, default focus Cancel, Back posts cancel message: ACCEPTED (SPEC_GUI_ART 2.4, 6, 9.3; vector tvg10). It supersedes the earlier pause-menu QUIT_DIALOG default (IG_GenericDialog_SMALL, Yes/No). That earlier document lives outside this spec dir (bkup tree) and was not edited by me; the coordinator should mark it superseded.
2. Default Settings dialog focus No, no wrap: ACCEPTED (SPEC_GUI 6 amended, tvg06 amended). Evidence: authored responses plus retail frontend quit-dialog observation; runtime capture still pending (capture request 3).

## Vectors
README: all synthetic, no retail payload; cross-checks against user's own screenshots stated with tolerances. Provenance stated per file.

## Provenance note
PROVENANCE_GUI exposure log: analyst saw file names/line counts of IMPL_DESIGN*.md only (content not opened). No implementer content reached the analyst; no contamination reached an implementer from this review. Coordinator should note IMPL_DESIGN_GUI.md was written before SPEC_GUI_ART/the two adjudications, so it may embody the superseded Yes-focus and Yes/No quit dialog and need updating.

## Remaining unknowns
SPEC_GUI_ART section 10 (activation message reaching handler, pause appearance during dialog, meanings of mode/race values 7 and sub-state 4, replay enable condition detail, tween time unit, hover/wheel/scrollbar clicks, in-game help bar).

## Approval (supplement)
2026-10-14 (UTC), role: reviewer
* SPEC_GUI.md sha256 (amended, supersedes 9bcb6c8b...): b62da2f03a77377592d42a6d9c388ae7f45a60646737bc8026bf188417ceb2b5
* SPEC_GUI_ART.md sha256: 33e706651bb69c73d9fe945e14e805cf308d1304afb0113015802005091d164b
* SPEC.md sha256 (unchanged): e79fe5fadcf5759f3862ce7e3de824b0e708a5c5446ae5a4d816227f5457c8ef
* tvg06_flow.json sha256: c22462ba11ffd4dcd5e3021ed9c4d9ae25e6d85410abc68aac9856ffb3b51b86

---
# SPEC_GUI_ART_AMEND1 review (coordinate frames, group composition, anchor read rule; tvg12)

## Scan result
Regex scan plus full read of SPEC_GUI_ART_AMEND1.md and tvg12_frames.json (valid JSON): no code addresses, sub_/FUN_/LAB_/DAT_, mangled names, register names, disassembly or pseudo-C. 8-digit hex values are object/script/message/texture lookup keys, flagged as functionally required data keys in the header. Public FEngLib file paths are public-source citations, not original source paths. SPEC_GUI_ART.md was not edited (its approved hash is unchanged).

## Changes
* Source-structure / evidence pointers: removed references to analyst-only paths (group audit evidence file, generator script) from A3 and section D; replaced by "analyst audit / generator, not distributed".
* Structure-copying: A6 rule rewritten behaviorally ("sampled once at screen creation") instead of describing constructors and per-frame update internals; the "chains to second script" wording rewritten as "automatically followed by a second authored script" (key kept as data).
* Arithmetic corrections: containment check focused slot y upper bound 14.70 -> 15.20 (-14.296 + 29.5); D1 symptom "about 183 units" -> "about 184" with x off by 0.39; tvg12 `coincides_with` tolerance text (was |dx|<=0.31, false for the hub glow at 0.39) -> "within 0.4 units (about 0.9 px at 1080p)".
* Status/precedence: draft -> approved; declared as a supplement to SPEC_GUI_ART.md with explicit precedence (A6 anchor read rule over the master reads of 3.2; A5/A7 complete 3.1/3.2; B2 replaces unknown 10 item 2, as inference).
* Unknown marking: below-4:3 mapping "guessed and untested"; post-Default-Settings master re-read "guessed; not verified"; B2 caption/icon alpha under pause dialogs marked [guessed] until capture request 2.
* README: tvg12 entry added.

## Arithmetic verification (reviewer script, scratch only)
All expected_px centres and sizes in tvg12 (every case, three resolutions) recomputed from virtual values with s = min(W/640, H/480): 2.25 at 1920x1080 (origin 960,540), 1.5 at 1280x720 (origin 640,360), 1 at 640x480; parent-chain sums, slot centres = anchor.x + (i-1)*158 + s, 0.8 neighbour scaling, and the group-scale example all match within 0.01 px. Retail cross-checks lie within 2.5 px (strip 2.1 px, glow 1.4 px). Spot checks of the written text: pillarbox columns 240...1679, y 922.5 px for the chained value, hub Display neighbour at (1316.4, 507.8) and (877.6, 338.6).

## Vector provenance
Synthetic (authored numbers + rules), stated in the file; no copyrighted asset payload; retail cross-checks come from the user's own screenshots and are informative.

## Contamination
None found; none reached an implementer via this review.

## Remaining unknowns
Non-4:3 mapping (guessed); post-Default-Settings master re-read; B2 pause appearance under dialogs (inferred, alpha guessed); dialog second label steady vs pulsing; sub-pixel conventions; z order; group scale/rotation order (inferred from public FEngLib, unexercised).

## Approval (AMEND1)
2026-10-14 (UTC), role: reviewer
* SPEC_GUI_ART_AMEND1.md sha256: 51c255546b0f977f8f2822b3b9c167bc56cb409a9e0bde2e6c3070125587d539
* test-vectors-gui/tvg12_frames.json sha256: a2a3d94bdbd6f50f3b37f29bb5ce67bb33cad04b2a14881b777901b588d41d53
* test-vectors-gui/README.md sha256: 6b583ce629875a7e1f212dd59952ec3afaa0cb7d567a72f5ae640dc3e7c8acb4
* SPEC_GUI_ART.md sha256 (unchanged): 33e706651bb69c73d9fe945e14e805cf308d1304afb0113015802005091d164b
