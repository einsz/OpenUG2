# Settings spec — questions

Implementer/reviewer questions go here. The analyst answers by amending SPEC.md.

## Analyst open questions (2026-10-14)

These need a runtime session with the retail game (see SPEC §12); static examination could not
settle them.

1. EA TRAX playlist bytes in a profile that never opened the EA TRAX screen (SPEC §5.6, §11.1).
2. Defaults of the online flags and the meaning of the two five-word online arrays (§5.5).
3. Meaning of the unidentified audio, gameplay and per-player fields (§5.2–§5.4) and of the
   16-byte unknown block (§5.9).
4. Input binding table: action-slot order and default codes (§6).
5. Career difficulty default and whether the option is reachable in retail menus (§5.7).
6. Language registry string that matches nothing (index −1) (§2.4).
7. Case sensitivity of the `/s` safe-mode switch (§4.6).
8. Which save points trigger the autosave/prompt check (§5.8).

## Implementer questions (2026-10-14, settings backend)

Each item was implemented with the stated interim choice; none blocks the vectors.

1. **Defaults of unknown fields (SPEC 5.5, 5.6, 5.8, 11.1/11.2).** `st_profile_defaults` uses
   exactly the synthetic-vector placeholders (EA TRAX identity order with mode 3, autosave 1 /
   prompt 0, Show Player Names 1, Bad Connection Warning 1) so that defaults + name reproduce
   tv03 byte for byte. These are guesses; real defaults need the capture in SPEC 12.1.
2. **EA TRAX masks (SPEC 5.6).** "Two 27-bit masks derived from the modes": is bit *n* indexed by
   track id (implemented) or by playlist position?
3. **Auto-detect and `g_PerformanceLevel` (SPEC 4.2).** The text does not say whether auto-detect
   stores P into `g_PerformanceLevel`. Implemented: yes (otherwise the slider would show a stale level).
4. **Preset columns outside 0..5 (SPEC 4.2).** Implemented: level <= 1 uses the "P <= 1" column,
   level >= 5 uses the "P = 5" column. Registry values can hold anything; what does the original do?
5. **Safe-mode switch case (SPEC 4.6, analyst Q7).** Implemented case-insensitive (`/s` and `/S`).
6. **Container with trailing slack (SPEC 3.5 step 4).** If header `total` equals the file length but
   sizeA + sizeB < total - 28, step 4 reads succeed. Implemented as accepted (extra bytes ignored,
   checksums cover only the two blocks); sizeA + sizeB > total - 28 is an I/O error. Confirm.
7. **Block B shorter/longer than 54 930 (SPEC 3.5 step 6).** The game-level check only mentions the
   size field inside block A. Implemented: actual block B size != 54 930 is also "incompatible".
8. **Autosave/prompt byte values (SPEC 5.8).** Only 0 and 1 are described; implemented as
   "non-zero = set" for display, writes are always 0/1.
9. **Normalisation on write.** Online offsets 0 and 4 are always written as 0 ("always written 0,
   ignored on read"), and the profile-name field is NUL-padded; bytes after the NUL terminator of a
   loaded name are not preserved. All other bytes (including padding) are preserved. Confirm that no
   retail save relies on non-zero bytes there.
