# Review log (spec/)

## 2026-10-13 reviewer pass
Contamination scan (regex plus full read of SPEC.md and test-vectors): no
addresses, symbol/mangled names, decompiler fragments, source paths or
register material. Regex hits were format constants (hash/chunk/flag values,
flagged interop) only. Test vectors are synthetic; hash vectors independently
recomputed and all match; made-up labels do not appear in analysis/evidence.
No retail assets, tables or bytes embedded.

Changes:
- Branding/UI text: removed a retail product-brand label in hint tables (generalised).
- Cross-references: fixed wrong section numbers in test-vectors README and track vector note.
- References: added research-only note for the public FEngLib citation (no names to be reused as design).
Retained, noted: font family identification (8.1, factual, substitute advised);
7.4/7.7 label and hint lists are short paraphrased functional lists, not dumps.
No contamination reached an implementer as far as this review can tell.

Unknowns: present in section 11 and marked G/I per requirement; accepted.

APPROVED 2026-10-13 (UTC), role: reviewer
sha256 SPEC.md: 92c9026fe6747f4272a2c6086a744c42c7eee649b3d0c64df51ade8a8f9924b8
(Any later edit to SPEC.md invalidates this approval.)

## 2026-10-13 reviewer pass, round 2 (amendment 2)
Scope: SPEC.md (incl. Part II sections 13-18 and edited 1.4, 3.2, 3.5.2,
3.5.4, 6.9, 6.11, 6.13, 7.1, 8.1, 8.4, 11, 12), AMENDMENTS.md, QUESTIONS.md,
test-vectors/ (all files).
Contamination scan: regex plus full read. No executable addresses, symbol or
mangled names, decompiler/disassembly fragments, register material, or
source-file paths. Regex hits are format constants (chunk/tag ids, hashes,
flag bits, the FEngLib commit hash) and 2D coordinate names in a vector.
Retained as functionally required interop names, flagged as such: chunk/tag
magics, message hashes, glyph token names, font names and file names in 14.2,
15.9, 16.4 (lookup keys), texture-icon name families, screen package names in
18 (capture targets). Retail-derived counts (e.g. glyph/pair counts) are
facts, not assets. No embedded retail bytes, images or tables: the 15.7
palette notes and 16.x metrics are formulas/short lists. Section 7.4/7.7
lists remain short paraphrases.
Vectors: all synthetic (made-up names such as SYN_*, Demo_*; printable
strings in every hex file inspected). Independently re-verified with fresh
decoders written from SPEC 13.2/13.3/2: 4/4 JDLZ cases, 2/2 HUFF cases, all
hash vectors, and texture keys for SYN_PALETTE and SYN_BGRA match.
DXT/font/tpk pixel data were not re-derived (accepted from analyst, +-1).
Changes:
- Testability/cross-reference: section 18 referenced undefined A-numbers (and
  section 11 uses them); added a sentence defining them as verifier-side
  tracking ids tied to section 11 Unknowns.
- References: confirmed research-only wording for FEngLib and psx-spx
  (no names reused as design); 13.3 is described in the spec's own words.
Notes for the coordinator (no action taken by reviewer):
- Section 12 and the vectors README cite repository files docs/FORMATS.md
  and tools/jdlz.py. The spec does not depend on them (13.2 is
  self-contained). If their provenance is unclear, log it in PROVENANCE.md.
- QUESTIONS.md quotes the round-1 hash; it is the implementer's log and is
  superseded by AMENDMENTS.md answers. It contains no contamination.
- Round-1 hash 92c9026f... is stale. Implementer code written against
  round 1 must be re-checked against the amended sections (1.4, 3.5.2,
  3.5.4, 6.9, 6.11, 6.13, 7.1, 8.x). No contamination reached an
  implementer as far as this review can tell.
