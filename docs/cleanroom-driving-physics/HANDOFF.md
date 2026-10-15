# Session handoff — driving-physics clean-room (2026-10-15)

## Current review status — 2026-10-15

Q-1–Q-16 passed a fresh specification review. Approved SPEC.md SHA-256:
`61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb`.
A fresh implementer pass and independent verifier round are complete: 3,606 harness checks,
strict C99, ASan/UBSan (leak detection disabled), and 40,011 independent property cases passed.
See VERIFICATION.md and VERIFIER_AMENDMENTS.md. No behavioral amendment was required.
Q-2 (rounding), Q-11 (general rear multiplier), Q-16 (clock/clamp), and U-1–U-16 remain as specified.
Runtime differential testing remains unperformed; derived-vector success establishes spec consistency only.
The following session notes are historical.

## Historical state
- Branch `driving-physics` (from `development` 520c3f5). **Nothing committed.** Uncommitted: `.gitignore` (adds `/cleanroom/`),
  `Makefile` (`drivetrain-test` target), `src/drivetrain.{c,h}`, `tools/drivetrain_test.c`, `docs/cleanroom-driving-physics/`.
- `src/physics.c` (arcade model) deliberately untouched; integration of the new model comes after the stages are done.
- Stage 1 (acceleration): analyst → reviewer (APPROVED) → implementer done. `make drivetrain-test` = 3434 checks, 0 failed.
  Approved SPEC.md sha256: `620bfb86586a98a62714be10c20fb4ca3f300bcbf2b6623b45700065e8e73311` (any edit invalidates it; re-review).
- Verifier step not yet run. Vectors are derived from the spec, not captured from the game (U-16), so passing = spec-consistent, not retail-verified.

## Layout
- `cleanroom/` (git-ignored, analyst-only): `analysis/notes.md`, `evidence/`, `scripts/`, Ghidra project copy, `runtime/prefix` (copied Wine prefix). Implementer must never read it.
- `docs/cleanroom-driving-physics/`: `SPEC.md`, `REVIEW.md`, `PROVENANCE.md`, `IMPL_DESIGN.md`, `QUESTIONS.md` (Q-1..Q-16), `test-vectors/`.
- Subject binary: `~/Games/Need for Speed Underground 2/speed2.exe`, sha256 `f9dd86c0…24168a7` (same as docs/cleanroom-settings).

## Staging plan
S1 acceleration (done, pending open questions) → S2 drag/rolling resistance/top speed → S3 braking → S4 steering/lateral grip
→ S5 handbrake/weight transfer (normal loads)/surfaces → integrate into `src/physics.c`.
Roles each run as a fresh subagent: cleanroom-analyst → cleanroom-reviewer → cleanroom-implementer → cleanroom-verifier.
Coordinator passes only file paths and the approved hash across the wall.

## Historical open items (superseded for question status by QUESTIONS.md)
1. **Decide:** resolve QUESTIONS.md Q-1..Q-16 via analyst→reviewer→implementer loop before committing, or commit Stage 1 and start S2.
   Notable: Q-1/Q-2 (spec readings fixed to match vectors — spec should be amended to say so), Q-4 lock correction per pass vs once,
   Q-5 stationary neutral/first hunting, Q-6 manual-assist sub-rule not implemented, Q-7 AWD slip-lock `ωw,old`.
2. Spec unknowns U-1..U-16 (menu auto/manual gear command, shift jitter stream, grip factors q/X, torque bonus flag, performance class,
   frame-time bounds, etc.) are exposed as `DrvTune` parameters with documented defaults.
3. Retail-derived vectors: PEUGOT/240SX/SKYLINE rows in `s1_params.json`, `s1_engine_torque_*`, and the three bulky
   `s1_freerev_neutral_*` CSVs. CONTRIBUTING forbids committing retail dumps; reviewer judged them acceptable but recommends the
   user decide. SYNTH_RWD covers every vector type. Note the tests currently fail if any freerev file is removed.
4. No vectors for S1-7.5 force update (spec flags it dimensionally irregular, U-9), S1-7.7 slip-lock branch, manual-assist, `kv`.
5. Runtime validation (U-16): analyst ran the game once under Wine (copied prefix, headless, memory read-only) with no VM and
   network not disabled — logged in PROVENANCE.md; reached only profile-load, captured nothing. Any future runtime capture should
   use a VM/no-network sandbox or be captured by the user.
