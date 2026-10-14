# Graphics options GUI clean-room — analyst intake

- Date: 2026-10-14
- Role: analyst (clean-room). No implementation code written.
- Project purpose: interoperability / preservation. OpenUG2 is a reimplementation of a game the user
  owns; this work specifies the retail graphics/display options screens so OpenUG2 can present and
  drive them with the user's own retail data.
- Subject binary: `speed2.exe`, sha256 `f9dd86c054878ce6276beb07c1fd61874f7a1e4bf1f241b084c65b73e24168a7`
  (same binary as SPEC.md, verified).
- Retail data used (user's install, original 2004 files only): `FRONTEND/FRONTB.LZC.bacc`,
  `GLOBAL/InGameCommon.bun`, `GLOBAL/InGameCommon.lzc`, `GLOBAL/GLOBALB.BUN`,
  `GLOBAL/GLOBALB.LZC.bacc`, `LANGUAGES/English.bin` (via the settings analyst's label table).
  The 2023-modified `FrontB.lzc`/`GlobalB.lzc` were not used.
- Observation data: one user-supplied retail screenshot of the Audio options page (taken
  2026-10-13 for the earlier GUI work) and that work's analyst measurements.
- Public sources: FEngLib (public FEng package library; object type and flag enumerations).
- Method: package extraction and parsing with analyst-only scripts; static examination of the
  executable in Ghidra (project of the settings analyst). The game was not run.
- Inputs read besides the above: docs/cleanroom-settings/SPEC.md, QUESTIONS.md, REVIEW.md,
  test-vectors/README.md; docs/FRONTEND_ASSETS.md; tools/pause_menu_vectors/README.md and two
  vector files (style only); the earlier analyst workspaces and reviewed pause-menu spec in the
  local backup `bkup/OpenUG2-gui/cleanroom/{analysis,spec}` (analyst/spec material only).
- Not read: `src/` (only a directory listing of file names), `impl/` trees in backups,
  `docs/cleanroom-settings/IMPL_DESIGN.md` (its line count was printed by a `wc -l` over the
  directory; content not displayed). No implementer notes or conversation were seen.
- Outputs: docs/cleanroom-settings/SPEC_GUI.md, docs/cleanroom-settings/test-vectors-gui/.
  Private evidence: analysis/settings-gui/ (not for commit).

## Authored-art supplement (SPEC_GUI_ART.md) — analyst intake, 2026-10-14

- Role: analyst (clean-room), same binary and project purpose as above. No implementation code
  written; `src/` and `tools/` not touched.
- Task: pause-menu entry catalogue, Options route and quit confirmation; data needed to render the
  Display, Customize, pause and pause-options-hub screens from the retail FEng packages.
- Retail data used (original 2004 files only): `GLOBAL/InGameCommon.bun`/`.lzc` (`UI_pause.fng`,
  `UI_PauseOptionsMain.fng`, `UI_PauseOptions.fng`, `UI_InGameDialog.fng`,
  `IG_GenericDialog_SMALL.fng`, `UI_PC_Customize_Options.fng`), `FRONTEND/FRONTB.LZC.bacc`
  (`UI_Options.fng`, `UI_PC_Customize_Options.fng`), `GLOBAL/GLOBALB.BUN` (`GenericDialog_SMALL.fng`),
  `LANGUAGES/English.bin` (via the settings analyst's label table); texture-name lists from the
  earlier GUI analyst's evidence (names/keys only).
- Observation data reused (no new captures): the Audio-page screenshot measurements (SPEC_GUI) and
  the pause-screen / frontend-quit-dialog screenshot facts recorded in the earlier pause-menu
  analyst spec (`bkup/OpenUG2-gui/cleanroom/spec/pause-menu/{SPEC,BINDING,QUIT_DIALOG}.md`).
- Public source: FEngLib message-response command enumeration (local copy in the earlier analyst
  workspace `bkup/OpenUG2-gui/cleanroom/analysis/public/FEngLib`).
- Method: static examination of the executable in the settings analyst's Ghidra project
  (decompilation/disassembly of the pause, hub, carousel, list-row, slider, dialog and property
  setter routines; a few function boundaries were created in the analyst's Ghidra database);
  package parsing with analyst-only scripts. The game was not run.
- Exposure log: a directory listing of `docs/cleanroom-settings/` and a `wc -l` over its `*.md`
  files printed the file names and line counts of `IMPL_DESIGN.md` and `IMPL_DESIGN_GUI.md`;
  their content was not opened or displayed. No implementer notes, `impl/` trees or `src/` were read.
- Outputs: `docs/cleanroom-settings/SPEC_GUI_ART.md`; test vectors
  `docs/cleanroom-settings/test-vectors-gui/tvg07…tvg11` and README section; private evidence under
  `analysis/settings-gui/` (`evidence/decomp/`, `evidence/json2/`, `evidence/art/`,
  `scripts/objdump.py`, `scripts/gen_vectors_art.py`, `notes/evidence-index-art.md`; not for commit).
