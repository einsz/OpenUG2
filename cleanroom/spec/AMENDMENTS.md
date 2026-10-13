# Spec amendments

## CHANGELOG

### Amendment 7 — 2026-10-13, rendered dialog legibility acceptance

Source: verifier observation of the independently recreated demo at 1280×960
and 1280×720, plus user-authorized runtime-check source configuration. No
original executable execution or new retail analysis.

- SPEC 20.9 and DIALOG_RENDER_ACCEPTANCE.md refine DEMO_DESIGN D2 page 8/D3:
  every enabled choice has final-frame filled-ink acceptance, all focus
  destinations and both sizes are covered, and primitive/mirrored tint tests
  alone are insufficient.
- Preserve genuine font/art, approved unfocused palette/alpha and authentic
  focus pulse. Opaque pulse endpoints test focused ink; zero-alpha focused
  phases remain intentional and never excuse missing unfocused ink.
- Runtime-only inputs may be passed to recreated loaders/renderers; direct raw
  original inspection, analyst/evidence access and original execution remain
  forbidden to an implementer. No retail pixels are included in the vectors.
- All prior demo/Xbox requirements and explicit unknowns remain applicable.

### Amendment 6 — 2026-10-13, demo composition and verified Xbox source

Source: reviewed PC reference facts, independently rendered demo audit, and
read-only observation of the newly supplied owned Xbox disc. No original game
execution. DEMO_DESIGN.md marks new composition choices D; XBOX_CONTROLLER.md
separates observed source/texture facts, public-format interpretation and policy.

- SPEC 11/20.1/20.3/20.5/20.6/20.8 distinguish earlier PC/PS2-only availability
  from the new verified 47-glyph Xbox source and narrow BC1/BC2 decoding scope.
- Earlier absent-source Xbox cases retain their meaning; validated new source
  enables genuine Xbox gallery art without a claimed retail action-token map.
- Default PC text-only pills, original-only assets, full corner transforms and
  one active strip remain required. Original-screen authored geometry remains
  authoritative; new margins/grouping/diagnostic separation apply to demos.
- New synthetic vectors exercise BC blocks, record/descriptor bounds and XDVDFS
  directory structure without disc pixels or original assets.
- Retail console bindings/baseline/size, Xbox descriptor/mip/compressed variants,
  other PS2 formats and the original runtime uncertainties remain explicit.

### Amendment 5 — 2026-10-13, PC text-only pill restoration

Source: user correction and already reviewed PC geometry/slot observations in
SPEC 7.4/7.4.1 and 19; no new original analysis. Normative rules are in 20.7.

- Restore PC original help-bar strip, pill art, labels, hit targets, hover/glow
  animation and application-bound click actions, with no PC/mouse/key icons.
- Default desktop demo and PC original viewer to PC text-only; allow explicit
  PS2/Xbox selection. Scope 20.3/20.6 suppression to console profiles.
- Preserve dialog hiding, original-only resources, corner corrections, console
  icon provenance and honest unavailable Xbox behavior.
- RESTORATION_PC_PILLS.md adds acceptance cases derived from the user correction
  and prior reviewed observations. Historical round-4 console-only cases remain
  valid when their console profile is explicitly selected.


### Amendment 4 verification clarification — 2026-10-13

Reviewer-gated requirements are in SPEC 20.6 and VERIFICATION_ROUND4.md.
Source: existing approved presentation policy (20.3), plus verifier observations
of the independently recreated demo. No original executable was run and no new
retail-console behavior is claimed.

- Console-strip expectations supersede historical PC tests, including when
  glyph resources are unavailable.
- All family-selection labels agree with the selected PS2 or Xbox family.
- Showcase selection and configuration text do not overlap at 1280×960.
- A tour classifies deliberate PC overlay suppression separately from unexpected
  load failures; actual missing/unsupported resources and GL checks remain.


### Amendment 4 — 2026-10-13, analyst round 4 (needs reviewer approval)

