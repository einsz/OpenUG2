# Amendment 6 — authored demo composition and Xbox source

Inputs were only reviewed specification and permitted independent baseline.
SHA256 gate matched REVIEW.md before implementation:

- SPEC.md `76125569604ff34be9a0b72f981d200b2ebdb3326af3228205c787d8e507f773`
- AMENDMENTS.md `96113c34530762775e5f07e70d7b159e3477db993d5a3b4b558c60246c289b40`
- DEMO_DESIGN.md `861f25cb31c01c7a82427bc24f8783c19a5154bd63ed9a7779389d3ab9aa1cef`
- XBOX_CONTROLLER.md `551417742c4e54876e463accaec9907cf18dff7041bd0181087dff0f7f74b776`

This round supersedes historical Xbox-unavailable and animation-gallery pill
suppression decisions below, while retaining PC text-only default/pills/modal
hiding, exact authored retail-screen geometry, original-only assets and full
projected-corner transforms. No original executable, data, analyst material,
forbidden handoff, quarantine or recovered source was accessed. Runtime source
loading/captures are delegated to an independent verifier. No retail assets
were copied into source, tests or documentation.

| Reviewed requirement | Independent design |
| --- | --- |
| D1 | Shared authored title header uses genuine panel art and original-font title helper (scale .8, measured ink anchor). Main-menu carousel retains its own right title/bands. Runtime viewers retain authored composition. |
| D1.4–7 | Short default captions, light-panel dark text and explicit `--diagnostics`/F3 view. Cues, resource paths, hashes, durations and counters stay in diagnostics/terminal audit. PC chrome remains the original one strip/pills; source loading never selects console profile. |
| D2 page 0 | Aligned filename columns inside a light body and dark category header. Scroller moved inward, with fully bounded hit/silhouette extents at both endpoints. Metadata is confined to footer diagnostics. |
| D2 page 1 | Grouped genuine panel/corner samples, rotating original box, geometry gradient, eight justification samples, wrapping and localized labels. Controller family guidance is separate from sample ink. |
| D2 page 2 | Mixed Settings title matches the displayed categories; existing .7-scale rows, 36 pitch, disabled state, original slider pieces and focused value arrows retained. Family selection updates labels and chrome. |
| D2 page 3 | Meter labels above fills; category and slider groups separated; 12 swatches retain 42 pitch and original box/corner surfaces. Selected grid uses genuine focus art. Scroller moved inward. Action button now posts an observable result; unavailable movie is explicit. |
| D2 page 4 | Coherent light keyboard panel; entry above keys with independent bounds, unchanged explicit key links, clickable/physical typing and command row. |
| D2 page 5 | Light date panel retains aligned field values, field-specific arrows/captions, valid existing update rules and visible acceptance result. |
| D2 page 6 | Light rankings body, dark header, aligned columns and dark selected backing; rank marker remains clear of rank ink. Original ten-row geometry and selection/scroller retained. |
| D2 page 7 | Genuine header and lower stat bands; authored title/filter/scroller, frame/badge slot, stats and end-arrow suppression retained. Manufacturer text no longer pretends to be a logo; omitted preview/manufacturer/badge categories are explicit. No unreviewed dynamic asset keys are guessed. |
| D2 page 8 / D3 | Launchers grouped in light body; modal geometry/default focus/interaction/pulse retained. Enabled dialog labels sample the original atlas alpha as filled glyph coverage (`UG_M_TEXT`), with opaque approved grey/base colour and unchanged focused pulse overlay. This is a D rendering choice, not a claim about original raster mechanisms; ordinary image modulation/retail viewer rendering is unchanged. |
| D2 page 9 | Two-column state selector plus one genuine preview, readable caption, disabled sample identification. Existing scripts/timing retained. Duration/time values appear only in diagnostics. Authentic PC pill focus/glow examples restored. |
| D2 page 10 | Genuine original-font/cap-art example button shares the page with a separately inspected synthetic package. Button is pointer reachable, posts once and produces visible result; diagnostics retain resource/event inspection. Invisible synthetic resource identifiers never serve as promised art. |
| D2 page 11 / X5 | Family gallery shows native verified art, separate descriptive captions, available subset count and consistent selector/source status. PC explains loaded shared resources and active text pills. PS2/Xbox absence cannot substitute another family. |
| D2 page 12 | Reviewed controls band/seven-row 36-pitch hierarchy retained, with Action/Binding headings (no primary/secondary parity claim). Existing PC reset/back/quit assignments and reset action remain. |
| X1–X4 | Bounded XDVDFS member traversal, narrow 60-byte descriptor Xbox pack validation, BC1/BC2 decoding and base 23-key gallery are documented in DESIGN_XBOX.md. Explicit family-specific mappings are demo configuration only. |
| D3 | `--demo-check` captures all thirteen authored pages plus browser/car endpoints, keyboard typing/click, date acceptance, package result, table/binding changes, family cycling and modal rest/focus endpoints/chrome restoration. `--check` additionally tours runtime original packages. |

