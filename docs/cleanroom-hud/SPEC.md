# Native single-player HUD interoperability specification

Status: approved bounded interoperability specification; approval and exact hash are recorded in REVIEW.md. Date: 2026-10-12. At original approval no retail execution or original-game screenshot was available; user-taken retail PC captures have since been reviewed and are incorporated only through the sections "Runtime capture amendments, capture set 1", "capture set 2", "capture set 3", "set 4 (implementer questions)", "capture set 5", "capture set 6", "capture set 7" and "Runtime verification amendments, set 8 (glyph sampling)" and the in-place notes that cite them. This document specifies an independently implemented HUD reading the user's own assets; it does not establish exact runtime or pixel parity.

The permitted implementation dependency is the public independent OpenUG2 baseline at commit `03edcd0732b38e7077e8883e50dbead85fbd4dee`. Integrate in its existing C engine in `/tmp/openug2-cleanroom/impl`, target native ARM64, and retain its established texture decoding/drawing facilities where useful. The analyst has not inspected that dependency. No mapped executable, original instruction execution, recovered implementation, original program symbols, or x86 compatibility layer is required or permitted by this spec.

Integration preservation: replace or extend only the standard driving drawing path needed for this scope. Preserve the independent baseline's public HUD interfaces and established utility behavior outside that path, including map picking, status updates and existing auxiliary gauge/NOS helpers. This includes public constants, types and declarations used by baseline callers and tests as well as function implementations; the retained regression target must compile before its assertions run. These names describe permitted baseline functionality, not original-program symbols. Do not satisfy integration by replacing existing helpers with no-ops, constant failures or empty results. Features excluded from new drawing recovery remain available with their prior baseline contracts; this scope does not authorize unrelated utility regressions. Preserve existing independent baseline rendering for race/status/notification/auxiliary NOS paths where practical, even though their exact retail replacement is outside this specification. Prior provisional visuals may remain explicitly provisional with no original-equivalence claim. They must not reveal stored sample text from the newly loaded standard driving package. Retain and run the baseline helper regression suite alongside the new tests. Verify representative caller inputs against the permitted baseline's existing behavior, adding integration checks where existing coverage does not detect a changed result. If an unavoidable behavior change is necessary, record the concrete dependency conflict as a spec question before changing that contract.

## Evidence and confidence

“Asset fact” means directly inspected original data, not observed live behavior. “Public report” means behavioral prose in the explicitly authorized public sources, independently corroborated where possible. “Provisional” means an original integration policy chosen to make this bounded HUD usable; it must not be presented as a verified game fact. The analyst evidence remains outside the shared spec.

Analyst-only provenance references: public native-demo, HUD-behavior and minimap-behavior reports retained in the evidence workspace. The reviewer filtered their data-format and behavioral statements into this document. Those reports also contain recovered implementation material and are not approved implementer inputs. Public report claims here are secondary reports, not fresh original-program runtime observations. Their recovered code, executable addresses, module structure and authoring/debug source paths are excluded.

Scope: authored standard single-player driving HUD, image/text/group hierarchy, bitmap fonts, skin selection 0–10, speed/gear/RPM, minimap road layer/player/north/border, viewport fitment and cash suppression. Engine physics, career simulation, race-only overlays, notifications, GPS routes, replay, pause menus, multiplayer, DRM and retail executable analysis are excluded.

## Explicit interoperability whitelist

The following exact strings may appear in the implementation only as asset/package/resource lookup keys. They are functional identifiers, not code to reproduce. Asset paths are relative to the user-selected extracted-data directory:

* `GLOBAL/InGameFreeRoam.bun`, package `HUD_SingleRace.fng`; `GLOBAL/InGameCommon.bun` for shared standard HUD textures.
* `LANGUAGES/English.bin`, `LANGUAGES/LanguageTextures.bin`, `GLOBAL/GLOBALA.BUN`.
* `GLOBAL/HUD_CustomTextures_ALL.bin`, `GLOBAL/HUD_CustomTextures_nn.bin` where `nn` is 00–10.
* `GLOBAL/GLOBALB.BUN`, `TRACKS/TRACKMAP4000.BIN`, texture key `TRACKMAP4000`.
* Package aliases `ConduitMdITC_TT14i.ffn`, `Font_ConduitMdITCTT38bi.ffn`; font keys `conduitmditc_tt14i`, `font_conduitmditctt38bi`, `conduitmditc_tt21i`.
* Texture keys `TACH_FILL_CUSTOM_nn`, `3RDPERSON_RPMDIAL_CUSTOM_nn`, `3RDPERSON_7500LINES_CUSTOM_nn`, `3RDPERSON_8500LINES_CUSTOM_nn`, `3RDPERSON_9500LINES_CUSTOM_nn`, `3RDPERSON_10500LINES_CUSTOM_nn`.
* Object lookup names `3rdPersonNeedle`, `3rdPersonSpeed`, `3rdPersonGear`, `TrackMapTargetRing`, `PLAYERCARINDICATOR_1`.
* Package image aliases `Track4000_map.tga`, `Minimap_Mask.tga`, `Minimap_Border.tga`, `Minimap_North_indicator.tga`, `Minimap_Icon_Car.tga`, `Tach_Fill.tga`, `3rdperson_redline.tga`, `3rdperson_rpmdial.tga`, `3rdperson_10500lines.tga`.

Other identifiers may be read as opaque data to resolve package references. Do not publish authoring strings or tables. Disregard embedded original authoring/debug paths; they have no runtime interoperability purpose. Exact uppercase unit strings `MPH`, `KM/H`, and gear tokens `R`, `N` are allowed because they are required display outputs. Synthetic vectors contain no retail asset samples or authoring text.

## Archive and package data

Asset facts unless otherwise qualified:

1. Uncompressed archives consist of sequential records: little-endian unsigned 32-bit type, unsigned 32-bit payload length, then that many bytes. Length excludes the eight-byte header. A type whose high bit is set can contain records in the same framing. Type zero is padding, not a resource. Never assume every file has the same nesting depth. Skip an unsupported record without losing framing. Reject an out-of-bounds record; do not read beyond a containing record.
2. Package record type is `0x00030203`. Its payload starts with four bytes `46 45 6e e7`, followed by a little-endian unsigned 32-bit extent excluding these first eight payload bytes. Internal blocks begin after those eight bytes and have four-byte tags, an unsigned little-endian 32-bit payload length excluding their eight-byte header, then that many payload bytes. Recognized tags are `PkHd`, `TypS`, bytes `52 65 73 cc` (resources), bytes `4f 62 6a cc` (objects), and `Targ`. Only required data blocks need be interpreted. All reads stay inside the enclosing package and block.
Package selection: within the `PkHd` block payload, offset 16 contains an unsigned 32-bit package-name byte count and offset 20 an unsigned 32-bit embedded-authoring-path byte count. The first 16 bytes are not needed for selection. The package-name bytes start at payload offset 24; their count includes the terminating null byte. The path bytes immediately follow the name range and have their own stated count; do not use or publish this authoring path. The inspected headers include trailing alignment padding after these two ranges. Require both ranges to fit the `PkHd` payload, a positive name count and a null within the declared name range; compare the bounded name to the whitelisted package name. Other package records are skipped. If two matching packages occur, report ambiguity rather than select by accident. This is an asset fact corroborated across ten uncompressed packages. Exact byte matching of the requested package name is the provisional selection policy; no hash or executable names are needed.

3. Inside the resource block, `RsNm` holds a null-terminated name pool. `RsRq` starts with a 32-bit count and then count records of 24 bytes. Each request contains a 32-bit identifier at offset 0, a byte offset into the name pool at 4, resource kind at 8 (0: untextured, 1: image, 2: font), and three remaining dwords that this bounded implementation does not interpret. Validate count/extent and null termination. Treat identifiers as supplied facts, avoiding recomputation when available.
Image request resolution: the identifier in `RsRq` identifies the package resource request and is not necessarily the texture identifier. For a request of image kind, remove only a terminal case-insensitive `.tga` or `.bmp` extension from its alias and hash the remaining stem using the specified case-folded hash; look up that identifier in the loaded texture catalog. Texture metadata corroborates this stem-hash convention, including the redline and other driving images. Do not hash or access embedded authoring paths. One explicit city-map exception is required: package alias `Track4000_map.tga` resolves to texture key `TRACKMAP4000`, not its ordinary stripped stem. Use supplied texture identifiers from the texture catalog as authoritative; truncated display names in texture metadata must not override those identifiers. If a stem-hash lookup or the explicit exception does not resolve, report a missing image and follow the existing per-feature fallback. No other alias-renaming rules are established by this spec.

Resource completeness: search every texture package in each applicable whitelisted source archive, at its actual record nesting depth. A required texture or font atlas present in any such package must resolve and draw; selecting only the first package or stopping after an unrelated atlas is insufficient. Use the permitted independent baseline's format/decoder facilities where needed, supplemented by the reviewed required-texture layout below; compressed archive catalog handling remains unspecified. Resolve each font's atlas using its texture key and the appropriate source archive, independently of the number of decoded font tables. Only report an asset as absent after searching the applicable packages; distinguish an unsupported/failed decode from absence. Fallback is for an absent or unsupported resource and cannot count as successful original rendering of an available supported resource.

4. Object block records with marker bytes `46 4f 62 ea` have a 32-bit payload size. Within each, `ObjD` is a descriptor block, followed by zero or more independently framed script blocks. Skip scripts without flattening the object hierarchy. Descriptors contain repeated two-byte tags, unsigned 16-bit value lengths and value bytes. Unknown fields are skipped using their lengths.
5. Descriptor `Ot` is a 32-bit kind: 1 image, 2 text, 5 group. `Oh` is a 32-bit name identifier. `OP` is 16 bytes: stable package-local object identity at 0, name identifier at 4, an uninterpreted four-byte value at 8, and resource index in the low 16 bits at 12. `PA`, when present, is a 32-bit parent identity matching the first field of another object's `OP`. A name identifier is not a unique instance identity: the supplied package has repeated name identifiers. Resolve parents by object identity, not name. Missing `PA` means package root. Reject cycles and excessive depth; preserve unrecognized nodes as inert containers or omit their draw safely.
6. Public report: names are resolved with a case-folded ASCII hash, initialized to unsigned 32-bit all-ones, iterating `h = (33*h + uppercase_byte) mod 2^32`. Convert ASCII a–z to A–Z without locale-dependent conversion, and omit the terminating null. Use this for the explicitly whitelisted object names. No executable string inventory is required.
7. `SA` holds initial appearance. First four unsigned dwords are blue, green, red, alpha, each in 0–255. Subsequent floats: pivot x/y/z at 16/20/24, local position x/y/z at 28/32/36, quaternion x/y/z/w at 40/44/48/52. Size x/y/z occupies 56/60/64 (68-byte text/group appearance). Images use x/y as width/height, retain size z at 64, and append UV rectangle floats at 68/72/76/80 (84-byte appearance). Preserve data order; do not reinterpret colors as floats. Require finite positions, sizes and quaternions. Handle quaternion projection including x/y mirrors, not only z-axis rotation.
8. Text `St` is little-endian UTF-16 authoring content. `Sj` holds justification bits: bit 0 center, bit 1 right; right takes precedence when both are present (provisional precedence). Neither means left. `Sl` is leading data, `Sw` wrap width; their full layout semantics are unknown and unnecessary for short numeric HUD strings. `SH` is a localization label, not a font. Text font comes from its resource index. `Sb` is not used as font selection.
9. Public report: flag bit 0 represents hidden state, but the exact serialized flag field has not been confidently located in this analysis. Do not treat `If` as this state: it is zero on inspected image records. Provisional runtime visibility is defined below rather than guessing a serialized field.

