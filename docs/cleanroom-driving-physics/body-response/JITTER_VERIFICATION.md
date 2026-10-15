# Driving presentation jitter verification

2026-10-15. Fresh clean-room verifier; no implementation changes. Original executable was not run. This is independent-host verification, not a retail handling-parity claim.

## Baseline evidence

Preserved independent native executable: `/tmp/combined350z-verifier/nfsu2-final-debug`, SHA-256 `f2f85e9ea18269b7101c37acbc0e48b89257b238945a20abc4b5d14a3e7974d0`. Native CSVs and normal-loader commands are identified in `../test350z/COMBINED_VERIFICATION.md`. Diagnostic artifacts: `/tmp/driving-jitter-verifier/metrics.py`, `baseline-metrics.json`, `smooth.c`, `smooth.csv`, `smooth-build.log`.

The existing deterministic audit explicitly forces presentation alpha=1. Its finite-state and endpoint-image checks cannot verify interactive display sampling at fractional alpha. The no-throttle phase is also not a parked fixture: temporary-profile ticks30–119 move at forward .601408…1.064895 m/s.

Collision-free temporary-profile straight ticks120–169 move at .954050…4.972307 m/s, with all contacts active throughout. Maximum consecutive wheel travel change is .095931083 m; body z step .01006746 m; pitch step .002312012 rad; roll step .00664689348 rad. During steering ticks210–239 the maximum wheel travel change is .0960810483 m, still without collision/contact-mask changes. Example travel discontinuities: wheel2 tick160, -.095931083 m with load6.94046211→0 kN; wheel0 tick175, -.09408801796 m with load4.9359417→0; wheel3 tick217, -.0960810483 m with load6.64661789→0. Native road variation and load-clamping behavior mean these observations alone are not a demonstrated contract violation. Reversal-count frequencies are retained in the JSON as descriptive statistics, not spectral oscillation measurements.

## Controlled smooth-road separation

Private compiled fixture calls the public contact API with the existing invented synthetic car and explicit body trajectory. Four scenarios each contain600 samples at60Hz: flat parked, flat10m/s, 5% slope parked, 5% slope10m/s with body z following ground slope. No dynamic solver, camera, rendering, original assets or original executable participates. Samples after the initial six establish the comparison.

Flat parked/moving travel and rate are exactly constant for all wheels. Slope parked is exactly constant. Slope moving maximum consecutive travel difference is4.768e-7m, maximum rate2.861023e-5m/s; all contacts stay active. Thus a generic moving-on-smooth-road contact-travel defect was not demonstrated. This fixture separates contact sampling from display timing; it does not establish the native road is flat or that every dynamic trajectory is stable.

## Proposed independent-host behavioral amendment (review required)

Provenance: existing independent host render policy and controlled synthetic motion, not an observation of retail runtime. The baseline host obtains camera/player position, steering and spin from a fractional-alpha presentation sample, while dynamic chassis, wheel root transforms, suspension travel and orientation use current simulation state. This creates an observable mixed-time car/camera relationship only when adjacent ticks differ. No original algorithm is inferred.

Proposed statement: During ordinary dynamic driving, one rendered player sample shall describe a single presentation time for chassis origin/orientation, wheel-hub suspension positions, wheel steering/spin, associated lamps and camera target. Rendering extra frames between simulation ticks shall advance the displayed sample continuously without changing simulation state. At a stationary identical pair of snapshots, presentation shall remain stationary for every alpha. Discontinuous reset/car-change snapshots shall not sweep between unrelated poses. This requirement concerns independent presentation only and shall not change the approved physical derivative/preparation boundaries or forces.

Proposed vector: synthetic identity orientation and zero centre, before origin(0,0,.32), after origin(1/6,0,.32), duration1/60s, constant10m/s. For alpha0,.25,.5,.75,1 the displayed origin is respectively0,1/24,1/12,1/8,1/6m along x, with the same sample time for camera target and wheel/chassis roots. Baseline current-body versus interpolated-position discrepancies are respectively1/6,1/8,1/12,1/24,0m. These are mathematical mixed-sampler predictions supported by source call-site observation, not a captured native pixel measurement. Additional vectors should cover changing orientation/travel, unequal individual wheel rotation, zero/multiple ticks per render and reset; expected values require the chosen independent presentation contract to be reviewed first.

## Completion status

Baseline diagnosis and contact-isolation checks complete. Candidate post-change build, full approved-vector regression, fractional-alpha presentation checks and native visual confirmation are pending. No user-visible fix or retail parity is claimed by this report.

