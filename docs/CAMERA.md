# Gameplay camera

The player's driving camera: six view types, a per-car camera record read from
the user's own `GLOBAL/GlobalB.lzc`, a smoothed motion model, shake and the
projection scales. It replaces the old fixed chase spring and landing-bounce
camera for ordinary driving. The behavioural source is the approved camera
specification (`docs/cleanroom-camera/SPEC.md`, SHA-256
`d4e52dd2e801db41e40742187a8f1e0687aa6a0240604e3afaca845d4dbaa3da`, plus
`docs/cleanroom-camera/test-vectors/`). Section references below (`SPEC n`) point there.

Code: `src/camera.c/.h` (pure maths, no GL/SDL), glue in `src/main.c`
(`GameCam`, `gamecam_*`), tests in `tools/camera_test.c`.

## Using it

| input | action |
|---|---|
| `C` | next view: Bumper, Hood, Close, Far (then Super Far in drift events), wrap (SPEC 2.3) |
| `Shift+C` | previous view |
| hold `B` | look back (SPEC 6.13); the view mirrors through the car's vertical axis |
| `F`, hold right mouse | existing free-flight camera / bounded orbit (unchanged, they override the gameplay camera while active) |

The choice is stored per player (two slots; slot 1 is driven) and written to a
small text file whenever it changes, so it survives restarts:

* `$OPENUG2_CONFIG_DIR/camera.cfg`, else `$XDG_CONFIG_HOME/openug2/camera.cfg`,
  else `$HOME/.config/openug2/camera.cfg`;
* lines `camera_option_p1=3`, `camera_option_p2=3`; default is `3` (Far).

Starting a drift event selects Super Far (option 4); starting any other race
event moves option 4 back to Far and otherwise leaves the choice alone
(SPEC 2.3). Drift view can be cycled to when the event is a drift event or its
id is 4174..4179.

Projection: `--projection wide|original` (SPEC 7.1, default `wide`). See D9.

Capture options: `--camera OPT` (0 Bumper, 1 Hood, 2 Close, 3 Far, 4 Super Far,
not saved) and `--look-back` (look-back held for the run). The ImGui "Camera"
panel shows the active view. Build `make camera-test` (optionally
`DATA="path/to/install"`) to run the vector tests.

## What stays on the old code

The menu orbit (`race_state == 3`), `--shot-yaw/--shot-pitch` captures, the
right-mouse orbit, the `F` free camera and `--chase D,H` keep the previous
spring/free-camera code (`render_camera_ease`, `render_free_camera`, the
`chase_*` sliders). The gameplay camera takes over whenever none of those own
the view. `phys_landing_camera` is still computed (its tests and callers are
unchanged) but is no longer added to the gameplay camera: the spec has no
landing bounce; vertical motion arrives through the car pose, the acceleration
terms and shake. `world_camera_clip` is reused for the world-collision pass.

## Spec coverage

| SPEC | where |
|---|---|
| 2.1 views | `CAM_FAR..CAM_DRIFT`, `cam_is_chase`; DRIVER exists in data/tuning but no input selects it |
| 2.2 option map, default, persistence | `cam_view_for_option`, `cam_option_label`, `CamSettings`, `cam_settings_*` |
| 2.3 cycling, event set-up | `cam_option_cycle`, `cam_option_menu_step`, `cam_option_for_event`, `cam_drift_allowed`; `gamecam_cycle`, `gamecam_event` |
| 3 per-car record | `cam_car_data_load` on top of `n2_global_car_record` (`nfsu2.h`), `cam_car_entry`; loaded per car by `gamecam_load_car` |
| 4 tuning | `k_tuning` in `camera.c` |
| 5 smoother | `cam_smooth_*` |
| 6.1 effective view | `cam_tick` + `gamecam_blocked` |
| 6.2-6.8 targets, heading, offsets | `cam_tick`, `cam_base_offsets`, `cam_targets`, `cam_heading_weight` |
| 6.9 airborne | wheels in contact from `PhysRideState.contact_mask`; `<= 2` is airborne |
| 6.10-6.12 up, FOV, snapping | `cam_pose`, end of `cam_tick` |
| 6.13 look-back | `cam_set_look_back`, mirror in `cam_pose` |
| 6.14 final eye/look/view | `cam_pose`, `cam_view_matrix` |
| 7 projection | `cam_projection_scales`, `cam_projection` |
| 8 shake | `cam_shake_params/values/apply`, `cam_fbm` |
| 9 collision | world pass `world_camera_clip`; car pass `cam_raise_over_sphere` (main.c render block) |
| 10 render flags | `CamPose.hide_car` -> `player_hidden` at the player draw sites |
| 11 cinematic cameras | not implemented (unspecified in the spec) |

