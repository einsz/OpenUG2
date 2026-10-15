# Session handoff — 2026-10-16

## Stop status

The user explicitly requested: **“stop all work create a session handoff right now.”**
Stop respected. No further implementation, builds, tests, commits or pushes are
authorized by this handoff. Resume only on a subsequent user instruction.

`ordinary_mode_implementation` had just completed and released ownership.
`ordinary_mode_launch_verification` was interrupted before its post-fix execution.
No active build/native workload was found in the coordinator's process snapshot.
All source changes and diagnostic artifacts were preserved.

## Repository and published state

- Workspace: `/home/dq/Documents/Development/OpenUG2`.
- Current branch: `codex/driving-physics-body-response`.
- HEAD: `f8231443823fd24e0a9857bbd1a80105c2841aef` — Add coupled driving body physics and coherent wheel rendering.
- Existing upstream PR: https://github.com/whoismept/OpenUG2/pull/85
  (“Add Stage 5 contact and coupled body physics”), targeting `development`.
- Its source is `troikoss:codex/driving-physics-stage5`; this source branch was
  fast-forwarded to `f823144` and the PR description updated before the current
  bogging investigation. **The current ordinary-mode fix is not committed or pushed.**
- `origin`: `https://github.com/troikoss/OpenUG2`.
- `upstream`: `https://github.com/whoismept/OpenUG2`.
- The authenticated account has no direct upstream push permission. An earlier
  generic fork push was rejected by automatic review due to the user's requested
  upstream destination. The later explicitly requested PR #85 update succeeded
  by pushing its verified fork source branch; the destination remained upstream.
- Stage 1–4 PRs #81–84 were still open at the last check.

## User's current bug

The stock **240SX** sometimes bogs/cannot launch after handbraking.
Releasing and reapplying W does **not** recover; resetting the car does.
The temporary modified opening-350Z profile has already been removed from runtime;
do not reintroduce it. Body physics and wheel/jitter repairs at HEAD must remain.

## Reproduction and findings

See `stage5/HANDBRAKE_LAUNCH_VERIFICATION.md` and
`stage5/HANDBRAKE_LAUNCH_QUESTIONS.md` (HB-Q1–Q3).

The normal independent loader plus a controlled flat-ground fixture reproduces
the 240SX failure. Schedule: 120 ticks idle, 900 ticks handbrake (or matched
no-handbrake idle), 600 W, 60 W-off, 300 W, explicit reset, then 600 W.

| Observation | Handbrake case | Matched idle case |
| --- | ---: | ---: |
| Speed after first 600 W ticks, m/s | .372869551 | .372869581 |
| Stabilization history z | 1.14291179 | 1.14291155 |
| Speed after release/repress, m/s | .509792328 | .509792566 |
| Speed after reset + 600 W ticks, m/s | 29.301878 | 29.3018684 |

During the bog: first gear, approximately 800 rpm, full shaped throttle, no
service brake, normal contact and cleared rear wheel locks. Prepared forward
force is about +4.200 kN; stabilization damping is about −4.159 kN. The 350Z
control recovers normally. Thus the reproduced persistent bog is not a retained
handbrake lock, and long idle can trigger it without handbraking.

A separate exact-rest retained-lock state also traps a pure-RWD supplied fixture,
but ordinary tested handbrake release clears locks. Do not conflate this edge
case with the reproduced stock 240SX bog or invent an unconditional unlock.

## Reviewed explanation and candidate fix

Original static evidence identifies the previously named `race` input in body
damping as **session lifecycle status**, separate from event family. Status 4 is
active driving: a generic completed-start transition reaches it, and free roam
also explicitly reaches it. In that status, shaped throttle strictly above .3
disables translational stabilization `−8*z*P` and its paired angular-history
term under the already approved rules. History evolution itself is unchanged.

The host previously supplied status 0, leaving stabilization active during
throttle. The uncommitted patch supplies the reviewed active/countdown lifecycle
through a narrow independent host policy. No torque boost, coefficient tuning,
blanket history reset or new wheel-unlock rule was added.

Approved implementation input:

- `ordinary-mode/SPEC.md` SHA-256
  `596c690aa0916cbdc3ce23f585e31000c96f39040a7a7d26857278c0f77dc1be`.
- Allowed auxiliary hashes are in `ordinary-mode/REVIEW.md`; 13 derivative vectors.
- OM-2a defines supplied snapshots and readiness/countdown/pause/resumption/reset
  boundaries. Original findings remain static-inferred; retail trajectories
  were not captured.

