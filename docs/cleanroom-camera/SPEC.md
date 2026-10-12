# In-game (driving) camera — behavioural specification

Status: draft for review. Scope: the player's driving camera in races and free roam (all
selectable views, look-back, motion, shake, collision handling), the per-car camera data in
the retail data files, and the built-in tuning constants. Cinematic cameras (race intro,
jump/highlight cameras, post-race, crash/reset) are only partly covered; see §11.

Provenance: findings come from (a) reading the user's retail data files and (b) static
examination of the retail executable's behaviour by the analyst, cross-checked against
public community documentation of the per-car record (SpeedReflect "Nikki" library,
https://github.com/SpeedReflect/Nikki, file `Nikki/Support.Shared/Parts/CarParts/Camera.cs`
and `Nikki/Support.Underground2/Class/CarTypeInfo.cs`). No runtime capture of the camera has
been made yet; capture requests are listed in §12.

Confidence tags used on statements:

* **[data]** read directly from the retail data files (reproducible with the vectors).
* **[static]** behaviour determined by examining the executable; not yet confirmed at runtime.
* **[inferred]** follows from the above with one reasoning step (e.g. the meaning of a field).
* **[guessed]** plausible, untested.

---

## 1. Conventions

* Lengths are metres, times seconds, speeds m/s.
* **World frame**: right-handed, **+Z up**; X and Y are horizontal. [static]
* **Car frame**: origin at the car's position (the point the car's world transform
  translates to), **+X forward, +Y left, +Z up**, right-handed. [data]+[inferred]: in the
  per-car record the front-left wheel has +Y and the driver-seat offset has +Y (left-hand
  drive); camera entries with positive "forward" values sit over the bonnet.
* **Heading frame** (chase views, §6.6): origin = car position, +X = smoothed heading
  direction, +Y = (world +Z) × X normalised (left), +Z = X × Y (up, roll-free). [static]
* **Camera space** (used by shake, §8): +x right, +y down, +z along the view direction.
  [inferred]
* **Binary angle units (BAU)**: 16-bit angles where 65536 BAU = 360°, 16384 = 90°.
  Signed fields use two's complement (−1342 BAU = −7.372°). [static]
* `clamp3(v, lo, hi)` clamps each component independently.
* `dt` is the elapsed time of the camera update in seconds. The original advances game time
  in fixed 1/60 s steps; all derived vectors use dt = 1/60 unless stated. [inferred]
* "Game clock" below means the game's monotonic time in seconds (the original keeps it in
  integer ticks of 1/4000 s). [static]

## 2. Views

### 2.1 View types

Each per-car data entry and each built-in tuning record is keyed by a **view tag**: [data]+[static]

| tag | view    | class   | player-selectable | built-in FOV (BAU / degrees) |
|----:|---------|---------|-------------------|------------------------------|
| 0   | FAR     | chase   | yes ("Far")       | 13020 / 71.521°              |
| 1   | CLOSE   | chase   | yes ("Close")     | 13020 / 71.521°              |
| 2   | BUMPER  | in-car  | yes ("Bumper")    | 16384 / 90°                  |
| 3   | DRIVER  | in-car  | **no**            | 16384 / 90°                  |
| 4   | HOOD    | in-car  | yes ("Hood")      | 16384 / 90°                  |
| 5   | DRIFT   | chase   | drift events only ("Super Far" in the options screen) | 13020 / 71.521° |

"Chase" views are FAR, CLOSE and DRIFT; "in-car" views are BUMPER, DRIVER and HOOD. The
class decides almost every behavioural branch below. [static]

The DRIVER view has data for every car but no menu option, cycle slot, or other path that
selects it was found. [static] An implementation may omit it.

### 2.2 The stored camera option

Each player has a stored **camera option** `opt` in 0…4. It maps to a view: [static]

| opt | view   | English label (language file, hash) |
|----:|--------|--------------------------------------|
| 0   | BUMPER | "Bumper" (`0xC3E9AE58`)               |
| 1   | HOOD   | "Hood" (`0x414F19D7`)                 |
| 2   | CLOSE  | "Close" (`0x5AE3441F`)                |
| 3   | FAR    | "Far" (`0x1EA4CEC2`)                  |
| 4   | DRIFT  | "Super Far" (`0x916039B4`)            |
| other | FAR  | —                                    |

