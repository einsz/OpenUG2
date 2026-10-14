# Persistent settings backend — behavioural specification

Status: draft for review. Scope: every user-adjustable option of the retail PC game, where and
how it is stored, its value domain and default, how stored values are validated and applied,
and what happens when stored data is missing or corrupt. Out of scope: career/garage content
of the save (only its size and position are given so that settings can be located and the
rest preserved), the GUI that edits the options, ASI mods.

Provenance: static examination of the retail executable (sha256
`f9dd86c0…24168a7`, 4 800 512 bytes) plus the retail language file `LANGUAGES/English.bin`
(for option label keys). No runtime capture was possible (see §12); no user save existed on
the analysis machine, so all save-file facts are from static examination and the test vectors
are synthetic.

Confidence tags:

* **[static]** determined by examining the executable's behaviour; not yet confirmed at runtime.
* **[data]** read from retail data files.
* **[inferred]** one reasoning step from the above (usually the *meaning* of a field).
* **[guessed]** plausible, untested.

Registry value names (including the `g_...` names in §2.3), hexadecimal label keys and the
quoted English labels are flagged as functional data (interoperability keys / UI text identifiers),
not implementation identifiers; implementers need not reproduce any original variable naming.

Strings quoted in `code` style below are **functionally required** (registry value names,
directory names, file magic) — an interoperable implementation must use them byte-exactly.
Option labels are given as language-file keys (hash as in docs/cleanroom-camera §2.2:
`h = 0xFFFFFFFF; for each byte c: h = h*33 + c mod 2^32`) followed by the English text the
key resolves to in `LANGUAGES/English.bin`.

All multi-byte integers and floats in files are **little-endian**; floats are IEEE-754 single.

---

## 1. Overview: three storage locations

| Store | What | Scope | §  |
|---|---|---|---|
| Windows registry, machine-wide key | graphics/display options, install facts, language | per machine | 2, 4 |
| Profile save file (one per profile) | audio, gameplay, HUD, per-player, music playlist, online, controller bindings, plus career | per profile | 3, 5, 6 |
| Command line | safe mode | per launch | 4.6 |

There is no INI/config file for settings. `server.cfg` in the install directory belongs to the
online server component and is not a settings store. [static]

## 2. Registry

### 2.1 Key

* Root: `HKEY_LOCAL_MACHINE`. Path: `Software\EA Games\Need for Speed Underground 2`
  (a 32-bit process; on 64-bit Windows this lands under `WOW6432Node`). [static]
* A second key `Software\Electronic Arts\EA Games\Need for Speed Underground 2\ergc` holds the
  product registration code (default value, string). It is not a setting; a reimplementation
  needs neither to read nor write it. [static]

### 2.2 Install values (written by the installer, read once at start-up) [static]

| Value name | Type | Use |
|---|---|---|
| `Install Dir` | string (path) | data root; normalised to a long path |
| `CD Drive` | string (path) | disc location |
| `NotFirstTime` | DWORD | only *existence* matters: absent = first run. The game writes DWORD 1 on every start-up, after having checked existence. |
| `CacheSize` | string, decimal | parsed as a signed decimal integer (C `atol` semantics: leading digits, else 0) |
| `SwapSize` | string, decimal | as above |
| `Language` | string | see §2.4 |
| `StreamingInstall` | string | case-insensitively equal to `off` → streaming disabled; any other value or absent → enabled |

### 2.3 Graphics values

29 DWORD values, all read at start-up before the display is created and all written together
(§4.4). Names, column "struct index" = position in the 30-entry in-memory block whose first two
entries are `VERSION` and `SIZE` (the block is not stored as a blob; each value is separate).
"Static default" is the value used when the registry value is absent **and** no auto-detect has
run (§4.2). [static]