The supplied driving package contains 258 descriptors: 174 images, 60 strings, 24 groups. This is an acceptance observation, not a parser requirement; valid synthetic or other package versions may differ. All counts and byte extents require checked arithmetic before allocation or reading. Reject descriptor fields shorter than a required interpreted layout, unterminated strings, missing required identity, duplicate identities and dangling parents safely, with a diagnostic. An image/text node requiring a resource with an unresolved or out-of-range resource index omits that draw; group nodes do not require a resource. No special sentinel resource-index meaning has been established. A decoder may retain other well-framed objects after rejecting an individual object; framing failure rejects the containing record. Depth and allocation limits are implementation-chosen documented safety limits, not asserted retail constants. A missing or malformed appearance omits that draw. Names with repeated identifiers remain valid distinct instances.

## Composition, fitment and feature visibility

Public report plus corroborated authored geometry: coordinates use a 640×480 logical viewport centered at (0,0), x right, y down, with lower z in front. Compose an object's appearance with all ancestors. A group's size represents scale. For an image, create a centered rectangle using its authored width/height and UVs; transform its local vertices around the authored pivot, then through ancestors. In planar rotation notation, a local point is `position + pivot + R*(vertex - pivot)` before ancestor transforms. Text positions likewise remain local. Keep stable package order for equal depth (provisional tie-break).

Provisional fitment: scale uniformly by `s = min(viewport_width/640, viewport_height/480)` and center the logical viewport at the physical viewport center. Do not stretch one axis independently. Apply this same projection to map, gauges and text. For widescreen, use the resulting centered 4:3 fit until a separate explicitly selected widescreen policy exists. Do not globally shift all descendants to the viewport corners. (Capture set 1 amendment C1-1: this centred uniform fit is observed for a 1920×1080 output; other output modes remain unobserved.)

Provisional standard driving visibility starts from a deny list: only the gauge objects, chosen scale markings/redline/fill, the map road/mask/north/player/border children and their ancestor containers are eligible. Showing an ancestor must not make unrelated siblings eligible. Resolve primary gauge descendants through the ancestors of the whitelisted speed/gear/needle instances, then select gauge resource aliases within that subtree. Reveal the border group with its children recursively when map visibility is enabled. Hide event messages, reward strings, region labels, placeholders, race-only widgets and multiplayer alternatives. Do not display stored authoring sample text as live data. No coordinate table need be copied: consume positions, pivot, color, size, UV and parent links from the assets each run.

Integration notification availability: on the standard driving path, inherited phone/message visuals may render only when the host explicitly reports a pending message. No pending message means no phone/message banner or notification indicator. Preserve inherited rendering for an explicitly pending message, the baseline race path and existing message helpers. (Amended by set 4, D4: while the authored top-left widget is drawn on the standard path, inherited phone/message visuals are suppressed.) This focused availability gate is an authorized change to the inherited standard-path visuals to prevent fabricated gameplay state; it does not establish original retail notification behavior or add notification scheduling to this scope.

Inputs to the HUD are values from the host engine: signed or unsigned speed magnitude in metres/second, nonnegative RPM, gear state, maximum RPM where known, unit preference, skin index, map position/heading, viewport, optional explicit career balance, and explicit feature visibility. They are not produced by the HUD. Engine values remain authoritative; do not introduce fabricated wallet/reward/region/gameplay state. A nonpositive viewport suppresses the HUD draw. Nonfinite map coordinates or heading suppress the map for that frame and emit a diagnostic. Unsupported gear states suppress gear text and emit a diagnostic. These are provisional input-validation policies.

Cash: public report says cash visibility requires career context; authoring cash/reward text is cleared at initialization. Provisional bounded contract: show a numeric balance only when career context is true, a balance value is supplied and cash is not suppressed. Otherwise draw no cash text, including no zero or label. Do not show reward text without an explicit reward event; this scope does not implement reward animation. If the authored balance target cannot be identified without expanding the whitelist, keep it hidden and report the missing target. Negative supplied balances suppress cash with a diagnostic; nonnegative balances may be grouped by an explicit locale separator, default comma. This formatting policy is provisional, not an assertion of every retail locale rule.

## Required standard HUD texture sources and decoding

Asset facts: the road texture comes from `TRACKS/TRACKMAP4000.BIN`. Shared mask, north, player and border textures come from texture collections in `GLOBAL/InGameCommon.bun`; there is more than one collection in that file, so do not stop at the first. Their stem-derived identifiers resolve without another alias exception. `GLOBAL/HUD_CustomTextures_ALL.bin` contains all eleven standard custom needle textures as well as scale-marking and fill textures. Prefer that complete collection for this bounded skin path. Bind the selected needle object to the already whitelisted `3RDPERSON_RPMDIAL_CUSTOM_nn` texture family, with `nn` matching skin00–10: despite the key wording, this family is the needle artwork, not the background dial. All eleven identifiers were corroborated by the specified hash against catalog identifiers. Numbered archives can remain an optional fallback; their compressed catalog handling is not specified here. Merely loading the driving package's own image collection cannot resolve all standard HUD resources.

For these uncompressed collections, an archive container type `0xb3300000` contains metadata container `0xb3310000` and data container `0xb3320000`. Use the general bounded archive framing to find their leaf records. Metadata leaf `0x33310004` is a sequence of 124-byte catalog entries. Metadata leaf `0x33310005` is a parallel sequence of 32-byte format entries: the i-th catalog entry uses the i-th format entry. Reject mismatched counts or payload sizes not divisible by their respective entry lengths; do not pair by an independently sorted hash list. Data leaf `0x33320002` contains initial alignment padding followed by data. The data-offset base is the first absolute file byte offset divisible by 128 at or after this leaf's payload start (eight bytes after its header). All data slices must remain within this leaf. These record types and numeric codes are data-format identifiers, not executable addresses.

Catalog offsets are byte offsets from the beginning of a 124-byte entry. Texture identifier is unsigned little-endian dword at36. Image-data byte offset at48 and palette-data byte offset at52 are unsigned little-endian dwords relative to the aligned data base. Image stored byte extent at56, palette byte extent at60, and top-level image byte extent at64 are unsigned dwords. Width/height at68/70 are unsigned 16-bit values. Catalog byte74 is auxiliary format information; it does not independently select the decoder. In the missing standard needle this byte is `0x08`, while the companion format dword at20 is decimal41. In the shared map mask this byte is `0x24`, while the companion value at20 is four literal bytes `DXT3`. Do not equate the catalog's `0x24` byte with a Direct3D numeric format36 or infer an alpha-only image from its stored byte size. Reject zero dimensions and top-level extent exceeding stored image extent. Require every declared image/palette slice and the required decoded top-level input extent to fit the containing data leaf, using checked arithmetic. Higher mip levels may follow the top-level image; this bounded HUD may decode only the top level and skip the remaining image extent.

Companion format41 means an 8-bit indexed image ([Microsoft format definition](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dformat)). For the standard needles, top-level data has exactly width×height unsigned byte indices in row-major order, and the palette has 1024 bytes, 256 entries of four bytes. Interpret each palette entry as blue, green, red, alpha bytes, yielding RGBA `(entry[2],entry[1],entry[0],entry[3])`. Preserve alpha, including zero for transparent background; do not derive alpha from index zero or from RGB brightness. This palette interpretation reconstructs a coherent transparent needle silhouette from the original data. Channel order is the conventional interpretation used for this bounded decoder; grayscale needle evidence strongly confirms alpha placement but alone does not distinguish red/blue reversal. A future colored reference can further confirm channel order. Require enough top-level index bytes and the entire palette; otherwise report a malformed image rather than reading beyond the leaf.

