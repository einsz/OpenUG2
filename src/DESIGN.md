# Independent body-response implementation decisions

Implementation inputs: approved `docs/cleanroom-driving-physics/body-response/SPEC.md`
SHA-256 `3a415ca2c95941f3f5c02749e444ec566e48c2e818adacefc0d953eed23a0b8e`,
its approved synthetic vectors and separately approved Stage1–5 contracts; existing
independent source. All hashes, including body auxiliary files, were verified
against REVIEW before work. No original executable, data, analysis/evidence,
private generator or original source was accessed.

- BR-0/1: `BodyMotion` owns X,B,L,held Omega and selection; existing `ResBody`
  continues to own P,V,z,F,T. `BodyCar` is a supplied effective inertia/yaw-scale/
  torque-limit record. Stock numeric loader reads the approved row fields only.
  A restored complete `Stage1Drive` can retain prepared accumulators and held
  publication fields directly. Explicit velocity initialization uses basic inertia.
- BR-1/2/3: small selection, gate, damping and torque-limit functions expose the
  component boundaries for testing. Selection modifies world L.z only on adjusted
  to basic transition. Frame gates/limit are held across subdivisions; limit
  replacement tests raw T.z after computing the damped derivative. Host c=1,
  p=1,race=0 are labelled independent supplied policy, never contact-derived facts.
- BR-4: collect all body derivatives before integration. Position and all three
  basis columns use held velocities. Publish Omega using candidate Euler B with
  its transpose; cap magnitude without rewriting L. Contacts, optional zero
  sweep, aero/coupling and once-only wheel preparation see candidate B. Repair
  forward first only after preparation, without republishing Omega. Aforce uses
  consumed stored F rather than net finite-difference acceleration.
- BR-5: late opposing torque is separate from momentum damping and is added after
  wheel point forces for the following derivative.
- BR-6: the common independent `res_point_force` caps each linear application
  and requested-force arm moment separately; aero and wheel applications share
  that boundary. Body impulses cap the linear vector, form/cap its arm moment,
  then apply the optional angular scale. Bottoming uses scale 1 and explicit
  global suppression. Accumulated force, moment and momentum are not capped.
- BR-4 / Stage5 S5-5: contact alignment retains both cross products, including
  candidate non-unit lateral axes. No unit-axis simplification at the interior
  boundary and no extra normalization.
- BR-7: `stage5_drive_body` opts the Stage5 adapter into dynamic motion. The
  application `--drivetrain` selects it; prescribed-pose tests retain their
  distinct adapter. Current pose drives queries, arms, point velocities,
  travel/rate and load. Tyre/grip/brake laws and five-pass planar coupling are
  reused without another load-transfer law or prescribed pose writer.
- BR-0/7: host and physics vector/pseudovector axes are identical, right-handed
  x-forward/y-left/z-up. Geometric origin is X-B*centre; host ground proxy is
  that origin plus B.up*mean(mount.z-radius). Level zero-travel spawn chooses X
  from the supplied proxy, zero vertical/angular velocity, supplied horizontal
  velocity and adjusted inertia selection. This is independent spawn policy.
  The renderer consumes full B and origin; existing body drop is cosmetic.
  Wheel display transforms use effective mounts plus computed travel.
- BR-7/8: existing independent collision services remain external. Their explicit
  proxy-position/horizontal-velocity correction translates X and seeds horizontal
  P/V, without wheel/angular reset. Ordinary ground-motion/ride integration is
  bypassed. Camera/HUD ride values are output-only. This does not implement full
  original collision/reset/rest/stuck/guide/lift/special-state producers.

## Validation

`tools/body_response_test.c` encoded every approved vector before implementation;
its initial link failed on the unimplemented body operations. The completed test
runs all 63 vectors, including strict gates and candidate-basis publication.
`tools/body_feedback_test.c` uses invented car coefficients, flat unit normals,
fixture surface 1, unit grip, c=1,p=1,race=0 and fixed 60 Hz frame entry. Wheel
braking and steering produce signed pitch and roll; a raised contact produces
heave. Tests check delayed momentum/pose changes and subsequent suspension
travel/rate/load/force feedback. Release does not reset pose. Bottoming and
external correction are checked separately. The CSV records derivative and
preparation boundary h,X,B,P,L,V,Omega,z,s,F,T, contact snapshots and wheel state.