Vectors: tv01 (the three published cars, round-tripped through a synthetic GLOBALB
image), tv01b (synthetic records with exact 16-byte entry encodings: parsed by
`cam_car_data_load`, declared-count limit, duplicate tag, tag not found), tv02,
tv03/tv03b (full settle through `cam_tick` + `cam_pose`, both look-back states,
three poses), tv04, tv05, tv06, tv07, tv08, tv09, tv10 and tv11 (projection
policy) all run from the files in `docs/cleanroom-camera/test-vectors`.

Retail comparison (`make camera-test DATA="path/to/install"`): the real GLOBALB is
always checked structurally (every car record loads, declared counts per set, view
tags 0..5, finite values). Values are compared only when the optional full per-car
table is supplied as a third argument (local file, not in the repository):
`./build/camera_test docs/cleanroom-camera/test-vectors "path/to/install" tv01_full.csv`
or `make camera-test DATA=... FULL=tv01_full.csv`. Without it the test prints that the
value comparison was skipped.

## DESIGN

Each decision is tied to a spec section. `Dn` ids are referenced from code.
The "unknown" decisions are isolated behind a named constant or input flag.

* **D1 (SPEC 3, 13.14) Missing record.** A car with no GLOBALB camera record
  (a mod car whose name is absent, an unreadable file) gets built-in entries
  (`cam_car_data_default`, the typical retail values of SPEC 3.3) and a message
  on stderr. The 47th record of the installed `GlobalB.lzc` is read normally.
* **D2 (SPEC 1, 6.5, 13.11) Time step.** The spec assumes a fixed 1/60 s
  update. The engine already ticks the player at 60 Hz, so smoothers, the
  acceleration filter and `tGround` advance inside the tick loop
  (`gamecam_tick`, dt = 1/60), independent of frame rate. The final eye/look
  are evaluated every render frame from the interpolated car pose
  (`cam_pose`), so the camera is rigid to the displayed car (SPEC 6.14). The
  shake clock is the tick clock plus the render fraction.
* **D3 (SPEC 2.2) Persistence.** The engine has no save profile, so the stored
  option lives in `camera.cfg` (above). Two slots are kept for fidelity with
  the two players; only slot 1 is driven (no split screen).
* **D4 (SPEC 3.2, 13.14) Camera set.** Primary set always; `CamInput.split_screen`
  selects the secondary set and is covered by the data loader tests but not
  driven (no split screen in the engine).
* **D5 (SPEC 13.4) Car origin.** The frame origin is the model origin: car pose
  plus the body up axis times the body ride height (`car_body_ride`), which is
  exactly where `mat_car` places the body. A centre-of-mass offset, if the
  original has one, would shift all heights; change `gamecam_tick` /
  the `gframe.pos` line in `main.c`.
* **D6 (SPEC 6.1.2, 6.1.4) In-car veto.** Other cars are tested as upright
  boxes from `AiCar.half_length/half_width/height` within 5.5 m of the would-be
  eye, margin 0.5 m. The 2 m up-probe uses `world_camera_clip` with radius
  0.05 m, starting 0.3 m above the car origin so the road under the car is not
  hit (`GAMECAM_PROBE_LIFT`). The probe only runs when an in-car view is
  requested. Condition 3 (vehicle state S2) has no engine equivalent
  (`CamInput.vehicle_s2` stays 0).
* **D7 (SPEC 8, 13.8-13.10) Shake.** Surface type per wheel: the engine knows
  only `WSURF_ROAD`/`WSURF_TERRAIN`; road maps to ROAD, terrain to GRASS
  (0.05 roughness), a wheel without contact to NONE. vmax is 100 m/s
  (`GAMECAM_SHAKE_VMAX`). "Drag event" = race kind drag, nitrous = the engine's
  `nitro_active`, "effectively stopped" = below 2.2 m/s. The camera-space axes
  and signs follow SPEC 1/8 literally: pitch about camera x then yaw about camera
  y, then a translation, converted to the GL view convention by conjugating
  with diag(1,-1,-1); amplitudes are millimetres and tenths of a degree.
  `GAMECAM_SHAKE 0` turns it off; it is also skipped in static captures
  (`sstatic`, frozen motion).
