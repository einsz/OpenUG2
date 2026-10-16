# Body-coupling verification

2026-10-15, fresh clean-room verifier. No implementation files were edited. The original retail executable was not executed. The clean-room skill was read; this verifier is on the analyst side and reports behavioral findings only.

## Scope and provenance

Confirmed approved SPEC.md SHA-256 `b09041f16661995375af617766d672db594c27a5bcbe2e8e5783c43b156d6f33`, companion body-response `3a415ca2c95941f3f5c02749e444ec566e48c2e818adacefc0d953eed23a0b8e`, temporary profile `9008ae923699cc529220e89a979683af0b5940926d76763cab0565969c6cb28f` and effective composition `2db456b62e6711a6d4c487688f94430e4c614c8dceebd6bce62ea0b204f8513c`. Read coupling REVIEW.md and all seven approved vectors. Current independent source was permitted for API/build inspection; no original code, disassembly or private original evidence was needed. Mathematical expected values are synthetic derivations from the approved laws, not original runtime outputs.

## Project commands and verbatim results

`make body-coupling-test body-coupling-host-test body-response-test body-feedback-test test350z-test` exited 0. Build log is private `/tmp/combined350z-verifier/project-tests.log`. Existing shared-header misleading-indentation warnings were emitted; no build failure occurred.

```text
BC-1..3: seven approved body-coupling vectors passed
BC host replay: 112 preparations matched independent five-pass callback
body-response: 63 vectors, 0 failures
BR-7 dynamic feedback acceptance passed; trace: build/body_feedback.csv
test350z_test: PASS (0 failures)
```

`make drivetrain-test resistance-test braking-test lateral-test contact-test drive-stage1-test drive-stage4-test drive-stage5-test physics-clock-test` exited 0; private log `/tmp/combined350z-verifier/companions.log`.

```text
PASS: 3606 checks, 0 failed
Stage 2: 348 checks, 0 failures
Stage 3: 479 checks, 0 failures
Stage4: 208 checks, 0 failures
Stage5: 77 vectors + 29 surfaces, 503 checks, 0 failures
Stage 1 gameplay adapter tests passed
Stage4 gameplay adapter tests passed
Stage5 direction handoff: 123220 substep observations passed
Stage5 adapter: contact lifecycle, force accounting, handbrake/recovery, host pose, surfaces, reset/collision/clock passed
physics_clock_test: PASS (synthetic OpenUG2 contracts; no retail equivalence claim)
```

## Independent probes

Private scratch `/tmp/combined350z-verifier/bc_oracle.py` builds inputs with seed 35020261015 and computes BC equations independently in Python binary64 from binary32 supplied state. It calls a privately compiled shared instance of the independent drivetrain/brake modules. It exercises four nonzero force pairs, signed steering bases, both mount coordinates, positive mass/inverse inertia, prepared x/y force and large excluded external moments. Its final accelerations are rounded to binary32 and compared using BC-3 tolerance.

```text
Independent BC arithmetic: 10000 cases, 40000 acceleration comparisons, 0 divergences; 10000 external-moment invariance checks passed
```

Private `host_properties.c` uses existing synthetic Stage5 fixture constructors solely as input setup, with fresh property assertions and sequences. Two dynamic instances use identical finite controls/geometry and different isolated-fixture yaw scalars (.001 and 1000). Their complete drivetrain, body and motion state match after every tick. Three independently supplied selectors 0/1/2 check basic versus adjusted local inverse yaw inertia, including nonzero-selector equivalence. Initial centre offsets are (.17,-.13,.09); 180 ticks per selector include drive, brake, sinusoidal steering and handbrake. Reset retains the dynamic path and clears initialization/preparation; the next tick prepares it again.

```text
Independent composed BC properties: 540 matched ticks across selectors 0/1/2; selected scalar and reset/reprepare passed
```

The project host replay additionally compares all five-pass results at 112 preparation boundaries including candidate orientation, nonzero centre and selected inertia. This is a project test, separately identified from the fresh independent probes above. Companion feedback tests cover dynamic pose/contact/load feedback and force latency.

## Divergences and limits

No divergence was found in this coupling scope; no new behavioral amendment was needed. The separately reported test350z invalid-centre issue belongs to its separate reviewed validation amendment and is not duplicated here. No parse/serialize round-trip interface exists for this arithmetic coupling; invariance, reset/reprepare and deterministic composition properties were used instead. NaN, overflow and degenerate orientation are outside BC-3 domain, so no invented rejection law was asserted.

Original runtime differential behavior, exact x87 arithmetic, selector producer/reachability, measured trajectories and retail handling parity remain unverified. These are agreement checks against statically inferred laws and independent integration contracts. Combined latest native application smoke and exact executable/source identity are recorded separately in `../test350z/COMBINED_VERIFICATION.md` after final build release.

## Final source checkpoint

After the consumer-domain validation worker released `src/contact.c` and profile changes, reran `make body-coupling-test body-coupling-host-test body-response-test body-feedback-test test350z-test contact-test drive-stage5-test`: exit 0, including all 31 newly approved consumer-domain validation vectors. Final log: `/tmp/combined350z-verifier/final-api.log`. Coupling/body/contact/Stage5 result lines remained those reported above. Recompiled the private composed-property executable against the final sources and repeated 540 ticks successfully; repeated the 10000-case arithmetic oracle successfully. Independent validation-amendment property/sanitizer work belongs to the separate fresh validation verifier.
