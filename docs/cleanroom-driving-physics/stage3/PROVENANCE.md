# Stage3 provenance — service braking

- Date: 2026-10-15; role: fresh cleanroom-analyst, stage3_braking_analyst.
- Purpose/authorization inherited from ../PROVENANCE.md: user-owned legal NFSU2
  installation; interoperability and preservation in OpenUG2; user requested
  continuation to Stage3. No licensing/DRM/anti-cheat work is in scope.
- Reference subject: speed2.exe, 4,800,512 bytes, SHA-256
  `f9dd86c054878ce6276beb07c1fd61874f7a1e4bf1f241b084c65b73e24168a7`.
  The complete executable hash was independently matched during this stage.
- Baseline supplied by coordinator: codex/driving-physics-stage3 at ace2c78.
  Stage1/Stage2 approved behavioral specifications were read; they remain
  unchanged. No implementation, implementation design notes or verification
  reports were read. Source directory listings are not behavioral evidence.
- Method: reused analyst-only Ghidra decompilation cache, independently compared
  relevant original executable bytes/disassembly and numerical constants.
  Existing stage evidence was used as navigation only. No target executable,
  original instruction harness or Wine session was run. No runtime, stopping
  distance or differential claim is made.
- User-offered supplemental reconstructed/decompiled physics reference was
  consulted solely by the analyst as candidate leads. Its identity and file
  fingerprints are private in the evidence manifest. It was not assumed to
  be an exact reference version: selected behavior/constants were checked
  against the intake executable. No original source names, paths, comments,
  code or control-flow transcription crosses into the behavioral contract.
- All normative executable behavior is marked inferred; directly read constants
  are data facts. No guessed defaults are normative. The 54 vector cases are
  newly calculated synthetic mathematical examples with numeric tolerances;
  they contain no runtime captures, original source or copied car tables.
- Analyst-only reproducibility: cleanroom/analysis/stage3/evidence/ (manifest,
  static extracts, findings), cleanroom/analysis/stage3/scripts/ (capture and
  vector calculations). Ghidra RPC status showed no daemon; cache plus direct
  bytes were sufficient. A local tooling note was saved under /tmp.
- Boundaries: ordinary service braking, supplied contact/grip/load/input states;
  handbrake production, lateral/load/surface systems, tuning and timing/runtime
  limitations remain explicit U3-1–U3-7. No forbidden implementation access or
  contamination was identified in this analyst context.
- Readiness: analyst draft ready for a fresh cleanroom-reviewer; this file does
  not approve a hash. Implementers must receive only the separately approved
  specification and listed test vectors. Legal sufficiency remains for counsel.

## Reviewer gate

- Date: 2026-10-15 UTC; role: fresh cleanroom-reviewer, stage3_braking_review.
- Reviewed every Stage3 shared document and all 54 numeric cases; consulted
  private evidence solely for traceability. Read only approved Stage1/Stage2
  behavioral companions. Did not read implementation, implementation design,
  verification reports or run the original executable/instructions.
- Cleaned the inferred/observed wording, made companion overlap and once-only
  body braking explicit, and attached per-case synthetic vector provenance.
  Independent mathematical recomputation matched every expected record exactly
  using the documented vector convention; this is not runtime validation.
- No original code, recovered symbols, executable addresses or copied original
  assets were found in the shared draft. No contamination reaching an
  implementer was identified by this review; implementer exposure was not
  independently audited. Approval and exact SPEC.md hash are in REVIEW.md.
- U3-1–U3-7 remain explicit. Legal sufficiency remains a question for counsel.

## Implementation and independent verification

- 2026-10-15: fresh implementer `stage3_braking_implementation` worked from
  approved Stage3 hash `d4689a16861df810159760a6a668ef3714966fd6c20a30e3b2b54f355c41c6f7`,
  Stage1 hash `61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb`
  and Stage2 hash `d3072359aa25bf0ef541504805f70b3579829bb9b1310cda8ba1a568286c6cb6`.
  Its input restrictions and results are self-reported in IMPL_DESIGN.md; verifier
  did not independently audit that session's accesses. GLES/debug/native build
  results are implementer-reported, not independently rebuilt by verifier.
- Fresh verifier `stage3_braking_verification` independently matched those same
  three hashes, read only approved shared contracts/vectors, independent source,
  tests/design/usage docs, existing provenance/handoff and native OpenUG2 log.
  No original executable, decompilation, disassembly or analyst-only evidence
  was accessed, and no original program/instructions or Wine was run.
- Verifier independently reran six suites (479 Stage3, 348 Stage2, 3606 Stage1,
  adapter/clock/collision passes), strict standalone compilation and Stage3 plus
  adapter ASan/UBSan with leaks disabled. A separate mathematical oracle passed
  346,974 checks. Full synthetic evidence is in ignored
  cleanroom/verification/stage3/. No implementation/spec/vector edits were made.
- No divergence/amendment or new review cycle was required. No contamination was
  identified in this verification context. Original-runtime differential,
  stopping fidelity and the native nonzero-service-pedal case remain unverified;
  precise limits are in VERIFICATION.md. Host reset/relief/contact/Space choices
  remain explicit fixtures and do not resolve retail unknowns. Legal sufficiency
  remains a question for counsel.

## Supplemental diagnostic audit verification — 2026-10-15

Fresh verifier `stage3_audit_verification` independently matched all three approved
spec hashes, inspected the independent diagnostic input block and recorded native
baseline/new logs, compiled an extracted-block schedule harness (10,000 passing
cases), and checked 40 matching pre-interval rows plus braking/release trajectory
and zero collision/nonfinite/bounds flags. No original binary/code/analyst notes
were accessed and no implementation/spec/shared vector was edited. Full evidence
and limits are in NATIVE_AUDIT_VERIFICATION.md; reproducible checks are ignored
under cleanroom/verification/stage3/native-audit/. Native process exit/build remain
implementer-reported; the verifier checked the completed evidence without a new
native run. No divergence/new review cycle required. The new log supplies native
nonzero-service-pedal evidence; retail parity/stopping fidelity remain unverified.
