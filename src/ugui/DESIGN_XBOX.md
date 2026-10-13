# Xbox controller source design

Implementation input: approved `cleanroom/spec/SPEC.md`
SHA-256 `76125569604ff34be9a0b72f981d200b2ebdb3326af3228205c787d8e507f773`,
and approved `XBOX_CONTROLLER.md`
SHA-256 `551417742c4e54876e463accaec9907cf18dff7041bd0181087dff0f7f74b776`.
The Xbox vector files and their README hashes were verified against REVIEW.md.
Only the approved specification and explicitly permitted independent ugui code
were read. No original resource, binary, analysis, or verifier material was read.

- **X1:** A reader abstraction serves runtime files and synthetic memory fixtures.
  XDVDFS lookup validates the full visited entry tree in each directory before
  accepting a name. Per-table visited offsets reject repeated/cyclic child nodes;
  a separate directory-sector history rejects cyclic directory descent. Child
  addresses use four-byte units. Sector extents use 64-bit arithmetic and stay
  inside the source. The supplied partition-base-zero descriptor is read at
  sector 32. Runtime loading resolves only `/NFSUNDER/ZZDATA5.BIN` and scans that
  member for bounded uncompressed packs, rather than depending on a retail byte
  offset. Candidate reads must fit inside the recorded member extent.
- **X1/X2:** Defensive resource limits match the baseline PS2 loader: directories
  and candidate packs are at most 16 MiB; directory descent is at most 64 levels;
  chunk nesting uses the existing eight-level limit. These limits exceed the
  approved observed source and bound allocations on malformed input. Other
  partition layouts and compressed sources remain unsupported.
- **X2/X3:** The independent existing chunk walker is parameterized by opaque
  descriptor stride (84 PS2, 60 Xbox). Xbox requires version-5 pack metadata,
  matching record/key/descriptor counts, and an aligned pixel base computed from
  the candidate pack start. Table corruption rejects the pack. Known-key records
  with unsupported dimensions/formats/mips, bad image bounds, wrong lengths, or
  a palette are unavailable individually, allowing an honest validated subset.
  Successfully decoded RGBA is owned by the shared texture library; freeing the
  scanned pack therefore cannot invalidate texture memory.
- **X3:** The existing independently written SPEC 15.7 BC1/BC2 primitive is reused
  through a narrow wrapper. The wrapper permits dimensions 1..32 for synthetic
  codec vectors; actual source records must be 32x32 and have exactly their
  specified base-level lengths and one mip level. No palette/swizzle/tint changes
  are applied. The approved BC1 key is required to use BC1; the other base keys
  require BC2.
- **X4:** The gallery chooses the approved 23 base keys. The remaining localized
  keys are unspecified individually, so no aliases or language filter are
  invented. Names are descriptive category captions; record fragments are never
  used to recover a key. Existing PS2 catalog APIs remain unchanged. New family
  APIs supply catalog count/key/name, validated texture handle, and availability.
  Handles are 1-based, with zero representing an unavailable texture.
- **X5:** Separate Xbox availability/storage preserves PS2 compatibility and PC
  default selection. Bindings carry family and are configured explicitly; token
  queries cannot use another family's texture. Legacy callers may configure PS2
  bindings while PC is selected. Demo composition and CLI integration are owned
  by the parent implementer; this module supplies truthful resource status.

Tests were authored before implementation: the isolated translation unit failed
linking because the new Xbox APIs did not exist. The seven Xbox cases and the
XDVDFS root/invalid-child/invalid-name cases are encoded from reviewed JSON.
Additional fixtures check subset availability, exact Xbox member selection,
malformed formats/dimensions/extents, family isolation, descriptor truncation,
cyclic entries, and source bounds. Tests generate artificial BC blocks and disc
structures only. No retail-resource tests were performed by this implementer.

Validation: isolated synthetic suite: 7,194 checks, zero failures. The same
suite passed AddressSanitizer and UndefinedBehaviorSanitizer with leak detection
disabled (existing baseline ownership is outside this change). Controller module
compiled with `-Wall -Wextra -Werror`; assigned-file diff whitespace checks passed.
The parent subsequently reported the integrated suite at 7,269 checks, zero
failures, and a successful demo build. No questions were added.
