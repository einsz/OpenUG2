# Provisional acceleration vectors

Each vector in provisional.json is a synthetic input/output pair calculated from BC-1/2. No vector came from original execution. Scalars persist as binary32; these cases use exactly representable arithmetic. Apply absolute or relative tolerance 1e-6, whichever is larger.

- selected_0 / selected_1 differ only in retained inertia selection; alpha is -1 / -.5, and wheel 0 Ai is 3 / 2.5.
- signed_lateral_projection distinguishes the specified lateral-axis x factor: wheel 0 Ai=-2, while conventional heading projection would give +2.
- external_torque_and_spin_excluded has nonzero prepared moment, held spin and a nonzero mount arm but zero provisional acceleration.
- tilted_local_force makes a world vertical force a local forward force; all four Ai=3.
- mixed_forces_candidate_basis combines four nonzero planar wheel forces, both mount coordinates and a pre-repair Euler basis. It requires transpose(B) without early normalization; the local prepared force is (0,-5,6), ax=2, ay=.5, alpha=-1.25 and Ai=(4.5,-3,.75,1.75).
- normal_force_moment_excluded has a positive off-centre normal load but no provisional yaw acceleration.

The files specify the acceleration callback boundary before a single pass's eligible force updates. Existing Stage1 five-pass scheduling and wheel correction remain companion requirements. Fprepared_world is the already prepared accumulator and does not include the fixture's normal wheel forces. Tprepared_world, held_Omega_world and normal_load are explicit distinguishing state, deliberately excluded from this callback's arithmetic. The reviewer may inspect the private derivation/evidence register; no original material is needed to consume these vectors.

Vector schema: mount_xy already contains (mount.x-centre.x, mount.y-centre.y), not an absolute mount. B_columns lists world-vector columns. Fx/Fy are the provisional Fi/Yi, heading/lateral are hi/li, and Ai is ordered wheel0…wheel3. Units and domains are BC-2/3. The mixed candidate is a valid pre-repair Euler orientation from identity with yaw increment .5; this is a boundary arithmetic fixture, not a normal timestep prescription.
