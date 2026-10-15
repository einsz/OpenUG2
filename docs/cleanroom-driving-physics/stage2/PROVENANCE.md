# Stage-2 provenance

- Date: 2026-10-15. Role: fresh cleanroom-analyst, `stage2_resistance_analyst`.
- Purpose/authorization: interoperability and preservation of the legally owned
  game, as recorded by the user and the parent stage provenance.
- Subject: owned retail `speed2.exe`, 4,800,512 bytes; SHA-256 independently
  rechecked as `f9dd86c054878ce6276beb07c1fd61874f7a1e4bf1f241b084c65b73e24168a7`.
- Permitted original inputs: executable, installed vehicle data, existing private
  analyst evidence/cache and previously saved public research notes. Public notes
  were leads only; functional findings were checked against original instructions.
- Additional user-offered decompiled-source reference was not inspected or used.
- Method: direct static byte disassembly and one-shot local PyGhidra decompilation
  of the original analysis project. No target launch, original-instruction
  execution, Wine session, VM run or network action took place in this stage.
- Ghidra RPC daemon could not bind its local socket in the sandbox. One-shot
  PyGhidra avoided the socket; this is a tool limitation, not behavior evidence.
- All executable behavior is statically inferred. Data constants are directly
  read. No runtime-equivalence claim and no guesses are normative.
- Analyst-only evidence and generators:
  `cleanroom/analysis/stage2/evidence/`, `cleanroom/analysis/stage2/scripts/`.
  The manifest fingerprints the original regions and cached decompilations;
  FINDINGS.md maps specification sections to those private records.
- Shared outputs: SPEC.md, QUESTIONS.md, this provenance, and 52 synthetic cases
  in test-vectors/s2_cases.json with acquisition/arithmetic notes in its README.
  Numeric vectors use deliberately invented car coefficients and state fixtures;
  no retail row dump, executable fragment or copied asset is in this shared stage.
- Stage-1 specification was read as an approved dependency and not modified.
  No implementation source, implementation design or verification report was
  opened. No contamination incident was observed in this analyst context.
- Analyst hand-back status: ready for a fresh reviewer gate; not approved at
  that point.
  The analyst hand-back hash is recorded in the private findings and hand-back.
- Legal sufficiency remains a matter for counsel, as stated in the parent workflow.

- Date: 2026-10-15 14:20:23 UTC. Role: fresh cleanroom-reviewer,
  `stage2_resistance_review`. Reviewed shared Stage-2 docs and all 52 synthetic
  cases, the approved Stage-1 behavioral dependency, and permitted private
  evidence for traceability. No implementation tree or reports were inspected.
- Review amendments: removed an unsupported reduction-state initialization
  default; clarified slope projection, supplied-input and host integration
  obligations; attached per-case synthetic derivation provenance. Remaining
  unknowns U2-1…U2-8 are retained. Details are in REVIEW.md.
- Validation: independently recomputed 52 cases within the published tolerance;
  this is specification consistency only, not original runtime verification.
- Status: reviewer approved SPEC.md SHA-256 `d3072359aa25bf0ef541504805f70b3579829bb9b1310cda8ba1a568286c6cb6`.
  Later SPEC.md edits invalidate approval. No contamination incident was found.

- Date: 2026-10-15. Fresh implementation role:
  `stage2_resistance_implementation`, followed by fresh independent verifier
  `stage2_resistance_verification`. Implementation input hashes were unchanged
  Stage-2 `d3072359aa25bf0ef541504805f70b3579829bb9b1310cda8ba1a568286c6cb6`
  and Stage-1 `61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb`.
  Implementer records permitted inputs and separation in IMPL_DESIGN.md.
- Verifier executed all five requested project suites, fresh standalone/adapter
  ASan+UBSan runs (leak detection off), and 177,204 independent formula/property
  checks. See VERIFICATION.md; reproducible script/logs are under ignored
  `cleanroom/verification/stage2/`. No implementation/spec/vector changes,
  original launch, original code inspection or private analyst evidence access
  occurred in verifier context. No contamination incident was observed.
- Verification status: tested spec consistency passes; no amendment required.
  Runtime differential remains unperformed. Standalone 3D behavior and documented
  planar gameplay scope are distinguished; full world-motion parity is not claimed.