The labels are string-table keys in `LANGUAGES/<Language>.bin` (key = the game's
string hash, `h = 0xFFFFFFFF; for each byte c: h = h*33 + c (mod 2^32)`). They are listed
because the options screen needs them. The option's title label is key `0xF6CCDC5F`
("Camera"). [data]+[static]

* **Default:** `opt = 3` (FAR) for both player slots when settings are reset or created. [static]
* **Persistence:** `opt` is stored in the per-player settings block. That block is written to
  and read from the save profile, so the choice survives across sessions. [static]
* Two player slots exist (split screen). Each has its own `opt`. [static]

### 2.3 Changing view

**Options screen** (outside a race): left/right moves `opt` through 0…3 with wrap-around.
DRIFT (4) cannot be chosen there. If `opt` is already 4, the screen shows "Super Far". [static]

**In-game "change camera" input** (the bindable "Change Camera" control): [static]

```
maxOpt = 4 if driftCameraAllowed else 3
if next:  opt = opt + 1; if opt > maxOpt: opt = 0
if prev:  opt = opt - 1; if opt < 0:      opt = maxOpt
view = viewForOption(opt)          # table in 2.2
if the car has a data entry for view and a driving camera is active:
    switch the driving camera to view (an instant cut, §6.12)
```

`driftCameraAllowed` is true when the current event is a drift event. It is also true when
the current event/track id is in 4174…4179; these ids have track maps in the install and
are probably the drift tracks [inferred]. Cycle order with next: Bumper → Hood → Close → Far
(→ Drift) → Bumper. [static] Vectors: `tv10_view_cycle.csv`.

**Event set-up** (quick race; career not checked): [static]

* Starting a **drift** event sets `opt = 4` (DRIFT camera).
* Starting any other event type sets `opt = 3` (FAR) if `opt` was 4. Otherwise `opt` is unchanged.

## 3. Per-car camera data (retail data format)

### 3.1 Location

* File: `GLOBAL/GlobalB.lzc`, which is the file the game loads. `GLOBAL/GLOBALB.BUN` is
  the same content decompressed. The container is the standard chunk stream
  (`u32 id, u32 size, payload`; ids with top nibble 8 are containers). [data]
* Chunk **`0x00034600`** holds the per-car table. Its payload starts with a run of `0x11`
  filler bytes (8 in retail), then **2192-byte records**, one per car model. In this install
  there are 46 records in `GLOBALB.BUN` and 47 in the installed `GlobalB.lzc` (a user mod
  added one car; the camera values of the other 46 are byte-identical). [data]
* Each record begins with the car's collection name (zero-terminated ASCII, 32 bytes), e.g.
  `PEUGOT`. Match the active car's model to its record by that name. [data]

### 3.2 Camera block inside a record

Offsets are from the start of the 2192-byte record. [data]

| offset | type | meaning |
|---:|---|---|
| `+0x720` | u32 | number of entries in the **primary set** (6 in all retail records) |
| `+0x724` | u32 | number of entries in the **secondary set** (6 in all retail records) |
| `+0x728` | 8 bytes | zero |
| `+0x730` | 6 × entry | primary set |
| `+0x790` | 6 × entry | secondary set |

Entry (16 bytes): [data] for layout, [static] for meanings:

| offset | type | meaning |
|---:|---|---|
| +0 | u16 | view tag (0–5, §2.1) |
| +2 | s16 | **pitch** in BAU. Positive means the camera looks downward (§6.7). |
| +4 | f32 | **forward offset** (car-frame X) of the eye, metres. Negative is behind the car origin. Community tools call this field "lag"; it is a position, not a time. |
| +8 | f32 | **up offset** (car-frame Z) of the eye, metres |
| +12 | f32 | **lateral offset** (car-frame Y, +left) of the eye, metres |

Lookup: within the chosen set, scan the first `count` entries and take the first whose tag
equals the requested view. [static]

**Set choice:** the primary set is used normally. The secondary set is used only while the
game runs in its two-local-viewport (split-screen) arrangement. [static] for the switch,
[inferred] for what it means.

### 3.3 Observed values (primary set)

Identical for all 46 retail cars unless a range is given. The published `test-vectors/tv01_car_camera_entries.csv` holds only the three cars used by other vectors; the full per-car table is deliberately not published (see `test-vectors/README.md`). The synthetic file `tv01b_synthetic_car_records.csv` exercises the record format. [data]