Companion `DXT3` means standard explicit-alpha block compression. The mask's top level is 256×256 in the supplied assets and occupies 65536 compressed bytes. Decode 16-byte blocks arranged left-to-right then top-to-bottom, each representing 4×4 texels. In each block, bytes0–7 are sixteen little-endian four-bit alpha values in row-major texel order; alpha output is nibble×17. Bytes8–11 are two little-endian RGB565 endpoints; bytes12–15 contain sixteen little-endian two-bit color selectors. Colors use the two endpoints plus their 2:1 and 1:2 interpolations, including when the first endpoint is numerically below the second. DXT3 alpha is separate from these color selectors. RGB565 expansion and interpolation may use the host's established standard decoder; pixel rounding parity is not asserted. Required byte count is `ceil(width/4)*ceil(height/4)*16`; crop partial edge blocks to dimensions. The mask decoded with this format has a circular alpha silhouette, transparent outside, with an interior alpha below255. Preserve that authored alpha rather than normalizing all nonzero values to255. [Microsoft explicit-alpha description](https://learn.microsoft.com/en-us/windows/win32/direct3d9/textures-with-alpha-channels) confirms the four-bit alpha representation.

The map mask supplies coverage to the map composition described below; it must not render as an opaque foreground square. North/player/border can use the existing standard block-compressed decoder selected from their companion entries. Restoring normal blending after map composition remains required. This extension establishes asset decoding and a coherent original mask/needle, not retail renderer or edge-blending parity. Unsupported companion values use the existing missing-resource fallback and diagnostic. No complete catalog, palette or image sample is embedded in this spec.

## Original bitmap fonts

Asset facts: font archive record type `0x00030201`. Its payload contains a null-terminated font key in the first 256 bytes, a null-terminated texture key in the next 256, then a table beginning at offset 512 with magic `FNTF`. Referenced atlas textures come from the language texture archive for the 14i and 38bi fonts, and from the global archive for 21i. Font keys compare case-insensitively, ignoring the package `.ffn` suffix. Use the original atlases; a system font is an explicit fallback only. Reject nonfinite/nonpositive reciprocal atlas dimensions and glyph rectangles outside the resolved atlas bounds; omit invalid glyph draws with a diagnostic.

Offsets below are byte offsets relative to the first byte of `FNTF`. Header length is 128 bytes; glyph count is unsigned 16-bit at 10. A table of count glyphs follows, each 16 bytes. Supported fields of each glyph: UTF-16 character code at 0, unsigned byte pixel width at 2, unsigned byte pixel height at 3, atlas x/y as unsigned 16-bit at 4/6, unsigned 16-bit advance at 14. Byte 8 is opaque (zero in all inspected glyphs); do not interpret it as a bearing. Byte 9 is an inferred signed horizontal pixel bearing; byte 10 is an inferred unsigned vertical top offset. Bytes 11–13 relate to kerning and remain opaque in this scope. The 9/10 interpretation is supported by character metrics: narrow and descending glyphs have plausible negative/positive horizontal adjustments, and capitals versus lowercase have differing top offsets. This is a static inference, not a rendering observation; the reference baseline and exact vertical meaning still require visual confirmation. Header float at 64 is reciprocal atlas width, float at 68 reciprocal atlas height, unsigned dword at 36 supplies an apparent line-height metric. Header dword at 24 is a byte offset equal to `128 + 16*glyph_count` in every inspected font: it marks the start of additional metric/kerning data after glyphs. Header dword at 28 is a byte extent equal to the remaining font-record payload beginning at `FNTF`, in every inspected font. The additional area's internal encoding remains unknown. For the bounded reader, require the glyph area to fit the enclosing record; do not rely on these header dwords as pointers or decode the extra area. If their values disagree with the observed equalities, issue an unsupported-font diagnostic and use the explicit fallback. Confidence: character, dimensions, atlas coordinates, advance and byte-offset/extent equalities are asset facts; bearing interpretation, line metric and baseline are inferred.

For the bounded numeric text path, look up each character in this table, form its quad from the glyph's cell width and height and sample its whole atlas cell, use its advance only for subsequent pen positions, and scale by the authored text size. (Capture set 7 amendment G: the quad width comes from the cell and may exceed the advance; the quad is never clipped to the advance. See G4.) Compute justification using the sum of advances. Provisional placement policy: place each glyph at the text anchor plus its inferred horizontal bearing from byte 9 and unsigned top offset from byte 10, scaled by the text size; full baseline/leading behavior is not proven. (Capture set 1 amendment C1-2: this placement is observed-consistent within 1.5 logical units at one resolution; it remains not pixel-exact. Capture set 7 note: pitch is the pen step, while drawn glyph width is the scaled cell width, which may exceed the pitch.) ~~Use half-texel inset sampling where glyph dimensions permit it to avoid adjacent glyph bleeding (public report).~~ (Amended by set 8, G6: by default glyphs are sampled with no inset and with bilinear filtering; the transparent ring around each cell prevents bleeding. An inset is only an explicitly labeled compatibility setting, default off.) Preserve original text tint/alpha (set 8, G7: coverage is linear in stored alpha). Do not use `SH` as font key.

Provisional missing-character policy: use an available question-mark glyph, otherwise advance without drawing. Missing requested font resolves first to original 21i, then to an explicitly reported host font fallback; if no host font facility exists, omit text with a diagnostic. Do not silently claim original-font rendering in these cases. Kerning, multi-line wrapping, localization tokens, non-English language suffixes and exact baseline/leading behavior remain unknown; short numeric HUD strings may use the specified unkerned path. This limitation can affect spacing and prevents pixel parity claims.

## Speed, units, gear and RPM

Public report: speed is velocity magnitude times 3.6 for km/h or 2.237 for mph. Write `KM/H` or `MPH` to the authored unit text object in the same gauge subtree. Identify this unit target by its supplied authoring unit token before clearing sample content; do not hardcode coordinates. Provisional rounding: truncate the nonnegative converted value toward zero and print decimal digits with no leading zeros (`0` when stationary). Nonfinite input or conversion output, and values outside the implementation's documented safe numeric range, suppress the numeric speed with a diagnostic; do not invoke undefined integer conversion. Use magnitude for reverse speed.

Gear output is `R` for reverse, `N` for neutral, otherwise the positive forward gear number in decimal. The forward gear number is supplied by the engine; do not infer it from speed. Amended by capture set 1 (C1-3): the authored gear tint was not the displayed colour in the one observed state (forward gear 1, stationary at idle), so the former "preserve authored gear tint" rule is replaced by the gear-colour setting in C1-3. Exact shifting tint, per-gear rules and timing are not established and remain outside this bounded contract.

Public report: four scale families exist, 7500, 8500, 9500 and 10500 RPM; the reported sweep is `(maximum_rpm / scale_rpm) * 235 degrees`. Original assets provide the needle and markings; do not generate substitute dial artwork when the original resources exist. The exact scale selection from a car's maximum RPM and the zero-angle/direction formula are not fully documented in the allowed material. Provisional policy: choose the smallest listed scale not below the supplied maximum RPM, otherwise 10500; if maximum RPM is unknown choose 10500. Clamp indicated RPM to 0–maximum RPM when a positive maximum is supplied, otherwise 0–scale RPM. Set needle planar angle to `235 * indicated_rpm / scale_rpm` degrees, with zero planar angle at zero RPM. Static asset inference: the standard skin00 needle bitmap points downward from its central base at identity rotation, and the original8500 scale zero tick is also downward; increasing markings progress toward the left, top, then upper-right in the centered y-down coordinate system. Positive planar rotation therefore aligns the usable needle with the increasing scale. This provisional calibration treats the authored initial needle planar rotation as an authoring sample pose and does not add it to calibrated zero; static data alone does not establish its retail runtime role. This is an inferred usable pose from original artwork, not a retail-runtime measurement; other skin/layout variants and exact endpoint placement still require visual confirmation. Retain its authored pivot, local position, size and all ancestor transforms; calibration must not move the needle anchor. Override only the needle's planar z-rotation component; preserve any projected x/y mirror factors and ancestor rotations. The standard inspected needle has no x/y mirror factors, so its calibrated zero is planar identity. Unsupported quaternion decompositions must be reported rather than flattening the hierarchy. Nonfinite RPM falls back to zero. A configuration override of direction/zero is permitted only as an explicitly labeled compatibility setting, not hidden guessing.

Skins: accept integer 0–10. Load fills and selected needle/scale textures from the ALL archive first; its catalog contains all eleven variants. A numbered archive may provide an optional fallback when its existing loader supports it. Substitute only resources belonging to the gauge skin set; map and fonts are unchanged by skin selection. Apply the selected scale markings with matching skin. Assets missing a requested scale or skin fall back per resource to skin 00, then authored default, reporting that fallback with a diagnostic; if still missing, omit that image with a diagnostic while speed/gear and map remain functional. Out-of-range skin input selects 0 (provisional). Cache/load switching must not retain dangling texture references. All 11 selections must be exercised during verification. No visual design or texture is generated to imitate a missing retail skin.

## Minimap metadata and bounded behavior

Asset fact: TrackInfo is archive type `0x00034201` in the global archive. The observed payload has 131 records of 296 bytes. Each record's signed 32-bit track number at offset 140 is the lookup key; do not assume record order is track order. City record 4000 exists. Offsets 172,176,180 are finite floats that appear to describe map origin x, origin second-axis and world extent. City map and related tracks share them. Their transform interpretation is inferred, not established by static bytes alone. Other fields are not required here. Lookup the requested record; missing record disables map and yields a diagnostic. Reject nonpositive/nonfinite extent for the provisional projection. No authored track-coordinate table is embedded in this specification.

Public report: city roads use the original trackmap RGB, the original mask silhouette, the original border group, a centered single-player indicator and an orbiting north marker. The map texture has an all-zero decoded alpha plane, so ordinary image alpha rendering would incorrectly hide the roads. Original destination-alpha/stencil composition and soft-edge parity are unresolved. Provisional implementation: sample road RGB and clip it using the mask's alpha, preferably preserving that alpha continuously; a binary stencil threshold is an explicitly reported approximation if required by the host engine. Do not treat road alpha as visibility. Restore ordinary drawing state after map composition.

Resolve map, mask and north using their resource aliases; the supplied main map and mask share authored center. Resolve the border using `TrackMapTargetRing` and its authored children. Resolve player using `PLAYERCARINDICATOR_1` (the suffix identifies the authored pool slot, not player ID). Use its artwork/color but replace its authored dormant placement with the map center when active. Keep north's authored nonzero pivot: it orbits about the map center when rotated; rotating it about its own center is incorrect. Read all authored geometry from the package rather than a copied layout table.

Provisional world/map contract: the host supplies east-increasing x, north-increasing second coordinate, and heading clockwise from north in radians. With record origin `(ox, oy)` and positive extent `E`, unrotated road UV center is `u=(x-ox)/E`, `v=(oy-y)/E`. Map display rotates by negative heading so the player's forward direction points up; rotate map UV sampling and north position consistently around the authored center, while the player glyph stays at center. The north marker rotates/orbits with negative heading. This axis convention and projection sign need original-reference confirmation; the engine must translate its own axes at the HUD boundary rather than silently assuming names. Set a provisional world window size of 500 world units across the authored map width, hence UV window width/height `500/E`; expose a named map-window setting. Clamp texture sampling at atlas edges rather than wrapping city roads. The exact retail zoom coefficient, heading offset and off-map behavior remain unknown.

Integration map picking: when the authored standard map is drawn and enabled, accepted screen clicks must invert that same actual drawing transform: authored geometry and ancestor transforms, centered uniform viewport fitment, current heading, world center and configured world-window size. A click at the rendered map center returns the supplied world center. A world point projected onto the visible map and then picked must return that point within documented numerical tolerance. Use the same captured frame state for draw and picking; retaining unrelated baseline panel dimensions or radius for the authored path fails this requirement. Reject clicks outside the actual drawn map panel or where the mask has zero coverage, and reject authored-map clicks when that map is hidden, lacks required resources or has invalid input. This is an independent host interaction contract, not a newly inferred retail click rule. When the authored path is unavailable or the baseline race path is active, preserve the prior baseline map-picking interface and behavior for any map that path actually draws; do not route such clicks through stale authored state. No new gameplay destination/state is fabricated by picking.

No fictional GPS path, police icon, event icon or career marker is drawn. Map off disables roads, mask, north, player and all border children together. Missing road asset disables that map layer; never replace it with a procedurally drawn road network. Gauge rendering continues independently.

## Verification and explicit unknowns

Build and test portability: the shipped project must include the approved spec, review record and vectors under `docs/cleanroom-hud` with their approved content intact. New tests must locate these shipped vectors after relocation and from a clean checkout, or accept an explicit overridable vector path; a staging-directory absolute path must not be required or the default for shipped tests. Validate the tests from a checkout without the staging spec directory available. Build dependency tracking must include new HUD public/internal headers: after changing a relevant header, an incremental build must rebuild each affected binary or probe and leave unrelated outputs unchanged where the build system supports that distinction. Test this with a temporary header change or equivalent timestamp update followed by the normal build, restore it, and ensure the final build uses restored content. These are independent integration requirements, not original-program behaviors.

Run the synthetic vectors under `test-vectors/`; these are independently authored behavioral examples, not retail captures. Exercise the actual package through its parser and validate parent/resource references without copying the loaded arrays into code. Verify font atlas draw, all 11 skins, unit switching, neutral/reverse/forward gears, several RPMs, finite-input handling, map masking, north orbit, viewport resizing, hidden placeholders and cash absent/present/suppressed. Include at least one resize to a wide viewport; inspect that every HUD feature uses the same projection and the map/gauge arrangement derives from the data.

Asset acceptance requires a per-resource resolution/draw report for the original needle, selected scale markings/fill and any enabled authored redline, requested numeric font atlas, road texture, map mask, north/player artwork and border children. The visible dial is the composition of these authored gauge layers. The RPMDIAL_CUSTOM texture family counts as needle artwork only; no second distinct dial image or duplicate draw is required by its name. For supplied assets that contain these supported resources, each must resolve and appear in an enabled HUD state. Descriptor counts, found target objects, parsed font counts or found map metadata alone are insufficient. A screenshot containing only fill and host-font fallback, with available needle/markings/map absent, fails this acceptance requirement. Exercise resources in later texture packages with a synthetic multi-package catalog test; verify font-table presence separately from atlas presence. Also verify that baseline utility interfaces retain their preintegration behavior outside the replaced standard draw path.

Required report: build/test outcome, native architecture exercised, original-font success or fallback, resources missing, and screenshots of selected HUD states. An ARM64 build must be attempted; host-only compilation does not demonstrate ARM64 runtime behavior. Retail runtime evidence available to this spec is limited to capture sets 1–3 (three stills, three states), with set 4 re-measuring the same stills; see the amendments sections. Any supplied public gameplay screenshot of uncertain platform/version is qualitative context only and does not resolve exact PC asset, geometry, projection or rendering parity. Do not claim pixel parity, exact RPM angle, exact minimap projection, exact font baseline/kerning or complete feature-mask parity. Record visible discrepancies as spec questions for the analyst/reviewer, never by consulting recovered implementation.

Further unknowns include serialized hidden flags, full animation scripts, gauge dynamic tint, redline placement rules, installed turbo/NOS visibility, NOS destination-alpha composition, full original minimap zoom/clamp logic, career balance object selection and locale formatting beyond the provisional policy. Keep these outside the standard bounded path or explicitly label their provisional handling.

## Runtime capture amendments, capture set 1

Reviewed 2026-10-12. Evidence: one still capture taken by the user of the installed retail PC game (career free roam, stationary, forward gear 1, 0 MPH, imperial units, Bank 0, 1920×1080 output; car, skin, heading, NOS level and engine upgrades not recorded), cross-checked against the user's data files. The capture itself and analyst measurements remain analyst-only evidence and are not implementer inputs. One capture shows one state: it can establish static placement and what was visible in that state, never a dynamic rule. Confidence tags: **observed** (measured in the capture), **asset fact** (read from data files), **inferred** (one reasoning step beyond observation), **guessed**. Measurement tolerance is about ±1.5 logical units. "Dial angle" is measured about the tachometer needle's composed rotation centre from straight down, increasing screen-clockwise (down → left → up → right), matching positive planar rotation in the y-down frame.

Nothing in this section is derived from executable code. Unless an item below states a requirement, it is information only and adds no drawing obligation.

### C1-1 Fitment (observed)

At a 1920×1080 output the game image is a centred 4:3 region (columns 240..1679, black outside) at 2.25 output pixels per logical unit in both axes, and every visible standard-HUD element lies within about 2 logical units of its authored composed position. This matches the existing uniform centred fit, which is now observed for this output mode. Other aspect ratios and resolutions remain unobserved. Vector: `capture1.json` `fit-1920x1080`.

### C1-2 Glyph placement (observed-consistent)

The existing glyph placement policy (anchor + byte-9 bearing and byte-10 top offset scaled by text size, right justification by summed advances) reproduces the measured glyph boxes of the speed, gear, unit, bank label and bank value texts within 1.5 logical units at one resolution. It is upgraded from provisional to observed-consistent; pixel parity, kerning and multi-line baseline remain unknown. Vector: `glyph-right-justify`.

### C1-3 Speed, unit and gear colour (observed / guessed / unknown)

* Observed: speed digits and unit text were drawn in their authored tint.
* Observed: in forward gear 1 at idle the gear digit was orange (peak about RGB 252,130,71), not its authored grey tint.
* Requirement (provisional): draw gear text with a named gear-colour setting instead of the authored tint. Its default approximates the observed colour, RGB(255,128,64) (exact value guessed). ~~Applying the same colour to every gear state~~ Amended by capture set 3 (C3-3): gear 2 was observed in its authored tint, so the setting now applies to forward gear 1 only; all other gear states use the authored tint (provisional). **Further amended by capture set 5 (E1): gear 6 was also observed orange, so the gear-1-only restriction is withdrawn; see E1, and F1 for the current rule.**
* Unknown: whether the colour differs for N, R, other gears, near redline or at a shift point. No flashing or animation is specified.

Vector: `gear-colour-setting`.

### C1-4 Tachometer markings and needle (observed / asset fact / inferred / unknown)

* Observed: the 9500 marking family was displayed for the captured car, drawn unrotated at its authored pose. The needle was not at its authored sample pose.
* Observed: at idle (RPM not recorded) the needle pointed at a dial angle of about 21°, adjacent to the printed 1000 mark. This is consistent with the existing inferred zero (straight down) and screen-clockwise direction. A single state at unknown RPM does not observe the zero or the sweep, so both remain **inferred**.
* Asset fact: on every marking face the highest printed thousand sits at a dial angle of about 225.6°, with non-uniform hand-drawn spacing. The linear `235 × rpm / scale` rule therefore differs from the printed graduations by up to about 6° (7500 face) and about 3° (9500 face).
* The linear rule stays provisional. Unknown: whether the retail needle follows the printed graduations; how the scale family is selected (car maximum RPM not recorded). No per-face graduation table is approved.

### C1-5 Redline (observed / inferred / unknown)

* Asset fact: the redline artwork is an arc about the tach hub (radius about 58–61 logical units at its authored pose) whose untrimmed span is dial angles about 135°..235.6°.
* Observed: only the portion from about 187° to the scale end (about 235°) was visible; the untrimmed arc was not shown.
* Inferred (unverified): the visible part runs from the car's redline RPM to the scale end, with its trailing end fixed and its leading part removed.
* Amendment to "any enabled authored redline": drawing the whole untrimmed arc is not consistent with the observed state. By default the redline is not drawn. A host may enable an explicitly labeled inferred redline setting, which takes a host-supplied redline RPM `r` and the selected scale `S`. If `0 ≤ r < S`, draw only the part of the artwork whose dial angle is at least `235 × r / S`, up to its authored trailing end. If `r ≥ S`, or no `r` is supplied, draw nothing. A negative `r` clamps to 0. The trimming technique is an implementation choice. Rotating the artwork is not consistent with the observed fixed trailing end.
* Unknown: the exact start mapping, colour/blend, pulsing, and the source of the redline RPM.

Vector: `redline-inferred-optin`.

### C1-6 NOS arc (observed / inferred / unknown; information only)

Observed: a cyan arc with dark ticks and lettering covered dial angles about 91°..239° outside the dial, matching the full opaque extent of the authored NOS arc images. The displayed colour equals no single authored tint. The separate glow label image was not visible. Inferred: the arc was at full level, and the glow label appears only while NOS fires. Unknown: the level mapping, depletion direction, masking/blend, and whether visibility depends on NOS being installed. No new requirement: the NOS arc is not added to the standard path. (Capture set 3, C3-4, adds an opt-in inferred NOS level display with whitelisted keys; default unchanged: not drawn.) Existing baseline auxiliary NOS paths remain as SPEC states.

### C1-7 Boost gauge (observed / asset fact / unknown; information only)

Observed: a small dial was drawn lower-left of the tach at its authored position, with printed 0 (left), +30 (upper-right), −30 (lower-right) and two red zone marks. At idle the needle covered the 0 label. Asset fact: the boost face, fill and needle artwork exist in eleven skins in the custom-texture collection, following the tach's `_CUSTOM_nn` convention (texture names are from the game's data files). Unknown: the boost value at capture, units, range, endpoint angles, direction, the green light, and the fitted/not-fitted visibility rule. No requirement or whitelist entry is added. A boost-needle mapping test was rejected as unsupported.

