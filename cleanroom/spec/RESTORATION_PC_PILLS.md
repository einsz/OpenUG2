# Reviewed PC pill restoration cases — 2026-10-13

These are acceptance cases for SPEC 20.7. Provenance for every case: the user's
restoration request and prior reviewed observation/data facts in SPEC 7.4,
7.4.1, 6.9 and 19. Expected product profile selection is user direction, not
new retail observation. All cases are freshly authored; no original artwork,
executable text or asset bytes are included.

1. Input: shared PC install resources; no explicit profile selection, with or
   without a supplied PS2 source. Expected: selected PC text-only profile,
   original 51-high strip from y 189.2 to 240, dark player box and original
   pill art/labels; no console strip, mouse/key/controller badges or Help pill.
2. Input: main menu in the PC original viewer. Expected: Quit Game at slot 1
   (+223,+226), Continue at slot 2 (+223,+203); remaining slots hidden. Options
   changes slot 2 to Back. PC controls adds Reset Keys To Default at slot 3
   (+76,+203). Car select uses Continue at slot 2 and Back at slot 3.
3. Input: pointer enters a visible pill's 144×24 hit rectangle, then primary
   click activates it. Expected: original focus/glow behavior and one dispatch
   of the corresponding application-bound action. For Back and Continue this
   preserves the screen's existing Back/Accept behavior; for Quit Game and
   Reset Keys To Default the existing bound application action is invoked.
   Clicking a hidden slot produces no pill activation. No badge appears on
   hover. Application outcomes beyond existing bindings are not retail claims.
4. Input: open a modal dialog over the PC menu, then close it. Expected: PC
   help-bar strip and pills hidden during the dialog and restored afterward
   with the same applicable slot assignments; original dialog buttons retained.
5. Input: switch PC → PS2 → PC. Expected: exactly one appropriate strip at each
   step; PS2 suppresses PC overlay, uses only verified mapped genuine PS2 icons
   or reports/omits unavailable ones; PC restores text-only pills and labels.
   Xbox selection reports unavailable with current PC/PS2 sources, with Xbox
   labels and no silent PS2 art substitution. Mouse/keyboard actions persist.
6. Input: PC-profile all-screen verification tour includes the help-bar package.
   Expected: eligible for rendering, not intentional-suppression status. Repeat
   in console profile: intentional-suppression status as V4-4. Missing resources
   and GL errors remain separately reported in either profile.
7. Input: missing original pill texture or required original font. Expected:
   unavailable-resource behavior under 20.1, no generated pill/icon or substitute
   font. Authored corner transformations still follow 20.2 in all profiles.
