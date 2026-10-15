# SPEC_GUI_ART Amendment 1: coordinate frames, group composition, anchor read rule

Status: **approved by review 2026-10-14** (hash in `REVIEW.md`). This file **supplements**
`SPEC_GUI_ART.md` (§1.2, §3.1, §3.2, §3.4, §4.2, §6, §10 item 2); it is read together with it.
It adds the frame and transform definitions those sections leave implicit, and the anchor read
rule whose absence caused verifier defect D1 (`VERIFY_GUI.md`, "Verification 2026-10-14", §D).
Where this file and `SPEC_GUI_ART.md` both give a number, they agree.

**Precedence.** Where the two files differ, this file prevails in exactly these places:
(1) §3.2's "OPTION_MASTER.x/.y" is the cached anchor of A6 (authored value read at creation), never
a value re-evaluated through scripts; (2) §3.1/§3.2 geometry is completed by A5/A7; (3) §10 item 2 of
`SPEC_GUI_ART.md` ("final alpha unknown") is replaced by B2, which is an inference, not an
observation (see C5). Hexadecimal values below are lookup keys (object, script, message and
texture hashes) and are given only as functionally required data keys.

Provenance: the retail 2004 package data (`GLOBAL/InGameCommon` `UI_pause.fng`,
`UI_PauseOptionsMain.fng`, `UI_PauseOptions.fng`, `UI_InGameDialog.fng`; `FRONTEND/FRONTB.LZC.bacc`
`UI_Options.fng`, `UI_PC_Customize_Options.fng`; `GLOBAL/GLOBALB.BUN` `GenericDialog_SMALL.fng`);
static examination of the retail executable (same binary as the earlier specs); the public FEngLib
project (renderer transform order and colour blending: `FEngRender/Data/RenderTreeNode.cs`,
`FEngLib/Structures/Color4.cs`, https://github.com/NFSTools/FEngLib); and the user-supplied retail
screenshots (pause screen in free roam, SHA-256 `387d97bb…852a3`; frontend Audio page; frontend
quit dialog). The game was not run. Tags are used as in `SPEC_GUI_ART.md`: **[data]**, **[static]**,
**[observed]**, **[inferred]**, **[guessed]**, plus **[public]** for the FEngLib reconstruction.

---

## A. Rules

### A1. Virtual screen and the mapping to pixels [observed]

* Every position in `SPEC_GUI.md`, `SPEC_GUI_ART.md` and this file is in **centred virtual units**.
  The virtual screen is 640×480. The origin (0, 0) is the screen centre, x points right and y points
  down.
* The mapping to an output of W×H pixels is a uniform scale about the centre:

  `s = min(W/640, H/480)`, `px = W/2 + s·X`, `py = H/2 + s·Y`. Sizes scale by s on both axes.

  At 16:9 this leaves pillarbox bars at both sides. At 1920×1080 s = 2.25 and the origin is pixel
  (960, 540). At 1280×720 s = 1.5 and the origin is (640, 360). [observed: all retail reference
  shots at 1920×1080 are 4:3 pillarboxed with s = 2.25, content in columns 240…1679.] Whether the
  retail game stretches instead when no widescreen patch is present is unknown (§C1). The `min` form
  for aspect ratios below 4:3 (for example 5:4 letterboxing) is [guessed].
* No rounding to whole pixels is defined. Vectors use a ±1 px tolerance.
* The third (z) component of positions **does not affect** screen x/y. There is no perspective
  projection: the pause panel group sits at z ≈ 533 and the dialog dim quad at z 583, yet the
  retail screenshot shows them exactly at their unscaled x/y extents. [observed]

### A2. Object frames [data]

* Every object has a position, a size, a rotation (quaternion), a pivot and a colour. For images,
  the position is the **centre** of the image rectangle. For text objects it is the text anchor of
  the base text rules. For **groups** (FEng object type 5) it is the origin of the children's frame.
  A group draws nothing, and its "size" is a **scale factor** applied to its children, not a
  rectangle.
* An object's position is **relative to its parent group's origin**. A top-level object (no parent)
  is positioned directly in the screen frame of A1.
* Nesting depth in the packages covered here is at most 3 (UI_pause: presentation group → row group
  → box).

### A3. Composition (child to screen)

For an object o with parent chain G1 (nearest) … Gk (top level):

```
P0 = position(o)                                  // local, parent-relative
for j = 1..k:
    Pj = position(Gj) + Rot(Gj) · ( Scale(Gj) ⊙ P(j-1) )   // scale first, then rotate, then translate
screen centre (virtual) = Pk
drawn size   = size(o) ⊙ Scale(G1) ⊙ … ⊙ Scale(Gk)
drawn rotation = rotation(o) applied about o's own centre (pivot), then the ancestors' rotations
drawn colour = colour(o) ⊗ colour(G1) ⊗ … ⊗ colour(Gk),  a ⊗ b = (a·b + 128) >> 8 per channel
```

* Scale is the group's size vector, rotation is the group's quaternion and ⊙ is per-component
  multiplication. The order (scale, rotation about the pivot, translation, then the parent's
  transform) and the colour product follow the public FEngLib renderer [public]. That the retail
  executable composes in the same order is [inferred].
* **None of the objects quoted in `SPEC_GUI_ART.md`, `SPEC_GUI.md` or this file has an ancestor
  with non-unit scale, non-identity rotation or non-zero pivot.** For all of them, composition is
  exactly the **sum of the positions along the chain**, and drawn size = own size. [data: audit of every
  group in the listed packages, performed by the analyst; not distributed] The only groups with non-unit scale in
  these packages are:
  * `OPTION_BACKING_14/15` (`UI_Options`) and `_26/_27` (Customize), scale (1, 1.5). No page of
    these screens has that many rows.
  * Two console/decoration groups (`0xB1B1EAB0`, scale (0.78, 1); no children in the dialogs).
  * The pause/hub `Package` → `0x550E571C` "appear" group, whose only child has zero size.

  None of these draws anything on these screens [data; inferred for "no page reaches row 14"].
* Object rotations (own centre, not moving the centre) occur on: `ScrollBar_Arrow_1` (180° about z)
  in the carousel packages; `LEFT_ARROW_n` (180°); the list scroll arrows (±90°); some corner
  pieces (180° about z, or about y, which mirrors horizontally). [data]

### A4. Which value of a property is used [data; public]

* Each frame, every property comes from the **current script** of the object if that script has a
  track for the property. The value is the track's base key plus the interpolated **delta** of the
  later keys. Keys after the base are offsets added to the base. Evidence: `OPTION_MASTER` `Init`
  position base (0, −12.3512) plus key (0.3884, −1.9449) gives (0.3884, −14.2961), which equals the
  stored value. If the script has no such track, the property comes from the object's stored data.
  The application's property writes follow `SPEC_GUI_ART` §1.2 (base keys of every script, or the
  stored data).
