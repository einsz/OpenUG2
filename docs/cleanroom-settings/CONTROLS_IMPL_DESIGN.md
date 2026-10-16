# Controls screen -- implementation design log

Implements, from the approved documents only: `CONTROLS_SPEC.md` (sha256 `3295a4d4...639c18`, approved in
`CONTROLS_REVIEW.md`), the vectors `test-vectors/controls/tvc01..tvc09`, and the project decisions
`CONTROLS_DECISIONS.md` (D1-D6, resource rule). Baseline code built on: `src/options_menu.*`, `src/gui_*`,
`src/pause_*`, `src/ugui/*`, `src/settings.*`, `src/main.c`. Nothing was taken from the retail executable,
analyst notes or disassembly. The retail data directory was used only to *run* the game (screenshots of
the genuine package).

## What was added

| Module | Role | Spec |
|---|---|---|
| `src/controls_bind.[ch]` | pure model: action ids, row lists, DIK<->SDL table, keyboard table + defaults + capture/steal + display, joystick defaults (generic and 9 known devices), capture rules (button/POV/axis), display rule, name formatting | 5, 6.1-6.5 |
| `src/controls_menu.[ch]` | pure screen state machine: device list, selector, 7-row window, capture/lock/end stages, reset request, pointer, queries | 3.8, 4.1-4.7 |
| `src/controls_store.[ch]` | persistence: keyboard table in the INI store, pad records in the profile | 6.6, D4 |
| `src/controls_sdl.[ch]` | SDL side: joystick discovery and defaults, platform names, per-frame samples, driving lookups, save/load glue | 8 "Game input", D2, D3 |
| `src/options_menu.*` | fifth hub entry, `OM_SCREEN_CONTROLS`, input routing, reset dialog, change event | 2.1, 4.3, 4.6, 4.7, D1 |
| `src/gui_dialog.*` | `GD_CONTROLS_RESET` (same package, OK / Cancel, keys only) | 4.6 |
| `src/gui_view.*` | `GvControls`: authored-object binder and writer | 3.1-3.9 |
| `src/pause_menu.*` | view switch, hover/raw key/pad frames, resource check, persistence hook | 2.2, 4.5 |
| `src/main.c` | binding lookups replace the hard-wired keys | 8 |
| `tools/controls_test.c`, `tools/controls_sdl_test.c`, `tools/gui_view_test.c` | tests | 13 |

## Resource rule (R1-R4) -- how it is kept

* No new art, font, layout, package or string. Every text on the screen is a language-file key
  (`gv_text(..., NULL)`: no fallback); the new hub entry carries `label = NULL`; the reset dialog spec has no
  English text. Only literals produced in code: `---`, `Button n` (the `Left Shift/Alt/Ctrl` replacement is a
  rewrite of platform names) and SDL's own key names.
* Availability (R3): `pause_menu_set_controls()` verifies once that the package parses and binds
  (`gv_controls_bind`), the eight package textures and the hub icon decode, the dialog package exists and all
  30 language keys resolve. If anything is missing the hub keeps its four baseline entries; if the view fails
  later the screen is simply not drawn (the usual fail-closed path of the authored screens).
* R4: nothing from the retail data is in the repository; the tests use synthetic packages and the vectors.

## Decisions

* **D1 (route).** Fifth entry in the pause options hub: `OM_HUB_ENTRIES` stays 4 (the baseline table and its
  tests), `om_hub_count()/om_hub_entry_of()` add the fifth (`OmHubEntry`: icon `0xA04A7B26`, key `0x04DC6DB5`,
  page 4). Back from Controls returns to the hub at once (exit-complete is posted at 0 ms, 3.9) with the
  Controls entry remembered. `UI_Options_PC_Controller.fng` is deliberately not in `gui_timing`'s table (its
  count is asserted by tvg11); `om_exit_ms` is 0 for it and no exit script is started, since the package's
  `FORWARD` posts exit-complete at 0 ms without a fade-out.
* **D2 (names).** `SDL_GetScancodeName`; the 3.x substitutions are applied by `cb_format_name` (tvc08), the
  64-byte and 23-character cuts too. Axes/POVs: see QUESTIONS item 1.
* **D3 (indices).** Raw SDL joystick indices: button i = bit i of the sample, hat p = POV p, axis index i =
  axis slot i (record offset 4*i, so `Rz` = slot 5 = SDL axis 5). The defaults depend on the SDL button count
  and an axis-exists mask from `SDL_JoystickNumAxes`.
* **D4 (persistence).** Keyboard: one INI string value `ControlsKeyboard = "1:<53 scancodes>"` (one flush,
  versioned, malformed or foreign values are ignored, out-of-range keys dropped, duplicates keep the first
  holder). Always the INI store (`st_default_config_path`), also on Windows where the graphics settings use
  the registry. Pads: `StInputDevice` records of the profile `OPENUG2` (slots 1-5, slot 0 is the keyboard's
  and stays unused), matched by SDL joystick GUID, the n-th pad of a GUID takes the n-th record; invalid
  records are ignored. Written when Back leaves the screen after a change (`OM_EV_CONTROLS_CHANGED`).
