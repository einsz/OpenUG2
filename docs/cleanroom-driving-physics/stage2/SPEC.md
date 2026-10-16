# Stage 2 — longitudinal resistance and speed limiting

Subject: retail PC executable identified in this directory's PROVENANCE.md.
Companion contract: Stage 1, `../SPEC.md`, approved SHA-256
`61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb`.
Stage 1 remains unchanged. This stage specifies ordinary circuit/sprint/free-roam
longitudinal behavior, with supplied contact, orientation and mode inputs. It does
not claim a complete suspension, collision, tuning, nitrous or race-mode simulation.

**Evidence status:** every executable behavior below is **[inferred]** from original
static instructions; parameter encoding/constants were directly read as data.
No retail runtime or original-instruction execution validates these formulas.
All vectors are synthetic calculations from this specification. No guessed
retail defaults are normative. Unknowns are listed in S2-9.

## S2-0 Conventions and scope

Use the Stage-1 units: tonnes, kN, kN·m, metres, seconds and rad/s. Let world
velocity be `V`, linear momentum `P=m V`, angular speed magnitude `w`, and the
orthonormal body basis have columns forward `f`, lateral `l`, upward `u`.
The world vertical coordinate is z. `B q = f qx + l qy + u qz` maps a local vector
to world coordinates. Positive longitudinal force points along `f`, regardless
of the direction of travel. `F` means the total world body force accumulator,
including wheel, gravity and aerodynamic contributions.

All persistent state and parameters are binary32. `b32(x)` below means the
nearest binary32 value of the stated decimal. Comparisons are strict where
written strict. Exact intermediate x87 rounding is outside this stage; vector
arithmetic tolerances are stated in the vector README. Inputs are finite,
`m>0`, positive aerodynamic speed scale, `0<=t<=1`, `0<=a<=1`, nonnegative
coefficients and enable/surface scales, valid contact counts and orthonormal bases. Malformed inputs, NaNs and infinities are outside the contract.

A standalone straight-line test must explicitly state: level body, zero angular
velocity, supplied wheel contact loads and surfaces, no braking/handbrake/nitrous,
and the state/scale inputs below. Those inputs are not silently inferred from
speed or gear.

## S2-1 Car data and environmental inputs

Car-record framing and the effective record convention are those of S1-1.
Offsets here are relative to the **2,192-byte source row**, not the effective
record's start. Fields are little-endian binary32.

| Row offset | Symbol | Meaning |
| ---: | --- | --- |
| +0x220 | m | Mass, tonnes; same mass as Stage 1 |
| +0x370 | cD | Effective quadratic body drag coefficient, kN·s²/m² |
| +0x374 | cF | Front aerodynamic vertical-force coefficient, kN·s/m |
| +0x378 | cR | Rear aerodynamic vertical-force coefficient, kN·s/m |
| +0x3A0 | L | Imposed positive-forward-force threshold parameter, mph; zero disables S2-5 |

Stock ordinary-mode composition leaves these fields unchanged. Performance
upgrades, tuning and mode variants can change effective coefficients; their
composition is deferred. Do not substitute a drag coefficient from a UI rating,
another game's density/area equation, or a wholesale retail table.

Additional inputs are:

- `t`: shaped throttle from Stage 1, not raw controller input.
- `G>0`: shared vehicle speed scale used by aerodynamics and S2-5. It starts at 1.
  Its general producer remains U2-1. The frame setup copies the same value to
  both uses. Do not identify it unconditionally with the engine torque scale:
  an additional exceptional engine-only multiplier exists.
- `a`: body-drag reduction input. The aerodynamic multiplier is `1-a`; initial
  reduction-state initialization and its frame producer are not established by
  the reviewed evidence (U2-1). Supply `a` explicitly, including at creation.
- `e`: body-aerodynamic enable scale. Ordinary vehicle-state codes 0 and 2 set
  it to 1, unless the special alternate movement flag sets it to 0. State code 1
  sets it to 0. Other state codes retain the preceding values (U2-2).
- `d`: body-drag surface multiplier. For state codes 0 and 2, with no contacting
  wheels it is 1; otherwise it is the mean of the contacting wheels' surface
  body-drag factors. State 1 sets it to 1. Contact flags and count refer to the
  same supplied contact snapshot. These surface body-drag factors are distinct
  from wheel rolling-drag factors `rho_i` (U2-3).