Component and synthetic-host agreement does not establish original runtime,
differential, measured handling or opening-preset trajectory parity. Complete
selector/configuration/material/event producers and other BR-8 exclusions remain
external. No ambiguity was filled by undocumented tuning. The reviewed BC amendment
below resolves the provisional-coupling inertia boundary previously recorded as BR-I1.

## Reviewed provisional body coupling (BC-1..3)

Implementation input: approved `docs/cleanroom-driving-physics/body-coupling/SPEC.md`
SHA-256 `b09041f16661995375af617766d672db594c27a5bcbe2e8e5783c43b156d6f33`.
The spec and vector README/JSON hashes matched REVIEW before implementation;
the separately approved body-response and Stage1–5 hashes also matched. Only
approved public contracts and the explicitly permitted independent baseline were
used; no original binary/data/source or analyst material was accessed.

- BC-1: `body_inverse_yaw` uses the same retained selection and effective yaw
  diagonal as body publication. The dynamic Stage5 preparation supplies this
  local scalar; prescribed and lower-stage planar fixtures retain their explicit
  supplied scalar boundary. BR-I1 is resolved by this approved amendment.
- BC-2: the existing `DrvBodyPlanar` context holds the once-per-preparation
  transpose(candidate B) linear-force snapshot, local steered directions and
  frame-held mount x/y arm. `drv_couple` retains its five shared pass-start force
  snapshots, simultaneous eligible-wheel corrections and subsequent clutch
  corrections. Each callback recomputes planar sums without accumulating forces
  or applying body caps. Final wheel/normal forces and late angular torque remain
  once-only later preparation operations.
- BC-2: `drv_body_accel_planar` uses the signed lateral-axis x factor, and its
  moment sum starts at zero. The legacy external-moment fixture field remains
  accepted but ignored. Angular momentum, held spin, prepared torque and normal
  wheel load have no independent term at this boundary. Full 3D body dynamics and
  contact-velocity projection retain their separately specified formulas.
- BC-3: calculations use double intermediates with binary32 acceleration stores,
  following existing kernels. `drv_body_planar_acceleration` exposes the three
  pass-start scalar results for component tests without changing the callback's
  intermediate precision. Exact extended-precision compatibility is excluded.

`tools/body_coupling_test.c` encoded all seven approved vectors before the new
operations; its initial compile failed on the missing APIs. All seven now pass,
including each expected prepared local-force, selected inverse, scalar
acceleration and wheel-acceleration field. `tools/body_coupling_host_test.c`
replays dynamic and prescribed Stage5 preparations against an independent callback
written directly from BC-1/2. All 112 preparations (560 callback evaluations)
match the full drivetrain state, covering centre offsets, candidate axes,
selection modes and independence from the lower-stage supplied scalar during
dynamic creation and later steps. Superseded Stage1 heading probes now test their
unchanged force/moment components directly; the Stage4 external-torque callback
assertion now requires zero. No approved vector file was changed.

Validation: seven BC vectors; independent host replay; existing drivetrain,
Stage1/4/5 adapter, 63 body-response vector and BR-7 feedback suites all pass.
No questions were added. Results establish synthetic arithmetic/composition
agreement only; runtime/differential and measured handling parity remain absent.

## Independent driving display sampling (RS-0..5)

Approved implementation inputs: render-sampling/SPEC.md SHA-256
`ae4830e01a38054b37fc4aede4545bab80eb602450ffdfa63eb4f7d8bb259825`
and its 15-vector JSON SHA-256
`976dc0dc16c2b3763acbcae261e58225251c5569ac7bf9d92d84db858dcc841c`.
Both matched REVIEW before use. Proposal and external analysis material were
not used. This is independent host presentation policy, without a retail claim.

