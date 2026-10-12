# Retail HUD sprites

The standard cluster now loads `3RDPERSON_RPMDIAL_CUSTOM_00` (key `0x95479060`)
from the player's HUD skin pack and uses its authored colour/transparency for
the needle. OpenUG2 already loads the tach backing, arc, fill, nitrous art,
minimap, message art and drag sprites. This change replaces the standard
cluster's drawn needle; it does not import or redistribute those textures.

## Decoder correction

TPK record discovery previously admitted only names beginning with `A`..`Z`.
That excluded valid `3RDPERSON_*` records. Both key enumeration and lookup now
also admit `0`..`9`, retaining existing format and payload bounds checks.
Synthetic P8 tests cover all ten leading digits, RGB/alpha preservation,
enumeration and truncated data. A local skin-pack audit decoded eleven numbered
needle records after the fix, versus none before.

## Geometry and fallback

The local skin-00 needle is a 16x128 paletted sprite. Its padding places the root
at the texture centre and the tip in the bottom image half. Draw the
whole texture around that centre, preserve its width/height ratio and rotate
from the HUD transform's downward image direction into the fitted arc direction. The existing HUD
scaling supplies the desired reach. White tint preserves the decoded artwork.

The procedural needle remains a fallback when the record is absent, rejected or
fails upload; `hud_missing()` reports the named record. An absent tach-fill
sprite is skipped rather than drawn as a solid untextured rectangle. TPK lookup indexes are
released once each source pack has been processed.

This independently written integration is informed by the sprite identity in
[nfsu2-re's HUD research](https://github.com/troikoss/nfsu2-re/blob/main/docs/NATIVE_DEMO.md)
and local format/alpha measurements. No recovered code, retail byte table,
executable or image fixture is committed. Skin selection beyond 00, retail
FEng pivots/layout, exact dial scales, redline overlays and original tint state
remain outside this change. The fitted geometry is an OpenUG2 presentation
seam, not a complete retail HUD reproduction.