Source: user direction removing pills and invented artwork, two supplied corner
crops, original PC global texture data and authored screen transforms, and
read-only data observation of the owned USA PS2 disc. No game execution.

- Goal/header and 5/8.1/10: remove old procedural-art, substitute-font,
  mouse-hint, compact-PC-grid and placeholder permissions.
- 7.5 is historical; 7.6's mouse artwork/design is explicitly superseded.
- New 20.1: audit every visual path; runtime original resources only, provenance
  records, no original assets committed; unavailable resources fail closed.
- New 20.2: native panel-corner orientation, full quaternion projection including
  x/y half turns, signed width/height and fixed UV association.
- New 20.3: one console strip, suppression of PC overlay/pills, persistent mouse
  input, PS2/Xbox selection with honest availability and explicit binding maps.
- New 20.4: verified 19 PS2 controller textures from the disc's packed file,
  124-byte records, 84-byte opaque descriptors, 32×32 format 4, RGBA CLUT alpha ×2,
  observed swizzled nibble addressing. No image pixels copied into the spec.
- New 20.5: no verified Xbox assets from provided sources; unknown controller
  token bindings, baseline and arbitrary PS2 texture formats remain explicit.
- New synthetic vectors: `corner_transform_vectors.json`,
  `ps2_icon_codec_vectors.json`, `original_visual_policy_vectors.json`.

Historical retail PC pills remain documented as observations. Their earlier
screenshots/checklists do not override the requested console presentation.

### Amendment 3 — 2026-10-13, analyst round 3 (needs reviewer approval)
Base: SPEC.md sha256 3a6b4b3f… / AMENDMENTS.md 102c07da… (REVIEW.md round
2). Source of new facts: six still captures of the original PC game supplied
by the user (1920 × 1080, mouse active, English; the user's install contains
third-party-modified data files, 14.3). They are described in SPEC 19.1 as
C1–C6:

* C1: main menu with the second item selected.
* C2: audio options list with the first row focused.
* C3: quit confirmation over the main menu with its last item selected.
* C4: customize car select at the first car.
* C5: PC keyboard-controls page.
* C6: two-line save confirmation.

The analyst did not run the game. New numeric facts carry the confidence
tag **R** (measured on these captures, ±1 virtual unit unless stated).
Measurements were cross-checked against the screen-package data: every
authored position checked agreed within 1–1.5 units.

Changed sections:
- Header: draft 3; legend adds **R**.
- **1.1** Non-4:3 presentation: pillarboxed 4:3 viewport, black bars,
  everything clipped (R; caveat: possibly from a fix in the capture
  install).
- **4.5** Wrap: icon scrollers and the car carousel do not wrap; the end
  arrows are hidden (R). Key repeat unchanged (G).
- **4.6** Cursor drawn at 32 × 32 virtual units, clipped, texture key named
  (answers Q23).
- **6.4** Icon scroller: 3 visible icons, neighbours 96 × 48 at ±158, full
  opacity; focus-frame line geometry; band, arrow and track positions;
  thumb formula confirmed; main menu has 7 items on PC with a partial order
  (replaces the G spacing of 128; answers Q16, Q20).
- **6.5** Option list: row pitch 36 (replaces G 30), visible rows 8 / 7,
  focused-row look, value arrows only on the focused row, scroller
  positions (answers Q16).
- **6.6** Option-row slider positions and colour.
- **6.9** Dialog: default focus is the right/negative button (replaces G
  "first button"); focused vs unfocused button look; visible button extent;
  border line runs along the panel outline; title placement without an icon;
  message positions; ×0.40 darkening of the frame beneath; PC help bar
  hidden while a dialog is open.
- **6.11** Stat bars: measured label, fill and knob positions.
- **6.17** Car select: no wrap (left arrow hidden at index 0); measured
  positions.
- **6.21** Cursor size and clipping.
- **7.1** On PC the console strip is not drawn (answers Q19).
- **7.4.1** (new) The PC help bar as observed: strip geometry, dark player box
  (answers Q17), pill extents and label placement, slot assignment per
  screen, no Help pill.
