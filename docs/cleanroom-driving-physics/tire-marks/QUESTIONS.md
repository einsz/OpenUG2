# Tire marks — open questions

Each question corresponds to an Unknown in SPEC.md. Answers must be added by amending
SPEC.md through review, not by implementer contact.

- **TM-Q1 (TM-U1)** Which systems (tyre smoke, skid audio, camera shake, force feedback)
  consume the published per-wheel utilisation / its mean, and with what thresholds? Do any
  reuse the TM-3 lay predicate?
- **TM-Q2 (TM-U2)** In ordinary single-player free-roam and races, is the vehicle-level gate
  G always true for the player car and for AI/traffic cars?
- **TM-Q3 (TM-U3)** What sets the per-wheel suppression mask in retail play (blown tyre,
  drag-race event, scripted state)? Until answered, hosts supply all-clear.
- **TM-Q4 (TM-U4)** Is vertex colour/alpha 128 unity (×2 modulate) in the original renderer?
  This decides whether a full-intensity mark shows the texture at ~50 % or ~100 % opacity.
  A controlled in-game screenshot comparison on plain road would settle it.
  **Answered by SPEC.md TM-15 (2026-10-16, static [obs]/[inf]):** colour is doubled (RGB 128 ≈
  unity, ×256/255) but alpha is not (vertex alpha 127 → 0.498). Net opacity of a full road mark on
  an opaque texel is 0.248 per layer (texel alpha 127/255 × 127/255). Residual (still open): draw
  order of the mark block versus cars/smoke/other decals; a screenshot comparison would still be a
  useful runtime confirmation.
- **TM-Q5 (TM-U5)** Do vehicle reset, race restart or replay start end open trails or clear
  stored marks?
- **TM-Q6 (TM-U6)** Confirm the "no practical distance fade" reading (TM-11.3), e.g. marks
  remain at full strength until abruptly culled at the cell distance limit.
- **TM-Q7 (TM-U7/U8)** Ground probe behaviour with no ground found / stacked geometry, and
  any producer of the per-wheel ground offset.
- **TM-Q8 (TM-U9)** Do visual/performance tyre upgrades change the record tyre width?

## Implementer questions (raised during implementation; SPEC.md sha256 99b70795...)

These are places where SPEC.md is silent or ambiguous. Each is implemented as a clearly
named host choice (see IMPL_DESIGN.md); none is claimed to be retail behaviour.

