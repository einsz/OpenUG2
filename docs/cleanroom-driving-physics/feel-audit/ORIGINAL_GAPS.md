# Original physics feel: bounded gap audit

2026-10-15. **Reviewed findings for communication only; not approved for implementation.**
This supplements, without modifying, approved Stages 1–5. Every original-law
finding below is **inferred from static evidence**, cross-checked against the
intake executable. None is runtime-observed. Synthetic examples illustrate
arithmetic, not retail trajectories. Evidence IDs resolve in the analyst-only
register. No numeric handling parity or proven cause of the user's subjective
experience is asserted.

These are confirmed omissions from the bounded approved contracts. The allowed
question records Q4-I1/Q4-I2 and I5-1 additionally document independent host
substitutions for history, angular response, and prescribed body ride motion.
The original-behavior analyst did not inspect implementation or the independent
integration audit.
Priority ranks likely explanatory reach for steering, acceleration, braking,
shifting and high-speed cornering; it is an inference, not measured causality.

## 1. Dynamic body angular response and suspension feedback — highest priority

**Confirmed boundary:** S4-6/U4-5 and S5-8/U5-6 leave the body/angular solver
external. I5-1 explicitly identifies prescribed heave/pitch/roll rather than a
complete dynamically coupled trajectory. The suspension formulas alone therefore
do not establish braking pitch, acceleration squat, roll, or consequent grip.

**Recovered behavior, EFA-1:** use world linear momentum P, world angular
momentum L, body-to-world orientation B and mass M. For selected body inertia I,
`V=P/M` and `Omega=B*inverse(I)*transpose(B)*L`. Position derivative is V;
each body basis vector b has derivative `Omega cross b`. World momentum
receives force and moment derivatives on the existing derivative/Euler boundary.
The stock basic and adjusted inertia diagonals are those in S5-1; adjusted yaw
inertia multiplies the basic yaw inertia by kI. State codes 0/2 select adjusted
inertia when a supplied inertia selector c is
nonzero, basic inertia when c is zero. State code 1 selects basic inertia. The
identity and upstream production of c are not established by this reviewed
capture; it must not be equated to the contact-count or torque-limit selectors.
Other state codes require an explicit mode input rather than a guessed mapping.

The transition from adjusted to basic additionally replaces **world** L.z by
`L.z/kI`; transition to adjusted has no reciprocal momentum adjustment. This
asymmetry is observable state behavior, including when the body is tilted. Do
not describe it as conserving yaw angular momentum across both transitions.
Post-step publishes Omega limited to magnitude 250 rad/s; this limit rescales
Omega without rewriting L. Body orientation is orthonormalized once after each
completed integration substep. The exact numerical repair is outside this audit.

The Stage5 point forces already supply `F` and `r cross F`. Their moments can
change B, which changes geometric travel, travel rate, spring loads, contact
velocities and next prepared tyre forces. This closes a real feedback loop;
a prescribed pose cannot establish the same response. No direct algebraic
acceleration-to-axle-load formula has been established as a substitute.

The bottoming interface's omitted per-application impulse limits are also
resolved: cap the requested linear impulse vector to magnitude **1000 kN·s**,
compute its moment from that capped impulse, then cap the angular increment to
**10 kN·m·s** before adding it to L. These differ from Stage2 force limits.
Suppression and zero-duration post-step rules remain those in S5-4/8.

**Observable candidates:** turn-in acceleration, transient roll/load transfer,
braking balance, acceleration traction, slide recovery, and crest/landing motion.
Confidence: high for equations and predicates; no measured trajectory effect.

## 2. Ordinary angular damping — highest priority

**Confirmed omission:** S2-6 explicitly defers angular damping; Q4-I2 documents
zero additional damping at the independent gameplay boundary. Tyre-generated
yaw moment by itself is consequently an incomplete ordinary chassis contract.