Unknowns: section 11 items 1-17 are explicit and marked; accepted.
APPROVED 2026-10-13 (UTC), role: reviewer
sha256 SPEC.md: 3a6b4b3f25c3bdfe20337f44f2193e01c43a6ed9bdbbb2a2100f779d0ed8cd38
sha256 AMENDMENTS.md: 102c07daf3d1ba9211c7a2255978ce939e498dcda84abc0f8d2b97a84c99ac15
(Any later edit to SPEC.md or AMENDMENTS.md invalidates this approval.)

## 2026-10-13 reviewer pass, round 3 (amendment 3)
Scope: SPEC.md draft 3 (new 19; edited 1.1, 4.5, 4.6, 6.4-6.6, 6.9, 6.11,
6.17, 6.21, 7.1, 7.4.1, 7.6, 8.1, 11, 16.5, 18), AMENDMENTS.md (Amendment 3).
Test vectors unchanged since round 2 (not re-reviewed beyond regex scan).
Contamination scan: regex plus full read of SPEC.md and AMENDMENTS.md. No
addresses, symbols, mangled names, decompiler/disassembly or register
material, source paths. Regex hits are format constants and 2D coordinate
names. No embedded pixels, image data or retail asset content: the capture
set (19.1) is described only by screen/state and measured geometry numbers
(facts). Retail strings are limited to short functional pill/label names and
interop tokens (7.3, 7.4, 7.7, 15.9). No text turns the implementation's own
renders into design instructions; 19.2 compares against the original's
measurements only. Captures are referred to as user-supplied, analyst-side
evidence; no file names or paths of the images appear.
Changes:
- Cross-reference: 16.5 pointed to a non-spec file name for a tracking id;
  reworded to the section 18 verifier tracking id A13.
- Testability/tagging: 19.2 lacked an explicit confidence tag; added a line
  stating its values are R (measured) and the list is a verification aid.
Checked: numeric self-consistency (thumb formula vs 273.8 / -182.4; row
centre -126 vs backing y; button extents vs button row centre y; dialog
button y ranges), unknowns list count (19) matches 11, caveat on modified
capture install carried in 1.1, 11 item 12, 19.1.
Remaining unknowns: 11 items 1-19 (marked, accepted).
No contamination reached an implementer as far as this review can tell.
APPROVED 2026-10-13 (UTC), role: reviewer
sha256 SPEC.md: 49d49bf04dfe468c8332f69cd18d30ffcd939a271e637abdb266e3df71c278df
sha256 AMENDMENTS.md: 47ff48f467ee412c7a350fbfdefd4e87de5a7fa0b709ea83fe643bb9e494f900
(Any later edit to SPEC.md or AMENDMENTS.md invalidates this approval.)


## 2026-10-13 reviewer pass, round 4 (amendment 4)
Scope: full SPEC.md, AMENDMENTS.md, QUESTIONS.md, REVIEW.md and all test
vectors. Only analysis/evidence/round4_findings.md and
analysis/evidence/ps2_controller_round4.json consulted for traceability;
no implementation or original executable/code was read.

Contamination gate: regex scan plus document/vector inspection found no
original executable addresses, recovered symbols, decompiler/disassembly,
register sequences, original source paths, copied artwork or wholesale retail
tables. Numeric regex hits are required format/hash/flag facts; coordinate
names are synthetic. QUESTIONS.md references this independent implementation's
own tuning header and decisions, not recovered original identifiers. Retained
19 PS2 names/keys are explicitly flagged runtime lookup facts [interop].
Swizzle and quaternion equations describe data mapping/observable geometry,
not original code structure. No contamination incident identified; this review
cannot independently establish what earlier implementer sessions accessed.

Changes by category/location:
- Consistency: SPEC status now points to this approval; 11 items 7/17 now
  distinguish verified PS2 icons from remaining Xbox/binding/format unknowns.
- Requirement precedence: 6.17/6.20, 10 item 9, 14.2 and 16.4 remove stale
  placeholder, pill-animation and texture/font substitution permissions.
- Testability: 19.2 marks its PC strip/pill checks as historical and superseded
  for current presentation, preserving the original PC observations.
