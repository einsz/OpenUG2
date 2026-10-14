# Graphics options screens -- implementation design log

Implements `SPEC_GUI.md` (sha256 `9bcb6c8b...81da8`) and the amended `SPEC.md` (sha256
`e79fe5fa...c8ef`), both approved in `REVIEW.md`; nothing was derived from outside the specs and
`test-vectors-gui/` / `test-vectors/`. Scope of this pass (v1): the graphics **Display** and
**Customize** screens, reachable from the in-game pause menu; plain native drawing; **no renderer
changes** (resolution, vsync, filtering, FSAA are only edited in memory and persisted).

## Files

| File | Role |
|---|---|
| `src/options_menu.h`, `src/options_menu.c` | state machine (GL-free, SDL-free; includes `settings.h` and, for the drawing function only, `ugui/ugui_canvas.h`) + `options_menu_draw` |
| `src/pause_menu.c/.h`, `src/frontend/frontend.h` | `Options` entry, input routing, update/draw hooks; `FE_ACTION_OPTIONS` appended to `FeAction` |
| `src/main.c` | lends the settings to the pause menu, consumes the re-init hook |
| `tools/options_menu_test.c` | all of `test-vectors-gui` (tvg01..tvg06) + behaviour tests, JSON read at run time |
| `tools/pause_menu_options_test.inc` | pause adapter <-> options integration (run as `pause_menu_test options`) |
| `Makefile` | `options-menu-test`; `src/options_menu.c`, `src/settings.c` added to the game sources and the pause test link |

## The apply / re-init hook (SPEC_GUI 5.4)

The menu never touches the renderer. It edits the `StGraphics` the host lent it (`om_open`) and
raises two flags immediately in the row handler:

* `om_take_apply_request(m)` -- any graphics value changed (takes effect when the next frame is drawn);
* `om_take_reinit_request(m)` -- the change needs a display reset (SPEC 4.4 list: resolution, texture
  filtering, vsync, FSAA, road reflection, light trails, light glow, motion blur, depth of field, over
  bright, enhanced contrast, car reflection rate, level of detail; Default Settings -> Yes).

