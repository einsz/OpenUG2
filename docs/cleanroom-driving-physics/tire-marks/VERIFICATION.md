# Tire marks - verification (verifier, 2026-10-16)

SPEC.md sha256 verified = 99b7079559ee599a861ac3baa0a107e2623be9097d1494c06f3605af38b242b5 (matches approval).
Implementation tree: uncommitted, branch tire-marks. No implementation code was edited.

## 1. Test results (verbatim summaries)
- make tire-marks-test: `tire_marks_test: 627 checks, 0 failures`
- make tire-marks-drive-test: `tire-mark drive adapter: gate, lay predicate on locked wheels, utilisation range passed` (cruise strips 0; handbrake slide strips 3, sliding wheels 0011)
- make drivetrain-test: `PASS: 3606 checks, 0 failed` (no regression from the added utilisation output)
- make drive-stage5-test: all three Stage5 lines passed
- make (game binary): `'nfsu2' is up to date` (it builds; only pre-existing -Wmisleading-indentation warnings seen in physics.c/nfsu2.h)
All PASS. Caveat: vectors are synthetic consequences of the spec (README, REVIEW.md), so passing proves spec self-consistency only.

## 2. Differential testing: not possible / not done
- No runtime differential against speed2.exe was attempted (original not run; spec itself states it was never executed). No recorded reference outputs exist.
- GL draw path (texture load from InGameCommon.bun, quad draw, uVista blend) compiled/linked but never run (IMPL_DESIGN "Not verified"); I also did not run it. Visual parity, alpha unity (TM-U4), draw order, frustum behaviour: unverified.
- Nothing about retail thresholds (8 kN, 1 m/s, capacities 128/64/40, 300 frames, 4 m) is verifiable here.

## 3. Section-by-section review (static read of src/tire_marks.c/.h, tire_marks_drive.h, hooks)
- TM-0/1.1/1.2/1.3: surface table, k_s (b32(1.4) for 8/9/15), texture map, hashes match spec. Ids >28 or <0 treated as id 0 (spec silent). tm_car_read reads +0x134+0x30*i; rejects non-finite/<=0 widths (spec silent; keeps default .22). OK.
- TM-2: sampler order 0..3 and gate semantics (G false: no end, no add) match. DIVERGENCE/extension: G in tm_drive_gate additionally requires contact_ready && prepared && session != SUSPENDED, and main.c additionally requires !sstatic && !capture_policy.freeze_motion; when stage1 not active the trails are ended (tm_vehicle_destroy). Spec's G = effects enabled + not alternate mover + game-state code. Behaviourally defensible but beyond spec (see TM-IQ9, amendment V3).
- TM-3: tm_lays matches (contact, mask, strict horizontal speed in binary32, sliding or u*N>8 strict). Utilisation: tm_utilisation does the arithmetic in double (spec says binary32 thresholds; only matters at ties) and the double ratio is rounded to float after; M uses `mag` incl. 1.5 factor per implementer note. Locked wheel -> u=1 relies on `e->locked`. OK. GAP: spec TM-12/TM-2 says published u is forced to 0 in the special mover mode; implementation does not force it (no consumer exists, so unobservable).
- TM-4: all 7 steps present. zR kept in double, zL b32 as in spec. Degenerate Am (n=0) falls back to (w,0,0); spec silent. OK.
- TM-5: matches. TM-6: truncation toward zero; saturation instead of wrap (TM-U10, accepted, unreachable).
- TM-7: matches steps 1-5. Seed handling is a byte copy; see IQ1 below. strip_create can return NULL (unreachable). After step 5 the seed point is appended via strip_append with n=0 so S=0, then the sample is n=1; matches.
- TM-8: matches (kappa break first, replace test, full, store). Box: initial box = point 0, growth by cube d, union into cell box. OK.
- TM-9: end on predicate false, ended, continuation, wheel destroy (tm_vehicle_destroy). OK. No reset/teleport end (TM-U5): note main.c car switch does not call tm_vehicle_destroy; a teleport therefore bridges via the 4 m origin gate / kappa / L rules only. Consistent with spec ("no other end trigger").
- TM-10.1: eviction order strip-before-cell, recency stamp, 128 cap fixed in main (64 for two local players not implemented; no such mode in host). OK within spec; the 64-cap is only reachable through tm_init.
- TM-10.2: cell key int64 floor; cell store 40; LRU-touched destroy with strips; touch on create and on >300 frame refresh. Frame counter g_tm_frame increments once per presentation loop pass in main.c (location after physics near g_skid), NOT demonstrably per 60 Hz tick: if the host runs at another frame rate the 300-frame window changes length. See V4.
- TM-11.1/11.2: quad corners, UVs, +b32(.05) lift, RGB 128, alpha max(Ib-F,0)>>1 correct. The host multiplies alpha by alpha_scale=2 (TM-U4 choice, TM-IQ3) and ignores vertex RGB (texture colour x vertex alpha). Spec-silent choice; not retail-verifiable.
- TM-11.3: formula implemented as stated; K from 256*P[5] (IQ6). TM-11.4: cell/strip tests present; view mask/f fixed 0 (single view). Strip recency promoted only for strips that pass frustum and p>4 (spec: "passes all per-view tests"); consistent.
- TM-12: u and mean published (DrvWheel.utilisation, utilisation_mean). No consumers; OK.
- TM-13: covered by non-vector tests per IMPL_DESIGN.
- Coverage gap in play: host ground query assigns surface id 1 to every surface (IQ7), so dirt/grass textures, k_s=1.4 and the puddle rule never occur in the game; only unit tests exercise them.
- Coverage gap: tm_world_ground uses the Stage5 ride probe (world_wheel_support, reach down/up limits), not literally "the same query at probe (x,y)"; probe z is wheel-centre minus radius, i.e. spec TM-4 step 5 says the probe z is the wheel-centre height. The query reach window is therefore a host choice affecting TM-U7 (no ground -> missing probe).

