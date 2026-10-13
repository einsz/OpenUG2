# Underground 2 frontend GUI library — behavioural specification

Status: revision 7 (reviewed final-render dialog acceptance, runtime-check inputs, demo composition and verified Xbox source, PC text-only pills, original-only visuals, corner transforms and console profiles; approval hashes recorded in REVIEW.md; changes listed in AMENDMENTS.md). Audience: an implementer who has never seen the
original program or its data. Everything here is described in fresh terms;
retail identifiers are avoided except where a value is *functionally
required* for interoperability with the retail screen files (hash constants,
chunk/tag magic numbers, the `.fng` extension). Such values are flagged with
**[interop]**.

Confidence legend used per section: **O** = observed directly in data,
**I** = inferred from consistent data patterns or public documentation,
**G** = guessed (no direct evidence; a reasonable default the implementer may
change), **R** = observed (measured) on reference captures of the running
original PC game (amendment 3; capture set described in 19.1; values in
virtual units, ±1 unless stated). The legend letter R is unrelated to the
numbered capture-plan entries R1–R14 of section 18.

Goal served by this document: a GUI widget library able to (a) optionally
load retail-format screen packages and (b) present every widget kind of the
original frontend in a demo window. Amendment 5 restores the original PC text-only pill buttons as the default
desktop presentation, with selectable PS2/Xbox console profiles and genuine
game assets; desktop mouse input remains available. Section 20 supersedes all
older permissions for invented visuals, mouse/key glyph artwork, invented
pill-grid demo layouts and no-install visual fallbacks.

---

## 1. Coordinate space, drawing model, colour (O/I)

### 1.1 Virtual screen
* All layout is expressed in a **640 x 480 virtual screen**. (I)
* The **origin (0,0) is the centre of the screen**; **+x is right, +y is
  down**. Visible range is x in [-320, +320], y in [-240, +240]. (I — every
  retail screen's visible content lies in this range; titles sit near
  y = -215, the bottom information strip at y = +206.)
* Content intentionally parked off-screen (|x| > 320 or |y| > 240) is
  invisible; the retail screens use this to keep pools of template objects
  that the program positions at run time. (O)
* Safe area (I): retail content keeps roughly a 20-24 unit margin from every
  edge; panels span x = -310 .. +310 and y = -222 .. +222 at most. A 600 x 440
  safe rectangle centred on the origin reproduces the original framing.
* A real window maps the virtual screen uniformly. For non-4:3 windows
  (amendment 3, R): in a 1920 x 1080 capture the whole frame, including the
  3D background, is drawn into a centred 4:3 viewport of 1440 x 1080 pixels
  (scale `H/480` = 2.25, x offset 240). The side bars are pure black
  (0,0,0) and nothing, not even the pointer cursor, is drawn into them.
  So: scale = `min(W/640, H/480)`, centre, clip to the viewport, fill the
  rest with black. Caveat: the capture came from an install with
  third-party modifications, so it is not proven that this presentation
  is unmodified retail behaviour (11, item 12).

### 1.2 Object transform
Each object has: pivot (x,y,z), position (x,y,z), rotation (unit quaternion
x,y,z,w), size (x,y,z), colour (RGBA). (O)

* **Groups** transform their children: a child's position, rotation, scale
  and depth are relative to its parent. (I — verified on many screens, e.g.
  a highlight frame nested two levels deep lands exactly on the template slot
  it frames.)
* For **images**, `size.x`/`size.y` are the drawn width/height in virtual
  units; the quad spans `[-size/2, +size/2]` around the position, i.e.
  **position is the centre** of the image. Negative size mirrors the image on
  that axis (used for corner pieces). (O/I)
* For **groups** and **text**, `size` is a **scale factor** (groups are
  almost always 1.0; text is typically 0.45 – 1.15). (O)
* Rotation is applied about the pivot (pivot is relative to the object
  position; retail pivots are almost always 0). Retail screens use rotation
  mainly to orient corner pieces in 90° steps. (O)
* World transform of an object = its local Scale, then Rotate (about pivot),
  then Translate, then the parent's world transform. (I)

### 1.3 Depth and draw order
* `position.z` is a depth value; **smaller z is nearer the viewer** (drawn
  later). Children's z is relative to the parent's z. Typical values: panels
  100–500 (back), text 1–20 (front). (I)
* Objects are drawn back-to-front by accumulated z. For equal z, order is
  unknown (see Unknowns); use list order (earlier first) (G).

### 1.4 Colour and alpha
* Colours are 8-bit-per-channel RGBA (0–255). Stored order in files is
  **B, G, R, A** (each as a signed 32-bit integer, see 3.4.3). (I)
* An object's effective colour = its own colour multiplied channel-wise by
  its parent's effective colour, per channel `c = (a*b + 127) / 255`
  (integer division), so 255 x 255 = 255. This is how fading a group fades
  all its children. (I for the product; G for the exact rounding, which data
  cannot reveal. Amendment 2 replaces the earlier `>> 8` form.)
* Images: drawn pixel = texture texel x effective colour (modulate), then
  alpha-blended `src*a + dst*(1-a)`. Untextured images use a white texel. (I)
* Animated colours can temporarily exceed 0..255 before clamping (track base +
  delta); clamp each channel to 0..255 after evaluation. (O)
* Retail palette (O):

| Use | RGB | Typical alpha |
| --- | --- | --- |
| Accent (titles, focused text, highlight frames) | 202,244,138 (also 204,244,138) | 255 |
| Fill/progress green (slider fill, stat bars) | 99,178,34 | 255 |
| Dialog border | 197,218,165 | 255 |
| Dark text on light rows | 50,50,50 / 60,60,60 | 255 |
| Light body text / player strip | 230,230,230 | 255 |
| Secondary label (hint labels) | 192,192,192 | 255 |
| Dimmed / disabled text | (same RGB) | 120 |
| Dark panel fill (texture is black at 60%) | 0,0,0 | 153 x object alpha (objects use 150–255) |
| Light panel fill (texture is white at 60%) | 225,225,225 tint | 153 x object alpha (objects use ~200) |

---

## 2. Name hash (O) [interop]

All identifiers in the data (screens, objects, scripts, messages, string
labels, texture keys) are 32-bit hashes of ASCII strings:

```
h = 0xFFFFFFFF
for each byte c of the string:  h = (h * 33 + c) mod 2^32
```

* The empty string hashes to 0xFFFFFFFF; "A" to 0x00000020.
* Case matters to the function itself. The data uses **upper-case** inputs
  for: screen file names (e.g. the file name with extension, upper-cased),
  object names, script names, message names, string labels, and texture keys.
* Texture key = hash of the upper-cased file base name **without** extension
  (O). Resource-request ids (3.3.4) are a different, opaque value (except the
  untextured placeholder whose id is the hash of its upper-cased name).
* Vectors: `test-vectors/hash_vectors.tsv`.

---

## 3. Screen package format (`.fng`) (O unless noted)

### 3.1 Primitive encoding
* Little-endian throughout. `u32`/`i32` 4 bytes, `u16` 2 bytes, `f32` IEEE
  single.
* **Chunk** = `u32 id` + `u32 size` + `size` bytes payload. All chunk payload
  sizes in retail files are multiples of 4.
* **Tag** (inside certain chunks) = `u16 id` + `u16 size` + `size` bytes
  payload. All tag sizes are multiples of 4 (strings are NUL-padded).
* Four-character ids below are written as the ASCII of their little-endian
  bytes; `\xNN` denotes a non-ASCII byte. [interop]

### 3.2 Containers that carry packages (outside the package)
Packages are stored inside larger resource bundles:

| Chunk id | Payload |
| --- | --- |
| `0x00030210` | `u32 screenHash` + one *compressed or raw package*. `screenHash` = hash(upper-case file name, e.g. `"MENU.FNG"`). The package bytes start with `"JDLZ"` (13.2), `"HUFF"` (13.3; used only by 6 tiny packages) or directly with the package chunk. The hash prefix equals hash(upper-case `PkHd` name) (14.3). |
| `0x00030203` | One raw package (no hash prefix). The 2023-modified installation stores every package this way (14.3). |

Bundles may also hold texture packs, fonts and other chunks that a GUI
loader can skip. Padding chunks have id 0.

### 3.3 Package chunk tree

```
"FEn\xE7" (0xE76E4546)  package root
  "PkHd"  header
  "TypS"  object-type size table
  "Res\xCC"  resources container
      "RsNm"  resource names
      "RsRq"  resource requests
  "Obj\xCC"  objects container
      "Butn"  button count
      "FOb\xEA" (0xEA624F46)  one per object, in file order
          "ObjD"  object data (tags)
          "Scrp"  zero or more, one per script (tags)
          "MsgR"  optional, message responses (tags)
  "PkgR"  optional, package-level message responses (tags)
  "Targ"  message target index (tags)
```
Order observed: PkHd, TypS, Res, Obj, PkgR, Targ. Readers should accept any
order and skip unknown chunk ids. Public tools also know chunks `"MsgN"`,
`"EDat"`, `"TLs\xF4"`; none occur in this game's data.

#### 3.3.1 `PkHd`
| Offset | Type | Meaning |
| --- | --- | --- |
| 0 | u32 | format version, always 0x00020000 |
| 4 | u32 | 0 |
| 8 | u32 | number of resource requests |
| 12 | u32 | number of root objects (objects without a parent) — matched 130/130 packages |
| 16 | u32 | N1 = length of package name including NUL |
| 20 | u32 | N2 = length of source path including NUL |
| 24 | N1 bytes | package name (ASCII, e.g. `Menu.fng`) |
| 24+N1 | N2 bytes | source path (ASCII, backslashes) |
| … | 0–3 bytes | zero padding to 4 |

#### 3.3.2 `TypS`
Array of `{u32 objectType, u32 dataBlockSize}` for each object type present.
Observed sizes: group 68, image 84, text 68, movie 68, colour-gradient
image 148. (These equal the size of the `SA` tag, 3.4.3.)

#### 3.3.3 `RsNm`
Concatenated NUL-terminated ASCII names, then zero padding to 4. A name is
referenced by its byte offset from the start of this payload.

#### 3.3.4 `RsRq`
`u32 count`, then `count` records of 24 bytes:

| Offset | Type | Meaning |
| --- | --- | --- |
| 0 | u32 | resource id (opaque; see 2) |
| 4 | u32 | name offset into `RsNm` |
| 8 | u32 | resource kind: 0 untextured placeholder, 1 image, 2 font, 3 model, 4 movie, 5 effect, 6 animated image, 7 multi-image (kinds 0,1,2,4 observed) |
| 12 | 3 x u32 | zero |

Image names carry an extension (`.tga`, `.bmp`); the texture is found by the
texture key of the base name (section 2). A font resource names a font file.
The special name `(Untextured Simple Image)` with kind 0 means "plain
coloured quad". [interop]

#### 3.3.5 `Butn`
`u32` = number of objects with the *button* flag (3.4.2). (O: equal in every
package that has it.)

### 3.4 Object record (`FOb\xEA`)

#### 3.4.1 `ObjD` tags
Tags appear in this order in retail files: `Ot, Oh, OP, [PA], [If], [text
tags], SA`. A reader should process them in any order.

| Tag | Size | Payload |
| --- | --- | --- |
| `Ot` 0x744F | 4 | u32 object type: 1 image, 2 text, 3 model, 4 list, 5 group, 6 code list, 7 movie, 8 effect, 9 colour-gradient image, 10 animated image, 11 simple image, 12 multi-image. Observed: 1, 2, 5, 7, 9. |
| `Oh` 0x684F | 4 | u32 object name hash (lookup key used by program logic). |
| `OP` 0x504F | 16 | u32 guid (unique within the package; used as message target and parent reference), u32 name hash (always equal to `Oh`), u32 flags (3.4.2), i32 resource index into `RsRq` (-1 none). |
| `PA` 0x4150 | 4 | u32 parent guid. Absent = root object. The parent always appears earlier in the file and is always a group. |
| `If` 0x6649 | 4 | u32 image flags (images only; always 0 observed). |
| `Sb` 0x6253 | 4 | text: u32 buffer capacity in characters (>= default text length + 1). |
| `St` 0x7453 | n | text: default string, UTF-16LE, NUL-terminated, padded to 4. |
| `Sj` 0x6A53 | 4 | text: u32 justification bits (8.4). |
| `Sl` 0x6C53 | 4 | text: i32 extra line spacing ("leading") in unscaled units; usually 0. |
| `Sw` 0x7753 | 4 | text: u32 wrap width in unscaled units; 0 = no wrapping. |
| `SH` 0x4853 | 4 | text: u32 string-label hash for localisation (8.3). |
| `SL` 0x4C53 | n | text: label name (ASCII) — not present in retail data. |
| `On` 0x6E4F | n | object name (ASCII) — not present in retail data ("stripped"). |
| `M1..M3`, `Ma..Mc` | 4 | multi-image extra texture refs / flags — not present in retail data. |
| `SA` 0x4153 | see 3.4.3 | object data block. |

#### 3.4.2 Object flags (u32)
| Bit | Meaning | Confidence |
| --- | --- | --- |
| 0x00000001 | invisible | I (never set in data) |
| 0x00000004 | unknown; set on hint labels and the player-info texts | O (meaning unknown) |
| 0x00000008 | **PC-only**: draw/use only on the PC build | I (77 objects; all are mouse buttons, PC arrows, PC filter labels) |
| 0x00000040 | **console-only**: hidden on PC | I (402 objects; all controller-glyph hints) |
| 0x00080000 | **not reachable by directional focus movement** (pointer-only / code-controlled button) | I |
| 0x08000000 | locked in editor | I |
| 0x10000000 | **button**: can hold focus, receives focus/press messages | O |
| 0x40000000 | set on every object | O |
| 0x80000000 | hidden in editor | I |

#### 3.4.3 `SA` data block layout
Offsets in bytes; "word" = 4 bytes. The word index is also the *property
offset* used by animation tracks (3.5.2).

| Byte off | Word | Field | Type |
| --- | --- | --- | --- |
| 0 | 0 | colour B, G, R, A | 4 x i32 (0–255) |
| 16 | 4 | pivot x, y, z | 3 x f32 |
| 28 | 7 | position x, y, z | 3 x f32 |
| 40 | 10 | rotation quaternion x, y, z, w | 4 x f32 (identity = 0,0,0,1) |
| 56 | 14 | size x, y, z | 3 x f32 |
| 68 | 17 | (images) texture top-left u, v | 2 x f32 |
| 76 | 19 | (images) texture bottom-right u, v | 2 x f32 |
| 84 | 21 | (colour-gradient image) corner colours top-left, top-right, bottom-right, bottom-left, each B,G,R,A | 16 x i32 |
| 84 | 21 | (multi-image, unused) 3 x (u,v) top-left, 3 x (u,v) bottom-right, pivot-rotation x,y,z | 15 x f32 |

Group, text and movie blocks are 68 bytes (words 0–16 only); image 84;
colour-gradient image 148. Texture coordinates are normalised (retail almost
always 0,0 – 1,1).

### 3.5 Scripts (`Scrp`) — animation clips

A script is a named, timed clip that drives object properties and can emit
messages. Every object has at least one script, normally one whose name
hashes to **0x001744B3** (the *initial* script, applied when the screen is
created). [interop]

#### 3.5.1 Script tags (order: `Sh`, [`Sc`], then per track `FI`,[`To`],`Kd`, then [`EV`])
| Tag | Size | Payload |
| --- | --- | --- |
| `Sh` 0x6853 | 16 | u32 script id (name hash), u32 length in ms, u32 flags (bit0 = loop), u32 number of tracks |
| `Sn` 0x6E53 | n | script name — not present in retail data |
| `Sc` 0x6353 | 4 | u32 id of the script to switch to when this one ends ("chain") |
| `FI` 0x4946 | 8 | track header: u8 value type, u8 value size in bytes, u8 interpolation, u8 action flags, u32 (low 24 bits = track length in ms, high 8 bits = 0) |
| `To` 0x6F54 | 4 | u32 property offset in words (3.4.3) that this track drives |
| `Kd` 0x644B | n | keys: repeated `{i32 time_ms, value}`; size = count x (4 + value size) |
| `EV` 0x5645 | 12·n | events: repeated `{u32 message hash, u32 target, u32 time_ms}` |

