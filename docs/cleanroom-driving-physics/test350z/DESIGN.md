# Independent provisional configuration design

Implementation inputs: approved `test350z/SPEC.md` SHA-256 `9008ae923699cc529220e89a979683af0b5940926d76763cab0565969c6cb28f`, effective companion SHA-256 `2db456b62e6711a6d4c487688f94430e4c614c8dceebd6bce62ea0b204f8513c`, reviewer-listed hashed synthetic vectors, and the coordinator-authorized independent baseline. Both spec and auxiliary hashes were checked before implementation. No original program/source, analyst evidence or provenance was accessed. Local assets were used only through independent approved loader/application paths for validation; raw records were not inspected.

- Local asset interpretation/host loader contract: `src/test350z.c` walks an explicitly selected uncompressed stream with a 128 MiB host limit and depth limit 16. It rejects incomplete framing/records, invalid indices, duplicate/ambiguous presets and disagreeing parsed package copies. The independently existing car-table locator supplies the unique base row. Package tables are compared as zero-initialized semantic records, including only the used contribution channels. Read-only parsing never mutates source bytes.
- Package composition/E0: two separately ordered applications derive level-major and category-major masks. Activation compares every flag. Table-level missing rows are permitted. Runtime profiles retain the locally derived mask, not a hardcoded selection or population replacement. Inspected-source count 34 is checked only after SHA-256 identification; other source identities carry an explicit warning.
- E1–E4: independent scalar transformations work on an isolated typed-access subrecord. Weights round each addition to float; retained means and transformation intermediates use double; explicit effective stores use float. Integer count and opaque compatibility fields retain their bit patterns. Source target offsets remain relative to row+0x110; transforms never write stock or source data.
- E5: effective parameters are decoded once into a complete set of Stage1–5 consumer structures, with consumer-domain validation. The runtime drivetrain secondary channel is zero because E4 already composed it; the retained secondary remains in the local effective record. Existing finalization runs on this final consumer record and on fresh drivetrain initialization, rebuilding shift and engine caches. Body inertia uses the effective mass/dimensions and supplied yaw scale. Active tuning entries and instance extra flags are absent by construction; there is no interface that silently accepts unsupported tuning/extras.
- E6: a separate scoring curve uses final primary torque, retained pre-shift rpm, exactly one shift decision, final trim and 64-sample resampling. It reports strict class thresholds or the signed row override. The scoring curve does not replace dynamic shaft/clutch integration.
- Reversible switching/E7: `t350_apply` builds a fresh complete Stage1Drive and publishes it only on success. Main keeps an isolated stock configuration created through the existing stock loader. F7 reconstructs/reset state when returning to stock and resets host velocity/ride/simulation timing. Car swaps remove the provisional profile; unsupported drag/drift modes explicitly remove it before using their existing host family. Failed activation preserves the previous active instance. Ordinary defaults and assets/preferences/saves are not rewritten.
- Trace contract/E7: JSONL metadata includes explicit provisional identity, source identity, both spec hashes, full component set, all computed weight/mean/grade channels, entry policy, assumptions, actual supplied host/transmission/assistance settings, final consumed scalar/array summary and class/score. It does not emit raw car records, opaque bytes or package tables. CSV remains an ordinary CSV; metadata uses its own JSONL file. Initial configuration is emitted before driving; switches append new metadata.

Tests were encoded before the new API existed, producing the expected missing-header compile failure. `tools/test350z_test.c` consumes every approved vector and adds newly authored host boundary/transaction/metadata fixtures; these invented fixtures do not claim original runtime behavior. Public test/build commands are in `docs/TEST350Z.md`.

No questions were added: unknown opening caller, original mode/settings, source precedence, other entry branches/families, tuning/extras, nitrous dynamics and complete driving parity remain explicitly excluded by the approved contracts.

## Consumer-domain validation amendment V1–V3

Implementation input: approved `test350z-validation/SPEC.md` SHA-256
`3793a90991565fafed9ef0e1f92d126617415d6024b2aaaf77a983ef61ffe41c`
and approved `test-vectors/validation.json` SHA-256
`35f398a9fedb8790efae4eb97277c7f618b6b8f35fe997d870d34c9a60e0c7a8`.
Both hashes and the parent/effective/Stage5 spec hashes matched the approvals
before work. Only those approved contracts, the clean-room skill and the
coordinator-permitted independent implementation were read; no original program,
original data, analysis/evidence or private reports were accessed in this round.

- V1: validate the final metre-valued centre against the closed rear/front
  support interval using strict comparisons before rounding shares. Check the
  double-intermediate ratio and binary32 per-wheel shares for finiteness and
  nonnegativity. Endpoints, signed coordinates, coordinate translations and
  signed zero retain their specified meanings. The shared Stage5 preload
  denominator now subtracts in double precision, avoiding binary32 overflow for
  finite large opposing mounts; no magnitude bound or clamp was introduced.
- V2/E1/E5/E6: reject nonfinite final drivetrain shift/torque caches, retained
  secondary presentation samples, static preloads and centre-relative arms.
  Existing Stage1–5 domain checks remain in force. Scoring rejects nonfinite
  generated rpm/acceleration/resampled values and clamps endpoint queries before
  the shared interpolator's integer index conversion. This preserves the
  specified endpoint behavior for finite distant queries without integer UB.
- V2: loader/application continue to publish isolated candidates only after all
  checks succeed. Apply rechecks static support and drivetrain caches before
  constructing its fresh instance. Main's existing success-only identity/trace
  publication needed no change.
- V3: the test executable reads all 31 approved vectors as a third vector input.
  Each rejected vector is tested from stock and provisional driving snapshots;
  it checks full profile/configuration/cache/state preservation, fresh-candidate
  readiness, identity/success publication and fixed-input replay equality.
  Existing malformed parsing cases now use the same dual-state checks. Added
  invented host regressions exercise finite-input cache overflow, inherited
  consumer domains, nonfinite retained samples, signed geometry/travel,
  opaque nonnumeric bytes and large finite opposing mount coordinates.

All 31 cases and these regressions were encoded before the implementation
changes; the baseline run failed with 127 assertions. The final strict profile
suite and ASan/UBSan/float-cast-overflow suite pass. Leak detection is disabled
because the environment tracing setup does not support LeakSanitizer. Synthetic
results establish the approved arithmetic and new-host validation boundary;
original runtime/differential/trajectory parity remains unclaimed. No questions
were added.
