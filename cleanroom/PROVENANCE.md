# Provenance — UG2 GUI library clean-room project

Date opened: 2026-10-13
Purpose: interoperability / preservation. Independent reimplementation of the
Underground 2 frontend GUI library (screen/widget model, navigation, controller
hint bar) for the open-source OpenUG2 engine, plus a demo window showing every
widget type with controller hints replaced by mouse-navigation hints.
Authorization: user states they own both retail copies (PC install, PS2 USA ISO)
and requests analysis for this purpose (recorded in chat, 2026-10-13).
Requested scope: GUI element library + test window with all elements + hint bar
(mouse navigation replaces gamepad buttons).

## Inputs
- PC: /home/dq/Games/Need for Speed Underground 2 (speed2.exe sha256 f9dd86c054878ce6276beb07c1fd61874f7a1e4bf1f241b084c65b73e24168a7, 4800512 B; FRONTEND/FrontB.lzc sha256 7e1f3f89bd036300171235132f1d06d8c94cf978525649c6b2bae9ebcf95658f)
- PS2: Need for Speed - Underground 2 (USA) PS2.iso sha256 25c807d7bff0c51d234bd98d01e9447d65bac098842e44b42fc7718170a9fccd, 2874605568 B

## Layout
- cleanroom/analysis/  ANALYST ONLY (gitignored)
- cleanroom/spec/      reviewed, implementer-visible
- implementation: src/ugui/ + tools/ugui_demo.c (implementer only)

## Role log
(append entries: date, role, subagent, notes)

## Contamination incidents
none

