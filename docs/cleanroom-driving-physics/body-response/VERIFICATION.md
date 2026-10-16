# Body-response independent verification

Date: 2026-10-15. Role: fresh clean-room verifier. No implementation files were edited. Original executable was not executed or inspected; verification uses the approved static-inferred synthetic reference vectors, independent mathematical properties, and the independent OpenUG2 host. No original runtime differential parity is demonstrated.

## Gate and checkpoint

At initial verification entry all approved hashes in REVIEW.md matched, including SPEC.md `3a415ca2c95941f3f5c02749e444ec566e48c2e818adacefc0d953eed23a0b8e` and the 63-vector JSON `8c973876c4540c631c6c8421418b6ccf0b22d14124ec30cefa7aff02c278d1a1`. Existing separately approved Stage1–5 contracts were used as companions. The body-module checkpoint hashes are:

```
03fc6f4fa3806a4d26705d694b157d8a383a37c4497ae171153267c691318490  src/body_response.c
dd9ce5593451e35f6999f589a9c1461d695c0b71824a736f826fffbfd38ee85d  src/body_response.h
abbba481f258f86389bfbeffb2d3c60f83ec48f731163933b414e3fdc2c71151  src/drive_contact.c
c7638a78c3b2eaea09391f5a7ec165807fc0c731ed9dda89f4df0769805782d6  src/drive_stage1.c
84eebd748a7e7de412bbb582b18847339e9a25e17a97df0836eb6fe69ddae05d  src/contact.c
ede833d0159f12acf5713476c452f6e1c124b7be7cf1f02de89f53758a3a1dcb  src/resistance.c
22e4b87fd5c797db88786318fe2d325152a377b83184347cb9e3493a129ade95  src/main.c
9f95e0c1f75cbf221264f47490e94518f5e0388864c9b97a3287a161b5f9e541  Makefile
```

Main/Makefile are shared with a subsequent independent configuration task; these hashes identify the stock body host inspected here, not future configuration revisions.

## Builds and existing tests

Command: `make body-response-test body-feedback-test drive-stage5-test contact-test lateral-test drive-stage4-test braking-test resistance-test drivetrain-test drive-stage1-test physics-clock-test`. Exit status 0. Relevant result lines, verbatim:

