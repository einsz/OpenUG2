# Provenance — driving physics clean-room project

- Branch: `driving-physics` (from `development` 520c3f5).
- Purpose: interoperability/preservation. Recreate the retail PC game's car driving
  physics (longitudinal first, then lateral) in OpenUG2 from observed behaviour, without guessing.
  Current `src/physics.c` is an arcade approximation and is NOT a source of truth.
- Authorization: user owns a legal NFSU2 install and requested the analysis (2026-10-15).
- Subject: retail `speed2.exe`, 4 800 512 bytes, sha256
  `f9dd86c054878ce6276beb07c1fd61874f7a1e4bf1f241b084c65b73e24168a7` (same binary as docs/cleanroom-settings).
- Layout: analyst-only scratch in `/cleanroom/` (git-ignored: binary-derived notes, traces, scripts);
  reviewed spec, vectors, review log and provenance in this folder; implementation in `src/`.
- Roles (fresh context each): analyst → reviewer (hash gate) → implementer → verifier.
- Staging: S1 acceleration → S2 resistance/top speed → S3 braking → S4 steering/lateral grip → S5 handbrake/weight transfer/surface.
  Each stage = separate spec section + vectors + approved hash + implementation + verification.

## Role log
(append: date, role, agent, stage, approved hash)
- 2026-10-15 ANALYST (cleanroom-analyst, fresh) Stage 1 acceleration: static analysis + data files. Also launched
  speed2.exe once under Wine 10 (copy of user's prefix in cleanroom/analysis/runtime/prefix, headless gamescope, audio off,
  memory read-only harness) to try to capture rpm/gear/speed; reached only the profile-load screen, no data captured or used.
  NOT run in a VM/snapshot and network was NOT disabled (deviation from workflow safety guidance). No writes to the game
  folder or original prefix observed. Public notes from troikoss/nfsu2-re were used as address leads only.
  SPEC.md sha256 at hand-back: a6254518ec49d3d5a8fa474ea349b30a3f0d6f8a69606fb4050f9e1efcdc7308 (unreviewed).
- 2026-10-15 REVIEWER (cleanroom-reviewer, fresh) Stage 1: APPROVED, SPEC.md sha256 620bfb86586a98a62714be10c20fb4ca3f300bcbf2b6623b45700065e8e73311 (see REVIEW.md).

- 2026-10-15 ANALYST (fresh cleanroom-analyst, driving_questions_analyst): examined Q-1–Q-16,
  original executable hash verified equal to recorded intake. Used existing analyst Ghidra evidence
  and direct static byte disassembly; no retail process launched, no runtime captures claimed.
  Amended SPEC.md and question statuses; added explicitly derived synthetic regression examples.
  Q-2 intermediate rounding, Q-11 general rear-grip context, Q-16 end-to-end timing remain partial.
  No implementation files or IMPL_DESIGN.md accessed. Evidence/commands under
  cleanroom/analysis/evidence/questions-2026-10-15/ and cleanroom/analysis/scripts/questions_*.py.
  Prior spec approval is stale; fresh reviewer gate and later runtime verification are required.
  Analyst hand-back SPEC.md SHA-256: `202c817768242d63fa0533eae4f6c7873ea7e2efb4dfb8e36322869ce6c6ceb2` (unreviewed).

- 2026-10-15T13:35:25.760343+00:00 REVIEWER (fresh cleanroom-reviewer, driving_questions_review): reviewed Q-1–Q-16
  amendments and all implementer-visible spec/vector/shared documents except IMPL_DESIGN.md.
  Consulted analyst-only findings for traceability; no implementation or executable run/read.
  Naming hygiene, explicit assist initialization unknown, vector tolerances/entry conditions and historical
  handoff status clarified. All 18 synthetic question cases checked against spec arithmetic/predicates;
  no runtime or differential verification claimed. No confirmed contamination incident identified.
  APPROVED SPEC.md SHA-256: `61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb`. Q-2, Q-11, Q-16 partial; U-1–U-16 retained.

- 2026-10-15 IMPLEMENTER (fresh cleanroom-implementer, driving_amendment_implementation), coordinator-relayed report:
  worked from approved hash `61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb`;
  reported changing only src/drivetrain.c, src/drivetrain.h, tools/drivetrain_test.c and IMPL_DESIGN.md,
  with no forbidden access. Reported 3,606 checks, strict C99, ASan/UBSan passing and LeakSanitizer
  unavailable due to the ptrace environment. Access history and changed-file scope are implementer-reported,
  not independently established by this verifier.
- 2026-10-15 VERIFIER (fresh cleanroom-verifier, driving_amendment_verification): independently checked
  approved spec hash, reference intake hash, all 3,606 harness checks, strict C99 compilation and
  ASan/UBSan execution (leak detection disabled), plus 40,011 independent deterministic numerical property
  cases. No implementation/spec edits, no original executable launch, no disassembly inspected.
  Harmless namespace isolation probe failed; runtime differential testing unperformed. Vectors remain
  spec-derived, not runtime captures. No behavioral amendments proposed and no new contamination observed
  in this context. Reports: VERIFICATION.md, VERIFIER_AMENDMENTS.md; logs in cleanroom/verification/.

- 2026-10-15 IMPLEMENTER (fresh cleanroom-implementer, `drivetrain_game_integration`),
  coordinator-relayed integration role: approved Stage-1 hash
  `61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb` and explicitly
  permitted independent application baseline. Access history is role-reported;
  no original/analyst access reported. See IMPL_DESIGN.md for implementation scope.
- 2026-10-15 VERIFIER (fresh cleanroom-verifier, `drivetrain_integration_verifier`):
  hash independently matched; four focused test targets passed (3,606 approved
  checks), plus 20,066 independent synthetic integration checks in strict project
  warning policy and ASan/UBSan (leak detection off). Native 120-tick Stage-1/HUD
  smoke exited 0 and endpoint image inspected; art decode limitation recorded.
  No implementation/spec edits, no original executable/Wine/disassembly or analyst
  evidence accessed. Retail differential unperformed; no behavioral divergence or
  spec amendment proposed. See INTEGRATION_VERIFICATION.md and ignored
  cleanroom/verification/integration/ for reproduction. No new contamination observed.