Value types: 0 bool, 1 int, 2 float, 3 vec2 (8 bytes), 4 vec3 (12),
5 quaternion (16), 6 colour (16, B,G,R,A i32). Observed: 3, 4, 5, 6.
Interpolation: 0 none, 1 linear, 2 spline, 3/4 "move-to" variants. Observed:
1 everywhere (one track uses 3; treat as linear) (O).
Action flags: bit0 = wrap interpolation around the end of a looping track;
bit7 = constant track (only a base key). Values 0, 1, 0x80, 0x81 observed.

If `To` is absent, tracks map positionally: 0 colour, 1 pivot, 2 position,
3 rotation, 4 size, 5 UV top-left, 6 UV bottom-right (I, public tooling).

#### 3.5.2 Keys
* The **first key has time -1 and is the base value**.
* Every following key (ascending time >= 0) is a **delta**; the animated value
  is `base + interpolated delta` (for quaternions: base composed with the
  interpolated delta rotation).
* Quaternion detail (amendment 2): only one retail screen (the custom-HUD
  chooser) has non-identity quaternion deltas: rotations about z of up to
  about 290 degrees. Recommended (G): interpolate deltas component-wise
  (linear, no shortest-path sign flip), normalise, and compose as
  `delta * base` (rotate the base orientation by the delta). Not enforcing
  the shortest path lets a delta of -184 degrees turn the long way, as
  authored. Screenshot request A12 can confirm.
* Example (O): a fade-in colour track has base RGBA = (50,50,50,255) with
  deltas (0,0,0,-255) at 40 ms and (0,0,0,0) at 180 ms → invisible until
  40 ms, then alpha ramps to 255 at 180 ms.
* A zero-length script may still contain delta keys at t = 0; the applied
  value is base + delta(0). (O)

#### 3.5.3 Track evaluation (I; vectors in `test-vectors/track_eval_vectors.json`)
Given keys `(t0,d0) … (tn,dn)` and evaluation time `t`:
1. No delta keys → value = base.
2. Non-wrapping track (action bit0 clear):
   * `t <= t0` → `d0`; `t >= tn` → `dn`;
   * otherwise find `ti <= t < ti+1` and linearly interpolate.
3. Wrapping track (action bit0 set, length L > 0, t already in [0, L)):
   * between keys as above;
   * `t >= tn`: interpolate from `(tn, dn)` to `(t0 + L, d0)`;
   * `t < t0`: interpolate from `(tn - L, dn)` to `(t0, d0)`.
4. value = base + delta; colours rounded to nearest integer and clamped to
   0..255.

#### 3.5.4 Script playback (I)
* Each object runs **one current script** with a time cursor in ms.
* Starting a script sets its time to 0 and immediately applies its tracks at
  t = 0.
* Each frame: `t += dt` (ms). Then:
  * if `t < length`: apply tracks at `t`;
  * else if a chain target exists: apply final values, switch to the chained
    script with `t = t - length` (overshoot carried over);
  * else if loop flag: `t = t mod length` (length 0 → 0);
  * else: apply final values and stop (time held at the end).
* Events whose time lies in `(previous t, new t]` fire in time order; an event
  at time 0 fires when the script starts. (I)
* Loop wrap (amendment 2, G): when a looping script wraps, events in
  `(previous t, length]` fire (2 retail looping scripts have an event at
  t = length), then events in `(0, t']` of the new cycle. Events at exactly
  t = 0 fire only when the script is started (by command, chain or
  creation), **not** on every wrap. Retail looping highlight scripts (1200 ms)
  carry t = 0 events (focus messages to sibling objects and one
  application notification). Re-sending those every cycle would be
  redundant at best.
* **A script only changes the properties it has tracks for**; other
  properties keep their current value. (I — public tooling instead falls back
  to the object's stored data; that breaks run-time positioned objects, so the
  data implies the former.)
* Event targets: `0` = broadcast to all objects of the same screen that
  respond to the message (via the target index, 3.6.3); `0xFFFFFFFF` = to the
  application; `0xFFFFFFFC` = observed only for intra-screen broadcasts in the
  PC help bar — treat like `0` (difference unknown).

### 3.6 Message responses (`MsgR` per object, `PkgR` per screen)

#### 3.6.1 Tags
A response list is a sequence of entries:
```
Mi  u32 message hash        (starts an entry)
MC  u32 command count       (may be 0: the message is consumed, nothing done)
then per command:
  Ri  u32 command id
  Ru  u32 integer argument  (or)
  Rs  string argument (ASCII, NUL-padded)
  Rt  u32 target guid (0 = self / this screen)
```

#### 3.6.2 Commands
| Id | Effect | Observed |
| --- | --- | --- |
| 0x000 | start script `Ru` on this object | yes (41k) |
| 0x001 | post message `Ru` inside the GUI: to object `Rt`, or with `Rt = 0` re-dispatch to this screen as if it came from input (I) | yes |
| 0x002 | post message `Ru` to the application | yes |
| 0x003 | post sound cue `Ru` to the audio system | no |
| 0x100 | give focus to button `Ru` (guid) | yes |
| 0x101 | enable/disable input processing (`Ru`) | no |
| 0x102 | remember the currently focused button | no |
| 0x103 | restore the remembered focus; `Ru` = fallback guid | yes |
| 0x104–0x108 | focus routing between nested screens | no |
| 0x200 | replace current screen by screen `Rs` | no |
| 0x201/0x202/0x204 | push screen `Rs` (variants) | no |
| 0x203 | pop current screen | no |
| 0x2C0–0x2C2 | screen-stack markers | no |
| 0x300 | if this object's current script == `Ru` … | no |
| 0x301 | if this object's current script != `Ru` … | yes |
| 0x500 | else | no |
| 0x501 | end-if | yes |

Ids are from the observed data plus public tooling (I). Conditional blocks
nest by `0x300/0x301 … [0x500 …] 0x501`. A typical use (O): on focus gain,
"if current script is not the fade-in, start highlight" — prevents a
highlight from cancelling the screen's entry animation.

#### 3.6.3 `Targ` (target index)
`Tc u32 n`, then `n` tags `Mt` each holding `u32 message hash` followed by the
guids of all objects that have a response to that message. It is a derived
index (rebuildable from `MsgR`). Broadcasts (target 0) go to exactly these
objects.

### 3.7 Validation and edge cases (O/I)
* Reject a package if any chunk/tag would run past its parent.
* Unknown chunk ids and unknown tags: skip using their size.
* A child whose parent guid is unknown or not a group: attach to root (G).
* Text whose label hash has no localisation entry shows the default `St`
  text (I); retail placeholders such as "option_name" or "WWWWWWWW" are
  overwritten by the program at run time.
* Packages may contain objects positioned off-screen as templates (O).

---

## 4. Runtime model (screens, focus, input, messages)

### 4.1 Screens and the stack (I)
* A **screen** is one loaded package. Screens form a **stack**; the top screen
  owns input. Modal dialogs and the help popup are separate screens pushed on
  top of the current one; the screen below stays drawn.
* Additional always-on screens can be layered (the PC help bar, 7.4, is its
  own package drawn above menus). (I)

### 4.2 Screen lifecycle choreography (O pattern, I semantics)
Every retail menu contains a hidden, off-screen text object acting as the
**screen controller**. Its scripts sequence the screen:

| Phase | Controller behaviour | Effect on other objects |
| --- | --- | --- |
| Create | initial script chains to the *enter* script | — |
| Enter (length 180–500 ms) | at 0 ms broadcasts the **enter** message; at 180–320 ms notifies the application "enter complete" | objects play **fade-in** (180 ms) |
| Exit/forward (140–500 ms) | at 0 ms broadcasts the **exit** message (and notifies the app); at 140 ms notifies "exit complete" | objects play **fade-out** (140 ms) |
| Dim (dialog opened above) | broadcasts **dim** | objects fade to low alpha in 100 ms |
| Undim (dialog closed) | broadcasts **undim** | objects restore in 100 ms |

The application starts the exit when a button is pressed or Back is
accepted, waits for "exit complete", then replaces/pops the screen.
Hash constants [interop]: enter broadcast `bdfd2df9`, exit broadcast
`aadd0187`, dim broadcast `a301c015`, undim broadcast `9f218e63`,
enter-complete to app `35f8620b`, exit-complete to app `e1fde1d1`; requests
from the app to the controller: back `cb1aa27f`, forward `ade14ec3`, dim
`c2a825e8`, undim `18bfc04b`.

### 4.3 Input actions (O names of the action set; semantics I)
The frontend consumes abstract **actions**, not devices:

| Action | Console meaning | Message hash [interop] |
| --- | --- | --- |
| Accept | confirm / press focused button | `406415e3` |
| Back | cancel / previous screen | `911ab364` |
| Up / Down / Left / Right | move focus / change value | `72619778` / `911c0a4b` / `9120409e` / `b5971bf1` |
| Left trigger / Right trigger | switch category/filter/page | `5073ef13` / `d9feec59` |
| Button 0 … 3 (extra face buttons) | screen-specific (e.g. "Customize", "Delete", "Messenger") | button n = `c519bfbf + n` |
| Start | start / skip | `b5af2461` |
| Help | open the help popup for this screen | (handled by the app) |
| Pointer secondary click (PC) | mapped by screens to Back | `14dd0705` |

Each directional and accept/back action also has an "alternate" binding
(second key / analog stick) — same meaning. (O: action list contains
alternates.)

Messages the engine sends to buttons [interop]: focus gained `abc08912`,
focus lost `55d1e635`, pressed `0c407210`, disabled `5c7a5c2e`.

### 4.4 Message dispatch (I)
For one input action:
1. If a button has focus and has a response to the action message, run it.
2. Run the top screen's package response for that message (if any). Package
   responses typically forward to the application and/or re-post to a
   specific object (e.g. make a scroll arrow blink).
3. Accept on a focused button additionally sends *pressed* to that button.
Messages sent with target 0 reach every object listed in the target index
for that message.

### 4.5 Focus and navigation (I/G)
* Only objects with the button flag can hold focus. Exactly one button per
  screen has focus while the screen has input (G: the first button in file
  order on entry, unless the program sets one).
* On focus change the old button receives *focus lost* (typically starts its
  "unhighlight" script) and the new one *focus gained* ("highlight"). (O)
* **Explicit links** override everything: a button may respond to a
  direction with "give focus to guid X" (used by dialogs: left/right between
  the two buttons; by the on-screen keyboard: all four directions per key).
  (O)
* **List-style widgets** (icon scroller, option list) take directions
  themselves (their package responses forward directions to the app/widget
  logic) and move the logical selection; focus does not travel geometrically
  there. (O: those buttons have no direction responses; the screen forwards
  directions to the application.)
* **Automatic directional movement** for remaining cases (G): choose the
  nearest focusable button whose centre lies in the pressed direction
  (within a ±45° cone), excluding buttons flagged "not reachable by direction"
  (0x00080000); no wrap.
* **Wrap** (amendment 3): icon scrollers and the car-select carousel do
  **not** wrap visually (R): with the last main-menu item selected, no item
  is drawn to the right of it and the right scroll arrow is hidden; with the
  first car selected, the left arrow is hidden. The arrow on a side is shown
  only when an item exists on that side. Whether pressing Right on the last
  item is ignored (I: yes, as no wrap target is drawn) is not visible in a
  still. Option lists: no wrap (G, unobserved).
* **Key repeat** for held directions (G): 400 ms initial delay, then every
  110 ms.

### 4.6 Original PC pointer behaviour (O/I)
* A mouse cursor image (32 x 32, white arrow with dark drop shadow, hot spot
  at the tip, top-left) is drawn by the program. (O: texture exists; I: hot
  spot.) Amendment 3 (R): the cursor is drawn at **32 x 32 virtual units**
  (it scales with the virtual screen, about 72 px at 1080 lines), on top of
  everything, and is clipped to the 4:3 viewport. It appears in three of the
  six captures (main menu, options list, and over a dialog) and is absent
  in the other three; when it is hidden is not known (11, item 5). The
  texture key is
  hash(`PC_CURSOR`) in the PC-specific global texture pack (14.2) [interop].
* **Secondary (right) click** posts the pointer message `14dd0705` to the
  screen. Almost all screens map it to **Back** (re-post the Back action to
  themselves and/or the app); the main menu maps it to nothing. (O)
* Clickable PC-only **pill buttons** (Back, Continue, Search, …) are buttons
  flagged "not reachable by direction" — they exist for the pointer only. (O)
* PC-only **left/right arrow icons** (20 x 20) beside category/filter labels
  respond to the left/right (or trigger) action messages; the program locates
  them by name and turns a click into the corresponding action. (I)
* Hover and wheel behaviour of the original are not recoverable from data (see
  Unknowns). Recommended pointer model for this library (G):
  * hovering a focusable button gives it focus (same visuals as keyboard
    focus); leaving does not remove focus;
  * primary click on a button = focus + Accept;
  * secondary click anywhere = Back;
  * wheel up/down over a list = Up/Down; wheel over a horizontal scroller,
    slider or value selector = Left/Right;
  * click on a scroll arrow = one step in that direction; click on a slider
    track = jump; drag of a slider knob or scroll thumb = continuous change.

---

## 5. Visual kit (O measurements, descriptive only)

Amendment 4: load genuine textures at runtime (20). The descriptions below
identify expected appearance; they do not authorize procedural substitute
art. Sizes are the texture sizes; on screen they are stretched.

| Element | Description |
| --- | --- |
| Dark box | solid black, 60% alpha, 16x16 (stretched to any rectangle) |
| Light box | solid white, 60% alpha |
| Rounded corner (dark/light) | 16x16 quarter-disc (radius 16) in the box colour; four placed around a box form a rounded panel; orientation via 90° rotation or negative size |
| Large end cap | 32x32 rounded corner variant used at the ends of strips and buttons (half of a 32-high pill) |
| Highlight frame | 8-unit-thick border lines plus 16x16 rounded corner pieces, drawn in accent/border colour |
| Scroll-arrow badge | 32x32: thin white circle outline with a white right-pointing arrow, translucent fill; rotated for other directions |
| Arrow badge (white) | same shape, fully white (used in animated "activity" arrows) |
| Arrow highlight ring | 32x32 white circle outline only |
| Scroller track | light box stretched to 8 units thick |
| Scroller thumb | 16x16 small white vertical bar / tab |
| Scroller end dot | 8x8 small white dot with dark rim |
| Slider track | 256x32 soft translucent white bar (alpha ~40 average) |
| Slider fill end | 16x16 small solid white rounded square (tinted green) |
| Slider knob | 16x16 dark grey rounded square with light rim |
| Icon highlight | 128x64 white rounded-rectangle outline with soft glow, tinted accent, pulsing |
| Icon backing | 128x64 black rounded rectangle |
| Mini icon / dot | 16x16 white disc |
| Mini icon backing / highlight | 32x32 black rounded square / white outlined rounded square |
| PC pill button | 256x32: black fully rounded pill with a ~2 px white outline; "small" variant has extra transparent margin left/right; a 128x32 variant exists |
| PC pill glow | 256x32 soft white horizontal elliptical glow, transparent edges |
| Lock icon | 32x32 padlock (accent tint) |
| Save icon | 32x32 memory-card symbol |
| Busy spinner | 32x32 ring segment, rotated continuously |

Standard animation set (O; durations in ms; "accent" = 202,244,138):

| Name (descriptive) | Duration | Behaviour |
| --- | --- | --- |
| Fade-in | 180 | alpha 0 for first 40 ms, then linear to the resting alpha |
| Fade-out | 140 | linear from resting alpha to 0 by 100 ms, hold |
| Dim / undim | 100 | alpha to ~60/255 (panels) or 0 (some groups) and back |
| Focus pulse (text) | 1200, loop | colour = accent; alpha 0→255 (0–440), hold (440–760), 255→0 (760–1200) |
| Focus (row backing) | 0 | row backing alpha 80 → 200; its depth is 11 units nearer than the unfocused state (z 89 vs 100) |
| Blink (scroll/value arrow on step) | 400 | tint to accent, alpha 150→255→150, size +6 units at 200 ms then back; then returns to static |
| Icon highlight breathe | 1200, loop | accent; full alpha 0–320, fade to 0 by 640, hold 0 to 1000, back to full at 1200 |
| PC pill focus | 1200, loop | pill tint white→accent and alpha 150→255 at 500 ms, hold to 700, back at 1200; glow overlay alpha 0→255→0 on the same timing; label switches to accent |
| Activity arrows | 3600, loop | three arrow badges light in sequence (online/status indicator) |
| Player strip swap | 320 | cross-fade between player name and cash |
| Disabled | 0 | alpha 120 (text) |

