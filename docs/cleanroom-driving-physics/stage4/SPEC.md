# Stage 4 — ordinary steering and lateral tyre response

Companions are Stage1 `../SPEC.md` (SHA-256
`61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb`),
Stage2 `../stage2/SPEC.md` (`d3072359aa25bf0ef541504805f70b3579829bb9b1310cda8ba1a568286c6cb6`)
and Stage3 `../stage3/SPEC.md` (`d4689a16861df810159760a6a668ef3714966fd6c20a30e3b2b54f355c41c6f7`).
They remain unchanged. This document replaces the supplied gripping lateral-force
boundary of Stage1 and establishes a bounded steering-to-planar-force contract.

**Evidence status:** all executable requirements are **inferred** from static
instructions cross-checked against the intake executable. Numeric constants and
car data were read directly. All vectors are synthetic mathematical cases;
none is a runtime capture. There is no retail handling/parity claim. The complete
human input-history producer and complete 3D chassis response remain explicit
boundaries. Approval is valid only for the hash recorded by a fresh reviewer.

## S4-0 Domain, state and composition

Use metres, seconds, tonnes, kN, kN·m, radians and rad/s unless a formula explicitly
uses degrees or angular units. Persistent inputs/state are binary32. `b32(x)` is
the nearest binary32 value (ties to even). Finite inputs only; positive radius, mass, axle lateral
reference load and angle scale; nonnegative loads/grip. Wheel numbering is 0/1
front, 2/3 rear. Body x is forward, y lateral, z up, and positive planar yaw rotates
forward toward lateral. Ordinary mode excludes drift/drag variants, nitrous,
wheel damage steering perturbation, nonzero handbrake and special alternate
steering selection. Pedal exclusivity and throttle shaping retain Stages1/3 order.

The simulation-facing steer command is an unsigned 16-bit turn: 65536 units per
turn. Positive steer rotates the wheel heading toward positive body y; in a
symmetric straight forward-rolling fixture its front tyre contribution has
positive yaw moment. Keyboard and analog input mapping are distinct.
A caller may enter at this command boundary, or at the bounded input-filter
boundary S4-2 with explicitly supplied history averages. These two boundaries
must not be applied twice. No wheelbase-based direct yaw-rate assignment or
additional lateral velocity decay is part of the established tyre contribution.

Stage1's five iterative passes continue to change longitudinal gripping forces
only; lateral forces computed here remain fixed during those passes. The common
friction cap then acts on both components. Stage3 brake preparation/lock uses the
same contact velocity and pre-decision slip snapshot. Stage2 body force clearing,
aero, stabilization, imposed limits and integration remain in force.

For host composition, replace the supplied gripping lateral-force input of
S1-7.4 with S4-5, including wT exactly once. S4-6 refines the existing S1-7.6
cap and angle correction; execute that combined operation once. Replace any
host steering-to-yaw assignment or lateral damping used to stand in for this
tyre response with the force/moment contribution here. Preserve separately
specified Stage2 stabilization/damping and external chassis torques. Use the
same command, contact snapshot, load, grip, lock state and prepared forces
across all stages. S4-6 body accumulation happens after the passes, once per
force preparation; evaluating provisional pass accelerations does not add
another persistent body force. The next derivative consumes the prepared
result under the S2-7/S3-7 lifecycle.

## S4-1 Data and numeric curves

Offsets are relative to the 2,192-byte source car row, little-endian binary32.
The effective record begins at row +0x110. Stock ordinary composition copies
these values; active upgrades/tuning/other modes can change them and are excluded.

| Row offset | Meaning |
| ---: | --- |
| +0x120 + i*0x30, three floats | Wheel mount x,y,z, i=0…3 |
| +0x1E0 / +0x200 | Front/rear lateral angle scale A |
| +0x1E4 / +0x204 | Front/rear lateral reference load D |
| +0x1E8 / +0x208 | Front/rear lateral output factor K |
| +0x1EC / +0x20C | Front/rear peak friction coefficient (Stage1) |
| +0x1F0 / +0x210 | Front/rear sliding coefficient (Stage1) |
| +0x224,+0x228,+0x22C | Body dimensions dx,dy,dz |
| +0x380 | Steering gain g |