| view | pitch (BAU / °) | forward | up | lateral |
|---|---|---|---|---|
| FAR | 0 / 0 | −5.8 (three cars use −6.7 or −5.5) | 1.7 (three cars: 2.3 or 1.9) | 0 |
| CLOSE | 0 / 0 | −4.7 | 1.7 | 0 |
| BUMPER | 500 / 2.747 | 0.716 … 1.016 (per car) | 1.03 … 1.09 (per car) | 0 |
| DRIVER | 0 | −0.195 | 0.805 | −0.44 |
| HOOD | 0 or 560 / 3.076 | 0.25 … 1.1 (per car) | 1.2 … 1.7 (per car) | 0 |
| DRIFT | 3040 / 16.699 | −5.8 | 3.45 | 0 |

Secondary set (split screen): FAR (0, −8.2, 1.85, 0), CLOSE (0, −6.8, 1.7, 0), BUMPER
(−1342 BAU, per-car forward, up − 0.15, 0), DRIVER (0, −0.26, 0.772, −0.315), HOOD per car,
DRIFT (2027 BAU, −8.2, 3.45, 0). [data]

Check value: DRIFT aims its look-at point (§6.7) at height 3.45 − 5.8·tan 16.699° = 1.71 m
over the car origin, which matches FAR's 1.7 m. [data]+[inferred]

The camera depends on the car only through these entries. No car bounding box or other
dimension enters the camera maths. [static] For scale, the PEUGOT body spans z ≈ −0.01 … 1.23 m
and x ≈ −1.9 … +1.9 m in its car frame. [data]

## 4. Built-in per-view tuning (constants in the executable)

Each view type has a tuning parameter set **K** (per-view table below). The retail values are
fixed constants of the game, not stored in any data file; an implementation must embed them.
Values are the effective ones to use (the original combines two internal presets per view with
equal weight; only the combined result is specified). Unlisted entries are 0. [static]
Full table: `tv02_view_tuning_effective.csv`. Meanings: `T_*` are smoother time constants;
`*K` are gains applied to acceleration; `*Min`/`*Max` clamp the resulting offset; `T_head` are
per-axis heading smoothing times. The index column is only a row label, not a required layout.

| index | name | FAR / DRIFT | CLOSE | HOOD | BUMPER / DRIVER |
|---|---|---|---|---|---|
| 0 | `T_eye` (s) | 1.0 | 1.0 | 0.6 | 0 |
| 1 | `T_look` (s) | 0.35 | 0.35 | 0.6 | 0 |
| 2 | `T_fov` (s) | 0.75 | 0.75 | 0.75 | 0.75 |
| 3 | `T_up` (s) | 0.75 | 0.5 | 0.6 | 0 |
| 4,5,6 | `upK` | 0.004, 0.004, 0 | 0.003, 0.003, 0 | 0 | 0 |
| 8,9,10 | `upMin` | −1, −1, 1 | −1, −1, 1 | 0 | 0 |
| 12,13,14 | `upMax` | 1, 1, 1 | 1, 1, 1 | 0 | 0 |
| 16,17,18 | `eyeK` | −0.15, 0, 0 | −0.125, 0, 0 | −0.01, 0, 0 | 0 |
| 20,21,22 | `eyeMin` | −1.5, −1, −1 | −1.25, −1, −1 | −0.1, −0.1, −0.1 | 0 |
| 24,25,26 | `eyeMax` | 1.25, 1, 1 | 1, 1, 1 | 0.2, 0.1, 0.1 | 0 |
| 28,29,30 | `lookK` | −0.06, 0, 0 | −0.06, 0, 0 | 0, 0.005, 0 | 0 |
| 32,33,34 | `lookMin` | −1.25, 0, −1 | −1.25, 0, −1 | 0, −0.08, 0 | 0 |
| 36,37,38 | `lookMax` | 1.25, 0, 1 | 1.25, 0, 1 | 0, 0.08, 0 | 0 |
| 40,41,42 | `T_head` (s, per axis x,y,z) | 0.3, 0.3, 0.55 | 0.3, 0.3, 0.5 | 0.3, 0.3, 0.3 | 0 |

## 5. Smoother ("restarting cubic Hermite")

Every smoothed camera quantity is a scalar channel (a vector is three independent channels)
with: [static]

* state: value `p`, velocity `v` (units per unit of normalised time), target `P`, time
  constant `T`, flag `active`;
* end tangent `V1 = 0` always.