---

## 6. Widget catalogue

Each widget lists: structure (parts), properties, states, animation, input
(console action → behaviour) and pointer mapping (G unless stated). Geometry
numbers are the retail values (O) in virtual units relative to the screen
centre unless "relative".

### 6.1 Image / panel
* Parts: one textured or untextured quad.
* Properties: rect (centre + size), texture, UV rect, colour/alpha, rotation,
  mirror (negative size).
* **Rounded panel** (composite, O): a centre box plus four edge strips (16
  wide) plus four 16x16 rounded corners; e.g. menu panel 562x152 + strips,
  alpha 215–255; light inner panel 548x62 tinted 225 with alpha 200.
* **Colour-gradient image**: four corner colours interpolated across the quad
  (O: type exists; rare).
* No input.

### 6.2 Text label
* Properties: string or label hash, font, scale (size.x/y), colour,
  justification (8.4), wrap width, leading, buffer capacity.
* States: normal, focused (pulse), disabled (alpha 120), hidden.
* Inline tokens `$NAME$` are replaced by glyph images (7.3); `^` is a line
  break in localised strings; `\n` also breaks lines.
* Typical scales (O): screen title 0.75–0.9 (accent, left-aligned at the top
  left of the panel or right-aligned at top right); body/value 0.7; hint
  labels 0.7 (grey 192); PC pill labels 0.45; small captions 0.5–0.55.

### 6.3 Group
* Container that transforms, tints and fades its children; carries scripts
  for fade/dim. No visual of its own.

### 6.4 Icon scroller (horizontal carousel menu) — main menus, category pickers
* Parts (O): a rounded dark panel (~562 x 152) across the upper part of the
  screen; a light title band inside it; an **item template slot** 118 x 59 at
  about (0, -134); a pool of N item icons (buttons, 118 x 59 each, N up to 20)
  that the program positions; a **focus frame** (accent glowing rounded
  rectangle 126 x 62) and a dark icon backing at the slot; **left/right
  scroll-arrow badges** (26 x 26) at x = ±270, y = -133; a horizontal
  scroller under the icons (track 564 x 8 at y = -172 with a 16 x 16 thumb);
  an **item title** text centred under the slot (accent, scale 0.7); a screen
  title (accent, scale 0.8–0.9); optional lock badge on locked items.
* Behaviour: Left/Right move the selection one item; the arrow on that side
  plays *blink*; the item title text updates; the thumb moves proportionally
  (`thumb_x = track_left + (track_width - thumb) * index / (N - 1)`) (I).
  Accept presses the selected item (exit animation, then the app reacts).
* Amendment 3 (R, main menu captures): **three icons are visible**: the
  selected one in the fixed centre slot and one neighbour on each side.
  The selection stays centred and the strip scrolls.
  * Selected icon: drawn 118 x 59 centred at (0, -133.5), inside the focus
    frame. Frame: accent rounded-rectangle line about 2 units thick whose
    line centres are at x = ±60 and y = -161 / -106 (so 120 x 55 between
    line centres), plus a soft glow about 3 units outside. At its brightest
    the line is the accent colour (202,244,138). Inside the frame the area
    is darkened, with a faint green tint (backing plus tinted highlight).
  * Neighbours: drawn **96 x 48** (0.81 x the selected size; texture
    128 x 64 at 0.75 units per texel), centred at **x = ±158**, y = -133.5,
    **full opacity** (white parts reach 255), no backing, no frame.
  * Icons at ±316 (second neighbours) are not drawn (they would lie outside
    the light band, which ends at x = ±290).
  * At the first/last item the missing neighbour is not drawn, and the
    arrow on that side is hidden (4.5).
  * Arrow badges centred at (±270, -132.5); the visible disc is about 23
    units across.
  * Light icon band x -290 .. +290, y -165.5 .. -103.3; scroller track
    y -176 .. -168.5 (8 thick), visible x -290 .. +290.
  * Scroller thumb (R, confirms the formula above): centre
    `x = -273.7 + 547.4 * index / (N - 1)`, y ≈ -171. Main menu on PC:
    N = 7 (thumb at -182.4 for index 1 and +273.8 for index 6).
  * Item title: accent, scale 0.7, centred at x = 0, cap top at y ≈ -95.
* Main menu on PC (R/I): 7 items. Observed order: index 0 career, 1 quick
  race, 2 go online, …, 5 profile management, 6 options (last). Indexes 3
  and 4 were not seen (I: LAN and customize car, the remaining icon
  families of 15.9 that apply to PC).
* Locked item: lock badge shown, Accept plays an error cue (I: cue exists).
* Pointer: click an item = select + accept; click arrow badges = step;
  wheel = step; hovering an item selects it (G).

### 6.5 Option list (vertical rows) — options screens, part lists
* Parts (O): a screen title; header masters for the **name column**
  (240 x 36 at (-151,-120)) and the **value column** (216 x 32 at
  (126,-122)); a pool of rows, each = name text (button, dark grey 50, scale
  0.7, left-aligned) + value text (centred, scale 0.7) + left/right value
  arrows (24 x 24) + a light row backing (alpha 80, rounded caps 36 x 36)
  + optional slider parts; a **vertical scroller** at x = 273 (track 8 x 282,
  arrows 24 x 24 at y = -164 and +167).
* Row pitch (amendment 3, R): **36 units**, measured on two screens. On the
  audio settings page the first row's text is centred at y ≈ -126, and row k
  is at -126 + 36k.
* Visible rows (R): the audio page shows 8 rows (all of its 8 entries; the
  thumb sits at the top). The PC controls page, which has a header band and
  column headings above the rows, shows **7** rows and more exist below. So
  the visible count depends on the screen: 8 for a plain options list, 7
  for the controls page. Whether a plain list would show a 9th row is
  unknown, because the audio page has only 8 entries. The list scrolls
  when the selection moves past the last visible row (G; unobserved).
* Observed row look on the light panel (R, audio page): name text dark
  grey (≈ 32–50), left edge at x ≈ -269, scale 0.7; value text centred at
  x ≈ +127, scale 0.7. Unfocused rows show **no visible backing**. The
  focused row has a dark grey backing (≈ 65,65,65 as composited) spanning
  x -286 .. +262 and y -144.4 .. -109.6 (35 high); its name text pulses
  (5); its value arrows are visible, centred at x ≈ +6 and +246 (visible
  disc ≈ 22). The arrows are absent on unfocused rows.
* Vertical scroller (R): track x 269–277, y ≈ -147 .. +151; up arrow
  centre (273, -163.5), down arrow centre (273, +167); thumb near y -130
  when the first row is selected.
* States per row: normal (dark text), focused (text pulse in accent; backing
  alpha 200 and moved forward; value arrows visible), disabled (alpha 120,
  skipped by navigation (G)), hidden.
* Input: Up/Down change the focused row (scroll arrows blink on scroll);
  Left/Right change the row's value (the side's arrow blinks); Accept on
  action rows opens a sub-screen; Back leaves.
* Value kinds supported by a row (I, from screen content):
  * **Selector / spinner**: cycles through a list of strings (wraps (G)).
  * **Toggle**: a two-value selector (e.g. On/Off).
  * **Slider**: numeric 0..1 shown with the slider parts (6.6).
  * **Action**: no value; Accept triggers.
* Pointer: hover row = focus; click left/right arrows = step value; click on
  row = Accept for action rows; wheel = scroll list (or change value while
  the pointer is over the value column) (G).

### 6.6 Slider
* Parts (O): track (≈235 x 32, translucent), **fill** = green bar (height ~6)
  with green rounded end pieces (15 x 15) at both ends, **knob** 16 x 16 at
  the value position; in option rows the slider sits in the value column.
* A vertical variant exists on one screen (reputation meter) (O).
* Option-row slider as observed (amendment 3, R, audio page): the green
  fill (99,178,34, opaque, ≈ 7 units high, centred on the row) starts at
  x ≈ +54.6. The knob (white rounded tab with a dark rim) is centred at
  x ≈ +196.6 for the maximum value, so the knob travels about 142 units. A
  dim translucent track continues to x ≈ +203. The slider is drawn on
  every row that has one, focused or not.
* Input: Left/Right step by 1/10 of range (G); hold repeats.
* Pointer: click on track = jump; drag knob = continuous; wheel = step (G).

### 6.7 Scroller (scroll bar)
* Horizontal (under icon strips) or vertical (beside lists). Parts: light
  track 8 units thick, thumb tab 16 x 16, end dots 8 x 8, arrow badges 24–26
  at the ends (rotated). The arrows blink when the content scrolls in that
  direction. (O)
* Thumb position proportional to selection index (I); thumb size fixed (O).
* Pointer: click arrows = step; drag thumb = scroll; click track = page (G).

### 6.8 Push button
Three visual families (O):
1. **Dialog button**: dark pill 214 x 42 (centre box + 42 x 42 end caps
   each side), text label separate (scale 0.75, centred). Focused: label text
   pulses accent; unfocused text grey 150; button dims.
2. **PC pill button**: 144 x 24 pill, alpha 150, plus glow overlay (hidden
   unless focused), label scale 0.4–0.45 centred, white. Focused: PC pill
   focus animation (5).
3. **Icon button**: 118 x 59 icon image in a scroller (6.4).
* States: normal, focused, pressed (one-shot: starts exit/forward), disabled
  (alpha reduced; ignores Accept), hidden.
* Pointer: hover = focus; click = press.

### 6.9 Modal dialog (message box)
* Variants (O): plain, small, medium, large; animated small/medium/large
  (I: the animated ones additionally scale/slide in); a three-button variant;
  in-game copies of small/medium/large.
* Parts (O): full-screen darkening quad (≈1070 x 650, dark box at full
  object alpha → ~60% black) behind the dialog; dark rounded panel (sizes
  per variant in the table below; amendment 2 corrects the earlier claim that
  562 x 198 is the large one: it is the SMALL variant);
  a light inner panel; accent **border frame** (197,218,165); title text
  (accent, 0.75) with optional 32 x 32 icon at the left; message text (white,
  0.75, centred at about (0,-3), wrap width 720 unscaled = 540 on screen);
  panel centred at about (0,+8) spanning y ≈ -91 … +107; buttons at screen
  y = +74: two at x = -150 and +150 (e.g. Yes/No), or one centred, or three.
* Variant geometry (O, absolute virtual units, panel = 562-wide centre box
  plus 16-wide side strips and 16 x 16 rounded corners, so outer x -297..+297):

  | Variant | Dark panel y | Light inner panel (548 wide + strips, x -290..+290) y | Button row centre y | Accent border frame (8 thick, 197,218,165), approx. |
  | --- | --- | --- | --- | --- |
  | SMALL | -91 .. +107 | -49 .. +99 | +74 | x -303..+304, y -98..+115 |
  | MED | -144 .. +140 | -105 .. +133 | +107 | y -150..+146 |
  | LARGE | -214 .. +199 | -173 .. +191 | +165 | y -221..+206 |

  Buttons are 214 x 42 boxes with 42 x 42 end caps, at x centres -150 / +150
  (two) or 0 (one) for every variant; label scale 0.75, focused label accent,
  unfocused grey 150. Title (accent, 0.75) left-aligned next to a 32 x 32
  icon in the panel's top-left corner. The animated variants have the same
  geometry plus scale/slide scripts.
* Behaviour: pushed over the current screen; the screen beneath receives
  *dim*; Left/Right move focus between buttons via explicit links; Accept
  presses; Back = the "negative" choice (posts Back to the app) (O: dialog
  package maps Back to the app); on close the screen beneath receives
  *undim*. Default focus (amendment 3, R): in both captured two-button
  dialogs (a quit confirmation and a save confirmation) the **right-hand
  (second, negative) button** has focus. This agrees with the data, which
  authors the right button's label in the focused accent colour and the
  left one's in grey 150. Use the right button as the default for
  two-button dialogs; three-button default unknown (G: rightmost).
* Observed appearance (amendment 3, R):
  * **Focused button**: dark backing (clearly darker than the light inner
    panel); label pulses between transparent and accent (5, focus pulse).
    **Unfocused button**: light-grey backing close to the inner panel's
    tone; label grey (≈150).
  * Visible button extent: x = centre ± 134.7 (i.e. 15.5 .. 285 for the
    right button), height 41.4 (SMALL: y 53.0 .. 94.4). The 42 x 42 end-cap
    textures have transparent margins, so the authored 298-unit extent is
    not all visible.
  * Border: a thin (≈ 2 units) line in the border colour whose centre runs
    **along the dark panel's outline**: SMALL x -297 / +298, y -91.6 / +109;
    LARGE x -298 / +297.6, y -214.2 / +199.4. (The 8-unit quads of the
    table above are mostly transparent; their visible line is centred.)
  * Dark panel inside the border is nearly opaque black.
  * Title (LARGE capture, no icon shown): accent, scale 0.75, ink left edge
    at x ≈ -289, cap top y ≈ -198. The authored title position is 30 units
    further right (next to the 32 x 32 icon slot), so when the icon is
    absent the title starts at the panel's left inner edge (I).
  * No title and no icon in the captured SMALL quit dialog.
  * Message (R): SMALL single line centred on (0, -3) (ink y -9.8 .. +6.7);
    LARGE two lines (the string contains a `^`) centred on (0, -33),
    baseline pitch 25.4 (16.5).
  * Screen beneath: everything under the dialog, including the 3D
    background and the screen's own panels and text, is darkened uniformly
    to **0.40** of its brightness. That matches the full-screen 60%-black
    quad alone; no additional dim fade of the screen below was measurable.
  * The **PC help bar and its strip are not drawn** while a dialog is open
    (neither pills nor strip are visible, though they would survive a 0.40
    dim).
* Pointer: buttons clickable; secondary click = Back; clicks outside the panel
  ignored (G).

