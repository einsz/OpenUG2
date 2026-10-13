# Camera spec — questions

Implementer/reviewer questions go here. The analyst answers by amending SPEC.md.


## Implementer questions (2026-10-13)

1. **SPEC 4 vs `tv02_view_tuning_effective.csv`: look-at z clamp.** SPEC 4 lists
   `lookMin.z = -0.75`, `lookMax.z = +0.75` for FAR/CLOSE/DRIFT. tv02 has -1 and +1 for
   the same views. `lookK.z` is 0, so the value never matters in 6.8, but which is
   authoritative? Implemented as the vector (+-1), constant `CAM_LOOK_Z_LIMIT`.
2. **SPEC 7 projection rounding.** The formula `xScale = 1/tan(s0*F/2)` evaluated exactly
   does not reproduce `tv09_projection.csv` (e.g. BUMPER, main view, x scale 0.854081 vs
   0.854114; FAR y scale 1.486600 vs 1.486711). The vector is reproduced exactly when the
   half angle `s*F/2` is truncated to whole BAU before the tangent. Is the truncation real
   (integer-angle trig in the original), or is the vector model's rounding incidental?
   Implemented with truncation to match the vectors (`cam_projection_scales`).
3. **Road surface ids for shake.** SPEC 8 wants a surface id (0-28) per wheel. The engine's
   ground query only reports ROAD / TERRAIN / NONE. Implemented ROAD -> ROAD, TERRAIN -> GRASS,
   no contact -> NONE. Is there a data source for per-wheel surface ids in the track files
   this spec should name?
4. **SPEC 6.1.4 probe.** Which geometry does the 2 m "up" probe test (all world triangles,
   or only collision geometry), and does it start exactly at the car origin? Starting at
   the origin hits the road surface in this engine, so the probe starts 0.3 m up with a
   0.05 m radius (`GAMECAM_PROBE_LIFT`, `GAMECAM_PROBE_RADIUS`).
5. **SPEC 6.1.2 other-car box.** What are the dimensions of "another active car's collision
   box" (height especially)? Implemented from the AI car's half length/width and height.
6. **SPEC 2.3 event set-up for free roam.** Starting free roam is not an "event". Left
   unchanged (stored option kept, including Super Far). Confirm.
7. **Persistence medium.** The original stores the option in the save profile; this engine
   has none, so a small `camera.cfg` is used (docs/CAMERA.md D3). No spec question, noted
   for the analyst's awareness.


## Reviewer answers (2026-10-13, amendments 1)
1. ANSWERED: look z clamp is -1 / +1; tv02 authoritative (SPEC 4 corrected).
2. ANSWERED: truncation is real; half angle truncated to whole BAU (SPEC 7 updated).
3. OPEN: SPEC 8 / 13 item 15; mapping is an implementation choice.
4. OPEN: SPEC 13 item 16; probe lift is an implementation deviation, not a requirement.
5. OPEN: SPEC 13 item 17.
6. OPEN: SPEC 13 item 18; unchanged-option is consistent but unconfirmed.
7. CLOSED: implementation choice, no spec change.
