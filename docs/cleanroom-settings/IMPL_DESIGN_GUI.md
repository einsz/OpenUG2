# Pause menu and graphics options screens -- implementation design log

Implements, from the approved specs only: `SPEC_GUI.md` (sha256 `b62da2f0...b2b5`, the amended
version approved in `REVIEW.md`), `SPEC_GUI_ART.md` (`33e70665...164b`) and `SPEC.md` (`e79fe5fa...c8ef`),
with the vectors `test-vectors-gui/` (tvg01-tvg11) and `test-vectors/`. Nothing was derived from outside
those documents, from the retail data or from analyst notes. Baseline code that was built on:
`src/ugui/`, `src/pause_menu.*`, `src/options_menu.*`, `src/settings.*`, `src/frontend/`.

This revision (v2) **replaces the native-drawn v1**. The screens are the retail authored packages,
loaded at run time and evaluated by ugui; the state machines stay GL-free and are tested without any
retail data. It also corrects v1 statements that the approved corrections superseded (see "Corrections
to v1").

Screens covered: the pause screen (`UI_pause.fng`), the pause options hub (`UI_PauseOptionsMain.fng`),
Display (`UI_PauseOptions.fng` in game, `UI_Options.fng` front end), Customize
(`UI_PC_Customize_Options.fng`), the confirmation dialogs (`UI_InGameDialog.fng`, `GenericDialog_SMALL.fng`).

## Architecture

```
SDL events / replay ---> pause_menu.c (adapter) ---+--> pause_screen.c   pause entries, carousel, dialog, exits
                              |                    +--> options_menu.c   hub, Display, Customize, Default Settings
                              |                           (+ gui_dialog, gui_carousel, gui_layout, gui_timing)
                              +--> gui_view.c: GvScreen/GvCarousel/GvList/GvDialog
                                     binds authored objects by name hash, writes state with the base-key rule
                                     (ug_scene_app_write) and lets the authored scripts animate; draws via UgScene
                              +--> ugui_gl: renders the canvas (as before)
```

State machines (no GL, no SDL, no drawing, no package access):

| Module | Role | Spec |
|---|---|---|
| `gui_timing.[ch]` | enter/exit scripts and event times per package, exit-complete ms | ART 1.4, tvg11 |
| `pause_entries.[ch]` | entry catalogue per game context (icon keys, label keys, order, action, timing, replay gate) | ART 2.2-2.4, tvg07 |
| `gui_carousel.[ch]` | slot centres/sizes/visibility, Hermite scroll, edge fade, tab x, arrows, pointer | ART 3, tvg08 |
| `gui_dialog.[ch]` | two-button dialogs: texts per context, focus on the second button, no wrap, result after the 140 ms exit | ART 2.4, 6, tvg10, tvg06 |
| `gui_layout.[ch]` | row placement, slider geometry, vertical scrollbar tab | ART 4.2, 4.3, 4.5, tvg09 |
| `pause_screen.[ch]` | pause screen flow: focus, activation, exit then action, immediate actions, dialogs, creation-frame guard | ART 2 |
| `options_menu.[ch]` | hub (Audio/Display/Gameplay/Player), Display (4 rows), Customize (<= 22 rows + spacer), Default Settings dialog, leaving transitions, apply/re-init hook | SPEC_GUI, ART 2.5, 5, 6 |

Presentation (`gui_view.[ch]`, GL-free; uses `ugui_scene`): a `GvScreen` owns a parsed package and its `UgScene`
(PC build, console-only objects not drawn, hint text hidden). Binders: `GvCarousel` (option_k slots,
OPTION_MASTER, ICON_TITLE, header, rail/tab/arrows), `GvList` (row slots n: OPTION_NAME/DATA, LEFT/RIGHT_ARROW,
OPTION_BACKING, SLIDER_*; TITLE_MASTER/DATA_MASTER anchors; header(s); scrollbar), `GvDialog` (message_blurb,
button1/Button2/button3, labels). `ugui_scene` gained `ug_scene_app_write` (+ `app_set_world_pos/size/alpha`,
`start_script`, `find_name`): the base-key rule of ART 1.2, on a private per-node copy of the script track tables.

Adapter (`pause_menu.c`): maps SDL/controller/pointer to semantic inputs, advances time, switches the loaded
package when the state machine changes screen (at exit-complete), manages the dialog overlay, draws, and keeps
the PC Back/Continue pills of the pause screen (earlier pause work). Test seams: `ps_open_entries`,
`gv_screen_attach`, `gv_dialog_attach`; production loads packages from `UgAssets` only.

Fail closed: a missing package, unresolved object, ambiguous name, missing row-arrow texture, missing font or
image makes that screen (or only the dialog picture) unavailable: it is logged once and not drawn; input and
results keep working. There is no native fallback drawing anywhere (ART 8.5). A missing icon omits that slot.

## Files

| File | Role |
|---|---|
| `src/gui_timing.*`, `src/pause_entries.*`, `src/gui_carousel.*`, `src/gui_dialog.*`, `src/gui_layout.*` | pure building blocks (above) |
| `src/pause_screen.*`, `src/options_menu.*`, `src/settings.*` | state machines and settings backend |
| `src/gui_view.*` | authored presentation |
| `src/ugui/ugui_scene.*` | + app property writes (base-key rule), script start, find by name |
| `src/pause_menu.*`, `src/pause_menu_layout.h`, `src/pause_menu_binding.h` | adapter, window transform and pill regions, identity table of `UI_pause.fng` |
| `src/frontend/frontend.h` | `FeAction` gained REPLAY, RESTART, EXPLORE, WORLD_MAP (pause actions) |
| `src/main.c` | one `pause_menu_build()` used by the game path and `--pause-shot`; resumes on any pause action |
| `tools/gui_art_test.c` (+ `test_json.h`) | tvg07-tvg11 and the pause screen state machine |
| `tools/options_menu_test.c` | tvg01-tvg06, the options state machine, hub |
| `tools/gui_frames_test.c` | tvg12 (AMEND1): mapping, composition, anchor read rule, carousel states, list masters, dialogs, B2-B4 |
| `tools/gui_view_test.c` (+ `synth_pkgs.h`, `synth_fng.h`) | synthetic packages: app writes, carousel, list, dialog binders |
| `tools/pause_menu_test.c` + `.inc` + `pause_menu_*_check.py` | the adapter end to end on serialized synthetic packages; earlier pause vectors |
| `Makefile` | `gui-art-test`, `gui-view-test`, `gui-frames-test`, `options-menu-test`, `pause-menu-test`, `gui-test` |

Removed: `src/pause_menu_quit.h`, `tools/pause_menu_quit_test.inc`, `tools/pause_menu_quit_check.py`,
`tools/pause_menu_vectors/quit_dialog.json` (the Yes/No quit dialog is superseded, ART 2.4/6/9.3).

## Flow and timing

```
pause screen --Options (exit 320 ms)--> hub --Display (exit 320)--> Display --Customize (exit 140)--> Customize
     ^   Back/Start/Return (exit 320, then resume)   |  Back (exit 320)  |  Back (exit 320, write registry)  | Back (exit 140)
     +------------------ Back (exit 320, remembered entry reset) ------+                                       v
                                                                      hub (reopens on the remembered entry) <-+
quit/restart/explore/map: dialog over the DIM-med pause screen; OK/Cancel/Back deliver after the dialog's 140 ms exit
Default Settings: dialog over Display; Yes (after 140 ms): auto-detect, re-init request, rebuild with focus on row 1
```

Exit lengths come from `gui_timing` (tvg11): 320 ms for `UI_pause`, `UI_PauseOptionsMain`, `UI_PauseOptions`;
140 ms for `UI_Options` and `UI_PC_Customize_Options` (both contexts); 140 ms for both dialogs.

## Apply / re-init hook (SPEC_GUI 5.4) -- unchanged from v1

The menu never touches the renderer. It edits the `StGraphics` the host lent it and raises two flags
immediately in the row handler: `om_take_apply_request` (any value changed) and `om_take_reinit_request`
(the change needs a display reset; SPEC 4.4 list and Default Settings -> Yes). The host consumes them at the
start of its next frame; flags survive the screens closing until consumed. `om_notify_resolution_lowered`
rebuilds the front-end Display list only (5.5). Exactly-once events (`om_poll_event`): `LEAVE_TO_HUB`
(Display left with Back, values written), `LEAVE_TO_PAUSE` (hub Back), `RESUME_GAME` (Start). `src/main.c`
logs where the renderer would act; resolution/vsync changes are **not** wired (task scope).

## Decision log

| # | Decision | Spec |
|---|---|---|
| D1 | Packages are loaded from the install at run time through `UgAssets` and parsed with `ug_fng_parse` (raw `0x00030203` chunks included); no object geometry, text or art is built natively. | ART 8.1, 8.5, SPEC_GUI 1.3 |
| D2 | Objects are bound by case-insensitive name hash (`OPTION_NAME_n`, `OPTION_k`, `TITLE_MASTER`, ...); a name that matches several objects, or a missing required object, fails the screen. `UI_pause.fng` additionally verifies the reviewed identity table (`pause_menu_binding.h`). | ART 3.1, 4.1 |
| D3 | Every application write of position, size, colour goes through `ug_scene_app_write`: value into the node and into the base key (time -1) of that property's track in every script of the node (private copy; the package is never modified); if no script has that track only the node changes. Text and texture use the node overrides. | ART 1.2 |
| D4 | Visibility uses the object's app-hidden flag, never scripts. Slots/rows outside the window are hidden **and** moved to y = 6969; slots without a list row are treated the same. | ART 1.3, 4.2 |
| D5 | Pause entries come from `pe_build(PauseContext)`; entry k binds to `option_k`; texture = icon key looked up in the loaded texture library, label = language table entry with the English text as fallback. Replay is disabled (skipped by Left/Right, not activatable) unless `replay_available`. The Options entry needs the settings the host lends **and** `options_ok`. | ART 2.2, 2.3, tvg07 |
| D6 | Carousel: slot i centre = master + i*158 + s; focused 118x59, others 0.8x; shown while \|x - master\| <= 240; scroll offset s = -158*focus, changed focus moves s with a cubic Hermite over 0.3 s (u = t/0.3, `3u^2-2u^3` from rest); focus, caption, tab, arrows change at once; first build snaps; no wrap; Up/Down ignored. The tab and arrows follow the focused position among all N entries (a disabled entry counts). | ART 3.2, tvg08 |
| D7 | Edge fade is off at build, on after the first dialog over the screen closes, off again when a dialog opens or an entry/Back is activated; alpha = trunc(f * A) with A = 255 / 150 (disabled), written as the slot's base colour (white); off = white 255. | ART 3.3 |
| D8 | Activation: "after exit" entries (Explore/Free Run, Options, Replay), Back and Start start the screen's 320 ms exit and act at exit-complete; a second Back/Start/Accept and every move are ignored meanwhile. "Immediate" entries: Resume Race resumes at once; the others open a dialog over the dimmed screen (handler `DIM`, `UNDIM` on close). Accept, Back and Start are ignored in the frame the screen was created. | ART 2.4 |
| D9 | The exit script is **not** started for dialog-opening entries (only DIM). ART 2.4 says it is started too but its visual result with the dim is unknown (10.2); fading the screen out would leave nothing to return to after Cancel. | ART 2.4, 10.2 |
| D10 | Dialogs: `UI_InGameDialog` OK/Cancel for quit/restart/explore/world map/garage (message key per context incl. the "all unsaved race progress" variants and the online/LAN cases), `GenericDialog_SMALL` Yes/No for Default Settings. Focus starts on the **second** button; Left/Right do not wrap; Up/Down/Start do nothing; Back = the cancel result; the result is delivered when the dialog's 140 ms exit completes. Button extents x +-(128+21) about the button centre, y 53..95. | ART 2.4, 6, tvg10, tvg06 |
| D11 | Hub: Audio, Display, Gameplay, Player; opens on the remembered entry (1-based, 0 = first); remembered value follows every Left/Right move, is reset by Back from the hub, by Start, and when a pause screen is built. Accept on Display exits (320 ms) into Display; the other pages do nothing. Back exits (320 ms) to a freshly built pause screen; Start exits then resumes. | ART 2.5 |
| D12 | Display always opens on its first selectable row (the remembered row is ignored). Display Back: exit (320 in game / 140 front end), registry write, then the hub (in game, inside this state machine) or close + `LEAVE_TO_HUB` (front end). Customize Back: exit 140, no write. Start: in-game Display only, exit then resume, no write; ignored elsewhere. | ART 4.7, 5, SPEC_GUI 5.1-5.3, tvg06 |
| D13 | Row placement uses the anchors read from `TITLE_MASTER`/`DATA_MASTER` (centre + authored size; the specified table is the fallback and the pointer geometry): name at (tx, ry) (centre x for a centre-justified name), value text centre x = data centre, arrows by top-left with their authored size, backing group at (tx+276, ry+10), slider base centred on the data column, knob/fill from base, margin 46, fill height authored. | ART 4.2, 4.3, tvg09 |
| D14 | Row state: Highlight/UnHighlight on name, data, arrows, backing and slider parts when the focus changes, started after the package reports enter-complete (or 600 ms) so the enter fade is not cut; the name object is the scene focus; arrows and backing show on the focused row only (plus 150 ms of fade-out); the Left/Right arrow plays Blink; DIM/UNDIM on the handler with the dialog; the exit script on the handler when a leave begins. | ART 4.4, SPEC_GUI 2.4, 3.6.2 |
| D15 | Vertical scrollbar: tab y = rail_top + 8 + (rail_h - 16)(c-1)/(N-1), rail, tab, arrows and end caps hidden for N <= 1; scrollbar arrows hidden on the in-game Display page. | ART 4.5 |
| D16 | Headers: front-end Display `Header_Text` <- Display; in-game Display `Header_text` <- PAUSE OPTIONS and `Header_text2` <- DISPLAY; Customize <- Customize Display Settings; hub and pause use their own keys. Console-only objects (`Back_button` group, glyph texts) are not drawn. | ART 5, SPEC_GUI 3.2, 3.3 |
| D17 | Pointer (centred units, edges inclusive): carousel arrows step on press, the selection slot activates on release only if the press began on it, neighbours/rail/tab/hidden arrows inert; list rows: focused row's left arrow, then right arrow, then value text (choice only); a button row's label column activates (and focuses) it; dialogs take clicks on their button extents. Hover and wheel on lists are not specified and ignored. | ART 3.2, 4.6, 6 |
| D18 | The pause adapter keeps the PC Back/Continue pills (earlier pause work, ink-fitted native text over the PC pill art) on the pause screen only; the hub, options screens and dialogs show none. | pause SPEC (earlier), ART 2.1 |
| D19 | Settings backend and the re-init hook are reused unchanged (`st_gfx_*`); the Level Of Detail / resolution preset behaviour of SPEC 4.2 stays as in v1. | SPEC 2.3, 4.2, 4.4 |
| D20 | `main.c`: both creation sites call `pause_menu_build()` (menu + stored graphics values + career free-roam context). Race contexts are not offered by this host (no race lifecycle, no replay data); any action the host cannot perform resumes the game. | ART 2.2; QUESTIONS 8 |
| D21 | `--pause-shot` replay events extended with Up, Down, Start so every screen can be reached; `--pause-reference` replays on the production adapter (no separate evaluator any more). | -- |

## Corrections to v1 (what changed and why)

| v1 statement (IMPL_DESIGN_GUI before this revision) | Now | Source |
|---|---|---|
| Default Settings dialog opens focused on **Yes**, clamped Left/Right | opens on **No** (second button), no wrap; Back = No; result after the 140 ms dialog exit | SPEC_GUI 6 (superseded text), ART 6, 9.1, tvg06 `defaults_yes` |
| Quit confirmation unchanged: "Resume / Options / Quit Game", Yes/No dialog (`IG_GenericDialog_SMALL`) | entries Return to Explore Mode / Options / Replay / Quit to Main Menu per context; `UI_InGameDialog` OK/Cancel, focus Cancel, Back = cancel | ART 2.2, 2.4, 9.3, tvg07, tvg10 |
| Exit animation 140 ms everywhere, Start resumes at once | 140 ms front end, 320 ms in game (Display, hub, pause), Customize 140 ms; resume/Back/Start act at exit-complete | ART 1.4, tvg11 |
| Options entry has no icon; no hub, Back from Display returns to the pause row | `INGAME_ICON_OPTIONS`; hub `UI_PauseOptionsMain` -> Display page 1 -> Customize | ART 2.2, 2.5 |
| in-game Display opens on a remembered row | always row 1 | ART 4.7 |
| native drawing with measured numbers, authored tracks not evaluated | authored packages evaluated; native drawing removed (`options_menu_draw`, the old Yes/No dialog) | ART 8 |
| pause slide: exponential ease, icons 118 -> 96, alpha ramp; resume immediate | Hermite 0.3 s, 0.8x, window +-240; resume after the exit script | ART 3.2, 2.4 (see QUESTIONS, authored-art pass, item 1) |

## Test changes forced by the supersession

* `tools/options_menu_test.c`: tvg06 `defaults_yes`/`defaults_no` now start on button 2, assert no wrap and the 140 ms exit; Display/Customize exit lengths per context; in-game Back goes to the hub screen; Start waits for the exit; pointer geometry from the grid anchors; the drawing smoke test is gone (nothing is drawn natively); tvg03 traces are also driven through the real Display and Customize screens in both contexts.
* `tools/pause_menu_test.c` and `.inc`: rewritten on the new adapter; pause fixtures are serialized synthetic packages loaded through `UgAssets`.
* `pause_menu_behavior_check.py`: skips by name the six slide cases of the exponential law, compares scroll samples only at t = 0, judges pointer gestures after 340 ms, asserts exit-then-resume for `authored_exit_fade` and `resume_immediate_*`.
* `pause_menu_vector_check.py`: corrections 0, 1, 3, 4, 5, 6 skipped (application-chosen art no longer exists); `pause_menu_supplement_check.py`: captions per ART 2.2, written slot alpha 255.
* Removed: quit dialog harness/checker/vectors (above).

## Not implemented / open

* Renderer: resolution, vsync, texture filtering, FSAA and the other values are not read by the renderer; only the hook exists.
* Audio, Gameplay and Player option pages, sounds, help bar pills and the Help popup, the front-end options hub, race lifecycle actions in the host, replay.
* Hover-to-focus, mouse wheel and scrollbar clicks on lists (unspecified, ART 10.7); in-game help bar content (SPEC_GUI 11.3).
* Visual parity with the retail screens is **unverified**: nothing in this pass opened retail data or screenshots (the verifier does that). The synthetic packages carry only published names, hashes, sizes and script names.
* Vendor FSAA overrides and the re-init retry loop belong to the renderer (SPEC 4.2, 4.4).

## Verification

`make gui-test` (= `gui-art-test`, `gui-view-test`, `options-menu-test`, `pause-menu-test`) plus `settings-test`,
`settings-boundary-test`, `ugui-test`, `make`, `make menu` -- results are in the implementer report; all test
binaries were also run clean under `-fsanitize=address,undefined`. Vector coverage: tvg01 (package names/keys/hashes
only; counts, sizes and sha256 need retail data), tvg02-tvg06 (`options_menu_test`), tvg07 + tvg10 + tvg11 +
tvg08 + tvg09 (`gui_art_test`, formulas and tables; the pause screen state machine around them), presentation and
adapter flows on synthetic packages (`gui_view_test`, `pause_menu_test options`).


## Amendment 1 (SPEC_GUI_ART_AMEND1, sha256 `51c25554...7d539`, vector `tvg12_frames.json`) -- decisions

Context: verifier defect D1 (icons about 184 units too low). Root cause: `gv_carousel_bind` read the centre of
`OPTION_MASTER` from the live scene node, after the first scene update had run the master's Init chain (a second
authored script, key `0x6048A18B`, that hides it and moves it to (0, 170)). D1-D21 above are unchanged except
where noted.