- **TM-IQ1 (TM-7 step 5 / TM-6)** The continuation seeds the new strip with "the old strip's
  last stored point decoded (P, Aq/64, Ib/255)". Re-encoding the decoded intensity,
  `trunc(255 * Ib * b32(1/255))`, can lose one count (e.g. 76 -> 75) in binary32. Is the seed
  a byte-for-byte copy of the stored point, or a decode/re-encode round trip? Implemented: byte copy
  (the vectors' 255 and 0 seeds cannot distinguish the two).
- **TM-IQ2 (TM-U7)** With no ground at either probe nothing can be laid (no height). Implemented:
  one missing probe reuses the other probe's height; both missing = no sample, and the trail is
  neither ended nor extended. Is that the retail outcome?
- **TM-IQ3 (TM-U4)** Alpha unity. **RESOLVED by SPEC.md TM-15 (sha256 1b864fc1...).** The earlier host
  choice `alpha_scale = 2` is removed. Vertex alpha is the TM-11.2 byte as is (127 -> 127/255); the pixel law is
  `out.rgb = clamp(2 * T.rgb * C.rgb)`, `out.a = T.a * C.a` (colour doubled, alpha not). Implemented in the
  mark shader path (`uVista > 1.5` in `src/shaders/ug2.frag`) and checked by `make tire-marks-test` /
  `make tire-marks-render-test` against `test-vectors/render_state.json`. See IMPL_DESIGN.md "TM-15".
- **TM-IQ4 (TM-10.1/10.2)** Order of eviction when a creation overflows both the strip store and the
  cell store, whether a cell is destroyed once its last strip is evicted, and whether a cell's box
  shrinks when strips leave. Implemented: strip eviction first, then cell lookup/creation; cells
  persist when empty (until evicted by the cell store); cell boxes only grow.
- **TM-IQ5 (TM-11.4)** The "world frame counter" for the cell-touch rule: the host uses a counter
  advanced once per 60 Hz simulation tick of the player sampler. The frustum test is a bounding-sphere
  test against the camera MVP (TM-U12); the strip box test uses the box half-diagonal as the radius.
- **TM-IQ6 (TM-11.3/TM-U6)** `K = 256 / tan(phi/2)` is taken from the live projection ([1][1] term =
  1/tan(vertical fov / 2)); whether the original's phi is a vertical or horizontal angle, and its
  units, are not stated.
- **TM-IQ7 (TM-1.2 / Stage5 surface ids)** The host ground query (`stage5_world_query`) assigns the
  numeric road fixture id 1 to every surface (U5-2, material-to-id mapping unreviewed), so in play only
  `SKID_ROAD` marks occur; dirt, grass and puddle behaviour is exercised by tests only.
- **TM-IQ8 (TM-6, TM-U10)** Out-of-range half-width components / intensity are saturated
  (int8 / 0..255) rather than wrapped; unreachable with stock data.
- **TM-IQ9 (TM-2/TM-U2)** Host gate G = contact model ready AND not the alternate-mover mode (aero
  disabled) AND session not suspended. No game-state-code range exists in this host.

## Render-parity questions (analyst, 2026-10-16)

- **TM-Q9 (TM-U4 residual)** Where does the mark block sit relative to car bodies, car glass,
  tyre smoke and other blended decals within a frame? Only "after the earlier world-surface
  render lists, before later flushes" is established (TM-15.7).
- **TM-Q10 (TM-15.3)** Which min/mag filter (point/linear/anisotropic and level) does the
  default user graphics setting select for world surfaces? Marks follow the same setting.
- Verifier item V9 is answered by TM-15 (vertex RGB 128 is colour unity; alpha is not doubled).

## Implementer questions (TM-15 update, SPEC.md sha256 1b864fc1...)

- **TM-IQ10 (TM-15.4)** Fog. The host's world fog is `exp(-(depth * density)^2)` with the frame's fog colour, not the
  linear `(Fx, Fy, Fz)` form the spec describes for the original. The mark pass uses the same host fog as the road
  under it ("marks are fogged exactly like the road"), so parity with the host's own road is kept; retail fog
  parameters (TM-U4 residual) are not reproduced.
- **TM-IQ11 (TM-15.5)** Mip chain. The host builds a full mip chain (down to 1x1) for the mark textures (CPU-decoded
  RGBA, `upload_tex`); the spec records 3 levels (64x32, 32x16, 16x8). The RHI sampler has no max-LOD, so this cannot be
  expressed. Mip means are within 0.3 of level 0 (spec: 114.2/114.3/114.4), so colour/opacity differ only at extreme
  minification (below 16x8 texels on screen).
- **TM-IQ12 (TM-15.5 "pixel bases")** The shared pack reader (`n2_tpk_open` in `src/nfsu2.h`) skips the 0x11 filler
  greedily and so also eats the first three bytes of the road palette entry (0x11,0x11,0x11,0x7f), shifting all three
  mark textures by 3 bytes (host decode gave road alpha max 24, RGB (113,11,11); dirt alpha max 238). The mark loader now
  restores the 0x80-aligned payload start with `tm_payload_base` (same rule as TM-15.5); decoded statistics then equal
  TM-15.5 (alpha max 127/102, mean 114.16/90.22, road RGB 8.31). The shared reader was NOT changed: any other texture
  whose palette/data starts with 0x11 bytes has the same latent offset error. Should the shared reader be fixed?