- Dimensional clarity: 16.5 scaled block height includes scaled cell height.
- Vector correction: font_layout_vectors.json records cell height 21,
  leading 0 and provenance; AV^VA second-line quad y extents corrected to
  25..39 from the synthetic font header, resolving Q28. README documents it.
- Provenance wording: README distinguishes synthetic records from the short
  required interoperability constants/tokens they contain.
No removed sensitive content is reproduced in this log. AMENDMENTS.md unchanged.

Validation: all JSON parses; independently computed quaternion rotation matrix
matches 8/8 corner vectors. PS2 formula is a bijection over all 1024 nibble
positions and reproduces every expected index, all seven RGBA/address samples,
and the complete decoded RGBA SHA256. All 4 font layout vectors recomputed
from synthetic font_chunk.hex glyph/kerning metrics pass. Existing vectors
retain synthetic generation/observation or public-format provenance in README;
printable hex payload strings inspected. Retail image bytes remain outside spec.

Remaining unknowns: SPEC 11 and 20.5 remain explicit. Material limits include
no verified Xbox assets/source or decoder; unobserved retail PS2/Xbox action
binding, glyph baseline and sizing; no console execution/captures; arbitrary
PS2 formats unsupported. User-configured mappings are product configuration,
not retail claims. Historical pointer/timing, Korean strings, and other earlier
marked unknowns remain accepted as scoped uncertainties. Approval is of the
specification, not a finding that all unknown behavior was observed.

APPROVED 2026-10-13 10:21:28 UTC, role: reviewer
sha256 SPEC.md: cb1a7a83d388bb09bdc030103dc083b54ef775f8c9090489b29fc51e24767fc5
sha256 AMENDMENTS.md: 9f0e2b3c3d4f0a8fb7390ad8b0c1645ea56ef790eba9b728a63df1601b0c14c2
(Any later edit to SPEC.md or AMENDMENTS.md invalidates this approval.)


## 2026-10-13 reviewer pass, round 4 verification clarifications

Scope: full SPEC.md and amendment documents, VERIFICATION_ROUND4.md,
QUESTIONS.md and review history; regex scan of all spec files plus vector
provenance/string/payload inspection. Existing numeric/codec vectors retain
round-4 approval; no vectors were changed or regenerated in this pass.
Traceability consulted only analysis/evidence/round4_verification.md; no
implementation tree, implementation source or original executable was read.

Contamination gate: no original code addresses, recovered symbol identifiers,
disassembly, decompiler text, source paths, copied original assets or wholesale
original tables were found. Scan hits remain flagged interop format/hash/flag
facts, synthetic coordinates, and prior independent implementation question
references. The verifier's proposed command/result text concerned independent
demo diagnostics, not original executable strings. No contamination incident
identified; prior implementer exposure cannot be independently established here.

Changes by category/location:
- Behavioral testability: added SPEC 20.6 for only the four concrete verifier
  discrepancies; strip applicability is limited to menus using that strip;
  nonoverlap is defined by disjoint vertical ink extents at the reported size.
- Requirement precedence: current console profile supersedes historical PC
  assertions; intentional overlay suppression is separately classified and
  does not mask actual missing/unsupported resources or GL errors.
- Provenance/confidence: SPEC 20.6 and AMENDMENTS clarification state these are
  product/demo tooling requirements, not newly observed retail-console behavior.
- Evidence separation: VERIFICATION_ROUND4.md now contains reviewed input/output
  cases and paraphrased discrepancies; removed local capture paths, test command
  details and literal diagnostic output. No sensitive removed text reproduced.
- Amendment tracking: AMENDMENTS records the reviewed 20.6 addition.

Validation: read back all edits, all JSON vectors parse, contamination regex
reviewed; every V4 case states input, expected result and provenance. Evidence
supports the reported discrepancies through recreated demo tests/captures;
those are not original-game runtime evidence. No new codec/pixel vectors or
retail asset data added. Approval is for requirements, not completed fixes.