- `n`: number of contacting wheels, integer 0…4; `q=n/4`.
- `bonus`: nonzero per-car-instance flag selecting a 1.03 threshold multiplier.
  It is the creation flag also used by Stage 1's torque bonus; producer U-6 there.
- `j`: optional alternate aerodynamic mode selecting the orientation factor in
  S2-3. Ordinary family uses `j=false`; broader mode mapping is U2-2.
- `z>=0`: body stabilization state, initialized to 0; `stabilize` enables its
  translational damping (S2-6). State codes 0/2 enable it, code 1 disables it;
  an exceptional global mode can disable it again (U2-2).

For a controlled ordinary road fixture use explicitly supplied `G=1`, `a=0`,
`e=1`, `d=1`, each `rho_i=1`, `bonus=false`, `j=false`. This is a fixture
configuration, not proof that every gameplay state has those values.

## S2-2 Body drag

Define `s = |V|/G`. The raw aerodynamic drag vector is

`D = -cD * d * (1-a) * e * (2-t) * s * V`.

Each component of `D` is stored as binary32. It is added once to the body force
at world-relative arm `(0,0,b32(-0.1)*(1-t))` when `q>=0.5`; when `q<0.5` the
arm is zero. Arm effects on torque are deferred, but do not alter the added
linear force. The per-application force cap is S2-3's cap.

Consequences required by the contract:

- On level straight motion with `G=d=e=1,a=0`, `Dx=-cD*(2-t)*v*|v|`.
  Forward and reverse signs oppose motion. Drag is zero at rest and is present
  in the air when `e` remains enabled. It is not contact-gated wheel friction.
- At fixed state and throttle its magnitude is quadratic in speed. Closed
  throttle doubles this body drag relative to full throttle. Gear does not
  directly enter this formula. There is no additional air-density or one-half
  factor: `cD` is already the effective coefficient.
- Body drag and Stage 1's closed-throttle **engine torque** are separate effects.
  The latter travels through the drivetrain; it must not replace this force.

## S2-3 Vertical aerodynamic forces and accumulation boundary

These forces must be accounted for in world-coordinate motion on slopes or when
passing aerodynamic loads to a later contact stage. Their direct projection onto
the orthogonal body-forward axis is zero. Their suspension feedback is outside
this stage.

Let `r=max(u.z,0)` if `q<0.5`, else `r=1`. For `s>b32(0.0001)`, let
`c=max(f dot (V/s),0)`; otherwise `c=max(f.x,0)` (the low-speed fallback uses
world vector `(1,0,0)`). This is intentionally `V/s`, so its length is `G`,
not generally 1. Define `A=c^4` if `j=true`, otherwise `A=max(c²,b32(0.4))`.

If `q=0`, set `W=(cF+cR)*A*r*s`, `WF=b32(0.4)*W`,
`WR=b32(0.41)*W`. Otherwise set `WF=cF*A*r*s`, `WR=cR*A*r*s`.
Apply world forces `B(0,0,-e*WF)` at relative arm `B(1,0,0)` and
`B(0,0,-e*WR)` at `B(-1,0,0)`, once each, in that order after body drag.
The reported scalar aerodynamic vertical load is `WF+WR`, before multiplying
by `e`; it is not necessarily the sum of the enabled applied forces.
The formula is linear in `s`, not quadratic. It is independent of `a` and `d`.

For **each** point-force application above, if its force magnitude exceeds
1200 kN, scale that force to magnitude 1200 before adding it to the body.
The corresponding moment is `arm cross force`; scale that moment to magnitude
100 kN·m if necessary before adding it to the angular accumulator. Caps are per
application, not on the sum. Ordinary vectors are far below them.

In a level straight Stage-2 fixture vertical forces have zero longitudinal
projection. Their effect on wheel loads must be supplied consistently by the
later contact/load stage; they are not a second direct rolling-resistance term.

## S2-4 Wheel rolling resistance

Stage 1 already defines this wheel derivative; Stage 2 **does not add it again**.
For each wheel its dissipative torque is

`Troll_i = -b32(0.002)*rho_i*omega_i - b32(0.1)*damage_i*omega_i`.

`damage_i` is a 0/1 flag copied from the corresponding bit of a four-wheel
vehicle flag mask during frame setup. Complete damage-state semantics remain
U2-4. This flag is independent of the wheel-contact flag. Both terms act on
wheel angular velocity, including when airborne and in neutral. They reverse
sign when the wheel reverses, and vanish when its angular speed is zero.
A locked wheel follows Stage 1's locked-wheel rules instead of receiving an
extra body resistance force.

