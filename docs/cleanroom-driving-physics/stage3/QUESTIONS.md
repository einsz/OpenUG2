# Stage3 questions

No implementer questions are pending at initial analyst handback.

| ID | Scope question | Specification disposition |
| --- | --- | --- |
| Q3-1 | Is there a separate service-pedal curve? | S3-2: linear after exclusivity; device mapping U3-1. |
| Q3-2 | Which strength/share and units enter wheel torque? | S3-1/S3-2; stock data only; variants U3-6. |
| Q3-3 | Is brake relief symmetric or a timed ABS controller? | S3-3: one-sided combined-torque scaling; disable producer U3-3. |
| Q3-4 | When can spin stop without a persistent lock? | S3-4 table and S3-6 unconditional direction crossing. |
| Q3-5 | Does no contact suppress all brake torque? | S3-4/S3-5: no explicit contact gate; load governs body force. |
| Q3-6 | Do lock snap and pedal release recompute slip/clear all state? | S3-3–S3-7: sampled slip retained; lock retention and prepared-force ordering explicit. |
| Q3-7 | How do brakes interact with clutch/engine/body? | S3-5/S3-7; no extra host body deceleration. |
| Q3-8 | Where does handbrake belong? | S3-8 shared B/H interface only; complete producer/grip behavior U3-7. |
| Q3-9 | Does composing the stages repeat shaping, reset lock on release, or add body braking? | Companion-overlap rules at the start of SPEC.md: one exclusivity operation; S3-4 lock retention; S3-5 derivative scope; one Stage2 contact-force lifecycle. |

Future questions must be answered by an analyst amendment to SPEC.md and a fresh
review/hash gate. Unknown IDs U3-1–U3-7 are retained and are not hidden defaults.