- **7.6** "PC original" mode uses 7.4.1.
- **8.1**, **16.5** Line pitch corrected to (cell height + leading) × scale
  (I from one capture). The width rule was confirmed on 15 strings.
- **11** Unknowns rewritten: closed or narrowed items listed; 19 open items.
- **18** Capture status note.
- **19** (new) Reference capture set C1–C6 and a 40-item reference
  comparison checklist for the verifier.

Not changed: key repeat (4.5 G), hover/wheel semantics (4.6 G), animation
timings (5), help-popup glyph rendering (7.3, 7.5 G); none of them can be
seen in stills.

## Answers to QUESTIONS.md (amendment 3)

| Q | Answer | Spec |
| --- | --- | --- |
| 16 | Icon spacing: neighbours at ±158 and drawn 96 × 48; 3 icons visible. Row pitch: 36. Visible rows: 8 (plain list) / 7 (PC controls page). No visual wrap at either end of icon scrollers. Key repeat: still G. | 4.5, 6.4, 6.5 |
| 17 | The retail player box is **dark**, darker than the hint box, never grey. Modulate (texel × colour) is correct; the procedural fallback should also draw it dark. | 7.4.1 |
| 18 | Moot on PC: the console strip is not drawn there (C1, C2, C4, C5). The PC strip's left end is a 16 × 16 rounded corner over a 16-wide box at x -300.4 .. -284.4, matching the captures (left edge at x -300). | 7.1, 7.4.1 |
| 19 | The PC shows **only** the PC help bar's strip (51 high, y 189.2 to the bottom edge) plus its pills. The console 32-high strip is not drawn, and the two never stack. | 7.1, 7.4.1 |
| 20 | Neighbours are drawn **96 × 48** (not 118 × 59, not 108 × 44) at ±158, **full opacity**, without backing. Only one neighbour per side is visible. The thumb follows index/(N − 1): x = -273.7 + 547.4·i/(N − 1), observed for i = 1 and 6 with N = 7. | 6.4 |
| 21 | Not answerable from stills (message hashes are application logic). Observed result only: the two-button layout shows buttons at x ±150 with the right one focused, and the three-button set is not shown. | 6.9 |
| 22 | No data-level rule found. Observed: "option title", player name and value placeholders are replaced in every capture; "Option title"-style texts never appear. Keep the current approach. | 3.7 |
| 23 | Texture key = hash(`PC_CURSOR`) in the PC global pack; drawn 32 × 32 virtual units, clipped to the viewport. | 4.6 |
| 24 | Still no evidence (the captures show only grey frontend P8 content). | 15.7 |
| 25 | The icon focus frame was measured (line centres ±60 × -161/-106, about 2 units thick, accent at peak, glow about 3 units). The dialog border line runs along the panel outline (6.9). Line/corner orientation per object: not resolvable from stills beyond this. | 6.4, 6.9 |
| 26 | No action (acknowledged). | — |
| 27 | Not covered by the captures (English only). Still open. | 11 |

### Amendment 2 — 2026-10-13, analyst round 2 (needs reviewer approval)
Base: SPEC.md approved sha256 92c9026f… (REVIEW.md). Any approval is void
until the reviewer re-gates the sections below.

New sections (Part II, appended after section 12):
- **13** Compression codecs: JDLZ and HUFF wrapper headers; JDLZ algorithm;
  EA 30FB Huffman algorithm; where compression appears.
- **14** Resource bundles: chunk walk rules; which install files the GUI
  needs; texture-resolution census; original vs 2023-modified install
  container.
- **15** Texture packs: chunk tree; pack info; key list;  124-byte record;
  pixel-data base alignment; compressed-slot variant; pixel formats
  DXT1/DXT3/BGRA8/P8; format descriptor; how screens and the program
  reference textures (program-assigned icon families, main-menu icon and
  label names).