| # | Decision | Spec |
|---|---|---|
| D22 | `GvScreen` caches three anchors (`opt_master`, `title_master`, `data_master`: found, centre, size) in `screen_start`, immediately after the scene is built and **before the first `ug_scene_update`**, i.e. before any chained script has run. The centre is the object's stored (authored) position, its size the authored size. The cache is the only source: `gv_carousel_bind` (-> `gc_set_master`) and `gv_list_bind` (-> `gl_anchors_from_centres`) never look at the master nodes' evaluated position or size again, so a later bind (re-bind after a rebuild, bind 100 ms after creation) still gets the authored value. A carousel without the cached anchor fails to bind. | A6 |
| D23 | The list rebuild after Default Settings -> Yes keeps using the cached authored masters (AMEND1 marks the alternative unverified). | A6, C7 |
| D24 | Frames: positions the application writes (slots, tab, arrows, rows, row arrows, sliders, `OPTION_BACKING_n` group origins, dialog button groups) are in the object's own frame, which is the screen frame because every one of them is top-level (`ug_scene_app_set_world_pos` degenerates to the local write). Children of groups (glow, backing, strip, caption, header, backing parts, dialog panels and dim quad) are composed by the scene's existing parent chain (scale, rotate about pivot, translate; colour product); nothing was changed there. | A2, A3, A5, A8 |
| D25 | The current-script rule (track base + deltas of the running script, else stored data) is what the existing player already does: the hub's presentation group places glow/backing/caption/strip 0.26 units left of the pause screen after the enter fade, with no special code. Covered by the fixtures (scripts carry x -39.26 against stored -39.0). | A4 |
| D26 | Pixel mapping is `gl_px_map` (`s = min(W/640, H/480)`, centre origin, no rounding); `ug_view_fit` is asserted to agree. | A1 |
| D27 | Carousel geometry needs no new numbers: slot i centre = cached anchor + (i-1)*158 + s; focused 118x59, others uniform 0.8 about the same centre (2:1, full UV, no skew, asserted). Containment of every visible slot in the light strip is asserted for all six rest states. | A7, B3 |
| D28 | B2: `gv_screen_dialog(open)` sends DIM and then starts the screen's own exit script on the same handler object (so the presentation group, slots, rail, tab and arrows fade to alpha 0 in 100 ms / arrows 140 ms), without marking the screen as leaving (`exit_started` stays clear; exit-complete at 320 ms has no pending action). If the exit script is already the handler's current script the call changes nothing (no restart). Closing sends UNDIM (alpha 60 -> 255 in 100 ms). This **supersedes D9**, which kept the pause screen visible under dialogs. The caption text is not changed by activation. | B2 |
| D29 | Dialog focus look (B1) is data: the second button is focused (group alpha 225/255, dark box, lime label), the first sits at alpha 80 (drawn box alpha about 63); asserted on the two synthetic dialog packages. No second label pulse is forced (C4: either state acceptable). | B1 |
| D30 | B4: the first focused Customize row is row 1 (Car Reflection Update Rate) when its capability is non-zero, otherwise the next present row (Car Reflection Detail); asserted. | B4 |