```
setTarget(x):  P = x;  if x != p: active = true
snap():        p = P;  v = 0;  active = false
step(dt):
    if not active: return
    A = 2(p − P) + v              # V1 = 0
    B = 3(P − p) − 2v
    u = dt / T  if T > 1e-5 else 1
    if u >= 10:  p = P; v = 0; active = false; return
    p = ((A·u + B)·u + v)·u + p
    v = (3A·u + 2B)·u + v
```

Each step restarts a cubic Hermite segment from the current (value, velocity) to (target,
0) and evaluates it at `u = dt/T`. The result behaves like a second-order low-pass filter
with slight overshoot (≈1 % for a unit step at T = 1 s, dt = 1/60). It depends on frame rate
(see `tv04_smoother_step.csv`, which includes a 30 fps case). T = 0 means "follow exactly".

## 6. Driving camera update (per frame, per player view)

The steps below run in this order every frame. Values set as **targets** in step 6.8 are
consumed by the smoothers in step 6.5 of the **next** frame, unless snapped (§6.12).
[static]

### 6.1 Requested and effective view

`requested` = the view chosen by §2. `effective` starts equal to `requested`. If `requested`
is BUMPER or HOOD, `effective` becomes **FAR** when any of these hold: [static]

1. `effective` was already a chase view (an earlier fallback) **and** the car is
   airborne (§6.9).
2. Placing the requested in-car eye at its data offset in the car frame would put it within
   0.5 m of another active car's collision box. Only cars whose reference point is within
   5 m (plus the 0.5 m margin) of the eye are tested.
3. The car is in an unidentified vehicle state that also selects special offset S2 (§6.7).
4. A world-collision probe hits something on the segment from the car's reference point to
   the point 2 m along the car's up axis. Inferred purpose: catches a car lying on its roof
   or wedged under geometry. [inferred]

When `effective` changes from one frame to the next, the snap flag is raised (§6.12):
switching to FAR is a hard cut, and so is switching back.

### 6.2 Tuning for the frame

`K` = effective tuning (§4) for `effective`. In **drift events**, and in the special event
type E11 (§6.7), `eyeK` is set to (0,0,0) for the frame, so no acceleration-induced eye
shift happens. [static]

### 6.3 Acceleration filter

`accAvg` = mean of the last 4 samples (fewer at start-up) of the car's world-space linear
acceleration vector, sampled once per camera update. [static] for the 4-sample mean;
[inferred] that the sampled vector is acceleration in m/s².

### 6.4 Smoother time constants

Set every frame: [static]

* eye channels (x, y, z): `T_eye`, divided by 1.3 when the view is a chase view and the car
  is airborne (§6.9);
* look-at channels: `T_look`;
* FOV channel: `T_fov`;
* up channels: `T_up` (all three);
* heading channels x, y, z: `T_head.x`, `T_head.y`, `T_head.z`.

### 6.5 Advance smoothers

`step(dt)` on the up, FOV, eye, look-at and heading channels, in that order. [static]

### 6.6 Heading target and frame

**Chase views.** If the car is airborne (§6.9) and no snap is pending, the heading target
is left unchanged (heading freezes in the air). Otherwise: [static]

```
vel = car world velocity;  f = car forward axis (unit)
d = dot(vel, f)
if d < 0: vel = vel − 2·d·f                 # reversing: mirror velocity to point forward
w = clamp((speed − 8) / 5, 0, 1)            # speed = |car speed| in m/s; w = 0 ≤ 8 m/s, 1 ≥ 13 m/s
h = w·normalize(vel) + (1 − w)·f
e = clamp(1 − (now − tGround)/2, 0, 1)      # tGround: §6.9
h.z = h.z · e                               # the vertical part fades out over 2 s in the air
heading.setTarget(h)                        # 3 channels
if snap: heading.snap()
```

Then build the **heading frame** from the current heading value `H`:
`X = normalize(H)`, `Y = normalize(Zworld × X)`, `Z = X × Y`, origin = car position. [static]

**In-car views.** The heading target is the car forward axis and is snapped every frame. The
frame used is the car's own frame: full orientation including pitch and roll, origin = car
position. [static]

Vectors: `tv05_heading_weight.csv`.

### 6.7 Base eye offset and look-at

`(pitch, fwd, up, lat)` = the data entry for `effective` (§3), unless a special case applies.
The cases are checked in this order: [static]

