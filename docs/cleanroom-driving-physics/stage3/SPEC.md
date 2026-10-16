# Stage 3 — ordinary service braking

Companions: Stage 1 `../SPEC.md`, approved SHA-256
`61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb`, and Stage 2
`../stage2/SPEC.md`, approved SHA-256
`d3072359aa25bf0ef541504805f70b3579829bb9b1310cda8ba1a568286c6cb6`.
Their units, frame subdivision, drivetrain, contact-input boundaries and force
lifecycle continue to apply. This document supplies their deferred brake behavior.
The overlap rules below apply when these three contracts are composed; they do
not amend either companion file.

- S1-5.1 and S3-2 describe one shared pedal-exclusivity operation, applied once
  per frame. Its remaining throttle then enters the engine-only S1-5.2/5.3
  shaping; the service pedal enters S3-2 directly.
- The S1-9 shorthand that brakes/lock are zero with pedals zero describes the
  Stage1 brake-free fixture boundary. It is not a release/reset rule for an
  existing Stage3 lock: S3-4 specifies retention and reevaluation. Likewise,
  S3-3 relief can affect a negative drive/drag torque even when B is zero.
- S3-5 gives the scope of S1-8.1 locked angular-acceleration zeroing: apply it
  to the torque/drag result, then apply the separately conditional gripping
  correction. S3-6 supplies the direction-crossing rule deferred by S1-8.1.
- S2-7 remains the body-force lifecycle. Brake-modified final tyre forces enter
  the same contact-force accumulation once, alongside the existing aerodynamic
  force. A host service-brake deceleration, direct velocity reduction or
  additional equivalent contact force must be reconciled so the Stage3 service
  contribution is counted once. Stage2 stabilization and imposed speed rules
  continue to apply on their own stated predicates.

**Evidence status:** executable behavior is **[inferred]** from static examination
of the intake executable. Constants were read directly from that executable.
No retail physics runtime or original-instruction execution was performed.
Vectors are synthetic calculations from these requirements, not captures.
No guessed retail default is normative. Approval is recorded in REVIEW.md for the
exact SHA-256 of this file.

## S3-0 Domain and vocabulary

Use tonnes, metres, seconds, kN, kN·m and rad/s. All persistent state and supplied
parameters are binary32; `b32(x)` denotes rounding x to binary32. Exact extended
intermediate rounding remains U3-4. Inputs are finite, wheel radius and inertia
positive, loads and brake strengths nonnegative, pedals and rear share in [0,1].
NaNs, malformed injected records and negative load/strength inputs are excluded.
Wheel indices 0/1 are front and 2/3 rear.

Per-wheel state includes angular speed `omega`, direction `dir` (forward,
stopped, backward), Boolean `locked`, the prepared torque `tau`, stored slip
`s`, sliding/gripping flag, and smoothed net torque from Stage 1. These are
separate states: omega zero does not necessarily imply locked. Wheel construction
sets dir to stopped. Supply lock state explicitly and supply direction when
entering at an arbitrary saved-state boundary (U3-2). Test initial states are fixtures.

The main Stage3 domain is ordinary circuit/sprint/free-roam with handbrake
pedal zero. S3-8 defines the minimal shared handbrake interface needed to avoid
contradicting Stage5; it does not establish handbrake/yaw/grip producers.

## S3-1 Effective stock data

Framing follows S1-1. Offsets are relative to the 2,192-byte source car row,
not the effective sub-record. Fields are little-endian binary32.

| Row offset | Symbol | Meaning |
| ---: | --- | --- |
| +0x384 | Kb | Service-brake strength, kN·m before the factor 4 below |
| +0x388 | Kh | Handbrake strength, kN·m before the factor 10 below |
| +0x38C | beta | Rear service-brake share, dimensionless |

For stock ordinary composition these three values pass through unchanged.
Upgrades, tuning and other mode variants can change effective values and are
deferred. The adjacent row field +0x380 is not a substitute for Kb. Do not
apply mass, wheel radius, gravitational acceleration, mph conversion or a
second axle split to Kb. The torque formula already specifies its scale.

