# Reviewed round 4 verification cases — 2026-10-13

Approved requirements: SPEC 20.6, subject to the hashes recorded in REVIEW.md.
These are demo presentation/tooling expectations, not new retail observations.
No original executable was run. Provenance for every case: prior approved
SPEC 20.3 policy and verifier observations of recreated demo captures/test
output, recorded in analysis/evidence/round4_verification.md. That evidence
remains outside implementer inputs. No original artwork or image bytes are
included here. Command-line options below identify the independent demo's
public test inputs, not original executable symbols.

## V4-1 Current strip expectation supersedes historical PC test

Input: shared UI resources from the user-supplied PC install; a menu using the
bottom hint strip, or that menu in the original-screen viewer; PS2 or Xbox
profile explicitly selected; no PS2 ISO. Amendment 5 makes PC text-only the
default; this case remains a console-profile test.
Expected: console hint family selected, exactly one console strip, PC help
overlay absent, unavailable glyphs omitted, action dispatcher retained.
Repeat with the PS2 source supplied: the same strip/overlay expectations apply;
only verified, explicitly mapped controller glyphs become available.

Observed discrepancy: historical test expectations demanded PC hints and an
absent console strip. These conflict with 20.3 and are superseded. A recreated
main-menu capture showed one console strip and no PC pills. This does not
change historical retail PC observations in 7.4.1 and 19.

## V4-2 Family label agrees with selected family

Input: recreated demo with `--controller xbox --page 11`, shared original PC
resources and no verified Xbox source.
Expected: unavailable Xbox status, no PS2 controller imagery, and every
family-selection label identifies Xbox.
Observed discrepancy: unavailable status and omitted icons matched, but a
lower selection label identified PS2.

Input: same page with `--controller ps2` and the verified owned PS2 source.
Expected: all 19 verified icons in the showcase and PS2 family-selection labels.
Observed: matched. Neither case specifies an unobserved retail action map.

## V4-3 Showcase explanatory text remains legible

Input: demo page 11 at 1280×960 with the verified PS2 source.
Expected: selection label and F2/configuration explanatory text have disjoint
vertical ink extents (one bottom edge at or above the other's top edge), and
neither text is covered by the other.
Observed discrepancy: selection label overlapped the explanatory text.
This concerns demo configuration tooling, not retail console layout or baseline.

## V4-4 Deliberate PC overlay suppression is expected in the tour

Input: all-screen original-resource verification tour using the console profile,
including the historical PC help-bar overlay package.
Expected: the overlay is reported as intentionally suppressed and excluded from
unexpected load failures; suppression alone produces no failure exit status.
Actual unsupported or missing resources and GL errors are separately reported
and checked.
Observed discrepancy: deliberate suppression was counted as a load failure and
caused failure exit status, while the tour reported zero GL errors. Section
20.3 requires suppression; no claim of universal screen fidelity is made.