| case | condition | offset (fwd, lat, up) | pitch |
|---|---|---|---|
| S1 | special event type E11 (unidentified quick-race type; §13) | (−10, 0, 5) | 3003 BAU (16.5°) |
| S3 | look-back is held **and** the per-player "variant F" flag is set (§6.13; not set by default) | (−17.2, 0, 6.0); in split screen (−15.8, 0, 4.4) | 5200 BAU (28.56°); split screen 3540 (19.45°) |
| S2 | unidentified vehicle state (also forces fallback in §6.1) | (−5, 0, 1.5) | 910 BAU (5.0°) |
| — | otherwise | entry values | entry pitch |

```
t = tan(pitch)
off = (fwd, lat, up)
if chase view:  look = (0,        ·, up + fwd·t)      # aims at the car origin's vertical line
else:           look = (fwd + 1,  ·, up − t)          # 1 m ahead of the eye, tilted down by pitch
```

The look-at y comes only from the acceleration term (§6.8). The base lateral offset is not
copied to the look-at point. [static]

### 6.8 Targets

Let `aL` = `accAvg` rotated into the frame of §6.6 (rotation only). This applies only while
the acceleration-offset feature is enabled; it was enabled in every normal path examined.
An unidentified per-player flag can disable it, giving `aL = 0`. [static]

```
eyeTarget  = clamp3(eyeK ⊙ aL,  eyeMin,  eyeMax)  + off·(1.3 if chase view and airborne else 1)
lookTarget = clamp3(lookK ⊙ aL, lookMin, lookMax) + look
upTarget   = clamp3((upK.x·accAvg.x, upK.y·accAvg.y, upK.z·accAvg.z + 1), upMin, upMax)   # world-space accel
fovTarget  = built-in FOV of the effective view type (§2.1), in BAU, as a float
eye.setTarget(eyeTarget); look.setTarget(lookTarget); up.setTarget(upTarget); fov.setTarget(fovTarget)
```

`⊙` is component-wise multiplication. Note that `look` from §6.7 uses the **unscaled**
`off`. Vectors: `tv06_accel_offsets.csv`, `tv03_static_pose.json`, `tv03b_airborne_offsets.csv`.

Effect with FAR (inferred from the numbers): accelerating at +5 m/s² moves the eye 0.75 m
further back and the look-at 0.3 m back. Braking at −12 m/s² moves the eye 1.25 m forward
(clamped). Lateral acceleration tilts the up vector by about 0.004 per m/s², i.e. about 2° of
roll at 9 m/s².

### 6.9 Airborne

`airborne` = the car has 2 or fewer wheels in ground contact. The executable tests this
in three overlapping ways that all reduce to "≤ 2 wheels touching". [static] +
[inferred] simplification. The airborne effects apply only when the effective view is a
chase view.

`tGround` (used in §6.6) is set to the game clock on every frame in which more than one
wheel touches the ground. [static]

Airborne effects (chase views): eye base offset ×1.3 (§6.8), `T_eye` ÷ 1.3 (§6.4), heading
target frozen (§6.6), and the vertical heading component fades out over 2 s.

### 6.10 Up vector used for the view

* Chase views: `(up.x, up.y, 1)` from the up channels (world space). The z channel is
  ignored and 1 is used.
* In-car views: the car body's up axis (world space). [static] for the selection;
  [inferred] that this is the up axis of the car as rendered.

### 6.11 FOV

The FOV used for rendering is the FOV channel's current value, converted to integer BAU.
Each view type has a fixed target (§2.1) and every view change snaps, so in practice the
FOV is constant per view. **No speed- or nitrous-dependent FOV change exists in this
camera.** [static]

### 6.12 Snapping (cuts)

A **snap** sets value = target and velocity = 0 on the eye, look-at, up and FOV channels,
and on the heading channels for chase views. It happens after the targets of §6.8 are set:
[static]

* on the first frame after the driving camera is created;
* on the frame after a view change (§2.3) or an effective-view change (§6.1);
* **every frame** while the effective view is BUMPER or DRIVER. These views are rigid; their
  tuning is zero anyway. HOOD is not snapped and uses its smoothing (0.6 s).

### 6.13 Look-back

While the look-back control is **held**, look-back is on. Releasing it turns it off. Each
toggle notifies the renderer of a camera cut. [static]

Look-back has two variants, selected by a per-player flag. [static]

