# Independent implementation integration audit

Date: 2026-10-15. Audited implementation: `ff32818eaade94b92711ffd914af2a430644669f`.
Role: fresh implementer-context auditor, read-only implementation inspection.
This report uses approved specifications, independent implementation/tests/design,
and the supplied OpenUG2 manual-driving CSV only. It contains no original-runtime
comparison or claim about the original game's handling. No original executable,
assets, source, disassembly, analyst notes/evidence or sibling report was accessed.

**Manual CSV scope correction (supplemental review):** this is the prior
`--car 240SX` launch diagnostic for an earlier bug. The CSV contains no per-row
car identity and does not prove unchanged vehicle selection. Every trace-derived
metric in this report, including shifts, collision differences and high-speed
statistics, is confined to that diagnostic. None is a matched 350Z observation
or an original reference oracle. Structural source-audit findings are separate.

All five `SPEC.md` digests matched their supplied approved hashes and approval
records before code inspection: Stage1 `61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb`,
Stage2 `d3072359aa25bf0ef541504805f70b3579829bb9b1310cda8ba1a568286c6cb6`,
Stage3 `d4689a16861df810159760a6a668ef3714966fd6c20a30e3b2b54f355c41c6f7`,
Stage4 `02144d187323c6d74a6fb21e6e0b3e2842097cd0e28c935eea97907c1cbc911a`,
Stage5 `131e2c9e26091c647fb96bdb9ac0891e43c309487abe1e984b4408b711eec16e`.

## Outcome and classification

The bounded contact/tyre components are connected to gameplay, but the playable
vehicle is still a hybrid: XY momentum and yaw use those forces; vertical motion,
pitch and roll come from a different ride solver. Several other influential
inputs are explicit fixtures. This is the clearest verified reason that component
completion alone cannot establish complete handling fidelity.

- **A:** Approved behavior demonstrably omitted in its established valid runtime
  domain. No such defect was established by this bounded audit.
- **B:** Explicit unknown, excluded producer or declared host fixture. Findings
  1–5 are these boundaries, not evidence that an invented substitute should be added.
- **C:** Unproven conformance issue or missing validation. Findings 6–7 require
  clarification/measurement before a compatibility fix can be asserted.

Ranking reflects plausible influence on steering, sliding and acceleration/braking
feel, inferred from the independent code; it is not an original-game comparison.

## Ranked findings

### 1. B — Computed suspension/aero forces do not close the dynamic load-transfer loop

`src/drive_contact.c:61` accumulates every transformed wheel force and all three
moments. However, `src/drive_contact.c:94` integrates only the world-z moment into
yaw. Lines 97–100 integrate XY position, run momentum integration, then replace
vertical velocity/momentum with `ride->vz` every substep. `pose()` takes pitch,
roll and their rates from the supplied ride (`src/drive_contact.c:24–31`). Its
vertical position is prescribed as well. Therefore Fz, Mx and My are computed
and retained for diagnostics but do not drive the playable heave/pitch/roll.

The actual ride step uses its own geometry-based normalized spring/damper forces,
gravity and pitch/roll proxy inertias (`src/physics.c:131–199`). The main loop
advances it after Stage5 (`src/main.c:7904–7910`). The lateral-acceleration lean
only changes cosmetic `body_roll`, explicitly separated from contact suspension
torque (`src/physics.c:238–245`). There is no Stage5 tyre-force-driven brake dive,
acceleration squat or cornering roll feedback into its geometric loads.

**Applicable contract:** S5-0 replacement rules 1/5; S5-2; S5-5; S5-8's expressly
permitted prescribed planar host; U5-6. The spec does not authorize substituting
an algebraic load-transfer formula. This is a known host boundary, not a proof
that any particular missing 3D solver would match the original.

**Observable consequence (inference):** pedal/tyre forces cannot themselves
produce dynamic pitch/roll redistribution, although road pose, supplied rates,
preload and contact geometry still change load. This can materially change
braking capacity, driven-axle traction, corner balance and transient recovery.

**Probe:** 30 m/s flat prescribed ride, 30 service-brake ticks: prepared
Fz=-4.22905064 kN and My=4.25405264 kN m, while final vz=0, pitch=0 and up=(0,0,1).
The existing `tools/drive_stage5_test.c:39–45` tests the consequence of injected
pitch/roll, not generation of those poses by pedals.