Remaining unknowns: SPEC 11 and 20.5 unchanged, including Xbox source/decoder,
retail console bindings/baseline/size, general PS2 formats, and unobserved
original runtime behavior. Evidence records unresolved visuals on 14 screens
and no console execution; no universal rendering-fidelity claim is approved.

APPROVED 2026-10-13 10:49:31 UTC, role: reviewer
sha256 SPEC.md: c5fa8e37e527759bcde30baf0389767a6d63d8f0d29b4701602279cac2b13c34
sha256 AMENDMENTS.md: d7d113dd93b36ad310b06d3cfcb56f5517df21cfda68790e81872c3668de406b
sha256 VERIFICATION_ROUND4.md: f211dcf52b224234cb262954e7f36276fbe7344fa04ae1af8b5c5e2cac1893d7
(Any later edit to an approved document invalidates its approval.)

## 2026-10-13 reviewer pass, amendment 5 PC pill restoration

Scope: spec-only correction requested by the user; SPEC presentation precedence,
7.4/7.4.1 geometry and slot rules, 7.6 historical mouse design, checklist 10/19,
20.3/20.6 and new 20.7; amendment history and relevant policy cases. No original
binary, implementation tree or new original analysis was read. Existing reviewed
codec/geometry facts and vector provenance remain unchanged.

Contamination gate: specification and vector scan hits are required format/hash
constants, synthetic coordinate names and previously reviewed independent-project
question references. No original code/disassembly, recovered debug symbols,
copied artwork or new wholesale table dumps were added. The restored short
functional PC navigation labels are explicitly flagged as interoperability text.
No contamination incident identified; this pass does not independently establish
prior implementer access history.

Changes by category/location:
- Requirement precedence: SPEC goal, 7.6, 10, 19 and 20 reconcile default PC
  text-only restoration with suppression limited to explicit console profiles.
- Behavioral testability: 20.7 and RESTORATION_PC_PILLS.md specify original strip
  geometry, slot labels, hit targets, hover/glow, once-per-click action dispatch,
  dialog hiding/restoration, profile switching and tour classification.
- Provenance: new acceptance cases identify user direction and previously reviewed
  PC observation/data facts; no new retail-console or application mapping claims.
- Vector consistency: VERIFICATION_ROUND4.md V4-1 and the policy JSON suppression
  case explicitly select console profiles. README records this amendment.
- Preserved scope: original-resource-only policy, full corner projection, PS2
  source/codec and console unavailable-family behavior remain applicable.

Validation: reviewed revised requirements and conflicts; all JSON vectors parse.
New cases contain no original asset bytes. Remaining unknowns in 11 and 20.5 are
unchanged: unobserved PC application bindings/outcomes and runtime timing, Xbox
source/decoder, console retail action map/baseline/size and general PS2 formats.
Approval covers specification requirements, not completed implementation fixes.

APPROVED 2026-10-13 11:03:44 UTC, role: reviewer
sha256 SPEC.md: 6a1fc9b2750614bd2d030e732cbbb0f1a76ce7204dbda7ec9b96a12b3e766fec
sha256 AMENDMENTS.md: 7477298e5c08766e5df42d22872c3177255ee79cc9e5892b7e0e88c729d18f58
sha256 RESTORATION_PC_PILLS.md: b4f8f9cda6b6ee9d4df994240e7a4cf92ce9a20a947aa61127ca4b314f1e3be3
sha256 VERIFICATION_ROUND4.md: fafe2978cbb647c5f4d3ba06cb236276be43471509e23e815e98ac858ae42d58
sha256 test-vectors/original_visual_policy_vectors.json: a34909997ce841a73cf6068872c74f4160245f945bf6bfff04755f303e68e42e
(Any later edit to an approved document invalidates its approval.)


## 2026-10-13 reviewer pass, amendment 6 demo composition / Xbox source

