# Stage4 questions

No implementation questions were received during this fresh analyst pass.
The analyst answered the stage intake through the following spec sections:

| Intake topic | Resolution | Remaining boundary |
| --- | --- | --- |
| Ordinary keyboard/controller steer | S4-1/2 speed, mapping, clamp, rate kernels and command quantization | U4-1/2 history, events, initialization/cadence |
| Wheel steer geometry | S4-3 rational front mapping and rear zero | U4-3 trig precision |
| Contact velocities and slip angle | S4-4/5 centre velocity, heading quantizer, reverse folding | Stage5 mount/contact producers; U4-3 marginal rounding |
| Gripping lateral force | S4-5 load/angle interpolation, signs and clamps | U4-7 alternatives excluded |
| Combined grip | S4-6 shared cap, drive test and post-cap correction | U4-4 effective full grip inputs |
| Planar chassis/yaw | S4-6 force/moment and substep contribution | U4-5 full 3D and extra yaw torque |
| Airborne | S4-5/6 zero supplied load/prepared tyre force | Stage5 contact flag/load refresh |
| Interaction with prior stages | S4-0/3/6 carried force timing, five passes and shared final force | U4-6 runtime unvalidated |

Future questions must be added here and answered by an analyst amendment to
SPEC.md followed by a fresh reviewer gate. Unknown producer inputs must not be
turned into undocumented defaults in the implementation.

## Implementer questions, 2026-10-15

- **Q4-I1 (U4-2):** What are the complete history startup, expiry and reset
  semantics? The normative kernel requires supplied h/e. Gameplay supplies an
  explicitly independent 60 Hz FIFO adapter (33 input samples, nine rate samples,
  zero/one initial fill); the original timed producer remains unimplemented.
- **Q4-I2 (U2-8/U4-5):** What is the angular momentum damping/mode-adjustment law?
  The level host accepts explicit inverse yaw inertia and damping coefficient.
  Gameplay supplies the S4-1 stock box inertia and zero additional damping;
  no missing original damping law or assistance torque is inferred. Existing
  Stage2 translational stabilization uses the actual yaw-speed magnitude.