## 4. Implementer questions TM-IQ1..9 (verifier opinion; answers belong to the spec via amendment)
- IQ1: I tested the stated premise. Over all 256 intensity bytes, re-encoding trunc(255 * (Ib * b32(1/255))) returns Ib exactly, both in true binary32 (numpy float32) and with double intermediates. The "76 -> 75" example is NOT reproduced; byte copy and decode/re-encode are therefore observationally identical for stock arithmetic. Position/aq round trips are exact (aq/64 then trunc(64*)). Question is moot unless the original uses a different precision (TM-U11); recommend closing as no-difference.
- IQ2: reasonable host choice; spec TM-U7 unresolved. Keep as host choice; note it also means no-ground frames neither end nor extend a trail (could draw a long bridge segment later, bounded by TM-8).
- IQ3: unresolved by evidence; needs TM-Q4 screenshot. Keep as parameter.
- IQ4: spec-silent; implementation picks reasonable order. Observable only under heavy overflow. Accept as host choice.
- IQ5: spec-wise "world frame counter" cannot be verified; recommend the host counter be tied to the fixed 60 Hz simulation tick (see V4).
- IQ6: K definition plausible; units unknown (TM-U6). Observable effect negligible given "no practical fade", but it changes the cell hard-visibility distance (TM-11.4), which is observable as marks vanishing at distance. Treat as unverified.
- IQ7: confirmed by reading main.c (`.surface=1`). This makes TM-1.2 non-functional in play; requires the surface-id mapping work (not a tire-marks defect).
- IQ8: accept.
- IQ9: confirmed (see section 3); accept with spec amendment V3.

## 5. What could not be verified
Retail parity of every [inf]/[gue] item; the draw path at runtime; alpha unity; cell visibility distance; frame-rate dependence of the 300-frame window; behaviour with surface ids other than 1 in actual play; two-player capacity 64.