Include this torque once in S1-8.1 using the existing wheel inertia and smoothed
net torque. The explicit gripping slip correction of S1-8.1 remains subsequent
and uses the same substep duration. The rolling term is not included in the
smoothed net-torque state. Do not replace the actual solver with a generic
constant `Crr*m*g` body force. For comparison only, a perfectly rolling wheel
at radius R would have `omega=v/R` and an opposing equivalent force
`Troll/R`; that identity is not an additional body-force application.

## S2-5 Per-car imposed threshold

This is a force rule, not a direct speed clamp and not an equilibrium estimate.
At the beginning of each body derivative evaluation, when `L!=0`, let
`b=f.z` and `k=min(abs(b),b32(0.3))`. Define the slope factor

- if `b>0`: `H=1-((1-b32(0.9))/b32(0.3))*k`;
- otherwise: `H=1+((b32(1.1)-1)/b32(0.3))*k`.

The threshold is `T=(G*L+b32(1.9))*(bonus ? b32(1.03) : 1)*H`.
If `sqrt(Vx²+Vy²)*b32(2.236024856567383) > T` **and** `f dot F > 0`,
replace the forward component of the current body-force accumulator with
`min(-5*k,-0.5)`, retaining its other body-frame components. The modified
force is also the force subsequently stored in that accumulator. Otherwise
leave it unchanged. Equality at either strict predicate does not activate it.

The threshold is mph; the measured speed uses world horizontal magnitude,
not full 3-D magnitude and not signed forward speed. The trigger's force sign
is always **positive body forward**, independent of travel direction. Thus a
reverse-driving negative propulsive force does not activate it; a positive
force during reverse travel can activate it if the magnitude test passes.
At level orientation an activated forward force becomes -0.5 kN. This allows
overshoot and external acceleration; it does not assert `speed<=T` each tick.

## S2-6 Additional body stabilization

There is a further translational momentum damping term, separate from both
quadratic aerodynamics and wheel drag. Let `x=max(|V|/4,w/2)`. Evaluate the
piecewise-linear curve C with these (x,value) pairs and endpoint clamping:
`(0,1.5), (1/7,1), (2/7,0.6), (3/7,0.22), (4/7,0.06),
(5/7,0.03), (6/7,0.015), (1,0)`; ordinates are binary32.

`zdot=b32(0.1)` when `C(x)>z`, otherwise `zdot=-10`.
The Euler update is `z <- z+h*zdot`; the body's post-step clamps negative z
to zero, with no upper clamp. When `stabilize` is true, after S2-5 the linear
momentum derivative is `Pdot=F-8*z*P`. Otherwise `Pdot=F`.
This damping acts on all velocity components and both travel directions.
It uses the pre-integration z, not the just-integrated z. It is not a constant
road rolling coefficient, and is not omitted merely because aero is disabled.
Angular damping also exists; its specification and the angular state producer
are deferred. Supplied `w=0` is sufficient for the straight fixture.

## S2-7 Absolute speed cap and scheduling

After Euler integration the body post-step derives velocity `V=P/m`. If the
full 3-D magnitude exceeds 200 m/s **and mass exceeds the binary64 constant
0.7 tonnes**, scale all components of P by `200/|V|` and recompute V.
The magnitude used as the divisor crosses a binary32 store. Equality does not
activate either predicate. The cap applies equally in forward and reverse,
independent of L, gear and throttle. A binary32 mass written as decimal 0.7 is
slightly below the binary64 threshold. This is an imposed cap, nominally
720 km/h, separate from S2-5 and any physical balance of forces.

The existing Stage-1 frame subdivision/Euler order is retained. In particular:

1. Frame setup refreshes parameters, mode/surface/scale inputs, throttle,
   damage flags and contact fraction; it does not add an extra resistance pass.
2. The derivative collection sees accumulated forces prepared by the preceding
   post-step. The body derivative first applies S2-5 and then S2-6. The wheel
   derivative includes its one Stage-1 rolling torque. All derivatives precede
   integration of any state.
3. After integration the body post-step applies the cap, clamps z, saves and
   clears its force/moment accumulators, then seeds gravity if enabled. The
   other blocks run their post-steps in Stage-1 order. Contact correction may
   cause another post-step sweep without an additional Euler integration.