* A finished script keeps its last value until another script is started. A group's current script
  therefore moves all its children. Example: in `UI_PauseOptionsMain.fng` the presentation group
  `0x8D7E0C53` has stored x −39.0, but its `FadeIn`/`fadeout`/`DIM`/`UnDim` tracks have base
  x −39.26. From the enter fade on, its children (glow, backing, caption, panel row) sit 0.26 units
  further left than their stored values suggest. `UI_pause.fng` has no such difference.

### A5. Positions written by the application [static; data]

* The application writes positions in the **object's own (parent-relative) frame**. Every object it
  moves in these screens is **top-level**, so in practice these writes are in screen virtual
  coordinates:
  * carousel: `option_1…10`, `ScrollBar_Handle`, `ScrollBar_Arrow_1/2`, `ScrollBar_Backing`;
  * list rows: `OPTION_NAME_n`, `OPTION_DATA_n`, `LEFT_ARROW_n`, `RIGHT_ARROW_n`, all `SLIDER_*_n`;
  * the `OPTION_BACKING_n` **groups**: their children keep their local offsets;
  * the dialog button groups `button1/Button2/button3`.

  [data: parent fields]
* `SPEC_GUI_ART` §3.2 "set centre" on a slot therefore sets the slot's screen centre directly. There
  is no parent or presentation-group offset to add.

