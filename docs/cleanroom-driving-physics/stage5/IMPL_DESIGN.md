# Stage5 independent implementation

Approved input: SPEC.md SHA-256
`131e2c9e26091c647fb96bdb9ac0891e43c309487abe1e984b4408b711eec16e`,
reviewed components.json (77 cases), surfaces.json (29 six-channel rows) and
README. Stage1–4 companion hashes were checked against their approvals before
implementation; no specification/vector was edited. Implementer inputs were
approved shared specs/reviews/questions/vectors, this skill, and the explicitly
permitted independent src/tools/Makefile and design/documentation baseline.
No original executable, raw asset, analyst evidence, original source,
PROVENANCE/HANDOFF or verification report was inspected. Only the independent
native application's ordinary loaders used the user-authorized installation.

## Design and requirement mapping

- S5-0/1: `contact.h/.c` defines an independent finite-input component API.
  `drive_contact.c:stage5_car_read` adds only approved row fields through the
  baseline GLOBALB framing, validates them, and leaves upgrades/variants outside
  the contract. `ct_inertia` supplies both stock diagonals; `ct_change_centre`
  handles explicit changed-centre geometry. The host chooses the existing
  unscaled box yaw-inertia mode. Drivetrain wheel inertia remains `.01`.
- S5-2: `ct_frame` refreshes held mount arms/preloads; `ct_preload` uses signed
  exported speed, previous aero enable, stock centre-enabled shares and explicit
  ride offsets. The playable host supplies zero lift offsets. It does not add
  Stage2 reported load to preload or spring load.
- S5-3/4: `ct_post` queries in 0/1/2/3 order and samples each pair's currently
  stored travel. `ct_spring` clamps geometric travel, retains rates at small/zero
  h, distinguishes exact lower endpoint from clamped noncontact, and applies
  progressive spring/damping/paired coupling without presentation-reference
  subtraction. World height/normal/ID are external. Bump is sampled before
  override/steep-normal processing. Noncontact produces zero scalar/aligned load.
  `ct_bottom/ct_impulse` expose requested impulse and independent supplied vector
  caps. The playable host reports requests with application suppressed, then runs
  one zero-duration ordered sweep after any upper-limit test, without an Euler
  update. Unknown caps/rest impulses are not guessed.
- S5-5: `ct_apply` supports all four force selectors, retained noncontact point,
  full world wheel heading/lateral/up transformation, contact application point
  and full cross-product moment. Positive-load alignment is independent of a
  component contact flag; composed noncontacts have scalar zero. All contacts
  are aligned before coupling. Provisional five-pass coupling retains body-local
  planar forces and r0.x/r0.y arms; it never accumulates final normal forces.
- S5-6: separate blend, eta, grip and speed conversion helpers expose strict
  component boundaries. `DrvEnv.supplied_frame_grip` bypasses the lower-stage
  shorthand gamma when the complete Stage5 front/rear grip is supplied. Stage1
  player rear-speed factor and response scale still apply once. The composed
  brake distribution selects corrected binary32 signed-forward mph; direct S3
  fixture entry remains unchanged. Signed reverse B is neither absolutized nor
  clamped. Nonzero H uses existing Stage3 relief/lock/spin/direction/smoothing.
- S5-7: the small normative numeric mapping is encoded directly in independent
  `CtSurface` records. Peak/sliding/spin/body/bump remain independent; bump uses
  floor cells and nonnegative modular sample indices. `sample` supplies fresh
  wheel channels; frame refresh uses exported surfaces/contact flags for body
  drag. No unreviewed semantic material labels are mapped.
- S5-8: `stage5_drive_step` is the selected composition entry, separated from the
  existing lower-stage adapter. Creation performs zero-time preparation before
  the first changed pedal frame. Frame preload precedes mode refresh; shared
  exclusivity/brake distribution occurs in `drv_frame_begin` once. Handbrake/
  grip refresh once, then derivative/Euler/post/geometry/aero/coupling/final
  accumulation proceeds. No current frame input removes preceding prepared
  wheel forces before their first derivative. Contact/surface/load/speed snapshots
  export only after all substeps. Reset preserves config/providers and restores
  creation travel/rate/blend, without attempting restored-state reconstruction.

## Explicit host architecture and limits

The original full body/momentum/angular/world/material producer is unknown. This
round does not invent one. The independent OpenUG2 ride supplies heave, pitch,
roll and their rates; these are prescribed/extrapolated trajectories, with
cosmetic body_roll excluded. The host uses XY body-centre coordinates and the
fixed vertical bridge `P.z=carpos.z+centre.z-mean(mount.z-radius)`. During substeps
P.z/pitch/roll extrapolate the input rates. P.xy and yaw integrate the actual
transformed forces/moments. Its body accumulators retain full vertical forces
and pitch/roll moments for inspection/external 3D consumers, but the prescribed
ride owns playable vertical motion. Integrated vertical momentum is discarded
at the explicit constraint; it does not feed another ride force/gravity pass.
Dynamic brake/throttle pitch and full 3D load transfer therefore remain open.
No `M*a*height/wheelbase` shortcut or cosmetic-lean contact producer was added.
Prescribed pose tests are clearly distinguished from dynamic pedal causality.