### 6.10 Help popup (button legend)
* A dialog-style panel (small/medium/large) titled "Help", whose body is a
  multi-line legend: each line = one or two glyph tokens, a spacer token,
  then a description ("Change Selection", "Rotate Car", "Select Highlighted
  Option", "Back to Previous Menu" …). A single hint "[Back glyph] Back" sits
  at the panel's bottom right. (O)
* Opened by the Help action; closed by Back or Accept (G for Accept).

### 6.11 Stat bars (progress / meter)
* Parts (O): label caption (scale 0.55, light grey) above a translucent
  track (256 x 32), green **current-value bar** (height 6) with a rounded
  green end cap, and a knob marker (16 x 16) at the value. Three bars side by
  side (acceleration, top speed, handling) spaced 200 units.
  Amendment 2 (O): the 256-wide backing quads really do overlap at 200
  spacing. Their texture is a soft, mostly transparent bar drawn at alpha
  35, so with the original texture the overlap is not noticeable. Each fill
  bar starts at its end cap, about 42 units right of the backing's left edge.
* Observed (amendment 3, R, car select): labels upper-case, light grey,
  scale 0.55, left-aligned with ink left edges at x ≈ -280, -77, +121
  (pitch 200) and y 157–168. The green fills (height 6, centred on y ≈ 176)
  start at x ≈ -284, -82, +116, i.e. about 4 units left of their labels.
  The knob is at the end of the fill (at y ≈ 175). The backing is a faint
  bar that fades out about 165 units to the right of the fill start.
* Read-only; may animate from old to new value (G: 300 ms linear).

### 6.12 Category / filter switcher ("tab" control)
* Parts (O): a caption text (scale 0.5, centred) between two small arrow
  badges (20 x 20) on PC, or between left/right trigger glyphs on console.
* Input: Left/Right trigger cycle the category (wrap (G)); the arrow on that
  side blinks.
* Pointer: click arrows; wheel over the caption (G).

### 6.13 Grid picker (swatches / map markers)
* Parts (O): a grid of small items (paint screen, amendment 2: one row of
  12 swatches, each a 16 x 16 disc texture inside a group scaled x2, so
  32 x 32 on screen, at about 21 local units = about 42 on-screen units
  pitch; the program places them; or map event markers), one highlight square (24 x 24 outline,
  breathing like the icon highlight) that moves to the selected cell; the
  selected cell grows 16 → 24 units in 140 ms, a deselected cell shrinks
  24 → 16 in 300 ms.
* Input: directions move within the grid (no wrap (G)); Accept selects.
* Pointer: hover = move highlight; click = select.

### 6.14 On-screen keyboard
* Parts (O): title, entry text line (dark text, scale ~1.15, wrap 480),
  ~50 key buttons laid out in rows (digits, QWERTY rows) at ~40-unit pitch,
  key captions scale 0.8 dark grey; control keys (wide 200 x 32 buttons: e.g.
  space/caps/symbols/done/backspace).
* Navigation: **explicit per-key links** in all four directions (O).
* Hints: Backspace on Button1, Caps on right trigger, Symbols on left trigger,
  Help, Back (O, from the screen's legend).
* Pointer: click keys; typing on a physical keyboard should insert directly
  (G).

### 6.15 Text entry field and date entry
* Text entry: single-line field, shows typed text and a caret (G), used for
  names/passwords on PC (O: PC-only name-entry objects exist).
* Date entry (O): three numeric fields (e.g. month/day/year); Up/Down cycle
  the focused number, Left/Right change field, Accept confirms, Back cancels;
  legend "Cycle Numbers / Accept Date / Back".

### 6.16 Table / multi-column list
* Rows of 3–4 aligned text columns (e.g. rank, player, time), up to 10 rows,
  with a focused-row highlight and a vertical scroller; PC lists use a small
  white triangle arrow (32 x 32 texture, drawn ~16) to mark the selected row
  (O: PC list arrow exists).
* Input: Up/Down move; Accept opens details.

### 6.17 Car-select carousel
* Parts (O): manufacturer logo box (128 x 64), car name badge (256 x 64),
  left/right arrows, filter switcher (6.12), three stat bars (6.11), the 3D
  car behind (outside scope; omit it when genuine runtime art is unavailable, 20.1).
* Input: Left/Right previous/next car; triggers change filter;
  Accept selects; Button1 "Customize".
* Observed (amendment 3, R, first car of the list): the left arrow is
  hidden, the right arrow is at (269.6, -143.6), and the scroller thumb is
  at x ≈ -273 (index 0) on a track at y -192 .. -184. So the carousel
  does not wrap (4.5). The manufacturer logo is in an accent frame (outer
  x -223 .. -92, y -178 .. -111). The car-name badge image is centred near
  (93, -144). The filter caption is white, scale 0.5,
  centred at x ≈ 185, between the PC arrows at (95, -208) and (268, -208).
  The screen title uses the same title text style as 6.5 (ink left
  x ≈ -288, cap top ≈ -214, scale 0.8).

### 6.18 Player information strip (bottom-left)
* Parts (O): within the bottom hint strip (7.2), a 167-wide grey box at the
  strip's left containing the player name and cash (scale 0.6–0.7, light
  grey, centred), which alternate with a 320 ms cross-fade (G: every 4 s).

### 6.19 Activity / busy indicators
* Three white arrow badges in a row lighting in sequence (3600 ms loop) for
  online activity (O); a rotating ring for loading (O texture; G speed
  1 rev/s).

### 6.20 Movie placeholder
* Object type "movie": a rectangle where video plays; the demo may draw a
  genuine runtime panel with a descriptive demo label, or omit the region
  when its resources are unavailable (20.1). (O: type exists; content out of scope.)

### 6.21 Pointer cursor
* 32 x 32 white arrow with soft dark outline/shadow; drawn above everything;
  hidden when keyboard/pad input is used (G) and shown on pointer movement.
* Size and clipping (amendment 3, R): 32 x 32 virtual units, clipped to the
  4:3 viewport (4.6). No hover highlight was captured, so hover behaviour
  remains unknown.

---

## 7. Controller hint bar, PC help bar, and the mouse rework

### 7.1 Console hint strip (O)
Every menu has a **bottom strip** at y ≈ +206 (height 32):
* Background (O, amendment 2, absolute x ranges, all y 190..222): left end
  cap 32 x 32 at x -309..-277 (alpha 215); grey box (128,128,128, alpha 215)
  x -277.1..-109.7 (167.4 wide); a 2.7-unit gap; dark box (alpha 150)
  x -107..+278 (385 wide); right end cap x +278..+310 (alpha 150). The
  pieces do not overlap.
* Left part: player information (6.18).
* Right part: **hint items**, each = glyph (scale 0.9, white,
  left-aligned/vertically centred) followed **30 units to the right** by its
  label (scale 0.7, grey 192, left-aligned/vertically centred).
* Fixed slots (absolute positions of the glyph):
  * **Help slot** at x = +212 (label at +242): "[Help glyph] Help" on nearly
    every menu;
  * **primary slot** at x ≈ -102 (label ≈ -72): the screen's main extra
    action (e.g. "[Accept] Install", "[Button1] Customize");
  * a **middle slot** at x ≈ +61 is used occasionally.
* Some screens place extra hints elsewhere (e.g. next to a filter caption, or
  inside dialogs at their bottom right).
* On PC builds all controller glyph items are console-only (hidden), and the
  strip's right side is visually replaced by the PC help bar (7.4).
* Amendment 3 (R): on PC **this 32-high console strip is not drawn at
  all**. All four captured menus show only the PC help bar's own strip
  (7.4.1). Its edges (top at y = 190, left end at x = -300, the gap at
  x ≈ -151) match the help-bar package and not this strip. Only one
  darkening layer is present, so the two strips are not stacked. The
  console strip pieces (left cap, grey box, dark box, right cap) therefore
  matter only for console-style rendering.

### 7.2 Hint definition in data (O)
* Hints are **ordinary text objects** whose default text is a token
  `$<ACTION-TOKEN>$`; the label is a separate text object (localised via its
  label hash). There is no dedicated hint record type.
* Glyph substitution happens at text-render time: the token is replaced by
  the platform's button image for that action (7.3).
* Which hints show is decided by the screen data plus the program toggling
  visibility (show/hide scripts on hint groups).

### 7.3 Inline glyph tokens
Token syntax: `$` + name + `$` inside any string (labels and localised
strings). Observed tokens (functional names [interop], one per action):

| Token suffix | Action |
| --- | --- |
| `SELECT` (also `ACCEPT`) | Accept |
| `CANCEL` | Back |
| `HELP` | Help |
| `PAD_UP`, `PAD_UP_ALTERNATE` | Up/Down pair (shown together as "up/down") |
| `PAD_LEFT`, `PAD_LEFT_ALTERNATE`, `PAD_RIGHT_ALTERNATE` | Left/Right pair; the right-alternate (right stick) means "rotate view" in legends |
| `LTRIGGER`, `RTRIGGER` | triggers |
| `BUTTON0` … `BUTTON3` | extra face buttons |
| `START` | Start |
| (spacer) `ICON_SPACER` | blank of one glyph width, aligns legend columns |

Full token text = `$JOY_EVENT_FENG_<suffix>$` except the spacer
`$ICON_SPACER$`. [interop — required to render retail strings]
Rendering rules (G): a glyph occupies one square cell whose side equals the
line's cap height × ~1.2, vertically centred on the text line; following
text continues after one space.

### 7.4 Original PC help bar (O)
The PC build adds an always-available overlay screen with up to **six
clickable pill buttons** at the bottom right, laid out as a 3 x 2 grid:

| Slot | Centre (x, y) |
| --- | --- |
| 1 | (+223, +226) bottom right |
| 2 | (+223, +203) top right |
| 3 | (+76, +203) top middle |
| 4 | (-72, +203) top left |
| 5 | (+76, +226) bottom middle |
| 6 | (-72, +226) bottom left |

* Each slot: pill 144 x 24 (alpha 150), glow overlay (same size, hidden), and
  a label (scale 0.45, centred both ways, white). Slots fill from the right:
  slot 1 is the primary (I).
* Each label is set by the program (I) from a PC-specific navigation label
  set present in the string tables (O):
  Back, Continue, Quit Game, Customize, Car Stats, Delete, Install Part /
  Paint / Vinyl / Decal / Package, Messenger, Logoff, Read Message, Message
  Management, Search, Reset Keys To Default, Activate GPS / Deactivate GPS,
  Hide on world map, Left, Right.
* Slot buttons are flagged "button" + "not reachable by direction": they are
  pointer-only; pressing sends *pressed* to the application, which maps it
  to the action.
* Focused (hovered) slot: PC pill focus animation (5).
* The overlay also re-creates the player information strip at its left.
* Context presets (O): the overlay holds named presets that show/hide slots
  for: pause menu, pause options, world map, dyno results, event results,
  in-game, plus others. Unused slots are hidden.

#### 7.4.1 PC help bar as observed (amendment 3, R)
* **Strip** (the help bar's own, authored in its package; absolute
  coordinates computed from the data agree with the captures within 1
  unit): it starts at y = 189.2 and runs to the bottom edge of the virtual
  screen (y 240; the quads are 51 high).
  * Left rounded end x -300.4 .. -284.4: a 16 x 16 rounded corner on top
    and a 16-wide box below it.
  * Player box x -284.4 .. -151.8 (132.5 wide). Colour (128,128,128) at
    alpha 215 on the black 60% box texture. On screen it is **darker**
    than the right part, never grey (Q17: modulate is correct).
  * A ≈ 3-unit gap (-151.8 .. -148.9) where the background shows through.
  * Hint box x -148.9 .. +281.7 (430.7 wide), white at alpha 150 on the
    same black texture.
  * Right rounded end x +281.6 .. +297.6 (from data; not distinguishable
    in the captures).
* Player name: centred at (-225.7, 212.2), scale 0.5, colour 230 grey,
  centre/v-centre justification (R: ink centre (-224, 214)).
* **Pills** (R): the visible pill outline spans about 143.6 x 18.7 around
  the slot centres of 7.4. Slot 2 is x +151 .. +294.6, y 194.0 .. 212.7;
  slot 1 is y 216.7 .. 235.8; slot 3 is x +3.8 .. +147.8. The pill is black
  inside with a light (≈ 230) outline about 1–2 units wide. The label is
  white, scale 0.45, italic, centred on (slot x, slot y - 2), e.g.
  (+223, +201).
* **Slot assignment observed per screen** (labels from the PC navigation
  label set above):

  | Screen | Slot 1 (+223,+226) | Slot 2 (+223,+203) | Slot 3 (+76,+203) |
  | --- | --- | --- | --- |
  | Main menu | Quit Game | Continue | — |
  | Options list page | Quit Game | Back | — |
  | PC controls page | Quit Game | Back | Reset Keys To Default |
  | Car select | Quit Game | Continue | Back |
  | Any dialog open | help bar hidden | | |

  Rule consistent with these (I): slot 1 is always the quit entry; slot 2
  holds the screen's forward action (Continue) if it has one, else Back;
  the next free slot (3) holds Back or the screen's extra action. Slots 4–6
  were never seen in use on these menus.
* There is **no Help pill** on PC, and no key-cap or controller glyph is
  drawn anywhere on these screens (R).
* Hover/focus appearance of a pill was not captured (no pill was under the
  cursor). The PC pill focus animation (5) remains the data-derived
  expectation.

### 7.5 PC key-cap glyph set (O; historical PC asset description)
16 x 16 white rounded-square outline (1 px, corner radius ~3) with a centred
white symbol, transparent background:

| Glyph | Symbol inside the key-cap |
| --- | --- |
| Enter | return arrow (↵) |
| Esc | "Esc" lettering (very small) |
| Back | left arrow to a vertical bar (⇤) |
| Continue | right arrow followed by dots (→…) |
| Help | "?" in a filled circle |
| Delete | circle with diagonal slash (⊘) |
| Letters | A, C, D, P, Q, R, T, Z |
| Punctuation | comma, period, `[`, `]` |

Also: a 32 x 32 directional arrow badge (black rounded square with white
left-pointing triangle; rotate for other directions), and the pointer cursor
(6.21). None of these is referenced by the retail screen data; their exact
binding to actions is unknown (see Unknowns). Suggested binding (G): Accept =
Enter, Back = Esc, Help = Help-cap, Delete = Delete, triggers = `[` / `]`,
Button0..3 = letter caps assigned per screen.

### 7.6 Superseded mouse-oriented hint-bar design (historical)
Sections 20.3 and 20.7 supersede this entire subsection. Its mouse/key artwork
and alternate compact layout remain historical. The original PC pill layout
and focus animation in 7.4/7.4.1 are restored by 20.7; this does not restore
the invented mouse hint bar.

Historical description follows:
Requirement from the project: keep the original look but show mouse
equivalents.

**Layout (G, based on O geometry):**
* Strip at y = +206, height 32, span x = -309 … +310, same two-part
  background as 7.1 (grey left box for player info / screen info, dark right
  box for hints).
* Hint items laid out **right-to-left** starting at the Help slot position:
  rightmost item's glyph at x = +212; each item = glyph (16 x 16 at scale
  1.0 of the glyph cell, i.e. ~18 units on screen) + 6-unit gap + label
  (scale 0.7, grey 192); items separated by 24 units; maximum width of the
  hint area = 385 units; items that do not fit are dropped from the left
  (lowest priority first).
* Each hint item is **clickable** (it is a button flagged not reachable by
  direction): hover = label turns accent and the glyph pulses (PC pill focus
  timing); click = performs the action, identical to pressing the key.
* Alternative compact mode: the original PC 3 x 2 pill grid (7.4) may be
  used instead; both must share the same action binding table.
* Amendment 3: the original PC look (what a user of the PC game sees) is
  7.4.1: the 51-high strip from y = 189 to the bottom edge, the pills, and
  no Help item. When the library renders original screens in "PC
  original" mode, use 7.4.1 and do not draw the console strip of 7.1. The
  y = +206 strip layout above remains this library's own design for the
  mouse hint bar.

**Mouse glyph set (new artwork, same style as 7.5: 16 x 16, white 1 px
outline, transparent fill, centred symbol):**

| Glyph id | Drawing | Meaning |
| --- | --- | --- |
| mouse-left | mouse outline (rounded rectangle, top split into two buttons), left button filled | primary click |
| mouse-right | same, right button filled | secondary click |
| mouse-wheel | same, short vertical bar in the middle filled, small up/down ticks | wheel |
| wheel-up / wheel-down | mouse-wheel with single up / down tick | wheel step |
| mouse-hover | small arrow cursor over a dotted rectangle | hover/point |
| mouse-drag | mouse-left plus a horizontal double arrow below | drag |
| key glyphs | as 7.5 (Enter, Esc, Help…) | keyboard fallbacks |

A hint may show two glyphs (mouse + key) separated by a "/" in grey.

**Action → hint mapping (G):**

| Action | Mouse hint | Key hint | Default label |
| --- | --- | --- | --- |
| Accept | mouse-left | Enter | per screen ("Select", "Continue", "Install"…) |
| Back | mouse-right | Esc | "Back" |
| Help | click on the Help hint | Help-cap | "Help" |
| Up/Down (lists) | mouse-wheel | arrow badge | "Change Selection" |
| Left/Right (value) | mouse-left on arrows / wheel over value | arrow badge | "Change Value" |
| Triggers (category) | mouse-left on arrows | `[` `]` | "Change Category" |
| Slider | mouse-drag | arrow badge | "Adjust" |
| Button0..3 | click on the hint itself | letter cap | per screen |
| Rotate view (right stick) | mouse-drag on the 3D area | — | "Rotate Car" |

### 7.7 Default hint sets per screen kind (O from the retail screens; labels paraphrase the retail English where generic)