* **Variant M (mirrored), the default.** The final local eye and look-at points are
  mirrored through the vertical axis of the frame: `(x, y, z) → (−x, −y, z)`. Chase views
  then sit in front of the car looking back at it. In-car views look out of the rear at the
  mirrored position (BUMPER: ≈1 m behind the origin, at the same height, looking backward).
  [static]: the per-player flag that selects M is initialised to "M" when the player's
  camera controller is created, and no code path was found that clears it.
* **Variant F.** Used only if that flag is cleared. There is no mirroring, and special
  offset S3 (§6.7) places the eye high and far behind. Treat it as unused unless a capture
  shows otherwise. [static]

During look-back with HOOD, the car is hidden like in BUMPER (§10).

### 6.14 Final eye, look-at and view matrix

```
k = −1 if (look-back and variant M) else +1
eyeL  = (k·eye.x,  k·eye.y,  eye.z)          # current smoother values
lookL = (k·look.x, k·look.y, look.z)
eyeW  = frame origin + eyeL.x·X + eyeL.y·Y + eyeL.z·Z
lookW = frame origin + lookL.x·X + lookL.y·Y + lookL.z·Z
view  = lookAt(eyeW, lookW, up from §6.10)    # camera +z toward lookW, +y down, +x right
```

Then shake is applied (§8). For chase views, collision handling follows (§9). [static]

Consequence: the chase camera follows the car **position** rigidly, with no positional lag
in world space. All "lag" comes from (a) the heading direction smoothing (0.3 s horizontal)
and (b) the acceleration-driven offsets in the heading frame. [static]+[inferred]

## 7. Projection

The view's FOV `F` (BAU) is a horizontal angle. The projection scales are: [static]

```
xScale = 1 / tan(s0 · F / 2)
yScale = 1 / tan(s1 · F / 2)
```

Integer-angle rule [static, confirmed]: the half angles are first truncated toward zero to whole
BAU values, A0 = trunc(s0 · F / 2) and A1 = trunc(s1 · F / 2) (taken as 16-bit angles), and the
tangent is evaluated on those integer angles: xScale = 1 / tan(A0), yScale = 1 / tan(A1).
`tv09_projection.csv` is authoritative; the untruncated formula does not reproduce it exactly.

For the main player view(s): `s0 = 1.1`, `s1 = 1.1 × 1.15 × (H/W) = 1.265 × (H/W)`.
For other views: `s0 = 1`, `s1 = H/W`. W and H are the viewport width and height.
[static] for the formula; [inferred] that W and H are the viewport dimensions and that the
driving camera renders through a "main player view" (likely, but its id was not confirmed).
The near plane is **0.5 m**, or 0.4 m for the DRIVER view. The far plane is set elsewhere
(10000 m default; one view id uses 15000 m). [static]

At 640×480 the main player view gives FAR an effective horizontal angle of 78.67° and a
vertical angle of 67.85°; BUMPER gets 99.0° × 85.4°. See `tv09_projection.csv`. The
non-square result (pixel aspect ≈ 0.914) is what the formula yields. A screenshot capture
should confirm it (§12).

### 7.1 Project policy: widescreen projection (OpenUG2 addition, NOT original behaviour)

Everything in §7 above describes the original program. The policy below is an OpenUG2 project
decision layered on top; it must be selectable and the original behaviour must stay available.

* **original**: §7 exactly, with the real viewport W and H.
* **wide** (project default): the 4:3 result is the reference. Let `A = W/H` and `R = 4/3`.
  * If `A > R`: compute `xScale4`, `yScale4` by §7 (including the integer-angle rule) for a 4:3
    viewport (H/W = 3/4, same main-player-view choice). Then `xScale = xScale4 × R/A` and
    `yScale = yScale4`. The horizontal scale is divided by `A/R`; the vertical view is unchanged
    (the extra width is added to the horizontal view, "Hor+"). No further truncation is applied
    to the divided scale.
  * If `A ≤ R`: §7 exactly with the real W and H.
* Testable consequences: at 4:3 both policies give identical scales; at 16:9 the wide `yScale`
  equals the 4:3 `yScale` and the wide `xScale` is 0.75 of the 4:3 `xScale`.
* A command-line option selects the policy; the default is wide.
* Vectors: `tv11_projection_policy.csv` (derived, FAR and BUMPER at 640x480, 1920x1080 and
  2560x1080, both policies, main-view scaling on and off). The effective angles in that file are
  `2·atan(1/scale)`.

## 8. Camera shake