| idx | Value name | Domain | Static default | Options-screen label (key → English) |
|---:|---|---|---:|---|
| 0 | `VERSION` | written as 1, never validated | 1 | — |
| 1 | `SIZE` | written as 120 (= 30×4), never validated | 120 | — |
| 2 | `g_CarEnvironmentMapEnable` | 0…3 | 0 | `0x7D3930E7` Car Reflection Update Rate (slider) |
| 3 | `g_CarEnvironmentMapUpdateData` | 0/1 | 0 | `0x8FE9288E` Car Reflection Detail (0 Low, 1 High) |
| 4 | `g_CarShadowEnable` | 0…2 | 0 | `0xA7A683AC` Car Shadow / Neon (slider) |
| 5 | `g_CarHeadlightEnable` | 0/1 | 0 | `0xCFBEDB50` Car Headlight (Off/On) |
| 6 | `g_CarLightingEnable` | 0/1, capped to 0 | 0 | not in menu |
| 7 | `g_CarDamageEnable` | 0/1, capped to 0 | 0 | not in menu |
| 8 | `g_CrowdEnable` | 0/1 | 0 | `0x06CC3750` Crowds |
| 9 | `g_RoadReflectionEnable` | 0…3 | 0 | `0x2A2A4AB3` Road Reflection Detail (slider) |
| 10 | `g_FogEnable` | 0/1 | 0 | `0x95B8F38D` Fog |
| 11 | `g_MotionBlurEnable` | 0/1 | 0 | `0x822E4E9B` Motion Blur |
| 12 | `g_LightStreaksEnable` | 0/1 | 0 | `0x6F2C7417` Light Trails |
| 13 | `g_LightGlowEnable` | 0/1 | 0 | `0x62925361` Light Glow |
| 14 | `g_AnimatedTextureEnable` | 0/1 | 1 | not in menu |
| 15 | `g_ParticleSystemEnable` | 0/1 | 0 | `0x2522A4E9` Particle System |
| 16 | `g_DepthOfFieldEnable` | 0/1 | 0 | `0xA9C27DFD` Depth Of Field |
| 17 | `g_WorldLodLevel` | 0…3 | 0 | `0xBDE2E94B` World Detail (slider) |
| 18 | `g_CarLodLevel` | 0/1 | 0 | `0x4AC50BCF` Car Geometry Detail (0 Low, 1 High) |
| 19 | `g_OverBrightEnable` | 0/1 | 0 | `0xD6F5F38C` Over Bright |
| 20 | `g_BleachByPassEnable` | 0/1 | 0 | `0x571F6C00` Enhanced Contrast |
| 21 | `g_TintingEnable` | 0/1 | 0 | `0x11504D0E` Tinting |
| 22 | `g_FSAALevel` | 0…N (hardware) | 0 | `0x4CD775AC` Full Screen Anti-Aliasing (slider) |
| 23 | `g_HorizonFogEnable` | 0/1 | 0 | `0x4E57D355` Horizon Fog |
| 24 | `g_RainEnable` | 0/1 | 0 | `0x4CDDBEBB` Rain Splatter |
| 25 | (none — not persisted) | 0/1 | 1 | — |
| 26 | `g_TextureFiltering` | 0…2 | 1 | `0x61A50B8F` Texture Filtering (slider) |
| 27 | `g_RacingResolution` | 0…5 | 0 | `0xA9A76BBA` Resolution |
| 28 | (holds the first-run flag, not this key) | — | — | — |
| 29 | `g_VSyncOn` | 0/1 | 0 | `0x5B4AA6E4` Vsync |
| — | `g_PerformanceLevel` | 0…5 (menu 1…5) | 0 | `0xB55E7665` Level Of Detail (slider) |

Write order is the table order (idx 0…27, then `g_PerformanceLevel`, then `g_VSyncOn`). Read
order is the same. [static] On/off toggles display `0x0000063C` "On" / `0x0000CCFA` "Off"
(Vsync uses `0x88576CFF` for its "Off"). [static]

Read semantics: each value is queried with a 4-byte buffer. If the query fails (absent, or
larger than 4 bytes) the current value is kept. The value *type* is not checked. No range
check is made at read time; range enforcement happens only through the capability clamp (§4.3).
[static]

Meaning of the enumerated values [static unless marked]:

* `g_RacingResolution`: 0 = 640×480, 1 = 800×600, 2 = 1024×768, 3 = 1280×960,
  4 = 1280×1024, 5 = 1600×1200. Used for the whole game (front end included) despite the name.
  Full-screen, 32-bit. [inferred for depth]
* `g_TextureFiltering`: 0 = bilinear, 1 = trilinear, 2 = anisotropic (anisotropic falls back to
  trilinear on hardware without the capability). [inferred from the sampler states chosen]
* `g_FSAALevel`: 0 = off; n > 0 = multisampling enabled with quality level n. The maximum is
  (number of quality levels the display adapter reports) − 1. [static]
* `g_VSyncOn`: 1 = present on vertical retrace, 0 = present immediately. [static]
* `g_PerformanceLevel` additionally selects the audio output rate: level 0 or 1 → 22 050 Hz,
  otherwise 44 100 Hz (unless an internal override is set). [static]

### 2.4 Language

`Language` is compared case-insensitively against this table; the first match sets the
language index. If the value is **absent**, the index is 0. If it is present but matches
nothing, the index keeps its built-in initial value −1 (behaviour from there is unknown, §11).
[static]

| Registry string (functionally required) | Language index |
|---|---:|
| `English US` | 0 |
| `English UK` | 0 |
| `French` | 1 |
| `German` | 2 |
| `Italian` | 3 |
| `Spanish` | 4 |
| `Dutch` | 5 |
| `Swedish` | 6 |
| `Danish` | 7 |
| `Korean` | 8 |
| `Chinese (Traditional)` | 9 |
| `English China` | 9 |
| `Japanese` | 10 |
| `Thai` | 11 |