| Screen kind | Primary slot | Middle | Help slot | Other placement |
| --- | --- | --- | --- | --- |
| Main menu, career hub, profile manager, category pickers, options hub | — | — | Help | — |
| Options list | Button1 "Player Select" | — | Help | — |
| Options hub (online-capable) | Button1 "Messenger" | — | Help | — |
| Car select (career/quick race) | Accept "Select Car" / Button1 "Customize" | — | Help | triggers beside filter caption |
| Car lot | Accept "Select Car" | — | Help | triggers beside filter |
| World map | Accept "Select Event" | — | Help | — |
| Parts / rims / spinner / performance browsers | Accept "Install" | — | Help | — |
| Paint | Accept "Install" | — | Help | triggers beside paint-type caption |
| Paint category | Button1 "Cancel Changes" | — | Help | — |
| Vinyl layers | — | — | — | Left trigger "Reorder Layers" |
| Audio (ICE) shop | Accept "Install", Button1 "Open/Close Doors" | — | Help | — |
| Decal overlay | Accept "Install", Button1 "Decal Color" | — | — | — |
| Brand select | Left/Right "Change Brand", Up/Down "Change Selection", Accept "Continue" | — | — | — |
| Quick-race mode/track select & options | Accept "Continue", Button1 "Messenger" | — | Help | — |
| Jukebox | Accept "Preview", Button1 "Grab" | — | Help | triggers for list page |
| Online lobby/main | Accept "Continue", Button1 "Messenger" | — | Help | — |
| Online game room | Button1 "Messenger", Button2 "Chat" | — | Help | — |
| Rankings | Accept "View Player", Button1 "Messenger" | — | Help | — |
| Persona select | Button1 "Delete Persona" | — | Help | — |
| On-screen keyboard | Button1 "Delete" | Left trigger "Symbols", Right trigger "Caps" | Help | — |
| Date entry | Up/Down "Cycle Numbers" | Accept "Accept Date" | Back "Back" | — |
| Memory-card list / credits | Back "Back" | — | Help | — |
| Dialogs | none (buttons are the affordance) | | | |
| Help popup | — | — | — | Back "Back" at panel bottom right |
| Magazine / reward screens | Accept "Continue" | | | |
| In-game pause menus | Accept "Select", Back "Back" | | | |
| In-game map | Accept "Activate GPS", Back "Back" | Button1 "Activate GPS" | | triggers "Full Map" filter |
| Message inbox | Accept "Read Message", Button1 "Delete Message", Back "Return to Gameplay" | | | |

Implicit everywhere (not drawn on console): Back via the Back action; on
PC the right mouse button = Back (except the main menu, where Back does
nothing and the PC help bar offers "Quit Game").

---

## 8. Text, fonts, localisation

### 8.1 Font (O/I)
* Main UI font: a condensed, medium-weight, **italic** humanist sans-serif
  (the ITC Conduit family, medium italic), white glyphs in a 256 x 256 atlas,
  ~212 glyphs covering ASCII, Latin-1 and a few typographic symbols (quotes,
  dashes, ellipsis, ™, minus). A smaller (14) and a bold-italic large (38)
  size and a HUD font also exist. **Amendment 2: draw text with the original
  font files (16) loaded from the user's install. Amendment 4 removes the
  substitute-font fallback; missing fonts are reported as unavailable.**
* Metrics at scale 1.0 (O, measured on the 21-size font): line-height byte
  26 units, but the rendered line pitch is the cell height 32 plus leading
  (16.5, amendment 3); capital height ≈ 20; x-height ≈ 15; descender ≈ 5; digits are
  tabular, advance 13; typical advances: capitals 13–19 (M = 19, A = 14),
  lowercase ≈ 11–13, `i`/`.`/`|` = 6. Kerning pairs exist (e.g. `Y`/`e`, `T`/`o`)
  with small negative adjustments (O; values not needed for a lookalike).
* Text drawn at scale s: all metrics × s.

### 8.2 Layout
* Lines break at `\n` and at `^` (localised strings). (O)
* Word wrap: if wrap width W > 0, break at spaces so that each line's
  unscaled width ≤ W. (I: dialog message W = 720 at scale 0.75 fits a 562
  panel.)
* Leading adds `Sl` unscaled units between lines. (O field; I semantics)

### 8.3 String table (localisation file) (O) [interop]
* One chunk `0x00039000`; payload:

| Offset | Type | Meaning |
| --- | --- | --- |
| 0 | u32 | header size = 0x20 |
| 4 | u32 | number of entries |
| 8 | u32 | offset of the entry table (from payload start) |
| 12 | u32 | offset of the string area |
| 16 | 16 bytes | table name, NUL-padded |
| 32 | u32 | 256 (code map length) |
| 36 | 256 x u16 | code map: for byte b ≥ 0x80, if map[b] ≠ 0 the character is U+map[b]; otherwise Latin-1 |
| … | | reserved/zero up to the entry table |
| table | count x {u32 hash, u32 offset} | sorted by hash ascending; offset relative to the string area |
| strings | | NUL-terminated 8-bit strings |

* Entry key = hash(label name) (label names are upper-case ASCII).
  4053/4107 retail labels verify against a companion label-name table.
* Lookup = binary search by hash; missing → keep the default text.
* Vectors: `test-vectors/string_table.hex`, `string_table.expected.tsv`.

### 8.4 Justification bits (`Sj`) (I; values 0,1,2,4,5,6,8,10 observed)
| Bits | Horizontal anchor at position.x | Vertical anchor at position.y |
| --- | --- | --- |
| none | left edge | top of first line |
| 0x1 | centre | |
| 0x2 | right edge | |
| 0x4 | | centre of the text block |
| 0x8 | | bottom of the last line |

Each line is aligned individually (G for multi-line centre/right). The block
height used for vertical anchors is (lines - 1) x line pitch + font cell
height (16.5) (I).

---

## 9. Sound cue hooks (O names of cue categories; G mapping)
The program has distinct UI sound cues for: focus, unfocus, up, down, left,
right, select, back, start, wrong/invalid, slider, toggle, scroll bar, icon
scroller, browser, date entry, stat change, text input, keyboard
(left/right/up/down/enter/delete), triggers (left/right), install, generic
button, summon dialog, summon help, summon customize. A library should expose
these as events (no audio assets are specified).

---

## 10. Demo coverage checklist (testable)
A conforming demo window shows, on one or more pages reachable from an icon
scroller:
1. rounded panels (dark and light), images with mirrored corners;
2. text in all eight justification modes, wrapped text, localised text from a
   string table, inline glyph tokens;
3. icon scroller with ≥ 6 items, arrows, scroller, item title, one locked item;
4. option list with selector, toggle, slider and action rows, disabled row,
   vertical scroller, more rows than fit;
5. stat bars; category switcher; grid picker; on-screen keyboard; text field;
   date entry; table list; car-select style carousel with genuine runtime art;
6. modal dialogs: one-, two- and three-button, small/medium/large; dim/undim of
   the screen beneath; help popup with a legend using glyph tokens;
7. busy indicators; movie region when genuine media is available; pointer cursor;
8. the original console hint strip (20), original controller icons and
   per-page actions; mouse input remains usable;
9. standard animations of section 5 for the retained original visual families
   with the listed durations; PC help-bar pill/glow animations apply in the PC profile (20.7);
10. right click = Back everywhere except the root page.

---

## 11. Unknowns and ambiguities (updated in amendments 2 and 3)
Closed in amendment 2: HUFF compression (13.3); the font (16, use the
original); dialog sizes (6.9); hint-strip geometry (7.1); stat-bar overlap
(6.11); swatch pitch (6.13); the installed-file container (14.3); texture
pack layout and formats (15).

Closed or narrowed in amendment 3 (reference captures, 19.1):
* Icon spacing and visible icons (6.4): 3 visible, neighbours 96 x 48 at
  ±158, full opacity. Closed.
* Option-list row pitch (6.5): 36. Closed. Visible rows: 8 on a plain
  list, 7 on the PC controls page. Narrowed (a plain list longer than 8
  rows was not captured).
* Icon-scroller and car-carousel wrap (4.5): no wrap shown; end arrows
  hidden. Closed for the visuals.
* Main-menu items on PC (6.4): 7 items. Order known for indexes 0, 1, 2, 5
  and 6; indexes 3 and 4 are I.
* Non-4:3 presentation (1.1): pillarboxed 4:3 with black bars, cursor
  clipped. Closed as observed, with the caveat that the capture install
  is modified.
* PC bottom strip (7.1, 7.4.1): only the help bar's strip is drawn; the
  player box is dark; there is no Help pill; the help bar is hidden under
  dialogs. Closed.
* Dialog default focus (6.9): the right/negative button. Closed for
  two-button dialogs.
* Pointer cursor size and texture (4.6, 6.21). Closed.
* Line pitch (16.5): corrected to (cell height + leading) × scale. One
  sample; I.

Still open:
1. **Tie order** for objects with equal accumulated depth (G: list order).
2. **Meaning of flag 0x4** and of event target `0xFFFFFFFC` vs `0`.
3. **Key-repeat timing, list scrolling and the Right press on the last
   item.** Not visible in stills; needs video (A2, A4b). G values of 4.5
   stand. Visible rows of a plain list with more than 8 entries (A4a).
4. **Whether absent tracks reset properties**: data implies they do not
   (3.5.4).
5. **Hover/wheel/left-click semantics** of the original PC build, the pill
   hover look, and when the cursor is hidden (A3, A5, A8). No capture had
   the pointer over an interactive element.
6. **PC key-cap glyph binding** and how `$JOY_EVENT_FENG_*$` tokens render
   on PC (A7). None of the six captures contains a token.
7. **Console glyph artwork and binding**: 19 PS2 icons are now verified
   (20.4); 47 Xbox glyphs now have a verified source/decoder (XBOX_CONTROLLER).
   Retail console action bindings and exact baseline/size remain unknown (20.5).
8. Resource-request ids (3.3.4) remain opaque. Eleven hash variants (h33,
   FNV-1, CRC-32, case variants, with and without extension) were tested and
   none matched. Not needed.
9. Interpolation types other than linear (unused), and the quaternion
   composition order (G in 3.5.2).
10. Screen-specific application behaviour beyond 7.4.1 (pill assignment on
    other screens; main-menu items 3 and 4).
11. The bottom strip on some screens is authored off-screen (y ≈ +306) and
    moved on-screen by the program. On PC this is moot for the console
    strip (not drawn, 7.1). The rule for other off-screen groups is unknown.
12. Whether the pillarbox presentation (1.1) is unmodified retail behaviour
    or comes from a third-party fix in the capture install.
13. Colour-product rounding (1.4) and DXT interpolation rounding (±1).
14. Palette colour byte order (15.7): no frontend evidence (greys only).
15. Font header fields marked opaque (16.2) and the Korean baseline-relative
    y offsets: not needed for Western languages.
16. Vertical centring rule (16.5): consistent with the captured dialog
    messages to within 1–2 units (ink-threshold limited); a second
    multi-line sample (A13) would confirm the line-pitch rule.
17. PS2 texture/swizzle variants other than the 32×32 format-4 controller
    textures of 20.4 remain unknown.
18. Whether the screen beneath a dialog plays its dim fade: no extra
    darkening beyond the 0.40 overlay was measurable on the main menu
    (6.9).
19. Dialog title placement when an icon is shown (only the icon-less case
    was captured).

## 12. References (public)
* NFSTools FEngLib (public library for this file family), commit
  34859b4dc2522d1d53c933158f55960a6cb00e8e:
  https://github.com/NFSTools/FEngLib — used for chunk/tag ids, the
  positional track mapping, command id list and resource kinds; all other
  statements were checked against the game data.
* Repository docs `docs/FORMATS.md` (name hash, texture pack container) and
  `tools/jdlz.py` (bundle decompression).
* psx-spx, "CDROM File Compression EA Methods (Huffman)", public
  documentation of the EA 30FB Huffman format:
  https://psx-spx.consoledev.net/cdromfileformats/ (the source of 13.3; the
  HUFF wrapper fields were observed).
* Microsoft Direct3D 9 `D3DFORMAT` enumeration (public docs; 21 = A8R8G8B8,
  41 = P8) and the public DXT1/DXT3/DXT5 (BC1–BC3) block compression
  formats, used in 15.7–15.8.

Research-reference note: the public repository above is cited only as
background research. Its class, type, member and function names are not used
here and must not be adopted as design names by an implementer; only the
format facts (numeric ids, byte layouts) listed above are relied upon, and
those were verified against observation or are flagged I.

---

# Part II — Original assets at run time (amendment 2, see AMENDMENTS.md)

The library must be able to draw the original frontend from the user's own
installation: screens, textures, fonts and strings are read at run time from
the install directory and never copied into the repository. Sections 13–18
give the byte-level formats and the run-time resolution rules. All facts
were checked against the user's PC install; the synthetic test vectors in
`test-vectors/` exercise every layout without containing retail bytes.

## 13. Compression codecs (O unless noted)

Two codecs occur in the frontend data. Both have a 16-byte wrapper header.

### 13.1 Wrapper headers [interop]
| Offset | JDLZ | HUFF |
| --- | --- | --- |
| 0 | ASCII `JDLZ` | ASCII `HUFF` |
| 4 | u8 = 0x02 (codec id) | u8 = 0x01 (codec id) |
| 5 | u8 = 0x10 | u8 = 0x10 |
| 6 | u16 = 0 | u16 = 0 |
| 8 | u32 decoded size | u32 decoded size |
| 12 | u32 encoded size **including** this 16-byte header | u32 length of the bit stream **excluding** this 16-byte header |

The encoded data may be followed by 0–3 bytes (JDLZ) or 0–2 bytes (HUFF) of
padding up to the end of the enclosing chunk; readers must ignore it.

### 13.2 JDLZ (LZ77 variant with two flag streams)
State: an output buffer; two 9-bit "flag registers" A and B, each holding up
to eight pending flag bits plus a sentinel bit. Both start *empty*. A
register is empty when only its sentinel is left (its value equals 1).
Reading a flag byte `f` into a register sets it to `f + 256`; consuming a
flag bit tests bit 0 and then shifts the register right by one.

Decoding starts at byte 16 and repeats until the output length equals the
decoded size:
1. If A is empty, read the next input byte into A.
2. If B is empty, read the next input byte into B. This check runs on
   **every** iteration, including iterations that end up copying a literal.
3. Consume one bit of A:
   * **0 → literal**: copy the next input byte to the output.
   * **1 → match**: read two bytes `p`, `q`, then consume one bit of B:
     * B bit **1 → near match**: distance = (low nibble of `p`) + 1
       (1..16); length = ((high nibble of `p`) × 256 + `q`) + 3 (3..4098).
     * B bit **0 → far match**: distance = ((top 3 bits of `p`) × 256 + `q`)
       + 17 (17..2064); length = (low 5 bits of `p`) + 3 (3..34).
     * Append `length` bytes, each copied from `distance` bytes back in
       the output. Copy byte by byte, so overlapping copies (distance <
       length) repeat a pattern.

Edge rules: a match whose distance exceeds the bytes produced so far is
corrupt; reject it. A copy must not write past the decoded size. In the
retail data the last token ends exactly at the decoded size, and the input
ends 1–3 bytes before the encoded size. Readers must stop on the output
count, not on the input position.

Vectors: `test-vectors/jdlz_vectors.json` holds plain bytes, the encoded
stream and the token list. One case, `jdlz_flag_refill`, contains 12
matches, which forces a B refill on an iteration that copies a literal.

### 13.3 HUFF (EA "30FB" canonical Huffman) (O for method 0x30FB; I for the others)
The stream after the 16-byte wrapper is the public Electronic Arts Huffman
format described in the psx-spx documentation (see 12). It is a big-endian,
most-significant-bit-first bit stream:
1. 16 bits method (0x30FB observed; 0x31FB–0x35FB are variants). If bit 8
   of the method is set, skip 24 bits.
2. 24 bits decoded size; 8 bits **escape symbol** E.
3. **Code-length histogram**: for width w = 1, 2, …, read a variable-length
   number n_w (the count of codes of width w). Keep a running value
   `c = 2c + n_w`, and stop after the width at which `c × 2^(16−w)`
   reaches 65536 (the code space is full).
4. **Symbol list**: for each of the Σn_w symbols, read a variable-length
   number k. Starting from the previously assigned byte value (initially
   255), step forward (mod 256) over byte values not yet assigned, and take
   the (k+1)-th of them as this symbol.
5. **Canonical codes**: walk the symbols in list order, widths ascending.
   Keep a 16-bit accumulator starting at 0. The code of a width-w symbol is
   the top w bits of the accumulator; then add 2^(16−w).