### 2. B — Gameplay chooses one unscaled yaw inertia and zero extra angular damping

The loader reads the stock yaw multiplier (`src/drive_contact.c:11`) and
`ct_inertia()` computes both allowed diagonals (`src/contact.c:41`). Gameplay
instead supplies `1/lat_inertia(mass,dimensions)` and damping 0
(`src/main.c:4652`, car-switch path `src/main.c:6779`). `stage5_drive_contacts()`
does not change that selection (`src/drive_contact.c:23`). The yaw equation uses
exactly this supplied inverse and damping (`src/drive_contact.c:94`).

The provisional contact coupling projects external moment on body up
(`src/drive_contact.c:57`), while final heading integration uses world Mz. This
is a further limit of the tilted planar host, not a demonstrated valid-domain
3D integration equivalence. No extra angular-velocity-opposing torque is added
after tyre coupling in `prepare()` (`src/drive_contact.c:54–62`).

**Applicable contract:** S4-1, S4-6, U4-5; S2-6/7 angular boundary; S5-1 and U5-6.
The mode selector, full angular damping and additional torque producer remain
unknown. The unscaled selection is documented in `docs/DRIVETRAIN.md`.

**Observable consequence (inference):** the loaded yaw multiplier has no influence
on gameplay unless a caller explicitly changes the supplied inertia; yaw decay
has tyre forces and any generated moments but no additional supplied damping.
The sign/magnitude of any fidelity error cannot be inferred from this audit.

**Probe:** changing only synthetic kI from 1 to 10 gives identical 60-tick
heading (0.527600765 rad) and vx (22.1694851 m/s). Gameplay inverse Iz remains
0.519750476; the component selected inverse is 0.0519750491 for kI=10.

### 3. B — Contact impulses and collision angular response are outside the composed body state

Upper-travel bottoming requests are calculated, but application is deliberately
suppressed (`src/drive_contact.c:33–47`). The required extra zero sweep is retained.
`ct_impulse()` supports separate supplied caps (`src/contact.c:49–51`), but
gameplay never calls it to change linear/angular momentum. This is an explicit
fixture, not a missing guessed cap value.

World-wall response changes host position/XY velocity after the Stage5 step
(`src/main.c:7784`, `src/main.c:7828`, `src/main.c:7875`). Car contacts likewise
copy position/velocity/ride back (`src/main.c:8406–8411`). Those interfaces do
not pass `Stage1Drive.yaw_momentum` or an angular impulse. At next tick entry
Stage5 imports host XY velocity, while retaining its yaw and wheel states
(`src/drive_contact.c:66`). Thus an off-centre collision cannot impart a new
contact-point yaw impulse through this adapter. Collision velocity changes can
still change later tyre slip and hence subsequent yaw forces.

**Applicable contract:** S5-4; U5-2/U5-6; S4-6's external collision-torque boundary.
**Observable consequence (inference):** landing/bottoming and off-centre impacts
can produce materially different transitions from a body solver that consumes
those impulses; this audit cannot specify their missing values.

The manual trace contains final host/body XY velocity differences up to
6.11407 m/s, confirming that the host collision path materially changes the
trajectory. This number alone does not attribute a particular collision type.

### 4. B — World surface variation and several grip/response producers are fixed fixtures

Every successful triangle query returns numeric surface ID 1
(`src/main.c:2061–2072`). The six independent channels exist
(`src/contact.c:6–39`), fresh tyre channels are consumed
(`src/drive_contact.c:52`), and previous exported body-drag channels are consumed
(`src/drive_contact.c:70–71`). With ID 1, the channel values are peak=slide=spin=
body-drag=1 and bump amplitude=0. Therefore none of the other reviewed surface
responses occurs through the live world adapter.

Initialization and live frame composition also use q=1, X/event=0, G=1,
reduction=0, no damage/torque bonus, zero initial locks and relief enabled
(`src/drive_stage1.c:119–125`, `src/drivetrain.c:56–61`,
`src/main.c:4650–4659`). Rear slip bonus and handbrake blend *are* calculated
and consumed (`src/drive_contact.c:87–90`, `src/drive_contact.c:51`); they are
not wholly bypassed. What is missing is the full production of effective
multiplier/state inputs and material-ID mapping, not their component arithmetic.