6. Before committing: confirm no binary-derived content is in tracked paths (`git status`, `/cleanroom/` ignored), and consider a
   PROVENANCE.md entry for the implementer role (hash 620bfb86…) — implementer entry not yet logged.

## Commands
- Test: `make drivetrain-test` (optional `VECTORS=dir`).
- Provenance checklist (skill) still to finish: verifier differential results, contamination log (none so far), legal-sufficiency
  reminder (counsel).

## Opt-in gameplay integration verification — 2026-10-15

Fresh implementer role `drivetrain_game_integration` and fresh verifier role
`drivetrain_integration_verifier` completed integration against unchanged approved
SPEC.md hash `61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb`.
Independent result: PASS for tested contracts; 3,606 approved checks, adapter,
clock and district collision suites, 20,066 added synthetic checks, ASan/UBSan
with leaks disabled, and native 120-tick HUD smoke exit 0. No divergence or new
spec amendment required. See INTEGRATION_VERIFICATION.md for exact scope/limits.
Native smoke is an early idle/low-speed segment, not native upshift proof;
interactive lifecycle/audio quality and original-game differential remain unverified.
Implementation untouched by verifier; captures stay in /tmp. Clean repro/check
scripts and logs are ignored under cleanroom/verification/integration/.

## Stage-2 resistance verification — 2026-10-15

Fresh implementer `stage2_resistance_implementation` and fresh verifier
`stage2_resistance_verification` completed the approved Stage-2 round. Hashes:
Stage 2 `d3072359aa25bf0ef541504805f70b3579829bb9b1310cda8ba1a568286c6cb6`;
Stage 1 `61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb`.
Five project suites pass (348 Stage-2 checks/all 52 cases; 3,606 Stage-1 checks;
gameplay, clock and collision). Fresh ASan/UBSan with leaks disabled and 177,204
independent numerical/property checks pass. No amendment/new review required.
See stage2/VERIFICATION.md and stage2/VERIFIER_AMENDMENTS.md; independent evidence
is ignored under cleanroom/verification/stage2/. Verifier edited no implementation.
Standalone 3D resistance is tested; playable adapter remains an explicitly level
XY fixture with deferred vertical/contact/angular coupling. Complete Stage-2
world-motion fidelity and original runtime differential are not established.
Implementer native smoke/build results were reported, not rerun by verifier.

## Stage3 service-braking verification — 2026-10-15

Fresh implementer `stage3_braking_implementation` and independent fresh verifier
`stage3_braking_verification` completed the approved Stage3 round. Hashes:
Stage3 `d4689a16861df810159760a6a668ef3714966fd6c20a30e3b2b54f355c41c6f7`;
Stage1 `61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb`;
Stage2 `d3072359aa25bf0ef541504805f70b3579829bb9b1310cda8ba1a568286c6cb6`.
Independent six-suite run passed: 479 Stage3, 348 Stage2, 3606 Stage1 checks,
gameplay adapter, clock and collision. Independent numerical/property oracle
passed 346,974 checks; fresh strict Stage3 and ASan/UBSan Stage3+adapter passed
(leaks disabled). No divergence/new reviewer cycle required. See
stage3/VERIFICATION.md and stage3/VERIFIER_AMENDMENTS.md; private synthetic
scripts/logs are ignored in cleanroom/verification/stage3/.
Verifier changed no implementation, specification or shared vector. GLES/debug
builds are implementer-reported. The native 2100-frame audit reaches its requested
braking phase, but sampled tick1920–2070 rows show service pedal/requested torque
zero after the near-rest host policy selects reverse; it does not demonstrate
native service braking. Synthetic adapter fixtures do cover nonzero braking.
Original-runtime differential remains unperformed; no retail parity, stopping
curve, exact x87 rounding or Stage4/5 contact/lateral/handbrake fidelity claim.

## Stage3 supplemental native service audit — 2026-10-15

Fresh verifier `stage3_audit_verification` passed the early diagnostic S interval:
10,000 extracted-block gating/schedule cases, 40 identical pre-interval baseline
telemetry rows, and independent parsing of moving service pedal/torque, wheel/body
deceleration, release and resumed acceleration in the completed 420-frame native
log. Wall/rail/nonfinite/bounds flags are zero. See
stage3/NATIVE_AUDIT_VERIFICATION.md. No implementation/spec changes or amendments;
no further phase needed for this diagnostic delta. Core tests were not repeated;
build/exit0 are implementer-reported. This closes the old native-pedal evidence
gap while preserving the old report's historical observations. Original-runtime
parity, retail stopping curve and Stage4/5 behavior remain unverified.
