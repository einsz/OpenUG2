# Reviewed final-render dialog acceptance — amendment 7

Status: normative D requirements; approval hashes in REVIEW.md. Scope is the
authored dialog launcher/modal examples in DEMO_DESIGN D2 page 8 and D3.
Every vector below is freshly authored from verifier observation of the
recreated demo and prior approved requirements. The observed failure was
invisible enabled unfocused modal labels at both acceptance sizes despite
passing text-state tests. This is no new retail-parity claim. No captures,
original assets, long original messages or raw resource bytes are included.

## R1 Approved runtime-check inputs and boundary

The user authorizes these local sources as runtime inputs:

| Input | Configuration value |
| --- | --- |
| PC shared font/UI resources | `/home/dq/Games/Need for Speed Underground 2` |
| Xbox controller source | `/home/dq/Games/Need for Speed - Underground 2 (USA) Xbox.iso` |
| PS2 controller source | `/home/dq/Games/Need for Speed - Underground 2 (USA) PS2.iso` |

Pass these paths only to the recreated implementation's existing or newly
implemented loader/render interfaces, to execute its own demo/tests and
measure its own rendered text. Reading own loader status, decoded output,
layout metrics, framebuffer readback or saved captures from that runtime is
allowed. Resource decoding remains an independently authored implementation
of approved SPEC formats. Do not use shell/file readers, custom analysis
scripts, disassemblers, original tools or other direct raw inspection to
examine the PC source, ISOs, original binaries or original resource bytes.
Do not inspect analyst notes/evidence, use them as golden art, or execute the
original PC/Xbox/PS2 game. Do not export retail assets into spec, implementation
source, committed test vectors or distributed artifacts. Local recreated
runtime captures used for acceptance remain uncommitted verification output.
Missing sources/fonts retain SPEC 20.1 unavailable behavior and cannot be
reported as a passing resource-backed render check. This permission is limited
to testing the recreated loaders and render outputs and does not relax the
clean-room information boundary.

## R2 Final-frame oracle

Run at 1280×960 and 1280×720 with SPEC 1.1 viewport mapping. For each enabled
choice record its label, focus/enabled state, label ink bounds in final-window
pixels and checked animation time. Observe the actual final framebuffer after
all drawing, modulation, filtering, blending, clipping and overlays. Pixel
readback or saved captures from that framebuffer must accompany the result.

Within each label's expected glyph bounds, identify filled interior strokes
of the loaded original raster font, away from antialiasing edges, button
outlines and pointer overlap. Every unfocused enabled label must have visible
filled character interiors distinguishable from the local button backing and
be readable at native capture size. Blank bounds, outline-only remnants,
clipped/covered text, or near-uniform bounds differing from the backing by
only one channel level fail. A border, focus frame, pointer, message text or
another choice's ink cannot satisfy this check. Report per-choice interior
pixel samples and nearby backing samples, together with the final capture and
native-size legibility assessment. A pixel difference alone is insufficient
unless it forms the readable filled glyph interiors of that choice's label.
No arbitrary contrast-ratio requirement or replacement palette is introduced.

A software mirror of tint/alpha, a primitive list, enabled flag, predicted
text width, isolated synthetic atlas, mocked renderer or text-state test is
supplementary only; it cannot replace this actual final-frame check using the
loaded original font and button art. Tests should be able to fail when final
label ink disappears even if those state assertions pass.

Preserve SPEC 5's 1200 ms focused pulse: alpha zero at 0/1200 ms, fully opaque
at 440/760 ms. At rest sample after dialog entrance completes, with focused
label in its opaque interval. For each focus destination capture pulse time
0, 440, 760 and 1200 ms (before wrap or equivalently the next cycle's 0).
Both opaque interval endpoints require readable filled focused glyph ink;
unfocused labels require readable filled ink in every captured frame. Only
the focused label's exact zero-alpha phases are exempt from filled-ink
visibility, and record that expected absence explicitly. Focus loss cannot
leave a previously focused label transparent. These pulse samples preserve
authentic timing; do not force constant focused alpha to make the test pass.
Capture the immediate focus-change frame as well if it differs from the
pulse-start frame; apply the same focused-only exemption. All enabled choices
must therefore be tested with actual visible ink in their focused opaque
phases and in every unfocused rest/focus-transition endpoint.

Keep genuine original font/art, unfocused grey/palette/alpha semantics in
SPEC 6.8/6.9, approved geometry and genuine corner transforms. No generated
font, brightened replacement art, disabled styling for enabled choices or
unrelated asset substitution is permitted.

## R3 Acceptance vectors

Provenance of each R3 vector: verifier observation of recreated authored dialog
variants at both sizes, with expectation refined from DEMO_DESIGN page 8/D3
and SPEC 5/6.9/20.1. Exact short choice labels identify independent demo
reproduction inputs (D), not recovered symbols or copied original messages.
Variant numbers are demo inputs. Modal message/title content need not be copied
from any retail source. Panel size/defaults follow existing approved facts;
three-choice placement/default remains D configuration.

| Vector | Demo variant | Enabled choices, in visual order | Focus coverage |
| --- | --- | --- | --- |
| R3-0 | 0, one choice | the displayed acknowledgment choice | its only focus destination |
| R3-1 | 1, two choices | Yes, No | initial right choice; then each choice in turn |
| R3-2 | 2, three choices | Save, Discard, Cancel | initial D default; then every choice in turn |
| R3-3 | 3, small two choices | Yes, No | initial right choice; then each choice in turn |
| R3-4 | 4, large two choices | Accept, Decline | initial right choice; then each choice in turn |

For every row, every acceptance size, and selected PC, PS2 and Xbox profile
with its supplied source, open the modal from page 8 and apply R2 at rest and
for every focus destination, including returning to the initial choice.
Inspect all enabled labels in each frame, not merely the selected one or the
first failing label. Keep the pointer clear of labels. Use directional focus
movement and verify pointer focus/activation as a separate action sequence.
Each row has expected result: all applicable R2 ink checks pass with labels
inside their own buttons/panel, without overlaps. Two-choice default is right.
If a displayed choice name changes as a D configuration, record the actual
label and retain the same all-choice coverage; no untested replacement case
may silently replace an existing variant.

For R3-1 specifically, the left label must remain visibly filled while the
right choice is focused, then the right label must remain visibly filled
when focus moves left. For R3-2, every pair of unfocused choices remains
visibly filled while the third is focused, including Save when Discard or
Cancel is focused. These cases detect a result where only focused text draws.

Close/reopen each variant to test click/Accept and Back according to existing
bindings. Underlying controls must not activate; default PC strip/pills are
hidden during the modal and restore correctly afterward. This preserves the
existing interaction acceptance and does not invent retail application results.
Dialog launcher enabled unfocused labels on page 8 also require R2 final-frame
filled-ink acceptance at rest and after focus changes at both sizes; preserve
the launcher's own approved focus treatment rather than imposing modal timing.

## R4 Evidence and limits

Acceptance output identifies window size, actual selected profile, resources
successfully loaded, variant, all choices, focus destination, animation sample,
per-choice final-frame result and capture. Unknown original runtime timing,
retail console action mapping, exact console glyph baseline/size and other
SPEC 11/20.5/XBOX_CONTROLLER unknowns remain unchanged. No original executable
execution or general retail rendering-parity proof is authorized by this check.
The observed defect's internal cause is unspecified. All other DEMO_DESIGN,
Xbox source/decoder/gallery, PC pills and original-only visual requirements
remain required; this refinement neither replaces nor proves those checks.
