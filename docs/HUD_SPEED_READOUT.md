# Player HUD speed readout

The standard and drag clusters now display the current speed magnitude with
integer truncation. They share `hud_speed_value`; reversing does not make the
speed negative. There is no extra render-frame filter or round-to-nearest step.

`HudState.speed_kmh` always carries km/h. `imperial_units` selects conversion to
mph for presentation only; it does not change physics, telemetry or navigation
units. `--hud-mph` enables the HUD and selects MPH; `--hud` defaults to KMH.
The mph conversion uses the unit identity 1 mile = 1.609344 km. It is not a
claim of bit-identical x87 arithmetic at every integer transition.

## Research provenance and limits

The magnitude, unit selection and truncation behavior is informed by original
PC v1.2 gauge-update research in
[nfsu2-re's HUD notes](https://github.com/troikoss/nfsu2-re/blob/main/docs/pc-hud-freeroam.md).
This implementation and its synthetic tests were written independently for
OpenUG2. No recovered code, instruction output, asset or retail-data fixture is
included. The original localization/profile setting and FEng text layout are
not implemented here; the CLI and current labels are OpenUG2 presentation seams.

NaN/infinite telemetry returns zero, and oversized finite values saturate to
`INT_MAX` to keep integer conversion defined. These are defensive API policies,
not recovered retail behavior. `make hud-test` verifies ordinary signed speeds,
truncation, conversion, stateless updates and the defensive cases without a GL
context. Original full HUD equivalence is not established.