**Applicable contract:** S5-6/7, U5-2/3/4; S2-1/U2-1/2/4/5; S1 U-6/7/11.
**Observable consequence (inference):** traversing different mapped world
materials cannot change these six surface responses, and contextual grip,
drafting/event, upgrades and other excluded states cannot provide their missing
variation. No semantic material-to-ID mapping is inferred here.

### 5. B — Steering histories, input cadence and driving controls are independent policies

Gameplay reads keyboard W/S as a single signed pedal, W taking precedence,
and A/D as {-1,0,1} (`src/main.c:7518–7524`). Stage5 always sends reverse or
automatic-forward selection (`src/drive_contact.c:73–86`); Q/E shifting is routed
only for the excluded drag mode (`src/main.c:6396–6400`). The component's manual
assist exists, but the ordinary gameplay adapter does not supply it. Device
mapping and that setting-to-command producer are explicit unknowns.

Steering uses keyboard I/Lk and fixed 33-input/9-rate box windows with zero/one
seeds and dt=1/60 (`src/drive_stage1.c:104–107`, `src/drive_stage1.c:209–225`).
The analog J/La branch exists (`src/lateral.c:20–30`) but is not selected here.
The fixed clock and bounded catch-up are independent host policies
(`src/physics.h:14–25`, `src/main.c:7460–7467`). Four derivative/Euler substeps
result from one 1/60 tick (`src/drivetrain.c:87–91`). The actual timed-history
sampling/reset rules and upstream original frame clock remain unspecified.

**Applicable contract:** S4-0/2/3, U4-1/2; S1 U-1/2/3/5; S3 U3-1; S5 U5-4/7.
**Observable consequence (inference):** command ramp, reversal and held-input
limits depend on these chosen histories; analog and manual-shift feel cannot be
assessed through this keyboard/automatic-only adapter. The handoff safety policy
can withhold accelerator for a tick after the gears agree; it does not erase
incoming forces or wheel/engine energy (`src/drive_contact.c:74–86`).

The shift itself is not a fixed legacy 0.15 s cut: automatic selection and
clutch state progress in `src/drivetrain.c:251–280,354–383,425–442`. In all 11
recorded ordinary upshifts, target request to gear commit took 10 ticks
(0.1667 s), and target request to first observed Locked took 30 ticks (0.5 s).
This establishes timing in the prior 240SX-launch diagnostic only, not matched
350Z timing. Gear jitter is suppressed
without a supplied RNG (`src/drivetrain.c:167–174`), another allowed fixture.

### 6. C — Current high-speed observations do not establish missing sliding behavior or top-end fidelity

The lateral curve, combined cap, post-cap longitudinal correction and tyre yaw
moment are present (`src/lateral.c:64–75`, `src/drivetrain.c:521–608`,
`src/drive_contact.c:61`). The approved sliding predicate does **not** introduce
a speed-only breakaway rule: with A=12, folded lateral angles cannot exceed A
in radians, so longitudinal slip >.5 or lock is the remaining sliding trigger
(`src/drivetrain.c:537–547`; S4-1/5). A synthetic alpha=1.5 rad, vx=1, vy=100,
s=0 remains gripping; s=.501 switches to sliding. This follows the approved
contract and is not a demonstrated implementation defect. Gripping still has
nonzero lateral force and a combined friction limit; it does not mean no sideslip.

The current trace reaches only 39.677948 m/s (142.84 km/h). At >=30 m/s, 97.30%
of sampled wheel states are gripping. That observation is compatible with the
predicate; it cannot establish that sliding is wrongly absent or that a new
speed-dependent slide rule should be invented. There is no original runtime
trajectory oracle in any of the five approved stages (U4-6/U5-1).

The 200 m/s absolute cap remains implemented (`src/resistance.c:92–100`), as
does the per-car force replacement (`src/resistance.c:76–86`). The supplied
trace cannot exercise the absolute cap and does not log enough inputs to
prove per-car threshold activation. No top-speed/cap defect is claimed.

### 7. C — S4's named response-scale cap input lacks a clear composed API mapping

S4-6 names `response_scale` separately in its final cap product. The composed
cap (`src/drivetrain.c:597`) has surface, axle, frame grip, load, player rear
speed and peak-scale factors, but no separately named response-scale input.
`ResInput.scale` is copied to torque scale and used by aero/speed limiting
(`src/drive_contact.c:72`, `src/resistance.c:24–25,41,61`); the approved specs
do not explicitly identify that G with S4's response-scale name. Substituting G
into the cap without clarification would guess an unresolved identity.

