# Widescreen HUD fitment

OpenUG2 project policy, not original behaviour. Specification: "Project policy:
widescreen HUD fitment (not original behaviour)" in
[cleanroom-hud/SPEC.md](cleanroom-hud/SPEC.md); synthetic vectors:
`cleanroom-hud/test-vectors/wide-policy.json`.

## Option

```sh
./nfsu2 DATA --hud                 # wide (default)
./nfsu2 DATA --hud --hud-fit wide
./nfsu2 DATA --hud --hud-fit 4:3   # also accepted: 4x3, 43
```

`--hud-fit` also enables the HUD. An unrecognised value is a usage error (exit
status 2). Programmatically it is `HudSettings.fit_policy` (`HA_FIT_WIDE`, the
zero value and default, or `HA_FIT_43`) applied with `hud_configure`.

## Behaviour

| | `4:3` | `wide` (default) |
|---|---|---|
| scale | `s = min(W/640, H/480)` | same (`H/480` when `W/H > 4/3`) |
| frame | 640x480 logical frame centred | same |
| groups | none moved | left groups `-D`, right groups `+D`, centred groups stay |

`D = (W/s - 640)/2` logical units. At 1920x1080 `s = 2.25`, `D = 106.667`; at
2560x1080 `D = 248.889`; at 640x480 and 1440x1080 `D = 0` so the policies are
identical, and `wide` is identical to `4:3` for any `W/H <= 4/3`. Final output
of a logical point `(x, y)` in a group translated by `t`:
`X = W/2 + s*(x+t)`, `Y = H/2 + s*y`.

A top-level group is classified by the horizontal centre `cx` of its composed
logical bounds: `cx < -640/6` left, `cx > 640/6` right, otherwise (including
exactly +-640/6) not moved. Descendants always move with their group.

## Design decisions (traced to the spec section above)

* Pure arithmetic lives in `src/hud_asset.c` (`ha_fit_scale`, `ha_fit_anchor`,
  `ha_fit_translation`, `ha_fit_point`, `ha_fit_parse`) so it is testable
  without GL; the existing `ha_fit` is unchanged. (Policies; final-position formula.)
* The package HUD classifies each top-level group once at `hud_init`
  (`fit_classify` in `src/hud.c`): bounds are the axis-aligned bounds of the
  composed authored rectangles of the elements the standard path can draw
  (gauge needle/dial/scale/redline/fill, map road/mask/north/player/border,
  Bank, top-left widget, shift/NOS/glow images) at rest (needle at zero RPM, no
  animation). Text contributes its anchor point, because its extent depends on
  the live string. Authoring sample items and event widgets are excluded; a
  group with no eligible geometry is not moved. (Bounds paragraph.)
  Eligibility is static: opt-in elements such as the redline are counted
  whether or not the option is on, so classification cannot change at run time.
* The translation (which depends on the window size) is applied per frame to
  every vertex of a node in its group, to the dial centre used by the redline and
  NOS arc clipping, and to the map-road mask coordinates, so the road texture
  stays registered to its mask. The player indicator follows the map's group.
  (Final transform; descendants move with the group.)
* Engine-drawn items (`src/hud_baseline.c`: race minimap, tach, NOS bar, drag
  gauge, status, message widget) are placed with `hud_baseline_place_fit`: the
  anchor is first resolved in the centred 4:3 frame, the item's centre is
  classified with the same rule, then moved by `D`. Under `wide` this equals the
  existing screen-edge anchoring; under `4:3` it hangs off the frame edges.
  Centred items (navigation arrow, drift banner, preview label) never move.
  (Engine-drawn items follow the same rule.) The legacy `hud_baseline_place`,
  `hud_place` and `hud_map_pick` are unchanged and keep the default (`wide`) result.
* Picking inverts the final transform. Package map: the pick frame centre
  includes the group translation (and so does the mask lookup). Race map:
  `hud_baseline_map_pick_fit` uses the same placement as the draw; it also now
  uses the draw's map window (`map_radius_m`), which the previous pick ignored.
  (Interaction paragraph.)
* The Bank value/label and the top-left widget are classified by their own
  groups' composed bounds like everything else; nothing is special-cased.

## Checks

`make hud-test` (engine-side placement and pick round trips, defaults, option
parsing) and `make hud-spec-test` (the 25 `wide-policy.json` vectors plus
identity, threshold and validity checks against the spec arithmetic). A manual
smoke run with retail data through the project's own loaders:

```sh
./nfsu2 DATA --hud --hud-preview --hud-fit wide --resolution 1920x1080 --shot wide.png
./nfsu2 DATA --hud --hud-preview --hud-fit 4:3  --resolution 1920x1080 --shot c43.png
```

## Known limits

Unknowns recorded in the spec remain: portrait/ultra-wide extremes beyond the
arithmetic, groups straddling the +-640/6 thresholds, and any user-facing HUD
scale option (none exists). Text bounds use the anchor point only (see
`docs/cleanroom-hud/QUESTIONS.md`).