`stage5_world_query` uses the existing independent reachable-wheel query, unit
triangle normals, upward .25 m host reach and downward max(stock droop, existing
host swept fall reach). Missing support supplies a valid up normal and ID1.
Every selected triangle uses the explicit ordinary numeric ID1 fixture; arbitrary
IDs remain injectable. This is not a recovered material mapping. The host selects
bottoming suppression because retail cap values are unknown. Query/reference,
impulse, vertical bridge and body choices are documented fixture policies, not
retail facts. I5-1 was added to QUESTIONS.md to target future reviewed producers.
Other supplied choices: q=1, X/event/v0/s0=0, reduction=0, scale=1, zero damage,
ordinary state, initial locks=0, relief enabled, zero extra angular damping,
no nitrous/upgrades/launch. Existing steering histories remain host box filters.

`--drivetrain` loads Stage5 for initial car and transactional car switch. Existing
reset/race-mode/pause paths keep the new configuration and clear transients.
Legacy default and drag/drift remain unchanged. Audio/HUD/wheel presentation keep
reading the composed engine/wheel state. Main no longer calls the arcade chassis
hold/velocity scaling on the Stage5 path. Collision-produced XY velocities enter
the following simulation sample without resetting wheel spin or yaw.

## Tests-first and coverage

`tools/contact_test.c` encoded all 77 reviewed cases before contact behavior was
written; the initial test build failed at undefined ct_* symbols as expected
(`/tmp/stage5-red.log`). No implementation architecture came from original code.
All component vectors and each of 29 x 6 surface parameters pass, 503 checks.
Additional `tools/drive_stage5_test.c` covers stock inertia/centre shift,
centre-disabled preload share, override-before-steep handling, independent impulse
caps/suppression, four force selectors, synthetic stock field mapping/rejection,
moving handbrake braking/release/blend, service brakes, mirrored steering,
prescribed opposite pitch/roll consequences, ordered liftoff/recontact,
first-derivative retained force with new HB/missing terrain, preload aero latency,
wheel/body surface snapshot timing, bottom extra sweep, reverse, reset/switch,
collision velocity, pause and exact 30/60/90/144/240 Hz state equality.
Accounting compares each full force/moment to once-only gravity, freshly computed
aero and the four final wheel contributions. Frame and blend/post counts are
checked against actual composed operations, not only component arithmetic.

Prior approved fixtures are retained byte-for-byte. Stage1–4 independent tests
continue to exercise their supplied boundaries; the S5 opt-in flag changes only
the composed signed brake speed and complete grip entry. No prior vector was
silently rewritten to accommodate supersession.

## Validation commands and results

Validation is recorded below after the source freeze. Native bounded drives use
normal controls/world queries and ordinary independent loaders, never original
execution or direct asset inspection. No runtime/differential parity is claimed.

Executed commands (workspace root; compiler `cc`, system C99 toolchain):

```sh
make contact-test drive-stage5-test lateral-test drive-stage4-test braking-test resistance-test drivetrain-test drive-stage1-test physics-clock-test district-collision-test > /tmp/stage5-all-tests.log 2>&1
cc -O2 -std=c99 -Wall -Wextra -Werror -Wno-unused-function -Wno-misleading-indentation -D_DEFAULT_SOURCE -Isrc tools/contact_test.c src/contact.c src/resistance.c src/braking.c -lm -o /tmp/stage5-strict-component
/tmp/stage5-strict-component
cc -O2 -std=c99 -Wall -Wextra -Werror -Wno-unused-function -Wno-misleading-indentation -D_DEFAULT_SOURCE -Isrc tools/drive_stage5_test.c src/drive_contact.c src/contact.c src/drive_stage1.c src/drivetrain.c src/braking.c src/lateral.c src/resistance.c src/physics.c -lm -o /tmp/stage5-strict-adapter
/tmp/stage5-strict-adapter
cc -O1 -g -std=c99 -Wall -Wextra -Wno-unused-function -Wno-misleading-indentation -D_DEFAULT_SOURCE -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc tools/contact_test.c src/contact.c src/resistance.c src/braking.c -lm -o /tmp/stage5-asan-component
ASAN_OPTIONS=detect_leaks=1 /tmp/stage5-asan-component > /tmp/stage5-asan-component.log 2>&1
cc -O1 -g -std=c99 -Wall -Wextra -Wno-unused-function -Wno-misleading-indentation -D_DEFAULT_SOURCE -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc tools/drive_stage5_test.c src/drive_contact.c src/contact.c src/drive_stage1.c src/drivetrain.c src/braking.c src/lateral.c src/resistance.c src/physics.c -lm -o /tmp/stage5-asan-adapter
ASAN_OPTIONS=detect_leaks=1 /tmp/stage5-asan-adapter > /tmp/stage5-asan-adapter.log 2>&1
ASAN_OPTIONS=detect_leaks=0 /tmp/stage5-asan-component > /tmp/stage5-asan-component-noleaks.log 2>&1
ASAN_OPTIONS=detect_leaks=0 /tmp/stage5-asan-adapter > /tmp/stage5-asan-adapter-noleaks.log 2>&1
make normal > /tmp/stage5-normal.log 2>&1
make gles > /tmp/stage5-gles.log 2>&1
make debug > /tmp/stage5-debug.log 2>&1
```

