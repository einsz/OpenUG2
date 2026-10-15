# Independent driving presentation sampling contract

2026-10-15. Implementation is authorized only for the exact hash approved in REVIEW.md. This is an independently chosen OpenUG2 host display policy, not recovered retail behavior, physics tuning, or a handling-parity claim. It supplements the separately approved body-response, body-coupling and applicable configuration contracts without replacing their laws or evaluation boundaries.

## RS-0 Provenance, domain and conventions

PROPOSAL.md is preserved as the original independent requirement proposal, not an approved implementation input. Baseline synthetic evidence is identified in the coordinator's body-response/JITTER_VERIFICATION.md; that report describes a mixed-time presentation discrepancy and explicitly does not establish a native fractional-alpha pixel result. No original binary, source, recovered presentation algorithm, proprietary data or original asset is used here.

Two complete compatible snapshots S[n-1], S[n] represent adjacent completed fixed simulation ticks separated by supplied h>0. Both are from one actor, one car/geometry generation and one compatible presentation mode. Snapshot origin O is the geometric origin, not the centre of mass. B=[f l u] maps body-local vectors into the world; body x is forward, y lateral, z up and f cross l=u. Each supplied B is a proper orthonormal basis. An explicit fixed host-coordinate conversion applies consistently to every channel.

A snapshot supplies O, B, each wheel's stable identity, body-local resolved hub position H[i], travel d[i], bounded steering angle a[i], unwrapped accumulated rotation q[i], and camera target channels that describe that actor. H already includes the snapshot's suspension displacement: do not apply travel twice. A host storing a mount plus travel may equivalently resolve H using its established local travel axis before sampling. Geometry and attached local lamp offsets are fixed across a compatible pair. Wheel identities and their geometry/side mapping are established by the active car geometry, not guessed from array parity. All values are finite; alpha is supplied finite in [0,1]. Invalid/nonfinite input and degenerate snapshot bases are outside this contract.

Units are metres, seconds and radians. Scalars and vector components use absolute tolerance 2e-6 for the bounded fixtures. Axis lengths, pairwise dot products, handedness and endpoint components use tolerance 2e-5. Exact comparisons below concern selection, identity and read-only state, not floating-point arithmetic order.

## RS-1 One presentation time

For ordinary presentation with latest completed tick n, the sample time is t=(n-1+alpha)*h relative to tick zero. Alpha is the residual fixed-clock fraction supplied by the host; rendering must not advance the simulation or synthesize another completed tick. Chassis origin/orientation/body-up, wheel roots and hubs, wheel travel/steering/rotation, attached lamps and the camera's actor target all use the same selected pair and alpha. Geometry transforms must not mix current-only channels with fractional channels.

The camera's own spring, offset and collision response remains independently defined. Its actor target position/orientation must derive from this same sampled actor pose and target channels. Camera position itself need not equal the body origin. In follow-other-actor mode the target comes entirely from that actor's sample; player dynamic axes, wheel data and history must not leak into it. Discrete camera switches select the new target coherently; any separately defined camera transition is not an interpolation of incompatible vehicle history.

## RS-2 Scalar, position and attachment sampling

For every scalar or vector component v in O, H[i], d[i], a[i], q[i], and continuous camera target channels, sample v(alpha)=(1-alpha)*v[n-1]+alpha*v[n]. Steering uses each wheel's supplied bounded angle; it is not averaged between wheels. Discrete target channels select the applicable current mode/identity and are never numerically blended.

Keep the supplied unwrapped q through interpolation, including negative rotation, wrap crossings and changes larger than pi per tick. An equivalent displayed angle may be reduced modulo 2*pi only afterward. For comparisons requiring a canonical display phase use r=q-2*pi*floor(q/(2*pi)), in [0,2*pi); equivalent rotations within tolerance are acceptable. Never choose the shortest angular path for accumulated wheel spin.

The sampled world wheel hub is O(alpha)+B(alpha)*H[i](alpha). A fixed body-local lamp attachment p has world position O(alpha)+B(alpha)*p and orientation derived from that same sampled body basis plus its fixed local attachment frame. All wheels retain their own hub, travel, steering and spin through the transform; no shared axle or last-wheel substitution is permitted.

Each wheel's visual geometry stays assigned to its physical mount identity and side. Any asymmetric outward face must face away from the car centre along its mount's lateral side, and steering/spin must retain the supplied wheel's physical axis and rolling sense after fixed mesh-coordinate/mirroring conversion. In the synthetic convention a mount with positive body-local y is on the positive-lateral side, and its outward face normal has positive dot product with l; a negative-y mount's outward face normal has negative dot product. A fixed geometry conversion may not swap left/right wheel histories or reverse physical rolling sense. This is a host display requirement, not an original wheel-rendering law.

