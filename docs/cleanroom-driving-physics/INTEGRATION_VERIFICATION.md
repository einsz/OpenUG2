# Independent gameplay integration verification — 2026-10-15

Result: PASS for the tested synthetic integration contracts; no behavioral
amendment proposed. This does not establish retail parity.

Fresh verifier role: `drivetrain_integration_verifier`. Approved SPEC.md SHA-256
independently checked:
`61f14ebb5c850fa8aa414ee20baae001fb64d4fff692e9a7d6698dbabce399bb`.
Branch `driving-physics`, HEAD `520c3f51f53ba18d83bc03ff3bb866f08fb69e8f`,
with uncommitted integration work. No implementation or approved spec edits.
No original executable, disassembly, analyst evidence, or Wine inspected/run.

## Executed checks

`make drivetrain-test drive-stage1-test physics-clock-test district-collision-test`
rebuilt each focused executable and returned 0. Verbatim result lines:

```text
PASS: 3606 checks, 0 failed
Stage 1 gameplay adapter tests passed
physics_clock_test: PASS (synthetic OpenUG2 contracts; no retail equivalence claim)
district_collision_test: PASS
```

Independent `edge_checks.c` reuses the synthetic car fixture only and adds its
own behavioral assertions. Both normal and ASan/UBSan runs returned 0:

```text
Independent integration checks: 20066 passed
```

Checks cover every 4-wheel contact mask in both directions, exact single host
surface-drag application with shaft/grip forces disconnected, exact once-only
5.5 m/s² handbrake loss, preserved airborne momentum despite deliberately stale
coupling forces, rejection of invalid replacement without state mutation, and
20,000 deterministic mixed contact/control ticks with finite engine/wheel/body
states. The project suite additionally exercises automatic shifts, forward and
reverse, braking, hold, reset, steering, FWD/AWD/RWD, collision impulses, stock
field mapping/errors and equal tick trajectories at 30/60/90/144/240 render Hz.

Strict compilation uses C99, Wall/Wextra/Werror, the project's existing
unused-static-parser suppression, and suppression of existing misleading
indentation warnings. An initial attempt without the unused-function suppression
failed on the shared static parser declarations; this is a compile-policy
limitation, not a runtime divergence. No claim of warning-free pedantic full-app
compilation. LeakSanitizer disabled; ASan and UBSan reported no findings.

Reproduction script and full project/edge/sanitizer logs are in git-ignored
`cleanroom/verification/integration/`. Its fixture contains no retail data.

## Integration assessment

Inspection of the independent adapter and orchestration supports these findings:

- Existing 60 Hz clock supplies one frame selection and four 1/240 s integration
  and coupling steps. Host metres/tick convert to metres/second and back; tonne,
  kN and kN·m quantities agree with the approved dimensional convention.
- Body advancement consumes the preceding coupling forces. Current contact and
  lock flags gate stale force authority. Contact loads are static quarter weight,
  held through the tick, as explicitly documented.
- Stage-1 chassis path skips legacy propulsion, coast braking and surface drag;
  adapter supplies drag once. Service braking is an explicit host approximation;
  locked-wheel force is omitted from body summation during host handbraking.
- Default selector is zero-initialized and requires `--drivetrain`. Legacy path
  remains selected otherwise; drag/drift fallback and disabled Stage-1 nitrous
  are explicit. No original timing or lateral/brake solver fidelity implied.
- Reset/spawn/restart paths clear transient drivetrain state; replacement validates
  tuning before committing the car. Unsupported replacements preserve the car.
  Mode transitions reseed wheels from body velocity. Pause supplies zero ticks
  and resume resets the host clock, discarding paused elapsed time.
- HUD gear/RPM/shift thresholds and audio target RPM/load share drivetrain state;
  wheel diagnostics use integrated spin. This is wiring inspection, not an audio
  perceptual evaluation. Cosmetic wheel scaling does not alter stock radii.

## Limits

Retail differential testing remains unperformed. All approved trajectories are
formula-derived. Original residual timing, general rear grip, lateral response,
weight transfer and original brake/ABS behavior remain unresolved/deferred.
Interactive pause/resume, car switching, race-mode transitions and every event
were inspected in orchestration but not driven through live keyboard/UI tests.
The pause synthetic assertion checks zero-tick immobility, not the live menu.
No full desktop/GLES rebuild repeated; those builds are implementer-reported.
Native asset smoke outcome is recorded below. Counsel determines legal sufficiency.

## Native smoke

On Linux x86_64 (GCC 15.2.0), existing native debug executable:

```sh
SDL_AUDIODRIVER=dummy timeout 55s ./nfsu2 '/home/dq/Games/Need for Speed Underground 2' --car 240SX --drivetrain --hud --track STREAML4RA --resolution 640x360 --drive-audit /tmp/openug2-verifier-stage1 --frames 120
```

Exited 0 within the bound. Stage 1 activation and contact mask `f` appeared;
summary: `DA SUMMARY peak=1.70 km/h final=1.70 km/h travelled=0.37 m`.
The 120-tick early audit segment showed idle RPM and first selection; it is
not an independent native launch/upshift verification. Endpoint PNG visually
inspected: rendered world, player car, minimap and tachometer with first gear
and approximately 1 km/h. HUD logged unsupported/failed texture decode; complete
art fidelity is not established. Dummy audio cannot establish audible quality.
The existing executable was not rebuilt or replaced. Capture and native asset
log remain exclusively under `/tmp`, with no asset material copied to docs.