### A6. Anchor ("master") read rule (fixes D1) [static; observed]

* `OPTION_MASTER` (carousel packages), and `TITLE_MASTER` and `DATA_MASTER` (list packages), have an
  `Init` script that is automatically followed by a second authored script (key `0x6048A18B`). That
  script makes the master invisible (alpha 0) and **moves it** [data]:

| Package | Master | Authored (stored = `Init`) centre, size | Position after the chain (must NOT be used) |
|---|---|---|---|
| `UI_pause.fng` | `OPTION_MASTER` | (0.38835, −14.29613), 118×59 | (0, 170) |
| `UI_PauseOptionsMain.fng` | `OPTION_MASTER` | (0.39, −14.30), 118×59 | (0, 170) |
| `UI_Options.fng` | `TITLE_MASTER` / `DATA_MASTER` | (−151, −120) 240×36 / (126, −122) 216×32 | (−132, −120) / (109, −122) |
| `UI_PC_Customize_Options.fng` | `TITLE_MASTER` / `DATA_MASTER` | (−152, −120) 240×36 / (126, −122) 200×32 | (−133, −120) / (109, −122) |
| `UI_PauseOptions.fng` | `TITLE_MASTER` / `DATA_MASTER` | (−150.2189, −120.9405) 240×36 / (124.663, −122.9405) 231.18×32 | (−131.22, −123.94) / (107.66, −125.94) |

* **Rule:** the application samples each master's centre and size **once, when the screen is
  created**, before any script of the freshly loaded package has run, so the values sampled are the
  authored stored values in the middle column. They are cached and used for every later layout
  update; later per-frame layout does not consult the master again. [static]
  Implementations must use the middle column, never the post-chain values.
* One retail path re-reads the list masters later: the list "rebuild" performed by the in-game
  Display page after Default Settings → Yes. The carousel never re-reads. Whether the retail engine
  has by then copied the chained values into the stored data, which would offset the rebuilt rows
  by the differences in the table, is unknown (§C7). Implementations should keep using the authored
  values on rebuild [guessed; conservative; not verified by any observation].
* Cross-checks [observed]:
  * The retail pause screenshot has the focused icon frame centred at (0.9, −13.8) and the Options
    neighbour icon centred at (158.4, −14.0) (the authored values give (0.39, −14.30) and
    (158.39, −14.30)).
  * The retail Audio screenshot has the row-1 value arrows at x 6.2 / 246.2. The authored
    `DATA_MASTER` gives 6.0 / 246.0. The chained one would give −11 / 229.
* Consequence: if an implementation evaluates the master through its scripts (current-script value)
  it gets y = 170. The slot centres land at y 170 (922.5 px at 1080p). That is the D1 symptom:
  icons drawn about 184 units too low, with x off by only 0.39 units.

### A7. Resulting carousel geometry (completes SPEC_GUI_ART §3.1/§3.2/§3.4)

* Slot i centre (screen virtual) = (M.x + (i−1)·158 + s, M.y), where M is the cached anchor of A6.
  The focused slot is 118×59. Others are 94.4×47.2 about the **same centre**: the carousel resizes
  and then re-centres vertically. It is a uniform 0.8 scale, so the aspect ratio is unchanged.
  [static] All icon textures are 128×64 with full UV (0,0)–(1,1) [data], so icons are never
  stretched: focused 265.5×132.75 px and neighbours 212.4×106.2 px at 1080p. Same-engine check on
  the retail main menu: neighbours measured 96.0×48.0 and 94.7×47.4, centred on the focused row y
  [observed].
