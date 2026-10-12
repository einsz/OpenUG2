# Tachometer telemetry

Both HUD clusters now use the current RPM sample directly. The former 0.35
filter advanced once per rendered frame, so an identical gear change produced
different display lag at different rendering rates. `hud_tach_fraction` is
stateless and introduces no extra filter beyond the source telemetry.

`HudState.rpm_maximum` carries the engine's upper bound separately from its
redline. The drag drivetrain supplies the decoded limiter; the provisional
audio gearbox supplies its existing RPM cap. Callers that leave the new field
at zero retain redline as the provisional bound. Negative RPM is bounded to zero
and RPM above the maximum is bounded to that maximum, before division by the
selected display scale.

## Provenance and presentation seams

Original PC v1.2 gauge-update research establishes a direct RPM update and a
needle fraction bounded against the engine maximum. See
[nfsu2-re's gauge evidence](https://github.com/troikoss/nfsu2-re/blob/main/docs/pc-hud-freeroam.md).
This is an independently written implementation of that mathematical behavior,
with synthetic tests; no recovered source or retail-data fixture is included.

OpenUG2 still fits its visual arc from loaded art. The standard cluster uses
the maximum as its visual scale; drag keeps its existing 7000/8000/9000/10000
art scale. This change does not implement retail dial-skin selection, the exact
235-degree sweep, the redline overlay or FEng rotation. Standard audio RPM is
still provisional. It does not claim complete retail gauge equivalence.

Invalid/non-finite samples or invalid scale/bound values yield an empty fraction;
that is a defensive OpenUG2 API policy. `make hud-test` covers direct sample
changes, redline-versus-maximum separation, display scale, bounds, availability
and the legacy fallback without a GL context.