No options-screen item edits the language [inferred: none of the option handlers found
references it]. The language index feeds two settings defaults:

* Speedometer units default (§5.3): index 0 → MPH, any other → km/h. Selecting a language also
  *forces* the stored units option to that value. [static]
* Number formatting (not stored): index 1 uses space as thousands separator and comma as decimal
  mark; indices 2…7 use `.` and `,`; others use `,` and `.`. [inferred: which argument is the
  thousands vs decimal separator]

## 3. Profile save container

### 3.1 Location and naming [static]

* Root directory: `<CSIDL_LOCAL_APPDATA>\NFS Underground 2` (Windows XP:
  `C:\Documents and Settings\<user>\Local Settings\Application Data\NFS Underground 2`;
  Vista+: `C:\Users\<user>\AppData\Local\NFS Underground 2`). The game creates it if absent.
  `NFS Underground 2` is functionally required.
* One profile = one sub-directory named after the profile, containing one file with the **same
  name and no extension**: `<root>\<NAME>\<NAME>`. Paths are built with `/` separators.
* Profile names are at most 15 bytes (stored in a 16-byte, NUL-terminated buffer); the
  directory/file component is limited to 31 bytes. [static] Names must be valid Windows file
  names (a name that is not cannot be saved). [inferred]
* Start-up check: the root is listed; if it contains anything besides `.` and `..`, "saves
  exist" is set (used to decide whether to show the load flow). [static]
* Deleting a profile deletes every file in its directory, then the directory. [static]

### 3.2 File layout

```
offset  size  field
0       4     magic, the bytes "20CM" (0x32 0x30 0x43 0x4D)  — functionally required
4       4     u32 total file size = 28 + sizeA + sizeB
8       4     u32 sizeA
12      4     u32 sizeB
16      4     u32 checksum(block A)
20      4     u32 checksum(block B)
24      4     u32 checksum(bytes 0..23 of this header)
28      sizeA block A
28+sizeA sizeB block B
```
[static]

For a game profile: block A is 8 bytes `{u32 version = 0x10D (269), u32 payload size}` and
block B is the payload (§5). Payload size is 54 930 bytes, so a profile file is **54 966**
bytes. [static]

### 3.3 Write semantics [static]

1. Build the payload (§5) and block A; compute the three checksums (block checksums first, then
   the header checksum over the first 24 header bytes with the header checksum field not yet
   included).
2. Create `<root>\<NAME>` (ignore "already exists").
3. Open `<root>\<NAME>\<NAME>` for writing, create/truncate, binary.
4. Write header (28 bytes), block A, then block B. A short write is an I/O error.
5. Not atomic: there is no temporary file or rename. A crash mid-write leaves a truncated file,
   which the loader rejects (§3.5). [static]

Before writing, the target directory is listed; if it already holds a save the front end asks
for confirmation to overwrite [static]. With autosave on, same-name saves are overwritten
without asking (the confirmation text `0xF3568B86` says so) [inferred].

Block B is written only when block A is present; for game profiles it always is. [static]

### 3.4 Checksum

`checksum(D)`, D a byte string of length n:

* n < 4 → 0.
* Otherwise, using the MSB-first CRC-32 table for polynomial 0x04C11DB7
  (`T[i]` = i<<24 shifted left 8 times, XOR 0x04C11DB7 whenever the top bit falls out):
  ```
  r = NOT(big-endian u32 of D[0..3])
  for each byte b in D[4..n-1]:
      r = ((r << 8) | b) XOR T[r >> 24]        (32-bit arithmetic)
  return NOT r
  ```
  [static]

Equivalent closed form (verified numerically for n = 4…199): `checksum(D) =
CRC32_BZIP2(D[0..n-5]) XOR bigendian_u32(D[n-4..n-1])`, where CRC32_BZIP2 is the standard
CRC-32/BZIP2 (poly 0x04C11DB7, init 0xFFFFFFFF, no reflection, xorout 0xFFFFFFFF). Note this
is **not** CRC32_BZIP2(D). The result is stored little-endian. Vectors: `tv01_checksum.json`.

### 3.5 Load and validation [static]

The loader rejects the file — outcome in brackets — when any of these hold, checked in order:

1. File cannot be opened [not found].
2. File length cannot be determined, or fewer than 28 bytes can be read [I/O error].
3. Header `total` ≠ actual file length, or magic ≠ `20CM`, or header checksum mismatch
   [corrupt].
4. Reading sizeA or sizeB bytes fails [I/O error].
5. checksum(block A) ≠ stored, or checksum(block B) ≠ stored [corrupt].

Then, at game level:

6. Block A version ≠ 0x10D, or block A payload size ≠ 54 930 → the profile is refused with an
   "incompatible/corrupt" message and **no** settings are applied. [static]

