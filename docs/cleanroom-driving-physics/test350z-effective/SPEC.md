# Temporary 350Z effective parameter composition

This companion extends `../test350z/SPEC.md` and the previously approved Stage1–5 contracts. Its domain is an ordinary driving family, ten requested category grades all equal to 2, no extra-instance adjustments, and **no active manual tuning entries**. These are temporary experiment assumptions, not a finding about the exact opening frame. All behavioral findings below are **inferred** from authorized static evidence; byte constants and format facts are observed file data. No target instructions were executed. Implementation approval is recorded in REVIEW.md against exact file hashes.

## E0 Scope and activation

The supported entry reconstructs category selections from requested grades. Start with all component flags clear. For each category 0 through 9, clear the flags for every nonnegative component index appearing in any package row of that category (levels 1 through 3), then process levels 1 and 2 using the companion's package activation behavior. Negative descriptor indices are ignored, missing rows are skipped, and requirement/exclusion checks use the current shared selection, including earlier categories. Category order is ascending. This differs in general from level-major aggregate selection. For the inspected asset, both orders produce the same 34 flags. Require equality to the companion's selected flag set before activating this temporary profile; report a mismatch and keep activation unavailable. Do not silently replace the input mask.

The separate preserved-flags entry can derive a new category-9 selection from categories 0,5,8. Its caller selection and global override are unresolved and that entry is outside this contract. The chosen entry policy must be recorded as `reconstruct-ten-category-grades`. This is a bounded, coherent original composition branch; exact opening configuration remains unproven.

The effective parameters below replace stock pass-through at the existing Stage1–5 interfaces. They must affect actual dynamics, not only displayed upgrade metadata. The experiment remains opt-in and reversible under the companion. No extra torque/grip/steering multipliers may substitute for composition. Nitrous capacity may be composed, but nitrous activation/dynamics remain excluded by Stage1–5.

## E1 Numeric and asset interface

All numeric input fields are little-endian binary32 except explicit integer fields. `b(x)` means binary32 rounding to nearest, ties to even. Effective numeric fields, accumulated weights, generated/resampled samples, speed and score are stored binary32. The retained mean f and other intermediates are not implicitly rounded; an explicit b(...) requires a binary32 store. Intermediates use at least binary64 without artificial b32 rounding unless stated. This specifies arithmetic tolerances, not bit-exact x87 execution. Inputs are finite, records nonoverlapping and physically valid. Invalid assets prevent activation; no original malformed-input behavior is claimed. Final consumed parameters must also satisfy the valid domains in the Stage1–5/body-response contracts: positive mass, dimensions, inertia, wheel radii and final drives, positive forward ratios through the declared count, forward count 1…6, finite nondegenerate engine curve domain (maximum rpm greater than idle rpm), valid travel ranges and the documented coefficient/share constraints. Reject a profile that violates a consumer domain or generates fewer than two scoring samples. This validation is a new-host requirement.

Use the local base car table and package table already identified by the companions. Each car row is 0x890 bytes; define `S[o]` as its f32 field at row offset `0x110+o`. Define `E[o]` as a field in a separate effective record. Initialize E from the base subrecord bytes 0 through 0x2AF. The +0x110 distinction is essential: offsets in the following equations are **subrecord offsets**, not row offsets. No original asset record is distributed here. Integer and opaque fields inherited from the companions retain their types; copying the initial bytes does not reinterpret them as floats. The effective forward count and row +0x844 override are signed 32-bit integers; the existing wheel indices remain unsigned 32-bit integers.

Extend the companion's 92-byte package component descriptor: bytes 48,52,56,60 contain four f32 contribution weights. The exact binary32 -1 value means no contribution for that component. Required weight count is 3 for categories 1 and 5, 2 for category 9, and 1 otherwise. Reject nonfinite used weights, out-of-range indices or inconsistent package copies under the companion's loader rules.

For category c, initialize the n weights to zero. In ascending package level 1,2,3, accumulate each selected descriptor's nonsentinel contribution into its corresponding weight, rounding **each addition** to b32. Preserve descriptor order within a group. Compute mean `f=sum(weights)/n` without a b32 spill. Only if f<0 or f>1, clamp each weight individually to [0,1], and recompute `f=sum(clamped_weights)*b(1/n)`. Thus individual weights can remain outside [0,1] if their initial mean is in range. Store output grade `G[c]=b(3*clamp(f,0,1))`. Retain the computed f separately. All ten category grades are recomputed; requested integer tier is not their physical fraction.

