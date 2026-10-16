# Temporary opening 350Z configuration

2026-10-15. Reviewer-gated provisional contract. This document specifies a reversible **best-evidence provisional profile**, not proven parity with the original opening driving instance. Evidence identifiers resolve in PROVENANCE.md; private evidence is not an implementation input.

## Findings and confidence

| Claim | Classification | Evidence |
|---|---|---|
| The inspected executable has the coordinator-supplied SHA-256 identity. | Observed file bytes | C350-1 |
| A single opening-preset candidate in the inspected uncompressed asset names the Nissan 350Z and stores aggregate performance tier **2**. | Observed file bytes; opening identity corroborated by public original research | C350-2 |
| A career setup branch requests that preset; an alternate branch obtains a profile-dependent vehicle instead. The branch condition's complete lifecycle is unresolved. | Inferred from original static instructions | C350-3 |
| Applying a nonnegative aggregate tier clears package selections, then enables available package levels from 1 through the tier across categories 0–9. | Inferred from original static instructions | C350-4 |
| Both inspected package-table copies agree. Tier 2 yields 34 enabled component flags after requirements and exclusions, with no rejected requirements. | Observed data agreement; inferred composition | C350-5 |
| The preset's 68 explicit component bytes are all zero, but those bytes are **not the active selection source when aggregate tier is 2**. | Observed bytes; inferred branch semantics | C350-2, C350-4 |

The evidence supports aggregate tier 2 for this preset. It does not support substituting all-level-3 upgrades. It also does not establish that every supported category corresponds to one familiar user-interface upgrade label.

## Required temporary profile behavior

1. Give this profile an explicit provisional identity distinct from an ordinary stock 350Z. It must be opt-in and reversible; changing it must not rewrite original game assets or user saves.
2. Select base vehicle 350Z and aggregate performance tier 2. Obtain component selection from the user's local package records using the composition rules below. Do not replace that selection with a blanket all-enabled mask, the zero explicit mask, or an unverified level-3 selection.
3. Preserve lower-level components unless a later package explicitly excludes them. Package tier 2 is not represented solely by components appearing in level-2 package rows.
4. Record the asset identities used for each run. The inspected uncompressed asset is a candidate source; its runtime load precedence relative to compressed files is unknown.
5. Treat manual tuning, special bonuses, event handling family and profile overrides as unresolved. For a temporary test, unchanged/default tuning, no additional instance bonuses, and the ordinary road-driving handling family are explicit provisional assumptions. They are not original-game findings. Report those assumptions with the run; do not silently identify an arbitrary numeric mode as the original opening mode.
6. Do not tune mass, grip, damping, steering or engine constants to compensate for unresolved configuration. Final effective physics values are outside this specification.

## Local asset interpretation

All integer fields below are little-endian. Chunk headers contain a 32-bit identifier followed by a 32-bit payload length. A container's high identifier bit denotes nested chunks. Validate that every payload fits within its containing range; reject truncated records.

The preset chunk identifier is `0x30220`; each payload record is 824 bytes. At record byte 8 is a 32-byte zero-terminated base-model field; byte 40 contains a 32-byte zero-terminated preset key. Byte 72 contains a signed 32-bit aggregate tier. Bytes 76–755 contain appearance/part references outside this task. Bytes 756–823 are 68 signed 8-bit component selectors. For exact local selection, the preset key is `DDAY_PLAYER_CAR`. This exact short asset identifier is functionally required to distinguish the candidate and is explicitly permitted interoperability text, publicly documented in the preset research cited in PROVENANCE.md. It is not a product-facing label.

The package chunk identifier is `0x34a15`; records are 380 bytes. Record bytes 0, 4 and 8 contain category, level and component count as 32-bit integers. At byte 12 begin at most four component descriptors of 92 bytes each. Within a component descriptor: byte 0 begins a signed 32-bit component index; bytes 64–83 hold five signed 32-bit excluded indices; bytes 84–91 hold two signed 32-bit required indices. A negative index denotes no entry. Other descriptor fields are outside this task. Categories used here range from 0 through 9; component indices range from 0 through 67.

For the preset path covered here, aggregate tier `-1` selects the explicit-component branch; tier 2 selects package composition instead. General explicit-branch behavior is outside scope.

## Host loader, switching and trace contract

These are new-host requirements for a reversible experiment, not inferred original-game error handling or user-interface behavior.

