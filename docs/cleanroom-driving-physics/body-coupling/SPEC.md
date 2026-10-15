# Provisional tyre/body coupling amendment

2026-10-15. Implementation is authorized only for the exact file hash approved in REVIEW.md. Companion body-response SPEC.md SHA-256 `3a415ca2c95941f3f5c02749e444ec566e48c2e818adacefc0d953eed23a0b8e` and approved Stage1–5 specs supply the remaining behavior. This amendment answers BR-I1 and explicitly supersedes S1-8.2 and its S1-7.5 heading/contact-arm wording, and the S4-6 implication that an external angular moment enters the provisional callback. BC-1/2 supplies the composed selection, mount-arm and projection boundaries; their other laws remain companion requirements. It does not change full body momentum integration, publication, force caps, final wheel-force application or the five-pass schedule.

All recovered claims below are **inferred** from static executable evidence, cross-checked against an authorized analyst reference. No original runtime was executed. Vectors are synthetic arithmetic examples, not measured original handling. Private evidence identifiers BC-E1…4 resolve in the analyst evidence register.

## BC-1 Selected scalar and boundary

The scalar inverse yaw inertia used by the provisional five-pass calculation is the local yaw component of the **currently selected** BR-1 inverse inertia. With basic diagonal `(Ix,Iy,Iz)`, positive multiplier `kI`, and current selection s, use

`j = 1/Iz` when s=0; `j = 1/(Iz*kI)` when s=1.

This is the same selection retained by full body angular-velocity publication. It is not an independently tuned scalar in a composed BR dynamic path, and is not always the basic inverse. Selection refresh and its asymmetric world-angular-momentum transition remain exactly BR-1. Isolated Stage1/4 component fixtures may still supply their scalar explicitly, but must identify that fixture boundary. Such a fixture does not establish the composed selection rule. (BC-E1, BC-E2.)

The callback remains a reduced **local planar** calculation even while the body follows BR full 3D dynamics. Do not substitute a world inverse-inertia component, a full angular-acceleration vector, the finite difference of held Omega, gyroscopic terms, or the BR damped/capped angular derivative. Full body inertia has a separate role in BR angular-velocity publication. No equality between this provisional scalar acceleration and the actual body yaw derivative is required. (BC-E1.)

## BC-2 Supplied state and provisional calculation

Use BR units and axes: M is tonnes, Iz is tonne·m², kI and s are dimensionless, mount/centre coordinates are metres, forces are kN, j is 1/(tonne·m²), ax/ay/Ai are m/s² and alpha is rad/s². B and the local heading/lateral directions are dimensionless. At coupling entry, after the established body post-step/optional zero-duration sweep and aero preparation, snapshot `e = transpose(B)*Fprepared`, where Fprepared is the existing world **linear** force accumulator before the once-only final wheel point-force applications. Use the current candidate B at that preparation boundary; do not repair it early. This snapshot is retained throughout the five passes. Gravity/aero or other contributions enter through this prepared force; normal wheel forces are not added to this snapshot merely to make it a full 3D acceleration. (BC-E1, BR-4.)

For wheel i, use current provisional longitudinal/lateral forces `(Fi,Yi)` and its current local wheel heading/lateral basis vectors `(hi,li)`. Use the two local mount-arm coordinates `(xi,yi) = (mount_i.x-centre.x, mount_i.y-centre.y)` from the established suspension mount state. These coordinates are the x/y components of the Stage5 S5-2 frame-held mount arm; the ride offset changes only its z component. They remain fixed across the frame, as do local steering directions (S4-3). These are not a world contact point minus centre. Travel, radius, surface normal and vertical contact offset do not change these two coordinates. The basis is the local steered wheel frame, before the final selected world contact-force basis. (BC-E1, BC-E3, BC-E4.)

At the start of each pass, using all four wheels' current force values, define

`qx_i = Fi*hi.x + Yi*li.x`

`qy_i = Fi*hi.y + Yi*li.y`

`ax = (e.x + sum(qx_i))/M`

`ay = (e.y + sum(qy_i))/M`

`alpha = j * sum(xi*qy_i - yi*qx_i)`.

For every wheel, its pass acceleration input is

`Ai = (ax-alpha*yi)*hi.x + (ay+alpha*xi)*li.x`.

The final factor is **li.x**, including its sign, rather than hi.y. This explicitly replaces the heading-dot-product expression in Stage1 S1-8.2 and the implication of that expression in Stage4 S4-6. For a local pure steering rotation with hi=(cos(theta),sin(theta),0) and li=(-sin(theta),cos(theta),0), it gives `Ai=(ax-alpha*yi)*cos(theta) - (ay+alpha*xi)*sin(theta)`. Do not replace this expression with the physically conventional positive-sine projection. No corresponding change is made to Stage4 force composition or Stage5 contact-velocity projection. (BC-E1, BC-E4.)

No existing angular torque is included in alpha. In particular do not add an externally supplied yaw moment, gravity/aero moment, normal-force moment, BR angular damping, torque limit or late opposing torque to this callback. Only the provisional planar wheel moments above enter alpha. Neither Omega nor L enters this arithmetic directly. They can affect the upstream pose/contact/load/velocity state through the already specified boundaries. No centripetal point-acceleration term is added. (BC-E1.)

All eligible wheel updates share ax,ay,alpha calculated at that pass's start, then use the existing Stage1 longitudinal correction, clutch correction and five-pass ordering. Selection s and mass/inertia inputs retain their frame-refresh values; e and the prepared wheel state are held across the five passes except for the established longitudinal-force/drive-torque corrections. Lateral provisional forces stay fixed during those passes. These are algebraic acceleration evaluations without a duration factor: five passes are not five Euler substeps, and the existing correction law is unchanged. Repeat this preparation at the established substep/creation boundary, including h=0 preparation; do not repeat frame refresh inside it. Recompute the sums for the next pass. The provisional calculation does not accumulate body forces/moments. Final forces, normal contributions and BR point-force caps are applied once at their established later sites. No body-force cap is applied inside these provisional sums. (BC-E1.)

## BC-3 Precision and vectors

Use finite binary32 state/inputs with M,Iz,kI positive; the BR parameter domain and nondegenerate orientation domain apply. The equations specify tolerance-based behavior, excluding exact x87 intermediate equivalence. Persist the calculated provisional accelerations and each updated longitudinal force as binary32. Test-vector tolerance is absolute 1e-6 or relative 1e-6, whichever is larger. BC-1 selection predicates and omission of excluded terms are exact requirements.

`test-vectors/provisional.json` contains one-pass acceleration-boundary inputs and outputs. These deliberately stop before longitudinal update/friction capping and therefore do not prescribe an entire tyre or vehicle trajectory. Each input has four wheels; zero-force wheels still receive an Ai. The vectors distinguish selected/basic inertia, signed projection, external angular torque exclusion, local force transformation, and omission of normal-force moments. All other companion component tests remain under their previously declared isolated boundaries; tests of the superseded provisional formula require replacement.

## Unknowns/Ambiguities

The original producer of BR selector c remains unresolved. Selection must be supplied as BR-1 specifies; this amendment does not identify it with wheel contacts. Effective parameter producers, contact/grip producers, full excluded rest/guide/collision/alternate-state behavior and exact x87 arithmetic remain companion unknowns. Runtime validation and measured trajectory parity remain absent. This amendment does not establish a full 3D tyre acceleration callback; the recovered callback is reduced as stated. A host choosing a different callback must label that behavior independently rather than silently deriving it from full body inertia.
