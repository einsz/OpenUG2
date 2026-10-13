# Implementer questions

## Widescreen policy (2026-10-13)

1. Bounds of a text element for group classification. The spec says bounds are
   those of "composed authored geometry"; a text node has a position, pivot and
   scale but its extent depends on the live string. Provisional implementation:
   a text element contributes only its composed anchor point. Please confirm or
   specify an extent (for example a nominal width).
2. Eligibility of conditional elements in the classification bounds (opt-in
   redline, NOS level arcs, shift icon, glow label, Bank, mail/highlight). The
   spec excludes "hidden or non-eligible descendants" but classification is
   "computed once per package load". Provisional implementation: every element
   the standard path can draw is eligible regardless of run-time options or
   host inputs. Please confirm.
3. `test-vectors/README.md` changed with the fold-in but REVIEW.md records no new
   hash for it (it records the old one). `tools/hud_spec_test.py` therefore no
   longer pins that file (it is documentation, not a vector). Please record its
   hash if it should be pinned.