Final normal build passes 7,271 checks and the approved PS2 hash; demo compilation
succeeds without warnings. Integrated ASan/UBSan passes 7,269 checks with leak
detection disabled because LeakSanitizer reports unsupported ptrace execution.

Tests were encoded before implementation: initial demo tests failed to compile
on missing diagnostic/example fields, then caught the browser margin. Xbox
synthetic vectors likewise preceded implementation. Final commands/results are
recorded in HANDOFF.md. Existing scoped input/codec/viewport/transform checks are
retained; synthetic checks cannot establish runtime visual fidelity. No new
questions were needed; approved unknown console bindings/baseline, dynamic car
art keys, unrelated formats and genuine 3D/media remain unavailable or explicitly
configured. No original-runtime parity claim is made.

---

# Amendment 5 — current PC text-only restoration

Approved inputs matched REVIEW.md before work:
- SPEC.md: `6a1fc9b2750614bd2d030e732cbbb0f1a76ce7204dbda7ec9b96a12b3e766fec`
- AMENDMENTS.md: `7477298e5c08766e5df42d22872c3177255ee79cc9e5892b7e0e88c729d18f58`
- RESTORATION_PC_PILLS.md: `b4f8f9cda6b6ee9d4df994240e7a4cf92ce9a20a947aa61127ca4b314f1e3be3`
- VERIFICATION_ROUND4.md: `fafe2978cbb647c5f4d3ba06cb236276be43471509e23e815e98ac858ae42d58`
- original_visual_policy_vectors.json: `a34909997ce841a73cf6068872c74f4160245f945bf6bfff04755f303e68e42e`

This round supersedes the historical console-only presentation decisions below
for the PC profile. Existing console, resource, codec and corner-transform
policies remain applicable. Inputs were only the approved spec and permitted
independent baseline, with src/gl_state.[ch] additionally permitted as build
dependencies. No original data, executable, analyst evidence or notes were read.

| Spec requirement | Independent design decision |
| --- | --- |
| 20.7; restoration cases 1, 5 | Add PC to the existing profile enum, retain PS2/Xbox values, select PC by default. Resource availability does not select a profile. CLI, F2 and configuration labels select PC → PS2 → Xbox → PC consistently. |
| 7.4/7.4.1; cases 1–2 | Reuse existing slot-assignment/strip-geometry helpers. Draw six original corner/box texture pieces, runtime pill/glow quads at 144×24 and original-font labels at scale 0.45 and y minus 2. No PC glyph or Help pill is rendered. |
| 5, 7.4, 20.7; case 3 | Restore focus/glow animations on pointer entry. Primary down dispatches the bound action once; primary up does not dispatch. Hit targets use the specified slot centers and only visible slots. Existing application bindings handle Accept, Back, Quit and controls reset. |
| 6.9, 20.7; case 4 | Retain stack-based modal strip hiding and prevent hidden-bar pointer activation. Underlying slot assignments return when the dialog closes. Dialog button rendering is untouched. |
| 20.3, 20.7; cases 5–6 | PC original-screen menus hide console strips/hint labels and draw the PC overlay once. Explicit console profiles retain authored strip ownership. The help-bar package is eligible on PC and suppressed only on console; changing profile reloads that package when required. |
| 20.1/20.2; case 7 | Pill/corner/font absence reports canvas UNAVAILABLE and never synthesizes a replacement. Existing full-corner projection and PS2 codec tests remain. |
| 20.7 navigation localization | Approved English baseline strings and caller PC-label overrides are restored. Original navigation string-table lookup keys are unspecified; Q32 records that automatic localization remains unimplemented. No keys are guessed. |