- **16** Fonts: chunk layout, header, glyph record, kerning, inventory and
  lookup, layout and rendering algorithm.
- **17** Strings: language-file structure, code map, inline markup.
- **18** Reference-capture plan (R1–R14) for the verifier.

Changed sections:
- Header status line (draft 2).
- **1.4** Colour product changed to `(a*b+127)/255` (answers Q2).
- **3.2** JDLZ/HUFF now point to 13; hash prefix = hash of PkHd name;
  raw-package note.
- **3.5.2** Quaternion delta recommendation (answers Q1).
- **3.5.4** Loop-wrap event rule (answers Q3).
- **6.9** Dialog geometry per variant. Correction: 562 x 198 is SMALL, not
  large (answers Q8).
- **6.11** Stat-bar overlap is authentic (answers Q7).
- **6.13** Swatch row geometry corrected: 32 on screen at ~42 pitch
  (answers Q10).
- **7.1** Exact bottom-strip piece ranges (answers Q6).
- **8.1** Use the original font files; the substitute is fallback only
  (answers Q14).
- **8.4** Block-height rule for vertical anchors.
- **11** Unknowns rewritten (closed vs open).
- **12** References: psx-spx EA Huffman; D3D format enumeration; BC1–BC3.
- `test-vectors/README.md` extended. New vectors: `jdlz_vectors.json`,
  `huff_vectors.json`, `tpk_table.hex`/`.expected.json`,
  `tpk_slots.hex`/`.expected.json`, `font_chunk.hex`,
  `font_layout_vectors.json`, `bundle_compressed.hex`, `bundle_raw.hex`,
  `bundle.expected.json`.

Not changed (still awaiting observation of the running original): 4.5 key
repeat and wrap, 4.6 pointer behaviour, 6.4 icon spacing, 6.5 row pitch and
visible rows. Their G values stand.

## Answers to QUESTIONS.md (amendment 2)

| Q | Answer | Spec |
| --- | --- | --- |
| 1 | One retail screen uses non-identity quaternion deltas (z rotations up to ~290°). Recommended (G): component-wise linear interpolation, no shortest-path flip, normalise, result = delta * base. Your `base * normalize(lerp)` gives the same result for these pure-z rotations. | 3.5.2 |
| 2 | Not observable from data. Normative choice (G): `(a*b+127)/255`, so 255 x 255 = 255. | 1.4 |
| 3 | Events at t = 0 fire only on script start, not on loop wrap (G). Events at t = length fire at the wrap. | 3.5.4 |
| 4 | Bool/int tracks do not occur in retail data. Your 32-bit choice is fine (unchanged). | 3.5.1 |
| 5 | Yes: the library must decode JDLZ (and HUFF) itself, to load original assets at run time. Algorithms and vectors are in 13. | 13 |
| 6 | The strip pieces are left cap, grey 167.4, gap, dark 385, right cap; no overlap; span -309..+310. On PC the console hint items are hidden and the PC help bar provides the right-hand content. | 7.1, 7.4 |
| 7 | The overlap is authentic; with the original faint texture it is not noticeable. Keep 256. | 6.11 |
| 8 | SMALL is 562 x 198 (panel y -91..+107); MED and LARGE are taller (same width). Buttons are 214 x 42 at x ±150 in every variant. | 6.9 |
| 9 | Not determinable from data (application logic). Keep your choice; screenshot request A5. | 11 |
| 10 | 12 swatches in one row, 32 on screen at ~42 pitch. | 6.13 |
| 11 | Only 0,1,2,4,5,6,8,10 occur in retail data, so precedence never matters for retail screens. Your rule is acceptable (G). | 8.4 |
| 12 | Unknown; screenshot request A7. | 11 |
| 13 | Reading confirmed as I (no new evidence). | 4.4 |
| 14 | Superseded: render with the original font (16). The pixel font is acceptable only as a no-install fallback. | 8.1, 16 |
| 15 | Still G. | 11 |
| 16 | Still G pending observation (A1, A2, A4). | 11 |
