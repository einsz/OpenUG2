# Temporary modified 350Z removal verification

Date: 2026-10-15. Role: fresh clean-room verifier. Scope: requested removal of the temporary configuration experiment while retaining stock per-car loading and approved body, coupling and presentation contracts. No original executable, disassembly, original source or private asset analysis was used. No implementation files were edited.

## Independent checks

`make body-response-test body-feedback-test body-coupling-test body-coupling-host-test render-sampling-test drive-presentation-test drive-stage1-test drive-stage4-test drive-stage5-test drive-trace-test drivetrain-test resistance-test braking-test lateral-test contact-test physics-clock-test` completed with exit status 0. Full compiler/test output: `/tmp/temp350z-removal-verifier/tests.log`. Relevant output, verbatim:

```text
body-response: 63 vectors, 0 failures
BR-7 dynamic feedback acceptance passed; trace: build/body_feedback.csv
BC-1..3: seven approved body-coupling vectors passed
BC host replay: 112 preparations matched independent five-pass callback
All 15 approved render-sampling vectors passed
drive presentation: common sample time, independent hub/spin, exact endpoints and immutable physics passed
Stage 1 gameplay adapter tests passed
Stage4 gameplay adapter tests passed
Stage5 direction handoff: 123220 substep observations passed
Stage5 adapter: contact lifecycle, force accounting, handbrake/recovery, host pose, surfaces, reset/collision/clock passed
drivetrain trace: schema, force/host distinctions and immutable state passed
PASS: 3606 checks, 0 failed
Stage 2: 348 checks, 0 failures
Stage 3: 479 checks, 0 failures
Stage4: 208 checks, 0 failures
Stage5: 77 vectors + 29 surfaces, 503 checks, 0 failures
physics_clock_test: PASS (synthetic OpenUG2 contracts; no retail equivalence claim)
```

Approved SPEC SHA-256 values still match REVIEW.md: body-response `3a415ca2c95941f3f5c02749e444ec566e48c2e818adacefc0d953eed23a0b8e`; body-coupling `b09041f16661995375af617766d672db594c27a5bcbe2e8e5783c43b156d6f33`; render-sampling `ae4830e01a38054b37fc4aede4545bab80eb602450ffdfa63eb4f7d8bb259825`.

Source/build inspection found no temporary test350z module, profile selection, effective-upgrade application, F7 branch, profile/source/profile-trace parser branch or sidecar writer in active src/tools/Makefile. The ordinary Stage1–5 and body readers still load by car name at initial load and car switching. Presentation capture/mixing and individual wheel sampling remain active. The render-sampling target links ordinary driving modules without feature-only crypto linkage; the shared UI crypto dependency remains. Ordinary geometry profiles and unrelated settings/sky/performance profiles remain valid independent features.

Current DRIVETRAIN.md and src/DESIGN.md describe stock loading and removal; three test350z archive READMEs explicitly mark historical commands and paths as historical. Approved historical evidence was retained, including prior native observations; those observations do not represent the current executable.

## Removed option behavior

Tested the preserved rebuilt desktop executable `/tmp/openug2-remove-temporary350z/nfsu2-debug`, SHA-256 `8f8d654f9d72a114afdba69f054e77c8bc80137ef886737a071661fd045fd4fb`. For each of `--drivetrain-profile temporary-opening-350z-provisional`, `--drivetrain-source /nonexistent/removed-global.bun`, and `--drivetrain-profile-trace /tmp/temp350z-removal-verifier/should-not-exist.jsonl`, supplied `/nonexistent/openug-test-data --car 350Z --drivetrain` and bounded frames. All returned exit 1 with `cannot derive an authored start region for STREAML4RA`; no sidecar was created. Exact invocations/stdout/stderr: `/tmp/temp350z-removal-verifier/removed-cli.json`.

The old options no longer activate their former behavior. They are **not explicitly rejected as unknown options**: the existing generic parser fallback treats unrecognized tokens as data-root arguments. No exit-2 rejection claim is supported. This is an existing parser boundary, not a demonstrated divergence from the approved physics specifications; strict unknown-option rejection would require a separately reviewed behavioral requirement.

## Native/build evidence and limits

The removal worker owns full debug/GLES builds and native execution; their results are supplementary, not independent verifier build results. The verifier independently inspected the produced stock350z.csv: 230 rows, 133 fields, ticks 0 through 229, no nonfinite numeric values. Native log identifies ordinary 350Z geometry and enabled stock Stage1–5 plus dynamic body. Evidence directory: `/tmp/openug2-remove-temporary350z`, including native.log, stock350z.csv and spawn/end PNGs. Worker reported native exit 0 for `DISPLAY=:0 SDL_AUDIODRIVER=dummy timeout 90s ./nfsu2 /home/dq/Games/Need\ for\ Speed\ Underground\ 2 --car 350Z --drivetrain --track STREAML4RA --resolution 640x360 --drive-audit /tmp/openug2-remove-temporary350z/stock350z --frames 230 --drivetrain-trace /tmp/openug2-remove-temporary350z/stock350z.csv`, with no profile sidecar. Worker reported debug and GLES builds exit 0 (debug.log and gles.log), then restored the exact copied desktop debug executable. These full builds were not independently rerun by the verifier.

No spec-vector divergence was found and no proposed physics amendment was needed. No original-runtime differential parity was attempted or established. Passing synthetic tests does not establish retail trajectories or complete handling parity. This removal check does not independently re-establish native fractional-alpha visual quality, every car asset, or live F7 input behavior; absence of the temporary branch is supported by source inspection. Historical jitter/native reports retain their original scope and limitations.