Let `mix(x,y,w)=b((y-x)*w+x)` and `add(x,y,w)=b(y*w+x)`. Unless category 4 is stated otherwise, skip a category's numerical modifications when its computed mean f is exactly zero. A component weight and its category mean are not interchangeable.

## E2 Composition equations

Apply these category transformations in numeric order 0 through 9. Subscripts denote that category's weights; f denotes its mean.

| Category | Effective changes |
| --- | --- |
| 0 | `E[0]=add(E[0],S[0x344],f)`; `E[0x160]=mix(E[0x160],S[0x348],f)`; `E[0x110]=b(((S[0x340]-1)*f+1)*E[0x110])`. These are centre x, yaw-inertia multiplier and mass. |
| 1 | See E3. |
| 2 | `E[0x2A0]=mix(E[0x2A0],S[0x580],w0)`. Set `E[0x2AC]=b((S[0x58C]-E[0x2AC])+E[0x2AC])`, with no fraction multiplication in this second expression. The latter scalar's full dynamic meaning remains outside scope. |
| 3 | For j=0…8: `E[0x200+4j]=add(E[0x200+4j],S[0x420+4j],f)`. For j=0…2 apply the same equation at destination `0x248+4j`, source `0x444+4j`. These are primary torque and closed-throttle drag samples. |
| 4 | **Always execute, including f=0.** For j=0…8 let `v=(S[0x510+4j]-E[0x224+4j])*f+E[0x224+4j]`. Store `E[0x224+4j]=b(v)` and `E[0x200+4j]=b(v+E[0x200+4j])` using the same unrounded v. This composes the secondary contribution into primary torque exactly once. |
| 5 | See E4. |
| 6 | Mix destinations `0x27C,0x278,0x274` toward sources `0x5B8,0x5B4,0x5B0` respectively using f. These are rear service share, handbrake strength and service-brake strength. |
| 7 | Independently of f, if selected component 45,46 or 51 is enabled set `E[0x290]=0` (speed-limit removal). When f!=0, let q=`trunc(G[7]+.5)`. If q is 1…3, add source `0x410+0x40q`, `0x414+0x40q`, `0x418+0x40q` to engine destination `0x1F0,0x1F4,0x1F8` respectively, without multiplying by f. Then add f-scaled source `0x4E0+4j` into primary torque `0x200+4j` for j=0…8 and source `0x504+4j` into closed-throttle drag `0x248+4j` for j=0…2. |
| 8 | For axle a=0,1 and k=0…4: mix destination `0xD0+0x20a+4k` toward source `0x540+0x20a+4k` using f. These supply Stage4 angle scale, reference load, output factor, peak grip and sliding grip respectively. No common grip multiplier replaces them. |
| 9 | `E[0x264]=b((S[0x590]+S[0x5A0])*w0+E[0x264])`; `E[0x268]=b((S[0x594]+S[0x5A4])*w1+E[0x268])`; `E[0x26C]=b((S[0x598]+S[0x5A8])*f+E[0x26C])`. These are Stage2 front/rear aero and Stage5 aero-to-preload factor. They are **additions**, not interpolation toward targets. Ordinary drag `E[0x260]` remains copied from base in this bounded branch. |

Fields without a listed modification remain copied from the base record. In particular wheel radii, body dimensions, wheel mount x/y, tyre parameters beyond the five specified per axle, and Stage5 rear-slip bonus coefficients receive no invented upgrade adjustment.

## E3 Transmission

For nonzero category-1 mean, let q=`trunc(3*w2+.5)`. If 1<=q<=3, use the source variant at subrecord `V=0x310+0x40q`. Copy its relative fields 0,4,8,12,16 into effective drivetrain `0x1B0` at the same relative offsets, and its eight relative ratio fields 32…60 into `E[0x1D0…0x1EC]`. Relative 4 is an opaque four-byte compatibility value; no new behavior is inferred for it. The copied fields include launch fraction, front/rear final drives and rear drive share. Do **not** copy variant relative 20 or 28 as clutch coefficients.

Count consecutive nonzero ratios at variant relative positions 40,44,…60 (forward gears, indices 2…7), stopping at the first exact zero. Store the count as signed 32-bit at `E[0x1C8]`. Original source-side count mutation is not required for independent read-only asset parsing; the effective count is required. For q outside 1…3 keep base ratios, count and copied fields.

Then `E[0x1CC]=mix(E[0x1CC],S[0x3EC],w0)`, `E[0x1C4]=mix(E[0x1C4],S[0x3E4],w0)`, and `E[0x1FC]=add(E[0x1FC],S[0x41C],w1)`. These are clutch rate, coupling strength and engine inertia. Recompute engine curve domains, gearbox shifts and all dependent cached values from the final effective record; retaining stock caches is a composition failure.

