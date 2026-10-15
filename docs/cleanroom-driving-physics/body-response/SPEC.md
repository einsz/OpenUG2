# Body and angular response amendment

2026-10-15. Implementation is authorized only for the exact file hash approved in REVIEW.md. Companions: the approved Stage1–5 SPEC.md files in the parent directory and stage2…stage5. They remain unchanged except for the explicit replacements below.

All original-behavior requirements are **inferred** from static evidence cross-checked against the identified executable. Parameters were read as numeric data. No original runtime was executed. Synthetic vectors check arithmetic and order, not original trajectories. Evidence BR-E1…8 resolves in the analyst-only register. The host-integration requirements below are independently specified integration contracts, not claims about an original host architecture.

## BR-0 Domain and symbols

This amendment closes the body/angular boundary for local ordinary driving with supplied effective data, selectors and world contacts. It replaces prescribed heave, pitch and roll with force-driven motion when the dynamic path is selected. It adds no tyre, engine, steering or brake law. The Stage1–5 laws and subdivision remain in force.

Use right-handed world coordinates with z up; body x forward, y lateral, z up. B has world-vector columns f,l,u, maps local vectors to world, and initially is a proper orthonormal rotation. Host coordinates require an explicit fixed conversion for vectors, pseudovectors, orientations and application arms. Units: metres, seconds, tonnes, kN, kN·m, rad/s; momentum has tonne·m/s and angular momentum tonne·m²/s. Distinguish centre position X from world linear momentum P (Stage5 names its position P).

Persistent body state is X, B, world P, world angular momentum L and Stage2 stabilization state z. Published V and Omega are separate held quantities; they are not independently Euler-integrated. F,T are prepared world force and world moment accumulators, including all established contributions once. A point arm r is a world displacement from the centre, not an absolute point. The geometric origin is X-B*c_body for effective local centre c_body (the vector named c in Stage5). The scalar selector c below is distinct from this centre vector. Existing Stage5 centre-change preservation applies.

All supplied values are finite; M>0, principal inertia Ix,Iy,Iz>0, yaw multiplier kI>0, h>=0. Persistent values and decimal parameters are binary32. b32 denotes nearest binary32, ties-to-even. Tolerance-based arithmetic excludes exact x87 intermediate equivalence. Strict predicates and the frame/substep boundaries are exact requirements. Normalization inputs must be nonzero and the first two candidate axes nonparallel; degeneracy, NaN and overflow are excluded. No arbitrary angle clamp is added.

The covered state codes are 0,1,2, with 0/2 the ordinary path. State1 equations are supplied for the established selector/gate transition only; its special centre, throttle and gravity producers remain companion/external boundaries. Remote movement, alternate race families, rest/stuck/reset, guide/lift torques, collision detection/response and full event reachability are outside scope. A caller supplies their contributions at declared boundaries or excludes their domains. A supplied zero contribution is an explicit independent fixture, never an inferred retail default.

## BR-1 Inputs and selected inertia

Use Stage5's basic inertia diagonal I0 and yaw multiplier kI from the effective source record. Define I1=diag(Ix,Iy,Iz*kI). Retain the current selection s, initially adjusted (s=1) at body construction. Store L in world axes regardless of s.

Each frame supplies these separate inputs:

| Input | Contract |
| --- | --- |
| state | Established numeric vehicle-state code 0,1,2 |
| c | Nonnegative integer inertia selector; only zero/nonzero matters to inertia selection; a separate comparison c<2 is used in BR-5 |
| p | Stage5 numeric driver-role selector, separate from c; p=0 versus p!=0 selects torque-limit data |
| race | Exceptional race-state code; only equality to 4 matters here |
| throttle | Shaped simulation-facing throttle at mode refresh, before later shared pedal redistribution |
| contacts, forward snapshot, v, sigma | Previously exported four contact flags/count, f, signed forward speed and Stage4 quantized wrapped heading difference |
| K0,K1 | Nonnegative effective torque limits from source-row offsets +0x398,+0x39C, binary32 kN·m |

The c producer is unresolved. It is not licensed to equate c with wheel contacts, p, or the exported contact count. Its zero/nonzero and less-than-two predicates must be supported explicitly. The p producer remains the companion's supplied driver-role boundary. Supplying values makes the component behavior defined; it does not prove a retail selector mapping. Effective data composition and car preset selection remain external.

