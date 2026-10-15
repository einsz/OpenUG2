# Controls menu — project decisions (user, 2026-10-15)

Answers to CONTROLS_SPEC §10. They override the spec where the spec offers alternatives.

* **D1 route:** add a fifth entry "Controls" to the existing pause options hub, using the original Controls icon and label keys named in the spec (original resources only). Deliberate deviation from retail.
* **D2 key names:** use SDL platform key names; substitutions in the spec (`---`, `Left Shift/Alt/Ctrl`, `Button n+1`) still apply.
* **D3 gamepad indices:** raw SDL joystick indices.
* **D4 persistence:** persist keyboard bindings to the OpenUG2 INI store (src/settings.*), gamepad bindings in the profile device records as the spec states.
* **D5 retail quirks:** fix them (no stale device index on re-entry, no wheel secondary-cell oddity, no double display for empty primary key, no POV read after a button was taken the same frame).
* **D6 defaults:** adopt the retail default keys from the spec as the reset target and initial bindings (replacing the hard-wired W/S/A/D/N/B/C).
* **Resource rule:** no created art, fonts, strings, layouts; spec Resource rule R1–R4 is binding.
* **Base:** branch `settings-controls` is based on `origin/settings-gui` (options_menu.*, gui_view.*, pause_*.c are the baseline).