The standalone reviewer-bound proposal and nine synthetic vectors are now in `../render-sampling/PROPOSAL.md` and `../render-sampling/test-vectors/render-sampling.json`. They supersede the informal sampling proposal above when approved. JSON parsing passed. Surface ID1 has specified bump amplitude0, so procedural bump noise does not explain the cited native discontinuities. Current frame CSV omits the per-wheel selected world height/normal and substep query points, so changing triangle/layer support versus changing body geometry cannot be separated from those recordings; the final-substep travel rate is not the finite difference between successive frame exports.

## Early candidate against newly approved sampling contract

Approved gate hashes independently matched: render-sampling/SPEC.md `ae4830e01a38054b37fc4aede4545bab80eb602450ffdfa63eb4f7d8bb259825`; vector JSON `976dc0dc16c2b3763acbcae261e58225251c5569ac7bf9d92d84db858dcc841c` (15 cases). Before final worker release, private `sampling_probe.c` compiles against the candidate public presentation adapter and observes identity→pure yaw pi/2, alpha .25/.75. Published yaw .321750551/1.24904573 rad differs from RS-3 required .392699082/1.178097245 by .07094853 rad. This is an existing RS-3 conformance failure, not a new recovered-law amendment. Alpha .5, position interpolation and independent per-wheel phase values pass in this small probe. Evidence `/tmp/driving-jitter-verifier/sampling_probe.log`; this provisional result must be rerun after final candidate release. No implementation remedy is proposed here.

## Native support discontinuity classification

A read-only independent normal-loader scene OBJ export was run using the preserved baseline app (`--track STREAML4RA --objdump /tmp/driving-jitter-verifier/scene.obj`, exit0). Only the independent OpenUG2 executable ran. Private `geometry.py` retains the extraction method and `selected-geometry.json` the two relevant meshes; the full389MB OBJ remains private. Worker-produced read-only query trace `/tmp/driving-jitter-fix/query.csv` identifies the cited travel events with support triangle switches4907/26→4909/11. Selected height drops .098679… .099130m while the queried body's z changes only .001492… .002388m. The small difference from .096m includes the ordinary local plane slope.

The independent loaded geometry places these triangles on opposite sides of one nearly shared XY edge (lateral discrepancy .000259m at OBJ .001m coordinate precision). Third vertices lie approximately4m to opposite sides. The two corresponding edge endpoints are both .096m lower on the second triangle; their normals are nearly parallel. This demonstrates a bounded adjacent-surface height step in the loaded geometry rather than a broad overlapping layer selection. The same road asset label is attached to both. The native side image shows a nearby roadside curb but does not establish that this precise edge is the curb; curb versus authored road-section seam remains unclassified. No original intended collision geometry or erroneous loader/world-query behavior is established by this observation, and no physical-smoothing amendment is proposed.

## Final candidate independent verification

The early RS-3 failure is resolved in the final candidate. Production desktop binary SHA-256 independently confirmed `ac7bc5d0943e4790c7be3526f14be9ef89131faaeb4b110a131f53c15f4371e7`. Final main/header checkpoints: `6ac5761aa12a647c1797e7fc3174ff094dc48a6ed6fec10da3071e98cbad2f85` and `35b1ecc40f240049a2361c41c06c4eec6a68a9fa1b949cc064dc2c57f18e6f27`. The implementer reports successful serialized GLES/debug builds; verifier links its own private native observer against the released final debug dependency objects, and executes the actual final desktop binary. GLES app SHA `65afe8eba25b30dae40db0aa561beb8c6208e4818203c343ffbce0041c77599e`; GLES runtime was not exercised.

Independent project commands, both exit0:

```
make render-sampling-test drive-presentation-test
make body-response-test body-feedback-test body-coupling-test body-coupling-host-test test350z-test drive-stage5-test drive-trace-test physics-clock-test
```

Verbatim key results from `/tmp/driving-jitter-verifier/sampling-project.log` and `final-regressions.log`:

```
All 15 approved render-sampling vectors passed
drive presentation: common sample time, independent hub/spin, exact endpoints and immutable physics passed
body-response: 63 vectors, 0 failures
BR-7 dynamic feedback acceptance passed; trace: build/body_feedback.csv
BC-1..3: seven approved body-coupling vectors passed
BC host replay: 112 preparations matched independent five-pass callback
test350z_test: PASS (0 failures)
Stage5 direction handoff: 123220 substep observations passed
Stage5 adapter: contact lifecycle, force accounting, handbrake/recovery, host pose, surfaces, reset/collision/clock passed
drivetrain trace: schema, force/host distinctions and immutable state passed
physics_clock_test: PASS (synthetic OpenUG2 contracts; no retail equivalence claim)
```

Private independently written `final_probe.c` tests1,000 synthetic yaw pairs at17alphas, independent scalar/hub mathematics, proper basis, unequal/fast/reverse phases, reset and read-only inputs. All17,000 samples pass; maximum yaw error5.11976175e-7rad. A600tick composed dynamic replay with1,795unequal fractional renders matches complete persistent drive state bitwise against the unsampled replay. This confirms sampling does not alter simulated dynamics for these fixtures, including zero-render tick cases. The approved15-vector suite additionally covers tilted/half-turn bases, supplied side mapping, pause/snaps, follow target and clock/discontinuity selection.

Host call-site review connects sampled dynamic origin/basis/hubs/steer/per-wheel accumulated rotation to actual chassis, wheel, lamp and gameplay camera target consumers. Mount lateral sign selects outward mesh side rather than wheel-index parity. Explicit initialization/reset/car-change/mode-change boundaries snap presentation history, pause/snap selects alpha1, and follow-other-actor bypasses player dynamic pose. Interactive keyboard reset/mode changes were not manually operated; these boundaries have synthetic and source-integration evidence.

## Final native fractional observation and visual comparison

Verifier privately copies the final `main.c`, overrides only the supplied display alpha after completed physics ticks to cycle0,.25,.5,.75,1, and adds read-only measurements of actual rendered chassis/wheelWorld/lamp matrices and gameplay-camera actor frame. It does not edit production files or change physics. `prepare_observer.py`, `main-observer.c`, `observer-build.log`, `observer-link.log`, `observer-app` and native artifacts remain under `/tmp/driving-jitter-verifier`. Debug dependency objects were explicitly released by the worker before linking.

Both bounded normal-loader runs use local authorized data, 350Z, STREAML4RA, temporary-opening-350z-provisional, authorized GLOBALB.BUN, 960x540 and230frames. The observer run uses normal gameplay camera and prefix `native-fractional`; the unmodified final debug binary uses fixed side view `--shot-yaw -20 --shot-pitch 10` and prefix `native-final-side`. Both use `--drive-audit PREFIX --drivetrain-trace PREFIX.csv`, exit0 and emit230finite rows. No original executable ran.

In all230native observer frames the actual chassis root exactly matches sampled origin and chassis/lamp/camera basis agreement is exact. Actual world hub maximum error is1.95205212e-6m, lamp attachment root1.89617276e-6m, and gameplay camera actor target3.74764204e-6m versus independently evaluated common sample. The latter is under one binary32 ULP at the native ~50m world coordinate; the stricter bounded synthetic vector tolerance is separately satisfied. All230pre/post render comparisons find zero persistent-drive mutation. The prior current-only chassis policy would lead the sampled actor origin by up to.0894737244m on these same frames; the corrected observed root mismatch is zero.

Most directly, the full230row native observer physical CSV is byte-for-byte identical to the preserved pre-fix default-view `before.csv` (245190bytes). The unmodified final side-view CSV is also byte-for-byte identical to pre-fix `before-side.csv`. Thus all exported physics channels, controls and contact/travel events are preserved in these before/after scenarios, including the physical road step. Summary: `final-summary.json`.

Final unmodified `native-final-side_end.png` was visually inspected against pre-fix `before-side_end.png`: rear rim outward faces now render visibly, rather than the dark reverse faces present before, with the same vehicle/road framing. Native motion was exercised and its actual transform sampling measured; static PNGs alone do not demonstrate perceptual smoothness or eliminate every possible user-reported jitter source. No native video or interactive variable-wall-clock session was captured. The observer varies supplied alpha each rendered physics tick, while the independent600tick probe verifies unequal zero/multiple-render scheduling; these are complementary, not a claim of a recorded natural-cadence video.

No final approved-contract divergence was found in these executed checks. No additional physics amendment is warranted by the road step observation. The measured mixed-time body/wheel/camera defect and rear-facing wheel appearance are addressed by this final candidate; arbitrary roads, all render devices/cadences and all interactive reset/mode paths remain unverified. Retail motion/rendering parity remains unproven.