## Role log
- 2026-10-13 analyst (cleanroom-analyst, fresh): wrote spec/SPEC.md + synthetic test vectors. Data note: installed FRONTEND/FrontB.lzc is a 2023-modified, non-JDLZ file; analyst used original FRONTB.LZC.bacc (sha256 50fc7b89…c66a). Public research: NFSTools/FEngLib @34859b4 cloned into analysis/public/ (analyst-only; no names used in spec). Contamination: none.
- 2026-10-13 reviewer (fresh): approved SPEC.md sha256 92c9026fe6747f4272a2c6086a744c42c7eee649b3d0c64df51ade8a8f9924b8; 3 edits logged in spec/REVIEW.md.
- 2026-10-13 implementer (fresh): src/ugui + tools/ugui_demo*.c; 1454 checks pass; contamination none (read first 60 lines of tools/frontend_test.c for macro style only). Verifier phase (differential vs original) not run: no original runtime available.
- 2026-10-13 analyst round 2 (fresh-resumed after rate limit): added spec Part II (§13-18: JDLZ/HUFF, bundles, texture packs, fonts, strings, capture plan), AMENDMENTS.md. One Wine launch attempt before user instruction "do not run the game; ask for screenshots"; crashed pre-menu, no user prefix/saves touched, nothing captured. Public refs: psx-spx (analysis/public/psxspx).
- 2026-10-13 reviewer round 2: approved SPEC.md 3a6b4b3f25c3bdfe20337f44f2193e01c43a6ed9bdbbb2a2100f779d0ed8cd38, AMENDMENTS.md 102c07daf3d1ba9211c7a2255978ce939e498dcda84abc0f8d2b97a84c99ac15.
- 2026-10-13 implementer round 2 (fresh): asset loaders + original-screen rendering; read only spec + own code; retail resource names used as lookup keys in ugui_assets.c/demo (interop keys). Tests 1784 ok.
- 2026-10-13 analyst round 3 (fresh): user supplied 6 reference screenshots (PC original, 1920x1080, user's install with 2023 mods; stored analysis/evidence/reference_shots, gitignored). Spec draft 3: §19 reference checklist, R tag. Analyst viewed implementation renders for comparison only; findings recorded as facts about original.
- 2026-10-13 reviewer round 3: approved SPEC.md 49d49bf04dfe468c8332f69cd18d30ffcd939a271e637abdb266e3df71c278df, AMENDMENTS.md 47ff48f467ee412c7a350fbfdefd4e87de5a7fa0b709ea83fe643bb9e494f900. docs/FORMATS.md & tools/jdlz.py are repo-owned independent clean-room files.
- 2026-10-13 implementer round 3 (fresh): Amendment 3 geometry/PC help bar/pillbox; 2317 checks pass; nothing outside permitted set accessed.

## Continuation scope — 2026-10-13
The user requests faithful original GUI visuals: correct menu corner orientation,
remove hint-bar pills and invented PC icons, use original PS2/Xbox button icons,
and remove nonoriginal visual assets. This supersedes earlier requests for
mouse-navigation hint replacements and visual fallbacks. Existing authorization
and supplied retail input hashes above carry forward. Retail assets remain
runtime inputs and must not be committed. Original game execution remains
excluded per the prior user instruction recorded in HANDOFF.md.
- Coordinator started a fresh cleanroom-analyst for original visual/data
  observation and Amendment 4; separate review, implementation and verification
  contexts will follow. Coordinator does not read original code or edit implementation.
- 2026-10-13 analyst round 4 (fresh): wrote original-only visuals amendment
  (§20), corner transform and PS2 controller decoding vectors; original Xbox
  asset source remains unavailable in the supplied PC/PS2 copies. No game launch.
- 2026-10-13 fresh reviewer round 4 started on spec/ with evidence traceability.
- 2026-10-13 reviewer round 4 (fresh): approved SPEC.md
  cb1a7a83d388bb09bdc030103dc083b54ef775f8c9090489b29fc51e24767fc5
  and AMENDMENTS.md
  9f0e2b3c3d4f0a8fb7390ad8b0c1645ea56ef790eba9b728a63df1601b0c14c2.
  Corrected conflicting older fallback permissions and Q28 font vector; no
  contamination found. Fresh implementer started with approved paths/hashes
  and permitted independent implementation baseline only.
- 2026-10-13 implementer round 4 (fresh): original-only visual paths, full
  corner transforms, runtime PS2 controller loader, explicit demo bindings.
  Default tests/demo: 6385 checks pass; ASAN/UBSAN pass, leak checking unavailable
  under host ptrace. Read only reviewed spec and permitted independent baseline.
- 2026-10-13 verifier round 4 (fresh): runtime asset loading and captures from
  supplied PC/PS2 sources; all 19 decoded PS2 image hashes match independent
  evidence. Four cleanup discrepancies recorded behaviorally in
  spec/VERIFICATION_ROUND4.md for fresh reviewer gate. No original game launch.
- 2026-10-13 fresh verifier-amendment reviewer: approved SPEC.md
  c5fa8e37e527759bcde30baf0389767a6d63d8f0d29b4701602279cac2b13c34,
  AMENDMENTS.md d7d113dd93b36ad310b06d3cfcb56f5517df21cfda68790e81872c3668de406b,
  VERIFICATION_ROUND4.md f211dcf52b224234cb262954e7f36276fbe7344fa04ae1af8b5c5e2cac1893d7.
  Fresh implementer started on reviewed §20.6 only via approved spec paths/hashes.
- 2026-10-13 cleanup implementer (fresh): approved §20.6 implemented, 6894
  synthetic checks and ASAN/UBSAN pass; no original data/analyst material read.
- 2026-10-13 final verifier (fresh): 6923 data-enabled checks pass; approved
  hashes unchanged. Final capture checks pass for corner orientation, genuine
  configured PS2 strip, separate showcase rows and correct family identity.
  Tour: 153 loaded, 1 intentionally suppressed, 0 failed/blank, 14 unresolved,
  31242 frames, 0 GL errors; exit 3 remains due to unresolved textures. Incidental
  Xbox lower-text clipping retained as presentation limit. Report in ignored
  analysis/evidence/round4_final_verification.md. No contamination reported.

## User clarification — PC pills restoration, 2026-10-13
The user clarified that only PC icons were to be removed; restore the original
PC pill buttons with text labels and no PC/mouse/key glyphs. Restore original
PC presentation and clickable action slots; preserve original-only asset policy,
corner correction, and separate genuine console icon profiles. A fresh reviewer
will reconcile the prior blanket suppression requirements before a fresh
implementer applies the correction from approved spec only. No new original
binary analysis or game execution is required for this narrow correction.
- Fresh PC-pill restoration reviewer approved SPEC.md
  6a1fc9b2750614bd2d030e732cbbb0f1a76ce7204dbda7ec9b96a12b3e766fec,
  AMENDMENTS.md 7477298e5c08766e5df42d22872c3177255ee79cc9e5892b7e0e88c729d18f58,
  RESTORATION_PC_PILLS.md b4f8f9cda6b6ee9d4df994240e7a4cf92ce9a20a947aa61127ca4b314f1e3be3.
  Fresh implementer received reviewed paths/hashes and permitted baseline only.
- Fresh restoration implementer: approved §20.7 implemented; 7048 synthetic
  checks pass, PS2 RGBA hash unchanged. Only reviewed spec/independent baseline
  read; original localization lookup keys left as Q32 rather than guessed.
- Fresh restoration verifier: 7077 data-enabled checks pass; observed original
  PC text-only Continue/Quit Game and Back/Quit Game pills, correct separate
  console presentation. Report analysis/evidence/pc_pills_restoration_verification.md.

## Demo quality correction — 2026-10-13
User requests fixing buggy demo screens and examples that do not follow UG2's
design language. Scope: every demo page, representative interactive/modal
states, layout/typography/action consistency using original assets and reviewed
references. Preserve text-only PC pills and original corner/controller policy.
Fresh verifier audits demo runtime behavior/captures; fresh analyst derives
behavioral design requirements from original references and rendered observations,
without implementation-code access. Review and independent implementation follow.
No original game execution; retail inputs remain runtime-only and uncommitted.
- Boundary incident during fresh demo-design analysis: analyst read shared
  spec/QUESTIONS.md and encountered implementation status/design descriptions
  and source identifiers embedded in that normally shared file. No implementation
  source inspected; no spec/code produced by this analyst. Analyst stopped;
  replacement receives only approved behavioral SPEC/AMENDMENTS/restoration
  requirements and observation evidence, explicitly excluding QUESTIONS.md.
  No existing implementation is implicated by this analyst-only exposure.
- User supplies additional original input during demo correction:
  /home/dq/Games/Need for Speed - Underground 2 (USA) Xbox.iso.
  Existing preservation authorization applies; fresh current analyst will
  record size/hash and inspect controller artwork/data without game execution.
- Additional analyst spawn returned 'agent thread limit reached'; current
  fresh analyst incorporates Xbox data work in the same analyst-only phase.
  Later roles must still use fresh contexts (CLI ephemeral sessions if needed).
- Xbox input: Need for Speed - Underground 2 (USA) Xbox.iso, 2724986880 B,
  SHA256 769d86d47798d8702cccd5d74743456aca7bac74b2042c5ed67ac6647df0fdbd;
  user-provided retail preservation input (2026-10-13). Analyst recorded original
  controller data observation, 47 genuine glyphs, BC1/BC2, source validation and
  synthetic vectors; no implementation source or original game execution.
- Demo audit verifier: all 13 pages plus modal/hover/viewport samples captured;
  behavioral defects recorded in ignored evidence and proposals. Fresh analyst
  drafted DEMO_DESIGN.md and XBOX_CONTROLLER.md; fresh reviewer gate started.
- Fresh amendment-6 reviewer: approved SPEC.md
  76125569604ff34be9a0b72f981d200b2ebdb3326af3228205c787d8e507f773,
  AMENDMENTS.md 96113c34530762775e5f07e70d7b159e3477db993d5a3b4b558c60246c289b40,
  DEMO_DESIGN.md 861f25cb31c01c7a82427bc24f8783c19a5154bd63ed9a7779389d3ab9aa1cef,
  XBOX_CONTROLLER.md 551417742c4e54876e463accaec9907cf18dff7041bd0181087dff0f7f74b776.
  Synthetic vector fields and all base resource keys verified; no contamination
  detected. Fresh implementer received approved paths/hashes and independent
  baseline only; analyst notes/original binaries excluded.
- Amendment-6 fresh implementer: all13-page demo composition/bug requirements,
  diagnostic separation and dialog ink changes; separate fresh implementer child
  used approved spec for Xbox loader/vectors only. Integrated synthetic suite
  7269 checks, zero failures; Xbox child ASAN/UBSAN reported passing checks.
  No original data/analyst material accessed by implementers. Fresh runtime
  verifier started from reviewed spec/current code and authorized original assets.
- Amendment6 runtime verifier: 7300 data-enabled checks pass; both authored
  tours: 3610 frames/zero GL errors. Package activation works, all 47 Xbox decode and
  encoded hash comparisons match; runtime 23 base glyph hashes match. One blocking
  dialog-label final-render defect recorded in ignored evidence/proposals.
- Fresh Amendment7 reviewer approved SPEC.md
  ea72cc6fa5745c517f527f5d86a540c9886a6768d3d6edf78f30c2ed9ed3cc12,
  AMENDMENTS.md 14952b119c345978ff336cac64b9bb14f73050750ae3553aeb092bdff8a58dc8,
  DEMO_DESIGN.md 5bcbbe2f98c98e3e279ddf5fbecbebbb46c4f8c9b0aeba17e2ecd2eee2122cd9,
  DIALOG_RENDER_ACCEPTANCE.md 2c9ad973c57d23216131d46caf0cc741a25c163f6a0e6bf795f682fcf3b1d709.
  Requirements approved only; dialog correction not implemented.
- User explicitly paused work and requested committing current changes.
  Coordinator records paused state and unfinished dialog defect in HANDOFF.md;
  no further implementation work proceeds. Original assets/evidence stay ignored.
