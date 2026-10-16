# Why current driving can feel different: reviewed bounded findings

2026-10-15. **Approved for communication only; not an implementation specification.**
Audited OpenUG2 commit: `ff32818eaade94b92711ffd914af2a430644669f`.

The clearest confirmed gap is that the playable vehicle does not close the full
body/suspension feedback loop. It calculates wheel forces and pitch/roll moments,
but gameplay supplies vertical motion, pitch and roll from a separate ride
trajectory. Several other influential inputs are explicit fixtures. Passing the
Stage1–5 component tests therefore establishes bounded arithmetic/composition
agreement, not original-game driving parity.

This conclusion combines the independently inspected current implementation in
[INTEGRATION_GAPS.md](INTEGRATION_GAPS.md) with the reviewed static-inferred
original behavior in [ORIGINAL_GAPS.md](ORIGINAL_GAPS.md). The original executable
was not run for this audit. No matched original-versus-OpenUG2 trajectory exists.
The importance rankings and explanations of perceived feel below are engineering
inferences, not measured causes of the user's experience. No valid-domain breach
of an already approved requirement was established by this bounded audit.

## The 350Z comparison and reference configuration

The user's comparison is the original game's **opening 350Z versus OpenUG2's
350Z**. [OPENING_350Z_REFERENCE.md](OPENING_350Z_REFERENCE.md) establishes no
opening preset, upgrade tier, tuning, instance bonus, handling family or profile
override. Stock equivalence, a special setup and the resulting effective values
are all unresolved. Differing active/backup asset hashes establish neither
corruption nor a modified installation; provenance and actual load precedence
remain unknown.

**Priority 0 is to establish the reference configuration.** Capture the opening
instance at the first controllable frame and after relevant transitions, with
executable/loaded-asset identity, profile/event state, controls and assists,
selected configuration and final effective physics values. Capture OpenUG2's
350Z with the corresponding fields and known timestamps. A same-build ordinary
stock original 350Z is a separate control, not a replacement for the opening
reference. Compare fields before interpreting acceleration, steering or stability.
The linked supplement defines the capture contract; no original runtime capture
exists yet.

The old manual CSV is a **prior 240SX-launch diagnostic**, launched with
`--car 240SX` for an earlier bug. It lacks per-row vehicle identity, so this
launch provenance does not prove the car remained unchanged throughout. All CSV
metrics below have that scope; none establishes matched 350Z shift timing or a
350Z high-speed curve. The structural integration findings below are independent
of treating that CSV as a 350Z oracle. Opening-configuration uncertainty prevents
attributing the user's particular comparison to a specific missing feature.

## Priorities and likely effects

| Priority | Confirmed current boundary | Original evidence / remaining contract | Possible player-visible effect (inference) |
| --- | --- | --- | --- |
| 1 | Wheel/aero vertical force and pitch/roll moments do not drive the playable heave/pitch/roll. Vertical velocity is overwritten by the supplied ride each substep. | EFA-1 establishes momentum, orientation and angular-velocity coupling. S5-8/U5-6 explicitly deferred the full solver. Inertia-selector identity and exact orientation repair remain unresolved. | Braking does not generate the corresponding tyre-force-driven dive, nor acceleration squat or cornering roll that feeds back into travel and grip. Balance, traction and recovery can differ even with correct local spring/tyre equations. |
| 2 | Gameplay selects unscaled yaw inertia and supplies zero extra angular damping; the loaded yaw multiplier does not affect that selection. | EFA-2 establishes an angular-speed curve and independently gated stabilization damping. EFA-3 establishes a conditional raw-world-torque restriction. Complete mode/selector/race reachability remains open. | Turn-in, steering-release yaw decay, countersteer and spin recovery may differ. Sign and magnitude of the actual error are unmeasured. |
| 3 | Keyboard steering uses fixed 33/9-sample histories with chosen startup seeds at the independent 60 Hz tick. | EFA-4 establishes timestamped expiry, live-count averaging and capacities 66/18. The 120 capacity calibration does not prove a 120 Hz physics or input clock. Reset/time-zero and full call cadence remain open. | Initial steering, short taps, release and reversals can have different command histories. A nominally similar averaging window does not make startup or expiry equivalent. |
| 4 | Every successful live world contact supplies numeric surface ID 1; effective grip/response/event inputs also include fixed fixtures. Bottoming impulses are suppressed, and collision feedback has no angular-impulse channel. | S5-7 supplies independent surface channels, but U5-2/3 leave material identity and full multiplier producers open. EFA-1 resolves bounded impulse-cap facts, not the collision engine. | Surface transitions, landing/bottoming response and off-centre impacts lack their missing variation/coupling. Contact geometry, loads, rear slip bonus and handbrake blend still operate. |
| Conditional follow-up | Current gameplay is keyboard/automatic; downstream shift-grip composition is outside the prior approved contract. | EFA-5 verifies conditional grip arithmetic on the two fully captured wheel operations. Complete wheel coverage, adjustment trigger/decay reachability and effective calibration remain unresolved. | Shift-related traction or corner balance could differ if this state activates. A possible multiplier up to 2 is not proof it activates during an ordinary free-roam 350Z shift (or a 240SX shift). |

