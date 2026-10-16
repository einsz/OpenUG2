# Handbrake launch verification — 2026-10-16

Fresh verifier on baseline f823144, branch codex/driving-physics-body-response. No original executable, original source, disassembly or raw asset analysis used. Native execution is OpenUG2 via its normal stock loader. No implementation/spec/vector edits. Private evidence: `/tmp/handbrake-launch-verifier/`.

## Current finding

Extended stock-loader flat-ground fixtures reproduce a prolonged 240SX bog after a 15-second handbrake hold. Seven of eight forward 240SX cases fail to reach 5 m/s within 10 seconds after release. The short native audit and invented-tuning synthetic cases recover. This reproduces a relevant symptom in a controlled independent fixture, not the user's exact world state or retail parity.

## Approved contracts and tests

All eight SPEC.md SHA-256 values (Stage1–5, body-response, body-coupling, render-sampling) match their REVIEW.md approvals.

Command (exit 0):

```sh
make drivetrain-test resistance-test braking-test lateral-test contact-test drive-stage1-test drive-stage4-test drive-stage5-test body-response-test body-feedback-test body-coupling-test body-coupling-host-test drive-presentation-test drive-trace-test
```

Verbatim relevant results from `tests.log`:

```text
PASS: 3606 checks, 0 failed
Stage 2: 348 checks, 0 failures
Stage 3: 479 checks, 0 failures
Stage4: 208 checks, 0 failures
Stage5: 77 vectors + 29 surfaces, 503 checks, 0 failures
Stage 1 gameplay adapter tests passed
Stage4 gameplay adapter tests passed
Stage5 direction handoff: 123220 substep observations passed
Stage5 adapter: contact lifecycle, force accounting, handbrake/recovery, host pose, surfaces, reset/collision/clock passed
body-response: 63 vectors, 0 failures
BR-7 dynamic feedback acceptance passed; trace: build/body_feedback.csv
BC-1..3: seven approved body-coupling vectors passed
BC host replay: 112 preparations matched independent five-pass callback
drive presentation: common sample time, independent hub/spin, exact endpoints and immutable physics passed
drivetrain trace: schema, force/host distinctions and immutable state passed
```

## Synthetic matched inputs

Private `probe.c`, `probe`, `synthetic.csv`: invented approved-domain Stage5 fixture, flat ground, 12 m/s entry; 180 ticks with handbrake on/off, steering 0/.2, pedal 0/1, then 300 ticks pedal 1/steering 0/handbrake 0. Run both planar supplied-ride and dynamic-body compositions. All 16 runs finite; all 8 handbrake cases reach 5 m/s within .900–.967 s, travel 65.67–73.83 m during the 5-second release window, and end 24.74–26.32 m/s. All final lock masks zero, throttle 1, forward gear 2/3. Matched no-handbrake cases are already moving 9.33–23.81 m/s at release and travel 105.50–155.12 m; their different release states prevent treating the travel difference as an independent recovery-time penalty. These runs do not use stock tuning and contain no external collision blocker.

Additional private `rest-probe.c`/`rest-synthetic.csv`: start at rest, extend hold phase to 900 ticks (15 s), same 16 combinations and 5 s release window. All 8 handbrake cases recover to5m/s in .917–1.650s, travel55.94–69.64m, end23.87–25.70m/s with lock mask0/throttle1/forward gear2 or3. Dynamic rest/no-pedal noHB controls reach 5 m/s in1.633s versus1.650s withHB; these are useful close release-state controls. A planar steering/throttle/noHB control develops a later signed reverse-speed/braking episode after extensive circular motion; it does not reproduce the handbrake symptom and is not used as a recovery reference.

## Native short audit

Copied existing executable `nfsu2-baseline`; normal-loader stock 240SX and 350Z, STREAML4RA, `--drivetrain --resolution 640x360 --drive-audit PREFIX --drivetrain-trace FILE --frames 420`. Existing input schedule: coast ticks 0–119, W from120, 15-tick Space with pedal0/steer.2 at170–184, W after185, short steering changes210–239 and255–284, service brake300–344. Thus 5 m/s recovery includes those steering changes.

| Stock car | Forward m/s at HB end184 | Release185 | Tick186 | Reach5 m/s after release | Distance to5 m/s | Forward m/s at299 |
|---|---:|---:|---:|---:|---:|---:|
| 240SX | 1.489 | 1.428 | 1.394 | 1.283 s | 3.787 m | 7.944 |
| 350Z | 2.655 | 2.573 | 2.492 | .783 s | 2.571 m | 14.568 |

Both rear locks clear on first release tick185. First forward gear persists; clutch transitions from mode1 at184 to2 at185 and3 by210; shaped throttle1 and service brake0 at release. Engine is near800 rpm at HB end, recovers to915/1271 rpm at210 for 240SX/350Z respectively. No walls/rails in inspected release rows. Initial speed dip accompanies clutch re-engagement and transient tyre force; subsequent speed rise demonstrates recovery in these cases. No adequate-force persistent no-motion interval observed. Both runs exited 0 and contain 420 finite contiguous rows (ticks 0–419). Trace files preserve wheel loads, slip/grip, torque, contact forces and incoming/prepared force channels for further attribution.

## Limits and proposed amendments

