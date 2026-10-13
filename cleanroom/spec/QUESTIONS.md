# Implementer questions (spec gaps / ambiguities)

Raised by the implementer while building `src/ugui/` against SPEC.md
(sha256 92c9026fe6747f4272a2c6086a744c42c7eee649b3d0c64df51ade8a8f9924b8).
Each item says what was implemented meanwhile; none blocks the demo.

1. **SPEC 3.5.2 / 3.5.3 quaternion deltas.** "base composed with the
   interpolated delta rotation" does not give the multiplication order or the
   interpolation (nlerp vs slerp). Implemented `result = base * normalize(lerp(delta))`
   (Hamilton product, shortest-path not enforced). No observed data uses it.
2. **SPEC 1.4 colour product.** `c = (a*b + 128) >> 8` maps 255 x 255 to 254, so a
   fully opaque child under a fully opaque parent is 1 level dimmer. Is that
   the intended behaviour (vs `/255`)? Implemented exactly as written
   (`ug_color_mul`) for scene objects.
3. **SPEC 3.5.4 events on loop wrap.** The test-vector README has a looping script
   with an event at t = length (400). Implemented: events in `(prev, length]` fire,
   then the loop restarts and events in `[0, t']` fire, i.e. an event at time 0
   fires again on every restart. Confirm (or say a restart must not re-fire t=0).
4. **SPEC 3.5.1 value sizes.** For bool/int tracks only "value size" from the track
   header is given. Implemented: stored as 32-bit integers; any size smaller than
   `components x 4` is rejected.
5. **SPEC 3.2 compressed packages.** `JDLZ` is "decoder already in the repository"
   but only `tools/jdlz.py` exists (no C decoder). The C reader refuses `JDLZ`/`HUFF`
   payloads with an error message. Should this library carry its own C decoder?
6. **SPEC 7.1 / 7.6 hint strip geometry.** A 385-wide dark box (right) plus a 167-wide
   grey box (left) do not cover the stated span -309..+310 (619). Also the area
   limit of 385 and the primary slot at x = -102 disagree about the left bound of the
   hint area. Implemented: one dark strip across the whole span, grey box on the
   left, hint area bounded at x >= -110. Long default labels ("Change Selection")
   often do not fit, so the key half of a glyph pair is dropped first, then the
   lowest-priority hints (not in the spec).
7. **SPEC 6.11 stat bars.** Track texture 256 x 32 with bars spaced 200 apart would
   overlap. Implemented a 180 wide track.
8. **SPEC 6.9 dialog sizes.** Only the large panel (562 x 198) is given. Small and
   medium are 380 x 150 and 470 x 176 in this implementation; button width is
   derived per button count (214 for one/two buttons on the large panel).
9. **SPEC 6.5 / 4.5 Accept on value rows.** Accept on a selector or toggle row is not
   specified. Implemented: cycles to the next value (clicking the value column
   does the same); Accept on a slider row does nothing.
10. **SPEC 6.13 grid pitch.** Swatches 16 x 16 at 14 pitch overlap and grow to 24.
    Demo uses pitch 22; the library takes pitch as a parameter.
11. **SPEC 8.4 justification.** Only the values 0,1,2,4,5,6,8,10 are observed. For
    combined/odd values (e.g. 3 = centre|right, 12 = vcenter|bottom) the bits are
    tested independently with CENTER taking precedence over RIGHT and VCENTER over
    BOTTOM. Is there a defined precedence?
12. **SPEC 7.3 BUTTON0..3 glyphs.** The key-cap letter bound to each extra button
    is unknown (7.5 says per screen). Defaults Q, C, D, R; the library lets
    hints supply any letter.
13. **SPEC 4.4 dispatch.** When the focused button *and* the package both respond to
    a message, both run (button first). Command 0x001 with target 0 re-enters the
    same dispatch (depth-limited to 8). Confirm this is the intended reading.
