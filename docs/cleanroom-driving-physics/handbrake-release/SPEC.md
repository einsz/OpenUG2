# Handbrake release, rest and launch boundary amendment

2026-10-16. Implementation requires the exact reviewer-approved hash recorded in REVIEW.md. Companions are the approved Stage1–5, body-response and body-coupling contracts. Their laws remain unchanged except that HR-2/3 closes a previously excluded rest boundary. This answers HB-Q1 conditionally and bounds HB-Q2/Q3 without resolving the reported bog; it does not establish retail 240SX or 350Z launch trajectories.

Every recovered claim is **inferred**, independently checked against the intake executable's instructions and numeric data. No original was executed. Vectors are synthetic arithmetic/state examples. Private evidence HR-E1…6 supplies traceability. Finite binary32 state, positive mass/radius/inertia and existing unit/precision conventions apply; strict predicates must not become tolerance comparisons.

## HR-1 Wheel lock, spin and release

The Stage3 lock rule is unchanged. Given service torque B, handbrake torque H, radius R, spin omega and moving-contact argument Q, let A=1.2*(B+H), C=.1*abs(omega)+R*Q. When evaluated, A>C snaps spin to zero and sets lock iff A>1; otherwise it clears lock and retains spin. The evaluator is reached only when both `hypot(R*omega-vx,vy)!=0` and `(vx!=0 or vy!=0)`. A failed call predicate preserves both incoming lock and spin. There is no extra pedal-release, throttle, gear-direction, clutch-mode or elapsed-time condition. (HR-E1.)

Consequently exact-rest release with incoming lock true retains that lock for either forward or reverse requests, including repeated throttle release/reapplication, **if all other state producers preserve exact zero contact velocity**. A nonzero forward, reverse or lateral contact velocity with nonzero slip/lateral norm permits reevaluation; with B=H=0 and finite nonnegative Q it clears lock. An exactly rolling straight wheel with zero slip also skips reevaluation. Handbraking at an exactly stationary contact does not itself establish a new lock through this evaluator. These component statements do not assert that original ordinary driving remains in any idealized zero-velocity manifold. (HR-E1.)

Keep the pre-evaluation slip cached for force preparation. Snapping omega during lock evaluation does not retroactively recalculate this slip. Incoming lock controls initial torque distribution and whether brake torque is added. Updated lock controls the subsequent five gripping corrections and later clutch torque distribution. Clearing lock during preparation does not rerun initial drive or insert previously skipped brake torque. In clutch Locked mode later correction passes can distribute torque to a newly unlocked wheel; in Engaging/Disengaging modes those five clutch corrections do not occur. The next preparation can distribute initial drive using the retained new lock. Engine reaction is produced even when wheel torque writes are skipped for locks. (HR-E1, HR-E4.)

A wheel's derivative still updates smoothed torque when locked. Lock zeros its torque/drag angular acceleration; the independent gripping slip correction can subsequently add `-slip/(R*h)` for h>.001. Thus 'lock implies zero complete spin derivative' is not generally valid. At exact rest with zero cached slip the additional term is zero. Post-step clears force/torque/slip accumulators, retains lock and smoothing history, and applies the established spin-direction latch. A spinning stationary-contact wheel still skips lock reevaluation. The established finishing synchronization may change driven spin under its own nonneutral, clutch-Locked, contact-count, throttle and positive-mean-slip gates; it has no added lock-release rule. (HR-E1, HR-E4.)

## HR-2 Conditional rest classification

Inputs at the rest boundary are current published body V and Omega, candidate body B, previous rest class, simulation-facing throttle t, current gear g (1=neutral), four lift inputs, four lift-activity states, an integer selector c and a separate integer d, reset-pending Boolean and touching-body Boolean. c is nonnegative and is the same owner selector used by BR-1; this amendment does not resolve its producer. d is independently supplied and must not be equated with c or contact count without evidence. The lift states below are actuator states, not brake locks, tyre slip states or spin-direction states. A supplied touching-body result is true when c>0 or an external collision contact reports touching. (HR-E2, HR-E3.)

Classify as moving (0) if any of these conditions holds:

- any lift input is strictly positive;
- any lift-activity state is 2 or 3;
- t>b32(.3), g!=1, c>0 and d>2, all together;
- |V|>b32(.7) or |Omega|>b32(.7);
- c>0 and either abs(B forward.z)>b32(.15) or abs(B lateral.z)>b32(.15);
- reset is pending;
- no touching body and previous rest class is not 2.

Otherwise classify as slowing (1) if |V|>.25 or |Omega|>.25; otherwise classify as fully resting (2). Equality at .7 and .15 does not exclude rest, and equality at .25 selects full rest. Neither handbrake nor service brake appears in these predicates. Forward and reverse nonneutral gear requests share the throttle exclusion. Neutral does not satisfy it. (HR-E2.)

The four lift-activity states initialize to 0. For their ordinary zero-input, zero-target, zero-offset fixture they remain 0. State 2 denotes an offset increasing toward a target; state 3 denotes an offset decreasing toward a target. State 1 denotes a positive reached target. Those states are distinct from the wheel lock and direction. Full lift impulses and target/offset producers remain outside this amendment. (HR-E3.)

## HR-3 Rest effects and publication timing

This is a supplied-input component contract. Host activation and ordinary producer mappings are unresolved; enabling this boundary does not establish a recovered ordinary-driving policy. Four wheels are present. The separate stuck/reset-request timer side path is excluded: its activation helpers, clock/tick wrap behavior, expiry and downstream callback effects require further specification. These outputs cover evaluations where that side path does not issue a request; do not interpret the omission as perpetual suppression or a lock/history clearing rule. For the covered state codes 0/1/2, restored gravity is b32(11) for state1 and b32(9.8) for state0/2. (HR-E2.)

For moving, preserve body momenta and wheel spins; enable the collision body's active flag and restore the existing state-dependent gravity parameter. Return 'no rest change'.

For slowing, multiply every component of body linear and angular momentum by b32(.95). Multiply all four wheel spins by b32(.95), irrespective of c. Enable the collision body's active flag, restore state-dependent gravity, and return 'rest change'. This factor acts per rest evaluation, not as a per-second exponential and not only once per outer frame.

For fully resting, set body linear and angular momenta to exactly zero. When c>0 set all four wheel spins to exactly zero; when c<=0 retain wheel spins. Disable the collision body's active flag, set the body gravity parameter to zero, and return 'rest change'. None of these three branches writes wheel lock or stabilization history z. (HR-E2.)

Rest classification runs after the ordinary post-step sweep and before new controller-force preparation, once per integration completion. When rest reports a change, a second post-step sweep runs with duration zero before controller-force preparation. The sweep republishes body V/Omega from the modified momenta, refreshes contacts as established, and clears/reseeds accumulators through normal post-step behavior. It is not another Euler integration. Thus slowing/full rest effects reach the contact-velocity and force preparation in the same completion; their newly prepared forces feed the following derivative. The existing bump-stop path can independently request this sweep. Do not run classification recursively inside the zero-duration sweep. (HR-E2, HR-E5.)

There is a **conditional reachable boundary state** consisting of locked wheels at exactly zero body velocity and spin: a moving handbraking preparation can set locks, followed by a fully-resting classification satisfying HR-2 and HR-3. The zero-duration sweep does not clear those locks. This is a statically established composition path under supplied predicates, not a measured stopping trajectory. Continuing throttle can prevent the rest snap only when all four throttle-exclusion predicates hold. Rest exclusion alone changes neither lock nor drive torque. (HR-E1, HR-E2, HR-E5.)

## HR-4 Long wait, stabilization and launch controls

The existing S2-6 z derivative is evaluated with the body derivative even while full rest repeatedly snaps body momenta to zero. The rest pass and its zero-duration sweep do not reset z, upper-clamp z, or integrate z a second time. A no-pedal stationary wait and an equally stationary handbrake wait with identical V/Omega/time histories therefore produce the same z evolution. z can continue evolving even when its damping gate is disabled. Merely releasing handbrake, changing gear, releasing/reapplying throttle or classifying as moving does not reset z in the bounded paths. (HR-E2, HR-E6.)