There is no partial recovery and no migration from other versions. A rejected file is not
deleted or rewritten automatically. [static]

### 3.6 Profile life-cycle (settings-relevant parts)

* A fresh profile starts from the defaults in §5 (in-memory state at boot), not from another
  profile. [static]
* Loading copies every stored settings field over the in-memory state, then *applies*
  (§5.10). Settings not stored in the profile (registry graphics, language) are untouched.
* "Unsaved changes" detection: the game keeps a byte snapshot of the serialized payload taken
  at load/save; it later re-serializes and compares byte-for-byte (all 54 930 bytes, so option
  changes count). Mismatch → the save prompt / autosave path is taken (§5.8). [static]

## 4. Graphics settings behaviour

### 4.1 Start-up order [static]

1. Enumerate display modes; mark which of the six resolutions (§2.3) the adapter supports at
   32-bit colour. Some old adapters are restricted to lower indices (vendor/device list,
   irrelevant on modern hardware).
2. Initialise the graphics block to the static defaults (§2.3).
3. Read install values (§2.2) and graphics values (§2.3) from the registry.
4. Create the display with the current values. On failure: if FSAA > 0, set FSAA = 0, the FSAA
   cap = 0 and resolution = 0, and retry once.
5. Determine capability maxima ("caps", §4.3).
6. If this is the first run (`NotFirstTime` was absent) **or** safe mode is on (§4.6): run
   auto-detect (§4.2). In safe mode, additionally write all graphics values to the registry now.
7. Apply the capability clamp (§4.3).

### 4.2 Auto-detect and presets

Auto-detect: compute the performance level P (§4.5; 0 in safe mode), set resolution to 0,
apply the preset for P, apply vendor-specific FSAA overrides, clamp. The menu item
`0x13F0EDEB` "Default Settings" (after confirmation `0x2BBCCA6B` "Reset to default settings?")
runs the same auto-detect. [static]

Preset for level P (applied to the current values; anything not listed is left unchanged —
notably `g_AnimatedTextureEnable`, `g_CarLightingEnable`, `g_CarDamageEnable` and, except as
stated, the resolution): [static]

| Value | P ≤ 1 | P = 2 | P = 3 | P = 4 | P = 5 |
|---|---|---|---|---|---|
| CarShadow | 0 | 2 | 2 | 2 | 2 |
| Tinting, LightGlow, Crowd, CarHeadlight | 0 | 1 | 1 | 1 | 1 |
| WorldLod | 0 | 1 | 2 | 2 | 2 |
| RoadReflection | 0 | 0 | 2 | 3 | 3 |
| HorizonFog, OverBright, CarLod, DepthOfField, ParticleSystem, LightStreaks, MotionBlur, Fog, CarEnvironmentMapUpdateData | 0 | 0 | 1 | 1 | 1 |
| CarEnvironmentMap | 0 | 0 | 1 | 2 | 3 |
| Rain, BleachByPass | 0 | 0 | 0 | 1 | 1 |
| FSAA | 0 | 0 | 0 | 1 | cap |
| TextureFiltering | 1 | 1 | 1 | 1 | 2 |
| VSync | 0 | 0 | 0 | 0 | 0 |
| Resolution | — | — | — | 1 if GPU score (§4.5) > 4 else unchanged | same as P=4 |

Then clamp (§4.3). Vectors: `tv04_video_presets.json` (with a "fully capable" cap set).

Changing the Level Of Detail slider sets `g_PerformanceLevel` to the new value and applies
the preset + clamp immediately (the resolution is not reset first). [static]

### 4.3 Capability clamp [static]

Each value with a cap is lowered to the cap if greater (never raised; no lower bound — a
negative registry value survives). Caps come from adapter capabilities. On a fully capable
modern adapter they are: CarEnvironmentMap 3, CarEnvironmentMapUpdateData 1, CarShadow 2,
CarHeadlight 1, **CarLighting 0, CarDamage 0** (never raised by anything — these two are
effectively always off), Crowd 1, RoadReflection 3, Fog 1, MotionBlur 1, LightStreaks 1,
LightGlow 1, AnimatedTexture 1, ParticleSystem 1, DepthOfField 1, WorldLod 3, CarLod 1,
OverBright 1, BleachByPass 1, Tinting 1, FSAA (levels − 1), HorizonFog 1, Rain 1,
idx 25 → 1, TextureFiltering 2. [static; "fully capable" values inferred]

Resolution: if the stored index is not supported by the adapter, step down to the highest
supported lower index; if none is supported the value is left unchanged. [static]

The clamp also runs after a profile load (§5.10). [static]

### 4.4 Options screen and persistence [static]

* Toggles flip 0↔1 on left/right/accept, but only if their cap is non-zero (Vsync has no cap
  and always flips).