4. Aerodynamics S2-2/S2-3 is added **before** the five-pass Stage-1 wheel/body
   coupling. Therefore the iterative contact acceleration sees it already in
   the external body force. It remains accumulated for the following body
   derivative; do not add it a second time at integration.
5. Later work can add other contact/mode forces. A post-coupling opposing term
   depending on angular velocity is angular torque, not extra longitudinal
   aerodynamic resistance. Its detailed rule is outside this stage.

Creation performs post-processing/coupling at zero substep duration before the
first ordinary derivative. It initializes z to zero; use only the scale initial
values explicitly established in S2-1 and supply `a` as an input. The unknown
upstream frame clock remains U-5
of Stage 1; the resistance formulas do not establish a render cadence.

### Host integration boundary

Stage 2 adds the specified body-force, stabilization and speed-rule behavior to
Stage 1. It refines the existing wheel-resistance inputs without replacing or
adding another copy of S1-8.1. It does not supply a new drivetrain or tyre solver.
A host adapter must use the same mass, body basis, momentum, velocity, shaped
throttle, contact snapshot and substep duration across both stages. Convert its
coordinate and unit conventions explicitly at that boundary.

The host must supply effective `cD/cF/cR/L`, `G`, `a`, mode/enable inputs, contact
flags and surface factors, damage bits and deferred contact loads. Known state
mappings in S2-1 apply when those state codes are used; other modes require
explicitly supplied values. Unknown producers are not permission to insert
unlabelled gameplay defaults. Fixture choices must be recorded as fixture inputs.
Preserve the prepared-force lifecycle in S2-7: aerodynamics enters the external
force seen by the wheel coupling and the next body derivative exactly once.
A host that already adds an aerodynamic or equivalent rolling body force must
reconcile that contribution with this contract so it is counted once. The
existing Stage-1 timing adapter remains subject to U-5; this stage establishes
neither frame cadence nor a complete retail clock.

## S2-8 Meaning of top speed

A **physical equilibrium** can occur where the coupled wheel drive, engine drag,
body drag and any active stabilization give zero longitudinal acceleration.
At full throttle in a controlled level fixture with z=0, the body aerodynamic
contribution alone is `-cD*d*(1-a)*e*v*abs(v)/G`; the drive and wheel terms
still come from Stage 1. This is not an instruction to impose an additional
terminal speed or to count the equivalent wheel drag as another force.

**Gearing/rev limits** come from Stage 1's torque/rpm/gear rules and rolling
kinematics. The kinematic relation `v=R*omega_e/(gear_ratio*final_drive)`
requires actual no-slip coupling on the chosen driven axle; it is not a universal
velocity clamp. A speed display/dyno estimate is not itself a body constraint.

**Imposed rules** covered here are S2-5's per-car positive-force replacement and
S2-7's global cap. Neither should be replaced with a generic terminal-speed
clamp. No claim is made that these are the only limits in every race mode,
AI controller, nitrous or upgrade configuration.

## S2-9 Unknowns / ambiguities

- **U2-1:** Complete gameplay producers/ranges of G and a, and initialization
  of the reduction state represented by a. G starts at 1 and its local shared
  copy behavior is established; broader drafting/assist/AI interpretation and
  reduction-state creation defaults are not proven.
- **U2-2:** Full user-visible mapping of vehicle-state codes, exceptional global
  stabilization override, alternate movement flag and alternate aero mode j.
  Ordinary/special-state input contracts are explicit; other modes are deferred.
- **U2-3:** Complete surface factor table and contact/load producers deferred to
  Stage 5; the two surface coefficient channels must remain distinct.
- **U2-4:** Complete damage flag producer/lifetime, distinct from its per-frame
  0/1 wheel mapping and damping formula.
- **U2-5:** Upgrade/tuning/mode composition of effective cD/cF/cR/L is deferred;
  use supplied effective values, not assumed raw rows for upgraded vehicles.
- **U2-6:** Runtime and end-to-end coast-down/terminal-speed traces unavailable.
  All vectors establish specification consistency only.
- **U2-7:** Intermediate rounding, exact FPU precision and upstream timing are
  Stage-1 U-14/U-5; this stage is tolerance-based.
- **U2-8:** Full angular, contact and suspension feedback is deferred. S2-3
  specifies applied aerodynamic force/arms; it does not specify new normal loads.