* **Glow and backing.** The glow (`0xDF422345`, 126×62) and the icon backing (`0x687FDA31`,
  126×62) are children of the presentation group `0x8D7E0C53`. The application does not move them.
  * Pause: group (−39, −21) + local (39.2555, 6.9964) gives the glow at **(0.2555, −14.0036)**, and
    the backing (39.26, 6.71) gives (0.26, −14.29).
  * Hub after the enter fade: group (−39.26, −21) gives the glow at **(−0.0045, −14.0036)**.
  * Both coincide with the focused slot centre (0.388/0.39, −14.296/−14.30) within 0.4 units
    (≤ 0.9 px at 1080p). Retail focus-frame line centre measured at pixel (962.0, 509.0).
    [data; observed]
* **Panel icon row (light strip).**
  * Pause: group `0x371F1C93` local (41.29, 7.0817) + presentation group (−39, −21) gives the strip
    `U2_Menu_Box_White` 548×62 centred at (2.29, −13.918): x −271.71…276.29, y −44.918…17.082.
  * Hub after the enter fade: centre (1.74, −14), x −272.26…275.74, y −45…17.

  [data] Retail strip measured y −44.0…+17.6 [observed].
* **Containment check.** At rest every visible slot rectangle lies inside the strip rectangle:
  * focused: y −43.80…15.20;
  * neighbours: y −37.90…9.30, x within M.x ± (158 + 47.2).

  This holds for the explore pause with focus on Return to Explore Mode, Options or Quit, and for
  the hub (vectors).
* **Other carousel objects (all top-level, centre, size):**
  * rail `ScrollBar_Backing` (3, −52), 562×8 (hub 564×8);
  * tab `ScrollBar_Handle`: y −52.2821, x set by the application (§3.2), 16×16 (retail
    (−270.2, −51.6) for N = 4, c = 1);
  * `ScrollBar_Arrow_1` (−250.2907, −12.7695) and `ScrollBar_Arrow_2` (259.1396, −12.7695), 26×26
    (retail right arrow (259.3, −12.7)).
* **Text objects (anchors, children of the presentation group):**
  * `ICON_TITLE`: (41.29, 41) + (−39, −21) = (2.29, 20.0), scale 0.7, centre-justified;
  * pause `Header_text`: (−247.71, −61.29) + (−39, −21) = (−286.71, −82.29), scale 0.75.

  In the hub, x is 0.26 smaller after the enter fade (A4).

### A8. List screens and dialogs: frames of the quoted positions [data]

* `UI_Options`, Customize and `UI_PauseOptions`:
  * Top-level: the masters, all row objects and the scroll objects. The positions in SPEC_GUI §8
    and SPEC_GUI_ART §4.2/§4.5 are screen frame.
  * Header texts are children of the presentation group, and the §8 values are already summed. For
    example, `UI_Options` `HEADER_TEXT` = (−247.71, −63) + (−42, −157) = (−289.71, −220).
  * `OPTION_BACKING_n` is a top-level group. The application sets its origin to (tx + 276, ry + 10)
    (§4.2) and its children are drawn at origin + local offset. Row 1 of `UI_Options`: origin
    (5, −128), box (−17.7, 1) → (−12.7, −127), 500.81×36; caps at (−281, −127) and (255.7, −127),
    36×36.
* Dialogs (`UI_InGameDialog`, `GenericDialog_SMALL`):
  * `button1`, `Button2`, `button3`, the label texts and `message_blurb` are top-level, so the
    centres in SPEC_GUI_ART §6 are screen frame.
  * The button art is inside each button group at local (0, 0) (box 214×42) and (±128, 0) (caps
    42×42).
  * The panels, border and the full-screen dim quad `0x014245E2` (`U2_Menu_Box`, 1070×650, local
    (43, 124) in group (−43, −124), so centred at (0, 0)) are group children.
  * Group alpha multiplies into all children (A3). Example: a button box drawn alpha =
    200 ⊗ group alpha.

## B. Answers to the verifier's other observations

### B1. Dialog default focus: which look is "focused" [data; static; observed]