A has a deliberate dual numeric use: the lateral curve uses angle converted to
degrees divided by A; the sliding decision compares a radian angle directly to A.
Do not insert a degrees-to-radians conversion into that comparison. For example,
a supplied stock A=12 cannot be exceeded by the folded slip angles in S4-5.
D and K are effective calibration inputs; their physical interpretation does not
license additional gravity, mass or unit conversions.

The stock body yaw inertia before the existing body-mode adjustments is
`Iz=m*(dx*dx+dy*dy)*b32(1/12)`; inverse inertia, centre offset and adjusted contact
arms may instead be supplied at the established Stage1 body boundary. The centre
and suspension/contact/load producers are Stage5. Do not substitute wheelbase for
dx or track for dy. Full mode-dependent yaw response remains U4-5.

For a uniform scalar curve `C(x;a,k,[y0…yn])`, use p=(x-a)*k, clamp to endpoint
samples outside the sample range, and linearly interpolate adjacent samples
inside. The decimal table entries below denote binary32 constants. They are
numeric compatibility parameters, not original identifiers or copied assets.

| Curve | a | k | Samples |
| --- | ---: | ---: | --- |
| Keyboard input I | -1 | 10 | -1,-.712,-.453,-.303,-.216,-.148,-.116,-.080,-.061,-.034,0,.034,.061,.080,.116,.148,.216,.303,.453,.712,1 |
| Analog input J | -1 | 10 | -1,-.800,-.615,-.483,-.388,-.288,-.220,-.161,-.111,-.057,0,.057,.111,.161,.220,.288,.388,.483,.615,.800,1 |
| Keyboard speed limit Lk | 0 | .05624999850988388 | 40,20,10,5,3.5,3.1,2.9,2.9,2.9,2.9 |
| Analog speed limit La | 0 | .05624999850988388 | 45,15,11,8,7,7,7,7,7,7 |
| Speed response R | 0 | .05624999850988388 | 1,.92,.78,.64,.54,.48,.42,.38,.34,.32 |
| Input-history limit H | 0 | 5 | 1,1,1.1,1.2,1.25,1.35 |
| Input-history rate T | 0 | 5 | 1,1.05,1.1,1.2,1.3,1.4 |
| Input-change rate E | 0 | .5 | 1,1.05,1.1,1.5,2.2,3.1 |
| Countersteer factor C | 0 | 4 | .5,.6,.7,.8,.95 |

The speed-curve abscissa `vctl` is the previously exported signed forward
velocity `V dot body_forward` in m/s, with no mph conversion. It is held at the
frame input boundary rather than recomputed for each substep. Reverse values
use the first endpoint of each speed curve. The magnitude |V| is a distinct
input used by the slip latch. At an arbitrary restored-state boundary supply
the exported snapshot explicitly; initial publication/caller cadence is U4-1.

## S4-2 Bounded human input filter

Inputs: raw signed axis r, frame duration dt>0, vctl, pedals t,b,hb from the
simulation-facing pedal snapshot, gain g, signed body slip sigma in degrees,
body speed magnitude v in m/s, old signed command angle d in degrees, previous
limit ell, slip latch z, previous mapped input jold, input-history average h,
and updated rate-history average e. h is the average before appending the
current mapped input; e includes the current input-change-rate sample.
S4-2 covers collision-recovery inactive, ordinary steering selection, analog
range 1 and analog speed limiting enabled. Other device settings and collision
recovery remain U4-2. hb=0 in ordinary fixtures. Pedals are in [0,1],
g>0, h is in [-1,1], and supplied e is nonnegative. These are effective
filter inputs; their unknown producers are not inferred from those bounds.

For keyboard, j=I(r). For analog, j=J(clamp(r,-1,1)). Define
`P=1-(t+1-(b+hb)*.5)*.5` and
`ell0=Lk(vctl)*(1+R(vctl)*P)*H(abs(h))` for keyboard,
`ell0=La(vctl)*(1+R(vctl)*P)*H(abs(h))` for analog.

