# Ordinary-play session selector amendment

2026-10-16. Implementation is authorized only for the hash approved in REVIEW.md.
Companions: approved Stage 1–5 and body-response/SPEC.md. This amendment narrows
only the producer boundary for the session selector used in BR-2. No physical
coefficient, stabilization derivative, wheel lock rule or reset law changes.

All original-behavior claims are **inferred** from static instructions in the
hash-identified original executable. No original runtime was executed. Vectors
are synthetic arithmetic, not measured retail launches. Private evidence keys
OM-E1–OM-E5 resolve in the analyst register.

## OM-1 Selector identity

The input called `race` by BR-1/BR-2 is a **session lifecycle status**, separate
from event family (free roam, circuit, sprint, etc.), vehicle `state`, inertia
selector `c`, and driver-role selector `p`. The numeric value 4 identifies the
active session phase; it does not identify an exceptional event family. In
particular, testing the event-family number against 4 is not equivalent to
BR-2's predicate. [OM-E1, OM-E2]

The established partial lifecycle mapping is:

| Status | Established meaning |
| --- | --- |
| 0 | Initial status at session construction; also an explicit initialization input. This fact does not establish ordinary active driving with status 0. |
| 3 | Pre-active timed start phase. Entering it records the start-phase timestamp. |
| 4 | Active phase. Entry seeds running-session timing and clears the start-phase timestamp. |

Labels describe independently inferred behavior; they are not recovered enum
names. Status 1/2 and the complete lifecycle are outside this amendment.

## OM-2 Ordinary reachability

For the ordinary timed-start path, session status 3 advances to 4 when the
start-phase timing query reports completion. This transition has no test for
circuit versus sprint versus free-roam event family. Consequently an ordinary
circuit or sprint that has completed its start phase supplies status 4 to
BR-2 during active play. This does not assert the duration or frontend route
of every event variant. [OM-E2, OM-E3]

Active free roam can also supply status 4. The original has an explicit
free-roam branch in the return-to-play flow that sets session status 4, and
entry to status 4 contains free-roam-specific handling. The branch that sets
4 and the consumer that tests 4 operate on the same session-status object.
Free roam must therefore not be equated to session status 0 merely because
its separate event-family code is zero. [OM-E2, OM-E4]

These mappings cover active ordinary local play. A supplied snapshot from
loading, prerace, pause/replay, restart or another special route must identify
its actual lifecycle phase; mapping all lifecycle phases to 4 is unsupported.
No original runtime trace establishes every frontend route.

### OM-2a Producer boundary and independent host policy

At each frame mode refresh supply one session-status value independently of
event family, vehicle state, `c`, `p`, contact count and pedals. A caller with
an actual lifecycle snapshot passes that status unchanged. A component fixture
supplies it explicitly. An ordinary local host without that producer may use
the following **independent integration policy**, not a recovered original
frontend state machine:

- Supply 4 for a ready, active free-roam, circuit or sprint session, after any
  host start countdown has completed and while ordinary dynamics are advancing.
  Readiness requires a valid body state and the companion preceding force/torque
  preparation; this selector does not bypass those requirements.
- If the host simulates its pre-active timed start, supply 3 until its explicit
  countdown-complete condition; switch to 4 at the next mode refresh. No exact
  original countdown duration or clock is prescribed.
- Loading, pause/replay, restart/reset and special routes are outside that active
  mapping. Keep their established host scheduling or use an explicitly supplied
  lifecycle snapshot; do not fabricate an original numeric mapping. A pause
  that advances no dynamics performs no derivative/Euler step. Resumption into
  an explicitly active ordinary session uses 4 at the next refresh. A selector
  change alone never initializes, clears or restores body/wheel/history state.

The adapter must state whether it uses supplied lifecycle data or this policy.
Acceptance checks are: the ready active three-family cases all supply 4; a
simulated timed start supplies 3 before completion and 4 afterward; a paused
non-advancing interval integrates no state; and status changes alone preserve
`z` and prepared accumulators. Existing explicit initialization/restoration
contracts still apply. Unsupported routes require separate supplied inputs or
remain excluded. Neither event class, contacts nor player-role flags may stand
in for session status. This contract adds no launch assistance or history reset.

## OM-3 Damping consequences at launch

Replace BR-1's description of `race` as an exceptional race-state code with
session lifecycle status. Replace BR-2's description of its status-4 rule as
exceptional: the already specified rule applies during ordinary active play.
The mathematical predicates and refresh/substep boundaries remain unchanged.
[OM-E1, OM-E5]

This also narrows S2-1/U2-2's unresolved exceptional global stabilization
override and BR-7's unresolved `race` producer for the ordinary active domain
established here. It does not resolve other vehicle-state mappings, inertia
selector `c`, driver-role `p`, contact producers or excluded lifecycle routes.

For vehicle state 0 or 2 and session status 4, shaped simulation-facing
throttle strictly greater than b32(.3) disables stabilization at mode refresh.
This removes both translational `-8*z*P` and angular `-2*z*L` damping for that
frame's substeps. The independent angular-speed curve damping remains enabled.
Four previous contacts with abs(previous forward.z)>b32(.15) also disable
stabilization as BR-2 already states. Equality at either threshold retains its
previously specified meaning.

This operation changes the enable gate, not the stored history `z`: it does
not clear, reset or replace `z`, nor change the Stage-2 derivative or negative
clamp. Throttle release to at most b32(.3), with the contact/slope exception
false, enables stabilization again on the next refresh. Reapplication above
the threshold disables it again. This statement holds even if prolonged
zero-pedal waiting or handbraking has left `z` positive. Preparation latency
and the distinction between shaped throttle at refresh and later redistributed
pedals remain as specified by the companions.

A controlled stalled-state derivative with nonzero prepared force must thus
respond differently when the supplied lifecycle status changes from 0 to 4
while full shaped throttle is held: the latter consumes prepared force without
the stabilization momentum term. This establishes a discriminating component
check, not a predicted complete 240SX speed or engine trajectory.

## OM-4 Vectors and unknowns

See test-vectors/ordinary-mode.json and README.md. The vectors preserve the
same positive `z` across gate changes and distinguish session status from event
family. They do not authorize a reset/history-clearing intervention.

Unknowns: full lifecycle mapping and all event routes; independently confirmed
numeric circuit/sprint event-family mappings; complete vehicle-state, `c` and
`p` producers; original prolonged-idle/handbrake trajectories and launch times;
complete rest/reset/stuck/wheel-lock lifecycle; runtime clock and frame timing.
This amendment does not establish that all independently supplied fixture inputs
match an original stock car. It does establish that session status 0 cannot be
claimed as the recovered ordinary active-play value in place of status 4.