* The focus is on the **second button (Cancel / No)** (SPEC_GUI_ART §6, unchanged).
* The focused button looks **darker**:
  * Its group alpha is 225 (Highlight), or 255 at the end of its enter fade. The 200-alpha dark
    `U2_Menu_Box_Layer1` therefore covers the light dialog panel almost fully (drawn box alpha ≈ 176
    to 200).
  * Its label is **lime** (204, 244, 138). It is either steady (`idle`) or pulsing with the 1200 ms
    looping `Highlight` (alpha 0 → 255 by 440 ms, held to 760 ms, → 0 at 1200 ms).
* The unfocused first button (OK / Yes) looks **lighter**:
  * Group alpha 80: the enter fade ends at 255 − 175 = 80, and UnHighlight is also 80. Drawn box
    alpha ≈ 63, so the light panel shows through.
  * Its label is light grey: (225, 225, 225) after its fade-in (`idle`), or (192, 192, 192) after
    UnHighlight. In `GenericDialog_SMALL` the stored first-label colour is (150, 150, 150) before
    the fade-in.
* Retail frontend quit dialog [observed]: the right button "Cancel" has a darker box and an accent
  label (at the pulse trough in the shot). The left button "Quit Game" has a lighter box and a grey
  label.
* **Conclusion:** "OK bright, Cancel dim" is the correct retail look of *Cancel focused*, provided
  that:
  * the "bright" OK is a lighter box with a grey or white label, and
  * the "dim" Cancel is a darker box with a lime label.

  It is a defect only if OK's label is lime, or Cancel's box is the light one. Testable at 1080p
  (tvg12 `dialog_button1`/`dialog_button2` rectangles): the mean luminance inside the Cancel box
  rectangle is lower than inside the OK box rectangle, and the Cancel label ink is green-dominant
  (G > R > B).
* Which of the two lime states the second label settles in depends on whether the focus is set
  before or after its 180 ms `Init` fade ends. If its current script is already `idle`, the label's
  Button_Highlight response does not start `Highlight`. This order is not determined (§C4). Either
  state is acceptable.

### B2. Pause screen (caption, icons) under a confirmation dialog: correction to SPEC_GUI_ART §10 item 2 [static; inferred]

This is the sequence for activation of an **immediate** entry (Quit, Restart, Return to Explore Mode
from a race, Return to World Map), all within one input event:

1. The carousel activates the entry. The entry opens `UI_InGameDialog.fng` and synchronously sends
   the pause screen the "start dim" request. That request sets the pause handler's current script
   to `DIM` and disables the edge fade.
2. The pause screen then starts its exit script `FORWARD` on the **same handler object**, unless it
   is already running. This replaces `DIM` as the handler's current script.

Whatever the order in which the two scripts' time-0 events fire, the last message the targets
receive is `0xAADD0187`. So the presentation group (header, **caption `ICON_TITLE`**, panel, light
strip, glow, backing), the slots, the rail, the tab and the arrows run `FadeOut`/`fadeout`, and
reach **alpha 0** after 100 ms (arrows 140 ms).

While the dialog is open, the pause caption and icons are therefore **not visible**: only the game
scene under the dialog's own full-screen dim quad remains.

On Cancel/Back the application sends "stop dim" (§1.4 `UNDIM`). The targets run `UnDim`, whose keys
start at alpha 60 and reach 255 at 100 ms. The pause screen reappears in 100 ms and the edge fade is
enabled.

`FORWARD`'s exit-complete at 320 ms has no pending action for immediate entries, so the pause
screen stays.

* So a caption or icons "faintly visible" under the retail pause dialog is **not expected**
  [inferred]. Capture request 2 of SPEC_GUI_ART §11 still stands to confirm it.
* The caption's text at that moment is the label of the activated entry. For example, it is "Quit
  to Main Menu" after Right ×3 in career free roam: activation does not change `ICON_TITLE`
  [static]. A caption reading "Return to Explore Mode" while Quit was activated would be a separate
  divergence, if confirmed.
* The frontend main-menu dialog in the retail shots leaves its caption dimly visible. That is a
  different package and handler and does not bear on the pause case.

