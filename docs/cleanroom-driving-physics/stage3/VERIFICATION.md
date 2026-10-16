# Stage3 independent verification — 2026-10-15

Fresh verifier: `stage3_braking_verification`. Result: PASS for tested approved
contracts; no behavioral divergence found and no new reviewer cycle required.
This establishes consistency with the statically inferred specification, not
retail runtime parity or measured stopping-distance fidelity.

The workspace was `codex/driving-physics-stage3`, baseline `ace2c78`, with the
implementer's uncommitted changes. Independently matched approved SHA-256 hashes:

- Stage3: `d4689a16861df810159760a6a668ef3714966fd6c20a30e3b2b54f355c41c6f7`.
- Stage1: `61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb`.
- Stage2: `d3072359aa25bf0ef541504805f70b3579829bb9b1310cda8ba1a568286c6cb6`.

## Project test command and verbatim results

`make braking-test resistance-test drivetrain-test drive-stage1-test physics-clock-test district-collision-test`
completed with exit 0. Selected final output, verbatim:

```text
Stage 3: 479 checks, 0 failures
Stage 2: 348 checks, 0 failures
PASS: 3606 checks, 0 failed
Stage 1 gameplay adapter tests passed
physics_clock_test: PASS (synthetic OpenUG2 contracts; no retail equivalence claim)
district_collision_test: PASS
```

The Stage3 harness loads all 54 approved synthetic vector cases and checks each
component output, exact decisions/enums, stored slip and initial tyre force.
Its extra boundaries cover pedal shaping before engine factors, first-derivative
latency, coefficients held during changed contact speed, incoming/new lock
ordering, pre-decision versus final-cap rear grip, engine reaction, derivative
smoothing and locked/gripping correction, and unconditional direction crossing.
The gameplay suite tests field mapping/rejection, brake-slider independence,
zero service strength, unsupported-wheel spin braking without body braking,
locked contact-force accounting, reset lock/configuration retention, reverse,
Space hold, replacement and clock behavior. Source inspection confirmed shared
pedal exclusivity runs once, body-force accumulation includes supported locked
tyres once, and the host service path has no additional service deceleration.
Car-switch validation precedes replacement; invalid fields preserve the current
car. That interactive switch itself was inspected, not driven through the UI.

## Independent numerical/property oracle

A separate Python/ctypes oracle computes approved mathematical requirements,
using binary32 supplied inputs/state stores and double intermediates under the
vector convention. Deterministic seed 31015 covers 30,000 preparations, 10,000
frame distributions and 21 direction threshold/crossing cases. It checks signed
B/H addition, incoming-lock omission, one-sided combined-torque relief and its
suppression, bypass retention, Q/A/C decisions, stored pre-snap slip, axle
coefficients/magnitudes, ordinary requested-torque sums, strict pedal/speed
boundaries, zero-load/zero-spin/contact-stationary states, and direction epsilon.

```text
Independent oracle: 346974 checks, 0 failures; 30000 preparation, 10000 distribution and 21 direction cases
```

Exact A/C and A=1 branches additionally come from the approved comparison vectors.
No serialization round trip applies to this stateful numerical API. Invalid
configuration/parser paths are covered by project harnesses; NaN physics inputs
and negative physical loads are excluded by S3-0.

Fresh standalone Stage3 compile/run passed `-std=c99 -Wall -Wextra -Werror
-pedantic`. Fresh Stage3 and gameplay-adapter ASan/UBSan builds/runs passed with
`-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer` and
`ASAN_OPTIONS=detect_leaks=0`. Leak checking was not performed. Existing unrelated
parser/physics indentation warnings occur in the ordinary project builds.
Private clean synthetic scripts, binaries and full logs are ignored under
`cleanroom/verification/stage3/` (oracle.py/oracle.log, project-tests.log,
strict.log, sanitizers.log, adapter-sanitizers.log).

## Native evidence and limits

The implementer reports passing final GLES/debug builds and a native debug audit
using `/tmp/openug2_stage3_final` with 2100 frames; those builds were not repeated.
The verifier inspected its live log and launched no second game. The log reaches
the requested braking phase: sampled Stage3 rows at ticks 1920–2070 all have
`brake=0.000` and zero requested service torque. The car is near rest at phase
entry and the host keyboard policy selects reverse, so this run does not prove
native service-brake application or a stopping trajectory. Synthetic gameplay
fixtures do exercise nonzero service braking. Native visual/audio quality,
interactive replacement/reset and complete race behavior remain unverified.

No original executable, original instructions, Wine or runtime snapshot was run.
No original-runtime differential was possible without the required isolated,
snapshotted, no-network environment. Comparison used approved synthetic reference
outputs and an independent specification oracle, not original captures. U3-1–U3-7
and companion unknowns remain open, including x87 marginal precision, retail
initialization/relief defaults, upstream clock, supplied load/contact/grip,
nonzero handbrake production and tuning. The host's zero initial lock mask,
relief false, static quarter-weight loads, planar chassis and Space hold are
explicit fixture choices. Space's direct hold and spin constraint remain separate
from service braking and do not establish Stage5 compatibility.

No implementation, approved specification or shared vector was edited by the
verifier. See VERIFIER_AMENDMENTS.md. Legal sufficiency remains for counsel.

## Supplemental native audit — 2026-10-15

A fresh verifier independently checked the diagnostic-only early S interval and
recorded 420-frame native log. Targeted extracted-block execution passed 10,000
schedule/gating cases; 40 pre-interval telemetry rows matched baseline. Moving
service pedal/torque, falling wheel/body speed, pedal release and resumed
acceleration are now demonstrated by the new recorded native audit, with zero
wall/rail/nonfinite/bounds flags. This supersedes only the missing native
nonzero-service-pedal evidence, not the historical observation about the old
2100-frame run. See NATIVE_AUDIT_VERIFICATION.md for evidence, hashes and limits.
No new divergence/amendment; numerical suites/build were not repeated for this
input-only delta. Original-runtime differential and retail stopping fidelity
remain unverified.
