# UG2 GUI clean-room handoff — paused 2026-10-13

User explicitly requested pausing work and committing the current changes.
All role agents finished. Do not resume fixes without user authorization.

## Implementation state
Independent GUI library: src/ugui/. Demo/tests: tools/ugui_*.
Current code implements Amendment 6: original-only visuals/corner transforms,
original PC text-only pills and pointer actions, selectable PC/PS2/Xbox profiles,
original console icon runtime loading, all 13 demo page demo layouts, F3 diagnostics,
visible actionable package example and authored-only --demo-check tour.
Original data and captures stay outside source. Do not run the original game.

Build/test/capture commands: src/ugui/HANDOFF.md. Normal targets:
make ugui-test ugui-demo. Binary: build/ugui_demo.
Latest synthetic suite: 7271 checks, zero failures. Data-enabled verifier:
7300 checks, zero failures. Both authored tours at 1280x960/1280x720: 3610 frames,
zero GL errors. Package button activation verified. All 47 Xbox decoded and
encoded hash comparisons match; runtime 23 base icons match original evidence.
ASAN/UBSAN passed with leak detection disabled; host ptrace blocks LeakSanitizer.

## Blocking unfinished fix
Enabled unfocused dialog labels remain effectively invisible on pale buttons,
at rest and focus endpoints at both sizes. Synthetic tint tests missed the
actual final-render defect. Do not claim dialog rendering fixed.
Latest reviewer approved Amendment7 with per-choice final-frame ink acceptance
and runtime-only check configuration, but no implementer has applied it yet.
Next step after resume: fresh cleanroom-implementer from current reviewed spec
only, then fresh verifier; do not relay analyst/verifier notes to implementer.

## Latest approved requirements (Amendment7, not fully implemented)
- SPEC.md: ea72cc6fa5745c517f527f5d86a540c9886a6768d3d6edf78f30c2ed9ed3cc12
- AMENDMENTS.md: 14952b119c345978ff336cac64b9bb14f73050750ae3553aeb092bdff8a58dc8
- DEMO_DESIGN.md: 5bcbbe2f98c98e3e279ddf5fbecbebbb46c4f8c9b0aeba17e2ecd2eee2122cd9
- DIALOG_RENDER_ACCEPTANCE.md: 2c9ad973c57d23216131d46caf0cc741a25c163f6a0e6bf795f682fcf3b1d709
- XBOX_CONTROLLER.md: 551417742c4e54876e463accaec9907cf18dff7041bd0181087dff0f7f74b776
All relevant vector hashes are in spec/REVIEW.md. Any spec edit voids approval.
Implemented Amendment6 SPEC hash was
76125569604ff34be9a0b72f981d200b2ebdb3326af3228205c787d8e507f773.

## Evidence and remaining limits
Ignored analysis/evidence/demo-audit-2026-10-13/: before captures/proposals.
Ignored analysis/evidence/demo-fixes-verification-2026-10-13/: final audit report,
captures, dialog failure and original Xbox hash comparisons.
Original runtime sources and hashes: PROVENANCE.md (includes new Xbox ISO).
Console action bindings and exact retail glyph baseline/size remain unobserved;
--bind configurations are demo configuration. Broader Xbox/PS2 formats unknown.
Q32 original navigation localization lookup keys remain unspecified; English
baseline text restored. Earlier 14 unresolved original-package texture screens
remain outside authored-demo pass. No original runtime parity claim.

## Walls/provenance
Fresh role contexts required at every phase. Implementers receive approved
spec paths/hashes and independent baseline only. Analyst-only boundary incident
and remediation logged in PROVENANCE.md; no implementation contamination found.
Legal sufficiency is for counsel. Commit excludes original retail data and all
ignored analysis material, screenshots, binaries and build outputs.