The first two priorities belong in one coupled body-response contract. Merely
adding damping or selecting a different inertia without the correct state,
coordinates and timing would not establish compatibility. In particular, the
inertia selector must not be guessed to equal contact count or the separate
torque-limit selector.

## What this means for each reported symptom

- **Steering:** timed-history production and body yaw response remain incomplete
  at gameplay entry. The steering mapping and lateral-force kernels are present;
  their passing tests do not exercise the missing producers.
- **Acceleration:** engine/clutch/gearbox arithmetic is present. Driven-wheel
  traction also depends on body/load feedback and effective grip. This audit
  establishes no new defect in the approved engine-torque equation.
- **Braking:** brake torque, lock/relief and load consumers are connected. The
  computed pedal-force moments do not dynamically generate pitch/load feedback
  through the playable body. No original stopping-distance comparison exists.
- **Shifts:** the prior 240SX-launch diagnostic contains 11 ordinary upshifts: request to gear
  commit is 10 ticks (about .167 s), request to first matching Locked state is
  30 ticks (.5 s). These are current implementation observations, not original
  timing targets or matched 350Z observations. Controls, assist-state producers and conditional grip effects
  need separate validation before timing or traction changes are prescribed.
- **Cornering/sliding at speed:** lateral force, combined grip cap, post-cap
  correction and tyre yaw moment are implemented. The strongest missing pieces
  are coupled body response, angular laws and effective inputs. A new speed-only
  breakaway rule is not supported by this audit.

## What the prior 240SX-launch diagnostic establishes

The supplied prior 240SX-launch diagnostic has 7,942 active rows. Its maximum exported speed is
39.677948 m/s, about 142.84 km/h. Only 714 rows reach 30 m/s (108 km/h); 349 of
those contain nontrivial raw steering. Within that sample, 97.3039% of wheel
states are marked gripping. Under the approved predicate, gripping can include
sideslip and lateral force; the label does not establish an absent slide model.

High-speed samples have nonzero prepared pitch/roll moments (approximately
-20.61…14.22 and -13.68…8.34 kN·m), which the playable body does not consume as
pitch/roll momentum. These are tick-exit, next-derivative accumulators. They do
not by themselves predict the trajectory of a corrected solver.

All contact masks are 15 and every surface ID is 1; individual aligned loads
can still reach zero. The trace does not test original terrain semantics,
airborne transitions or very high speed. It lacks direct steering-command,
yaw-rate, frame-grip and pre-cap-vector fields, so exact cap occupancy and
angular damping cannot be reconstructed. No top-speed/cap defect is proven.
The response-scale API mapping in integration finding 7 remains a separate
nonunit-input contract question; its unity fixture cannot explain variation here.

## Next contracts and matched acceptance

1. Prepare a separately reviewed body-response amendment: full coordinate and
   state domains; momentum/orientation publication and force latency; selected
   inertia and transition semantics with explicit unresolved selector inputs;
   ordinary angular damping gates; raw-torque override; impulse ordering/caps;
   and suspension feedback. Retain explicit unknowns for complete air/mode,
   collision/rest behavior and numerical orientation repair. Add synthetic
   boundary vectors before any implementation handoff.
2. Specify the ordinary timed-history producer, including append order, exact
   expiry, overwrite, startup, duplicate timestamps and clock source. Resolve or
   exclude time-zero/backwards/reset conditions explicitly. Determine actual
   producer call cadence independently of the capacity formula.
3. Trace effective grip, world material selection, device/manual controls and
   shift-adjustment producers. Complete the remaining wheel captures before claiming
   the entire shift-grip operation. Resolve response-scale identity with a
   nonunit supplied-input fixture. Do not replace unknown producers with tuning.
4. After priority 0, establish matched original-opening-350Z/OpenUG2-350Z
   runtime cases using verified effective configuration,
   controls, initial state, road/surface and timing: steering step/release and
   reversal; straight acceleration and braking; measured shifts; sustained turn
   and lift/countersteer; then crest/landing and surface transitions. Include
   the user's intended high-speed range beyond this run where applicable.
   Record command angle, P/L, orientation, yaw rate, wheel loads/slip, grip,
   forces/moments, gear/clutch and collision events at known timestamps. Set
   tolerances from measurement precision and observed repeatability before
   judging parity. Retain component tests alongside trajectory comparisons.

No code or tuning change was made. Stage1–5 approved specifications are unchanged.
A fresh complete behavioral-spec gate is required before implementation; this
findings-only review does not authorize one from these summaries.