### C1-8 Career bank display (observed; amends Cash)

* Observed: in career free roam with balance 0, the balance was shown as `0`, right-justified at the right of a translucent dark backing panel. A left label read `Bank:`. Sibling timer texts in the backing's group were not shown.
* Requirement: when cash is shown under the existing Cash contract, draw (a) the backing image of the bank area, (b) the bank label text and (c) the numeric balance in the authored balance text object. Keep the backing group's other texts hidden. When cash is not shown, all three are hidden together. The existing absence, suppression and negative-balance rules are unchanged.
* Label text: the label object carries a localization key that also occurs in the English language file. The language-table format is unspecified. Interim display string `Bank:` (English only, a required display output) is whitelisted below. Stored package sample text must still not be used.
* Lookup keys: resolved by capture set 3 (C3-8). If they do not resolve as specified there, follow the existing missing-target policy (keep hidden and report).
* Unknown: thousands separator and currency symbol (only zero observed); the existing comma default stays provisional.

Vectors: `bank-zero`, `bank-off`.

### C1-9 Top-left D-pad widget versus message gate (observed / unknown)

* Observed: with no message text on screen, the top-left widget was drawn: a translucent backing, a D-pad backing with four arrows and an orange centre icon. The mode icon, D-pad highlights and mail icon were not drawn.
* Observed: the centre icon is a different figure from the image the package references at that slot (position and tint are the authored ones). Matching artwork exists unreferenced in the free-roam texture pack. Its lookup key is unknown.
* Reviewer finding: this does **not** directly contradict the existing notification availability gate. That gate governs the phone/message banner and notification indicator. No banner was shown, and the mail indicator was hidden. Whether a message was pending at capture is unknown. The gate is unchanged.
* The widget's visibility trigger (free roam generally, engage/menu state, control scheme) is unknown, so the widget is not added to the standard path. If a later amendment adds it, the referenced package image must not be shown in place of the observed figure, and no imitation icon may be generated. The upper-area content, centre-icon selection and race behaviour are unknown. (Superseded in part by capture set 2, C2-4, which adds the career free-roam widget with keys pending; the gate finding above stands.)

### C1-10 Minimap north marker and off-window icon (observed / inferred; north rule only)

* Observed: the north marker was on the border ring about 71 logical units from the map centre (authored pivot offset 72), with its tip pointing outward. This is consistent with the existing orbit-about-centre rule and rotation with the orbit. The heading at capture was not recorded, so the heading sign convention remains inferred. Vector: `north-heading-90` (a consequence of the existing provisional convention).
* Observed: a point-of-interest icon was drawn on the border ring (radius about 70) with a blue selector ring concentric with it. Inferred: off-window icons clamp to the ring along their bearing. The in-disc case, icon semantics, edge radius versus icon size and map zoom are unobserved. POI/GPS drawing stays out of scope, and no clamp requirement or vector is adopted.

### C1-11 Objective arrow (observed; out of scope)

A perspective world-space arrow was drawn above the road. It is not part of the HUD package and is out of scope.

### Whitelist addition

* Display string `Bank:` (interim English bank label, C1-8). No other keys are added by this capture set.

### Open questions from capture set 1

Gear colour per gear/state; needle law (linear versus printed graduations), zero and idle RPM; scale-family selection; redline start mapping, colour and source RPM; NOS level mapping, direction, blend and installed visibility; boost value mapping, units, range, light and fitted visibility; top-left widget trigger and centre-icon key; POI clamping, categories and map zoom; balance separator and currency; bank lookup keys; fitment at non-16:9 outputs.

## Runtime capture amendments, capture set 2

Reviewed 2026-10-12. Evidence: a second still that the user took of the retail PC game, captured with the same build, output mode and frame mapping as set 1. State: career free roam, stationary, forward gear 1, 0 MPH, imperial units, Bank 100. The user describes the car as stock, without NOS or a boost device. Its heading, skin and maximum RPM were not recorded, and no host state was recorded beyond what is visible on screen. Confidence tags, dial angles and the analyst-only status of evidence are as in set 1. Comparing two stills shows two states. It does not establish a transition rule or a cause. Nothing here is derived from executable code.

### C2-1 Gauge without NOS and boost (observed)

* Observed: no NOS layer (backing, arcs, mask or glow label) and no boost-group element were drawn, and nothing replaced them.
* Observed: the tach, needle, speed, gear and unit text were at the same positions as in capture 1, within 0.5 logical units.
* Inferred: the NOS and boost elements are shown according to whether the device is installed. Installation was not independently recorded; the user described the car as stock.
* Requirement: the standard path's tach, needle and text placement does not depend on NOS or boost state.
* Unchanged: NOS and boost drawing remain outside the standard path, as in C1-6 and C1-7.

Vector: `gauge-placement-independent`.

### C2-2 Marking face per car (observed)

The second car displayed the 8500 marking family; capture 1 displayed the 9500 family. Face selection therefore varies between cars. The selection rule remains provisional, because neither car's maximum RPM was recorded. Gear 1 was again orange with the same measured colour, so C1-3 is unchanged and other gears remain unknown.

### C2-3 Redline (observed / unknown)

* Observed: the visible redline's trailing end sat at the scale end on both faces.
* Observed: the visible start was about 185° on the 8500 face and about 187° on the 9500 face.
* Unknown: whether the start follows `235 × redline_rpm / scale` (the C1-5 hypothesis) or is a near-constant trim independent of the car. These two near-equal start angles do not discriminate between the hypotheses.
* C1-5 stays an opt-in inferred setting, hidden by default, and its vector is unchanged.

### C2-4 Top-left widget: idle and message-indicator states (observed / inferred)

Two element sets were observed in career free roam.

* **State A** (capture 1): translucent backing, D-pad backing with four grey arrows, and the orange centre figure. No mail icon or highlight was drawn.
* **State B** (capture 2): everything in state A, plus:
  * an envelope icon at the authored mail-icon placement, using the package-referenced envelope artwork, orange and dimmer than the centre figure (exact colour/alpha unknown);
  * the right D-pad arrow highlight at its authored placement, drawn yellow (peak about RGB 252,193,53) instead of its authored white low-alpha tint.

  The other arrows were unchanged. **No message banner or message text was drawn** in either state.
