# Consumer-domain validation verification

2026-10-15. Fresh cleanroom-verifier context. Implementation was released stable by its worker before any tests. No implementation edits, original source, asset reads, or original executable runs occurred in this verification. Scratch harnesses and logs: `/tmp/test350z-validation-verifier/`.

## Approved inputs

Verified SHA-256 identities before testing:

- validation SPEC: `3793a90991565fafed9ef0e1f92d126617415d6024b2aaaf77a983ef61ffe41c`
- parent test350z SPEC: `9008ae923699cc529220e89a979683af0b5940926d76763cab0565969c6cb28f`
- parent effective SPEC: `2db456b62e6711a6d4c487688f94430e4c614c8dceebd6bce62ea0b204f8513c`

Verified auxiliary validation.json hash: `35f398a9fedb8790efae4eb97277c7f618b6b8f35fe997d870d34c9a60e0c7a8`.

Validation's 31 synthetic vectors are reviewer-authored mathematical inputs, not retail captures. Differential comparisons here mean approved mathematical oracle versus new host, not original-runtime comparison.

## Results

`make test350z-test contact-test drivetrain-test braking-test resistance-test lateral-test drive-stage5-test` exited 0. The test350z command includes all 6 parent composition vectors, 27 effective cases, profile metadata, all 31 validation vectors, and parser/activation boundaries. Relevant output verbatim:

```
test350z_test: PASS (0 failures)
Stage5: 77 vectors + 29 surfaces, 503 checks, 0 failures
PASS: 3606 checks, 0 failed
Stage 3: 479 checks, 0 failures
Stage 2: 348 checks, 0 failures
Stage4: 208 checks, 0 failures
Stage5 direction handoff: 123220 substep observations passed
Stage5 moving hb: coast 11.96928 hb 10.24024 blend 0.50000
Stage5 adapter: contact lifecycle, force accounting, handbrake/recovery, host pose, surfaces, reset/collision/clock passed
```

An independently generated private C oracle constructs the approved cases using the synthetic stream fixture, comparing exact acceptance/rejection and prior profile bytes. It independently computes per-wheel shares from final semantic coordinates with the approved 2e-7 display tolerance. All 31 cases passed. It additionally accepts 1,717 combinations: translations -50 through +50 metres, 17 centre points including both endpoints, negative centre height and negative mount height. This demonstrates no arbitrary position-positivity/magnitude rejection for that tested family.

The oracle compiles with GCC `-O2 -std=c99 -Wall -Wextra -Werror`, suppressing unused static functions and existing misleading-indentation warnings in shared headers/physics. The initial unsuppressed build stopped on those preexisting warnings; that was a build warning failure, not an observed behavioral divergence.

The independent oracle and the full project test350z suite both pass AddressSanitizer, UndefinedBehaviorSanitizer and float-cast-overflow instrumentation; the project sanitizer build produces no diagnostics and ends `test350z_test: PASS (0 failures)`. LeakSanitizer cannot run in this environment: `LeakSanitizer does not work under ptrace (strace, gdb, etc)`. Sanitizer success uses `ASAN_OPTIONS=detect_leaks=0`; leak detection remains unverified.

Reused prior verifier-authored mathematical composition/selection/parser harnesses, copied into this verifier's scratch directory and linked against a freshly rebuilt shared library. Results verbatim:

```
Independent effective differential: 1000 synthetic records, 172000 field comparisons, 0 divergences
Independent selection differential: 2000 order cases, 0 divergences
Independent parser/error matrix: 3465 checks passed; transactional failures and source immutability passed
```

The project rejection harness tests every rejected validation vector against prior stock and provisional states, preserving complete profile/configuration/cache/state snapshots, rejecting fresh ready indications, suppressing simulated successful activation publication, and replaying identical fixed driving inputs. Parser/composition failures are covered likewise. These are component/host-boundary simulations; a full native application's switching UI and trace output are not independently exercised here.

## Findings and limits

Prior proposed V350-1 is corrected for tested cases: both outside-support centres, both nearest binary32 outside neighbours, and both category-0 composition crossings reject; endpoint zero-share cases and nearest inside neighbours accept. Zero/reversed wheelbase, all nine coordinate nonfinite cases, invalid final mass and derived inertia overflow reject. No divergence or new proposed amendment was found, so no new amendment file was created.

No exhaustive valid-domain acceptance proof is claimed. The tests retain signed positions/travel/progression, opaque-byte treatment and conditional weight behavior through the approved suite and randomized arithmetic comparisons. Original invalid-input handling, retail trajectories/parity, exact opening setup, extended precision equivalence, full native integration and leak freedom remain unverified. Native/app and combined body-coupling checks belong to the separate combined verifier.

## Tested source identities

```
41987c66ed4f2c4ebba81eea2f95e5f327b5a3f46a4923ff6019ae3a3576f44e  src/test350z.c
7bc3fdef86babe5e0446d2962157ca3f0a28c329fab93d25bac124c2cb7716b7  src/contact.c
228e38d3452af5dc86faa1eb10068266f5d1521685c2efc75be7f52bc5ca4c64  tools/test350z_test.c
b36a481f892b59de3b1f17630d2a9393b526ebd1a09b6775cfa62623f9f8d6c0  Makefile
```

The four identities remained unchanged after the component/oracle run. No commit or push was performed.