New body construction initializes z=0. Importing a pose, momenta derived from supplied velocities, and the shared wheel/engine/gear/clutch state into an existing body retains that body's z. These are distinct initialization boundaries. Neither is sufficient evidence for the complete retail vehicle-reset dispatch. The only established post-step z clamp is `z=max(z,0)`; no rest/pedal-specific upper clamp was recovered. (HR-E6.)

The BR-2 gate remains exact: frame state0/2 enables stabilization and state1 disables it; for race=4, throttle>.3 or all four exported contacts plus abs(exported forward.z)>.15 disables stabilization for that frame. For race!=4 throttle alone does not disable it. This uses the input snapshot at mode refresh, before later pedal redistribution. This rule is already approved, is not a new tuning proposal, and is not evidence that a particular host's race selector should be 4. Rest classification and stabilization gating are separate operations. Their throttle threshold equality does not change either strict predicate. (HR-E6.)

For an isolated body derivative with S2 threshold mutation excluded, longitudinal `Pdot=F-8*z*P` when enabled, otherwise F. High accumulated z can oppose launch even with wheel locks false, no brake torque and positive prepared F. The damping vanishes at P=0; at positive P it must be accounted for independently of the published force-response channel, which omits this term. A positive prepared force does not by itself prove positive net acceleration. The same fact holds symmetrically for negative P. No new engine, clutch, first-gear finishing coefficient or rest-conditioned torque multiplier was established. Existing Stage1 first-gear finishing remains independent of whether the synchronization gates succeed. (HR-E4, HR-E6.)

## HR-5 Questions and test vectors

`test-vectors/release.json` supplies conditional state transitions, numeric body-derivative examples and timing assertions; README states provenance and scope. They cover moving forward/reverse release, zero/nonzero boundary, spinning while stationary, held/released throttle and clutch correction eligibility. They are sufficient to reject pedal-only unlock, rest-as-unlock, unconditionally frozen z, and a throttle-only universal stabilization disable.

For HB-Q1: rest provides a conditional exact-zero producer retaining locks. The established moving-contact reevaluation clears locks after released brakes. No unconditional exact-rest recovery writer was found in the bounded original paths. Ordinary full-vehicle trajectory reachability and complete reset clearing remain unresolved.

For HB-Q2: the existing z law can preserve different long-wait versus fresh-launch histories and a known supplied race-state gate can change its application. The consulted original paths do not establish that long-handbrake stock 240SX should remain bogged, nor establish ten-second acceleration figures for either car. Original trajectories cannot be inferred from host observations. A sequence of wait, handbrake, release/forward, throttle release/reapplication, reverse and reset must record at each derivative/preparation boundary at least t,g,clutch mode/engagement, c,d,race,state,rest class, V/Omega,P/L,z,stabilization gate,F/Pdot,locks,spin,slip,load,drive torque and engine reaction. Compare equal-duration zero-pedal/no-handbrake controls. Reset must be a distinct externally supplied control until its clearing semantics are established. No numerical recovery-time acceptance criterion is claimed.

For HB-Q3: fresh static evidence agrees with S2-6's argument, comparison, rise/decay and negative-only post-step clamp. The known frame gating and creation-versus-existing-state-import distinction above are established; the ordinary retail selector producer and full reset are still open. Host wait/bog/reset observations alone cannot establish a missing original local formula or authorize an extra history clear. This amendment is not claimed to resolve that bog.

## Unknowns/Ambiguities

- Retail producers/reachability for c,d,race,state and collision touching/pending-reset inputs; do not conflate those values with frame contact exports.
- Stuck/reset-request timer activation, expiry, clock wrapping and callback effects are excluded, including whether they alter later reachability.
- Complete reset/recreation state clearing, lock initialization, and all lock writers outside the bounded paths. No assertion that reset clears z or locks follows from this amendment.
- Whether exact stationary contact persists in a retail level-ground trajectory after handbrake release, including suspension/tilt/vertical coupling, collisions and other excluded producers.
- Original runtime 240SX/350Z launch behavior, long-wait comparison, measured engine/gear/clutch histories, and whether any additional producer outside the examined boundary changes them. No isolated snapshot VM was available; original runtime was not launched.
- Exact x87 intermediates, overflow/NaN/degenerate frames and other companion unknowns remain outside scope.