At frame mode refresh, desired s is 1 iff state is 0 or 2 and c!=0; otherwise desired s=0, including state0/2 with c=0 and every covered state1 input. If selection changes from 1 to 0, replace **world** L.z with L.z/kI, retaining L.x,L.y. On 0→1 retain all L. Staying in either mode retains all L. Then publish the new selection. This is asymmetric and is not a rotation into the body's yaw axis. No extra momentum correction occurs merely because effective parameters refresh without a selection transition.

Whenever the angular velocity is republished, use the selected I_s:

`Omega_raw = B * inverse(I_s) * transpose(B) * L`.

At the post-Euler boundary B need not yet be orthonormal. Use transpose(B) in this formula as written; do not substitute a numerical matrix inverse or repair B first.

At an explicit pose/velocity initialization boundary, the established momentum seed is P=M*V and L=B*I0*transpose(B)*Omega_input using the **basic** diagonal, irrespective of held s. Held V and Omega become the supplied velocities. This does not establish full reset behavior. An independently initialized host ordinary start may use zero V/L/Omega and an explicitly supplied valid pose. A restored-state entry supplies X,B,P,L,z,V,Omega,s and prepared F,T directly, rather than reconstructing them from controls.

## BR-2 Angular damping and gates

Frame state0/2 initially enables both `curve` and `stabilize`; state1 disables both. During the same refresh, when race=4, disable stabilize if either throttle>b32(.3), or all four previously exported contacts are present and abs(previously exported f.z)>b32(.15). Equality does not disable. This exceptional rule does not disable curve. When race!=4 neither exceptional predicate affects stabilize. Gates are held through that frame's substeps. The companion translational -8*z*P term uses this same stabilize gate.

Let w=|held Omega|. D(w) is endpoint-clamped piecewise-linear interpolation of the following (rad/s,1/s) knots, with decimal values interpreted as binary32:

`(0,.05),(2,.05),(4,.25),(6,.7),(8,2),(10,5)`.

After raw prepared T is available, before BR-3:

`Ldot = T - (curve ? D(w)*L : 0) - (stabilize ? 2*z*L : 0)`.

Use current world L and pre-Euler z. Damping is not a torque term added again during preparation. Both terms act on every world component and are independent of road speed, gear and body axes. Existing Stage2 z derivative and post-step negative clamp remain unchanged; its angular-speed input is this same held w. Do not replace D by a constant yaw-only drag or use the just-integrated z.

## BR-3 Conditional world-vertical restriction

At frame refresh for local movement, start K=K0 if p=0, else K=K1. Only when p!=0 and K1 is exactly zero, use sigma=0 when previous exported v<1 m/s, otherwise the established wrapped body-heading-minus-velocity-heading angle. If sigma*L.z<0 and abs(sigma)>b32(.1), set K=20. Other cases retain zero. The product uses world L.z **after** the inertia-selection transition in BR-1. The heading calculation is Stage4's existing quantized law; do not substitute unquantized atan2 in a strict boundary test. K is then held through substeps. No low-speed fallback applies to a supplied nonzero K1 or K0.

At each derivative evaluation apply this rule **after all BR-2 damping**, using raw prepared T.z:

- K!=0 and T.z>K: replace Ldot.z by K.
- K!=0 and T.z<-K: replace Ldot.z by -K.
- Otherwise retain the damped derivative unchanged.

Do not clamp the already damped derivative to [-K,K]. Equality retains damping. Ldot.x,y are unaffected. K is not multiplied by inertia, mass or dt here; Euler later multiplies the derivative by h. No body-axis transform is made.

## BR-4 Derivatives, publication, and final orientation repair

Preserve companion derivative collection before **any** Euler update. The body derivatives are:

`Xdot = held V`, `fdot = held Omega cross f`, `ldot = held Omega cross l`, `udot = held Omega cross u`.

Pdot is the Stage2 force-threshold result plus its gated stabilization, and Ldot is BR-2/3. Stage2's force-threshold operation can change the stored F before publication. z follows Stage2. Euler updates each persistent state by its collected derivative times h. Thus a fresh torque changes L during this substep, but its newly published Omega first changes B through the next derivative. Likewise a fresh force changes P before that velocity changes X on the next derivative. Do not use semi-implicit velocities, quaternion exponential integration, or direct Euler-angle increments as tolerance-equivalent alternatives to these specified steps.

The body post-step, before subsequent arm/contact/coupling work, performs these observable operations:

1. Derive V=P/M and apply Stage2's 200 m/s/mass rule, rewriting P and V only where that rule requires it.
2. Publish force-response vector `Aforce=b32(.10204081237316132)*F/M`. This is an established force-response channel, not the net finite-difference acceleration: it omits the separately applied stabilization contribution. Preserve the existing consumer meaning rather than using it to integrate X a second time.
3. Compute Omega_raw using current Euler B and L as BR-1 specifies; publish Omega_raw when its magnitude<=250 rad/s, otherwise publish `Omega=250*Omega_raw/|Omega_raw|`. This magnitude cap does **not** rewrite L. Do not cap individual components.
4. Clamp negative z to zero; save current F and T in the consumed-force outputs, clear F,T, and seed world gravity `(0,0,-M*g)` when the supplied current gravity g>0. At nonpositive g seed none. Gravity has zero arm and no moment. Ordinary frame state0/2 assigns g=b32(9.8); excluded special/rest writers require explicit inputs.

Preserve the complete Stage5 post-step, optional zero-duration sweep, aero/contact/coupling preparation and late torque ordering. These operations use the current Euler orientation and the held velocities just published. At this interior composed boundary, evaluate companion vector/cross-product formulas with that candidate B, superseding their isolated-component unit-basis assumption for this boundary only. Insert no extra normalization beyond operations explicitly required by the selected formula; in particular retain Stage5's written cross-product alignment formula rather than its identity valid only for unit axes. A zero-duration sweep may republish velocities and clear/reseed forces as established in Stage5; it is not an extra Euler update. A direct impulse after publication alters momenta immediately; held velocities change at the next body publication, including an explicitly requested zero-duration sweep.

Only after the completed integration/post-step/preparation work for each substep, repair the candidate axes a,b,d as follows:

`f=normalize(a)`, `u=normalize(f cross b)`, `l=u cross f`; publish `B=[f l u]`.

Discard d in this repair. This anchors the forward axis, preserves handedness and is not three independent normalizations or a polar-decomposition fit. The normalizing vector length is nonzero in the covered domain. Do not republish Omega or repeat force preparation after this repair merely to make B and Omega algebraically consistent: the original boundary holds the earlier published Omega. The next derivative sees repaired B and held Omega. Frame export publishes repaired B, X, held V/Omega, origin X-B*c_body and companion contact/load/speed outputs after all substeps. Body/world velocities cannot be replaced by visual pitch/roll/heave channels.

## BR-5 Late opposing angular torque

This bounded additional angular law reduces a previously explicit unknown. After once-only wheel point-force accumulation, use the current held Omega and c. For state1 or state2 it contributes zero. For state0 it contributes zero when c>=2 or w>=b32(.5). Otherwise add to prepared T:

`Topp = -M*H(w)*Omega`,

where H is endpoint-clamped piecewise-linear interpolation of `(0,0),(b32(.4),0),(2,20)`. The scale multiplying Omega is stored as binary32. This is a world angular torque; no linear force or point-arm moment is added. The strict speed gate cuts off this contribution at .5 even though the interpolation table extends to 2. It is prepared for the following derivative, not inserted into the already collected derivative. It remains separate from BR-2 momentum damping. The full air/rest/collision and selector production remain excluded despite this resolved torque contribution.

## BR-6 Point forces and impulses

This paragraph explicitly supersedes any Stage2-3 implication that capped force is used for the point-force moment. Define limit_length(q,C) as q when |q|<=C (including q=0), otherwise C*q/|q|. For an independent, nonaliased requested world force Q at world arm r, add `limit_length(Q,1200)` to F but add `limit_length(r cross Q,100)` to T. Moment uses the **requested** force, before its linear cap. Caps are per application, strict when magnitude exceeds the limit, and never caps on accumulated F/T. The Stage5 wheel application, Stage2 aero and gravity retain their established counts/sites. A direct supplied angular torque adds to T without inventing a point force or these point-force caps.

For a requested world impulse J at world arm r, let Jc=limit_length(J,1000), then dL=limit_length(r cross Jc,10). Add Jc to P and dL to L. The optional separately selected angular-scale variant instead adds a*dL to L **after** the angular cap; a=0 removes only angular response. These limits have units kN·s and kN·m·s, different from force/moment limits. There is no per-dt conversion at this interface. Preserve application order for multiple impulses; caps do not apply to total momentum. Normal bottoming uses the unscaled variant, Stage5's requested impulse and suppression/zero-duration-sweep rules. The global suppression input must be explicit. Generic collision impulse production and a's producer remain external.