No mismatch with approved component equations established. Proposed independent host-policy characterization is recorded below for reviewer consideration. Native short audit has no matched no-handbrake control or held-throttle handbrake case, and it does not reach a handbrake stop. User car, location and precise input duration remain unspecified. Extended stock-loader scenarios are recorded below; post-change verification remains pending. No original-runtime differential comparison or retail trajectory target is available; no parity claim.

## Extended stock-loader fixture observation

Worker-provided diagnostic `/tmp/handbrake-impl/stock-bench`, source `stock-main.c` and `stock_bench.inc`, uses normal production loader then clones loaded Stage1Drive into a flat-ground query with surface 1, no external collisions. Verifier inspected fixture schedule and independently ran350Z via `HB_STOCK_BENCH=1`;240SX run is worker-provided output independently parsed by verifier.16 cases per car vary dynamic/prescribed body, fresh/driven 300 ticks, pedal 0/1 during 900 ticks HB, then 600 ticks W or S; steering 0 throughout. Each reset is before sequence, never atHB release. There is no noHB control in this bench.

All 8 forward 350Z cases reach 5 m/s: dynamic1.383–1.433s, prescribed4.417–4.450s, final31.69–40.73m/s. Seven of 8 forward 240SX cases never reach 5 m/s within10s; final.373–1.362m/s and travel5.81–13.28m. Dynamic driven+pedal1 duringHB case recovers5m/s in2.117s, final29.039m/s, travel154.86m. All forwardcases clear rearlocks on first release tick. Full percase numbers: `/tmp/handbrake-launch-verifier/stock-summary.txt`; input/output CSVs under `/tmp/handbrake-impl/240SX-*.csv` and `350Z-*.csv`.

Representative240SX dynamic/fresh/pedal 0 HB: release speed .00006769m/s;60 ticks later1.066m/s;180 ticks later.678m/s;400 ticks later.441m/s;600 ticks later.373m/s. At final:800rpm,gear1,target1,clutchmode3/engagement1,throttle1,servicebrake0,contactmask15,rearlocks0,engine torque .140kN·m,feedback-.262773kN·m,preparedFx4.20013kN,rear longitudinalforces2.49301/2.49303kN,gripping1. This excludes a retained rearlock or withheld accelerator at that sampled boundary. Prepared force is not net momentum derivative: separately applied stabilization exists; adequate-net-force/no-blocker invariant is not established fromCSV alone.

User clarification via coordinator: 240SX stays stuck after W release/repress until reset. The extended observation below addresses this persistence and reset recovery in a controlled fixture. No reviewed contract amendment or source fix is inferred from the above observations.

## Persistence, reset and matched no-handbrake control

Extended worker fixture `/tmp/handbrake-impl/stock-bench-extended`, normal loader, fresh dynamic240SX:120 ticks pedal0,900 ticksHB/pedal0,600 ticksW,60 ticks pedal0,300 ticksW, explicit `stage1_drive_reset` with zero horizontal velocity,600 ticksW. Parallel `HB_SKIP=1` control replaces HB with noHB, retaining identical other controls. Verifier inspected fixture schedule and independently parsed both worker-produced2580row finite/contiguous traces; no state reset occurs until elapsed960. Logs `extended240sx.log`, `extended240sx-skip.log`; CSV suffixes `-s0.csv` and `-s1.csv`.

| Observation | HB | NoHB long wait |
|---|---:|---:|
| After600 ticksW: speed(m/s) | .372869551 | .372869581 |
| Same: stabilization z | 1.14291179 | 1.14291155 |
| AfterWoff60/repressW300: speed(m/s) | .509792328 | .509792566 |
| Same: stabilization z | .859196484 | .859196246 |
| After explicitreset/W600: speed(m/s) | 29.301878 | 29.3018684 |
| Same: stabilization z | 0 | 0 |

Both bog snapshots remain800rpm/firstgear/fullthrottle, rearlocks0; reset sample4762rpm/thirdgear. Thus accelerator release/repress does not restore ordinary acceleration in this controlled case, while reset does. The paired noHB result demonstrates the symptom is also reachable from the same sustained idle period without handbraking. It does not establish that every user incident has this cause.

At HB elapsed600, logged momentumPx=.45492059, preparedFx=4.20012712kN and z=1.14291179. Approved S2-6 separate stabilization term magnitude8*z*Px=4.15947285kN, close to prepared propulsion. This is an arithmetic attribution to the specified damping channel, not a sampled exact derivative or a claim of a tyre-force deficit. Engine/clutch/gear/control/brake/lock/contact outputs are ordinary at this boundary. The approved contract explicitly includes this force and an externally supplied stabilization gate; correct host gate selection requires a reviewed requirement before a source change.

No new retail target or original-runtime claim proposed. Candidate behavioral regression requirement for reviewer consideration: in this declared independent ordinary-drive flat-ground stock240SX fixture, record the above wait/HB, Wrepress and reset trajectories, and require explicit characterization of the supplied stabilization-gate policy. Acceptance bounds for desired launch recovery remain to be approved, rather than invented here.

Verifier independently ran the same extended HB350Z fixture (exit0): speeds40.6071m/s at600,48.4883m/s afterWrepress,40.6342m/s afterreset; z0 at allthree snapshots. This control does not exhibit the240SX idle bog. Log `/tmp/handbrake-launch-verifier/stock350z-extended.log`.
