# Stage-2 questions and boundaries

At analyst hand-back, no implementer had been given this stage. Questions must be answered
by amending SPEC.md and passing a fresh reviewer gate.

| ID | Status | Contract / remaining question |
| --- | --- | --- |
| S2-Q1 | Resolved statically | S2-2: signed vector quadratic drag; throttle multiplier 2-t; no extra density/half factor |
| S2-Q2 | Resolved statically | S2-4/S2-7: Stage-1 wheel resistance is retained exactly once; body aerodynamics precedes wheel coupling |
| S2-Q3 | Resolved statically | S2-5: per-car threshold is mph and replaces positive forward force; it does not clamp velocity |
| S2-Q4 | Resolved statically | S2-7: separate full-vector 200 m/s cap, mass strictly greater than binary64 0.7 |
| S2-Q5 | Resolved within supplied state boundary | S2-6: body stabilization damping and scalar evolution are distinct from wheel/body aero drag |
| S2-Q6 | Open | U2-1/U2-2: reduction-state initialization, general producers and user-visible mode meanings for scale/reduction/enable inputs |
| S2-Q7 | Deferred | U2-3/U2-4/U2-5/U2-8: surfaces, damage lifecycle, upgrades and contact/angular feedback |
| S2-Q8 | Open | U2-6/U2-7: retail runtime traces, precision and upstream timing |
