# Body-response questions and boundaries

2026-10-15. Analyst answers are normative only in SPEC.md after fresh review.

- BR-Q1: Is the inertia selector wheel contact count? **Unresolved producer.** SPEC BR-1 requires a separate supplied c and permits labelled independent host policy; no recovered contact mapping is asserted.
- BR-Q2: Can ordinary gameplay replace prescribed heave/pitch/roll now? **Yes within the explicit input boundary.** SPEC BR-4/7 defines the complete force-driven six-degree body path and required feedback acceptance. Full original world collision/rest/event reachability is not claimed.
- BR-Q3: How is orientation repaired? **Resolved for nondegenerate input.** SPEC BR-4 defines forward-anchored orthonormal repair after post-step/preparation and preserves pre-repair held Omega.
- BR-Q4: What impulse does bottoming apply? **Resolved cap/order interface.** BR-6 combines Stage5 requested impulse with linear cap1000, capped-impulse moment cap10 and existing suppression/sweep. Generic collision production remains external.
- BR-Q5: Does point-force torque use capped linear force? **No.** BR-6 explicitly corrects that implication for extreme forces; moment uses requested force before its independent cap.

Complete original selector production and runtime handling validation remain open. Implementer questions must be added here for a fresh analyst/reviewer amendment, without requesting original material.

## Implementer question BR-I1 — provisional planar coupling inertia (2026-10-15)

BR-1 explicitly defines selected inertia for angular-velocity publication. The
companion Stage1/4 five-pass provisional planar acceleration callback also has
a supplied scalar inverse yaw inertia. Must that callback use the selected
local Iz inverse, basic local Iz inverse, or remain a separately supplied host
boundary under this amendment? No replacement callback or gyro law is specified.
The independent baseline callback and its explicit supplied inverse are retained
pending clarification; full body publication uses the selected BR-1 diagonal.