## BR-7 Coupled suspension and independent host integration

Selecting this dynamic path requires these behavior contracts:

- The solver's centre X and full B drive Stage5 world wheel query points, arms, contact velocities, travel and travel rate. Use held V/Omega at their declared evaluation boundary. Current Stage5 loads feed existing tyre/brake/cap kernels; their resulting forces and moments feed F/T once.
- Every component of P,L,X,B participates. Vertical tyre/aero/gravity forces drive heave, and wheel/aero moments drive pitch and roll as well as yaw. Prepared forces consumed at the next derivative may come from the preceding frame; a changed pedal or surface does not retroactively replace them.
- The first ordinary derivative must have valid preceding preparation. Creation's established h=0 preparation is retained; starting with empty F/T and claiming equivalent first-step acceleration is invalid. Restored-state entry supplies prepared F/T explicitly.
- No prescribed road-following heave, target body pitch/roll, static quarter-weight override, algebraic axle-load transfer or direct yaw assignment may overwrite these dynamic states during covered ordinary motion. Visual body/wheel transforms may consume the dynamic pose; an explicitly separate cosmetic transform must not feed suspension geometry. Host world contact query remains a declared independent service with consistent units and normals.
- Unknown c/p/race/grip/material producers must appear as supplied inputs or labelled independent host policy. A host may choose a reversible independent policy for those producers, timestep entry, collision service or initial pose to make the path playable, but must document its chosen values/rules and cannot call them recovered retail behavior. Choosing c based on host contacts is allowed only as such an independent policy, not a claim established by this specification.
- Excluded rest/guide/lift/collision functionality may remain separately integrated with declared momentum/force entry points. It must not silently replace ordinary dynamic pose. A prescribed-pose fixture path may remain for component tests, clearly distinct from dynamic ordinary driving.

Acceptance must exercise the dynamic host boundary, in addition to component vectors: (1) an initially level, nonzero forward-speed braking case with below-centre force yields the correct signed pitch momentum and subsequently changes f.z; (2) lateral below-centre force changes roll momentum and subsequently lateral/up axes; (3) a vertical force imbalance changes P.z and subsequently X.z; (4) those pose changes reach Stage5 contact geometry/travel/rate and change subsequent loads/forces; (5) removing the excitation does not retain a prescribed pose but evolves through momentum and the specified damping. For isolated sign checks, start with B=identity and L=Omega=0 and zero other moments: braking Q=(-4,0,0) at r=(0,0,-.5) prepares positive T.y=2, hence positive L.y and subsequently negative f.z; lateral Q=(0,4,0) at the same arm prepares positive T.x=2, hence positive L.x and subsequently positive l.z and negative u.y. These numerical component cases also appear in the vectors; the host check must propagate its wheel-produced excitation through suspension feedback. Use controlled world queries and explicitly stated selector/grip inputs; no asserted retail dive/roll angle, trajectory or tuning target is implied. Record at least h, X,B,P,L,V,Omega,z,s,F,T, current/previous contacts, wheel travel/rate and aligned load at the derivative and preparation boundaries. Force-to-momentum and momentum-to-pose latency must be visible.

## BR-8 Validation and unknowns

Normative synthetic inputs/expected outputs are in test-vectors/body-response.json; README gives domains and tolerances. They distinguish inertia transition axes, damping gates, raw-torque replacement, publication/repair order, caps and force latency. The top-level provenance declaration applies individually to every vector; each vector also repeats its derivation source. All companion vectors must remain valid within their prior component entry boundaries, except the explicit extreme point-force correction. Full feedback acceptance in BR-7 is an independent integration check, not a fabricated original trajectory.

Open unknowns are: complete c and p producers and reachability; other state codes; exact runtime frame clock; complete reset/rest/stuck/collision/guide/lift behavior and effective special gravity/centre writers; upstream effective car/upgrade/tuning/opening-preset configuration; material-to-surface identity and complete grip/event producers; exact x87 intermediate/trig equivalence and degenerate orientation repair behavior. Existing companion unknowns outside the explicit replacements remain. No original execution, differential trace, measured steering/braking/landing response or complete opening-350Z trajectory establishes numerical handling parity. These unknowns must not be filled by undocumented tuning.
