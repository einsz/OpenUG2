# Settings backend — implementation design log

Implements `SPEC.md` (sha256 `eac68834...a185d422`, approved in `REVIEW.md`). Files:
`src/settings.h`, `src/settings.c`, `tools/settings_test.c`, Makefile target `settings-test`
(`make settings-test`, optional `SETTINGS_SPEC=dir`). Plain C99, no engine/GUI dependencies.
Questions raised are in `QUESTIONS.md` ("Implementer questions"); nothing was derived from outside
the spec and its vectors.

## Decisions

| # | Decision | Spec |
|---|---|---|
| 1 | Checksum implemented literally (table-driven MSB-first, first four bytes inverted). The test cross-checks the closed form `CRC32_BZIP2(D[0..n-5]) xor BE32(last4)` with an independent bitwise CRC for n = 4..199. | 3.4 |
| 2 | Container is a pure in-memory API (`st_container_parse/build`) returning the status classes of 3.5 (`NOTFOUND`, `IO`, `CORRUPT`, `INCOMPATIBLE`). Check order follows 3.5: length < 28 -> IO; total/magic/header checksum -> CORRUPT; sizes past end -> IO; block checksums -> CORRUPT. File loading is a thin wrapper. | 3.2, 3.5 |
| 3 | `StProfile` holds typed fields **plus the raw 54 930-byte payload it was unpacked from**. Packing copies `raw` and overlays typed fields, so career data, the six opaque regions, padding bytes and trailing unknown bytes round-trip byte for byte without being modelled. The struct is ~60 KB (allocate statically/heap). | 5.1, 3.6, 9 |
| 4 | Unidentified fields that have a known offset/size (e.g. `audio.unk28`) are exposed as `unkNN` typed fields, so retail values are preserved even if the caller edits the struct. | 5.2-5.5 |
| 5 | Defaults of fields the spec marks unknown use the vector placeholders (see QUESTIONS 1) so `defaults + name == tv03`. | 5, vectors README |
| 6 | Online offsets 0/4 are written as 0; name field zero-padded on write; everything else preserved (QUESTIONS 9). | 5.5, 5.9 |
| 7 | Game-level refusal (version != 0x10D, size field != 54 930) happens in `st_profile_parse`, which also requires the real block B size to be 54 930 and block A >= 8 bytes (QUESTIONS 7). A rejected file is never modified or deleted; there is no recovery or migration. | 3.5 step 6 |
| 8 | Profile files: `<root>/<NAME>/<NAME>`, `/` separators, name 1..15 bytes with no path or Windows-invalid characters, no trailing space/dot. Writing creates root and directory (ignoring "exists"), then create/truncate + write; deliberately **not** atomic, as in 3.3. Delete removes every file, then the directory. "Saves exist" is a directory listing. | 3.1, 3.3 |
| 9 | Dirty detection = re-pack and memcmp against a payload snapshot (all 54 930 bytes). | 3.6 |
| 10 | Registry behind `StRegStore` (vtable: exists/get_dword/get_string/set_dword/set_string/close). Backends: Windows `HKLM\Software\EA Games\Need for Speed Underground 2` opened with `KEY_WOW64_32KEY` (same view as the 32-bit original; writing needs administrator rights, as the spec's HKLM key implies; link with advapi32); an INI file elsewhere (and available everywhere via `st_reg_open_ini`); an in-memory store for tests. Reads: failure (absent, wrong kind, too long) keeps the current value; type of an existing Windows DWORD query is not checked. | 2.1, 2.3, 9 |
| 11 | INI format: one section named after the registry key path, values in `.reg` notation (`Name=dword:0000002a`, `Name="string"` with `\\` and `\"` escapes), case-insensitive names, rewritten whole through a temp file + rename after each write. Default path: `$XDG_CONFIG_HOME` or `~/.config` (macOS `~/Library/Application Support`, Windows `%LOCALAPPDATA%`) + `/OpenUG2/settings.ini`. Profile root: `<local app data>/NFS Underground 2` on Windows, `<data dir>/OpenUG2/NFS Underground 2` elsewhere. Platform choices, not in the spec. | 3.1, 9 |
| 12 | Graphics block = `int32_t v[31]` indexed by the spec's "struct index" (+ slot 30 for `g_PerformanceLevel`). Values are signed so a negative registry DWORD survives. 29 values persisted in 2.3 order (idx 0..27 minus 25, level, vsync); `VERSION`/`SIZE` are always written as 1/120. | 2.3 |
| 13 | Capability clamp lowers only (`ST_NO_CAP` = no limit); resolution steps down to the highest supported lower index, unchanged if none. A resolution index outside 0..5 is left alone (spec 11.10 unknown). | 4.3 |
| 14 | Presets are a data table (rows of five levels, FSAA row uses the cap at P = 5). Auto-detect: resolution 0, preset, clamp, records P (QUESTIONS 3). Level slider: preset + clamp without touching resolution. Level <= 1 / >= 5 map to the first/last column (QUESTIONS 4). | 4.2 |
| 15 | `st_perf_level` uses plain `trunc(0.2*gpu + 0.8*cpu)` in double; all tv05 cases pass without any epsilon. Vendor overrides are not implemented (spec: not specified, irrelevant on modern hardware); safe mode passes P = 0 via `st_gfx_startup`. | 4.5 |
| 16 | `st_gfx_startup` = 4.1 steps 3, 6, 7 (defaults, load, first-run/safe-mode auto-detect, clamp; safe mode saves). It never writes `NotFirstTime`; the caller calls `st_install_mark_started` (the original does that on every start). Display creation and the capability probe (steps 1, 2, 4, 5) belong to the renderer, which supplies `StGfxCaps`. | 4.1 |
| 17 | Language: case-insensitive table; absent -> 0; present but unmatched -> -1 (`StInstall.language_index`). Install strings use `atol` for sizes; `StreamingInstall` is off only for case-insensitive `off`. | 2.2, 2.4 |
| 18 | Option helpers implemented where the rule is fully stated: jump-camera toggle, road-feedback mapping both ways, difficulty wrap, EA TRAX mode cycle and masks, save-option get/set/cycle (with autosave skipping), resolution stepping, audio rate, safe-mode argument. | 4.4, 4.6, 5.x |

## Not implemented (spec gap or out of scope)

* Input-device runtime logic of section 6 (default device tables, calibration rules, binding
  defaults): the records are parsed/written and `st_input_find` matches GUIDs; default binding codes
  are unknown (SPEC 11.4).
* Vendor-specific FSAA/performance overrides (4.2, 4.5), display re-init retry loop (4.4), number
  formatting by language, `.reg` import (9, guessed usefulness).

## Verification

`make settings-test`: 2288 checks, 0 failed (Linux, gcc). Also clean under
`-Wall -Wextra -Wpedantic -Wshadow -fsanitize=address,undefined`. Coverage: tv01 (all cases + closed
form), tv02 (parse, rebuild, every bit flip rejected), tv03 (parse, byte-exact round trip, defaults
reproduce the file), tv03b (corrupt, header and block A fine), tv04 (12 cases x two start states),
tv05 (12 cases), tv06 (all 128 listed fields: vector value = listed default, parsed value, patched-file
read, and write touching only its own bytes; opaque regions preserved), plus registry/INI store,
install/language, graphics startup/safe mode, profile files on disk (save, load, overwrite,
garbage/truncated file rejected and kept, delete). The Windows registry and `_WIN32` directory code
were written to the same interface but **not compiled or run** (no Windows toolchain available).