- Provide an explicit profile choice between the existing ordinary stock setup and `temporary-opening-350z-provisional`. Keep the existing normal default. Entering the provisional profile constructs an isolated configuration from the selected local base vehicle and selected component flags; returning to stock reconstructs the ordinary stock setup without retaining provisional flags, tuning or assumptions. Switching must use a fresh/reset driving instance so old simulation state does not contaminate a comparison. Never overwrite shared stock records, normal user preferences or saves.
- Read user-selected local assets directly. This is host asset parsing, not execution of the original program. The supported candidate source is an uncompressed chunk stream. Unsupported compression must produce an unavailable-profile result with an actionable explanation, rather than attempting to parse compressed bytes or implying original loader precedence.
- Locate exactly one matching preset key in the selected source, verify base model `350Z` and tier 2, and derive selections from its local package records. Absence, duplicate matching presets, mismatching model/tier or unavailable package data prevents activation. Duplicate package-table copies are acceptable only when their relevant parsed category/level/descriptors agree; disagreement prevents activation. Within a table, reject duplicate category/level records for this temporary loader rather than guessing precedence. Missing individual category/level rows remain valid and are skipped by composition.
- Reject incomplete headers, payload lengths extending outside their parent, incomplete fixed-size records, descriptor counts greater than four, nonnegative descriptor/requirement/exclusion indices outside 0–67, unsupported category/level values, and unterminated required text fields. Supported package categories are 0–9 and levels 1–3. Negative requirement/exclusion indices denote no entry; a negative descriptor component index is ignored. Enforce bounded parsing and nesting. A failed activation leaves the previous configuration intact and visibly reports failure; it never silently activates stock under the provisional label.
- Emit run/trace metadata before driving: chosen profile identity, parity status `unproven`, local source SHA-256 identities and selected source identifier, selected base/tier, selected component indices and count, configuration source `local_asset_package_composition`, assumed tuning/bonuses/handling family, actual host handling mode identifier, transmission and assistance settings. Unknown original values remain explicitly unknown; actual host values are reported separately. Include the SPEC.md hash. Local paths may remain local; do not redistribute source asset records or descriptor tables.
- For the inspected uncompressed asset SHA-256 in `test-vectors/profile.json`, require a selected population of 34; a mismatch blocks activation as an extraction/composition error. Other source hashes may be used only with an explicit visible warning that the inspected-data check does not apply, with their derived selection/count retained in metadata. A count match alone never proves exact opening parity.
- If the host cannot yet apply the selected components to effective physics, expose the parsed configuration/trace as diagnostic-only and mark driving activation unavailable. Do not present ordinary stock physics as a successfully applied upgraded profile. Effective physics extraction and formulas require their own approved specification. Existing Stage1–5 stock pass-through contracts explicitly defer upgrade composition; this gate independently approves profile parsing, reversible switching requirements and trace metadata only until that derivation is approved.

Acceptance checks: the ordinary default remains unchanged; opt-in produces this profile and complete metadata; switching back removes provisional state; malformed/ambiguous/unsupported inputs fail without changing the active setup; diagnostic-only extraction cannot be mislabeled as driving activation. These checks use synthetic chunk fixtures built from the minimal schema and invented descriptors, with provenance as new-host test data. They assert this contract, not original runtime behavior.

## Package composition behavior

Start with no selected component flags. Process levels in ascending order from 1 through the requested tier. Within each level process categories 0–9; skip missing package records. Process the recorded descriptors in their stored order. An already-selected component remains selected without rechecking requirements or applying its exclusions again. To enable a currently unselected component, require each nonnegative required index to be selected. If a required component is absent, leave the component disabled. Otherwise disable each listed nonnegative exclusion and enable the component. This invocation does not automatically enable missing prerequisites. Do not reinterpret this as a physics formula or multiply physical attributes by the tier.

Negative descriptor component indices are ignored. The host validation rules above govern malformed data; no original malformed-input behavior is claimed.

For the inspected asset and tier 2, all requested descriptors pass their prerequisites; the final selected population is 34. This population count is a useful extraction check, not a substitute for retaining the actual locally derived selection.

## Synthetic test vectors

`test-vectors/composition.json` contains invented package data exercising prerequisite retention, exclusions, missing requirements, missing package rows and ascending-level application. These vectors test the behavior inferred from static instructions; they are not captured original-runtime outputs. No proprietary descriptor table or preset record is redistributed.

## Unknowns/Ambiguities

- Whether the inspected uncompressed asset or another candidate is actually loaded in this installation; pristine-release provenance is unknown.
- Whether the studied career setup branch is used for the user's precise first controllable opening frame, and the lifecycle/meaning of its profile-dependent alternate branch.
- Manual tuning values at that frame, instance additions, selected transmission and assistance settings, and the active road/event physics family.
- The semantic UI names of all ten package categories; no guessed name mapping is required.
- Final effective mass/inertia, engine/transmission/tire/suspension/aerodynamic values and their composition with mode settings.
- Runtime comparison against the same-build ordinary stock 350Z. No original runtime was executed on the host or elsewhere for this analysis.

Only SPEC.md and the auxiliary files expressly hashed in REVIEW.md are implementation inputs. PROVENANCE.md provides reviewer/coordinator traceability; its research pages may contain original code and are not implementation inputs. Private evidence and analyst scripts are never implementation inputs.

These unknowns prevent any claim that this provisional profile reproduces the exact opening configuration or driving behavior.
