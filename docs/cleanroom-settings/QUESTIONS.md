# Settings spec — questions

Implementer/reviewer questions go here. The analyst answers by amending SPEC.md.

## Analyst open questions (2026-10-14)

These need a runtime session with the retail game (see SPEC §12); static examination could not
settle them.

1. EA TRAX playlist bytes in a profile that never opened the EA TRAX screen (SPEC §5.6, §11.1).
2. Defaults of the online flags and the meaning of the two five-word online arrays (§5.5).
3. Meaning of the unidentified audio, gameplay and per-player fields (§5.2–§5.4) and of the
   16-byte unknown block (§5.9).
4. Input binding table: action-slot order and default codes (§6).
5. Career difficulty default and whether the option is reachable in retail menus (§5.7).
6. Language registry string that matches nothing (index −1) (§2.4).
7. Case sensitivity of the `/s` safe-mode switch (§4.6).
8. Which save points trigger the autosave/prompt check (§5.8).

## Implementer questions (2026-10-14, settings backend)

Each item was implemented with the stated interim choice; none blocks the vectors.

1. **Defaults of unknown fields (SPEC 5.5, 5.6, 5.8, 11.1/11.2).** `st_profile_defaults` uses
   exactly the synthetic-vector placeholders (EA TRAX identity order with mode 3, autosave 1 /
   prompt 0, Show Player Names 1, Bad Connection Warning 1) so that defaults + name reproduce
   tv03 byte for byte. These are guesses; real defaults need the capture in SPEC 12.1.
2. **EA TRAX masks (SPEC 5.6).** "Two 27-bit masks derived from the modes": is bit *n* indexed by
   track id (implemented) or by playlist position?
3. **Auto-detect and `g_PerformanceLevel` (SPEC 4.2).** The text does not say whether auto-detect
   stores P into `g_PerformanceLevel`. Implemented: yes (otherwise the slider would show a stale level).
4. **Preset columns outside 0..5 (SPEC 4.2).** Implemented: level <= 1 uses the "P <= 1" column,
   level >= 5 uses the "P = 5" column. Registry values can hold anything; what does the original do?
5. **Safe-mode switch case (SPEC 4.6, analyst Q7).** Implemented case-insensitive (`/s` and `/S`).
6. **Container with trailing slack (SPEC 3.5 step 4).** If header `total` equals the file length but
   sizeA + sizeB < total - 28, step 4 reads succeed. Implemented as accepted (extra bytes ignored,
   checksums cover only the two blocks); sizeA + sizeB > total - 28 is an I/O error. Confirm.
7. **Block B shorter/longer than 54 930 (SPEC 3.5 step 6).** The game-level check only mentions the
   size field inside block A. Implemented: actual block B size != 54 930 is also "incompatible".
8. **Autosave/prompt byte values (SPEC 5.8).** Only 0 and 1 are described; implemented as
   "non-zero = set" for display, writes are always 0/1.
9. **Normalisation on write.** Online offsets 0 and 4 are always written as 0 ("always written 0,
   ignored on read"), and the profile-name field is NUL-padded; bytes after the NUL terminator of a
   loaded name are not preserved. All other bytes (including padding) are preserved. Confirm that no
   retail save relies on non-zero bytes there.

## Implementer questions (2026-10-14, graphics options GUI, SPEC_GUI.md)

Each item was implemented with the stated interim choice; none blocks the vectors
(`make options-menu-test`). Design log: `IMPL_DESIGN_GUI.md`.

1. **Length of the Display exit animation (SPEC_GUI 5.1, 11.7).** The `Forward` script duration of
   the event-handler object is not given. Implemented as `OmConfig.exit_ms`, default 140 ms (the
   frontend `FadeOut` figure of 2.4). Only ordering matters to the vectors.
2. **Exit animation on Customize -> Back, and input while an exit animation runs (5.1, 5.3).** 5.3 names
   the animation only for "Customize Display Settings". Implemented: Back from Customize uses the same
   animation, and every input (not only a second Back) is ignored while it runs. Confirm.
3. **Default Settings dialog input (6, 11.5).** Left/Right between Yes and No: implemented *clamped*
   (Left selects Yes, Right selects No, no wrap). Up/Down/Start are ignored (as in the pause quit
   dialog). No pointer support in the dialog: button extents and y of `GenericDialog_SMALL.fng` are
   not in the spec (only the x centres -150/+150).
4. **Pointer on rows (2.7, 11.2, 11.10).** Implemented: click on the focused row's left arrow = Left;
   right arrow or the value text of a choice row = Right; click on the label area of any visible
   *button* row focuses it and activates it (inferred button flag). A click on an unfocused
   non-button row, the mouse wheel, hover-to-focus and scrollbar clicks do nothing. Arrow hit box
   28 units wide (visible disc is ~21.5), value-text region = between the arrows. All guesses.
5. **Spacer when row 1 is absent (4.1).** The spacer is "inserted directly after row 1"; if the
   Car Reflection Update Rate row is omitted (cap 0) no spacer is inserted. Confirm.
6. **Slider domains and unknown caps (4.1, SPEC 4.3).** Slider max = min(cap, domain) with domains
   Car Reflection Update Rate 3, Car Shadow 2, World Detail 3, Road Reflection 3, Texture Filtering
   2; FSAA has no domain, so its max is the cap. A cap of `ST_NO_CAP` ("no limit", no hardware
   information) shows toggles and bounded sliders but **hides the FSAA row** (unbounded range).
   The game host passes `st_gfx_caps_full(&caps, 1)` (FSAA cap 0) until the renderer reports
   multisample levels.
7. **In-game Display initial focus (2.5, 11.4).** `om_open` takes a `remembered_focus` (1-based,
   0 = none) used only in game. The pause adapter always passes 0, and every rebuild (return from
   Customize, Default Settings) focuses row 1. What the remembered index is remains unknown.
8. **Start (5.2).** Only specified for the in-game Display screen. Start in the front end, on
   Customize and inside the dialog is ignored.
9. **Pause row (1.2, 7).** The in-game route "pause -> Options hub -> Display" has no hub screen in
   this project; the pause row gains an `Options` entry (Resume / Options / Quit Game) and Back from
   Display returns to that row. There is no authored icon for the Options entry (the row shows the
   caption only). Which authored art (if any) exists for it is not specified.
10. **Level Of Detail and resolution on modern hardware (SPEC 4.2 table, SPEC_GUI 3.1).** With the
    recommended GPU score 5 the P = 4/5 presets set the resolution to index 1 (800x600) and
    Default Settings resets it to 0 first. That is what the table says; confirm the intent, since on
    a modern GPU it lowers the resolution when the slider reaches 4. Score and CPU MHz are
    `OmConfig` fields (default 5.0 / 3000 MHz, P = 4).
11. **Registry write failure (5.1).** The write status is kept (`registry_writes`, `last_write_status`)
    but nothing is shown and the screen is left regardless; the spec is silent.
12. **Visuals (2.3, 2.4, 3.2, 3.3, 8).** v1 draws the screens natively from the measured numbers
    (row pitch 36, label/arrow/slider x, title anchors, scrollbar) with plain ugui primitives, not by
    evaluating the authored packages, scripts and textures; the blink/pulse/fade tracks are not
    reproduced. Package names and keys are exposed for a later loader.
