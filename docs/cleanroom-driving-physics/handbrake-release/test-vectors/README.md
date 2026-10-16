# Synthetic release and history vectors

release.json is normative after review. All inputs are invented component-boundary states; expected outputs are arithmetic/state consequences of SPEC.md and its approved companions, not original runtime captures. No recorded full-vehicle trajectory, retail car-specific launch speed or measured recovery time is present.

Every vector records its boundary. Unspecified rest exclusions are not assumed: rest vectors explicitly supply all of them. The z history sequence prescribes stationary published velocities and h=.01 over 1500 steps; its equal-history assertion compares each listed handbrake/rest/stabilization-gate case while preserving those prescribed velocities. It does not claim an unconstrained vehicle stays motionless. No extra Euler update occurs for zero-duration post-step sweeps.

Thresholds and booleans are exact. Numeric outputs allow max(absolute 1e-6, relative 1e-6) tolerance; the accumulated history checkpoints allow 1e-5 absolute. Input decimal binary32 thresholds are the rounded numbers shown in JSON. The handbrake-lock zero-slip case deliberately skips evaluation despite nonzero brake demand. Timing vectors assert eligibility, not invented clutch torque magnitudes. Pose import and construction are distinct boundaries; full retail reset is explicitly unknown.

Reviewer-added cases are synthetic arithmetic or logical consequences of HR-1…4 and the approved companions, with provenance repeated in each case. HR-3 cases exclude the stuck/reset-request timer side path. Sequence assertions describe the required boundary ordering; no original host or retail trajectory was executed. The c=0 retained-spin case does not identify c with grounding or contact count.