## RS-3 Orientation

B(alpha) must be continuous in alpha for each compatible pair, proper, orthonormal and right-handed, stationary for equal endpoint orientations, and equal the supplied endpoints at alpha=0/1. Continuity is in the actual axes, not wrapped Euler labels. For pure yaw pairs whose shortest yaw separation has magnitude at most pi/2, interpolate that shortest yaw linearly; this includes crossing the +/-pi label boundary. This limited choice gives explicit test values without imposing an interpolation implementation for arbitrary 3D pairs.

For arbitrary proper 3D endpoints, require endpoint agreement and continuous proper axes; no unique intermediate orientation is prescribed. At a 180-degree ambiguity any deterministic continuous branch meeting these conditions is allowed. Sampling repeated identical inputs yields identical output. Origin, wheel hubs, lamp frames and camera target orientation consume the same B(alpha). Rendering does not republish physical B or held angular velocity, normalize simulation state, or perform another physics preparation pass.

## RS-4 Clock and discontinuity boundaries

Zero simulation ticks between renders retain the same adjacent snapshot pair and sample each render's supplied alpha. A frame completing several ticks selects only the final adjacent completed pair. Every completed tick, including interior substeps if the host distinguishes them, retains its approved simulation timing; presentation history is defined at the completed fixed-tick export boundary, not at an interior Euler/preparation state.

At initialization before two compatible ticks exist, reset, teleport, car/geometry generation change, dynamic/legacy presentation-mode change or incompatible actor generation, history becomes two copies of the new current snapshot before presentation. Every channel comes from the new snapshot at every alpha; no sweep from the old car, basis, wheels or target occurs. This rule requires no new physics reset law: it reacts to the host's explicit discontinuity event. Automatic teleport detection thresholds are outside scope.

Pause, explicit snapped presentation and deterministic audit alpha=1 select the current snapshot for every actor channel. Pause/snap overrides a residual fractional alpha. Returning from a discontinuous mode uses compatible history only. Ordinary alpha=0 selects the previous snapshot and alpha=1 selects the current snapshot. Equal snapshots remain visually stationary at all alpha, including repeated renders.

## RS-5 Read-only dynamics and acceptance

Sampling must leave physics persistent state, held/exported channels, momenta, forces, contacts, queries, derivative/preparation timing and tick sequence bitwise unchanged. Rendering may update its separate presentation output/history, but cannot issue simulation/contact/geometry queries that alter or replace the approved simulation inputs. Varying render cadence with identical fixed-tick inputs produces the same simulation outputs. Do not smooth native road travel in the dynamics, add damping, change tyres/suspension or tune physical parameters to meet this display contract.

Approved synthetic vectors are test-vectors/render-sampling.json. Each vector declares synthetic provenance and states numeric outputs or testable properties; property cases intentionally allow any qualifying 3D interpolation. Run these at multiple alphas and repeated inputs, including zero/multiple-tick schedules, equal snapshots, wrap/fast/reverse spin, individual wheel channels, side mapping, attachments, camera actor switching, tilted orientations and each discontinuity event. Compare pre/post simulation state bitwise and compare complete fixed-tick output under two varying render cadences. Tests may observe presentation outputs through a host-specific adapter; no architecture is prescribed.

Before claiming the user's moving-body/wheel symptom fixed, verification must exercise fractional-alpha motion under varying render cadence and record observed chassis/hub/lamp/camera target agreement. An alpha=1 deterministic native drive checks physical-trajectory preservation only. Native visual confirmation remains a separate acceptance result and cannot be inferred from synthetic arithmetic or a successful build.

## Unknowns and exclusions

Retail interpolation, wheel mesh conventions, camera latency and original trajectories are unknown. This contract does not establish the active host mesh conversion, camera response, teleport producer, exact rendering appearance, physical contact stability or whether all reported jitter has one cause. These require implementation verification; the spec supplies no new original physics laws or tuning targets. Existing companion unknowns remain unchanged.

Only SPEC.md and the vector file expressly hashed in REVIEW.md are implementation inputs from this directory. REVIEW.md supplies gate metadata. PROPOSAL.md and external diagnostic evidence are provenance, not implementation inputs. Any later SPEC.md edit invalidates approval; vector changes require re-review.