* Sliders range from 0 to the cap (Level Of Detail: 1…5, step 1). Slider positions are
  converted to integers by truncation.
* Resolution left/right moves to the previous/next *supported* index (not wrapping, 0…5); if
  no supported index exists in that direction, the value is unchanged.
* Some changes (resolution, texture filtering, vsync, FSAA, road reflection, light trails,
  light glow, motion blur, depth of field, over bright, enhanced contrast, car reflection rate,
  level of detail) mark the display as needing re-initialisation; it happens when leaving the
  screen. [static; which items need a restart vs a reset is inferred]
* All graphics values are written to the registry when leaving the display options screen
  (from the main menu or the pause menu), on safe-mode start, and on one window-message path
  (inferred: application close). [static]
* Display re-initialisation failure: FSAA → 0, retry up to 3 times, then lower the resolution
  index one step at a time until it succeeds or reaches 0; a failure message is shown. [static]

### 4.5 Performance-level auto-detection [static]

```
cpuScore = 1 if MHz < 1400; 2 if < 1800; 3 if < 2400; else 4     (unknown CPU vendor: MHz = 0)
gpuScore = per-(PCI vendor, device) table value, 1.0 if not listed
P = trunc(0.2 * gpuScore + 0.8 * cpuScore)
```
Overrides (vendor-specific, irrelevant on modern hardware, not specified further): Intel and XGI
adapters and a few legacy ATI/SiS devices give P = 0; two low-end nVidia devices are capped at 3; safe mode gives 0. The device table has 120
entries with scores 0…5, so auto-detect yields at most 4; level 5 is reachable only through the
slider or registry. Vectors: `tv05_autodetect.json`.

A reimplementation on modern hardware should treat the GPU as score 5 and the CPU as score 4
(P = 4), or simply offer P as a user choice. [inferred recommendation]

### 4.6 Safe mode