Separately approved lower lifecycle research:

- `handbrake-release/SPEC.md` SHA-256
  `48c2c9c15e62dc9458eeee3ac753500e73ae34a88a0169ab78075e99994d52be`.
- 56 vectors; rest/lock/history findings and exclusions. This contract alone
  does **not** fix the observed bog. Its unrelated rest machinery was not applied.

## Uncommitted implementation at stop

Modified: `Makefile`, `docs/DRIVETRAIN.md`, `src/DESIGN.md`,
`src/body_response.c`, `src/body_response.h`, `src/drive_contact.c`,
`src/drive_stage1.c`, `src/drive_stage1.h`, `src/main.c`.

New source/tests: `src/drive_session.h`, `tools/ordinary_mode_test.c`.
New research/report directories: `handbrake-release/`, `ordinary-mode/`, plus
the two Stage 5 handbrake-launch question/verification files listed above.

Implementer reports OM-1–4 complete: supplied snapshot passthrough; ordinary
active/countdown policy; no-step pause retaining state; excluded routes retaining
their established behavior; refresh after initialization/preparation; per-tick
host propagation; reset preserving the selected policy. Physical equations are
unchanged. Do not discard these uncommitted files or use a destructive reset.

## Validation completed versus pending

Implementer-reported completed:

- All 13 ordinary-mode vectors and integrated lifecycle/preparation/history,
  coasting, reset, pause/resume and readiness/countdown checks pass.
- All 18 targeted regression suites pass.
- Strict compilation passes with existing warning-category suppressions.
- ASan/UBSan pass with `detect_leaks=0`; LeakSanitizer is unavailable under tracing.
- `make debug`, `make gles`, then `make debug` pass; desktop executable restored.
- `git diff --check` passes.
- Logs: `/tmp/ordinary-mode-*.log`.

Current desktop executable SHA-256, read at stop:
`fbee2cf82829d215a785e76ce2a0e83735987434bf8f998564b4848ea885e5e2`.

**Still pending — do not claim the user's launch failure is verified fixed:**

- Fresh independent post-change ordinary-mode vector/host lifecycle checks.
- Replay the exact stock-loader 240SX handbrake and matched idle schedule;
  confirm launch without reset, preserving history and correct coast behavior.
- Verify actual native host session propagation, separately from manually
  supplied component fixture status; include creation/reset/car change and
  ordinary active/countdown boundaries to the supported extent.
- Record recovery time/distance, gear/throttle/RPM/locks/force/history and check
  the 350Z control. No retail acceleration target should be invented.
- Finish `ordinary-mode/VERIFICATION.md`, currently explicitly pending.
- No commit/push or PR update of this patch has occurred.

## Preserved diagnostics and clean-room boundaries

- `/tmp/handbrake-impl/`: independent fixture source, baseline/extended executables,
  logs and stock-loader CSVs. Includes `stock_bench.inc`, `stock-main.c` and
  reproduced handbrake/idle/throttle-off/on/reset cases.
- `/tmp/ordinarymode-verifier/`: copied baseline fixtures, `PLAN.md`,
  `summarize.py`, `baseline-summary.jsonl`, initial logs. Post-fix work was stopped.
- `ordinary-mode/VERIFICATION.md`: verifier's exact planned acceptance and limits.

Apply `/home/dq/.codex/skills/clean-room-re/SKILL.md`. The main session remains
coordinator only: no recovered-code reading or implementation writing. Use fresh
analyst/reviewer/implementer/verifier contexts for new phases. Implementers receive
only approved specs/hashes and independent baseline source, never original code,
analyst notes/evidence or original binaries. Verifier findings return through
reviewed behavioral amendments, not implementation advice.

Authorized analyst-only original: `/home/dq/Games/Need for Speed Underground 2/speed2.exe`,
SHA-256 `f9dd86c054878ce6276beb07c1fd61874f7a1e4bf1f241b084c65b73e24168a7`;
original reference `/home/dq/Documents/Development/nfsu2-re/`.
Private original analysis is under ignored `cleanroom/analysis/`.
Do not execute the original on the host. Native independent OpenUG2 may use the
legal installation through its normal loader, with `DISPLAY=:0 SDL_AUDIODRIVER=dummy`
when appropriate. No original assets, code or private evidence may be committed.

Earlier shipped state, still applicable: temporary profile removed; wheel sides
fixed; chassis/hubs/lamps/camera share interpolated render time. Physical roughly
9.6 cm road-height seam remains unchanged. Full retail parity remains unverified.