Tests (tests first; `make gui-test` now includes `gui-frames-test`):

* `tools/gui_frames_test.c` encodes every tvg12 case at 1920x1080, 1280x720 and 640x480: the mapping, all 17 composition
  chains, the anchor read rule with its `must_not_equal_px`, the six carousel states (pause c=1/2/4, hub c=1/2, hub
  tween at 0.15 s) with centre, size, visibility, containment and 2:1 aspect, the list masters (UI_Options row-1 arrows,
  the other two list packages), the backing group's children, the dialog parts and B1, B2 and B4.
* The hazard is modelled by `anchor_chain` in `tools/synth_pkgs.h`: every shared synthetic `OPTION_MASTER`,
  `TITLE_MASTER` and `DATA_MASTER` has an Init script chained to a second script that sets alpha 0 and moves it
  (carousel to (0, 170), lists to the shifted columns). Every suite using these fixtures (gui_view_test,
  pause_menu_test, gui_frames_test) therefore fails if the anchor is read late. gui_frames_test additionally runs
  each carousel case in three variants (chain at the first update; chain 40 ms into the Init script and the bind 100 ms
  after creation; package serialised and re-parsed, bound 100 ms after creation) and asserts the fixture really
  moved the master (so the guard cannot go vacuous). Checked by mutation: replacing the cached anchor by the live
  node position fails gui_view_test (6 checks) and gui_frames_test (hundreds, with the pixel positions of D1 at
  y 922.5 / 615 / 410).
* Fixtures are synthetic: names, hashes, sizes and script names published in the specs; no retail data.