Results: component Stage5 503 checks, Stage4 208, Stage3 479, Stage2 348,
Stage1 3,606, all zero failures. Stage1/4/5 adapters, physics clock and district
collision suites pass. New adapter test after the source freeze also passes its
opposite prescribed pitch/roll and real lifecycle-count assertions. Strict new
component/adapter executables pass. Existing baseline misleading-indentation and
unused static-header warnings are disabled in the strict command; no new warning
category was hidden. ASan+UBSan executes both suites successfully with leak
detection disabled, with no memory/undefined-behavior diagnostic. Both initial
`detect_leaks=1` runs failed at LeakSanitizer's environment/ptrace limitation;
therefore no leak-check success is claimed. Normal and GLES builds pass with
existing baseline warnings. Debug/native completion is recorded below.

### Final lifecycle correction and repeat

A self-audit before final delivery found that the upper-limit branch repeated
only contact geometry. S5-8 requires the established full post callback sweep.
The implementation now repeats body publication/force clearing/gravity and
`drv_post` (wheel, engine and clutch post callbacks) before the h=0 contact
sweep. It still performs no derivative or Euler integration. A new regression
sets an explicit supplied OPEN clutch state with a pending target and observes
that its target-gear transition occurs during the zero sweep. The test failed
before this correction at `zero_sweep_gear==2`
(`/tmp/stage5-zero-sweep-red.log`), then passed. No requirement/vector changed.

The full eleven-target command above was repeated successfully against this final
source, logged as `/tmp/stage5-all-tests-final.log`. Source/tests were re-frozen
at that point. Strict adapter and ASan+UBSan adapter were recompiled with the
same commands/flags above; final logs are `/tmp/stage5-strict-adapter.log` and
`/tmp/stage5-asan-adapter-final.log`. GLES/debug were rebuilt from final source,
with final logs `/tmp/stage5-gles-final.log` and `/tmp/stage5-debug-final.log`.
The earlier normal build was before this narrow post-sweep correction; final
GLES/debug compile the corrected complete application sources.

The pre-correction native logs `/tmp/stage5-native.log` (420 ticks, normal build)
and `/tmp/stage5-final.log` (900 ticks, debug build) are preliminary evidence,
not final-source proof. The latter naturally recorded 106 wall-response frames,
61.67 km/h peak, 80.43 m travelled, and zero NaN/Z/XY-out-of-bounds flags. Both
used real controls and world queries; no body state was forced. A final-source
native run and capture inspection are recorded below.

### Final-source native acceptance

```sh
DISPLAY=:0 SDL_AUDIODRIVER=dummy ./nfsu2 '/home/dq/Games/Need for Speed Underground 2' --car 240SX --drivetrain --track STREAML4RA --resolution 640x360 --drive-audit /tmp/stage5-accepted --frames 900 > /tmp/stage5-accepted.log 2>&1
```

This completed with exit 0 after the final debug rebuild. The debug executable
is left on disk. Native summary: 61.67 km/h peak, 20.16 km/h final, 80.43 m
travelled, 106 naturally occurring wall-response frames, no rail response;
NaN/Z/XY-out-of-bounds flags all zero. All 78 Stage5 telemetry rows are finite.
At tick169 signed speed is 3.21296 m/s. The moving HB pulse at170 requests rear
H=5 (front 0), blend .666667; by184 speed is 1.49768 m/s and blend .5. Release
at185 gives H=0 and blend .516667; at195 blend .683333 and speed 1.89130 m/s,
showing recovery. The preceding prepared force persists at release as specified.
Loads respond to actual road/ride geometry: at169 aligned N is
(4.66100,3.33783,3.64865,4.57950) kN; at184 it is
(4.04868,4.06427,3.58194,3.55717). Steering/service-brake phases likewise retain
finite variable travel and loads. These are prescribed-host geometry outcomes,
not evidence of recovered dynamically integrated brake-pitch.

Spawn and end captures `/tmp/stage5-accepted_spawn.png` and
`/tmp/stage5-accepted_end.png` were viewed: the car, wheels and world render
coherently; the ending car is beside the wall naturally encountered by the
script. No body velocity/load/contact was forced to manufacture acceptance.
Logs/captures show independent OpenUG2 output only. No original executable was
launched, nor were original raw data files inspected directly.

Final source fingerprints: `drive_contact.c`
`4a4a3639846affff7521f5f31b62352338a68b705f362ddff80a179630788dd9`;
`drive_stage5_test.c`
`40a2c171b2f3e915e988a431f9c707a2ff714711adfd5ea52f4935799da4e51d`.
Approved Stage1–5 SPEC hashes and all Stage5 vector/README hashes remain unchanged.
The only appended shared question is I5-1; PROVENANCE/HANDOFF/report artifacts
belong to the coordinator/verifier and were not edited or read by this role.
No stage/commit/push was performed.