## S3-2 Pedals and once-per-frame distribution

Let `u,b,hb` be simulation-facing throttle, service brake and handbrake pedals
at the entry to Stage1's pedal exclusivity rule. Hardware mapping, driver
reverse/brake swapping and device filtering precede this boundary (U3-1).
With exclusivity enabled, apply S1-5.1 to all three pedals once: if
`u>b` and `u>hb`, set each of b and hb below 0.5 to zero; otherwise set u to
zero if u<0.5. Equality with 0.5 retains the value. With exclusivity disabled,
retain the supplied pedals. Ordinary-family tests explicitly enable it;
complete mode mapping remains Stage1 U-12.

There is no additional service-pedal power curve, dead zone, rate limiter,
ABS oscillation timer or smoothing in the brake distribution examined here.
Stage1's throttle dead zone, anti-stall and clutch throttle factor are engine
rules; do not apply them to b. For the simultaneous-pedal exception below,
use the throttle pedal remaining after exclusivity, before engine/clutch
throttle factors.

Define `vmph=|V|*b32(2.236024856567383)`, using full body-speed magnitude,
not signed forward speed. Each frame assigns service coefficients:

`a0=a1=2*(1-beta)*b`.

`a2=a3=2*beta*b`, except if `u>b32(0.8)`, `b>0.5` and `vmph<10`, then
`a2=a3=vmph*b32(0.05)`.

The exception applies to both rear wheels regardless of which axle is driven;
front coefficients retain their normal formula. It applies to both travel
directions. Exact equality at any strict threshold takes the ordinary split.
In particular at zero body speed under this simultaneous-pedal condition,
rear service coefficient is zero even for a nonzero service pedal.
Coefficients are held across that frame's substeps; do not refresh vmph per
substep or integrate an extra pedal state.

Per-wheel nonnegative service torque magnitude is `B=4*Kb*ai` kN·m.
Without the exception, the four-wheel sum is `16*Kb*b`; each front wheel
receives `8*Kb*(1-beta)*b`, each rear `8*Kb*beta*b`.
This sum describes requested torque, not guaranteed contact deceleration.

## S3-3 Signed torque and one-sided slip relief

At each coupling evaluation, Stage1 initial engine/clutch distribution runs
first and respects the **incoming** lock state. Its engine reaction is still
computed under S1-7.2 even when a locked wheel receives no drive torque.

Let `H>=0` be the shared handbrake torque magnitude (zero in service-only
fixtures). For a wheel whose incoming lock is false, add service torque and
then handbrake torque to its existing tau with sign `-` if omega>0, `+` if
omega<=0. If incoming locked is true, neither addition occurs. This sign uses
wheel spin, not gear, direction latch, body velocity or contact velocity.
At exact zero the addition is positive; the later lock/grip rules handle rest.

Sample slip `s=R*omega-vx`, using wheel-contact-frame longitudinal velocity vx
and the angular speed **before** the lock decision below. Store s for this
whole evaluation. Let `relief_disabled` be an explicit supplied Boolean
(U3-3). If all of the following hold, replace the combined tau with
`tau * (b32(0.5)/(-s))`:

- `s < -b32(0.5)` m/s;
- `tau < 0`;
- `H == 0`;
- `relief_disabled == false`.

Otherwise retain tau. This scales the combined torque including any drive or
engine-drag contribution; it does not scale only B, Kb, or the lock-test
magnitude. It can occur without service pedal input if the combined torque is
negative. Relief is one-sided in signed slip: no mirrored positive-slip rule
was found. Reverse tests must retain this asymmetry. No timer or pulsed
controller is established; do not replace this rule with generic ABS.

## S3-4 Lock decision and retention

Let `vy` be the lateral contact-frame velocity. Perform a new lock decision
only when both `sqrt(s*s+vy*vy)>0` and `(vx!=0 or vy!=0)`.
If this predicate is false, preserve incoming locked and omega; tyre
preparation takes its gripping branch. Pedal release alone does not reset
locked in this case.