Both are consume-and-clear; `om_apply_pending` / `om_reinit_pending` peek. The host calls them at the
start of its next frame (SPEC_GUI 5.4: "serviced at the start of the next frame, while the screen is
still open"); flags survive the screens closing and re-opening until consumed. If the host's reset
lowers the resolution (SPEC_GUI 5.5) it calls `om_notify_resolution_lowered(m)`: the **front-end**
Display list is rebuilt (Resolution text, focus row 1); the in-game list is not refreshed. No message
is shown in either case. `src/main.c` currently only logs a line ("renderer hook not wired") where
the renderer would act. Exactly-once events (`om_poll_event`): `OM_EV_LEAVE_TO_HUB` (Display left with
Back, values written) and `OM_EV_RESUME_GAME` (in-game Start).

Pause adapter API: `pause_menu_set_options(p, gfx, caps, store)` turns the Options entry on (off by
default, so the authored two-entry pause row and its vectors are untouched), `pause_menu_options(p)`
returns the `OptionsMenu` for the hook, `pause_menu_options_open(p)`.

## Decision log

| # | Decision | Spec |
|---|---|---|
| 1 | Two screens, one state machine: Display (4 rows) and Customize (<= 22 rows + spacer); package names and keys exposed (`om_package_name/key`, hub, dialog) and cross-checked against tvg01 with the name hash. Front end vs in game selects package names, titles, colours, remembered focus, Start and resolution-lowered handling through `OmContext`. | SPEC_GUI 1.1, 1.2, 1.3 |
| 2 | Row model `OmRow` = kind (choice / slider / button / spacer), 1-based object slot, label key + English text, selectable, ST_GFX slot, slider range, value-text kind, re-init flag. Slots are list positions, absent rows leave no gap. | 2.1, 2.2, 4.1 |
| 3 | List engine `OmList` is a separate pure unit (`om_list_init`, `om_list_move`): non-wrapping, past non-selectable rows to the first selectable or the end row, window adjusted minimally, initial window `top = focus` if `focus <= N-V+1` (1-based) else `N-V+1`; V = 8. Tested directly with tvg03 and again through the real screens. | 2.5, tvg03 |
| 4 | Initial focus: first selectable row; in game, Display uses an optional remembered 1-based row when it is a selectable row, else the first selectable (QUESTIONS 7). | 2.5, 11.4 |
| 5 | Slider rule `om_slider_step`: stored value clamped into [min, max] before stepping; "changed" only if it moved; Level Of Detail calls `st_gfx_set_level` (records level, preset + clamp, no resolution reset); other sliders store the value. Building a row never rewrites the stored value (tvg04 stored 0 shows 1). Fill fraction `(v-min)/(max-min)`. | 2.7, SPEC 4.2, tvg04 |
| 6 | Toggles (all two-state choice rows incl. Low/High and Vsync) flip 0 <-> 1 (non-zero -> 0) on Left, Right and Accept. Resolution moves only with Left/Right via `st_resolution_step` (supported indices, no wrap) and not at all for an index outside 0..5; Accept does nothing. Accept on a slider does nothing. | 2.7, SPEC 4.4, 3.1 |
| 7 | Value text: resolution literal strings (functional text), On/Off/Low/High keys, Vsync Off key `0x88576CFF`. For an out-of-range resolution the text shown is the last in-range one (`res_shown`), `NULL` before any ("left unchanged"). English text is built in; the language file is not read (no string-table access in this module). | 3.1, 4.1, tvg05 |
| 8 | Customize rows come from one table in the 4.1 order; a row is built iff its cap is non-zero (Vsync always). Spacer after row 1 only if `OmConfig.row1_label_has_caret` and row 1 exists. FSAA/other caps: QUESTIONS 6. | 4.1, 10 (amendment 3), tvg02 |
| 9 | Re-init classification is a per-row flag from the SPEC 4.4 list (vector: Light Glow yes, Crowds no). The flag is raised in the input handler itself, never deferred to leaving. | 4.4, 5.4, tvg06 |
| 10 | Back from Display starts an exit animation (`exit_ms`, QUESTIONS 1); while it runs all input is ignored (second Back ignored). On completion: `st_gfx_save` to the lent store (skipped when none), event `LEAVE_TO_HUB`, menu closes. Customize open and Customize -> Back use the same animation but never write. | 5.1, 5.3 |
| 11 | In-game Start on Display closes the screens, emits `RESUME_GAME`, does not write, keeps the edited values and any pending requests. Start elsewhere is inert. | 5.2, 10 (amendment 4) |
| 12 | Default Settings: Accept opens a modal dialog (message `0x2BBCCA6B`, buttons `0x0000F770` / `0x0000061C`, focus on Yes, package `GenericDialog_SMALL.fng`). Yes = `st_gfx_autodetect` with level `st_perf_level(cpu_mhz, gpu_score)` (SPEC 4.5 recommendation: 3000 MHz, score 5 -> P = 4), raises both requests, rebuilds Display with focus row 1, no registry write. No / Back = nothing changes, focus kept. Left/Right clamp (no wrap), Up/Down ignored. | 6, 5.6, SPEC 4.2, 4.5 |
| 13 | Pointer (centred virtual coordinates): arrows of the focused row = Left/Right, value text of a choice row = Right, button label area activates; the dialog is modal. Geometry from the measured Audio-page numbers (QUESTIONS 4). | 2.7, 2.3, 11.2, 11.10 |
| 14 | Scrollbar: visible iff more than one row; tab fraction = focus / (N-1). | 2.6 |
| 15 | Drawing is native: dim layer, panel(s) per variant, titles, rows at `y = -127 + 36 k`, label x -268.9, value x 127.8, arrows x 6.2 / 246.2 on the focused row only, slider fill from x 54.6 to the knob (full at 196.6), front-end focus backing grey (65,65,65), in-game text lime with no backing, scrollbar, hint line, dialog. Rows dim under the dialog. Authored tracks (Highlight pulse, Blink, FadeIn/Out) are not evaluated. Needs the host's text face to be current (the pause adapter sets it). | 2.3, 2.4, 3.2, 3.3, 8 |
| 16 | Pause integration: the entry exists only after `pause_menu_set_options`. Row = Resume / Options / Quit Game; entry art follows the action (Options has none); Quit confirmation unchanged (selected by action). Confirm on Options -> `FE_ACTION_OPTIONS` -> opened inside `pause_menu_update` and never reported to the game. While the screens are open Up/Down/Left/Right/Enter/Esc/right click/controller map to `OmInput` (Start -> Start) and nothing reaches the pause row or the game; opening the pause menu again closes them. | 1.2, 5.1, 5.2, 7 |
| 17 | Settings backend reused unchanged (`st_gfx_*`); `OptionsMenu` does not own `StGraphics`, caps or store. The game host loads the stored values (`st_gfx_load`), clamps them to the caps and stores into the platform store (INI file on Linux). First-run auto-detect is not triggered here. | SPEC 2.3, 4.3, 4.4 |

## Not implemented / open

* Renderer: resolution, vsync, texture filtering, FSAA and the other values are not read by the
  renderer; only the hook exists (above).
* Authored package rendering (FNG scripts, textures, fonts per screen), sounds, help bar pills,
  the Help popup (`0xED586157`), console-only objects; language-file lookup of labels.
* Mouse wheel, hover-to-focus, scrollbar clicks, dialog pointer input (spec unknown: SPEC_GUI 11).
* The in-game options hub (`UI_PauseOptionsMain.fng`) and the front-end options hub are not modelled;
  Back from Display emits `LEAVE_TO_HUB` and the pause adapter returns to the pause row.
* Vendor FSAA overrides and the re-init retry loop (SPEC 4.2, 4.4) belong to the renderer.

## Verification

`make options-menu-test`: `options_menu_test` 1092 checks, 0 failed (tvg01 package keys, tvg02 all
six row cases, tvg03 six navigation traces, tvg04 slider cases + build clamp, tvg05 all value-text
rows, tvg06 all eleven flow cases in both contexts where "either", plus persistence, dialog, exit
animation, pointer, scrollbar, spacer, resolution-lowered handling, draw smoke test) and
`pause_menu_test options` (Options entry, routing, Back/Start, hook, reopen). The options test was
also run clean under `-fsanitize=address,undefined`. Still passing: `make`, `make menu`,
`make settings-test`, `make pause-menu-test`, `make ugui-test`. Not run: the game with real data
(no retail data used; the on-screen look of the options screens is unverified).