Update z using sigma and v: if incoming z=true, clear it if abs(sigma)<5 or
v<0; otherwise retain it. If incoming z=false, set it only if abs(sigma)>15
and v>=b32(13.410818099975586). The v<0 clear condition cannot occur for a
valid magnitude; retain the actual zero threshold rather than inventing a
low-speed reset. The producer of sigma returns zero when signed forward speed
is <1 m/s; above that it is body heading minus world velocity heading wrapped
to (-180,180] with the angular quantizer of S4-4. Thus reverse motion enters
this producer's zero-slip case, regardless of |V|.

If j*sigma<0 and (z=true or abs(sigma)>ell0), let ell be
`C(abs(j))*min(abs(sigma),45)` for keyboard, or `min(abs(sigma),45)` for analog.
Otherwise let ell=ell0, except that abs(h)>=.5 imposes ell>=ell_previous.
Keyboard additionally caps ell at 45 and stores it as the next previous limit.
Analog does not update the stored previous limit and has no general ell cap
before gain/input multiplication. Both clamp the target `ell*g*j` to [-45,45].

Keyboard rate in degrees/s is `180*e*R(vctl)*T(abs(h))`.
The producer's current rate sample is `E(abs((j-jold)/dt))`.
Approach the target from d by at most `rate*g*dt` degrees, then quantize below.
Analog sets the target angle directly; it has no keyboard rate limit. Store j
as previous mapped input and append j to the input history after calculating
the angle. The two history averages are explicit supplied state inputs here,
not a license to use an arbitrary exponential smoothing constant. Their windows
are 0.55 s (input) and 0.15 s (rate), but complete sampling/expiry/reset behavior
is U4-2 and is not established by those durations alone.

For the resulting degree angle a, compute an integer truncation toward zero of
`a*65536`, divide that integer by 360 truncating toward zero, then retain its
low 16 bits. Do not round the command to the nearest unit. For the old signed
angle, decode unsigned u as u*360/65536 degrees and subtract 360 when >=180.
The filter initializes command=0, previous mapped input=0, slip latch=false,
previous limit=45, analog range=1, analog bypass=false; histories still require
the explicit initialization producer. Duration zero is outside this filter
boundary; do not infer a division-by-zero input policy.

## S4-3 Front-wheel geometry

Let L=mount0.x-mount3.x and W=mount0.y-mount1.y, both positive. Convert command
u to theta=`b32(u*b32(0.00009587380191078410))` radians; subtract twice
`b32(3.141592741012573)` when theta exceeds that pi value. Let a=abs(theta)
and c=L*a/(L+a*W). For theta>=0 the front angles are (c,a); for theta<0 they
are (-a,-c). Rear steer angles are zero. This is the evidenced rational mapping;
there is no tangent/atan Ackermann substitution. Exact zero gives both zero.
In exact arithmetic mirror commands exchange the front angles and negate
them; binary32 command conversion can introduce the small asymmetry shown
in the geometry vectors.

For each wheel angle delta, local heading h=(cos(delta),sin(delta),0), lateral
l=(-sin(delta),cos(delta),0), up=(0,0,1). The original trigonometric primitive
quantizes its input before an approximation; exact reproduction is U4-3.
A standard sine/cosine adapter is accepted only within the numeric vector
tolerances and cannot claim bit-exact retail matrices.

The command is transferred once per frame before substep integration; the
steering frames are held across the frame. Contact/world-frame refresh occurs
at the existing post-step; changing the command does not retroactively change
already prepared forces used by the first derivative.

## S4-4 Contact velocities and angular heading

Given body orientation B, linear velocity V, world angular velocity Omega and
body-local wheel mount arm r after centre/suspension offsets, the sampled wheel
centre velocity is `U=V+Omega cross (B*r)`. There is no extra vertical suspension
travel-rate term in this sample. With world wheel heading h and lateral l,
`vx=h dot U`, `vy=l dot U`. Translation of the wheel frame is excluded from this
velocity transform. On a level body with yaw rate w and mount (x,y), U in body
coordinates is `(Vx-w*y,Vy+w*x,Vz)`. Mount arms and the contact application arms
are distinct supplied geometry; do not use wheel radius as the yaw moment arm.