* **D5 (retail quirks fixed).** Q1: `cm_open` always starts on the keyboard and reset acts on the displayed
  device. Q2: wheel Career Statistics Secondary captures *and* displays action 18 (`tvc02` records the retail
  value 17; the test asserts exactly that deviation). Q3: `cb_kb_display` shows column 1 only when the first
  entry holds a key (`tvc04` "step 1" expects `W | ---`, the table itself still follows 6.3 so step 2 matches
  the vector). Q4: POVs are not read after a button was assigned in the same frame. Escape stays unbindable
  and there is no conflict prompt (steal semantics, as specified).
* **D6 (defaults).** `cb_kb_reset` walks the transcribed default table (6.1) and is also the initial table;
  main.c reads actions instead of `W/S/A/D/Space/N/B/C/Esc`. Default pad bindings come from `cb_pad_defaults`.

## Behaviour notes (spec section -> code)

* 4.1 entering: keyboard rows, focus row 1 Primary, instruction 0. 4.2: wrap on both ends, one device does
  nothing, arrows hidden for one device, the click on an arrow also moves the focus to the selector (U7).
* 4.3/4.4: table in `cm_input`; window `if row >= top+7: top++; if row < top: top--` (tvc07, three sequences).
* 4.5 capture: `begin_capture` locks all input, Accept = Pulse, pointer click = no Pulse; Escape cancels on any
  device; key presses are ignored in a pad capture; pad samples are ignored in a keyboard capture. The end is
  two ticks long (`end_stage` 1 -> 2 -> unlock), instruction 2 stays for the rest of the visit. The first pad
  frame of a capture only takes the reference (buttons held when the capture starts are not assigned).
  `pause_menu_event` hands raw `SDL_KEYDOWN` scancodes to the capture before the menu key mapping.
* 4.6: the dialog is `GenericDialog_SMALL.fng` through the existing `GuiDialog`, message key `0x2BBCCA6B`,
  OK `0x639`, Cancel `0x541389E5`, focus on Cancel; OK calls `cm_reset_device` on the displayed device.
* 6.3/6.5: `cb_kb_capture`, `cb_pad_assign_control`, `cb_pad_assign_axis` are the literal rules; the axis scan
  order is X, Rx, Y, Ry, Z, Rz, slider0, slider1; the POV direction uses integers (diagonals = left/right).
* 3.x view: anchors are the creation-time centres of `TITLE_MASTER`, `DATA_MASTER`, `DATA_MASTER2` (AMEND1 A6;
  `GvScreen.data_master2` added); label (tx, y_k), cell centres x 11 / 173, backing (tx+276, y_k+8), rows
  outside the window parked at y 6969, highlight scripts on cell + label + backing, `Pulse` on the capturing
  cell, selector arrows Blink/Static, scroll arrows Blink on Up/Down, tab not moved while the selector is
  focused. Vertically centred texts get half a line added to the anchor (QUESTIONS item 5). The reset dialog
  dims the screen with the handler's DIM/UNDIM like the other option screens.
* Game input (main.c): `cx_throttle/cx_steer/cx_action_down` for Throttle, Back/Reverse, Steering, Handbrake,
  Nitrous, Look Back (held), and key events for Camera (Shift+key = previous, as before), Pause (also a bound
  pad button), Shift Up/Down and lane change in drag races. The controller Start button still pauses.
  Pad axes feed throttle/steering (QUESTIONS item 3). The old keys remain only for developer features, which
  stand back when the key is bound (QUESTIONS item 8).

## Tests (all synthetic; no retail data)

| Target | What |
|---|---|
| `make controls-test` -> `build/controls_test` | tvc01 labels/keys, tvc02 rows, tvc03 defaults + DIK map + SDL names, tvc04 capture, tvc05 pad defaults (7 generic + 9 known), tvc06 POV and capture cases (driven through `cb_pad_capture_frame`), tvc07 navigation (keyboard 18 rows, wheel 16 rows via the selector), tvc08 names, tvc09 hashes; capture/lock/reset/device flow; INI + profile persistence; the screen inside the options hub (entry hidden without resources, Back event, dialog) |
| `controls-test` -> `build/controls_sdl_test` | SDL virtual joystick: discovery, defaults, samples, POV/axis/button lookups, restart persistence (skips if SDL has no virtual joysticks) |
| `make gui-view-test` | `GvControls` over a synthetic package: anchors read at creation, placement, window, tab, Pulse/Highlight, arrows, unavailable when an object or the tab is missing |
| `make gui-test`, `settings-test`, `settings-boundary-test` | unchanged, still pass |

## Not verified

* A real gamepad/wheel by hand (only an SDL virtual joystick); the device-selector picture with two devices
  was only exercised synthetically, not with the retail art.
* Windows build; `make menu` compiles but was not run.
* Joystick axis semantics against hardware (QUESTIONS item 3); English-only name rules (the German, Swedish,
  Dutch and Korean substitutions of 6.2 are not implemented, the spec does not give their strings).