Tests were added before implementation; the first build failed on the missing
PC profile. Reconciled historical console assertions by explicitly selecting
console profiles, and re-enabled PC quit/reset/back/modal flows. The final
GL-free suite passes **7048 checks, zero failures**, including every existing
synthetic vector and the full decoded PS2 RGBA SHA256. Original-install tests
were disabled with UGUI_DATA/NFSU2_DATA unset. Demo compilation passes after
linking the permitted independent GL-state dependency. Runtime original-resource
captures belong to the verifier. Commands and capture options are recorded in
HANDOFF.md beside this implementation.

---

# ugui design decisions — amendment 4 and verification clarification

Approved inputs verified before implementation:
- `cleanroom/spec/SPEC.md`: `c5fa8e37e527759bcde30baf0389767a6d63d8f0d29b4701602279cac2b13c34`
- `cleanroom/spec/AMENDMENTS.md`: `d7d113dd93b36ad310b06d3cfcb56f5517df21cfda68790e81872c3668de406b`
- `cleanroom/spec/VERIFICATION_ROUND4.md`: `f211dcf52b224234cb262954e7f36276fbe7344fa04ae1af8b5c5e2cac1893d7`

The coordinator permitted the existing independent `src/ugui/`, `tools/ugui_*`
implementation and build configuration. This fresh implementer used those and
reviewed synthetic vectors. No original executable, original data inspection,
analyst material, disassembly, quarantine, or original software source was read.
No runtime original data was accessed in this round; the coordinator/verifier
will supply those paths to the loader. Tests construct synthetic resources only.

## Architecture and specification coverage

| Specification | Independent implementation |
| --- | --- |
| 1–2, 3–4 | GL-free color/hash helpers, package parser, message queues, scene/player state and widget input remain in their existing modules. |
| 5–6 | Widgets retain specified geometry and timings, but visual resource absence no longer permits generated art. PC pill animation gallery entries are suppressed. |
| 7–8, 16–17 | Font chunks, kerning, localization and text layout remain. Only original atlas quads render text. The corrected synthetic multiline font vector is compared directly, without the old +1 adjustment. |
| 13–15 | Existing independently implemented compression, resource bundles and PC texture decoders remain. Backup resource candidates are preferred; every texture records its supplying runtime file. A missing named font returns NULL instead of a different font. |
| 20.1 | Generated bitmap-font designs and mouse/key rasterizers removed. Kit fallback branches, generated cursor, car silhouette/logo, movie placeholder and decorative background removed. Unsupported spinner, generic symbol/lock and analytic quarter-disc APIs omit art. PC pill rendering is disabled. Shared font/kit absence prevents faithful demo drawing and causes a plain terminal diagnostic and exit. |
| 20.2 | Scene nodes keep row-major 3D affine matrices. Local quaternion rotation includes x/y/z, parent composition occurs in 3D, and projection occurs last. Signed image dimensions stay on geometry; UV endpoints are never swapped for that reflection. Kit panel corners use native bottom-right art, with signed geometry for the other corners. |
| 20.3 | A single console strip replaces all PC overlay modes; original viewer strips retain authored player, label/font and token placement. No PC pill hit targets remain. Mouse/keyboard dispatch remains independent. F2 and CLI explicitly select PS2/Xbox; Xbox never substitutes PS2 images. |
| 20.4 | `ugui_controller.[ch]` resolves ISO9660 directory records, bounds the selected packed file, validates candidate chunk extents, info/path/version, table/key/descriptor strides and all 19 icon records, then decodes their narrow swizzle and RGBA palette. Unsupported records reject the whole family before any icons are published. |
| 20.5 | Xbox remains unavailable. PS2 action bindings have no defaults: CLI/API mappings are explicitly demo configuration. Unknown bindings omit glyphs and return UNBOUND. Unsupported console formats remain unimplemented. |
| 20.6 / V4-1 | Historical runtime tests now require console strip selection, authored strip visibility and suppression of a duplicate library background. Synthetic drawing checks retain all four strip pieces when controller resources are unavailable. |
| 20.6 / V4-2 | `demo_select_controller` updates the controller profile and both configuration labels; CLI selection and F2 both use it. The labels identify family independently of availability status. |
| 20.6 / V4-3 | The showcase selection row is at virtual y 126 and its configuration explanation stays at y 155. Synthetic full-cell font metrics test their disjoint projected ink extents at 1280×960 and the 19 original icon slots. These are tooling positions, not retail measurements. |
| 20.6 / V4-4 | A viewer stores deliberate suppression separately from loaded state; every load resets its status and diagnostic counts. The tour reports suppressed packages independently, excludes them from load failures, and keeps missing-texture failures and GL errors effective. |

