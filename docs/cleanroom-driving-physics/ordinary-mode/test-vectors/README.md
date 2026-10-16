# Ordinary-mode vectors

These are synthetic derivative-boundary vectors, derived from inferred
OM-1–OM-3 and the already approved BR-2/S2-6 equations. They were not obtained
by executing the original program. Event-family labels are semantic fixture
labels; no numeric family enum is supplied or inferred from session status.

Each vector starts from the stated momentum/history and prepared force/torque.
The force threshold is disabled. This is a controlled derivative test, with
prepared accumulators supplied explicitly, not a claim of force preparation in
retail driving. Expected Pdot and Ldot are in companion units; absolute
arithmetic tolerance 1e-6. Gate comparisons and history preservation at refresh
are exact. D(0)=.05, hence Ldot.x=-.01 with stabilization disabled and -.41 with
it enabled. Pdot.x is 4 or 0 respectively. A mode refresh alone leaves z=1.

Release/reapply vectors are sequential mode refreshes with no Euler step
between them, to isolate gate changes from history evolution. They do not
imply z stays constant during a physical elapsed interval. The free-roam,
circuit and sprint full-throttle vectors share active session status 4 and
must agree. Changing only the separate semantic family label must not change
this gate. The initial/pre-active controls distinguish treating 0 or 3 as
active and are not asserted to be playable idle phases.

Full-duration launch, reset and engine-speed expectations remain unknown;
no invented retail trajectory is included.