### B3. Hub "Display" neighbour icon [data; static]

Neighbour = 94.4×47.2 units, uniform 0.8 of 118×59, same 2:1 aspect as the 128×64 texture
`OPTIONS_ICON_DISPLAY` (`0x8A006328`), full UV, no rotation.

At 1920×1080 with focus on Audio, the Display icon is a 212.4×106.2 px quad centred at
(1316.4, 507.8). At 1280×720 it is 141.6×70.8 centred at (877.6, 338.6).

Any non-uniform stretch or skew of the quad is a defect. The art inside the texture is what it is,
and its ink is not 2:1.

### B4. Customize first focused row and panel tone [static; data]

* "Car Reflection Update Rate" is the expected first focused row. It is Customize row 1 (SPEC_GUI
  §4.1), present whenever the CarEnvironmentMap capability is non-zero, and the screen focuses the
  first selectable row (SPEC_GUI_ART §4.7). If that capability is 0, the first focused row is "Car
  Reflection Detail" (or the next present row).
* The lighter panel is expected. `UI_PC_Customize_Options.fng` is the same frontend-style package
  in game and in the front end: outer `U2_Menu_Box` with a white inner panel at alpha 200
  (SPEC_GUI §8). The in-game Display page (`UI_PauseOptions.fng`) uses a black panel with a lighter
  inner panel at alpha 100.

## C. Unknowns / ambiguities

1. Retail mapping when the output is not 4:3 and no widescreen patch is active (stretch vs
   pillarbox). The reference shots are pillarboxed, but the user's install has modified files
   (SPEC §14.3). The `min` rule below 4:3 is guessed and untested.
2. Retail order of group scale and rotation composition (A3) is inferred from FEngLib. It is not
   exercised by anything these screens draw.
3. Sub-pixel conventions: pixel-centre offset, texel alignment and text baseline rounding.
4. Whether the second dialog label rests in steady lime (`idle`) or pulses (`Highlight`) (B1).
5. B2 is static inference about message ordering [inferred]. The resulting alpha 0 of the caption and
   icons under the pause dialogs is not observed; treat the alpha there as [guessed] until capture
   request 2 is answered.
6. Draw order of overlapping objects (z ordering) is not covered by this amendment.
7. Whether retail script evaluation overwrites an object's stored data, which matters only for the
   list-master re-read after the in-game Default Settings rebuild (A6). Capture request: in game,
   Display → Default Settings → Yes, and compare the row positions with those before.

## D. Test vector

`test-vectors-gui/tvg12_frames.json` (synthetic). It contains:

* the mapping of A1 at 1920×1080, 1280×720 and 640×480;
* the anchor read rule, with the forbidden post-chain value;
* composition cases for the glow, backing, panel strip, caption and header anchors, rail, tab,
  arrows, dialog dim quad and dialog buttons, plus one group-scale example tagged inferred;
* the `UI_Options` row-1 arrows from the pre-chain masters;
* carousel states:
  * pause N = 4 with c = 1 (Return to Explore Mode), c = 2 (Options) and c = 4 (Quit, Replay
    disabled);
  * hub c = 1 and c = 2;
  * one hub tween frame at 0.15 s.

  For every visible slot, the vector gives the screen-pixel centre and size at both required
  resolutions and an `inside_panel_row` flag (at rest).

Retail cross-check values are included where measured and are informative (±2.5 px). How the vector
was obtained: authored numbers read from the retail packages, rules A1–A7 applied by an analyst-only
generator (not distributed). The vector file carries its own provenance field and is listed in
`test-vectors-gui/README.md`.

Verifier's D1.2 reference (1920×1080): option_1 (960.874, 507.834), 265.5×132.75; option_2
(1316.374, 507.834), 212.4×106.2; option_3/4 hidden; glow (960.575, 508.492). At 1280×720: option_1
(640.583, 338.556), 177×88.5; option_2 (877.583, 338.556), 141.6×70.8; glow (640.383, 338.995).