**Recovered behavior, EFA-2:** let w=|Omega| and z be the existing Stage2
stabilization state. D(w) is endpoint-clamped linear interpolation of these
(rad/s, 1/s) pairs, with decimal parameters interpreted as binary32:
`(0,.05),(2,.05),(4,.25),(6,.7),(8,2),(10,5)`.
Given accumulated world torque T, the angular momentum derivative, before the
conditional vertical override in item 3, is

`Ldot=T - (angular_curve_enabled ? D(w)*L : 0)
          - (stabilize ? 2*z*L : 0)`.

The two gates are independent. Ordinary state codes 0/2 initially enable both;
state 1 disables both. Under the supplied exceptional race-state code 4,
throttle strictly greater than binary32 .3, or all four exported contacts plus
`abs(body_forward.z)` strictly greater than binary32 .15, disables `stabilize`
without disabling the angular curve. Full race-state production remains external.
The existing `-8*z*P` translational damping is separate and acts under the same
stabilize gate. Use pre-Euler z, as in Stage2.

At L=(0,0,2), w=2, z=0, T=0 with the curve enabled, Ldot.z=-.1.
At w=8 with the same L and z, Ldot.z=-4. These are synthetic component inputs;
they do not assert a particular stock inertia or yaw rate at a given road speed.

**Observable candidates:** persistence of yaw after steering release, oscillation,
spin growth and recovery; braking/acceleration changes when pitch/roll move the
shared stabilization state. Confidence: high static; no runtime magnitude claim.

## 3. Slip-dependent vertical torque restriction — high priority

**Confirmed omission:** angular/body unknowns leave this behavior outside prior
approved contracts. This is distinct from alternate-family yaw assistance.

**Recovered behavior, EFA-3:** at frame refresh, for local movement, select a
limit K from two effective source-car scalars: row +0x398 when the established
numeric selector p equals zero, row +0x39C otherwise. These offsets are
inside the same 2,192-byte source row used by S5-1. They are not contact count.
For p!=0 only, if the selected K is exactly zero, let sigma be the established
wrapped body-minus-velocity heading in radians, set to zero when exported signed
forward speed is below 1 m/s. If `sigma*L.z<0` and `abs(sigma)>b32(.1)`, set K=20
kN·m. Exact zero product or exact .1 does not trigger. Remote movement has its
own tiny override and is outside the local contract.

At derivative evaluation, if K!=0 and **raw accumulated** T.z>K, replace Ldot.z
by K; if raw T.z<-K, replace Ldot.z by -K; otherwise retain the already damped
Ldot.z. Thus this is not `clamp(T.z-damping,-K,K)`: it tests raw torque but
conditionally replaces the derivative after damping. No body-axis conversion
occurs at this override. Valid nonnegative K is the bounded fixture domain.

Synthetic distinction: K=20, raw T.z=25 and damping contribution -3 yield
Ldot.z=20, not 17. With raw T.z=19 and damping contribution -50, Ldot.z=-31;
the raw torque was in range. Effective per-car values and upstream selector/race
selection must be retained or explicitly supplied.

**Observable candidates:** countersteering response and transitions while sliding,
especially torque reversal. Confidence: high for local law; activation frequency
and effect on the user's selected car are unmeasured.

## 4. Steering history producer — high priority for response

**Confirmed omission:** U4-2/Q4-I1 leave an independent 60 Hz FIFO at gameplay
entry. Original static evidence establishes a different capacity/expiry contract.

**Recovered behavior, EFA-4:** input and keyboard-rate histories start empty,
with sum, count, average, write position and expiry position zero. Their windows
are binary32 .55 s and .15 s. Capacity is
`trunc(window*120+.5)`, giving **66 and 18 slots**. The 120 value calibrates
capacity; it does **not** prove simulation cadence, device poll cadence, or a
fixed sampling interval. Each actual history call records a separate sample;
equal timestamps are not coalesced and no intermediate samples are synthesized.

