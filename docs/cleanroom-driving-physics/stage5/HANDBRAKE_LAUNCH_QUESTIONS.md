# Handbrake release and launch questions

Status: open, 2026-10-16. This document is an implementation question, not an
approved extension of the behavioral contract.

## HB-Q1: stationary retained wheel locks

The approved S3-4 decision predicate preserves incoming lock state when both
contact-frame velocity components are exactly zero. S5-6 explicitly preserves
this rule after handbrake release. S1-7.2 skips drive torque on a locked wheel;
S3-4/S3-5 also skip its gripping correction and torque/drag angular acceleration.

In an invented level-ground, pure rear-wheel-drive fixture with zero body and
wheel speeds, rear locks supplied true, zero service brake and zero handbrake,
holding full forward throttle for ten seconds leaves both rear locks true,
rear drive torque and longitudinal force zero, engine speed at idle and vehicle
speed zero. This is a derived contract consequence, not an observation of the
original program or proof that ordinary handbraking reaches that state.

1. Can ordinary nonzero-handbrake stopping reach exactly zero contact velocity
   and wheel spin while retaining driven-wheel locks? Does a rest/stuck/reset
   state, input handoff, or another lifecycle producer make this state reachable?
2. If it is reachable, which observable condition permits a subsequent forward
   or reverse launch after service and handbrake pedals are released? Is there
   another state transition that changes lock state, or is retained locking at
   rest intentional until external motion occurs?
3. Please supply release/launch vectors covering exact rest, a nonzero velocity
   approaching rest, held versus released throttle during handbraking, and both
   forward and reverse requests. Distinguish a component predicate from the
   complete lifecycle, and leave unestablished behavior explicitly unknown.

No unconditional pedal-release unlock, timer, engine/clutch/grip retuning or
alternative rest-state producer has been implemented from this question.

## HB-Q2: sustained low-speed launch after a long handbrake hold

A separate independent host fixture uses the normal application's stock car
configuration loader, then supplies a level road surface, ordinary dynamic
body state, no collisions, straight steering and a fixed 60 Hz outer tick.
It waits two seconds at zero pedal, holds the handbrake for fifteen seconds
with zero throttle, releases the handbrake and holds full forward throttle
for ten seconds. The stock 240SX remains near idle, whereas the stock 350Z
accelerates beyond 5 m/s. These are observations of the independent host,
not original-program observations or a statement of intended retail behavior.

At the end of that 240SX launch, forward speed is approximately 0.37287 m/s,
engine speed 800 rpm, current and target gear first, clutch Locked with full
engagement, throttle 1, service and handbrake 0, all four contacts present and
both rear locks false. Prepared forward world force is approximately 4.2001 kN.
The failure therefore persists after handbrake torque and wheel locks clear.
Across fresh/driven, throttle held/released during handbraking, and
prescribed/dynamic body variants, seven of eight forward 240SX fixtures do
not reach 5 m/s in the ten-second observation window. All eight corresponding
350Z fixtures do reach that threshold.

1. Does the covered original stock 240SX launch after this long handbrake hold
   and after an equivalent long zero-pedal wait without handbraking? Please
   distinguish their rest-state transitions and subsequent launch behavior.
2. Which upstream ordinary state or coefficient producers, if any, change the
   existing low-speed stabilization, engine/clutch or first-gear finishing
   inputs during this sequence? Are the currently supplied ordinary settings
   sufficient to describe that launch, or is a documented producer missing?
3. Please supply a behavioral release/launch sequence that distinguishes
   persistent idle bogging, temporary clutch engagement and normal recovery,
   including throttle release/reapplication and a vehicle reset control.

The independent host's force/momentum attribution and equivalent no-handbrake
controls are still being measured. No physical law or stock tuning has been
changed on the basis of this observation.

## HB-Q3: low-speed history survives throttle release and resets with the car

The HB-Q2 stock 240SX fixture was extended while preserving the same vehicle
state: after ten seconds of the failed launch, release throttle for one second,
then reapply full throttle for five seconds. It remains slow, at approximately
0.50979 m/s and idle. Resetting the vehicle's transient state and launching for
ten seconds reaches approximately 29.3019 m/s. No stock tuning, road surface,
steering demand or collision input changes between these controls.

An equivalent seventeen-second zero-pedal wait with **no handbraking** also
produces the same approximately 0.37287 m/s launch after ten seconds, the same
failed release/reapply control and the same reset recovery. Thus the measured
independent-host behavior follows prolonged low-speed history rather than a
remaining handbrake pedal or wheel-lock state.

At the failed launch's ten-second boundary, the supplied ordinary dynamic
body has stabilization z approximately 1.14291, world forward momentum
approximately 0.45492 t m/s and prepared world forward force approximately
4.20013 kN. The specified instantaneous stabilization term -8*z*P is
approximately -4.15947 kN in that direction; it nearly balances the prepared
force. The fixture has no collision or external host velocity correction.
After throttle release/reapplication, z remains approximately 0.85920. After
the reset-and-launch control it is zero at the ten-second boundary.

1. Are S2-6's curve argument, interpolation, z derivative, initialization,
   ordinary reset/rest transitions and momentum-damping gate complete and
   correct for a prolonged stationary stock 240SX followed by full throttle?
2. Does an upstream race/state/throttle/contact producer change the ordinary
   stabilization gate or its history at launch? Please distinguish the
   approved exceptional race=4 predicate from the still-supplied ordinary
   race/state values and explain their observable reachability.
3. Please supply sequences or vectors covering a fresh launch, a prolonged
   zero-pedal wait, prolonged handbraking, throttle release/reapplication and
   reset, with intermediate z, momentum, prepared force and actual derivative
   expectations sufficient to distinguish a missing producer from an
   incorrect local formula.

No ad hoc history clearing, new launch aid, damping change or tuning change has
been implemented. These values describe independent-host diagnostic fixtures;
they do not establish original-program behavior or retail parameter defaults.
