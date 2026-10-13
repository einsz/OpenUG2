# Amendment 6 implementation handoff

Approved current specification: SPEC.md §20.8, DEMO_DESIGN D1–D3 and
XBOX_CONTROLLER X1–X5. Hash verification and design traceability are in
DESIGN.md and DESIGN_XBOX.md. No original inputs or analyst/verifier material
were accessed by either implementer. Original assets remain external.

## Build and synthetic checks

From the repository root:

```sh
make ugui-test ugui-demo
```

Current result: **7,271 checks, zero failures**. This includes all historical
reviewed vectors and the new artificial BC1/BC2, descriptor/member bounds,
XDVDFS valid/name/child/cycle/extent cases, subset/family/profile/binding
behavior, demo margin/once-per-click example and enabled-label ink coverage.
The exact synthetic PS2 RGBA SHA256 matches the approved vector:
`7f2865830f9937d3a4297b20410d9b78a1ba6a57a62d0b25bc62c77aef835662`.
`make ugui-test` unsets original-data environment variables. The normal test
and demo builds complete without compiler warnings. The Xbox subsystem's
isolated synthetic suite also passes ASan/UBSan and its controller module
compiles with warnings treated as errors. The integrated ASan/UBSan build
passes 7,269 checks with leak detection disabled. LeakSanitizer itself cannot
run in this ptrace environment; its initial run reported that tooling failure,
so this is not a leak-check result. Optimized and sanitizer builds can execute
different numbers of geometry-dependent checks.

## Runtime verifier commands

Only the runtime verifier supplies external owned resource paths. Set
`PC_DATA`, `PS2_DISC` and `XBOX_DISC` outside the source tree. No runtime
commands were executed against original inputs by the implementer.

```sh
mkdir -p /tmp/ugui-a6-960 /tmp/ugui-a6-720 /tmp/ugui-a6-xbox
build/ugui_demo --data "$PC_DATA" --controller pc --demo-check \
  --size 1280 960 --shots /tmp/ugui-a6-960
build/ugui_demo --data "$PC_DATA" --controller pc --demo-check \
  --size 1280 720 --shots /tmp/ugui-a6-720
build/ugui_demo --data "$PC_DATA" --ps2-iso "$PS2_DISC" \
  --xbox-iso "$XBOX_DISC" --controller pc --demo-check \
  --size 1280 960 --shots /tmp/ugui-a6-xbox
build/ugui_demo --data "$PC_DATA" --xbox-iso "$XBOX_DISC" \
  --controller xbox --page 11 --capture xbox-gallery \
  --size 1280 960 --shots /tmp/ugui-a6-xbox
```

`--demo-check` covers thirteen authored pages and D3 interaction snapshots:
first/last browser and car states, meters/action result, typing/clicked keys,
accepted date, selected rankings row, package result, PC/PS2/Xbox/PC profiles,
changed/reset controls binding, five modal size/button variants at rest and
focus endpoints, and restored chrome. It prints GL error counts; nonzero GL
errors fail the tour. `--check` additionally tours original runtime packages
and keeps suppression separate from missing/load failures. `--pillbox` checks
black clipped side bars at non-4:3 size. `--capture NAME` captures a selected
page or `--screen FILE.fng` after its transition. Output is PPM.

PC remains default when either/both console sources are supplied. F2 and the
visible selector cycle families. F3 or `--diagnostics` exposes metadata in a
separately labeled diagnostic view. `--audit` prints source/key/format/font
availability without packaging asset bytes. Gallery shows 23 verified base
Xbox keys or its validated subset. Console action bindings remain explicit
D mappings; for example `--controller xbox --bind SELECT=0x8522E0A0
--bind CANCEL=0x8522E0A1` is a user configuration, not a retail action claim.

## Material limits and follow-up boundary

Synthetic tests and successful builds do not establish runtime visual
acceptance. The separate runtime verifier must inspect all page captures,
enabled focused/unfocused dialog ink and family/source cases. Any discrepancies
return through specification review to a fresh implementer; this implementer
must not inspect verifier evidence or original captures.

Genuine 3D preview/scenery/movie rendering, unreviewed dynamic manufacturer/car
badge asset mappings, arbitrary console formats, localized Xbox glyph selection
and retail console action maps/baseline remain outside the approved behavior.
Missing categories are explicit and do not select replacement art. No new
QUESTIONS.md entries were needed.