The planar heading Q(x,y) is in unsigned turn units. Q(0,0)=0; axis values are
0,16384,32768,49152; equal positive magnitudes give a 45-degree octant boundary.
For unequal absolute magnitudes let n=trunc(256*min(abs(x),abs(y))/max(abs(x),abs(y)))
(n=0…255). Define
`q(n)=nearest_integer(atan(n/256)*1024/pi)+epsilon(n)`, using
mathematical atan and pi in this numeric characterization.
Epsilon is +1 at {5,9,20,24,44,48,57,62,67,85,92,101,113}; -1 at
{154,166,193,198,203,223,230,243,246}; zero otherwise. This compact mathematical
encoding specifies the observed static numeric input/output mapping, including
22 required one-unit deviations from the analytic approximation. A one-unit
deviation changes the heading by 32 turn units. The formula and sparse
corrections are compatibility facts; no original table or algorithm is supplied.
The analyst checked the relationship at all 256 indices; this is static data
inspection, not a runtime observation. There are no half-integer ties in this range. The first-quadrant angle is 32*q(n) if abs(x)>abs(y),
otherwise 16384-32*q(n). Reflect it through the actual x/y signs into [0,65535].
All component comparisons use the supplied binary32 x,y. Exact x87 ratio/index
boundary rounding remains U4-3.

## S4-5 Folded wheel slip and lateral force

From Q(vx,vy), form a=Q*b32(0.00009587380191078410) and subtract 2*pi when a>pi,
where pi=b32(3.141592741012573). If vx<0, add pi, then again subtract 2*pi
when a>pi. This folded radian angle is alpha. It is zero for both pure forward
and pure reverse and positive for vx>0,vy>0. For vx<0,vy>0 it is negative.
For vx=0 use the ordinary heading result; there is no zero-speed atan division.
At (0,0), alpha=0.

Use s=Rwheel*omega-vx and Stage3's sampling/lock semantics. Sliding is chosen
only when both sqrt(s*s+vy*vy)>0 and (vx!=0 or vy!=0), and when abs(s)>.5,
abs(alpha)>A, or newly locked. Equality at .5 or A does not trigger sliding.
Outside that predicate take gripping even for a spinning wheel at rest.

For gripping define z=6*abs(alpha)*b32(57.2957763671875)/A and x=b32(.8)*N/D.
Let seven load curves G0…G6 have x=0,2,4,6,8,10 with endpoint-clamped linear
interpolation, and samples:

| Curve | Samples |
| --- | --- |
| G0 | 0,0,0,0,0,0 |
| G1 | 0,1.2,2.3,3,3,2.8 |
| G2 | 0,1.7,3.2,4.3,5.1,5.2 |
| G3 | 0,1.8,3.5,4.9,5.8,6.1 |
| G4 | 0,1.83,3.6,5,5.96,6.4 |
| G5 | 0,1.86,3.7,5.1,6.13,6.7 |
| G6 | 0,1.9,3.8,5.2,6.3,7.1 |

For z>=6 use G6(x). Otherwise linearly interpolate Gfloor(z)(x) and
Gfloor(z)+1(x) by the fractional part of z. Multiply by K*2.5 and the supplied
per-wheel lateral multiplier wT. Its sign is negative when exactly one
of alpha>0 and vx<=0 holds, and positive otherwise.
This result is Fy; Fx is Stage1's wL*tau/Rwheel. At alpha=0 the magnitude is
zero. Do not additionally multiply the gripping lateral curve by surface grip,
axle sliding coefficient or complete frame grip: those inputs act in sliding
or at the final cap. Above the final angle/load knots this gripping curve
clamps; it is not an extrapolated linear cornering-stiffness model.

Sliding Fy and Fx are Stage1/3's normalized combined-slip vector, including
wT and wL and the low-forward-speed rule; N=0 yields zero sliding tyre force.
The low-speed normalization predicate uses the supplied signed forward speed,
not |V|. This refines Stage1's shorthand “body speed” for this particular rule.
Do not mirror it into a reverse-symmetric speed condition.

## S4-6 Combined force and body/yaw boundary

After the five gripping longitudinal passes, apply the existing Stage1 friction
limit to (Fx,Fy) together. In full notation the limit is
`N*surface_peak*axle_peak*frame_grip*response_scale*peak_scale`, including the
conditional player rear-speed multiplier of Stage1. `peak_scale` is an explicit
per-wheel input initialized to 1; other producers are U4-4. The sliding
coefficient has its Stage1 factors; the gripping curve is already prepared.

