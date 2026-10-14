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

## Implementer questions (2026-10-14, authored-art pass, SPEC_GUI_ART.md)

Items 3, 7, 9 and 12 of the previous GUI list are answered by SPEC_GUI_ART (dialog focus/no wrap: 6;
in-game focus: 4.7; the pause Options entry and hub: 2.2, 2.5; visuals: authored packages, 8). The old
text above is kept as history. New points, each implemented as stated and each easy to change:

1. **Conflict with earlier user decisions (BEHAVIOR R1.5-R1.7, R5; QUIT_DIALOG Q4/Q6).** SPEC_GUI_ART
   2.4/3.2/6 replaces the pause screen's exponential slide with a 0.3 s cubic Hermite scroll, makes
   Return to Explore Mode / Back / Start resume only at exit-complete (320 ms), and replaces the
   Yes/No quit dialog (`IG_GenericDialog_SMALL`) by `UI_InGameDialog` OK/Cancel. Implemented as the
   approved SPEC_GUI_ART says. The earlier vectors in `tools/pause_menu_vectors` that state the old
   rules are skipped by name in the checkers (list in IMPL_DESIGN_GUI.md); `quit_dialog.json` and its
   checker were removed. Confirm the user wants the SPEC_GUI_ART behaviour (it came from a static
   reading of the same executable, the older rules from the user's observation).
2. **Pause exit script when an immediate entry opens a dialog (2.4, 10.2).** The spec says the exit
   script is also started and that the visual result together with the dim is unknown. Starting it
   would fade the whole pause screen out with no way back after Cancel. Implemented: DIM only (and
   UNDIM on cancel); the exit script is started only by the "after exit" entries, Back and Start.
3. **Audio / Gameplay / Player hub entries.** The hub shows all four entries (2.5) but only Display has
   a screen in this project: activating the others does nothing (no exit, stays on the hub). What the
   other pages' rows are is outside SPEC_GUI / SPEC_GUI_ART.
4. **Start on Display (5.2, 2.5).** Only the hub's Start is specified as "exit, then close and resume".
   Start on the in-game Display screen now also runs the 320 ms exit first (no registry write).
5. **Dialog result timing (6).** "Plays the dialog's exit (140 ms) and delivers the button's message":
   implemented as delivery when the exit completes, for OK, Cancel, Back and the pointer alike.
6. **Online texts (2.4).** `session_test` is taken as "session state > 2" (inferred) and the "extra online
   flag" is a plain input (`PauseContext.online_flag`); their real meaning is unknown (10.3).
7. **Scroll retarget (3.2).** "Start and end velocity 0 when starting from rest" says nothing about a
   move that starts while the scroll is still running. Implemented: the new Hermite segment starts at the
   current offset with the current velocity (no jump, no stall).
8. **Host context (2.2, 2.3).** The game host passes the career free-roam context (race 0, replay data
   absent, Options available): it has no race lifecycle hooks (restart, leave race, world map) and no
   replay recorder. The catalogue, dialogs and actions for race contexts exist and are tested; the
   adapter reports them as `FE_ACTION_RESTART/EXPLORE/WORLD_MAP/REPLAY` and `main.c` resumes the game
   if one ever arrives.
9. **Pointer on dialogs and rows (4.6, 6).** Dialog buttons use the authored extents (6); hover does not
   change the focus (unspecified). A button row's "name object" rectangle is taken as the label column
   (TITLE_MASTER x range, row height) because text extents are not specified; arrow boxes use 24 x 24
   (the authored size is read from the package for drawing, the constant only serves hit tests).
10. **Row slots without a list row (4.2).** Objects of unused slots (e.g. slots 5-11 on Display) are hidden
    and parked at y = 6969 like rows outside the window; the spec only names the latter.
11. **Highlight timing (4.4, SPEC 3.6.2).** Highlight/UnHighlight scripts of the rows start after the
    package's enter-complete message (or 600 ms) so the enter fade is not cut; before that the name object
    is already the scene focus. Arrows and backing of a row that lost the focus stay visible 150 ms while
    UnHighlight plays and are hidden afterwards.
12. **`--pause-reference`.** The separately coded reference evaluator of the old pause is gone: the
    reference now replays on the production adapter, because both evaluate the same authored packages.
13. **SLIDER_VALUE_n placement (4.1).** Absent in all three packages; if a package had one its text would
    be the integer value at its authored position.
14. **AMEND1 B2, dialog opened while the exit script already runs.** B2 says the exit script is started
    "unless it is already running" but does not say whether DIM is still sent first in that case (which
    would replace the running exit script as the handler's current script). Implemented: if the exit script
    is already the handler's current script, opening a dialog changes nothing. Only reachable when a
    dialog-opening request arrives during a running exit (a second request in the same fade).
15. **AMEND1 A6, anchor sampling for a master inside a group.** No covered package nests a master. Implemented:
    top-level masters use their stored position; a nested one would use its composed centre at creation.