* Inferred (strong, but unrecorded): state B is the host's pending-message state, and state A is no pending message. The envelope is the only evidence; pending status was not recorded independently.
* Bearing on the existing notification gate: the gate is **confirmed, not contradicted**. A notification indicator appeared only in the inferred pending state. The gate's "may render" permission for inherited phone/message visuals is unchanged. Retail showed no banner in the observed pending state, so an inherited banner remains independent baseline behaviour with no retail-equivalence claim.

Requirement (provisional, career free roam only):

* When career context is true, draw the widget backing and D-pad backing with arrows in their authored placements and tints.
* When the host reports a pending message, also draw the mail icon (its authored artwork, placement and tint; exact retail dimming unknown) and the right-arrow highlight. The highlight uses a named highlight-colour setting, default RGB(255,192,48) (exact value guessed; superseded by set 4, D2: RGB(255,190,50) with alpha 255, and the setting replaces alpha).
* When no message is pending, draw neither the mail icon nor any highlight.
* The centre figure's lookup key is unknown. Omit it with a diagnostic. Never substitute the different package-referenced image or a generated imitation.
* Lookup keys: resolved by capture set 3 (C3-8), including the centre figure's texture identifier, which supersedes the "omit" rule above when that texture resolves. If the keys do not resolve as specified there, keep the widget hidden and report the missing targets, as under the cash policy.
* Unknown: widget visibility outside career free roam (races, non-career modes); which arrow highlights for other notification kinds; the upper-area content; animation and timing.

Vectors: `widget-idle`, `widget-message-pending`, `widget-non-career`.

### C2-5 Bank multi-digit (observed)

`100` was drawn right-justified, with its right edge at the same position as capture 1's single `0`. Glyph pitch (pen-to-pen step) equalled the scaled advance, with no extra digit spacing. (Capture set 7 note: drawn glyph width is the scaled cell width, which may exceed the pitch; see G4.) This confirms summed-advance right justification for multi-digit values. Thousands separator and currency remain unknown. Vector: `bank-100`.

### C2-6 Minimap (observed / inferred; roads only as requirement)

* Observed: the road layer's light road ribbons on dark ground were drawn clipped to the disc, consistent with the existing mask composition. Capture 1's sparse "outline" was the same layer in an area with few roads (inferred).
* Information only: icons were drawn inside the disc, and one icon of a second artwork type was drawn on the ring at radius about 70 logical units. This makes edge placement at about 70 observed for two icon artworks. Whether inside-disc icons sit at their projected positions cannot be checked without world coordinates.
* Information only: small orange triangles were drawn at differing rotations inside the disc (inferred to be other vehicles). An icon under the player position was drawn beneath the player glyph.
* POI, vehicle and GPS drawing remain out of scope; the existing no-fictional-icon rule stands. Icon categories by colour, vehicle-marker edge behaviour and zoom remain unknown.

### C2-7 North marker (observed)

At a second, different (inferred) heading, the north marker was again on the ring, at about 70–71 logical units, with its tip outward. Orbit radius therefore does not vary with heading, consistent with the existing orbit rule. The heading sign convention remains inferred. Vector: `north-heading-180` (a consequence of the existing provisional convention).

### Open questions added by capture set 2

Redline start rule (proportional versus constant trim); confirmation of the host message state behind state B; widget lookup keys and centre-figure key; widget visibility outside career free roam; highlight arrow and colour for other notification kinds; mail icon colour/alpha; POI and vehicle marker semantics and edge behaviour; map zoom; balance separator; gear colour for other gears; scale-family selection from maximum RPM.

## Runtime capture amendments, capture set 3

Reviewed 2026-10-12. Evidence: a third still that the user took of the retail PC game, captured with the same build, output mode and frame mapping as sets 1–2. State: career free roam, moving, forward gear 2, 60 MPH, imperial units, Bank 25. The car is the same family as capture 1 (9500 face). RPM, NOS level, boost, heading and message state were not recorded. Lookup keys in C3-8 come from the user's data files only. Confidence tags and dial angles are as in set 1. Nothing here is derived from executable code.

### C3-1 Needle, second state (observed / inferred)

* Observed: the needle sat at a dial angle of about 186° while moving in gear 2, against about 21° at idle in capture 1.
* RPM was not recorded, so zero and sweep remain **inferred** (C1-4 unchanged). The two states are consistent with screen-clockwise increase.
* At this angle the linear rule and the printed graduations differ by less than 1°, so this capture does not decide the C1-4 question. No new requirement.

### C3-2 Shift-up indicator (observed / inferred / provisional opt-in)

* Observed: in this state a green upward triangle was drawn at the authored shift-icon placement, left of the gear digit. Its colour (peak about RGB 213,247,153) is not the white authored tint, so it is a runtime tint. It was not drawn in captures 1–2 (idle, gear 1).
* Inferred: it is shown at high RPM. In this frame the needle was at about the observed redline start. The threshold is not observed.
* **Amended by capture set 7 (H):** visibility is now default-on whenever the host supplies a valid shift RPM, the icon flashes, and the default colour is the authored RGB(202,244,138). Read the struck wording below as history; implement H.
* ~~Requirement (provisional, opt-in, hidden by default)~~: a host may enable a labeled shift-indicator setting that takes a host-supplied shift RPM. Draw the icon tinted with a named shift-colour setting (default RGB(192,255,128); exact value guessed; refined by set 4, D2, to RGB(215,248,155), with alpha 255 (the same as the authored alpha)) when a forward gear is engaged and the indicated RPM is at least the shift RPM. With no shift RPM, a non-forward gear or the setting disabled, it is hidden.
* Unknown: threshold, hysteresis, flashing and top-gear behaviour. The indicator is independent of the opt-in redline setting.

Vectors: `shift-optin-on`, `shift-no-rpm`, `shift-default-off`, `shift-neutral`.

### C3-3 Gear 2 colour (observed; amends C1-3)

* Observed: gear `2` was drawn in its authored grey tint, while gear 1 at idle was orange in captures 1 and 2.
* This contradicts applying the gear-colour setting to every gear, and C1-3 is amended in place.
* ~~Requirement (provisional): the gear-colour setting applies to forward gear 1 only.~~ **Superseded by capture set 5 (E1)**, which found gear 6 orange. N and R remain on the authored tint (provisional; unobserved).
* Unknown: whether the orange depends on gear 1 itself, on low RPM or on standstill.

Vectors: `gear1-colour`, `gear2-authored`.

### C3-4 NOS level (observed / inferred; opt-in display admitted)

* Observed: the NOS backing (dark, with ticks and lettering) was drawn over the full arc span. A coloured segment, darker than capture 1's cyan, appeared only at the arc's screen-left start (dial about 89°–98.5°). The glow label was not drawn.
* Observed across captures 1–3: a full coloured arc (capture 1, NOS level not recorded); nothing drawn (capture 2, car described as stock); backing plus a start segment (capture 3).
* Inferred: the coloured layer shows the remaining level. It is revealed clockwise from the arc start (about 91°) toward the end (about 239°), with a radial boundary through the tach hub. NOS level was never recorded, so linearity is unknown.
* Requirement (provisional, opt-in, hidden by default): a host may enable a labeled inferred NOS-level setting. It requires host-reported NOS installed and a level `L`; a nonfinite `L` hides the display with a diagnostic, and NOS not installed hides it.
  * Draw the NOS backing over its full authored extent.
  * Draw the authored NOS alpha-arc image only for dial angles at or below `a0 + clamp(L,0,1) × (a1 − a0)`, with a radial boundary about the needle's rotation centre. `a0` and `a1` are named settings, defaulting to 91° and 239° (measured from capture 1's full-arc extent).
  * At `L = 0` the coloured layer is absent.
  * The main arc image is not drawn: its composition with the other layers is unknown, and it has no alpha. The glow label stays hidden.
  * The clipping technique is an implementation choice; using the authored half-disc mask is permitted but not required.
* Colour and blend parity are not claimed, and the cyan composition is unknown.
* Capture set 5 note (E4): while NOS was being used at standstill the lit-arc boundary moved continuously, radially and linearly in time. This supports the radial boundary and a linear mapping in time, but the level itself was still not recorded, so linearity in level remains inferred.

Vector: `nos-level`.

### C3-5 Boost under load (observed; information only)

Observed: in this moving state the boost needle was again near the printed 0, about 9° screen-anticlockwise of its capture 1 position. The boost value was not recorded. A needle mapping is still not established, so **the boost gauge is not admitted** to the standard path (C1-7 unchanged). The C3-8 boost keys were checked and recorded by the reviewer but are not whitelisted until a mapping is specified.

### C3-6 Two-digit balance (observed)

`25` was right-justified with the same right edge as `0` and `100`, and glyph pitch equal to the scaled advance. This confirms C2-5 (pitch is the pen step; drawn width is the cell width, see capture set 7, G4). Vector: `bank-25`.

### C3-7 Minimap (observed; information only)

* Observed: at the same location as capture 1 but a different heading, rotating this map by the angle between the two north-marker positions aligns its road features with capture 1's. Map rotation and north orbit therefore use the same angle, consistent with the existing provisional rule. The sign convention remains inferred.
* Observed (low precision): map scale at 60 MPH matched the stationary capture 1 within about 10%. No speed-dependent zoom requirement is added, and the map-window setting is unchanged. Zoom above 60 MPH is unknown.
* Information only: several icons were clamped to the ring close to one another. POI drawing remains out of scope.

### C3-8 Lookup keys (asset facts; reviewed and whitelisted)

**Key forms.** Two kinds of key appear here:

* **Lookup names** are alias-stem strings from the package's resource names. They are resolved with the existing case-folded name hash; the reviewer recomputed each stated stem hash.
* **Stored name identifiers** are unsigned 32-bit values found in an object descriptor's name-identifier field (the `OP` value at offset 4, equal to `Oh`) of the user's `HUD_SingleRace.fng` package. They are data values from the asset format, not executable addresses or program symbols, and their preimage strings are unknown. They are acceptable as opaque data keys on the same footing as other data-format identifiers.

**Resolution rule.** Name identifiers are not unique in general (Archive and package data, item 5). Resolve an object only when all of the following hold:

* exactly one descriptor in the selected package carries the key;
* its kind matches;
* its parent's stored name identifier matches;
* for images, its resource alias equals the listed alias (case-insensitive).

Any mismatch or ambiguity is a missing target with a diagnostic, never a best guess.

**Texture identifier.** One unreferenced texture identifier is also admitted. It is a value from the user's free-roam texture catalog (`GLOBAL/InGameFreeRoam.bun`) and is used directly as a texture lookup key, with no stem string. It is flagged as a data value whose key string is unknown.

| Use | Kind | Key | Parent key | Image alias |
|---|---|---|---|---|
| Bank group | group | name id `0x3F8644CA` | root | — |
| Balance value | text | name id `0x1930B057` | `0x3F8644CA` | — |
| Bank label | text | name id `0x31B0D166` | `0x3F8644CA` | — |
| Bank backing group | group | name id `0x30D59B88` | root | — |
| Bank backing | image | name `Show_case_backing` | `0x30D59B88` | `Show_case_backing.tga` |
| Widget group | group | name id `0x3345911D` | root | — |
| Widget backing | image | name `Engage_Backing` | `0x3345911D` | `Engage_Backing.tga` |
| D-pad backing | image | name `Engage_DPAD_Backing` | `0x3345911D` | `Engage_DPAD_Backing.tga` |
| Right-arrow highlight | image | name id `0xA206A0B4` | `0x3345911D` | `Engage_DPAD_Highlight.tga` |
| Centre figure slot | image | name id `0x50910822` | `0x3345911D` | (geometry and tint only; see below) |
| Mail icon | image | name `Engage_SMS_mail` | root | `Engage_SMS_mail.tga` |
| Tach gauge group | group | name id `0x5164D4EA` | root | — |
| NOS backing | image | name id `0x11461255` | `0x5164D4EA` | `Tach_NOS_Backing.tga` |
| NOS alpha arc | image | name id `0x11495CD6` | `0x5164D4EA` | `Tach_NOS_Alpha.tga` |
| Shift-up icon | image | name id `0x25CF2F74` | `0x5164D4EA` | `Shift_Up_Icon.tga` |
| Centre figure artwork | texture | catalog texture id `0xEA570722` | — | — |