## E4 Suspension and steering

For each axle a=0,1, the eight suspension destinations start at `0x170+0x20a`, with corresponding target fields at `0x5C0+0x20a`. Mix fields at relative 0,4,8,12,20,24,28 using w0, and field 16 using w1. They respectively represent progression, stiffness, positive/negative travel-rate damping, upper/lower travel, presentation travel reference, with field16 being paired-wheel coupling. The presentation reference remains a presentation reference under Stage5; it is not inserted into the spring-force equation.

Add `w0*S[0x604]` to mount z at `E[0x18]` and `E[0x48]`; add `w0*S[0x608]` at `E[0x78]` and `E[0xA8]`. Set centre height `E[8]=b(E[8]-S[0x600]*w0)`. Mix steering gain `E[0x270]` toward `S[0x5BC]` using w2. Recalculate the Stage5 centre-relative arms, shares and Stage4 steering geometry from these values.

## E5 Inertia, tuning and dynamic interfaces

After all category transformations, use final mass M=`E[0x110]` and copied dimensions dx,dy,dz=`E[0x114],E[0x118],E[0x11C]`. Set the three diagonal inertia values to `b((dy²+dz²)*M*b(1/12))`, `b((dx²+dz²)*M*b(1/12))`, `b((dx²+dy²)*M*b(1/12))`. The Stage5 second inertia replaces the third value Iz by `b(Iz*E[0x160])`; retain the established supplied body-mode selection. No inertia floor, wheelbase substitution or centre parallel-axis correction is added.

The supported experiment explicitly supplies all extra-instance adjustment flags as zero and disables all active manual tuning entries. A tuning byte of 100 by itself does **not** establish no active tuning: active-entry processing can constrain values even at zero offset. Any other tuning/extras state is unsupported by this amendment and must be visibly reported as such. It is not silently ignored. Ordinary family means no additional event-family parameter transformation; actual runtime mode provenance is unknown.

Supply the resulting E values once to all Stage1–5 consumers, including engine torque/drag/inertia/rpm limits, ratios/final drives/rear share/count, steering, five independent tyre channels, spring/damping/travel/pairing, brakes, mass/inertia and aero/preload. Do not add the secondary torque a second time under the old stock shortcut. These rules supersede the corresponding stock pass-through clauses only for this opt-in profile.

The mapping to existing consumers is direct: for every companion car field described at row offset r, use the final effective value at `E[r-0x110]`. This mapping does not move the source target fields, which remain `S[...]`. In particular:

| Effective field(s) | Consumer meaning |
| --- | --- |
| `0,4,8`; `0x10+0x30i` (three floats) | Body centre x/y/z; wheel-i mount x/y/z, metres |
| `0x20+0x30i` | Wheel-i rolling radius, metres; unchanged by these upgrades |
| `0x110`; `0x114,0x118,0x11C`; `0x160` | Mass, tonnes; dimensions, metres; dimensionless second-inertia multiplier |
| `0x1B0`; `0x1B8,0x1BC,0x1C0`; `0x1C4,0x1C8,0x1CC` | Launch fraction; front/rear final drive and rear share; coupling, count and clutch rate in Stage1 units |
| `0x1D0+4j`; `0x1F0,0x1F4,0x1F8`; `0x1FC` | Gear ratios; idle/shift/maximum rpm; engine inertia in tonne·m² |
| `0x200+4j`; `0x224+4j`; `0x248+4j` | Composed primary torque; retained secondary presentation curve; closed-throttle drag, all kN·m |
| `0x260,0x264,0x268,0x26C`; `0x270`; `0x274,0x278,0x27C`; `0x290` | Stage2 drag/front/rear aero and Stage5 preload factor; steering gain; service/handbrake/rear share; imposed speed threshold in mph (zero disables) |

E2/E4 give the per-axle tyre and suspension maps; their units and angle-scale dual use remain those of Stage4/5. Body-response consumes the recomputed inertias and centre from this same effective profile, preserving its independent supplied mode selectors and damping laws. Contribution weights, means and grades are dimensionless. Target/delta values have their destination units, except category-0 mass scale which is dimensionless. No conversion from rpm to rad/s or mph to m/s is made during composition; the existing consuming equations perform their stated conversions.

## E6 Performance-class calculation