For a new decision, let `mu` be the complete Stage1 sliding coefficient,
including supplied surface, axle, per-wheel and, when applicable, player
rear-speed factors. Evaluate those factors with the incoming lock and the
pre-decision omega. Use that same mu for this evaluation's sliding force even
if the lock decision changes omega/locked. Load N is the same supplied
nonnegative load used for the tyre force. Define

`Q=abs(vx)*N*mu/sqrt(vx*vx+vy*vy)` (kN),

`A=b32(1.2)*(B+H)` (kN·m),

`C=R*Q+b32(0.1)*abs(omega)` (kN·m).

B/H are the positive requested magnitudes from S3-2/S3-8, independent of
whether their signed torque was skipped or reduced in S3-3.

| Predicate | New locked | New omega |
| --- | --- | --- |
| A<=C | false | retain omega |
| A>C and A>1 kN·m | true | 0 |
| A>C and A<=1 kN·m | false | 0 |

A equality with C unlocks and retains spin; equality with 1 follows the third
row if A>C. No minimum hold time, hysteresis timer or unconditional pedal-zero
reset is specified. The lock decision does not clear or recompute tau, stored
s, its combined-slip magnitude or mu. In particular a newly stopped wheel can
still exert force based on its pre-stop s in this evaluation.

There is no direct contact-flag test in S3-3/S3-4. With supplied N=0, Q=0,
brakes can still slow/snap/lock wheel spin; their tyre force is zero through
the normal load/grip limit. Contact/load refresh and the source of N are
Stage5. Do not turn a zero-contact fixture into direct body braking.

## S3-5 Tyre, engine and body consequences

After S3-4, Stage1 tyre preparation chooses sliding when its nonzero-motion
predicate holds and `abs(s)>0.5`, the slip-angle threshold is exceeded, or the
**new** lock is true. Otherwise it chooses gripping. Use s sampled in S3-3;
do not recalculate s after the lock snap. Stage1's low-body-speed sliding-force
normalization still applies. The five iterative force passes run only for
gripping, unlocked wheels. Subsequent clutch lock corrections can add drive
torque to wheels that are now unlocked; they skip wheels that are now locked.

Stage1's final friction cap uses the newly current lock/omega when evaluating
the player rear-speed factor. This may differ from the factor used in S3-4.
The `tau>0` longitudinal 1.5 test is also still restricted to unlocked wheels.
The final stored tyre force is shared by wheel spin and body force as in
Stage1. Service braking enters the body's acceleration through that force and
its contact application, with the existing inertia, grip limits and five-pass
coupling. There is no separately evidenced service-brake body deceleration,
velocity multiplier or host speed clamp to add.

For a locked wheel the Stage1 derivative sets the torque/drag angular
acceleration to zero. The subsequent gripping-slip correction remains a
separate conditional operation: if gripping and substep h>0.001, subtract
`s/(R*h)` even if locked. A locked wheel ordinarily takes the sliding branch
under nonzero contact motion, but S3-4's bypass case can be gripping and retain
locked. Do not strengthen the zero-derivative rule into a broader invented
condition. The smoothed net-torque state still updates before the locked
zeroing; it is neither reset on lock nor on release.

An engine can remain coupled to locked or slow wheels under Stage1 rules.
Do not force neutral, disengage the clutch, set engine rpm to zero or suppress
engine braking solely from the service pedal. Idle clamps, anti-stall clutch
logic, engine feedback and finishing corrections remain those of Stage1.

## S3-6 Direction crossing after integration

This refines Stage1's phrase "changes rotation direction under braking":
the statically inferred direction-crossing guard is unconditional with respect to pedal,
contact and locked state.

At the wheel post-step, if old dir is forward and the integrated omega<0,
set omega=0. If old dir is backward and integrated omega>0, set omega=0.
For old dir stopped, retain either sign. Then derive the new dir:
forward if omega>`b32(0.00001)`; stopped if omega>=`-b32(0.00001)`;
otherwise backward. Exact thresholds are stopped; tiny nonzero omega inside
the band is retained. This guard does not itself set or clear locked.

