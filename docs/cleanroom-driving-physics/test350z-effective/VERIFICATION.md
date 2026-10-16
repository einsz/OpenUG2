# Provisional 350Z independent verification

Date: 2026-10-15. Role: fresh clean-room verifier. Result: approved synthetic vectors and bounded native activation checks PASS; one synthetic invalid-domain activation divergence remains OPEN for reviewer gating. No implementation files, approved specifications or shared vectors were edited. Original executable was never executed or inspected. No original runtime differential or retail parity is demonstrated.

## Approved inputs and checkpoint scope

Both SPEC hashes match: companion `9008ae923699cc529220e89a979683af0b5940926d76763cab0565969c6cb28f`; effective composition `2db456b62e6711a6d4c487688f94430e4c614c8dceebd6bce62ea0b204f8513c`. All five companion and three effective auxiliary files matched REVIEW.md. Prior approved Stage1–5 and body-response contracts are permitted dependencies.

Native tests used a private desktop executable copied after the configuration worker's successful debug build: `/tmp/test350z-verifier/nfsu2-debug`, SHA-256 `16e9722367ed232fba28b9ab5077e79a74324f8d1faf582f385af765cc9ce3db`. This copy is immutable during the tests. The body-coupling worker changed shared core sources during this phase; this native result does **not** validate the eventual combined executable or all latest core changes. A final fresh combined verification is required. The configuration-module checkpoint inspected/tested here was:

```text
5c9712ee9878fd0e353a7426bcf5f27b888fa377b20eaef31540d154ed7d4bef  src/test350z.c
d6ddb4509b2c9bfaee20fc6711c577a776799c00b5e40300c2a29cc890405df5  src/test350z.h
a47d5fdab1528ded3e1c23af88372ea23cb66b7d4cc828fd69d07d9c696c5534  tools/test350z_test.c
84203be0855210be1549cc7262fda2fd4b22d95547775eab46b2cef878186ca2  src/main.c
f43899e035b3870c99f9068a324186e9332209101a4fb7b3cc5e9fa74fed868f  Makefile
```

## Build and vector results

`make test350z-test` exit0. All six companion and 27 effective cases executed. Final output, verbatim:

```text
test350z_test: PASS (0 failures)
```

Private AddressSanitizer+UndefinedBehaviorSanitizer compilation of the same runner and all required adapters/modules passed; execution with `ASAN_OPTIONS=detect_leaks=0` exit0 and the same final result, with no sanitizer diagnostics. Leak detection was not tested. Strict module compilation with `-std=c99 -Wall -Wextra -Werror -pedantic` initially failed on preexisting shared-header unused-static and misleading-indentation diagnostics. Repeating with only `-Wno-unused-function -Wno-misleading-indentation` passed; this is not a claim of warning-free strict compilation without those exclusions.

`make body-response-test body-feedback-test drive-stage5-test contact-test lateral-test drive-stage4-test braking-test resistance-test drivetrain-test drive-stage1-test physics-clock-test` exit0. Relevant result lines, verbatim:

```text
body-response: 63 vectors, 0 failures
BR-7 dynamic feedback acceptance passed; trace: build/body_feedback.csv
Stage5 direction handoff: 123220 substep observations passed
Stage5 adapter: contact lifecycle, force accounting, handbrake/recovery, host pose, surfaces, reset/collision/clock passed
Stage5: 77 vectors + 29 surfaces, 503 checks, 0 failures
Stage4: 208 checks, 0 failures
Stage4 gameplay adapter tests passed
Stage 3: 479 checks, 0 failures
Stage 2: 348 checks, 0 failures
PASS: 3606 checks, 0 failed
Stage 1 gameplay adapter tests passed
physics_clock_test: PASS (synthetic OpenUG2 contracts; no retail equivalence claim)
```

Full debug build is configuration-worker reported and produced the copied executable above. GLES compilation is owned/serialized by that worker; this verifier did not independently run the full debug/GLES builds.

## Independent synthetic checks

Private scratch `/tmp/test350z-verifier` contains fresh mathematical Python oracles, generated fixtures, native artifacts and logs. Oracle equations are derived from approved specifications, not the implementation or original program. Component mathematical and parser tests use invented values and contain no retail tables.

```text
Independent effective differential: 1000 synthetic records, 172000 field comparisons, 0 divergences
Independent selection differential: 2000 order cases, 0 divergences
Independent parser/error matrix: 3465 checks passed; transactional failures and source immutability passed
Independent effective consumer routing: 146 assertions passed
```