If tau>0 and not locked, test using 1.5*Fx. If the tested magnitude exceeds
both the limit and .001, scale both tested Fx and Fy by limit/magnitude and
retain the tested/scaled Fx (do not divide it by 1.5 after saturation). When
no saturation occurs, undo the temporary 1.5. Then, only when abs(vx)>1,
subtract `b32(.15)*sin(alpha)*Fy` from Fx. This correction occurs after the cap
and is not capped again. The resulting Fx is shared by the next spin derivative
and the body. At exact abs(vx)=1 there is no correction.

For the level planar domain the applied horizontal force is `Fwheel=Fx*h+Fy*l`.
Here h,l and the supplied contact arm are expressed in the same body basis;
rotate force and arm by B before accumulation in world coordinates. This
horizontal reduction leaves the existing normal-force application intact;
it neither adds another copy of N nor replaces vertical contact dynamics.
Add each force once to the body's existing force accumulator and add yaw torque
`rx*Fwheel.y-ry*Fwheel.x` using the supplied contact arm relative to centre.
Other aero/collision/stabilization torques remain separate supplied inputs;
steering does not directly set angular speed. The body acceleration and yaw
acceleration boundary is `(E+sum(Fwheel))/m` and
`Iz_inverse*(Mext+sum(moment))`. The same per-pass force/moment result supplies
Stage1 contact-heading acceleration. Euler update uses the established substep
h, not the outer frame dt. Existing Stage2 angular momentum/damping/limit
operations must still be composed; the equation here is the tyre contribution,
not a replacement for those operations.

For tilted/nonlevel contacts the force basis selector and surface normal are
required external inputs (U4-5); this stage normatively covers their common
level-plane reduction only. The final load N already includes any contact
alignment scaling. Full suspension, weight transfer, slope normals, contact
flag refresh and handbrake/rear-grip blend remain Stage5. N=0 removes the
lateral curve and the finite capped horizontal tyre contribution (apart from
Stage1's <=.001 cap bypass); airborne body aero and angular state persist.
An airborne test must explicitly set both N=0 and prepared tyre forces=0;
a contact flag alone does not zero a positive supplied load.

## S4-7 Unknowns / ambiguities

- **U4-1** Device-event axis normalization, caller-side input cadence and initial
  exported snapshot publication. During ordinary frames vctl is the previously
  exported signed forward speed; arbitrary saved-state entry supplies it.
- **U4-2** Complete timed history sampling/expiry/startup, device preferences,
  collision-recovery steering, alternate input mode and nonordinary mode mapping.
  Supply h/e at the bounded filter boundary or supply the resulting command.
- **U4-3** Exact x87 intermediate precision, trig approximation and marginal ratio
  index selection. Quantizer data relationship is checked statically at all n;
  numeric tolerances are not permission to alter strict branch comparisons.
- **U4-4** Complete frame grip, response scale, peak_scale and rear grip producers
  (including ordinary slip-dependent bonus and handbrake blend). Stage1 Q-11
  remains partial. Supply their complete effective values; 1 is a fixture only.
- **U4-5** Complete 3D body inertia/mode adjustment, force basis and extra yaw
  assistance torques, contact application arms, centre/load/surface producers.
  Planar tyre force/moment is covered; no full retail chassis response is claimed.
- **U4-6** No isolated retail runtime/differential validation, perceptual handling
  assessment or measured steering radius/yaw trace. No original executable launched.
- **U4-7** Alternative lateral fallback selector, upgrades/tuning and nonordinary
  steering/grip modes are excluded. The evidenced ordinary lateral selector is 1.

## S4-8 Evidence and acceptance

Analyst evidence IDs E4-1…E4-8 resolve in the analyst-only findings record:
stock data/parameters; device filter; steering geometry; contact velocity;
angle quantizer; lateral curve; combined cap; scheduling/body boundary.
`test-vectors/components.json` is normative for component examples with supplied
state, entry boundary and tolerances. Its README distinguishes derived values
from static data checks. S4-7 boundaries are required inputs, not guessed
retail defaults. No prior companion hash is superseded by this stage.