14. **SPEC 8.1 font.** The spec asks for a free condensed italic substitute; an
    original 5x7 pixel font with a 0.2 shear was drawn instead (no kerning, digits
    advance 14.7 instead of 13). Acceptable?
15. **SPEC 1.3 equal-z order.** Left at "list order, earlier first" (G).
16. **SPEC 6.4 / 6.5 numbers.** Icon spacing 128, 4 visible icons (wrap on when more
    items than that), 9 visible option rows, row pitch 32, key-repeat 400/110 ms are
    the spec's G values and were used as given.

---

# Round 2 questions (original assets at run time)

Raised while building the loader and the "Original screens" page against SPEC.md sha256
3a6b4b3f25c3bdfe20337f44f2193e01c43a6ed9bdbbb2a2100f779d0ed8cd38 and AMENDMENTS.md sha256
102c07daf3d1ba9211c7a2255978ce939e498dcda84abc0f8d2b97a84c99ac15. Answers to Q1-Q16 above were
applied (DESIGN.md section 7). Each item says what was implemented meanwhile; the isolated
constants are in `src/ugui/ugui_tuning.h`.

17. **SPEC 7.1 grey player box vs the data.** The strip's left box carries colour (128,128,128) but its
    texture is the *black* 60% box, so texel x colour renders nearly black, not grey. Is the retail
    box really dark, or does the original combine colour and texture differently (additive tint,
    colour-key)? Implemented: original screens and the textured kit follow the data (modulate, dark);
    only the procedural fallback draws a grey fill. Screenshot request A1.
18. **SPEC 7.1 cap geometry.** The 32 x 32 strip end-cap texture is opaque only over its inner ~21
    columns (flat edge on the left, arc on the right); the cap object spans x -309..-277 and the grey
    box starts at -277.1. Implemented: the left cap is mirrored so the flat edge meets the box and the
    outer ~11 units stay clear (the visible strip starts near x -298). Confirm with a capture.
19. **Console strip on PC (3.4.2, 7.1, 7.4).** In the screen packages the strip group is console-only
    (hidden on PC) and the PC help bar package carries its own, differently scaled strip and the pill grid.
    Implemented: keep the console strip's non-text pieces visible in the viewer (the strip is then the data's
    art) and replace its text by the mouse hint bar. How does the PC build position the PC help bar's
    strip (its group sizes 430 x 51 / 132 x 51 at y about 215) relative to the 32-high console strip, and does the
    PC show both? Which is correct for A1?
20. **Icon-scroller content (6.4, 15.9, Unknown 10).** The packages hold a pool of off-screen item
    buttons (about 108 x 44, alpha 120, placeholder texture) plus a 118 x 59 placeholder slot and an
    "Option title" text. Implemented (G): the slot shows the selected item's icon, pool items show the
    neighbours at 118 x 59 / spacing 128 / alpha 150 fading out beyond 1 item, labels from `UI_MAIN_*`
    (else the icon name). Are neighbours drawn 118 x 59 or 108 x 44, at what alpha, and does the thumb
    follow `index/(N-1)` over the 564-wide track as the spec says? (A1, A2.)
21. **Dialog packages (6.9).** `GenericDialog_*` carry several button sets and layers (one, two, three
    buttons overlap in the data). Which messages / scripts select the layout the program wants, and what
    are the message hashes to set title and body text? Implemented: all sets are shown as authored.
22. **Placeholders.** Retail screens contain placeholder text ("Option title", "option_name", "WWWWWWWW",
    "PLAYER NAME") and placeholder images (untextured white slots, debug icons) that the program overwrites.
    Implemented: show them as authored except where the icon-scroller binding supplies content and the
    bottom-strip band text (hidden, replaced by the mouse hints). Is there a data-level rule (flag, buffer
    length) that tells "overwritten by the program" objects apart?
23. **Pointer cursor texture (4.6).** The spec says a 32 x 32 cursor texture exists but gives no key. Can
    the key (or the texture name) be given? Implemented: the procedural cursor.
