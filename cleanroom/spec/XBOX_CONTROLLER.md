# Genuine Xbox controller source — reviewed 2026-10-13

Status: reviewed; approval hashes are recorded in REVIEW.md. Newly supplied owned USA Xbox disc, read-only data analysis; no original game execution. **O** means observed in disc data or decoded pixels. **I** means interpretation using standard BC1/BC2 formats and public filesystem documentation. **D** means demo policy. This adds a verified source to SPEC 20.3/20.5; historical statements that no Xbox source had been supplied remain true for their earlier input sets. It does not establish retail input-token mappings or console baseline.

## X1 Source resolution [interop] (O/I)

The supplied ISO has size 2,724,986,880 bytes and an XDVDFS volume descriptor at sector 32 (65,536 bytes), partition base zero. Sector size is 2,048 bytes. The descriptor points to root sector 264 with root-table size 120. This disc also includes unrelated bonus content; its assets are not the frontend source described here.

Resolve the member `/NFSUNDER/ZZDATA5.BIN` [interop path]. Its observed size is 244,525,056 bytes. Inside it, a bounded uncompressed texture-pack chunk containing the verified Xbox glyph keys is at member-relative byte offset 181,316,096 and spans 102,144 bytes. Those are **data offsets**, not executable addresses. The offset is an observed locator for this image, not a required fixed offset in other dumps. Runtime source resolution may locate the same bounded texture pack by chunk metadata and verified keys. It must not treat an unrelated disc title's global texture file as this game's controller pack.

