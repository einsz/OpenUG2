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