Scope: DEMO_DESIGN.md, XBOX_CONTROLLER.md, new synthetic Xbox/XDVDFS vectors,
their provenance, and reconciled SPEC/AMENDMENTS. Full SPEC read and spec-wide
contamination scan completed; earlier reviewed vectors retain their approvals.
QUESTIONS.md was read by this reviewer only as a shared question log, not used
as original-behavior evidence and not forwarded to the analyst. No implementation
tree, implementation code or original executable was read. Traceability consulted
only design-analysis-2026-10-13.md, demo-audit audit-report.md and Xbox README,
inventory.json/icons.json under analysis/evidence.

Contamination gate: no original code addresses, debug/symbol identifiers,
decompiler/disassembly, original source paths, copied assets or wholesale tables
in the new material. Required numeric hash/chunk keys and explicit member/data
offsets are interoperability facts, not executable addresses. The short glyph
category list gives lookup facts, not retail input binding. Synthetic BC and
directory bytes contain no disc art. No contamination incident identified; this
review does not independently establish earlier implementer exposure history.

Changes by category/location:
- Requirement precedence: SPEC 11/20.1/20.3/20.5/20.6/20.8 and amendment 6 now
  distinguish earlier PC/PS2-only inputs from the newly verified Xbox source.
  Gallery availability does not require claiming unknown retail action mappings.
- Accuracy: corrected one base gallery key transcription in XBOX_CONTROLLER X4
  against the evidence record for direction pad right; no removed sensitive
  material reproduced here. All 23 listed keys now match observed records.
- Testability/status: reviewed status lines, D1 title-anchor tolerance, and
  vector README cross-references; 4x4 primitives are clearly synthetic codec
  tests, not observed retail glyph dimensions.
- Preserved policy: runtime original-only resources, PC text-only desktop
  default/pills/dialog hiding, one active strip, full projected corner transforms
  and original-screen authored geometry remain authoritative.

Validation: all spec JSON parses. Independently recomputed both artificial BC
block pixel results, 32x32 minimum byte lengths, record/descriptor arithmetic,
member bounds and XDVDFS TEST directory fields/offset plus invalid child/name
bounds. Confirmed evidence census 47 glyph records, 46 BC2/one BC1, all 32x32,
and all 23 base keys. Draft input hashes matched analyst no-edits confirmation.
No decoder implementation or original pixels were executed/read for validation.

Meaningful limits: no original game execution or console screenshots; retail
console action-token maps, exact glyph baseline/size, additional Xbox compressed
sources/mipmapping/descriptor meanings and other PS2 formats remain unknown.
Demo composition choices are D requirements, not retail parity claims. Six PC
stills cannot establish every demo grouping or original screen; genuine 3D
car/background/movie rendering remains outside scope. Earlier runtime timing,
pointer/localization and other SPEC 11 unknowns remain explicit.

APPROVED 2026-10-13 11:32:34 UTC, role: reviewer
sha256 SPEC.md: 76125569604ff34be9a0b72f981d200b2ebdb3326af3228205c787d8e507f773
sha256 AMENDMENTS.md: 96113c34530762775e5f07e70d7b159e3477db993d5a3b4b558c60246c289b40
sha256 DEMO_DESIGN.md: 861f25cb31c01c7a82427bc24f8783c19a5154bd63ed9a7779389d3ab9aa1cef
sha256 XBOX_CONTROLLER.md: 551417742c4e54876e463accaec9907cf18dff7041bd0181087dff0f7f74b776
sha256 test-vectors/README.md: 01e433b212308db3c073a779f743375762a86144c37e2acf2ce440e000df7b8b
sha256 test-vectors/XBOX_README.md: e832982bd920f1bf150906ba6e732bd1baab081f6b7fb60450d49c50e6dc3649
sha256 test-vectors/xbox_controller_vectors.json: 9fd408142a50137b0ef4485d7c2ccd20cac52b92326423e338d3e301998757a0
sha256 test-vectors/xdvdfs_vectors.json: 704e819283d9850570e000450ac2d1f876c4530d68ea180d3c4cacf8d48b8247
(Any later edit to an approved file invalidates that file approval; any later
edit to SPEC.md invalidates the SPEC gate.)


