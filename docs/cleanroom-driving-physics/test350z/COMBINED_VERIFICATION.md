# Combined native 350Z and body-coupling verification

2026-10-15, fresh clean-room verifier. This report covers the final combined independent host after body-response, reduced local planar body-coupling, temporary tier-2 profile/effective composition and consumer-domain validation changes. The original retail executable was never executed. No implementation files were edited by this verifier.

## Exact scope and builds

Base repository commit: `ff32818eaade94b92711ffd914af2a430644669f`, with the current uncommitted independent implementation. Final source checkpoint is a 150-file manifest of Makefile and all src C/C++/headers/shader sources: `/tmp/combined350z-verifier/source-manifest.sha256`, manifest SHA-256 `1ee9e88688354ea0c180cae0a1a8fb5de5c70bc0225893176336d6f9cb824e0a`. All manifest entries were checked unchanged after both build variants. Documentation is outside that source manifest.

After the validation worker explicitly released source/build ownership, `make gles` and then `make debug` both exited 0. Desktop debug was restored as the final workspace executable. Relevant final debug objects `test350z.dbg.o` and `contact.dbg.o` were rebuilt and linked with the preceding body/coupling objects. Logs: `/tmp/combined350z-verifier/final-gles.log` and `final-debug.log`. Existing shared-header warnings remain; successful builds do not claim warning-free compilation. GLES runtime was not tested.

Both final `nfsu2` and private copied `/tmp/combined350z-verifier/nfsu2-final-debug` SHA-256: `f2f85e9ea18269b7101c37acbc0e48b89257b238945a20abc4b5d14a3e7974d0`. Native runs below use that unchanged private desktop copy. `make test350z-cli-test` also exited 0 with `test350z_cli_test: PASS`, without changing executable identity.

Approved spec hashes confirmed:

- body-coupling: `b09041f16661995375af617766d672db594c27a5bcbe2e8e5783c43b156d6f33`
- body-response: `3a415ca2c95941f3f5c02749e444ec566e48c2e818adacefc0d953eed23a0b8e`
- test350z: `9008ae923699cc529220e89a979683af0b5940926d76763cab0565969c6cb28f`
- test350z-effective: `2db456b62e6711a6d4c487688f94430e4c614c8dceebd6bce62ea0b204f8513c`
- test350z-validation: `3793a90991565fafed9ef0e1f92d126617415d6024b2aaaf77a983ef61ffe41c`

## Final component and host checks

`make body-coupling-test body-coupling-host-test body-response-test body-feedback-test test350z-test contact-test drive-stage5-test` exited 0 after final release. Log `/tmp/combined350z-verifier/final-api.log` records all 31 approved validation vectors as well as the parent profile/effective vectors. Verbatim summary lines:

```text
BC-1..3: seven approved body-coupling vectors passed
BC host replay: 112 preparations matched independent five-pass callback
body-response: 63 vectors, 0 failures
BR-7 dynamic feedback acceptance passed; trace: build/body_feedback.csv
test350z_test: PASS (0 failures)
Stage5: 77 vectors + 29 surfaces, 503 checks, 0 failures
Stage5 direction handoff: 123220 substep observations passed
Stage5 adapter: contact lifecycle, force accounting, handbrake/recovery, host pose, surfaces, reset/collision/clock passed
```

Independent coupling probes passed 10000 synthetic cases / 40000 acceleration comparisons and 10000 external-moment invariance checks. A fresh composed property probe recompiled against final sources passed 540 matching ticks across selectors 0/1/2, checking selected inverse local yaw inertia, ignored legacy fixture scalar and dynamic reset/reprepare. Details and full Stage1–5 regression results are in `../body-coupling/VERIFICATION.md`. Fresh independent validation-amendment verification is separately owned; the 31-case project run here is identified as regression rather than duplicated independent validation work.

## Native commands

Each serialized run uses the independent OpenUG2 normal asset loader with data root `/home/dq/Games/Need for Speed Underground 2`, `DISPLAY=:0 SDL_AUDIODRIVER=dummy`, `--car 350Z --track STREAML4RA --resolution 640x360 --frames 900 --drive-audit PREFIX --drivetrain-trace PREFIX.csv --drivetrain-profile-trace PREFIX.jsonl`. Stock adds `--drivetrain-profile stock`. Provisional adds `--drivetrain-profile temporary-opening-350z-provisional --drivetrain-source '/home/dq/Games/Need for Speed Underground 2/GLOBAL/GLOBALB.BUN'`. A180-second external timeout bounds each process. Prefixes are private `/tmp/combined350z-verifier/stock` and `provisional`. No original assets or raw records are redistributed.

