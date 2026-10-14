# Settings backend test vectors

All vectors are **synthetic**. None is a copy of, or derived from, a user save or any retail
asset. They were produced by an generator written for this project that encodes the statements in
`../SPEC.md` (it is not code from the original program). Where the SPEC lists a value as
unknown, the vector uses a labelled placeholder (see below); such fields must not be used to
judge retail equivalence.

| File | Tests | SPEC | Provenance / confidence |
|---|---|---|---|
| `tv01_checksum.json` | checksum of byte strings (incl. < 4 bytes → 0) | §3.4 | algorithm [static]; values computed from the SPEC formula; the closed-form identity was checked on random inputs of length 4…199 |
| `tv02_container_small.bin` | container framing with tiny blocks (not a game profile) | §3.2 | synthetic; header fields listed in `tv02_tv03_containers.json` |
| `tv03_synthetic_profile_defaults.bin` | full 54 966-byte profile with default settings | §3, §5 | synthetic; career region (payload 0…50 210) is all zero and is **not** a valid career; settings section starts at file offset 50 247 |
| `tv03b_synthetic_profile_bad_payload_crc.bin` | same file with one bit flipped in Master Volume | §3.5 | must be rejected as corrupt (block B checksum) |
| `tv02_tv03_containers.json` | expected header values and parse outcomes for the binaries | §3 | computed |
| `tv04_video_presets.json` | preset + clamp for performance levels 0…5, GPU score 3 and 5 | §4.2, §4.3 | preset table [static]; `FULL_CAPS` (incl. FSAA cap 4) is an assumed modern adapter [inferred] |
| `tv05_autodetect.json` | performance level from CPU MHz and GPU score | §4.5 | formula and thresholds [static]; vendor overrides not covered |
| `tv06_profile_settings_layout.json` | machine-readable field table (payload and file offsets, types, defaults, domains, confidence) and opaque regions | §5 | mirrors the SPEC tables |

Placeholders in `tv03*` (unknown in the SPEC, §11):

* EA TRAX playlist: identity order (track i in slot i), every mode 3. Guess only.
* Autosave byte 1, prompt byte 0. Guess only.
* Online flags: Show Player Names 1, Bad Connection Warning 1, other online bytes 0. Guess only.
* Career difficulty 0 [inferred], unknown block `{0.5, 0, 0, 0}` [static], all opaque regions
  zero, input-device block all zero (no device records).

Parsing expectations for `tv03_synthetic_profile_defaults.bin`: header valid, block A =
`{0x10D, 54930}`, profile name `TESTPROFILE`, units MPH (language index 0), all §5 defaults
as tabled. Floats compare exactly (they are stored values, not computed). A writer given the
parsed state plus the untouched opaque bytes must reproduce the file byte-for-byte.