Applied every frame to all views after the view matrix is built. [static]

Inputs: `s` = car speed (m/s); `vmax` = 100, or a per-car maximum-speed value when one is
available (its source is unresolved); per-wheel road-surface type; event type; nitrous
state.

**Speed curve** — 5 control points, piecewise-linear over `x = 4·s / vmax`, clamped at both
ends:

| point | r0 | r1 | r2 | r3 |
|---|---|---|---|---|
| 0 (s = 0) | 0 | 0 | 1 | 1 |
| 1 | 0.1 | 0.5 | 1.2 | 1.1 |
| 2 | 0.3 | 0.8 | 1.5 | 1.2 |
| 3 | 0.6 | 1.0 | 2.0 | 1.3 |
| 4 (s ≥ vmax) | 1.0 | 1.0 | 3.0 | 1.4 |

**Surface table** — per surface type, two values `(fq, rough)`: NONE, ROAD, ROAD_WET,
ROAD_DRIFT, ROAD_SMOKE_1..3, ROAD_BRIDGE: (1, 0); DIRT (1, 0.05); GRAVEL (1, 0.02);
ROUGH_ROAD (1, 0.02); COBBLESTONE (3, 0.03); STAIRS (3, 0.048); PUDDLE (1, 0.01); DEEP_WATER
(1, 0.03); GRASS (1, 0.05); SIDEWALK (1.1, 0.03); WOOD, PLASTIC, GLASS, SOLID_WALL,
SEE_THROUGH_WALL (1, 0.01); PLANT, POST, PILLAR, METAL_GRATE, METAL, CHAINLINK, CAR (1, 0).
Surface ids 0–28 are in that order. The names are the engine's surface identifiers and
must match the track collision data's surface ids, which this spec does not cover.
The per-wheel surface id is the id carried by the track collision data at the wheel's ground
contact. How an implementation maps its own ground classes to these ids is its choice until the
track collision surface-id field is analysed. Status: open (§13 item 15).

```
r = speedCurve(s)
A = r0 · (1.0 if drag event else 0.2)
F = r2
if nitrous engaged and the car is not effectively stopped:  A = A + 0.2;  F = F · 1.4   # [inferred: nitrous]
G = r3 · mean over 4 wheels of fq
R = r1 · 10 · mean over 4 wheels of rough
freq1 = (7.5, 6.875, 6.125, 5.0) · F      amp1 = (0.003, 0.002, 0.7, 0.5) · A
freq2 = (3, 5, 7, 5.5) · G                amp2 = (0.007, 0.01, 0.6, 0.8) · R
c[i] = fbm(freq1[i] · t)·amp1[i] + fbm(freq2[i] · t)·amp2[i]      # t = game clock (s), i = 0..3
```

"Effectively stopped" means speed below about 2.2 m/s (5 mph) together with a near-zero
wheel quantity; the details were not fully decoded. [static]

**Noise**: `fbm(x) = Σ_{k=0..5} 0.5^k · n(2^k·x)`. Here `n(x)` linearly interpolates between
`L(floor x)` and `L(floor x + 1)`. `L(i)` is the well-known integer lattice noise ("Hugo Elias"):

```
m = (i << 13) XOR i                       (32-bit)
L(i) = 1 − ((m·(m·m·15731 + 789221) + 1376312589) AND 0x7FFFFFFF) / 1073741824
```

Vectors: `tv08_noise.csv`, `tv07_shake_params.csv`.

**Application** (camera space, after the view transform; [static] structure, [inferred]
axes and signs):

* `c0`, `c1`: translations in metres along camera x (right) and camera y (down).
* `c2`, `c3`: rotation angles in degrees, about camera x (pitch) and camera y (yaw).
  Pitch is applied first.
* Each angle `a` (converted to BAU with truncation toward zero) is rescaled for the current
  FOV before use: `a' = asin( sin(a) · sin(FOV/2) / sin(30°) )`, clamped to ±90°.

Typical size: on a smooth road at 100 m/s outside drag events, the pitch noise amplitude is
0.14°. Nitrous adds 0.14° and raises the frequency by 40 %.

## 9. Collision handling (chase views only)

Two passes run after shake. [static] for their existence and inputs; details below are
partial.

1. **World geometry.** World collision objects are gathered inside a sphere that encloses
   the segment from the car to the eye (radius = (|eye − car| + 1)/2, centred at the
   midpoint). A correction vector from the solver is added to the eye position. Further
   height adjustments may follow. The solver's algorithm is **unresolved** (§13).
