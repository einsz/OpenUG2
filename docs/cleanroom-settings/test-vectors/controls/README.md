# Controls screen test vectors

Companion to `../../CONTROLS_SPEC.md`. Produced by an analyst-side generator written for this
project (an independent model of the spec statements, not code from the original program and not
OpenUG2 code). No retail asset bytes are reproduced. Label keys, package/object hashes, action
ids, DirectInput key codes and the known-device default tables are **functional lookup data**;
English texts are included only as check values for the runtime lookup in `LANGUAGES/English.bin`.

| File | Content | Spec | Provenance / confidence |
|---|---|---|---|
| `tvc01_labels.json` | every label key used by the Controls screen, its reset dialog, the help-bar pills and the front-end options hub, with the English text | 3, 7, 9 | keys [static]; texts [observed] in `English.bin` |
| `tvc02_rows.json` | row list per device class: slot, label key, action id per column (capture vs display) | 5 | [static] |
| `tvc03_keyboard_defaults.json` | default keyboard key pair per row, full default table order, DIK → SDL scancode mapping, expected English names | 6.1, 6.2 | action/key codes [static]; DIK ↔ SDL mapping [inferred, public headers]; Windows names [inferred] |
| `tvc04_keyboard_capture.json` | synthetic rebinding scenarios (replace, steal, promote, Escape) | 6.3 | rules [static]; scenarios synthetic |
| `tvc05_joystick_defaults.json` | 32-entry binding tables and axis state for generic gamepads/wheels; the 9 built-in known-device tables | 6.4 | rules and tables [static] |
| `tvc06_joystick_capture.json` | POV direction from angle; synthetic button/POV/axis rebinding scenarios with resulting display | 6.5 | rules [static]; scenarios synthetic |
| `tvc07_navigation.json` | focus/window sequences (7 visible rows, two columns, selector row) | 4.4 | rules [static]; sequences synthetic |
| `tvc08_display_names.json` | cell text formation (left-modifier names, button renumbering, empty → `---`, 23-character cut) | 6.2 | English rules [static]; 23-char cut [inferred] |
| `tvc09_package.json` | `UI_Options_PC_Controller.fng` identity, row templates, roles of its objects with stored positions/sizes, related packages, texture keys | 2, 3 | [observed] in the original 2004 data files |

Notes for checkers:

* "display" entries in `tvc06` are `["axis", offset]`, `["button", index]`, `["pov", n]` or `null`
  (cell shows `---`). How an axis/button/POV is *named* is platform data (spec 6.2, 10.3).
* `tvc04` start state is "after Reset Keys To Default"; the retail start-up state may differ in
  one entry (spec 11, U2).
* The wheel row "Career Statistics" secondary cell intentionally displays action 17 (spec 5.4).