The runtime declares response scale 1, so this omission/ambiguity cannot explain
variation in the current unity fixture. It is a contract/API follow-up before
claiming support for a supplied nonunit response-scale fixture. No math fix or
new multiplier recommendation is made here.

## Prior 240SX-launch diagnostic bounds

Snapshot: `/tmp/feel-integration-audit/manual-snapshot.csv`, copied from the
authorized `/tmp/openug2-stall-live.csv`. 7,942 complete active rows, ticks 0–7941.
All contact masks are 15; all wheel surface IDs are 1. A contact flag does not
imply positive load under S5-3/5: zero-load wheels occur within this trace.

| Observation | At speed >=30 m/s |
| --- | ---: |
| Rows / rows with abs(raw steer)>.1 | 714 / 349 |
| Wheel gripping fraction | 97.3039% |
| Load range | 0…11.99517 kN |
| Maximum abs(longitudinal slip) | 1.95909 m/s |
| Prepared Mz range | -19.49260…15.63979 kN m |
| Prepared My range | -20.60540…14.21927 kN m |
| Prepared Mx range | -13.67895…8.34181 kN m |
| Prepared Fz range | -18.16343…6.43911 kN |
| Ground-motion-limited rows / wall-or-rail rows | 0 / 1 |

These are prepared, next-derivative accumulators at tick exit, not per-wheel
uncapped requests. The trace has no direct yaw-rate, steering-command, frame-grip
or pre-cap-vector fields, so exact cap occupancy and yaw-momentum damping cannot
be reconstructed from it alone. All high-speed force/moment extrema above are
within this particular run's approximately 108–143 km/h sample.

## Force/state consumption map

| Calculated output | Gameplay consumption |
| --- | --- |
| Final transformed wheel Fx/Fy and tilted normal XY | Body XY momentum and subsequent position |
| Wheel Mz / aero Mz | Supplied scalar yaw integration |
| Wheel/aero Fz, gravity Fz | Accumulated/integrated temporarily; resulting vz overwritten by ride each substep |
| Wheel/aero Mx/My | Recorded; no pitch/roll momentum integration |
| Aligned load N | Tyre curve, brake lock/relief, sliding force, final cap |
| Aero preload feed-forward | Spring preload; retained independently from actual aero force |
| Bottoming impulse request | Reported; application suppressed; extra zero sweep retained |
| Contact yaw multiplier kI | Component helper output; gameplay chooses unscaled inertia |
| Surface channels other than ID1 | Available through injection; live query always selects ID1 |
| Handbrake blend, rear slip-bonus grip | Computed once per frame and used by tyre preparation/cap |
| Host collision velocity | Imported on next tick; no collision angular-impulse channel |

## Reproducible checks and limits

Only private audit probes were built/run; no source/tests were changed and no
full suite or native game run was requested. `/tmp/feel-integration-audit/probe.c`
uses existing independent synthetic test fixtures. Its assertions all passed:
unconsumed kI, prescribed brake pose, lateral-branch thresholds. The keyboard
fixture additionally reported a first-tick 3.04321-degree command and 25.01587
degrees after 61 held-input calls at 30 m/s, illustrating the history-dependent
filter without claiming an original command trace.

```sh
cc -std=c11 -O0 -Isrc /tmp/feel-integration-audit/probe.c \
  src/drive_contact.c src/contact.c src/drive_stage1.c src/drivetrain.c \
  src/braking.c src/lateral.c src/resistance.c src/physics.c \
  -o /tmp/feel-integration-audit/probe -lm
/tmp/feel-integration-audit/probe
python3 /tmp/feel-integration-audit/trace_summary.py
```

The summary script reproduces the table and gear events; shift Locked times
were checked by finding the first row at/after each target change with matching
gear and clutch=3. The snapshot is private audit data, not a committed asset.

Follow-up questions are recorded here because this round owns only this report:
What is S4-6's separate response-scale input and its relationship, if any, to G?
Which reviewed angular-mode/damping producer should replace the current explicit
fixture? What timed-history/device/reset producer should replace the fixed boxes?
These are specification/measurement questions; none was silently implemented.

No source, tests, approved specifications, vector files or shared QUESTIONS.md
were edited. This report alone is the shared output of this audit.
