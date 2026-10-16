# Native service-brake audit input

The existing independent `--drive-audit` host script now requests S with zero
steer for ticks 300 through 344 when the Stage-1/2/3 drivetrain is active. The
script otherwise retains its idle, straight acceleration, steering, late S and
coast intervals. The early interval replaces 45 ticks of straight acceleration;
acceleration resumes at tick 345. Legacy audits and interactive input are unchanged.

This is a diagnostic input choice, not an additional physics requirement or
retail timing claim. S3-2 supplies the simulation-facing pedal contract and
S3-5/S3-7 require service braking to reach body motion through normal wheel forces
and the existing coupling lifecycle. The audit uses the existing host S mapping;
it does not write body/wheel state, change contact/load rules, or bypass physics.
The native baseline on STREAML4RA with stock 240SX reached 42.31 km/h at tick 300,
with four contacts and no wall/rail responses through tick 329, motivating an
early interval before turns and later possible obstacles.

Existing 30-tick STAGE1/STAGE3 telemetry samples ticks 300 and 330 during the
request and tick 360 after release. A 420-frame native run includes these phases
and the normal final capture. This checks diagnostic integration; neither native
host motion nor synthetic arithmetic vectors establish original-game runtime
equivalence or a normative stopping distance (S3-9 U3-5, S3-10).

The approved Stage3 specification hash checked before this work was
`d4689a16861df810159760a6a668ef3714966fd6c20a30e3b2b54f355c41c6f7`.
No behavioral specification, physics implementation or numerical test changed.

## Native check

`make debug` passed; existing warnings were emitted at unrelated `main.c` sites.
The resulting debug executable was retained. This finite command exited zero:

```sh
DISPLAY=:0 SDL_AUDIODRIVER=dummy ./nfsu2 \
  '/home/dq/Games/Need for Speed Underground 2' \
  --car 240SX --drivetrain --track STREAML4RA --resolution 320x180 \
  --drive-audit /tmp/openug2_stage3_brake_audit --frames 420 \
  > /tmp/openug2_stage3_brake_audit.log 2>&1
```

At ticks 300 and 330, STAGE3 reported pedal 1, requested front/rear per-wheel
torques `(1.804200,2.075800)` kN·m and negative longitudinal forces. Body speed
fell from 11.505 to 4.693 m/s; corresponding front/rear angular speeds fell from
`(36.663,36.480)` to `(14.919,14.747)` rad/s. At tick 360 the pedal and requested
torques were zero and body speed was 1.949 m/s; at tick 390 body speed recovered
to 3.288 m/s. These samples retained gear 1, contact mask `f` and locks `0000`.
The run reported zero wall/rail responses and zero NaN/position-bound flags,
then wrote `/tmp/openug2_stage3_brake_audit_end.png`.

These results show moving service-brake input, wheel/body response and release
on this independently simulated route. They do not isolate a measured retail
stopping curve, establish lock behavior in this run, or replace the approved
synthetic tests. No unchanged numerical suite was rerun for this input-only delta.