```
body-response: 63 vectors, 0 failures
BR pitch feedback: wheel Fx -11.8061 My 3.77797 f.z -0.000652974
BR roll feedback: wheel Fy 7.60966 Mx 2.43509 l.z 0.000257229
BR heave feedback: P.z 0.0180717 X.z 0.320116
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

Strict standalone compilation of `src/body_response.c` with `-std=c99 -Wall -Wextra -Werror -pedantic` passed. Private AddressSanitizer+UndefinedBehaviorSanitizer builds of the body-vector runner and full dynamic feedback runner passed with `ASAN_OPTIONS=detect_leaks=0`; no sanitizer diagnostics. Default LeakSanitizer failed because the execution tool runs under ptrace, so leak detection was not verified. Existing broader headers emit indentation warnings in normal companion builds.

## Independent probes and host inspection

Private scratch is `/tmp/body-response-verifier`; `tests.log`, `probe.py`, `edges.py`, `host_probe.c`, sanitizer binaries/logs and a saved `body_feedback.csv` retain reproducible checks without changing production or project test code.

- Seed 7731: 2,000 independent randomized cases each compare candidate nonorthogonal-basis angular publication against B diag(1/I) B-transpose L, magnitude cap without L mutation, forward-anchored proper orientation repair, requested-force moment caps, capped-impulse moment with signed scale after the cap, and damping/raw-torque replacement against separately evaluated mathematics. Passed.
- 216 frame-gate combinations include adjacent binary32 values below/equal/above .3 and .15, state0/1/2, race0/4 and contact counts0/3/4. 288 torque-selector combinations cover roles0/1/7, speed immediately below/equal/above1 and negative speed, strict .1 heading boundaries and signs, zero/nonzero limits and L.z. Passed.
- Independent composed host probe compares an extreme supplied visual ride (pitch1, roll-1, z400, vertical speed50) against NULL ride: dynamic motion, linear body and wheel geometry match exactly. One hundred ticks exercise c=0/1/2 and race transitions; independent host corrections translate X, seed horizontal P/V and retain axes, angular momentum, held Omega and vertical P. Reset clears preparation/initialization and the next tick restores valid preparation. Passed.
- BR-7 feedback runner uses wheel-produced forces and recorded derivative/preparation boundaries. Braking wheel Fx=-11.8061 and My=3.77797 cause subsequent negative f.z; lateral Fy=7.60966 and Mx=2.43509 cause positive lateral-axis z; vertical imbalance changes P.z then X.z. Travel/rate/load and subsequent wheel force change. Release continues evolving instead of assigning a pose. Preparation-to-momentum-to-pose latency assertions pass. Bottoming with explicit suppression disabled exercises the additional zero sweep and velocity publication.
- Static host review confirms --drivetrain configures the dynamic body with documented independent c=1,p=1,race=0 policy; no-flag and drag/drift remain separate legacy paths. Ordinary dynamic motion bypasses ground-motion limiting and prescribed ride integration; render/camera ride exports consume repaired B, and wheel queries consume physical X/B and held velocities. Identity right-handed coordinate mapping and ground-reference/geometric-origin distinction are documented. Spawn/reset, car switch and wall/rail/vehicle correction calls are connected. The correction API is covered above; original collision production/reset semantics remain outside the specification.

## Divergences and limits

No behavioral divergence found in the executed checks; no proposed specification amendment was written. Inputs excluded by BR-0 (nonfinite/degenerate axes, invalid mass/inertia, overflow), unresolved selector/configuration producers, special reset/rest/guide/lift/collision laws and original runtime equivalence are not established by these results. Agreement to synthetic equations does not demonstrate retail handling or trajectory parity.

## Recorded implementation boundary

After initial hash verification, the implementer appended question BR-I1 to QUESTIONS.md (not SPEC.md): the Stage1/4 provisional planar coupling callback retains its separately supplied inverse yaw inertia; full physical angular publication uses selected BR-1 inertia. This is an explicit unresolved composition boundary, not a measured divergence. The appended auxiliary question is outside the original auxiliary hash and needs the normal analyst/reviewer response if made normative. No new callback or gyroscopic law was inferred here.

The implementer reported serialized `make debug`, `make gles`, then restored `make debug` all passed. Native verification uses a copied desktop executable, SHA-256 `7a6e2f9f8e47148085ab2a4f29e7d5408b612aab37d82716c7f68b0cec8b7573`, preventing races with subsequent configuration builds. These full-build results are implementer-reported; verifier independently executed the component builds above.

## Native stock350Z audit

Verifier independently ran the copied desktop executable against local game data (only the independent OpenUG2 app, never speed2.exe):

```
DISPLAY=:0 SDL_AUDIODRIVER=dummy /tmp/body-response-verifier/nfsu2-debug '/home/dq/Games/Need for Speed Underground 2' --car 350Z --drivetrain --track STREAML4RA --resolution 640x360 --drive-audit /tmp/body-response-verifier/native350z --drivetrain-trace /tmp/body-response-verifier/native350z.csv --frames 420
```

Exit 0; 420 CSV rows, every numeric field finite, active=1 throughout. Application explicitly reports full dynamic body and c=1,p=1,race=0. Contact masks are 14/15 (three/four wheels). Forward speed ranges -0.0009904…17.6120 m/s; travelled 39.7913 m, peak/final63.40 km/h. Height4.4441…7.5307 m; pitch -0.113730…-0.001538 rad; roll -0.057197…0.028152 rad. Vertical velocity -1.25371…0.12709 m/s; pitch/roll rates remain finite. Individual aligned loads vary from0 to respectively7.33643,14.56215,12.66031,7.26749 kN. Wheel0 travel -0.150000…0.043829 m and rate -0.202496…1.000235 m/s. Prepared moments span all three axes and remain finite. The app summary reports `nan=0 oob_z=0 oob_xy=0`.

Spawn/end PNGs were visually inspected: the car remains rendered on the road, with full dynamic body presentation. No wall, rail or vehicle collisions occurred in this bounded run, so native collision-event reachability was not demonstrated. Reset/collision synchronization evidence here is the independent API probe and host call inspection, not fabricated native events. The native CSV exposes pose views, velocity, F/T and wheel state but does not expose every B/P/L component; exhaustive derivative/preparation state observation comes from the synthetic BR-7 trace, not this native audit. A 7-second bounded drive is evidence of this scenario's stability, not all-world or long-duration stability, nor retail trajectory parity. Native logs/CSV/PNGs remain in private scratch under the command's prefix.