For the ordinary positive, nondecreasing-clock boundary, retain samples in a
bounded ring, remove overwritten samples from the sum, remove samples whose
age is **strictly greater** than the window, and publish sum/live-count. Exact
window age is retained. Empty allocated slots do not dilute the average. Clock
seconds come from the global tick clock multiplied by binary32 .00025, not
from accumulated caller dt. Input history appends the current mapped input
**after** computing the steering command. For keyboard, the rate history
includes the current change-rate sample **before** the angle rate is calculated.
Analog does not insert a keyboard rate sample on its ordinary direct-angle path.

The original zero value/zero timestamp pair also marks an unused slot, and
expiry deletion checks positive timestamp. Complete time-zero, backwards-clock,
reset and count-overflow behavior remains outside this ordinary boundary;
a full specification must preserve or separately test those edge conditions.

Synthetic timing distinction: on the .55 s input history, samples (time=1,value=1)
and (time=1.25,value=-1) average 0; appending (1.75,1) expires the first sample
and leaves average 0. A fixed sample-count FIFO can retain the first sample and
produce 1/3. At startup, the first positive-time sample 1 averages 1, not 1/66.

**Observable candidates:** initial turn-in, repeated steering taps, holding the
steering limit, release and countersteering at different frame rates. Confidence:
high for ordinary history law; complete upstream call cadence remains unknown.

## 5. Shift-adjustment grip composition — conditional follow-up

**Confirmed omission:** S5-6/U5-3 explicitly excludes this multiplier although
Stage1 specifies a shift-adjustment state. This is a downstream grip effect,
not evidence that every ordinary shift triggers it.

**Recovered behavior, EFA-5:** after frame brake/handbrake/rear-grip distribution
and before substeps, let A be the current shift-adjustment state and R the
positive effective calibration selected by drivetrain variant and tuning index.
For the two prepared wheel-grip coefficients whose complete operations are
present in the reviewed direct capture, A>0 multiplies the coefficient by
`1+min(1,A/R)`. A<=0 bypasses those multiplications. The capture stops before
the remaining complete operations; an all-four-wheel law is not established by
this review and needs a complete direct capture before a full contract. The
captured multiplications are not guarded by the computed launch-enable value;
complete launch/shift composition still needs the missing capture. Do not apply it again per
substep or treat it as engine torque. Example: A=.5R gives a 1.5 multiplier;
A>=R gives 2. These are supplied-input synthetic cases.

**Observable candidates:** wheelspin and acceleration around shifts, transient
grip during a shift in a fast corner. Confidence: high for the captured downstream
arithmetic, incomplete for total wheel coverage; actual R selection,
A initialization/decay/trigger reachability in the user's
mode and tuned car require further producer tracing before end-to-end claims.

## Unknowns / ambiguities and next specification boundary

No retail runtime run, original differential trace, measured acceleration,
stopping distance, shift duration or high-speed cornering trace exists for this
audit. These mechanisms could explain symptoms; that causation is not proven.

A useful next reviewed contract would combine items 1–3 into a body-response
amendment, close item 4's ordinary timed-history boundary, and add item 5 only
with explicit A/R inputs until its trigger/calibration producers are resolved.
This is proposed specification scope, not implementation design or authorization.

World collision selection and material-to-surface identity remain U5-2; approved
numeric road-ID fixtures do not establish original terrain semantics. Complete
base grip, shared response, drafting and collision-event producers remain U5-3.
Alternative race-family yaw torque/rear-peak assistance exists conditionally, but
was not established as an ordinary-driving requirement and should not be added
to ordinary high-speed handling on that assumption. No newly established defect
in the approved engine torque, service-brake torque or basic shift-selection
arithmetic is claimed here. Those symptoms can also depend on the above body,
load, history, grip and external-input boundaries.

Additional airborne opposing torque, full mode/selector reachability and exact
orientation repair remain outside this audit. These findings do not establish
a complete ground/air angular solver.

The illustrative inputs above were invented and their outputs were calculated
from static-inferred equations; none is an original-runtime observation, a
public-source capture or a copied original asset. They check arithmetic only.

The five evidence entries and captures were reviewed for traceability. These
findings do not supersede any approved spec until a separately scoped full
behavioral specification and test vectors receive a fresh gate.