**Admissions to the standard path** (all other objects in these groups stay hidden under the deny list):

* **Bank panel (admitted; observed in 3/3 captures).**
  * When the existing Cash contract shows cash, draw the bank backing, the label and the value with their authored geometry, tint and fonts, as C1-8 describes.
  * The label displays the whitelisted interim string `Bank:`. The value uses the existing balance formatting.
  * The backing group's other texts remain hidden.
  * Default: on whenever the Cash contract is satisfied.
* **Top-left widget (admitted; state A observed once, state B twice; the pending mapping is inferred).**
  * When career context is true, draw the widget backing, the D-pad backing and the centre figure. Use the slot's authored geometry and tint with the catalog texture above, instead of the slot's referenced image.
  * If that texture is absent, omit the figure with a diagnostic. Never show the referenced image or a generated imitation.
  * When the host reports a pending message, also draw the mail icon (authored artwork, placement and tint; the retail dimming is unknown). Also draw the right-arrow highlight, tinted with the highlight-colour setting (set 4, D2: RGBA (255,190,50,255), replacing the authored tint including alpha).
  * Defaults: no pending message unless the host reports one; hidden when career is false (provisional).
* **NOS level:** opt-in only (C3-4), hidden by default.
* **Shift icon:** amended by capture set 7 (H): default-on when the host supplies a valid shift RPM (previously opt-in, hidden by default).
* **Boost gauge:** not admitted (C3-5).

The mode icon, the other arrow highlight, the empty widget sub-group, the NOS main arc, the half-disc mask and every boost element are not whitelisted, and remain hidden. The glow label is admitted only as the opt-in element of capture set 5 (E3); otherwise it stays hidden.

### Whitelist additions (capture set 3)

These are the stored name identifiers, lookup names, aliases and the catalog texture identifier listed in the C3-8 table, used only as asset lookup keys. Their names come from the game's data files. The flagged values are data values whose preimage strings are unknown.

### Open questions added by capture set 3

Gear-orange condition (gear 1, low RPM or standstill); N/R gear colour; shift threshold, hysteresis and top-gear behaviour; NOS level linearity and the composition producing the cyan; boost needle mapping; zoom above 60 MPH; separation of clamped icons; the host state behind widget state B; the centre-figure selection condition (for example control scheme).

## Runtime capture amendments, set 4 (implementer questions)

Reviewed 2026-10-12. Evidence: captures 1–3 re-measured by the analyst (the reviewer spot-checked the arrow and highlight colours independently) plus the user's data files. There is no new capture. On-screen colours are 8-bit values from lossy captures, accurate to about ±8 per channel. Where opacity is solved from these colours, that describes the visible result under ordinary source-over blending, not the original method. Nothing here is derived from executable code.

### D1 Widget arrow colours (observed; pins a verifier dispute)

* Observed: in both widget states the non-highlighted D-pad arrows are a **neutral mid grey**. The on-screen median is about 100–130 per channel, with peaks never above about 145, over dark backgrounds of about 30–70. They are **not near white**: a near-white reading is contradicted by all three captures.
* Inferred: drawing the authored D-pad backing image with its authored tint and alpha under ordinary blending reproduces this grey. No colour setting applies to the plain arrows.
* Observed: the highlighted right arrow (state B) is saturated yellow, about RGB(255,190,50), over the whole arrow shape. The effective opacity is at least about 0.95, so it fully covers the grey arrow beneath it. Keeping the authored low alpha would give a dim tan, which contradicts the captures.
* Acceptance for real-asset rendering (tolerance ±12 per channel):
  * Over a uniform background of (40,40,40), plain arrows render at about (115,115,115) in both states.
  * In state B, the highlighted right-arrow interior renders at about (255,190,50).
  * In state A, no highlight is drawn.
  * Up, down and left highlights were never observed and are never drawn.

### D2 Answer to Q3: tint settings and alpha

* **Highlight (observed result):** the highlight-colour setting replaces the authored tint **including alpha**. The default is RGBA (255,190,50,255). Texture alpha still applies.
* **Shift icon:** the authored tint alpha is already 255 (asset fact), so replacing RGB only and replacing RGBA give the same result. The default is refined to RGB(215,248,155) with alpha 255, inferred from the capture peak; the exact value is still approximate. (**Amended by capture set 7, H3:** the default is the authored lit colour RGB(202,244,138), and alpha follows the flash envelope.)
* **Gear colour:** the setting replaces RGB only, and the authored alpha (255) is kept. This is consistent with the observed gear-1 colour.
* **Mail icon:** it keeps its authored tint, with no setting. Its observed dimming has an unknown cause, and no requirement is set.

Vectors: `highlight-rgba-replaces-alpha`, `highlight-rgb-only-contrast`.

### D3 Answer to Q4: centre-figure sampling (asset fact + observed)

The slot's authored UV rectangle is the full texture, so authored UVs and the full extent coincide. Sample the substitute texture over its full extent (0..1), at the slot's authored size, transform and tint. The predicted figure (about 24×25 units, centred on the slot) matches the observed figure in captures 1 and 3, and the observed tint matches the authored tint. The implementer's interim is confirmed.

### D4 Answer to Q5: inherited banner versus authored widget (observed + reviewer decision)

* Observed: in both pending-indicator captures (stationary and moving), no message banner, message text or other notification visual was drawn. The widget's upper area was empty.
* Unknown: whether retail shows a banner at the moment a message arrives or when it is opened.
* **Decision:** on the standard driving path, while the authored top-left widget is drawn (career context true and widget targets resolved), suppress the inherited baseline phone/message visuals, including for a pending message. A pending message is then shown only through the authored mail icon and highlight.
* When the authored widget is not drawn (career false, or its targets missing), the existing notification gate applies unchanged: inherited visuals may render only when a message is pending.
* The baseline race path and message helper interfaces are unchanged.
* This amends the base integration gate. It is an integration decision based on the observed steady state, not a claim about retail arrival/open behaviour. Vectors: `banner-suppressed-with-widget`, `banner-fallback-without-widget`, `banner-fallback-targets-missing`, `banner-gate-no-pending`.

### D5 Answer to Q6: nonfinite and invalid thresholds (provisional host-input policy)

These rules are not retail facts; they extend the existing input-validation policies.

* A nonfinite redline RPM hides the opt-in redline with a diagnostic.
* A nonfinite or nonpositive shift RPM hides the opt-in shift icon with a diagnostic.
* The RPM compared with the shift RPM is the needle's clamped indicated RPM, with nonfinite RPM treated as 0 as in the existing rule.

The implementer's interim is confirmed, with a diagnostic added for the shift case. Nonfinite cases need runtime values outside JSON, as in the README. Vectors: `shift-nonpositive-threshold`, `shift-clamped-rpm`.

### Open items added by set 4

Highlight behaviour for other notification kinds and arrows; the cause of the mail-icon dimming; banners at message arrival or open; the exact shift colour.

## Runtime capture amendments, capture set 5

Evidence: user-taken lossless crops, two user NOS recordings (about 34 fps, variable frame rate, timing from presentation timestamps) and the user's verbal testimony, all career free roam with a stationary car in forward gear 1, the 9500 face and NOS fitted, plus the user's own data files. Tags: observed, asset fact, inferred, guessed. Dial angles use the C1 convention. Confidence is stated per item. Review note: the earlier gear-1-only rule was amended in place (C1-3, C3-3) because capture set 5 contradicts it.

### E1 Gear-digit colour (observed / inferred; amends C1-3 and C3-3)

* Observed: the gear digit was orange for gear 1 at standstill (several sources, high confidence) and for gear 6 in one lossless crop (single instance). It was in the authored grey for gear 2 while moving at 60 MPH. The plateau of the lossless orange is within about 2 per channel of RGB(255,128,64), so the existing default is confirmed. Glyph geometry is unchanged in the orange state.
* Testimony (not measurement): the user said the digit turns orange "between shifts". This is ambiguous (during a shift, or in the interval between shifts) and is not relied on for timing.
* Not supported: orange depends on low RPM (the gear 6 RPM was not recorded) or on gear 1 itself.
* ~~Inferred: every observed state is explained by "orange while drive is not engaged".~~ (Superseded by F1.) Original text: every observed state is explained by "orange while drive is not engaged". An alternative, "orange at standstill or during a shift", predicts the same for all observed states. The states that distinguish them (rolling with clutch disengaged, N, R) are unobserved. Confidence in the rule: low-to-moderate; it is provisional.
* ~~Requirement (provisional), including the *drive engaged* input and its fallback:~~ **Superseded by capture set 6 (F1)**; the "drive engaged" input is withdrawn. The struck text is retained only for history; implement F1.
  1. The host may supply an optional boolean *drive engaged*. When it is false, draw the gear text with the gear-colour setting (RGB only; authored alpha kept, as before). When true, use the authored tint.
  2. Fallback when the input is absent: apply the gear-colour setting when a forward gear is shown and the speed magnitude is below a named standstill threshold (default 0.5 m/s, guessed; only 0 was observed); otherwise use the authored tint. This reproduces all observed stationary states and the moving grey state but cannot reproduce orange during a shift.
  3. N and R keep the authored tint (provisional; unobserved), whatever the input.
  4. The colour change is immediate (no fade specified). No transition was captured.
* Unknown: orange duration and timing during a shift; N and R; rolling in gear without throttle; the standstill threshold.

`gear1-colour` and `gear-colour-setting` (earlier sets) remain valid under the fallback with speed 0, and `gear2-authored` with a moving car. Vectors in `capture5.json` (`gear-*`).

### E2 N2O glow label: lookup key (asset fact; reviewer recomputed the hash)

* The label is the root-level image with lookup name `Tach_NOS_Gamebreaker` and alias `Tach_NOS_Gamebreaker.tga`. Its stem hash (the specified case-folded hash) is `0xFE2467E3`, equal to its stored name identifier and to its texture catalog identifier in `GLOBAL/InGameCommon.bun`. Resolve it with the C3-8 resolution rule (unique descriptor, kind image, root parent, alias match); otherwise it is a missing target with a diagnostic, and the glow is not drawn. These are data keys admitted for lookup only.
* Authored appearance: tint RGB(204,244,138), alpha 255; centre (270,91) V; size 64x32 V; no rotation; full UV rectangle. The texture is 64x32 with white RGB and an alpha shape (an "N2O" inscription with a soft halo), so the drawn colour comes from the tint.
* Asset fact: the object carries animation scripts. Their key values are stored as one absolute reference followed by deltas added to it; the reference is not a timed key. One script, 800 time units long, matches the observed sequence. Parsing script blocks is **not required**: E3 specifies the curve directly. Other scripts on this object (a hidden rest state, a variant of the curve with another colour, a variant without size change) and the message and event identifiers that start them are not admitted, and their triggers are unknown.

### E3 N2O glow label: observed sequence and opt-in requirement (observed / inferred; provisional, opt-in, hidden by default)