6. **Decode loop**: read one code → symbol s.
   * If s ≠ E: output s.
   * If s = E: read a variable-length number r.
     * r > 0 → repeat the previous output byte r times.
     * r = 0 → read 1 bit: if it is 1, the stream ends. Otherwise read 8
       bits and output them as a literal (this is how E itself is emitted).
7. Methods 0x32FB / 0x34FB additionally apply one or two running-sum
   passes (mod 256) over the output. Neither occurs in this game's
   frontend data.

**Variable-length number**: count zero bits until a 1 bit; let z be that
count. Read (z + 2) bits as v. The value is v + 2^(z+2) − 4.

Check: decoding the 6 retail HUFF packages gives byte-for-byte the same
packages that the 2023-modified installation stores uncompressed (13.4).
Vectors: `test-vectors/huff_vectors.json` (plain bytes, stream, escape,
code table). One case covers runs and an escaped literal.

### 13.4 Where compression appears
* **Whole files.** An original `.lzc` file (e.g. the frontend and global
  bundles) is one JDLZ stream whose output is a chunk bundle (14).
* **Screen packages** inside a bundle (chunk `0x00030210`, 3.2).
* **Compressed texture slots** (15.6).
* Detection: the first 4 bytes are `JDLZ` or `HUFF`; anything else is
  uncompressed.

## 14. Resource bundles and which files the GUI needs

### 14.1 Chunk bundle (O)
A bundle is a flat sequence of chunks `{u32 id, u32 size, payload}`.
* Id 0 = padding; skip it.
* Ids whose top nibble is 8 (`0x8xxxxxxx`) contain nested chunks.
* The texture pack ids `0xB3300000`, `0xB3310000` and `0xB3320000` also
  contain nested chunks (15.1).
* Every other id is a leaf; skip unknown leaves by their size.

In the original files, chunk headers that start texture or pixel data sit
on 0x80-byte boundaries, and package chunks on 16-byte boundaries. Readers
must not depend on this beyond the pixel-data rule in 15.5.

### 14.2 Files used by the frontend (O)
Paths are relative to the install root; match names case-insensitively.

| File | Content relevant to the GUI |
| --- | --- |
| `FRONTEND/FrontB.lzc` | ~136 screen packages; texture packs "front-end textures" (~496 images: menu icons, logos, part icons, magazine art) and a 3-image PC-specific pack; also a 3D model and other chunks the GUI can skip |
| `GLOBAL/GlobalB.lzc` (original: JDLZ) or `GLOBAL/GLOBALB.BUN` (the same bundle, stored uncompressed) | the **global texture pack** (181 images; most menu frame pieces, boxes, corners, highlights, arrows, sliders), a PC-specific global pack (30 images: pointer, key caps, PC pill buttons), and further screens (dialogs, help popups, PC help bar) |
| `GLOBAL/GLOBALA.BUN` | a 4-image pack (including the **main UI font atlas**), the main UI font (16), two small screens |
| `LANGUAGES/<Language>.bin` | the string table (8.3) and five fonts (16.4) |
| `LANGUAGES/LanguageTextures.bin` | compressed-slot texture pack holding the atlases of the language fonts |
| `LANGUAGES/Labels.bin` | same format as a string table, mapping label hash → label name (debug aid only) |

Texture resolution census over all PC frontend screens (O). Of 2,311
image resource references, 1,957 resolve in the global pack, 74 in the
front-end pack, 34 in the PC global pack, 2 in the PC front-end pack, and
the rest in in-game packs or nowhere (the HUD screens and developer
leftovers). Many front-end-pack images are **not** referenced by screens
at all; the program assigns them at run time (15.9).
Recommended lookup: build one map key → texture over all frontend packs
listed above. No key is duplicated among these packs, so order does not
matter. If a key is missing, report an unavailable-resource status and omit
the image (20.1). Authored kind-0 untextured quads remain geometry, not substitutes.

### 14.3 Original vs 2023-modified installation (O)
Users may have either form, possibly side by side: the modding tools left
the originals as `*.bacc` next to the modified files. The library must
accept both.

| Aspect | Original (`FRONTB.LZC.bacc`, `GLOBALB.LZC.bacc`) | Modified (installed `FrontB.lzc`, `GlobalB.lzc`) |
| --- | --- | --- |
| Whole-file compression | JDLZ (13.2) | none: the file is the raw chunk bundle |
| Screen packages | chunk `0x00030210` = u32 screen hash + JDLZ/HUFF/raw package | chunk `0x00030203` = raw package only (no hash); all 136 decompressed, including the 6 HUFF ones |
| Chunk order | model, texture packs, packages, misc. | texture packs first, packages, model last |
| Front-end texture pack | 496 images, pack name `FRONTEND` | 509 images (13 added by the mod), different pack name/path strings |
| Package contents | — | byte-identical to the decompressed originals (136/136 checked) |

Rules that work for both forms:
* Decompress a whole file when it starts with `JDLZ`.
* Accept packages from both `0x00030210` and `0x00030203`.
* Key a package by `hash(upper-case(package name))`, using the name stored
  in its own `PkHd` (3.3.1). This equals the hash prefix of `0x00030210` in
  136/136 cases, so the prefix is redundant.
* Never rely on chunk order, pack names or texture counts.

`GLOBALA.BUN` and the language files are also modified in that install.
The format is unchanged; only the chunk order and the string contents
differ.

## 15. Texture packs (O unless noted) [interop]

### 15.1 Chunk tree
```
0xB3300000  texture pack
  0xB3310000  header part
    0x33310001  pack info (124 bytes)
    0x33310002  key list
    0x33310004  texture records (table variant)        - or -
    0x33310003  compressed slot table (slot variant)
    0x33310005  format descriptors (table variant only)
  0xB3320000  data part
    0x33320001  data info (24 bytes)
    0x33320002  pixel data area
```
Padding chunks (id 0) may sit between any of these.

### 15.2 Pack info `0x33310001` (124 bytes)
| Off | Type | Meaning |
| --- | --- | --- |
| 0 | u32 | version, 5 |
| 4 | char[28] | short pack name, NUL-padded |
| 32 | char[64] | logical pack path, NUL-padded |
| 96 | u32 | hash (2) of the path exactly as stored (not upper-cased) |
| 100 | 24 bytes | zero |

`0x33320001` (24 bytes) holds zeros, the value 1 at offset 8 and the same
path hash at offset 12. Neither chunk is needed to decode textures.

### 15.3 Key list `0x33310002`
n entries `{u32 texture key, u32 0}`, sorted by key ascending. In the table
variant, entry i describes the same texture as record i.

### 15.4 Texture record (124 bytes; table variant `0x33310004`, n records)
| Off | Type | Meaning |
| --- | --- | --- |
| 0 | 12 bytes | zero |
| 12 | char[24] | texture name, NUL-terminated. **Truncated to at most 23 characters** |
| 36 | u32 | texture key = hash(upper-case **full** name). Do not recompute it from the stored name: 89 of 496 front-end names are truncated |
| 40 | u32 | class key (opaque; ~10 distinct values observed, not needed) |
| 44 | u32 | 0 |
| 48 | u32 | data offset, relative to the pixel data base (15.5); always a multiple of 0x80 |
| 52 | u32 | palette offset, relative to the same base (palettized formats only) |
| 56 | u32 | data size in bytes, all mip levels |
| 60 | u32 | palette size in bytes (1024 when palettized, else 0) |
| 64 | u32 | size of mip level 0 in bytes |
| 68 | u16, u16 | width, height (all frontend textures are powers of two) |
| 72 | u8, u8 | log2(width), log2(height) |
| 74 | u8 | **pixel format** (15.7) |
| 75 | u8 | 0 |
| 76 | u8 | 0 |
| 77 | u8 | 1 if palettized |
| 78 | u8 | number of mip levels including level 0 (1–8) |
| 79–123 | | render hints and constants (opaque; per-texture byte values vary; not needed by the GUI) |

Size identity (O, 714/714 records): the data size equals the sum of the
level sizes for levels 0..m−1. Each level halves width and height (minimum
1). A DXT level occupies ceil(w/4) × ceil(h/4) blocks.

### 15.5 Pixel data base
The `0x33320002` payload starts with filler bytes of value 0x11. They pad up
to the next file offset that is a multiple of 0x80, counted from the start
of the (decompressed) bundle. That aligned offset is the **data base**. In
every observed file the `0x33320002` header itself sits on a 0x80 boundary,
so the filler is 120 bytes. Compute the base by alignment, **not** by
skipping 0x11 bytes: real texture data can begin with 0x11. One in-game pack
does exactly that, so a filler-skipping reader lands 3 bytes too far.

### 15.6 Compressed slot variant `0x33310003` (used by `LanguageTextures.bin`)
n records of 24 bytes:
| Off | Type | Meaning |
| --- | --- | --- |
| 0 | u32 | texture key |
| 4 | u32 | **absolute file offset** of the compressed blob |
| 8 | u32 | blob size (encoded) |
| 12 | u32 | decoded size |
| 16 | u32 | 256 (opaque) |
| 20 | u32 | 0 |

Each blob is JDLZ (HUFF is also possible in other files). Decoded, it is
`pixel data (all levels) |  124-byte texture record (15.4) | 32-byte format
descriptor (15.8)`. The record therefore starts at decoded size − 156. Its
data offset field is meaningless here, because pixel data starts at blob
offset 0. Language font atlases are all single-level DXT3, except the large
bold font, which is BGRA8.

### 15.7 Pixel formats
| Value | Format | Decoding |
| --- | --- | --- |
| 0x24 | DXT3 (BC2) | per 4×4 block: 8 bytes of 4-bit alpha (pixel i = bits 4i..4i+3, value × 17), then a colour block that **always** uses 4-colour mode |
| 0x22 | DXT1 (BC1) | per 4×4 block: c0, c1 (RGB565), 32 bits of 2-bit indices; c0 > c1 → 4 colours; else 3 colours + index 3 transparent black |
| 0x26 | DXT5 (BC3) | public format; not present in frontend data |
| 0x20 | 32-bit | 4 bytes per pixel in order **B, G, R, A** |
| 0x08 | 8-bit palettized | 1 index byte per pixel; palette of 256 entries × 4 bytes, alpha in byte 3 with the full 0..255 range |

RGB565 expansion: r = (r5 << 3) | (r5 >> 2), g = (g6 << 2) | (g6 >> 4),
likewise b. Interpolated colours: (2·c0 + c1)/3, (c0 + 2·c1)/3 or
(c0 + c1)/2, per channel. A ±1 difference from GPU rounding is acceptable.
Pixel order within a block is row-major from the top-left.

Palette byte order: alpha is byte 3 (O). For the colour bytes, the front-end
and global palettes contain only greys and white, so the order cannot be
observed there. The test vector stores B, G, R (as for format 0x20), and
the repository's own world-texture notes assert R, G, B for world
palettes. Treat this as I: frontend output is identical either way.

Frontend format census (O): front-end pack DXT3 481 / DXT1 14 / P8 1;
global pack DXT3 138 / DXT1 29 / P8 10 / BGRA 4; PC global pack DXT3 23 /
P8 4 / BGRA 3.

### 15.8 Format descriptor (`0x33310005`, 32 bytes per texture, record order)
`u32 0, u32 0, u32 alpha-flag (0/1), u32 5, u32 (2 or 6),
4 bytes surface format, 8 bytes 0`. The surface format is the four-character
code `DXT1`/`DXT3` or a little-endian Direct3D format number: 21 = 32-bit
ARGB, 41 = 8-bit palettized. It is redundant with record byte 74; use it
only as a cross-check.

### 15.9 How screens reference textures, and program-assigned textures
* An image object's resource (3.3.4) names a file such as `X.tga`. Its
  texture key is hash(upper-case base name without extension) (2). The key
  is looked up in the map built in 14.2.
* UVs (3.4.3 words 17–20) are normalised to the whole texture: (0,0) is the
  top-left texel corner and (1,1) the bottom-right. Use level 0 with
  bilinear filtering (G); the frontend does not visibly use mip levels.
* Every frontend image is drawn alpha-blended with tint modulation (1.4)
  (I). The texture records' render-hint bytes are not used for frontend
  quads.
* **Program-assigned textures (I).** Pools of item images, such as the icons
  of an icon scroller, carry a placeholder resource in the screen file. The
  application replaces their texture at run time with an icon from the
  front-end pack, chosen by name. Icons are 128 × 64 textures drawn into
  the 118 × 59 slots of 6.4. Icon name families [interop] are `MAIN_ICON_*`
  (main menu: career, new career, load career, quick race, go online, LAN,
  split screen, customize car, options, profile management), `OPTIONS_ICON_*`,
  `CRIB_ICON_*`, `TRACK_ICON_*`, `PERFORMANCE_ICON_*`, `VISUAL_PART_*`,
  `PAINT_ICON_*`, `ONLINE_ICON_*`, `DECAL_ICON_*`, `AUDIO_ICON_*`,
  `NEON_ICON_*` and `GENERIC_ICON_*`. The item title shown under the slot
  comes from the string table. The main menu labels are `UI_MAIN_<ITEM>`,
  with item suffixes `CAREER`, `QUICK_RACE`, `SPLIT_SCREEN`, `ONLINE`,
  `ONLINE_LAN`, `CUSTOMIZE` and `OPTIONS`; the title label is
  `UI_MAIN_TITLE`. Which items appear on PC, and in what order, is
  application behaviour (see 18 and Unknowns).

Vectors: `tpk_table.hex` with `tpk_table.expected.json` (five textures:
BGRA, DXT1 with 2 levels and a transparent block, DXT3, palettized, and a
name longer than 23 characters), and `tpk_slots.hex` with
`tpk_slots.expected.json` (two JDLZ-compressed slots).

## 16. Fonts (chunk `0x00030201`) (O for layout; I for rendering rules) [interop]

### 16.1 Chunk layout
| Payload off | Size | Meaning |
| --- | --- | --- |
| 0 | 256 | font name (lower case, NUL-terminated, e.g. `conduitmditc_tt21i`) followed by an unused build path |
| 256 | 256 | atlas texture name (NUL-terminated), then unused bytes |
| 512 | … | font block (16.2); offsets below are relative to its start |

### 16.2 Font block header (128 bytes)
| Off | Type | Meaning |
| --- | --- | --- |
| 0 | char[4] | `FNTF` |
| 4 | 2 × u16 | opaque |
| 8 | u16 | font version: 0x19E (fonts with a texture block) or 0x16E |
| 10 | u16 | number of glyphs |
| 12 | 2 × u16 | 9, then opaque flags (13, 12 or 4) |
| 16 | u16 | 0 |
| 18 | u8 | **line height** (unscaled units) |
| 19 | u8 | opaque |
| 20 | u32 | glyph table offset (0x80) |
| 24 | u32 | kerning table offset (0 = none) |
| 28 | u32 | font block size |
| 32 | i32 | −1 |
| 36 | u32 | **cell height**: the largest (y offset + glyph height) over all glyphs, i.e. the full height of a text line including descenders |
| 40 | u32 | 0 |
| 44 | f32 | 1.0 |
| 48 | u32/f32 | version 0x19E: offset of a 32-byte texture block whose last two u32 are the atlas width and height. Version 0x16E: f32 0.5 |
| 52 | u32/f32 | 127 / f32 0.5 |
| 56 | 2 × f32 | 1.0, 1.0 |

### 16.3 Glyph record (16 bytes, sorted by character code ascending, O)
| Off | Type | Meaning |
| --- | --- | --- |
| 0 | u16 | character code (Unicode; e.g. curly quotes are 0x2018–0x201D, ™ is 0x2122) |
| 2 | u8, u8 | glyph width, height in atlas texels |
| 4 | u16, u16 | glyph top-left x, y in the atlas (texels) |
| 8 | u8 | 0 |
| 9 | i8 | x offset from the pen position to the glyph's left edge |
| 10 | i8 | y offset from the **top of the line** to the glyph's top edge (Western fonts: 0..cell height). Korean fonts use negative values measured from the baseline (I) |
| 11 | u8 | number of kerning entries for this glyph |
| 12 | u16 | index of its first kerning entry |
| 14 | u16 | advance width (unscaled units) |