24. **P8 palette byte order (15.7, Unknown 14).** Still no frontend evidence; implemented B,G,R,A as in
    the synthetic vector. Any frontend texture that proves R,G,B order would settle it.
25. **Highlight frame art (5).** The 8 x 8 line and 16 x 16 corner textures exist, but their orientation
    and which edge each object uses are not given; implemented: the procedural frame everywhere except the
    icon highlight (128 x 64 texture, tinted by the breathe animation).
26. **Tab/filter screens.** The "Menus / Dialogs / Help / Other" tabs of the browser are filename heuristics of
    this demo, not data (no question for the spec, listed so it is not mistaken for one).
27. **SPEC 8.3 / 17 string table of the Korean file.** `LANGUAGES/Korean.bin`'s table has a code-map length
    other than 256 (the reader reports "bad code map length"; the other eight language files load). How are
    Korean strings encoded (header variant, 16-bit units, no code map)? Implemented: the table is rejected
    with a warning and screens fall back to their stored default text; nothing is guessed.

---

# Implementer status note, round 3 (against SPEC sha256 49d49bf0... and AMENDMENTS sha256 47ff48f4...)

Status of the earlier entries after amendment 3 (analyst sections above are unchanged):

* Q16 closed: icon spacing 158 / neighbours 96 x 48 / 3 visible, row pitch 36, 8 (plain) / 7 (controls) visible rows, no wrap
  at the ends of icon scrollers and the car carousel (`ugui_tuning.h`, "SPEC-FIXED GEOMETRY"). Key repeat stays G.
* Q17 closed: the player box is drawn dark (black, more opaque than the hint box) in the PC help bar and in the
  library's mouse strip, also in the procedural fallback.
* Q18/Q19 closed: on PC only the help bar's own 51-high strip (y 189.2 ..) is drawn; the console strip is off
  (`UG_TUNE_SHOW_CONSOLE_STRIP` 0).
* Q20 closed: neighbours 96 x 48 at +-158, full opacity, thumb formula -273.7 + 547.4 i/(N-1).
* Q21 still open for the viewer: which message selects a dialog layout is application logic; the viewer shows the two-button layout.
* Q22 unchanged (placeholders shown as authored; the bound icon strip replaces its title text).
* Q23 closed: skin role `cursor` = texture hash("PC_CURSOR"), drawn 32 x 32 virtual units; procedural fallback fits the same cell.
* Q24, Q27 still open (no evidence / not covered).
* Q25 partly closed: icon frame line centres and the dialog border follow the measured rectangles; per-object corner orientation unchanged.

New questions:

28. **Test vector vs amended 16.5.** `font_layout_vectors.json` case `AV^VA` places the second line one line-height byte
    (20) below the first, but 16.5 (amendment 3) says the pitch is the cell height (21 in the synthetic font). The library
    follows the amended rule; the test compares that vector with the known +1 shift. Please regenerate the vector.
29. **Slider backing art offset.** The slider backing texture's bar is not vertically centred in its quad; the library
    centres the bar on the fill (measured on the decoded texture at load, `UgSkin.slider_bg_dy`). Is the fill really centred on the bar in the original?
30. **PC controls page and car select (checklist 27, 28, 29, 30).** Only the listed measurements are given; header band width,
    column heading position, the vertical scroller position on the controls page and the left carousel arrow are guesses (G, noted in the code).
31. **Main menu items 3 and 4 and the profile-management label.** Order used: career, quick race, go online, LAN, customize car,
    profile management, options. The label key for profile management is not in the spec (several spellings are tried, else "Profile Management").

32. **Amendment 5 localized PC navigation label keys.** Section 20.7 requires original localized navigation labels, but the approved spec supplies English functional values without the original string-table lookup keys for Quit Game, Continue, Back and Reset Keys To Default. Please provide reviewed lookup keys (and any language-specific selection rules). This round restores the approved English baseline labels and preserves caller-supplied `pc_label` overrides; automatic navigation-string localization remains unimplemented rather than guessing keys.