## 2026-10-13 reviewer pass, amendment 7 rendered dialog acceptance

Scope: narrow refinement of SPEC 20.9, AMENDMENTS amendment 7, DEMO_DESIGN
page-8/D3 cross-reference and new DIALOG_RENDER_ACCEPTANCE.md. SPEC reviewed
with the spec-wide contamination regex; previously approved codec/assets
requirements and synthetic vectors retain their scope. Traceability consulted
only the verifier proposal, focused verification report and recreated modal
captures under analysis/evidence/demo-fixes-verification-2026-10-13. No
implementation tree, implementation code or original executable/resource was
read. Existing vector provenance reviewed; all JSON vectors parse.

Contamination gate: no executable addresses, recovered debug/symbol names,
disassembly/decompiler fragments, register sequences, original source paths,
copied assets or wholesale original tables added. Existing regex hits are
required format/hash constants, synthetic coordinates and prior independent
project references. New source paths are user-authorized runtime configuration,
not original source-code paths or inspection instructions. Short choice labels
are explicit D reproduction inputs; full modal messages were not copied.
No contamination incident identified; earlier implementer exposure history
cannot be independently established by this review.

Changes by category/location:
- Behavioral testability: SPEC 20.9/new R2/R3 require actual final-frame filled
  glyph interiors per enabled choice, both acceptance sizes, all focus
  destinations and selected-profile coverage; state/mirror tint assertions
  cannot substitute for rendered acceptance. One-channel near-uniform labels
  and outline-only or covered labels explicitly fail.
- Requirement consistency: opaque focused pulse endpoints test visible ink;
  exact focused zero-alpha phases remain expected under SPEC 5. Every
  unfocused label stays visible even at those phases. Font/art/palette/alpha,
  timing, modal exclusivity, PC chrome and all Xbox/demo requirements retained.
- Permission boundary: new R1 lists PC/PS2/Xbox runtime paths and permits only
  recreated loader/render execution and own output measurement; direct raw
  inspection, analyst/evidence access and original execution remain forbidden
  to the implementer. Retail assets remain outside spec/repository/distribution.
- Provenance: every new vector states recreated-render observation plus prior
  approved policy; expected results are D acceptance refinements, not newly
  observed original retail behavior. No implementation architecture prescribed.
- Cross-reference/status: SPEC revision 7, amendment history and DEMO_DESIGN
  link the new normative acceptance document. No unrelated scope broadened.

Validation: new/revised material read back, new document contamination regex
clean, spec-wide regex hits classified, all existing JSON parses. Actual
recreated captures substantiate absent unfocused ink at both sizes. Approval
only establishes clean testable requirements, not that the defect is fixed.
Remaining unknowns: internal defect cause unspecified; SPEC 11/20.5 and Xbox
unknowns unchanged, including original runtime timing, retail console binding
and glyph baseline/size, further formats and general retail rendering parity.

APPROVED 2026-10-13 11:51:58 UTC, role: reviewer
sha256 SPEC.md: ea72cc6fa5745c517f527f5d86a540c9886a6768d3d6edf78f30c2ed9ed3cc12
sha256 AMENDMENTS.md: 14952b119c345978ff336cac64b9bb14f73050750ae3553aeb092bdff8a58dc8
sha256 DEMO_DESIGN.md: 5bcbbe2f98c98e3e279ddf5fbecbebbb46c4f8c9b0aeba17e2ecd2eee2122cd9
sha256 DIALOG_RENDER_ACCEPTANCE.md: 2c9ad973c57d23216131d46caf0cc741a25c163f6a0e6bf795f682fcf3b1d709
(Any later edit to an approved file invalidates that file approval; any later
edit to SPEC.md invalidates the SPEC gate.)