2. **Other cars.** Other active cars visible in this view are considered when their position
   is within 12 m of the eye. Further relative-motion conditions involving a 15 m/s threshold
   were not fully decoded. For each such car the camera has a bounding sphere (centre c,
   radius r), and the eye is raised until it is outside the sphere enlarged by 0.1 m:
   `eye.z = c.z + sqrt((r + 0.1)² − (eye.x − c.x)² − (eye.y − c.y)²)`. This repeats until no
   car qualifies.

## 10. Render-side flags set by the driving camera

* In BUMPER and DRIVER views, and in HOOD during look-back, the player's own car is marked
  for exclusion from rendering. In normal HOOD and in chase views it is drawn. [static] for
  the flag; [inferred] for "exclusion from rendering".
* The renderer receives the eye-to-car distance every frame. [static]

## 11. Cinematic and special cameras (partial)

* Other camera kinds exist alongside the driving camera: an orbit-around-car camera (default
  radius 5 m, FOV 13020 BAU), a dyno camera, a customisation-shop camera, a showcase camera,
  scripted track/route cameras, an auto-pilot camera, and a cop-view camera. [static]
* The pause menu has on/off options labelled "Jump Cameras:" (`0xEE3EA0D4`) and "Highlight
  Cameras:" (`0xDCD8E7F0`). [data] Their triggers and camera paths were not decoded. Scripted
  sequences live in `NIS/Scene_*_BundleB.bun`; their format is out of scope here.
* Race-start intro, finish/post-race and crash/reset cameras: **unknown** (§13).

## 12. Capture requests (to confirm this spec at runtime)

Requested captures: per-view stills of a known
car at rest; a straight-line acceleration and braking recording in FAR view; a jump in FAR
and BUMPER views; look-back in FAR and BUMPER views; a 640×480 screenshot facing a known
square object; a drift-event start; a cycle through all views.

## 13. Unknowns and ambiguities

1. **Look-back variant F.** Variant M is the default (§6.13). The path, if any, that
   switches a player to variant F was not found. Capture should confirm M.
2. **Special event type E11.** A quick-race event type (mode index 11 of 0…12) forces offset
   S1 and turns off acceleration eye offsets. Which user-facing mode this is was not
   identified.
3. **Vehicle state S2.** The vehicle predicate that selects offset S2 and forces in-car
   fallback is unidentified. Possibly crash recovery or reset. [guessed]
4. **Car "position" origin.** The heading frame uses the car's world position. It is assumed
   to be the car model origin, i.e. the frame the car geometry is authored in. If the
   original uses a centre-of-mass offset, all eye heights shift. Capture needed.
5. **Sampled acceleration vector** (§6.3): assumed to be linear acceleration in m/s².
6. **Projection** (§7): whether the driving camera's view uses the 1.1 / 1.265 scale
   factors, and what W/H mean. Needs a screenshot.
7. **World collision solver** (§9.1): algorithm and parameters unresolved. Pass 2 is partial.
8. **Shake axes and signs** (§8): which camera axes and which sign. Small magnitudes, so the
   visual impact is low.
9. **vmax for shake**: the per-car maximum speed source is unresolved. 100 m/s is the default.
10. **Nitrous predicate**: identified as "nitrous engaged" by inference only.
11. **Timing**: the original's update `dt` (frame time vs fixed 1/60 step) is assumed to be 1/60.
12. **DRIVER view** is never selected. Its lateral offset (−0.44, i.e. right of centre) is
    unused.
13. Cinematic cameras, race intro, post-race, and crash/reset cameras are not specified (§11).
14. Secondary camera set: the split-screen interpretation is inferred.
15. **Per-wheel surface id source** (§8): the decoding of the surface-id field in track
    collision data is not done; the mapping is an implementation choice.
16. **Up-probe** (§6.1.4): the geometry set (all world triangles or collision-only) and the probe
    radius are undecoded. Any deviation in start point is an implementation choice, not a
    requirement; a car wedged with its reference point inside geometry may go undetected by a
    probe that starts above the reference point.
17. **Other-car box** (§6.1.2): only the 0.5 m margin and 5 m pre-filter are known; box
    dimensions are undecoded.
18. **Free roam set-up** (§2.3): not confirmed to count as an event; leaving the stored option
    unchanged on entering free roam is consistent with the text but unconfirmed.