Kerning table: `u32 count`, then `count` entries of 4 bytes:
`{u16 previous character code, i8 adjustment, u8 low byte of this glyph's
code}`. Each glyph's entries list the characters that may **precede** it,
e.g. the period's list contains `P` −4, `F` −3, `Y` −3. The codes in
kerning entries are compared with the codes of the drawn characters. A few
entries use legacy single-byte quote codes (0x91–0x94) that never match
Unicode text; they are harmless.

### 16.4 Font inventory and lookup (O)
* A text object's font resource names a file `<Name>.ffn` (3.3.4). The font
  is the `0x00030201` chunk whose font name equals the lower-cased `<Name>`.
* The **main UI font** `conduitmditc_tt21i` lives in `GLOBAL/GLOBALA.BUN`.
  It has 211 glyphs, line height 26, cell height 32, a 256 × 256 DXT3 atlas
  in the same file, and 89 kerning pairs. It is named by 201 of the 243 font
  resource entries across all screens.
* Each `LANGUAGES/<Language>.bin` carries five more fonts: `font_arial`,
  `conduitmditc_tt14i` (small; line 16, cell 20), `lcd_let48` (12 digit
  glyphs), `font_conduitmditctt38bi` (large bold italic; line 32, cell 39),
  `font_arial12`, `font_impact36`. The Asian language files carry six fonts,
  with localised variants of the two Conduit fonts (larger atlases, glyphs
  up to U+D7xx). Their atlases are in `LanguageTextures.bin` (15.6). The
  atlas key is hash(upper-case atlas texture name).
* Screens also name `Font_ImpactHud.ffn`, which has no font chunk (HUD
  only). Missing fonts produce unavailable-resource status (20.1); do not
  silently substitute the main UI font.
* Resource-name to font-name matching is case-insensitive:
  `ConduitMdITC_TT21i.ffn` → `conduitmditc_tt21i`,
  `Font_ConduitMdITCTT38bi.ffn` → `font_conduitmditctt38bi`.

### 16.5 Text layout and rendering (I unless noted)
Units: one atlas texel = one unscaled unit; at text scale s every
dimension is multiplied by s (scale = object size.x/size.y, 1.2).
For one line, with pen = 0 and prev = none, for each character c:
1. If c has no glyph, skip it and reset prev (G).
2. If prev exists and c's kerning list has an entry for prev, add its
   adjustment to the pen.
3. If the glyph has a non-zero size, draw the atlas rectangle
   `(x, y, w, h)` as a quad from `(pen + xoff, lineTop + yoff)` with size
   `(w, h)`. Texture coordinates = texel rectangle / atlas size. Colour
   = glyph texel × text colour (1.4).
4. pen += advance; prev = c.

* **Line width** = pen after the last character, kerning included.
  (R, amendment 3: measured ink widths of 15 captured strings divided by
  the widths computed this way give the authored scales to within 0.02,
  e.g. 0.74 for scale-0.75 dialog text and 0.88 for a scale-0.9 title.)
* **Line pitch** (amendment 3, corrected; I from one capture) = (**cell
  height** + leading `Sl`) × scale, not the line-height byte. Evidence: a
  two-line dialog message (main font, cell height 32, leading 2, scale
  0.75) has a measured baseline pitch of 25.4 ± 0.3 units. (32 + 2) × 0.75
  = 25.5 fits; (26 + 2) × 0.75 = 21.0 does not. Line k's top = first line
  top + k × pitch. The line-height byte (16.2 offset 18) has no observed
  use; keep it informational. Confirm with a second multi-line capture
  (verifier tracking id A13, section 18).
* **Block height** for vertical alignment in scaled screen units =
  (lines − 1) × pitch + cell height × vertical scale. Check: the help-bar label (vertical-centre justification, scale
  0.7) then centres capital letters on the strip centre to within 0.5 unit,
  because capitals span y offsets 6..26 of the 32-unit cell.
* Horizontal alignment applies per line using its line width; vertical
  alignment uses the block height (8.4).
* Word wrap (`Sw` > 0): break at the last space before the line width
  would exceed the wrap width (unscaled). A word longer than the wrap width
  stays on its own line (G).
* Line breaks: `\n` and `^` (8.2).
* Vectors: `font_chunk.hex` (a synthetic 6-glyph font chunk with kerning
  and texture block) and `font_layout_vectors.json` (strings → quads).

## 17. Strings (amends 8.3) (O)
* A language file's first chunk is the string table `0x00039000` (8.3). It
  is followed by padding and five font chunks (16.4).
* Byte → character: bytes < 0x80 are ASCII. For bytes ≥ 0x80, the table's
  256-entry code map gives the Unicode code if non-zero, else the byte is
  Latin-1. The English table maps bytes 0x81–0x88 to typographic quotes,
  dashes, ellipsis and ™. The resulting code is what is looked up in the
  font (16.3).
* Inline markup in strings:
  * `$NAME$` glyph tokens (7.3; 416 spacer tokens and ~820 action tokens in
    English).
  * `^` = line break (1,124 occurrences).
  * C-style placeholders `%s`, `%d` and `%l…`, which the application fills in
    before display. There are no other escape characters.
* Choosing the language: `LANGUAGES/English.bin` (case-insensitive) for
  English. Other languages use their own file of the same format.

## 18. Reference-capture plan (for the verifier)

Each entry gives the original screen, how to reach it in the original
game, and the screen package that the re-implementation must load to
produce the same frame. Capture at 4:3 (640 × 480 preferred) with the
original, unmodded frontend files. The identifiers A1-A13 used in section 11 are
verifier-side tracking numbers for open observations (not defined in this
spec; each corresponds to an Unknown in section 11, and the R-entries below
are the testable capture requests). Comparisons should overlay the two
frames at equal virtual-screen scale (1.1).

| # | Screen | Reach in original | Package(s) | Compare |
| --- | --- | --- | --- | --- |
| R1 | Main menu, first item | boot, skip movies, pick profile | `UI_Main.fng` + `UI_PC_Help_Bar.fng` | panel, icon strip, title, help bar, bottom strip |
| R2 | Main menu after Right ×1 and ×3 | as R1 | same | icon spacing, highlight position, scroller thumb |
| R3 | Options hub | Main menu → Options | `UI_OptionsMain.fng` | icon scroller variant |
| R4 | Options list page | Options → a settings page | `UI_Options.fng` | row pitch, visible rows, slider row, value arrows, vertical scroller |
| R5 | PC controller config | Options → controller (PC) | `UI_Options_PC_Controller.fng` | PC-only objects |
| R6 | Quick race mode/track select | Main menu → Quick Race | `UI_QRModeSelect.fng`, `UI_QRTrackSelect.fng` | icon scroller + item titles |
| R7 | Car select | Quick Race → mode → track → car | `UI_QRCarSelect.fng` | stat bars, filter switcher, PC arrows |
| R8 | Small yes/no dialog | e.g. PC help bar "Quit Game" on main menu | `GenericDialog_SMALL.fng` over `UI_Main.fng` (dimmed) | dialog panel, border, buttons, dim |
| R9 | Large dialog | any long message (e.g. memory-card notices) | `GenericDialog_LARGE.fng` | wrapped message text |
| R10 | Help popup | Help on main menu / options | `HelpDialog_SMALL/MED/LARGE.fng` | legend layout, glyph rendering on PC |
| R11 | On-screen keyboard | profile name entry | `UI_VirtualKeyboard.fng` | key grid |
| R12 | Profile manager / save list | Main menu → profile management | `UI_ProfileManager.fng`, `MC_List.fng` | list rows |
| R13 | Paint screen | Customize → Paint | `UI_Paint.fng` | swatch row and highlight |
| R14 | Career crib | Career (with a save) | `UI_CareerCrib.fng` | icon scroller with lock badges |

For each R-entry the verifier records: the screenshot, the input sequence
used, and (for R2/R4) a short video at ≥30 fps to compare timings in
section 5 and key repeat (4.5).

Capture status (amendment 3): partially covered by the reference set of
19.1. R1 is covered as "main menu, item 2" (not item 1). R2 is covered for
the last item only. R4, R5 and R7 are covered (first row / first car). R8
is covered, but the dialog was opened from the PC quit pill, not with a
Back press. R9 is covered as a two-line LARGE confirmation. All others are
still open.

## 19. Reference comparison checklist (amendment 3)

### 19.1 Reference capture set
Six still captures of the original PC game, taken by the user on their own
machine at 1920 × 1080 (pillarboxed, 1.1), mouse active, English language.
The install contained third-party-modified data files (14.3). The screen
packages are byte-identical to the originals and every measured layout
matched the package data, but presentation details (pillarbox, texture
sharpness) may reflect the modifications. Conversion used: virtual x =
(pixel x − 960) / 2.25, virtual y = (pixel y − 540) / 2.25.

| Id | Screen and state |
| --- | --- |
| C1 | Main menu, second item (quick race) selected, cursor idle over the 3D scene |
| C2 | Options list page (audio settings), first row (a slider row) focused |
| C3 | Two-button quit confirmation (SMALL geometry) over the main menu, whose last item (options) is selected |
| C4 | Car select used for customizing, first car, all-cars filter |
| C5 | PC controls page (keyboard configuration), first row focused |
| C6 | Two-button save confirmation with a title and a two-line message (LARGE geometry) |

### 19.2 Checklist (virtual units; tolerance ±1 unless stated)
Every expected value below carries confidence **R** (measured on the captures of 19.1) unless it is a rule restated from another section; the checklist is a verification aid for comparing against the original, not a set of drawing instructions beyond the cited sections.

For historical PC comparison, the verifier renders the same screen and state
at 640 × 480 or any 4:3 size and checks the table below. Amendment 5
restores items 11–19 as current PC-profile expectations (20.7). In explicitly
selected console profiles those PC-specific items are superseded by 20.3.
The remaining geometry/text observations continue to apply to the corresponding
widgets.

| # | Capture | Item | Expected |
| --- | --- | --- | --- |
| 1 | all | non-4:3 window | 4:3 viewport centred, scale `min(W/640,H/480)`, black (0,0,0) bars, nothing drawn outside (cursor clipped) |
| 2 | C1 | icons visible | exactly 3 (selected + one neighbour each side) |
| 3 | C1 | selected icon | 118 × 59 centred (0, -133.5) |
| 4 | C1 | neighbour icons | 96 × 48 (±1.5) centred (±158, -133.5), full opacity |
| 5 | C1 | focus frame | line centres x ±60, y -161 and -106; colour at peak (202,244,138) |
| 6 | C1 | arrow badges | centres (±270, -132.5) |
| 7 | C1 | scroller | track y -176 .. -168.5; thumb x = -182.4 for index 1 of 7 |
| 8 | C3 | last item | no right neighbour, right arrow hidden, thumb x = +273.8 |
| 9 | C1 | item title | accent, scale 0.7, ink centre x 0 (±1.5), cap top y -95 |
| 10 | C1 | screen title | accent, scale 0.9, right-aligned, ink right edge x 286.7 (±1.5), cap top y -207.6 |
| 11 | C1, C2, C4, C5 | console strip (7.1) | **absent** |
| 12 | C1, C2, C4, C5 | PC strip | top edge y 189.2; left end x -300.4; player box to -151.8; gap to -148.9; runs to y 240 |
| 13 | C1 | player box tone | darker than the hint box behind it (not grey) |
| 14 | C1 | player name | centre (-225.7, 212.2), scale 0.5 |
| 15 | C1 | pills | slot 1 (+223,+226) quit, slot 2 (+223,+203) continue; visible outline ≈ 143.6 × 18.7; labels scale 0.45 white centred at y = slot y - 2 |
| 16 | C2 | pills | slot 1 quit, slot 2 back |
| 17 | C4 | pills | slot 1 quit, slot 2 continue, slot 3 (+76,+203) back |
| 18 | C5 | pills | slot 1 quit, slot 2 back, slot 3 reset keys |
| 19 | C1–C6 | Help pill / glyphs | none drawn |
| 20 | C2 | rows | 8 rows; text centres y = -126 + 36k (k = 0..7); name ink left x -269; scale 0.7 |
| 21 | C2 | focused row backing | x -286 .. +262, y -144.4 .. -109.6, dark; unfocused rows show no backing |
| 22 | C2 | value arrows | only on the focused row, centres x +6 and +246 |
| 23 | C2 | slider | fill from x 54.6, height ≈ 7, colour (99,178,34); knob centre x 196.6 at maximum |
| 24 | C2 | value text | centred x ≈ 127 |
| 25 | C2 | vertical scroller | track x 269–277; arrow centres y -163.5 and +167 |
| 26 | C2, C4, C5 | screen title | accent, scale 0.8, ink left x -288, cap top y -214 |
| 27 | C5 | rows | 7 visible, pitch 36, first row ink top y -101.8, scale 0.5, left x -275; focused name accent; focused backing y -110.2 .. -85.3 |
| 28 | C5 | header band | dark band y -174 .. -142, centred caption scale 0.7, arrows at x ±265.5, y -157 |
| 29 | C4 | carousel ends | left arrow hidden at index 0; right arrow centre (269.6, -143.6); thumb x ≈ -273 |
| 30 | C4 | stat bars | label ink left x -280 / -77 / +121, scale 0.55; fill height 6 at y ≈ 176, fill starts x -284 / -82 / +116 |
| 31 | C3 | SMALL dialog | border line centres x -297 / +298, y -91.6 / +109; light panel x -290 .. +290, y -49.4 .. +98.6 |
| 32 | C3, C6 | buttons | centres x ±150; visible x ±15.5 .. ±285; height 41.4; SMALL y 53.0 .. 94.4; LARGE y 144.4 .. 185.8 |
| 33 | C3, C6 | default focus | right button focused (dark backing, accent pulsing label); left button light backing, grey label |
| 34 | C3 | message | one line, centred (0, -3), scale 0.75 |
| 35 | C6 | LARGE dialog | border line centres x -298 / +297.6, y -214.2 / +199.4; light panel y -173.4 .. +190.6 |
| 36 | C6 | two-line message | centred on y -33; baseline pitch 25.4 (±0.5) |
| 37 | C6 | title without icon | accent 0.75, ink left x ≈ -289, cap top y ≈ -198 |
| 38 | C3, C6 | beneath | whole frame below the dialog × 0.40 (±0.03); PC help bar and strip not drawn |
| 39 | C1, C2, C6 | cursor | 32 × 32 virtual units, white arrow with dark outline |
| 40 | any | text widths | ink width of a string = font layout width (16.5) × scale, within 2% |

Values are measured on the captures (evidence kept analyst-side). Items 4,
5, 9 and 10 depend on the breathe/pulse phase at capture time only for
colour, not for geometry.

## 20. Current presentation — original visual assets, PC pills and console profiles

This section is normative for the current demo and original-screen presentation.
It takes precedence over every older permission to recreate art, substitute a
font, draw mouse/key badges, invent compact pill layouts, or show placeholder
art. Amendment 5 restores the retail PC text-pill presentation in 7.4/7.4.1
and 19 for the PC profile; PS2/Xbox retain their console presentation. Requirements
marked **D** are the user's requested product choices, not retail observations.

### 20.1 Visual fidelity and asset audit (D)

* Every visible texture, font atlas, button icon, cursor and application item
  image comes from a genuine user-supplied Underground 2 resource. Geometry,
  clipping, blend operations, authored text and layout are independently
  implemented according to this spec. Descriptive shape tables do not license
  synthetic pixel replacements. Full-font procedural glyphs, mouse outlines,
  key-cap artwork, invented menu/car icons, decorative invented backgrounds,
  generated logos and substitute movie art are disallowed.
* Audit all demo and library visual-producing paths, including embedded pixel
  arrays, procedural rasterizers, fallback drawing branches and build-generated
  art. Remove substitutes; retain only original-resource rendering and the
  specified geometry/text operations. Record each retained visual's resource
  lookup key, supplying file, format and applicable original-data provenance.
* Genuine original resources are loaded at runtime from paths supplied by the
  user, never embedded in source, committed, packaged, or copied into vectors.
  The demo can consume an original PC install for shared art plus an owned PS2
  disc image for controller icons, or the verified owned Xbox source in
  XBOX_CONTROLLER.md. Prefer verified original backup resources
  over modified equivalents as described in 14.3; report which source is used.
* Missing or unsupported visuals fail closed: return an unavailable-resource
  status and omit the affected image. Never substitute an unrelated original
  texture, fabricated glyph, generic font or magenta checker. If the required
  font or shared UI kit is missing, the faithful demo does not open a fake UI;
  a plain diagnostic outside the game presentation identifies what is missing.
  Diagnostics and file-selection controls are application tooling, not game art.

