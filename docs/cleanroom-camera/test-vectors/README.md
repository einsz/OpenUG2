# Camera test vectors

Each file lists how it was obtained. "Data" vectors come straight from the user's retail
files. "Derived" vectors were computed by the analyst's reference model, which implements
the maths in SPEC.md. They check that an implementation matches the spec; they are not
runtime captures of the original game.

| file | kind | content |
|---|---|---|
| `tv01_car_camera_entries.csv` | data (subset) | Per-car camera entries (both sets) for only three cars (PEUGOT, NAVIGATOR, SKYLINE), the ones other vectors reference. Columns: car name, set, view tag, pitch (raw s16 and degrees), forward, up, lateral. Observed from the user's retail data; the full 46-car table is not published (project policy: no bulk retail data in the repository). |
| `tv01b_synthetic_car_records.csv` | synthetic | Two invented records (SYNTH_A, SYNTH_B), not from any retail file, with 16-byte entry encodings (hex, little-endian). Covers: both sets, all six view tags, signed pitch and BAU-to-degree conversion, declared count smaller than the stored slots, duplicate tag (first match wins), tag not found. |
| `tv02_view_tuning_effective.csv` | data (constants in the executable), averaged | Effective tuning vector K per view (SPEC §4) and FOV. |
| `tv03_static_pose.json` | derived | Steady-state eye and look-at, local and world, for 3 cars × 5 views × 3 poses × look-back on/off. Assumes no acceleration, speed ≥ 13 m/s (heading = velocity = car forward), settled smoothers, no shake, no collision. |
| `tv03b_airborne_offsets.csv` | derived | Chase-view eye offsets on the ground and airborne (×1.3). |
| `tv04_smoother_step.csv` | derived | Unit-step response of the SPEC §5 smoother for the FAR eye, look-at and heading constants, HOOD eye, and FAR eye at 30 fps. The target is set to 1 before each step; the value starts at 0. Row n is the state after n steps. |
| `tv05_heading_weight.csv` | derived | Speed → weight `w` in SPEC §6.6. |
| `tv06_accel_offsets.csv` | derived | Acceleration-induced eye/look-at deltas and up target per view for sample accelerations (heading frame = world frame in these rows). |
| `tv07_shake_params.csv` | derived | Shake frequencies and amplitudes (SPEC §8) at sample speeds, on three surfaces (same surface under all four wheels), drag on/off, nitrous on/off, vmax = 100. Nitrous rows assume the car is not "effectively stopped", including the speed-0 rows. |
| `tv08_noise.csv` | derived | Lattice noise L, interpolated n(x) and 6-octave fbm(x). |
| `tv09_projection.csv` | derived | Projection scales and effective angles for FAR and BUMPER at three viewport sizes, with the main-player-view scaling on and off. |
| `tv11_projection_policy.csv` | derived, project policy | SPEC §7.1: original vs wide (Hor+) scales and effective angles for FAR and BUMPER at 640x480, 1920x1080, 2560x1080, main-view scaling on and off. 640x480 rows equal `tv09_projection.csv`. Computed from SPEC §7 with the integer-angle truncation rule. |
| `tv10_view_cycle.csv` | derived from SPEC §2.3 | Option transitions for the in-race cycle (with and without drift) and for the options screen. |

Provenance: derived vectors come from the analyst's reference model of the SPEC maths (not from the original game at runtime); data vectors from the user's own retail files (values only, no asset content).

## Full per-car table (optional, not published)

A retail comparison may be given the full per-car table (same columns as tv01) as an optional
local-only input. When it is absent, the comparison checks structure only: every car record
yields the declared number of entries per set, view tags are in 0..5, and all values are finite;
it must report that the value comparison was skipped.