Launching with first argument `/s` (as the install's `SafeMode.bat` does) sets safe mode:
P = 0, auto-detect runs on every such start, and the result is written to the registry
immediately. [static; whether the comparison is case-sensitive is unknown]

## 5. Profile payload layout (block B, 54 930 bytes)

### 5.1 Map

The payload is the concatenation below. Offsets are from the start of block B (add 36 for file
offsets). "Opaque" regions must be preserved byte-exactly by a settings backend that writes
existing profiles. [static]

| Payload offset | Size | Content | Settings? |
|---:|---:|---|---|
| 0 | 50 211 | career, garage, cars, unlocks, statistics | opaque |
| 50 211 | 216 | EA TRAX playlist (§5.6) | yes |
| 50 427 | 60 | audio block (§5.2) | yes |
| 50 487 | 16 | gameplay block (§5.3) | yes |
| 50 503 | 80 | two player records (§5.4) | yes |
| 50 583 | 16 | unknown block (§5.9) | preserve |
| 50 599 | 380 | career progress (unidentified) | opaque |
| 50 979 | 4 | u32 career difficulty (§5.7) | yes |
| 50 983 | 4 | u32 unidentified | opaque |
| 50 987 | 1 | u8 autosave flag (§5.8) | yes |
| 50 988 | 1 | u8 save-prompt flag (§5.8) | yes |
| 50 989 | 4 | u32 unidentified (unaligned) | opaque |
| 50 993 | 2 640 | unidentified | opaque |
| 53 633 | 128 | unidentified | opaque |
| 53 761 | 32 | profile name, NUL-padded (§5.9) | identity |
| 53 793 | 51 | online options (§5.5) | yes |
| 53 844 | 1 086 | input devices, 6 × 181 (§6) | yes |

Total 54 930. The writer checks each field against the buffer end before writing it, so a
shorter buffer would silently truncate; with the fixed size everything fits exactly. The
reader applies the same per-field checks. [static]

Settings section = payload bytes 50 211 … 54 929 (4 719 bytes). Offsets in §5.2–§6 are
**relative to the start of their block** unless stated.

### 5.2 Audio block (60 bytes) [static]

| Off | Type | Option (key → English) | Domain | Default |
|---:|---|---|---|---:|
| 0 | f32 | `0x26782C3E` Master Volume | 0.0…1.0, step 0.05 | 1.0 |
| 4 | f32 | `0x9E5FB82A` Speech & Movie Volume | same | 1.0 |
| 8 | f32 | `0x418E681D` Menu Music Volume | same | 0.7 |
| 12 | f32 | `0xDF21EAC2` Game Music Volume | same | 1.0 |
| 16 | f32 | `0xFD487543` Sound Effects Volume | same | 1.0 |
| 20 | f32 | `0xA2B1F888` Engine Volume | same | 1.0 |
| 24 | f32 | `0x69B745C6` Tire Volume | same | 1.0 |
| 28 | f32 | unidentified level (the audio system truncates it to an integer) | — | 1.0 |
| 32 | f32 | unidentified | — | 1.0 |
| 36 | f32 | unidentified | — | 1.0 |
| 40 | u32 | `0x2881AB87` Audio Mode: 0 `0xC50FA35F` Mono, 1 `0x55DA8BF8` Stereo | 0/1, wraps | 1 |
| 44 | u32 | unidentified | — | 0 |
| 48 | u32 | EA TRAX play order: 0 `0x4CA36B89` Sequential, 1 `0xA9C9C8F7` Random | 0/1 toggle | 1 |
| 52 | u32 | unidentified | — | 0 |
| 56 | u32 | unidentified | — | 0 |

Volume sliders apply to the audio system immediately on change. [static]

### 5.3 Gameplay block (16 bytes) [static]

| Off | Type | Option | Domain | Default |
|---:|---|---|---|---:|
| 0 | u8 | `0x85A6CE05` Rearview Mirror | 0 Off / 1 On | 1 |
| 1 | u8 | unidentified | — | 0 |
| 2 | u8 | `0xF86A6272` Speedometer Units | 0 `0x8569AB44` MPH / 1 `0x8569A25F` KM/H | language ≠ 0 (§2.4) |
| 3 | u8 | `0x9FA5EC9E` Race Map Mode | 0 Off, 1 `0xF75595F2` Static, 2 `0xF4B00E99` Rotate | 2 |
| 4 | u8 | `0xC6269082` Explore Map Mode | same as above | 2 |
| 5–7 | — | padding | — | 0 |
| 8 | f32 | `0xF26A5CBF` Jump Camera | 0.0 = Off; any other value = On | 127.5 |
| 12 | f32 | unidentified, read by cinematic camera code | — | 127.5 |

Jump Camera toggle: if the value is exactly 0.0 it becomes 127.0, otherwise 0.0 (so a fresh
profile holds 127.5, a re-enabled one 127.0). The race map mode is copied into the race at race
start; the explore map mode at free-roam start. [static]

### 5.4 Player records (2 × 40 bytes) [static]

Record 0 = player 1, record 1 = player 2 (split screen). Each:

| Off | Type | Option | Domain | Default |
|---:|---|---|---|---:|
| 0 | u8 | `0xAC148579` Gauges | 0 Off / 1 On | 1 |
| 1 | u8 | `0x82CD8F92` Race Position | 0/1 | 1 |
| 2 | u8 | `0x969F96A5` Lap Info | 0/1 | 1 |
| 3 | u8 | `0xC03F9F19` Score | 0/1 | 1 |
| 4 | u8 | `0x358CDCFD` Feedback: (controller vibration/force feedback) | 0/1 | 1 |
| 5 | u8 | `0xC93FBFB5` Leaderboard | 0/1 | 1 |
| 6 | u8 | `0xBFB34120` Ask for Transmission | 0/1 | 1 |
| 7 | — | padding | — | 0 |
| 8 | u32 | controller configuration preset index [inferred] | 0…8 [inferred] | 0 |
| 12 | u8 | unidentified (+3 padding) | — | 0 |
| 16 | u32 | `0xF6CCDC5F` Camera: 0 Bumper, 1 Hood, 2 Close, 3 Far, 4 Super Far | 0…4 | 3 |
| 20 | u8 | `0x084BC378` Split Time: 0 `0xD93F8F55` Leader, 1 `0xC44D3943` Lap Record, 2 `0x17FAFC32` Best Lap, 3 `0x1EA459F8` Last Lap, 4 Off | 0…4 | 0 |
| 21 | u8 | `0xD31407E7` Transmission: 0 `0x61CB0720` Auto, 1 `0x1B3A2485` Manual | 0/1 | 0 |
| 22 | u8 | `0xDF06ED22` Stability Control: **0 = On, 1 = Off** | 0/1 | 1 |
| 23 | u8 | unidentified; consulted at race start when the race's own setting is 3 | — | 0 |
| 24 | u8 | unidentified | — | 1 |
| 25 | u8 | unidentified; copied into the input/steering handler with offsets 28 and 32 | — | 0 |
| 26–27 | — | padding | — | 0 |
| 28 | f32 | `0x56FEB0D2` Wheel Range | slider 0.1…1.0, step 0.1 | 1.0 |
| 32 | f32 | `0x1CC4B396` Road Feedback (stored inverted, below) | 2.0…20.0 | 12.4 |
| 36 | f32 | `0x28F42AE8` Overall Force Strength | slider 0.0…1.0, step 0.1 | 1.0 |

Camera behaviour is specified in docs/cleanroom-camera §2.2–2.3: when entering free roam or a
non-drift event, Super Far (4) is replaced by Far (3); drift events force 4. [static]

Road Feedback mapping: slider position s ∈ [0,1] (step 0.1) ↔ stored v by
`v = 20 − 18·clamp(s,0,1)` and `s = 1 − (clamp(v,2,20) − 2)/18`. [static for the write
direction and the clamp; the read-direction scale is inferred as 1/18]

Wheel options are only reachable with a wheel connected (message
`0x53691B86`). [static]

### 5.5 Online options (51 bytes) [static]

| Off | Type | Option | Domain | Default |
|---:|---|---|---|---|
| 0 | u32 | always written 0, ignored on read | — | 0 |
| 4 | u32 | always written 0, ignored on read | — | 0 |
| 8 | u8 | `0x0B660729` Show Player Names | 0/1 | unknown |
| 9 | u8 | `0xD1009B34` Bad Game Connection Warning | 0/1 | unknown |
| 10 | u8 | unidentified online flag | — | unknown |
| 11 | 5 × u32 | unidentified online preferences | — | unknown |
| 31 | 5 × u32 | unidentified online preferences | — | unknown |

Fields are byte-packed (the u32 arrays start at odd offsets).

### 5.6 EA TRAX playlist (216 bytes) [static]

27 entries × 8 bytes, in **playlist order**:

| Off | Type | Meaning |
|---:|---|---|
| 0 | u32 | track id 0…26 (index into the game's built-in track list) |
| 4 | u8 | play mode: 0 off, 1 front end only, 2 in game only, 3 both |
| 5–7 | — | padding |

Mode cycles 0→1→2→3→0 (and backwards). Re-ordering in the EA TRAX screen rewrites the whole
table from the on-screen order. After load (and after every change) the game derives two
27-bit masks (front-end tracks, in-game tracks) from the modes and hands them, with the play
order (§5.2 offset 48), to the music player. The intended content of the table for a profile
that never visited EA TRAX is **unknown** (§11): the in-memory initial state is all zero
bytes, while the music player independently starts with all 27 tracks enabled. [static]

### 5.7 Career difficulty [static]

u32: 0 `0xDBBB4E8F` Easy, 1 `0xCAB262DE` Medium, 2 `0xDBBCF37C` Hard; left/right wraps within
0…2. Label `0xBC0E6C55`. Default: no initialiser was found, so 0 [inferred]. Copied into
race set-up at race start.

### 5.8 Save behaviour flags [static]

Option `0x996463E6` "Save" combines two bytes:

| autosave (50 987) | prompt (50 988) | Shown |
|---:|---:|---|
| 1 | any | `0x1B7ABF8E` Autosave |
| 0 | 1 | `0x1B7ABF8F` Prompt |
| 0 | 0 | `0x1B7ABF90` Off |

Cycling right: Off → Autosave → Prompt → Off; cycling left: Off → Prompt → Autosave → Off.
Autosave is skipped (right: Off → Prompt; left: Prompt → Off) when a game-state condition
fails — inferred to be "a profile exists that can be saved". Selecting Autosave sets **both**
bytes to 1; Prompt sets autosave 0 / prompt 1; Off sets both 0. [static] Leaving the gameplay
screen after turning prompts off, or autosave on, shows a confirmation (`0x22BC94AF`,
`0xF3568B86`). The memory-card flow also sets the
autosave flag: a successful save/create sets it to 1; declining or a failure sets it to 0.
[static; the exact trigger list is inferred]

Autosave semantics: when the "unsaved changes" check (§3.6) finds a difference at a save point,
the profile is overwritten without asking. Prompt: the player is asked. Off: nothing happens.
[inferred]

### 5.9 Identity and unknown blocks

* Profile name: 32 bytes, NUL-padded ASCII; equals the directory/file name. [static]
* Unknown block at 50 583 (16 bytes): f32 (default 0.5), u32 0, u32 0, u8 0, 3 padding bytes.
  Preserve. [static layout and defaults; meaning unknown]

### 5.10 Applying a loaded profile [static]

After a successful load:

1. Copy every field (§5.1) into the in-memory state.
2. Re-apply the audio block to the audio system (volumes and mode).
3. Rebuild the EA TRAX masks from the playlist.
4. Re-run the graphics capability clamp (§4.3) — graphics values themselves are not in the
   profile, so this only re-clamps the registry-derived state.
5. For each player, select the controller configuration given by record offset 8.
6. Match input-device records to connected devices (§6).

Values are **not** range-checked on load beyond step 4. Out-of-range bytes produce undefined
labels (blank) in the menus; the camera falls back to Far (docs/cleanroom-camera §2.2). [static]

## 6. Input devices (6 × 181 bytes)

Six slots correspond to the game's six device slots, in enumeration order. A slot index below
the number of devices in use is written with the device's data; others are written as zeros.
[static]

| Off | Size | Field |
|---:|---:|---|
| 0 | 16 | device instance GUID (DirectInput); for the system keyboard, a GUID the game stores itself |
| 16 | 4 | i32 steering axis id |
| 20 | 4 | i32 throttle axis id |
| 24 | 4 | i32 brake axis id |
| 28 | 4 | i32 combined-pedal axis id |
| 32 | 4 | i32 steering direction (+1 / −1) |
| 36 | 4 | i32 throttle direction |
| 40 | 4 | i32 brake direction |
| 44 | 4 | i32 combined-pedal direction |
| 48 | 4 | four u8 flags: steering, throttle, brake, combined axis assigned |
| 52 | 1 | u8 combined-pedals mode (1 = throttle and brake share one axis) |
| 53 | 128 | 32 × i32 binding table (button / control codes per action slot) |

[static layout; axis ids are inferred to be byte offsets into the DirectInput joystick state
(0 = X, 4 = Y, 8 = Z, 12/16/20 = rotations, 24/28 = sliders) because the defaults use 0, 4, 20
and 24]

Axis calibration: when the user moves an axis while assigning throttle or brake, if the other
pedal is already on the same axis with the opposite direction, the device switches to combined
mode (combined axis = that axis, direction = the brake direction / negated throttle
direction). Assigning one axis clears the "assigned" flag of any other role that used the same
axis. [static]

Device defaults (no record matched): steering = axis 0, direction +1, assigned; combined mode
on with combined axis 4 (Y) direction +1 (20 or 24 on wheels with separate pedal sliders);
throttle/brake directions −1, unassigned. Button bindings: a default table depending on the
number of buttons (< 6, 6–7, ≥ 8) and the POV hat. [static; exact binding values listed in
§11 as not yet decoded]

Load: each stored record whose GUID equals a currently connected device's GUID overwrites that
device's axis and binding fields; records with no match are ignored; devices with no record
keep their defaults. Zeroed slots never match. [static]

## 7. Things that are *not* persisted [static]

* Online lobby/race options (lap count, traffic level, AI skill, catch-up, number of
  opponents, track direction, etc.) — per session.
* The "current options screen" selector and the "name entered" flag that sit next to the
  stored fields in memory.
* Graphics values are never in the profile; audio/gameplay values are never in the registry.

## 8. Edge cases summary

| Situation | Behaviour |
|---|---|
| No registry key | static defaults; first-run auto-detect; key created on first write [static] |
| Registry value with wrong size | ignored (keeps current) [static] |
| Registry value out of range | kept until clamped; values below 0 or above the menu range but under the cap survive [static] |
| Profile file truncated/extended | rejected as corrupt (total-size check) [static] |
| Bit flip anywhere | rejected (header or block checksum) [static] |
| Version ≠ 0x10D or wrong payload size | rejected at game level [static] |
| Profile from a different machine | loads; input records simply do not match devices [static] |
| Language changed between runs | units option is overwritten on language selection [static] |

## 9. Recommended model for OpenUG2 (non-normative)

The original splits settings between a machine-wide registry key and per-profile save. An
interoperable backend should:

1. Keep two stores with the same split (machine: §2.3/§2.4 values; profile: §5).
2. Read/write the profile container byte-exactly (§3), preserving opaque regions, so retail
   saves round-trip.
3. Offer import of a `.reg` export of §2 if registry access is not available on the host
   platform. [guessed usefulness]

## 10. Test vectors

See `test-vectors/README.md`. All are synthetic.

## 11. Unknowns / ambiguities

1. EA TRAX playlist content for a profile that never visited the EA TRAX screen (all zero in
   memory vs. the player's own all-enabled state). A runtime save from a fresh profile would
   settle it.
2. Online option defaults (§5.5 offsets 8–10) and the meaning of the two 5-word arrays.
3. Meaning of: audio offsets 28/32/36/44/52/56; gameplay offset 1 and 12; player offsets 12,
   23, 24, 25; the 16-byte unknown block; the 380/2 640/128-byte and two single-word opaque
   regions (likely career progress, map filters, statistics).
4. Binding-table action-slot enumeration and default binding codes (§6); keyboard record GUID.
5. Career difficulty default (0 inferred) and whether it is shown in the retail menus at all.
6. Behaviour when `Language` holds an unknown string (index −1).
7. Whether `/s` is matched case-insensitively.
8. Exact list of save points that trigger the autosave/prompt check.
9. Exact set of display changes needing a full device reset vs. an in-place change.
10. Behaviour with `g_RacingResolution` > 5 (resolution lookup leaves the size unset).

## 12. Capture requests (for a runtime session)

1. Create a new profile, save immediately, and keep the file (resolves §11.1, §11.2, §11.5).
2. Change each audio/gameplay option once and diff successive saves to confirm offsets.
3. Export the registry key after first run (auto-detect result) and after "Default Settings".
4. Bind each control once on a gamepad and diff the input-device block (§11.4).