### 20.2 Corners and full projected transforms (O/I)

The genuine 16×16 rounded panel corner is a **bottom-right** quarter-disc in
native top-left-origin UV coordinates: the top and left edges meet the panel
fill; the outer arc cuts away the bottom-right. The texture itself supplies
its precise pixel mask, alpha and shade; do not replace it by a mathematical
circle. Texture keys [interop] for examples are `0xd253b8f3` (rounded panel),
`0x71609bc0` (layered variant), `0xd434926f` (black variant), `0xad749e02`
(white panel). Each is DXT3, 16×16 in the original PC global pack.

For a centred quad, normalized corner `(u,v)` corresponds to signed local
position `((u−1/2)·width,(v−1/2)·height,0)`. Retain the authored UV at that
vertex. Apply the full unit quaternion rotation in 3D, then the object's
translation and each parent transform, then project onto screen x/y. A
negative width or height reflects geometry exactly once; taking its absolute
value loses the reflection. Do not also reverse UVs after reflecting geometry.

Fresh mathematical definition for a unit quaternion `(qx,qy,qz,qw)` and a
vector `v`: `R(v) = v + 2·qw·(qxyz × v) + 2·qxyz × (qxyz × v)`.
Use 3D transforms even when the final image is planar. An authored half turn
about x projects as a vertical reflection and a half turn about y as a
horizontal reflection. A z-only extracted angle cannot reproduce these cases.

For the native panel texture, equivalent flat-screen orientations are:

| Panel location | Quaternion example | Reflected-size alternative |
| --- | --- | --- |
| bottom-right | `(0,0,0,1)` | positive width and height |
| top-right | `(1,0,0,0)` | positive width, negative height |
| top-left | `(0,0,1,0)` | negative width and height |
| bottom-left | `(0,0,√0.5,√0.5)` | negative width, positive height |

These alternatives match the panel mask; for nonsymmetric textures they are
not necessarily pixel-identical rotations. Always honor the actual authored
transform. Positive 90° about z turns clockwise in the screen's y-down plane.
Do not apply the panel orientation table to unrelated highlight/dialog corner
textures: their native masks differ. Panel fill meets its side strips along
straight inner edges; exterior pixels expose the background smoothly, with
no inward bite, detached square notch or repeated reflection. No per-screen
coordinate patch may mask a transform error. See `corner_transform_vectors.json`.

### 20.3 Strip and input families (D; geometry from O)

* In a selected PS2 or Xbox console profile, draw one original console strip
  using 7.1 and its original textures. Preserve
  authored player-information, label fonts, glyph scale and slot positions.
  Do not stack the taller PC help-bar strip over or behind it.
* In a selected PS2 or Xbox console profile, suppress the PC help-bar overlay
  and its clickable pill controls, labels and glows. This covers the help/navigation strip; original modal-dialog button
  art remains as described in 6.9. Keep mouse navigation usable through the
  widget input model. Removing the PC help overlay must not remove the Back,
  Accept or other actions from the input dispatcher.
* In console profiles the strip contains genuine original controller images followed by original
  action labels. It contains no added pill backing, mouse/keyboard glyph,
  procedural controller symbol or key-cap fallback.
* Provide explicit demo presentation-profile selection for PC text-only, PS2
  and original Xbox. Profile selection changes strip presentation and icon
  lookup, not available mouse/keyboard input. Default to PC text-only, including
  when a verified PS2 or Xbox source is available. An unavailable
  family yields a specific status; never silently relabel PS2 art as Xbox.
* A profile associates action tokens (7.3) with available original icon keys.
  No screenshot or dynamic execution established the retail PS2/Xbox action
  map. Require explicit profile mappings for those bindings; record them as
  demo configuration, not observed retail behavior. Unknown action bindings
  are reported and their glyphs omitted rather than guessed. An icon showcase
  may display each verified original icon with a descriptive demo label so
  both the resource decoder and asset provenance can be reviewed independently
  of unverified action mapping.

### 20.4 Genuine PS2 controller source and pack format (O)

The owned USA PS2 ISO's ISO9660 filesystem contains `NFSUNDER/ZZDATA1.BIN`
[interop]. Its contents include a standard texture-pack chunk `0xB3300000`
whose info path is `Global\GlobalTexturesPSX2.tpk` [interop]. The pack has
21 table records, including the following **19 controller textures**, each
32×32, format byte `0x04`, one mip level, data size 512, palette size 64.
Names below are runtime resource lookup identifiers [interop], not symbols.

| Texture name | Key | Appearance |
| --- | --- | --- |
| `PS2_TRIANGLE` | `0x1ac94ca9` | triangle within ring |
| `PS2_RIGHTANALOG` | `0x59efa983` | R within ring |
| `PS2_DPADUPDOWN` | `0x5e800209` | vertical direction pair |
| `PS2_L1` | `0x739d7070` | outlined L1 cap |
| `PS2_L2` | `0x739d7071` | outlined L2 cap |
| `PS2_L3` | `0x739d7072` | L3 within ring |
| `PS2_R1` | `0x739d7136` | outlined R1 cap |
| `PS2_R2` | `0x739d7137` | outlined R2 cap |
| `PS2_R3` | `0x739d7138` | R3 within ring |
| `PS2_LEFTANALOG` | `0xbb9b93f0` | L within ring |
| `PS2_CIRCLE` | `0xd9a22505` | circle within ring |
| `PS2_CROSS` | `0xe795717d` | cross within ring |
| `PS2_DPADD` | `0xe7a633b0` | down highlighted |
| `PS2_DPADL` | `0xe7a633b8` | left highlighted |
| `PS2_DPADR` | `0xe7a633be` | right highlighted |
| `PS2_DPADU` | `0xe7a633c1` | up highlighted |
| `PS2_START` | `0xe8b7d6a1` | outlined right-pointing triangle |
| `PS2_SELECT` | `0xfea90a33` | outlined horizontal rectangle |
| `PS2_SQUARE` | `0xff871084` | square within ring |

These icons use the game's original yellow-green accent palette. Do not
recolor them into modern PlayStation multicolored button conventions.

Resolve the ISO file through its directory records (little-endian extent and
length), rather than hardcoding an absolute disc sector. Within its bounded
file extent, candidate texture packs may be discovered by chunk magic and
validated against their child extents, pack-info version 5,  124-byte table
stride, required keys, dimensions and format. The observed complete pack is
49,024 bytes; this is a validation observation, not a required size for every
edition. Honor every chunk's declared bounds; never scan beyond the enclosing
file or accept a matching printable name alone as a validated resource.
Chunk walk, key-table and pixel-base rules from 15 apply. Align the pixel
payload start to 128 bytes relative to the texture container. In this pack
payload starts at byte 5000 and aligned base is 5120. Per-record data and
palette offsets are relative to that aligned base. The format-descriptor
chunk uses **84 bytes per record**, not the PC 32-byte stride; treating that
opaque descriptor as a PC pixel format is invalid. The  124-byte record fields
needed for these icons are as listed in 15.4.

Palette: 16 RGBA entries in sequential index order, 4 bytes each. Alpha is
`min(255,2·stored_alpha)` (full opacity at 128). The PS2 palette's color byte
order is **R,G,B**, distinct from PC format-0x20 BGRA. Palette bytes are used
without tint/color invention; normal image modulation still applies.

The narrow observed 32×32 index layout can be defined without emulator code.
For output texel x,y (0 through 31), compute the position `a` in the virtual
expanded 1024-nibble stream:

```
b = 32·(y AND NOT 15) + 2·(x AND NOT 15)
s = 4·(((y+2) >> 2) AND 1)
r = ((((y AND NOT 3) >> 1) + (y AND 1)) AND 7)
a = b + 64·r + 4·((x+s) AND 7) + ((y >> 1) AND 1) + ((x >> 2) AND 2)
index = (pixel_bytes[floor(a/2)] >> (4·(a AND 1))) AND 15
rgba(x,y) = palette[index], with alpha conversion above
```

This is a bijection of the 1024 nibble positions. It is the PS2 8-bit transfer
swizzle applied after low-nibble/high-nibble expansion of the 4-bit data.
Direct row-major nibble decoding, generic direct PSMT4 VRAM addressing and
PSMCT32/type-1 conversion all produced scrambled output for this pack.
This formula is restricted to these 32×32 controller textures; it does not
claim support for arbitrary PS2 textures. Synthetic vectors are supplied in
`ps2_icon_codec_vectors.json`. Public hardware context: [Sony texture-swizzling
document](https://ps2linux.no-ip.info/playstation2-linux.com/download/ezswizzle/TextureSwizzling.pdf).
Independent public codec comparison: [PS2Textures project](https://github.com/leeao/PS2Textures/blob/main/PS2Textures.py).

### 20.5 Source availability and Unknowns/Ambiguities

* **Observed:** the 19 PS2 icons above exist and decode coherently. Their strip
  placement derives from 7.1, but exact retail glyph sizing/baseline and action
  bindings still need console captures or further original observation.
* **Historical input limit:** the earlier PC/PS2 inputs did not identify a
  genuine Xbox controller family. With the newly supplied owned Xbox disc,
  XBOX_CONTROLLER.md defines the verified 47-glyph family, bounded XDVDFS
  source resolution and narrow BC1/BC2 decoder. This review permits genuine
  validated Xbox gallery art; it does not establish retail token/action maps.
  Without that verified source, Xbox remains unavailable. External redraws,
  generated letter circles, modern assets and other-family substitution remain
  prohibited. Raw image imports still require explicit original-game provenance.
* PS2 formats other than the narrowly observed 32×32 format 4 remain unknown.
  Xbox formats outside XBOX_CONTROLLER's narrow glyph scope, descriptor meanings,
  mipmapping, additional compressed sources, retail bindings and exact console
  glyph baselines/sizes remain unknown. No original game execution or console
  screenshots support this amendment. Unrelated bonus-content assets are excluded.
* The two round4 crops do not establish whole-panel positions or scale. Use
  authored geometry and existing captures for those facts; the crops identify
  the orientation defect to eliminate, not permission to redesign the panels.


### 20.6 Demo verification clarifications (D)

These clarify the requested presentation and demo tooling, not retail behavior.
Input/expected-output cases and their provenance are in
`VERIFICATION_ROUND4.md` (V4-1 through V4-4).

* With a PS2 or Xbox profile explicitly selected and the shared original PC
  UI resources loaded, menus using the bottom hint strip and their
  original-screen viewer render exactly one console
  strip, with the console hint family selected and the PC help overlay
  suppressed. This also applies without a PS2 source: omit unavailable
  controller glyphs, retaining the strip and action dispatcher (20.3).
  Historical assertions expecting PC hints or an absent console strip are
  superseded only for the selected console presentation. For the default PC
  text-only profile, the historical PC geometry applies as specified in 20.7.
* Every demo controller-family selection label identifies the selected
  family. Selecting Xbox with only the earlier PC/PS2 sources yields Xbox
  selection labels, unavailable-family status and no PS2 controller imagery.
  Selecting PS2 with the verified source yields PS2 selection labels; the
  icon showcase displays the 19 verified textures. Availability and selection
  are separate facts; unavailable selection must not acquire a PS2 label.
* On the controller icon showcase (demo page 11) at 1280×960, the selection
  label and F2/configuration explanatory text have disjoint vertical ink
  extents: one text's bottom edge is at or above the other's top edge.
  Neither is covered by the other. This is a demo-tooling layout requirement;
  it does not establish a retail glyph baseline, binding or console layout.
* In an all-screen resource verification tour using the console profile,
  a discovered historical PC help-bar overlay is reported as intentionally
  suppressed and excluded from unexpected load failures. Suppression alone
  does not cause a failure exit status. Unsupported or missing resources and
  GL errors remain separately reported and checked; this classification does
  not turn those outcomes into successful rendering.


### 20.7 Amendment 5 — restore PC text-only pill buttons (D; geometry O/R)

Source: the user's correction requesting restoration of PC pills without icons,
and the previously reviewed PC observations in 7.4/7.4.1 and 19. No new original
analysis or retail-console claim is introduced. This subsection supersedes
20.3/20.6 suppression rules only for the PC profile. Reviewed acceptance cases
are in `RESTORATION_PC_PILLS.md`.

* With shared original PC resources available, the demo and PC original-screen
  viewer default to the PC text-only profile. Render the original PC help-bar
  strip, dark player-information area, pill textures and labels at the geometry
  in 7.4/7.4.1. Do not draw the 32-high console strip in this profile. Restore
  the original pill/glow hover animation described in 5 and 7.4.
* Use the observed screen-specific slot assignments in 7.4.1: main menu has
  Quit Game and Continue; options has Quit Game and Back; PC controls adds
  Reset Keys To Default; car select has Quit Game, Continue and Back. Unused
  slots remain hidden; no Help pill is added. Localized labels use the original
  navigation strings and original font. These short functional label values
  are required for the requested presentation [interop].
* Each visible pill retains its 144×24 hit target centered at the slot position
  in 7.4, pointer hover/focus and click action. Dispatch the action identified
  by its label through the existing per-screen application action binding,
  exactly once per primary-click activation. Restoring the overlay preserves
  the existing Back, Accept, Quit and screen-specific input actions. Application
  behavior not established by 7.4.1 remains explicitly unknown in 11; this
  amendment does not invent new retail action mappings.
* PC pills are text-only: no generated PC, mouse, keyboard/key-cap or controller
  badge appears inside or beside them, and the invented mouse hint strip stays
  disabled. The genuine pointer cursor and unrelated original widget/menu art
  remain governed by 20.1. Restore original pill art at runtime, not a redraw.
* While a modal dialog is open, hide the PC help-bar strip and pills as in 6.9;
  restore the applicable slots after the dialog closes. Dialog button art is
  unchanged. In a PC verification tour the help-bar package is available for
  rendering and is not classified as intentionally suppressed.
* Selecting PS2 or Xbox retains 20.3's console strip, overlay suppression,
  icon provenance, explicit binding and unavailable-source behavior. Selecting
  PC again restores the text-only strip/pills without stacking either strip.
  All selection labels identify PC, PS2 or Xbox consistently. The original-only
  resource policy (20.1), full corner transforms (20.2) and PS2 codec (20.4)
  continue to apply in every profile.


### 20.8 Authored demo composition and verified Xbox source (D/O/I)

DEMO_DESIGN.md is normative for authored demo examples and tooling. Its D
composition choices do not override the authored geometry of original-screen
packages or the retail PC measurements in 19. XBOX_CONTROLLER.md supplies the
new observed Xbox source facts and public-format decoder rules; its D profile
requirements preserve 20.1–20.3 and 20.7. These documents supersede earlier
Xbox-unavailable statements only when the newly verified source is supplied.
The desktop default remains PC text-only, with one active profile strip and
full original corner transforms. Synthetic Xbox/filesystem vectors and their
provenance are listed in test-vectors/XBOX_README.md.


### 20.9 Amendment 7 — final-render dialog legibility and runtime checks (D)

`DIALOG_RENDER_ACCEPTANCE.md` is normative for the dialog-launcher/modal
acceptance in DEMO_DESIGN D2 page 8 and D3. Its freshly authored vectors
require final rendered glyph ink for every enabled choice at both acceptance
sizes and across all focus destinations. Text-state, tint, alpha or mirrored
software calculations alone cannot establish rendered acceptance. Preserve
original font/art, approved palette and alpha semantics, full corner transforms,
authentic focused pulse, modal behavior and restored PC chrome. The intentional
zero-alpha phase of the focused pulse in 5 remains valid; it cannot excuse an
invisible unfocused label. These are D acceptance refinements, not new retail
observations or a prescribed implementation architecture.

The approved runtime-check configuration in that document permits supplying
the named owned PC/PS2/Xbox sources solely to the recreated implementation's
loader/render interfaces and inspecting its own outputs. It does not permit
direct original binary/resource inspection, original executable execution,
analyst/evidence access by an implementer, or copying retail assets into the
specification/repository. This limited permission supersedes a blanket ban on
passing original source paths to recreated runtime checks, and no other barrier.
All Amendment 6 demo and Xbox requirements continue to apply.
