# Ordinary-mode verification — 2026-10-16

Status: prepared; post-fix execution pending serialized implementation/build release.
Verifier owns this report and `/tmp/ordinarymode-verifier/`. No implementation edits, original executable execution, original code or raw original-data analysis are part of this verification.

Approved ordinary-mode SPEC.md SHA-256 independently checked: `596c690aa0916cbdc3ce23f585e31000c96f39040a7a7d26857278c0f77dc1be`. Scope includes OM-2a independent host policy and 13 derivative vectors. Original-behavior claims remain static-inferred; full-duration launch trajectories have no retail oracle.

## Preserved baseline

Source baseline HEAD `f8231443823fd24e0a9857bbd1a80105c2841aef`. Prior report: `../stage5/HANDBRAKE_LAUNCH_VERIFICATION.md`. Reproducer sources/executables/CSVs remain in `/tmp/handbrake-impl/`; source copies and independently generated finite/contiguous CSV summaries are in `/tmp/ordinarymode-verifier/`.

Matched stock-loader flat-ground fresh dynamic 240SX schedule: 120 ticks idle, 900 ticks handbrake or matched no-handbrake idle, 600 W, 60 W-off, 300 W, explicit reset, 600 W. No reset before the explicit reset point. Prior outputs are observed independent implementation behavior, not original runtime captures.

| Baseline observation | HB | Matched noHB idle |
| --- | ---: | ---: |
| Speed after first 600 W ticks, m/s | .372869551 | .372869581 |
| History z at same point | 1.14291179 | 1.14291155 |
| Speed after off/repress, m/s | .509792328 | .509792566 |
| History z after off/repress | .859196484 | .859196246 |
| Speed after explicit reset + 600 W, m/s | 29.301878 | 29.3018684 |
| History z after reset + 600 W | 0 | 0 |

Both stalled samples: first gear, approximately 800 rpm, full shaped throttle and cleared rear locks. Prior 350Z handbrake control reaches 40.6071 m/s after first 600 W ticks. This establishes baseline symptom/control evidence only.

## Planned acceptance and limits

Run all companion spec-vector commands and the new ordinary-mode command after release. Compare positive-z derivative vectors including strict throttle boundary, coast/reapply and session/event-family distinction. Check actual host-supplied lifecycle on creation/readiness/countdown/active free-roam/racing/reset/car replacement/pause/resume/legacy modes to the extent exposed, separately from manually supplied component fixtures.

Replay preserved stock-loader 240SX and 350Z HB/idle schedules and record time/distance to 5 m/s, later speed, gear, throttle, rpm, rear locks, force and retained history. Native audit executes OpenUG2 with normally loaded legal stock assets only. No original executable runs on the host. Recovery limits are observations rather than invented retail targets.

No post-fix pass, trajectory parity or absence of divergence is claimed yet. Any behavioral contract divergence will be recorded here for reviewer gating; no concrete implementation fix is proposed by verifier.