As in Stage1, that post-step clears tau, tyre forces and s, and retains the
smoothed net-torque state. S3-4 can subsequently change omega without refreshing
dir; dir is refreshed at the next wheel post-step. The brake sign in S3-3
therefore uses omega directly. Angle handling remains Stage1's presentation rule.

## S3-7 Scheduling and host boundary

At frame entry, parameter/load/mode refresh precedes input shaping and brake
coefficient distribution; all precede the first derivative. Frame coefficients
do not retroactively change torque/force prepared by the preceding post-step.
Therefore the first derivative after a pedal change consumes previously
prepared tau/forces; the following coupling uses the newly assigned coefficients.
Do not run an extra coupling or brake integration to remove this lifecycle.

Each substep retains Stage1/2's derivative collection, Euler integration,
post-steps, contact refresh, aero and coupling order. Within coupling: initial
drive distribution, per-wheel signed brakes/slip relief/lock/tyre preparation,
five force/clutch correction passes, final friction cap, drivetrain finishing.
Any extra contact-correction post-step sweep has no extra Euler integration.

A host supplies the same car strengths/share, radius/inertia, pedal snapshot,
body-speed snapshot, contact velocities/loads, complete grip inputs and states
to all stages. Preserve the torque and force accumulation once per evaluation.
No stopping-distance curve is normative without a fully supplied contact,
load, lateral, body and drivetrain fixture. Road slope/aero/engine drag remain
independent established contributions, not alternate service braking.

## S3-8 Shared handbrake boundary with Stage5

Service-only Stage3 requires hb=0 and H=0. The combined brake preparation and
lock rules nevertheless consume a shared handbrake magnitude:
`H=10*Kh*eta_i`, where eta_i is the effective per-wheel handbrake coefficient.
Stage5 must supply eta_i and any changed rear-grip multiplier together.
For hb=0 the statically inferred coefficient distribution produces eta_i=0 for all wheels.
The base pedal distribution is rear-only; its nonlinear boost and player
slip/speed eligibility are deferred rather than approximated here.

If Stage3 component tests inject nonzero H explicitly, they test only the
shared torque sign, relief suppression and lock rules. They do not claim a
complete handbrake simulation. Handbrake grip recovery/blending states,
yaw terms, surface/load producers and tuning remain Stage5. No handbrake
recovery state is a service-brake smoothing state.

## S3-9 Unknowns / ambiguities

- **U3-1** Full driver-device pedal mapping and reverse/brake role switching
  before simulation input; excluded from this boundary.
- **U3-2** Retail initialization/reset producer of lock state and general saved-state
  reset behavior. Constructor direction is stopped; the post-step updates are
  established. Explicit lock state is required; creation fixtures must not
  silently assume an unproved lock default.
- **U3-3** Semantic producer of the flag disabling one-sided slip relief.
  The Boolean is a required supplied input; fixture false is not proof of
  an every-vehicle retail default.
- **U3-4** Bit-exact x87 intermediate precision/rounding and marginal threshold
  differences. State is binary32; vector tolerances do not license changing
  strict comparisons. Equality fixtures use exactly representable values.
- **U3-5** No runtime/differential validation or measured stopping distances.
  Static formulas and derived vectors do not establish perceptual parity.
- **U3-6** Performance upgrades/tuning/mode composition of effective brake
  strengths/share; stock unchanged behavior only is covered.
- **U3-7** Complete nonzero handbrake coefficient/grip production and contact,
  weight-transfer, lateral and surface producers remain Stage4/5. Supply those
  inputs explicitly. Zero-load behavior is covered; airborne contact refresh
  itself is not newly specified here.

## S3-10 Traceability and vectors

`test-vectors/braking.json` and its README define synthetic component and
fixtures with inputs, expected outputs and tolerances. The scheduling contract
in S3-7 is not an end-to-end lifecycle capture or a numeric stopping fixture. Evidence IDs
E3-1 (data/distribution), E3-2 (sign/relief), E3-3 (lock), E3-4 (direction),
E3-5 (coupling/derivative), E3-6 (ordering) and E3-7 (shared handbrake boundary)
resolve to analyst-only evidence records. These identifiers contain no original
symbols or executable addresses. No original code/assets accompany this spec.