* Observed (two independent sequences, consistent): at standstill, while NOS was in use from a full tank, the label faded in, grew non-uniformly by 10 V per axis with a rotation of about 2 degrees counter-clockwise, dimmed while shrinking, re-brightened to near full, then faded out; it was not drawn outside the sequence. Colour stayed at the tint (core pixels within video tolerance), the centre stayed within 0.5 V of the authored centre, and source-over fitted better than additive. The user's words ("glows, gets big, gets small as it dims, glows then dims back to normal") match.
* Inferred (strongly supported; the one non-obvious step): authored alpha values above 255 wrap modulo 256. The authored curve reaches 500 and the observed re-brightening only fits that wrap; clamping gives no re-brightening and fits several times worse in both recordings. Confidence moderate-to-high, but it rests on 34 fps recordings that cannot resolve brief dips, so it stays opt-in.
* Fitted time unit: 0.82 ms per animation unit (0.81 and 0.83 ms from the two sequences; stated uncertainty 0.03; a nominal 1/1200 s is guessed). Taking recording timestamps as real time is an assumption (audio length agreed with video span in one clip). A 1 ms unit and a 0.61 ms unit fitted markedly worse. Confidence moderate; the unit is a named setting.
* Not established: the trigger. In both sequences the sequence began about 0.4 s after NOS use began, at a NOS level of about 0.9; a fixed delay and a level threshold cannot be told apart. "Tank full" comes from testimony only. Re-trigger handling is unknown.
* Requirement (provisional, opt-in, hidden by default): the host may send a *NOS glow* event; the HUD then plays this sequence on the label from unit 0 at the time-scale setting (default 0.82 ms per unit):
  * Alpha, raw values linear between keys: 0 at 100, 255 at 340, 500 at 360, 500 at 480, 0 at 620. Displayed alpha = (floor(raw) mod 256) / 255. Alpha is 0 before 100 and after 620.
  * Size: 64x32 V until 340, linear to 74x42 at 360, held to 460, linear back to 64x32 at 480.
  * Rotation: 0 until 340, linear to 2 degrees counter-clockwise at 360, held to 460, linear back to 0 at 480 (absolute angle known only within about 1 degree).
  * Position fixed at the authored centre; RGB fixed at the tint. Drawn source-over. The label is hidden from unit 800.
  * A new event while running: ignored (guessed).
* Tolerances: alpha 0.05, size 1.5 V, rotation 1 degree, timing one 34 fps frame. Do not sample within 2 units of 340 and 548.3 (the wrap points; the 340-360 wrap dip is about 16 ms and unobserved).
* Vectors: `glow-*` in `capture5.json`.

### E4 NOS level while firing (observed, information only; amends C3-4 by note)

* Observed in two recordings of one stationary car: the lit-arc boundary moved from the full position (about 240 degrees) toward the arc start at 36.7 and 36.3 degrees per second, linearly with residuals within 1 degree, stopped when NOS use stopped and did not refill while stationary. The boundary is radial and crisp (transition about 1.5 degrees). With the 148-degree span this is about 0.25 level per second (about 4 s for a full tank). Confidence: moderate; only one car and state were captured, and the clips may share a session. **No drain-rate requirement is made**: the rate is a host value, and the HUD only displays the level.
* The C3-4 reveal direction and radial boundary are confirmed in motion; see the note in C3-4. Vector: `nos-level-three-quarters`.
* The main arc object stays unadmitted. Its authored scripts include tint changes with unknown triggers that were not observed, so the cyan question stays open.

### E5 Needles under NOS (observed; information only)

At standstill the tach needle stayed at about 18.5 degrees (+-0.5) and the boost needle at about 93 degrees (+-1) throughout NOS use. NOS use at standstill moved neither. Needle dynamics under load remain unknown. No requirement.

### Whitelist additions (capture set 5)

The lookup name `Tach_NOS_Gamebreaker`, alias `Tach_NOS_Gamebreaker.tga` and identifier `0xFE2467E3`, used only as asset lookup keys for the opt-in glow label.

### Open items added by capture set 5

Gear: orange timing during shifts, N, R, rolling while disengaged, standstill threshold. Glow: trigger (delay or level), re-trigger, exact time unit, variants and their triggers, absolute rotation. NOS: drain dependence on car/upgrade/speed, a brief unexplained brightening of the lit arc (no requirement), the cyan composition. Needles under load.

## Runtime capture amendments, capture set 6

Reviewed 2026-10-12. Evidence: the same still captures and two stationary NOS recordings as set 5, re-examined frame by frame, plus two successive pieces of user testimony about the retail game (the second corrects the first). Tags: **observed** (measured), **observed (user-reported)** (testimony, not measured), **inferred**, **guessed**. Nothing here is derived from executable code. This set amends the gear-colour rule in place: E1 items 1-3 and the "drive engaged" input are superseded by F1 (see the notes at C1-3, C3-3 and E1).

### F1 Gear-digit colour (amends E1, C1-3, C3-3)

Observations:

* Observed: in every frame of both stationary recordings (about 4.4 s and 0.4 s of HUD) the gear shows 1, the speed is 0, the tachometer needle stays at its idle position, no shift indicator is drawn, and the digit is orange in every frame with no grey pixels. No frame shows a gear change, a rev or an RPM drop. Orange is therefore held for at least 4.4 s at standstill and idle without any visible shift.
* Observed: stills show gear 1 at standstill orange (two cars), gear 2 at 60 MPH grey, and gear 6 orange (a user crop whose speed and context are unknown).
* Testimony handling: the user first said the digit is orange only during shifts and grey at standstill in gear 1. That answer does not fit the recordings (it would need a shift lasting over 4 s with the needle idle). After relaunching the original game, the user corrected it: "first gear is orange when the car is at idle". The correction supersedes the first answer; the first answer is not used. Orange during shifts rests on the earlier remark "between shifts" (ambiguous: during, or between) plus the gear 6 crop; no frame shows a shift. The shift half is therefore **observed (user-reported)** with low-to-moderate confidence, and the idle half is observed with high confidence.
* Inferred: "idle" is expressed as standstill (speed magnitude below a threshold) because speed is already a HUD input; "no throttle" would need a new input. Both predict the same for all evidence. Confidence low-to-moderate; rolling in gear 1 without throttle distinguishes them and is unobserved.

Requirement (provisional):

