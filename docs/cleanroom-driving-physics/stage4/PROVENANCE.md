# Stage4 provenance

- Date: 2026-10-15. Fresh cleanroom-analyst session for ordinary steering/lateral
  interoperability and preservation; authorization inherited from the project
  provenance: user owns a legal install and explicitly requested Stage4.
- Subject: user's retail executable, 4,800,512 bytes, SHA-256
  `f9dd86c054878ce6276beb07c1fd61874f7a1e4bf1f241b084c65b73e24168a7`.
  Original car data from that same installation. File hash verified in this pass.
- Read only allowed prior approved specifications/process provenance and
  analyst evidence. No implementation, design/audit notes, verification reports
  or forbidden source tree were accessed. No contamination incident identified.
- Supplemental user-provided decompiled physics reference was used as
  analyst-only leads. Relevant claims/constants were cross-checked against the
  identified executable via fresh original-byte disassembly, existing cached
  decompilation and fresh offline Ghidra decompilation. Reference paths, hashes,
  addresses, original identifiers and raw material remain analyst-only.
- Ghidra RPC status/start was attempted. Java configuration initially failed;
  after setting the installed JDK/headless configuration, socket binding was
  denied by the sandbox. Offline Ghidra project reads/decompilation succeeded.
  No original game executable/instructions were executed. No Wine, network
  gameplay, VM or runtime process was launched.
- Every executable requirement is statically inferred; numerical parameter
  reads are static data facts. Vectors are derived synthetic component cases,
  with declared ideal-trig approximations and tolerances. They establish no
  retail runtime/differential or perceptual parity.
- Analyst evidence and derivation scripts: `cleanroom/analysis/stage4/`.
  Traceability IDs E4-1…E4-8 map privately to exact byte/disassembly evidence.
- Prior Stage1/2/3 specification files/hashes were not modified. This Stage4
  SPEC.md requires a fresh reviewer gate before an implementer reads it.
- This record describes technical separation, not legal sufficiency; legal
  assessment remains a question for counsel.

## Fresh reviewer gate

- UTC: 2026-10-15T15:31:32Z; role: reviewer in a fresh context. Reviewed Stage4 shared
  artifacts and approved Stage1/2/3 companion specifications; consulted private
  evidence only for traceability. Did not read implementation, design/audit
  material or verification reports, and did not launch the original executable.
- Cleaned behavioral wording and clarified composition, basis, numeric facts and
  predicate coverage as recorded in REVIEW.md. No contamination incident found.
- Independently checked 71 existing synthetic vectors; added one synthetic lock
  predicate case, yielding 72 reviewed cases. No runtime parity claim.
- Approved SPEC.md SHA-256: `02144d187323c6d74a6fb21e6e0b3e2842097cd0e28c935eea97907c1cbc911a`.
  Later edits invalidate approval; all remaining unknowns stay explicit.

## Fresh implementation and verification round

- Date: 2026-10-15. Separate fresh roles `stage4_lateral_implementation` and
  `stage4_lateral_verification`; coordinator passed approved specification paths,
  hashes and permitted independent baseline. Implementer input/access separation
  is reported in IMPL_DESIGN.md, not independently established by filesystem audit.
- Verifier independently confirmed all four approved hashes, executed ten safe
  project suites, strict Stage4 components and fresh ASan/UBSan adapter, and ran
  366,506 independent mathematical/property checks. No implementation, approved
  specification or shared vector edited; no original code/evidence consulted.
- Final debug/GLES builds and native process exit are implementer-reported.
  Verifier inspected completed final native log and end PNG independently;
  artifact hashes and clean repro/logs stay private in
  `cleanroom/verification/stage4/`. Existing legal assets were used only by the
  OpenUG2 loader. No original executable launched or retail runtime captured.
- No divergence, contamination incident or behavioral amendment identified in
  this verification context. Q4-I1/I2 and all approved producer/precision/runtime
  unknowns remain open. See VERIFICATION.md and VERIFIER_AMENDMENTS.md.
- Technical separation and passing derived vectors are not legal sufficiency or
  retail parity; the legal assessment remains a question for counsel.