## Native results

Both processes exited 0 within the timeout. Each emitted 900 active CSV rows; every numeric field in all 1,800 rows was checked finite. All 900 tick/pedal/steer/handbrake input tuples match, including the steering phase. Loader spawn is identical: `(-17.117,46.533,7.513)`, heading `1.9072`. First exported positions may differ after dynamics, so spawn equality is checked at the loader boundary. Both startup logs explicitly identify full dynamic body momentum/orientation and supplied c=1, p=1, race=0. The exact source checkpoint contains the verified reduced coupling path; component replay/properties above verify that composition independently of render smoke.

| Observable | Stock | Provisional |
| --- | ---: | ---: |
| Finite active CSV rows | 900 | 900 |
| Forward speed range, m/s | -5.19961…18.30565 | -5.57614…19.67751 |
| Host height range, m | 2.44345…7.53072 | 1.82173…7.53709 |
| Peak audit speed, km/h | 65.49 | 70.12 |
| Prepared roll moment range, kN·m | -35.50077…49.87607 | -97.30645…12.94226 |
| Prepared pitch moment range, kN·m | -9.49705…72.44086 | -14.90083…17.76998 |
| Wall-response count | 116 | 117 |

Both audit summaries report `nan=0 oob_z=0 oob_xy=0`. PNGs `stock_end.png` and `provisional_end.png` were visually inspected: both render the car on the roadside near a fence, with road/world geometry visible. These runs exercise wall responses and are bounded integration smoke checks, not uninterrupted route completion or prescribed trajectory tests. Negative forward speed appears after interactions and remains finite; no retail reversal comparison is asserted.

Provisional metadata records source SHA-256 `07e1c37a2e5ba4b3e6a17b257e653409c31d5f0b3f9df3464aec36adbbced9d0`, 34 selected flags, ten requested grades of 2, `reconstruct-ten-category-grades`, explicit no-active-manual-tuning/zero-extras/ordinary-road assumptions, actual host selectors, approved parent spec hashes and parity `unproven`. Final consumed mass is 1.36517513 tonnes, six forward gears, rpm [800,7500,8000], score 477.922485 and class 1. Independent recomputation of all three inertia values from the emitted effective mass/dimensions agrees within effective-spec tolerance. This verifies the emitted summary without re-extracting or distributing proprietary target tables. Stock metadata reports stock identity, zero selected provisional components and no provisional assumptions. Live engine torque and speed differ between runs, so the provisional profile is observable in dynamics as well as metadata.

Reproducible private oracle/output artifacts: `/tmp/combined350z-verifier/summarize.py`, `native-summary.json`, both CSVs, JSONLs, logs and PNGs. Raw local asset records remain outside reports.

## Reversibility, findings and remaining limits

Project API tests verify restoring stock through the normal adapters, clearing prepared/dynamic state on reset, failed application preserving state and input immutability. The final composed probe separately verifies reset/reprepare. Host source inspection connects F7 to the stock/provisional switch with fresh/reset state, metadata, and ordinary-family/car availability guards. Interactive F7 was not executed because enabled native UI control is unavailable; no input-injection or debugger workaround was used. Actual keyboard-switch behavior therefore remains unverified in native runtime.

No additional divergence was found in these final combined checks; no new amendment was written. The earlier invalid-centre divergence is addressed by the separately approved consumer-domain validation amendment, whose 31 vectors pass in the final project runner. Its independent verification is recorded separately by the fresh validation verifier in `../test350z-validation/VERIFICATION.md`.

Original runtime parity, exact opening instance/settings/load precedence, x87 equivalence, original selector producers, long-duration or all-world stability, every switch interaction, excluded events/tuning/extras/nitrous and original handling trajectories remain unverified. Finite native smoke and agreement with static-inferred arithmetic do not establish retail handling parity. Legal sufficiency remains a question for counsel.