Random seed3502026 exercises cross-category prerequisites/exclusions and both selection orders; category0–9 arithmetic includes out-of-range individual weights with retained in-range means, clamping, selected speed-limit flags and transmission variants. The parser matrix checks every strict prefix of a valid synthetic stream, malformed indices/categories/levels/counts, nonfinite used weights, unterminated preset fields, valid depth1/2/16 and rejected depth17, equal/disagreeing package copies, same-table duplicates, missing/duplicate preset, ignored negative descriptors, and invalid consumed domains. Failures preserve previous output and input bytes. Six CLI error cases return2 with actionable messages: missing/unknown profile, missing source, unreadable source, missing source argument and missing metadata path.

Consumer probe asserts effective mass, centre-relative mount arms, radii, dimensions/inertia, engine curve/domain/inertia, final drives/ratios/count/shift cache, five independent tyre channels, all suspension channels, steering, brakes and aero/preload at the live adapter interfaces. Runtime torque secondary is zero and final torque equals the already-composed primary once. Project boundary tests also exercise restoring stock via normal adapters, clearing prepared/dynamic state and failed application leaving the active instance intact. Host source inspection connects F7 to fresh/reset profile switching and metadata, and excludes drag/drift from provisional use. Interactive F7 input was not executed in this phase. No serialization API exists for a parse/serialize round trip; immutability and transactional properties were tested instead.

## Native stock/provisional matched smoke

Only the independent OpenUG2 executable was run against legal local assets through its normal loader; speed2.exe was never run. Two serialized runs used the copied executable, `DISPLAY=:0 SDL_AUDIODRIVER=dummy`, data root `/home/dq/Games/Need for Speed Underground 2`, `--car 350Z --track STREAML4RA --resolution 640x360 --frames 420`, and their own `--drive-audit`, `--drivetrain-trace`, `--drivetrain-profile-trace` prefixes under private scratch. Stock used `--drivetrain-profile stock`. Provisional used `--drivetrain-profile temporary-opening-350z-provisional --drivetrain-source '/home/dq/Games/Need for Speed Underground 2/GLOBAL/GLOBALB.BUN'`.

Both processes exit0, with420 active telemetry rows each; every numeric field is finite. The loader reports identical initial road spawn/heading, and all420 tick/pedal/steer/handbrake rows match. First CSV positions may already reflect distinct suspension response; equality is asserted at the host loader spawn, before dynamics. Engine torque and subsequent speed differ, demonstrating applied configuration distinction. Both summaries report `nan=0 oob_z=0 oob_xy=0`. End PNGs were visually inspected: the car is rendered on the road.

| Observable | Stock | Provisional |
| --- | ---: | ---: |
| Exported forward speed range, m/s | -0.0009904…17.6120 | -0.0024098…19.8748 |
| Host height range, m | 4.4441…7.5307 | 3.6575…7.5371 |
| Finite active CSV rows |420|420|

Provisional metadata has exact inspected source SHA `07e1c37a2e5ba4b3e6a17b257e653409c31d5f0b3f9df3464aec36adbbced9d0`,34 derived flags, policy `reconstruct-ten-category-grades`, no active manual entries, zero extras, ordinary-family assumption, actual host selectors and both spec hashes. It explicitly says parity unproven. Effective consumed mass1.36517513 tonnes, six forward gears and rpm[800,7500,8000] are reported. Independent mathematical recalculation from the **emitted consumed summary** agrees on inertia and performance curve:75 generated samples, score477.9224853515625, class1. This check does not independently re-extract proprietary asset targets. Stock metadata remains stock with zero provisional flags/assumptions. No stock masquerading as upgraded was observed for this actual local source.

## Open divergence and limits

`analysis/verification-2026-10-15-test350z.md` contains proposed behavioral amendment/vector V350-1 for reviewer gating: synthetic centres beyond the front/rear axle support interval activate despite negative Stage5 static load shares. This is a host invalid-domain validation issue; the actual inspected local source activation succeeds and is not this invalid synthetic case. No concrete implementation fix was given.

Original runtime parity, exact opening setup/mode/tuning/load precedence, long-duration/all-world stability, every runtime switching interaction, alternate tuning/extras/families, nitrous dynamics and eventual combined body-coupling executable are unverified. Agreement to static-inferred vectors and finite native smoke does not establish original handling parity. Legal sufficiency remains a question for counsel.