- RS-0/1/2: `DrivePresentation` stores completed-tick geometric origin, proper
  axes and each physical wheel's resolved local hub, travel, steering and spin.
  Presentation samples origin and scalar/vector channels linearly. Body, wheel
  roots, lamps and camera actor axes consume the same copied sample. The camera
  host proxy is derived from that sample through the existing fixed clearance
  conversion. Travel is resolved once. Follow mode selects the followed actor's
  sample entirely, including its existing generation-aware history.
- RS-3: copied endpoint axes convert to unit quaternions and use shortest-arc
  spherical interpolation. Largest-diagonal conversion chooses a deterministic
  half-turn branch. Equal bases and alpha endpoints return exact copies. No
  simulation basis or held velocity is modified or republished.
- RS-2: every wheel keeps its own phase. Output-only per-tick angular-travel
  counters observe existing explicit-Euler wheel derivatives before integration;
  they preserve fast/reverse/multiple-turn travel across bounded physical phase
  labels. Separate double presentation phases accumulate this travel; no physical
  phase, wheel torque, lock or angular velocity is changed. Mesh outward-face
  conversion uses actual mount lateral side rather than index parity. This
  matters when an effective rear axle's mount ordering reverses left/right.
- RS-4: zero-tick renders retain history; multiple ticks replace it with the final
  adjacent pair. Initialization, explicit host reset/teleport, car/mode change,
  rim/body geometry replacement snap display history. Existing physical resets
  remain separate. Pause, explicit snap and deterministic audit select current
  snapshots. Pure snapshot reset/sample operations expose these boundaries.
- RS-5: interpolation never queries world support or prepares/integrates physics.
  Synthetic cadence tests compare complete dynamic fixture states bitwise with
  different render groups. Native before/after 230-tick traces were byte-identical.

Validation: `render-sampling-test` reads every approved vector, with additional
quarter-yaw fractions, progressively halved 3D/half-turn continuity checks,
attachments, wheel identity and synthetic dynamic read-only cadence checks.
Vectors were encoded first; the initial compile failed on missing origin,
scalar channels and history/attachment APIs. The earlier independent display
regression test first failed before the sampler existed and separately failed
before the side-mapping helper existed. `drive-presentation-test` preserves those
specific regression checks. No approved vector was changed.

Native before/after moving wheel images and detailed output-only world-query
logs are retained in `/tmp/driving-jitter-fix`. The corrected rear rim faces
outward. Large precollision travel drops occur at selected adjacent world-road
triangles with a roughly 9.6 cm height discontinuity; the contact geometry, normal
loader and physical response are preserved. This does not establish exact curb
semantics, retail trajectories, or that every reported physical jolt is a display
sampling error. No physical smoothing/damping/parameter tuning was introduced.

## Temporary configuration removal

The temporary modified 350Z configuration loader, effective-composition module,
profile CLI/F7 controls and metadata sidecar were removed at the user's request.
The application now always uses its ordinary per-car stock loader for
`--drivetrain`, including `--car 350Z`. This cleanup preserves BR-0..8, BC-1..3
and RS-0..5: body integration, tyre/body coupling and display sampling are
unchanged. The render-sampling test links the ordinary driving modules directly;
it no longer depends on the removed configuration module or its crypto linkage.
The shared UI crypto dependency remains required by the independent glyph loader.
Reviewed temporary-configuration records remain archived under the three
`docs/cleanroom-driving-physics/test350z*` directories with unchanged approved
files. No new behavior or specification ambiguity was introduced.

Removal validation: all 17 targeted contact, Stage1/4/5, BR, BC, drivetrain,
braking, resistance, lateral, trace, presentation, clock and collision targets
passed. This includes all 63 BR, seven BC and 15 RS approved vectors. Desktop debug and GLES
builds passed; the desktop debug executable was restored after the GLES check. A bounded stock 350Z native run through the normal loader
completed 230 ticks with finite diagnostic values and no profile metadata
sidecar; its capture window stayed hidden. This is a loader/regression smoke
check, with no claim of original handling parity.
