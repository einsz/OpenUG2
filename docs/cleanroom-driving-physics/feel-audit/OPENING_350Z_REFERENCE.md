# Opening 350Z reference configuration audit

Status: reviewed findings, approved for communication only at the REVIEW.md hash.
2026-10-15. Not an implementation specification.

The comparison target is the original game's opening 350Z versus OpenUG2's 350Z. This audit does **not establish** that its effective physics configuration equals an ordinary stock 350Z, or that it differs. It establishes no upgrade tier, tuning setting, instance bonus, active handling family, or profile-dependent override for that opening instance. Those remain unknown; no community claim is substituted for evidence.

| Question | Result | Evidence |
|---|---|---|
| Does the executable match the supplied reference identity? | Yes, file hash verified. | O350-1 |
| Are the active assets proven to be the intended original asset revision? | Unknown. Active and backup candidate files differ; their origin and load precedence are unresolved. A hash difference proves neither corruption nor modification. | O350-1 |
| Is the exact opening vehicle preset and its selection chain verified? | No. Static candidate data was found, without a verified opening selection chain. | O350-2, O350-4 |
| Are opening upgrades and tuning verified? | No. | O350-3, O350-4 |
| Are instance bonuses, event handling family and profile overrides verified? | No. | O350-4 |
| Is opening-versus-stock equivalence or difference measured? | No. No original runtime capture was performed. | O350-4 |

Static inspection supports a limited general finding: the effective physics configuration can depend on configuration inputs beyond base vehicle identity. This is **inferred**, not runtime proof about the opening car. A base-record comparison alone cannot resolve this reference question. This supplement adds no numerical physics law and does not revise earlier approved specifications.

## Comparison requirements

A controlled reference comparison must identify the same executable revision, the actual loaded asset revision, profile state, event state, and effective vehicle configuration. The primary reference must be the opening instance. An ordinary stock instance is a useful additional control only after its effective configuration has been captured independently.

Record these together at the first controllable opening frame, and again after any event transition used for driving comparison:

- Fresh-profile status, assistance settings, transmission selection, input device and input mapping.
- Selected base vehicle and selected preset, performance selections, tuning values, and any instance-specific additions or overrides.
- Active event/handling family, player/control role, and any event-specific response or grip adjustments.
- Final effective mass, centre of mass and inertia; engine output and operating limits; transmission ratios and shift settings; tire/grip and steering parameters; suspension and aerodynamic parameters. Record the effective values after configuration and event composition.
- Surface/contact state, speed, gear, engine speed, and wheel speeds at each sample.

Obtain the same record for OpenUG2's 350Z, including its build identity and effective
configuration; capture matched controls, initial state, road/contact conditions and
timestamps for trajectory comparisons. Also obtain the original record for an
ordinary stock 350Z in the same original build as a separate control. Compare configuration fields before interpreting differences in acceleration, steering or high-speed stability. A difference must be reported field by field; equal base identity is insufficient evidence of equal effective configuration. Keep proprietary records and labels in analyst evidence and export only reviewed behavioral facts or the minimum required differences.

## Unknowns and priority implication

The exact opening preset, upgrade selections, tuning, instance additions, mode, profile overrides, asset provenance and resulting effective values are unresolved. No claim that the opening car is stock, upgraded, uniquely stabilized, or equivalent to an ordinary stock instance is supported here.

Resolve the opening configuration before calibrating vehicle-specific constants from the feel comparison. General physics gaps may still be investigated independently, but configuration uncertainty prevents attributing the opening-versus-current driving difference to any particular missing feature.

Evidence IDs resolve in the separate provenance record. No runnable input/output vectors are supplied because no original behavior was captured.