1. New optional host boolean *gear change in progress*, true for the host's own gear-change interval (the host defines it). Absent, false or non-boolean means not in progress.
2. Draw the gear text with the gear-colour setting (RGB only; authored alpha kept; default RGB(255,128,64)) when either holds: (a) a forward gear is shown and gear change in progress is true; or (b) forward gear 1 is shown and |speed| is strictly less than the standstill-threshold setting (default 0.5 m/s, guessed; only 0 observed). A nonfinite speed or threshold, or a threshold of 0 or less, makes no standstill claim (the Q7 interim, adopted).
3. Otherwise use the authored tint.
4. Without the input the HUD still shows standstill orange for gear 1 and never shows shift orange.
5. Forward gears other than 1 at standstill: unknown; authored tint unless a change is in progress. (E1's fallback applied to any forward gear; this narrows it.)
6. N and R keep the authored tint whatever the inputs (provisional; unobserved).
7. Colour changes are immediate; no fade (no transition was captured).

Unknown: duration and timing of the retail gear-change interval relative to the shift command, the digit change and the RPM drop; idle as standstill versus no throttle; gears 2 and above at standstill; N; R; whether a rev without a shift turns the digit orange.

Vectors: `capture6.json` (`gear-*`). In `capture5.json` the gear cases were amended: see its provenance note.

### Open items added by capture set 6

Gear-change interval duration and timing; idle definition; gears 2+ at standstill; N and R colour.

## Runtime capture amendments, capture set 7

Reviewed 2026-10-12. Evidence: the three retail stills and two recordings already described (re-measured for glyph extent), the user's data files, and two user findings about the retail game: (1) in the independent render italic digits looked cut off at the right while the retail captures show them whole; (2) with a manual transmission the green shift-up arrow shows while the engine is in its shifting range and flashes while shown. Tags: **asset fact** (read from the data files), **observed** (measured in captures), **observed (user-reported)** (testimony), **inferred**, **guessed**, **provisional** (an integration policy). Nothing here is derived from executable code. The captures and analyst measurements stay analyst-only. This set amends earlier text in place (see the notes at the font section, C1-2, C2-5, C3-2, C3-6, C3-8 and D2).

### G. Glyph drawn extent versus advance (amends the font section, C1-2, C2-5, C3-6)

#### G1 Cell versus advance in the font data (asset facts)

Terms: the **cell** is the glyph's stored atlas rectangle (atlas x/y at bytes 4/6, width and height at bytes 2/3). The **pen** is the running horizontal text position; it starts at the justified start and grows by each glyph's advance (bytes 14-15).

* Asset fact: the cell is the tight ink bounding box of the glyph in the 38bi and 14i atlases (ink touches all four cell edges for all but one non-empty glyph). The texel column just outside the left and right of every non-empty cell is empty (transparent), so sampling the whole cell does not pick up a neighbour.
* Asset fact: the cell often extends past the advance. With right overhang = `bearing (byte 9) + cell width - advance`, in font texels: in the 38bi font (speed, gear, bank value) nearly all glyphs overhang to the right (up to 7 texels); digit 1 does not overhang and the other digits overhang by about 2-4. In the 14i font (unit, `Bank:`) overhang is at most 1 texel and rare. In the 21i fallback font it reaches 9 texels. Per-glyph values are read from the font table each run; no table is copied here.
* Asset fact: for 38bi, `0` has bearing 2, cell width 18 and advance 17, so its cell spans pen+2..pen+20, three texels past the advance. The slash glyph has cell width 17 against advance 11.
* Inferred: a renderer that sizes or clips the quad to the advance removes the right 2-4 texels of most 38bi digits. At the speed text size 0.8 that is about 1.6-3.2 logical units, which matches the reported vertical cut.

#### G2 Captures: the full cell is drawn (observed)

Method: pen from the authored anchor, size and justification using summed advances. Ink is bright non-green-dominant pixels. Logical units V, 1 V = 2.25 px at 1920x1080, measurement error about 0.5 V plus antialiasing.

* Observed (ten 38bi digit instances across the three stills whose cell passes the advance): the right ink edge is 1.1 to 1.9 V past pen plus scaled advance, and within about -0.7..+0.1 V of the scaled cell right edge (pen + (bearing + width) x size). An advance-bounded quad cannot produce this.
* Observed: ink extends past the right-justification anchor (speed by up to about 1.9 V, bank value by 1.1-1.6 V). The anchor and the text element do not clip.
* Observed: adjacent cells abut without overlap in the captured strings. Whether overlapping quads stack in string order is unobserved.
* Observed (14i `MPH`, `Bank:`): consistent with the cell rule but too close to the advance to tell the rules apart.
* Observed: vertically the full cell height is drawn; ink top is 0.2-1.0 V below the scaled cell top and the bottom lies within about 1 V of the scaled cell bottom. No horizontal cut.

#### G3 Requirement (clarifies the passages named above; none previously said the quad width equals the advance)

For a glyph at pen `p`, text size `s`, text anchor y `ay`, bearing `bx` (byte 9, signed), top offset `ty` (byte 10), cell width `w`, height `h`, atlas origin `(ax, ayt)`, atlas size `W x H`, advance `a`:

1. Quad: `x0 = p + bx*s`, `x1 = x0 + w*s`, `y0 = ay + ty*s`, `y1 = y0 + h*s`. The width comes from `w` and the pen step from `a`; neither derives from the other. The quad may extend past `p + a*s` on the right, before `p` on the left (negative bearing), and over a neighbour's quad.
2. Texture rectangle before any inset: `u0 = ax/W`, `u1 = (ax+w)/W`, `v0 = ayt/H`, `v1 = (ayt+h)/H` (the whole cell). A half-texel inset moves each coordinate inward by at most half a texel; it must not change the quad geometry or restrict sampling to the advance. (Set 8 note, G6: the default is no inset; this inset bound applies only when the compatibility setting `glyph_half_texel_inset` is on.)
3. Next pen is `p + a*s`; justification width is `s * sum(a)`, unchanged. A glyph with `w = 0` or `h = 0` (for example a space) draws nothing and still advances the pen.
4. No implicit clipping: a glyph quad is drawn whole and is not clipped or scissored by its advance, the next pen, the justification anchor, the summed-advance width, the text element's box or wrap width, the line metric, a parent group, or any viewport other than the output. No authored text clip is observed for the speed, gear, unit, bank label or bank value. Clips specified elsewhere (minimap disc, NOS level) do not apply to text.
5. Overlap order (**provisional**): where quads of one string overlap, draw in string order with the text's normal alpha blending.
6. Acceptance (observed-consistent): at 1920x1080 with the authored HUD, each 38bi digit whose cell passes its advance has its right ink edge within 1 V of pen + (bearing + width) x size. Fail: an ink edge that ends at pen + advance x size, or a straight vertical cut through a slanted stroke.

Vectors: `capture7.json` (`glyph-*`, synthetic). The existing `glyph-right-justify`, `font-advance-right`, `bank-100` and `bank-25` stay valid.

#### G4 Unknowns

Stacking and blending of overlapping quads; whether the original applies a half-texel inset (invisible at measured scales; set 8 note: a re-measurement favours no inset, see G6); the 14i ink left edges sit 0.5-1.6 V right of the cell edge (unexplained, inside the C1-2 tolerance); authored text clips in other HUDs; the vertical baseline and line metric (inferred as before).

### H. Shift-up indicator: visibility, flashing, transmission (amends C3-2, D2 and D5 by note)

#### H1 Authored flash behaviour (asset facts, then inferences)

The shift icon (already whitelisted in C3-8) carries an authored colour animation, identical in the free-roam and race packages. This spec gives its behaviour directly; implementers need not parse any animation block.

* Asset fact: the cycle is 1200 animation units. The colour is RGB(202,244,138) with alpha 255 from unit 0 to 580, falls linearly to alpha 0 at unit 600, stays at alpha 0 to unit 1200, and returns to the lit state at once on repeat. The RGB at alpha 0 is irrelevant to the visible result and is not specified.
* Inferred: the cycle repeats while the icon is shown. It matches the user's "flashes while shown". No repeat was measured. Linear interpolation is assumed, as in set 5.
* Inferred timing, using the set 5 fitted unit of 0.82 +- 0.03 ms per unit (the same named time-scale setting as the glow, E3): period about 0.98 s (about 1 Hz), lit about 0.48 s, fade about 16 ms, dark about 0.49 s.
* Observed: the one still with the icon drawn shows a peak of about RGB(213,247,153), against the authored lit colour. The difference (about +11,+3,+15) is similar to the lossy-capture excess seen on another authored tint in set 5, so the authored colour is taken as the default. Confidence moderate; the colour is a setting.

#### H2 Per-car RPM data: information only

The user's car data files hold per-car RPM values that a host adapter might use to derive a shift RPM, but which value marks the shifting range, how engine upgrades alter it, and whether it matches the retail threshold are unverified (the single still fits some upgrade levels only if one hypothesis holds). **The HUD does not read car data and no layout is specified.** The threshold is host-supplied: a host-supplied shift RPM (D5) is the only input. This was decided because the fields are inferred and the installed upgrade state is not in these files.

#### H3 Requirement

1. The shift-indicator setting is **enabled by default** (amends C3-2's hidden-by-default). It remains a named setting a host can turn off.
2. Show the icon when the setting is enabled, the host supplies a finite positive shift RPM, a forward gear is shown, and the clamped indicated RPM (D5) is at least the shift RPM. Otherwise hide it. With no shift RPM the icon is hidden (unchanged). D5 validation (nonfinite or nonpositive shift RPM hides with a diagnostic) is unchanged.
3. Evidence for default-on: the icon is observed in the retail game at high RPM (one still) and reported shown with a manual transmission (**observed (user-reported)**). Hidden-by-default was an evidence-free caution. Because a host without a shift RPM still shows nothing, the flip is low-risk.
4. Threshold, hysteresis and top gear are **unknown**. No top-gear exclusion is specified (the earlier idea was a guess). A host that does not want the icon in top gear can withhold the shift RPM.
5. Flash: while shown, alpha follows H1 as a function of time since the icon became visible, `u = elapsed_ms / time_scale` taken modulo 1200 (alpha 1 for u < 580, linear to 0 at 600, 0 until 1200). The time scale defaults to 0.82 ms per unit (the E3 setting) and the clock is the host monotonic clock used for E3. A nonpositive or nonfinite time scale, or an invalid clock, draws the icon steadily lit with a diagnostic. A flash setting, default on, may be turned off to draw it steadily lit. The phase restarts at 0 whenever the icon becomes visible (**provisional**; retail phase is unobserved).
6. Colour: the shift-colour setting replaces RGB; its default is RGB(202,244,138) (replacing D2's RGB(215,248,155)). Alpha is the flash envelope (authored alpha 255 times envelope); texture alpha applies.

#### H4 Manual versus automatic (unknown)

* Observed (user-reported): shown and flashing with a manual transmission. The transmission in the still is not recorded. No HUD data depends on transmission mode.
* **Guessed**: automatic behaves the same. The host may supply an optional transmission mode (manual, automatic, absent). Manual or absent: apply H3. Automatic: apply H3 unless the named setting `shift_icon_automatic` (default on) is off. Confidence in the automatic default is low; it exists so the host can change it once evidence arrives.
* Capture request: same car at high RPM with automatic, and a short manual recording to measure the period and phase.

Vectors: `capture7.json` (`shift-*`, synthetic). In `capture3.json`, `shift-default-off` keeps its expectation (the setting is explicitly off) but its label is amended; `shift-optin-on` stays valid because it sets its colour explicitly. `shift-no-rpm`, `shift-neutral` and the set 4 shift vectors are unchanged.

#### H5 Unknowns

Exact threshold and its relation to car data and upgrades; hysteresis; top-gear behaviour; real-time flash period and phase on appearance; whether the cycle repeats; automatic-transmission behaviour; interaction with the gear-colour shift state (F1).

### Open items added by capture set 7

Overlap stacking and half-texel inset (inset addressed by set 8); shift threshold, hysteresis, top gear, flash period and phase; automatic transmission.

## Runtime verification amendments, set 8 (glyph sampling)

Reviewed 2026-10-12. Evidence: the user's data files (the 38bi font atlas and table) and a re-measurement of the speed digit `0` in the third retail still (the 60 MPH capture of set 3; lossy-compressed, about 1.8 screen pixels per font texel at the nominal scale). The re-measurement was prompted by a verifier report that italic digits in an independent render showed straight cut edges where the retail digits are rounded; that independent render is not evidence of original behaviour and is not described here. Tags as in set 7. Nothing here is derived from executable code. This set continues part G (items G5-G9) and amends the font section, G3 item 2, G4 and the set 7 open items in place.

### G5 Cell boundary content (asset facts)

* For the 38bi digits and the comma, the one-texel ring directly outside the cell, along the cell's own rows and columns (top, bottom, left and right), is fully transparent. This extends G1, which stated it for left and right. The four diagonal corner texels outside the cell were not inspected.
* The antialiased fringe lies inside the cell. The cell's own boundary columns and rows are only partly inked: on a slanted digit the left boundary column carries ink only in a short run of lower rows and the right boundary column only in a short run of upper rows, and several boundary columns never reach half coverage. Example, `0`: the left column is non-zero in 7 of 23 rows and at least half coverage in only 2; the right column never reaches half coverage.
* Every 38bi digit reaches the maximum stored alpha 255 in its interior; full opacity is stored as 255 (no half-scale convention).

### G6 Texel-to-screen mapping and filtering (observed-consistent, medium; provisional)

Requirement, for a glyph quad of on-screen size `w*k` by `h*k` (cell `w` by `h` texels, `k` screen pixels per texel from G3 and fitment):

1. The quad's outer edges coincide with the cell's outer texel edges, mapped linearly: texel column `i` (0-based within the cell) covers screen span `[x0 + i*k, x0 + (i+1)*k)`, and likewise for rows. This is G3 item 2's rectangle with **no inset** (default).
2. Sampling is bilinear. Texels outside the cell contribute alpha 0 (G5; for the inspected atlases this equals sampling the atlas itself along the cell's rows and columns). Consequently each outermost texel column and row fades out inside the quad: the coverage at a quad edge is about half of the edge texel's alpha, not the full value.
3. The outermost texel centres are **not** stretched to the quad edges (that is what a half-texel inset does).
4. A half-texel inset is permitted only as an explicitly labeled compatibility setting `glyph_half_texel_inset`, default **off**. When on, G3 item 2's inset bound applies.

Evidence and confidence: on the retail `0`, a model with quad edges on the outer texel edges and bilinear sampling of the cell plus its transparent ring fitted the measured luminance clearly better than a half-texel-inset model, both at the nominal scale and with the scale left free. The edge-to-edge mapping is **observed-consistent, medium confidence**: it rests on one lossy-compressed glyph, and its preference is entangled with the scale unknown in G9. Bilinear filtering is **inferred, low-to-medium**: the fitted models assumed it, and point sampling was not compared at this scale. Vector `glyph-map-*` tests mapping and filter together.

### G7 Coverage from stored alpha (observed-consistent, medium)

Requirement: glyph coverage is the filtered stored alpha divided by 255, linear, multiplied by the text tint alpha / 255 (the existing "preserve tint/alpha" rule), and blended source-over in output values. No doubling, clamped doubling, squaring or gamma conversion is applied to the alpha. Evidence: on the same retail glyph, alpha used as stored fitted better than a doubled or squared alpha. Confidence medium (one lossy glyph; the squared alternative was not far behind). Tint alpha below 255 is unobserved for text; the product is the existing provisional reading. Vectors `glyph-alpha-*`.

### G8 Rounded cut edges: no separate requirement (reviewer decision)

The proposed shape requirement ("rounded corners where the slanted stroke meets the cell; no straight full-coverage edge along the cell boundary") is a consequence of drawing the original atlas content (G5) with G6 and G7, so it is not adopted as a separate rule. It is kept as a real-asset symptom check only (observed, medium; the capture is lossy and was measured at about 1.7-1.8 px per texel): rendering the 38bi `0` at 1920x1080 at the authored speed size, at most 4 screen rows have their 50% left ink edge within 0.25 px of that edge's minimum, and each of the bottom four ink rows has its left edge at least 2 px right of that minimum. A straight vertical left edge with a flat bottom-left corner fails; that pattern points to glyph content that is not the original atlas cell, not to the mapping. G3 item 6 is unchanged.

### G9 Unknowns

* Effective scale: on the retail `0` the best fit was about 1.70 px per texel in both axes, against the nominal 1.80 for text size 0.8 at 2.25 px per logical unit (about 6% smaller, which matches the retail ink being about 2 px narrower than expected). A comparison against an independent render left an unexplained residual of about 3%. This rests on one glyph and is **not adopted**: G3 geometry and the C1-2 and G2 tolerances are unchanged. Other glyphs need re-measurement.
* Filter mode (bilinear versus point) not separated; diagonal corner texels outside the cell not inspected; the colour of the transparent ring texels (only coverage is specified); retail half-texel inset not excluded beyond G6's medium confidence.

Vectors: `capture8.json` (synthetic). In `capture7.json`, `glyph-uv-inset-bounds` remains valid as a case with the `glyph_half_texel_inset` setting on (its input sets it); all other set 7 vectors are unchanged.

### Open items added by set 8

Effective glyph scale versus nominal (about 6% on one glyph); filter mode; corner texels outside the cell; re-measurement on more glyphs and on the 14i font.