This resolves Stage1 U-11 for the bounded experiment. Use the final effective record to form an acceleration-versus-speed curve, with speed in m/s and acceleration in m/s². Define `torque_curve(rpm)` as Stage1 endpoint-clamped linear lookup of the final nine primary samples `E[0x200+4j]` on `[E[0x1F0],E[0x1F8]]`; do not add secondary torque or apply throttle, clutch, bonus, nitrous or runtime torque scales here. Use Stage1's ordinary, nonoptimal creation shift thresholds, without random jitter; this auxiliary curve does not include the later launch-scale modification to U[1]. Let rear share s, `R=b(s*Rrear+(1-s)*Rfront)`, `F=b(s*Frear+(1-s)*Ffront)`, `invR=b(1/R)`, M effective mass. Set gear index g=2, speed v=0. Generate at most 92 samples, with binary32 speed increment `d=1.3410818576812744`:

- Compute rpm=`b(invR*v*ratio[g]*F*Kr)` using Stage1's Kr.
- If rpm>=U[g], increase g by exactly one. If this reaches n+2, stop before adding a sample. **Retain that pre-shift rpm** for the next equation.
- Append `b(torque_curve(rpm)*ratio[g]*F*invR/M)` and set v=`b(v+d)`.

Valid input domain requires at least two generated samples and positive final speed. Replace the last generated sample by zero; all following scratch samples are irrelevant/zero. Interpret the generated samples as a uniform linear curve spanning [0,v], despite the last generated value having used the preceding speed. Resample to 64 uniformly spaced samples including endpoints, storing b32: with k generated samples, source positions are `i*v/(k-1)` for i=0…k-1; evaluate their endpoint-clamped linear interpolation at `j*v/63` for j=0…63. Do not round these position calculations before lookup. This is a scoring curve, not a replacement for dynamic drivetrain coupling.

Start score A=0 and u=0. While u<=v, evaluate the resampled curve Q(u) with endpoint clamp and linear interpolation. Let resistance=`u*E[0x260]*u/M`. If Q(u)<resistance stop. Otherwise A=`b((Q(u)-resistance)*d+A)`, u=`b(u+d)`. Strict score thresholds select class C: A>500→0; else A>401→1; else A>273→2; else→3. Equality to resistance continues integration with zero score increment; equality to a score threshold chooses the next lower-performance class. Equality rpm==U[g] shifts, and only one shift is considered per generated sample. If the source row's signed 32-bit field at **row +0x844** is nonzero, force C=3 without this score. Record C and A (or forced status). Recompute the class after profile switching. Performance-display standing-start simulation is not needed for this driving contract.

## E7 Validation and trace

Synthetic fixtures in `test-vectors/` use invented numbers and contain no retail tables. They exercise sentinel accumulation, conditional clamping, zero-secondary composition, transmission variant choice, distinct suspension channels, inertia and strict class thresholds. Expected values are fresh mathematical calculations, not original runtime captures. The provenance statement in each case identifies synthetic mathematical derivation, not a runtime observation. Component-only inputs such as mean or a supplied rpm/shift pair enter at the named intermediate boundary; they need not be complete car records. Require exact integers/flags and strict predicate outcomes; allow float relative error 2e-6 and absolute error 2e-7 for these component fixtures. This tolerance is not trajectory or handling parity.

Emit entry policy, requested grades, reconstructed selection identity, computed per-category weights/means/grades, assumptions, effective consumed parameter summary and class in the run trace. Source records and descriptor tables remain local. Activation must fail if a covered consumer still sources an old stock cache instead of this final effective configuration; a legitimately unchanged field is allowed to equal its stock value. An independent verifier may inspect trace changes without redistributing raw car rows.

## Unknowns/Ambiguities

- Exact opening instance, asset load precedence, original caller's entry flag, derived-aero policy/override and first controllable-frame state.
- Original manual tuning selection/values, extras, transmission/assistance settings and event-family transformations. This spec supplies explicit temporary assumptions for these.
- Preserved-flags category-9 nearest-selection search, alternate tuning, extra-instance flags and all nonordinary family transformations are not implemented by this contract.
- Nitrous runtime, malformed source records, NaNs, bit-exact x87 rounding and complete original handling trajectories remain outside scope.
- Earlier Stage1–5 unknowns, as refined by the separately approved body-response contract, remain in force (including world collision, timing and external state producers). Effective upgraded parameters do not establish full driving parity.

Only reviewer-approved SPEC.md and the auxiliary files expressly approved in REVIEW.md are implementation inputs. PROVENANCE.md is coordinator/reviewer traceability only and is not an implementation input. Analyst evidence and original-reference research remain private.