* **D8 (SPEC 6.3, 13.5) Acceleration.** Planar acceleration from the tick-to-tick
  difference of the world velocity (m/s^2), vertical 0 (every vertical gain in
  the tuning table is 0). The per-player flag that disables the acceleration
  term exists (`accel_offsets_disabled`) and is never set.
* **D9a (SPEC 7.1) Projection policy.** `cam_projection_scales(policy, ...)`
  with `CAM_PROJ_ORIGINAL` (SPEC 7 with the real viewport) and `CAM_PROJ_WIDE`
  (default). Wide: if W/H > 4/3, the 4:3 scales are computed (including the
  integer-angle truncation), then `xScale *= (4/3)/(W/H)` without re-truncation and
  `yScale` is kept; otherwise it is SPEC 7 exactly. Selected by `--projection
  original|wide` (an unknown value is a usage error). The option is per run and
  is not stored in `camera.cfg`. Vectors: tv11.
* **D9 (SPEC 7, 13.6) Projection.** `GAMECAM_MAIN_VIEW_SCALING 1` uses the
  1.1 / 1.265 scales ("main player view"); 0 uses the plain ones. The half
  angles `s * F / 2` are truncated to whole BAU: tv09 only reproduces that way
  (e.g. BUMPER x scale 0.854114, not 0.854081); see QUESTIONS.md. Near plane
  0.5 m (0.4 m DRIVER) from the spec; the far plane stays the engine's fog-derived
  value (SPEC 7: set elsewhere). The sky pass uses the same projection.
* **D10 (SPEC 9, 13.7) Collision.** World pass: the existing
  `world_camera_clip` (sphere sweep against world triangles) from an anchor 0.3 m
  over the roof (lowered to the eye height when the eye is lower), radius 0.55 m
  (`GAMECAM_CHASE_CLIP_RADIUS`, kept larger than the near plane so walls are not
  cut open). Car pass: eye raised over each present AI car's body sphere within
  12 m (`cam_raise_over_sphere`, margin 0.1 m, up to 4 passes); the 15 m/s
  relative-motion condition is not modelled. In-car views have no collision pass.
* **D11 (SPEC 6.7, 13.1-13.3) Unidentified states.** Offsets S1 (event E11), S2
  and S3 (look-back variant F) are implemented from the spec but their triggers
  are unidentified and stay off: `CamInput.event_e11`, `CamInput.vehicle_s2`,
  `CamState.variant_f` are 0. Look-back is variant M.
* **D12 (SPEC 6.13) Look-back.** Held `B`. The "notify the renderer of a cut"
  has no consumer in this engine (no motion blur or interpolation history for the
  camera), so no call is made.
* **D13 (SPEC 2.3) Event set-up.** Applied once per race event start/change
  (`gamecam_event`), not for plain free roam. Career is not specified.
* **D14 (SPEC 2.1, 13.12) DRIVER view.** Data and tuning exist and `cam_tick`
  supports it, but no key selects it.
* **D15 (SPEC 4 vs tv02) Look-at z clamp.** SPEC 4 gives +-0.75, tv02 gives +-1.
  `lookK.z` is 0 so the clamp never acts; the code uses the vector
  (`CAM_LOOK_Z_LIMIT`).
* **D16 (SPEC 6.1, 10) Hiding the car.** BUMPER/DRIVER, and HOOD during look-back,
  skip the player's shadow, body, glass, lamps, wheels and neon; headlight beams
  and the road lighting still cast. Implemented by `player_hidden` at the
  existing `follow_roam_ai` draw gates.
* **D17 (SPEC 6.12) Snapping.** The snap flag is consumed in the same tick that
  raised it, so a view change, a car change, a teleport or the first frame is an
  instant cut. In-car views snap every tick; HOOD smooths.
* **D18 States.** Countdown, racing and finish all use the driving camera.
  Cinematic cameras (SPEC 11) are not implemented.

## Known gaps

No split-screen, no cinematic/jump/highlight cameras, no per-surface ids (only
road/terrain), no per-car vmax, no world-collision solver beyond the existing
sphere sweep. The spec's capture requests (SPEC 12) have not been compared
against screenshots from the original; the tests are the derived vectors only.