## Visual-resource policy

Runtime texture provenance includes lookup key, supplying path, format and
size. `--audit` prints this metadata and original font atlas keys. The sources
are user-supplied files; filenames cannot establish provenance for arbitrary
imports. No raw-image import feature is provided. The PS2 path is retained as
`ISO:NFSUNDER/ZZDATA1.BIN`; pack bytes are released after decoding, and textures
exist only in memory. No original assets are embedded, copied into vectors or
packaged with source.

Kit roles resolve through the resource identifiers already defined by the
approved baseline/spec: boxes, native panel corners, button/strip caps, arrows,
scroller pieces, slider pieces, icon highlights/backing, save icon and cursor.
The resource loader populates role indices only when decoding succeeds. An
unresolved request never picks a different texture or font. A canvas exposes
`ug_cv_visual_status`; missing image/font paths report UNAVAILABLE while
specified untextured geometry can still render. Explicit controller lookup
reports OK, UNAVAILABLE or UNBOUND separately.

The GL backend's single white texel supports independently implemented flat
geometry/blend operations; it contains no artwork or replacement font. Rounded
panel fills use original box/corner textures, not analytic circle masks. Lines,
flat rectangles, gradients, clipping and text positioning are geometry allowed
by 20.1. No invented 3D/car/movie image is generated.

Console captures have not established glyph sizing/baselines or retail action
bindings (20.5). The icon showcase uses each verified image's native 32×32 size
and descriptive demo labels. Token rendering retains the spec's existing text
layout and the library strip's existing layout as demo configuration; these are
not claimed as console capture measurements. Runtime original scene transforms
are preserved. No guessed retail binding is installed.

## Tests and review boundaries

The new vectors were encoded as tests before implementation: initial build
failed on the absent controller API. Corner tests compare all four projected
vertices and their fixed UV association for all eight cases. PS2 tests compare
all 1024 indices through their palette RGBA, seven address/sample vectors,
alpha conversion and full decoded SHA256. Synthetic pack and ISO tests cover
successful loading, explicit mapping, unavailable Xbox, format/offset/stride
corruption and a candidate exceeding its file's declared extent. Policy tests
cover missing art/font, absent PC pill targets and Back mouse dispatch.

Historical tests asserting generated fonts/art or PC pills were rewritten or
retired where section 20 supersedes their presentation. Input, geometry,
script, format and earlier reviewed vectors remain tested. Runtime-original
comparisons and screenshots belong to the independent verifier, not this
implementer. No new specification question was needed; 20.5's accepted unknowns
remain explicitly unavailable or configured without retail claims.

The clarification regressions were written first: the initial build failed on
the absent selection helper and suppression field. After implementation, one
test incorrectly expected UNAVAILABLE for an action with no binding; it was
corrected to require no successful lookup/no image, preserving the approved
UNBOUND contract. All 6894 checks pass. Original-data checks were explicitly
disabled through unset UGUI_DATA/NFSU2_DATA; runtime visual verification remains
the verifier's responsibility. No QUESTIONS.md entries were added.

Public ISO reference: ECMA-119, volume and directory record layouts:
https://ecma-international.org/wp-content/uploads/ECMA-119_6th_edition_december_2025.pdf
