# Body-coupling clean-room review

UTC date: 2026-10-15. Role: reviewer. Decision: **APPROVED** for the exact hashes below and the supplied-input domain in SPEC.md.

## Gate and traceability

Read the complete public specification, questions, provenance and vector files. Consulted the private BC-E1…4 register, manifest and captured static evidence to confirm traceability. Checked the approved body-response hash and relevant Stage1/4/5 behavioral boundaries; approved companion files were not changed. No implementation source, design or tests were read or edited. No original host execution was performed.

The automated contamination scan and complete document review found no executable addresses, original recovered identifiers, original source paths, disassembly, decompiler fragments, expressive original strings/assets or wholesale tables in the public candidate. Matches for acceleration notation and the excluded extended-precision target are harmless. No contamination removal was needed and no evidence of implementer exposure to forbidden material was found. Private evidence, capture tools and analyst material are not approved implementation inputs.

Static evidence supports the selected local inverse yaw inertia, prepared-force transpose snapshot before all passes, local mount-arm x/y coordinates, lateral-axis-x signed projection, shared pass-start sums, exclusions of prior angular moment/spin/normal contributions, and subsequent force-update/final-cap ordering. These remain **inferred**, not runtime observations. Full body publication/damping/caps and late angular torque remain separate companion behavior.

## Reviewer changes by category and location

- Approval/scope clarity, SPEC preamble: require this exact approved hash and enumerate the superseded Stage1 heading/contact-arm wording and Stage4 external-moment implication.
- Units/state timing, SPEC BC-2: state dimensional units, identify frame-held mount x/y and steering directions, distinguish frame inputs and preparation snapshot from pass-updated forces, and explicitly exclude a duration factor or extra Euler substeps.
- Vector testability, test-vectors/provisional.json and README: add mixed_forces_candidate_basis, exercising all four nonzero planar forces, both mount coordinates, the selected scalar and transpose of an unrepaired Euler basis. Strengthen external_torque_and_spin_excluded with a nonzero mount arm. Document schema, units and the synthetic boundary-fixture domain.
- Gate wording, QUESTIONS.md: point the resolved answer to the exact approval in this file.
- Contamination: no removed content; public semantic identifiers and equations are behavioral facts written independently.

## Validation and limits

Independently evaluated all expected fields of all **7** unique JSON vectors. All pass exactly for their representable arithmetic and therefore satisfy BC-3 tolerance. The mixed case yields local prepared force (0,-5,6), planar acceleration (2,.5), alpha=-1.25 and wheel accelerations (4.5,-3,.75,1.75). Each vector carries synthetic derivation provenance. These are independently generated mathematical examples of a statically inferred law, not copied assets, original runtime observations, or measured vehicle trajectories. No original runtime or differential parity is claimed. The existing five-pass correction/clutch schedule is inherited; these vectors stop at one pass's acceleration boundary.

Remaining unknowns: BR selector producer and reachability; runtime validation/trajectory parity; exact extended-precision arithmetic; upstream effective parameters, contact/grip producers and excluded rest/guide/collision/alternate-state behavior. Companion unknowns remain unless explicitly superseded. The callback is reduced local planar arithmetic and does not establish a full angular-acceleration/gyroscopic law.

## Approved inputs

Only the following public files in this directory are approved implementation inputs. REVIEW.md is gate metadata. The separately approved body-response SPEC.md (`3a415ca2c95941f3f5c02749e444ec566e48c2e818adacefc0d953eed23a0b8e`) and Stage1–5 contracts remain required. PROVENANCE.md records analyst-time pending status; this review supplies the completed gate. Any later SPEC.md edit invalidates approval; auxiliary edits require re-review.

| File | SHA-256 |
| --- | --- |
| SPEC.md | `b09041f16661995375af617766d672db594c27a5bcbe2e8e5783c43b156d6f33` |
| QUESTIONS.md | `3586db8780ffe0f835dc26280153cbb8d7f757230480f07fdccc91ceda3c2e5d` |
| PROVENANCE.md | `b2df627ec179e8e65360ffc3bf8bd27ccd00e8639bb1697f383590bf48b366e4` |
| test-vectors/README.md | `234c992bcf2ec2570cae67b2bb0a4cf5109bdee43f817845358f2e7e6fb468e3` |
| test-vectors/provisional.json | `06d2b2a769ec33420bf65fb797ca7f96b40558cfd2947fd204983ce46b722c86` |

UTC date: 2026-10-15. Role: reviewer. Approved SHA-256 of SPEC.md: `b09041f16661995375af617766d672db594c27a5bcbe2e8e5783c43b156d6f33`.