XDVDFS structural facts needed for reading this source: little-endian volume fields at descriptor offsets 20/24 are root sector/byte length. A directory table's root entry is at byte offset zero. Entry offsets 0/2 hold 16-bit left/right child offsets measured in four-byte units; zero denotes no child. Offsets 4/8 hold 32-bit starting sector/byte size; bytes 12/13 are attributes/name byte length; name begins at 14. Attribute bit 0x10 denotes a directory. File content begins at partition base + sector×2048. Recurse into directory tables; directory entry child pointers remain relative to their own table. These format facts follow [Andrew de Quincey's XDVDFS documentation](https://multimedia.cx/xdvdfs.html), and traversal was observed to resolve the supplied member. Do not copy a filesystem implementation from that or another source.

Reject truncated descriptors/entries/names, out-of-table child offsets, repeated cyclic entry visits and any table/file extent outside the source. The recorded member extent bounds every later pack access.

## X2 Texture pack (O) [interop]

The common chunk IDs and 124-byte texture records from SPEC 15 apply, with a **60-byte opaque format descriptor per texture**, rather than PC's 32 or PS2's 84. The observed container structure below gives offsets relative to this 102,144-byte pack:

| Chunk | Header offset | Payload bytes |
| --- | ---: | ---: |
| texture pack 0xB3300000 | 0 | 102136 |
| header group 0xB3310000 | 64 | 9564 |
| pack info 0x33310001 | 72 | 124 |
| key list 0x33310002 | 204 | 392 |
| texture records 0x33310004 | 604 | 6076 = 49×124 |
| format descriptors 0x33310005 | 6688 | 2940 = 49×60 |
| pixel group 0xB3320000 | 9728 | 92408 |
| pixel metadata 0x33320001 | 9736 | 24 |
| pixel bytes 0x33320002 | 9856 | 92280 |

Pixel payload begins at 9864; align it upward to a 128-byte boundary, giving **pixel base 9984**. Record image offsets are relative to that aligned base. First record begins at offset 612. Same record fields as SPEC 15.4 apply: key at record +36, image/palette offsets and lengths at +48/+52/+56/+60, width/height at +68/+70, format byte at +74. Stored name fragments are not reliable full identifiers: several language-specific names truncate and repeat. Resolve by the verified 32-bit key rather than matching only such a fragment.

Of 49 records, **47 are Xbox controller textures**; two are unrelated visual textures. Every verified glyph is 32×32. Forty-six glyphs use format **0x24 (BC2/DXT3)** with 1024 bytes, and the key 0x68FDF2FD uses **0x22 (BC1/DXT1)** with 512 bytes. These glyphs have no required palette. Base-level blocks are arranged in ordinary row-major 4×4 block order. No PS2 nibble unswizzle or doubled palette alpha applies. Decoded pixels were visually inspected and show genuine coherent controller glyphs, including localized letter variants.

## X3 Narrow decoder behavior (I, pixel result observed)

Use the standard BC1/BC2 rules already covered by SPEC 15.7. For each 4×4 block, the two RGB565 endpoints expand by bit replication. Colour indices are little-endian two-bit fields in row-major pixel order. BC2 has eight preceding bytes of four-bit alpha, each multiplied by 17; it always uses the four-colour interpolation mode. BC1 uses the three-colour/transparent index-3 mode when endpoint 0 ≤ endpoint 1, otherwise four colours. Colour interpolation uses integer division. Output is ordinary 8-bit RGBA without extra tint/alpha adjustment; authored colour modulation occurs later as in SPEC 1.4.

Validate record/table extents, descriptor count/stride, dimensions, offsets and pixel lengths. This source only verifies 32×32 BC1/BC2 glyphs. Unsupported formats or malformed records are unavailable, with no replacement image. Expected base-level bytes are ceil(width/4)×ceil(height/4)×8 for BC1 and ×16 for BC2. Reject shorter data and out-of-payload extents; do not extend reads into another member. Other Xbox textures, mipmapping and descriptor meanings are not specified by this analysis.

Synthetic acceptance vectors: `test-vectors/xbox_controller_vectors.json` and `test-vectors/xdvdfs_vectors.json`. No disc bytes or decoded art are included in these vectors.

## X4 Verified English/base gallery keys (O) [interop]

The following 23 keys identify non-language-suffixed textures. Names below are descriptive glyph categories, **not retail action mappings**. Directional/stick variants must remain separate keys even where their visible letter is similar.

| Key | Glyph category |
| --- | --- |
| 0x09B6DB77 | black button |
| 0x0E7B631C | direction pad up |
| 0x285718ED | start |
| 0x3C5C25BC | left stick |
| 0x3FD3B7A3 | left-stick left/right variant |
| 0x4B47BC3B | white button |
| 0x523452D0 | right trigger |
| 0x68FDF2FD | left trigger (BC1) |
| 0x85104670 | back |
| 0x85119EB8 | direction pad |
| 0x8522E0A0 | A |
| 0x8522E0A1 | B |
| 0x8522E0B7 | X |
| 0x8522E0B8 | Y |
| 0x8A3F71D3 | direction pad up/down |
| 0x9AD74EEF | direction pad down |
| 0x9ADB8542 | direction pad left |
| 0xAD32EFD4 | right-stick up variant |
| 0xC3FC9001 | left-stick up variant |
| 0xC5A4E9A7 | right-stick down variant |
| 0xD0BE1B1F | direction pad left/right |
| 0xF2C274CF | right stick |
| 0xF6BAF515 | direction pad right |

The remaining 24 glyph records are localized French/Italian/Swedish stick/trigger variants. A gallery may show all 47, or show the 23 base glyphs and offer a language filter (D). Availability is determined by actual validated records, not solely the ISO filename. A source containing only some verified records exposes that subset and reports missing records honestly.

## X5 Presentation and unknowns (D)

With this verified source supplied, the Xbox gallery reports available Xbox glyphs, uses genuine Xbox art and Xbox family labels. With it absent, Xbox remains unavailable; PS2 art never substitutes. Preserve PC text-only default, original shared PC font/panel resources, explicit console-family selection, original corner transformations and exactly one active strip. Gallery captions/guidance have disjoint bounds under DEMO_DESIGN D1/D2.

Demo actions may be bound explicitly to selected verified keys, but a texture's existence/name is insufficient evidence for the retail mapping of Accept/Back/Help/etc. No automatic retail token map is established here. This analysis also does not determine Xbox runtime hint baseline, retail gallery composition, every language's preferred aliases, additional compressed-source variants, or unrelated bonus-content formats.
