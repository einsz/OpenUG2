# Independent dynamic-driving presentation sampling proposal

Status: proposed; requires fresh reviewer approval before implementation use.

Provenance: synthetic OpenUG2 host presentation requirements and baseline observable sampling discrepancy; no original binary, original code, runtime recording, proprietary data or recovered retail presentation algorithm is used. This is an independent host display contract, not a change to approved physics or a retail-parity claim.

## Domain

Two complete compatible player snapshots represent adjacent completed physics ticks. Each contains body origin, proper right-handed body axes, wheel-local hub placement/travel, per-wheel steering and unwrapped accumulated wheel rotation, and the player target channels consumed by the gameplay camera. Presentation alpha is finite in[0,1]. All supplied values are finite and basis axes nondegenerate. The sampling operation is read-only; physics state and derivative/preparation timing must remain bitwise unchanged.

## Requirements

RS1: A rendered player has one presentation time. Chassis, body up, wheel-root frames and hub travel, wheel steering/rotation, attached lamps and camera player target all use the same snapshot pair and alpha. The camera's independently defined spring/collision response is not required to equal body position; its player target must correspond to the same presentation time. Follow-other-car mode uses that actor's target and must not reuse player dynamic axes.

RS2: Position, travel and unwrapped wheel rotations interpolate linearly between the snapshots. Each wheel retains its own rotation and hub travel. Presentation rotation may be reduced modulo2pi only after interpolation; it must not take the shortest angular path for a wheel that turns more than pi during a tick. Steering interpolates its supplied bounded angle, not a shared average.

RS3: Body orientation is continuous, proper and right-handed at every sample, equals the supplied endpoints at alpha0/1 within binary32 tolerance, and is stationary for equal endpoints. For a pure yaw pair within a quarter turn, midpoint yaw is the angular midpoint; arbitrary finite proper-axis pairs require orthonormality and continuity, without prescribing a recovered retail algorithm. Origin and wheel-hub world transforms use this same sampled basis. No physical B or held angular velocity is republished by rendering.

RS4: Extra rendered frames with zero simulation ticks still sample the same pair at their current alpha. After multiple physics ticks, the pair is the last adjacent completed ticks, not stale history. At pause, a snapped frame, or deterministic audit alpha1, every player channel equals the selected current snapshot. A reset, teleport, car switch, dynamic/legacy mode switch or incompatible generation changes the pair to two copies of the new current snapshot before presentation; no sweep from the old actor occurs.

RS5: Identical stationary snapshots yield identical presentation at every alpha. Render rate cannot change tick sequence, body/wheel forces, contacts, momentum, geometry queries or simulation outputs. Native rough-road travel is not modified merely to make rendered travel smoother.

## Acceptance and limits

Run the synthetic vectors in test-vectors/render-sampling.json; compare numeric scalar channels with absolute tolerance2e-6 and axis dot products/lengths with tolerance2e-5. Verify zero/multiple tick schedules, read-only state and reset/mode transitions. A native deterministic drive at alpha1 provides a physical-trajectory preservation check, not evidence for fractional-alpha display quality. Exercise fractional-alpha motion at an interactive or synthetic varying render cadence before claiming the driving jitter fixed. Retail interpolation algorithm, exact retail camera latency and original motion trajectories remain unknown.
