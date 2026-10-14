# VERIFY_GUI - verification of the graphics options GUI (2026-10-14)

Basis: SPEC_GUI.md sha256 9bcb6c8b...81da8 (confirmed). Branch settings-gui, uncommitted.

## 1. Test results (verbatim summaries)
- make options-menu-test: "options_menu_test: 1092 checks, 0 failed -> PASS"; chained pause options: "pause options checks: PASS"
- make settings-test: "settings_test: 2288 checks, 0 failed -> PASS"
- make pause-menu-test: all stages PASS (behavior 47 cases, quit dialog 30 cases/326 checks). Its own output states retail package metadata, art parity and captures remain unverified.
- make ugui-test: "9992 checks, 0 failures"
- make: "'nfsu2' is up to date." make menu: builds (only pre-existing -Wmisleading-indentation warnings in src/nfsu2.h).

## 2. Vector coverage in tools/options_menu_test.c
- tvg02 (6/6 cases), tvg04 (6/6), tvg05 (42/42), tvg06 (11/11; an unhandled id fails the test): all executed, row count compared explicitly.
- tvg03 (6/6 cases) are run against the list engine (om_list_init/om_list_move) only, not by driving om_input on a built menu. Walk through the menu is covered elsewhere only indirectly. Gap (minor).
- tvg01: only (a) hash(lookup_name) == key and (b) om_package_name/om_package_key match for 5 name/context pairs. NOT exercised: decoded_bytes, sha256, objects, resources, row_object_slots, singletons_present. These need retail data, which the test deliberately runs without (UGUI_DATA/NFSU2_DATA unset). The GenericDialog_SMALL.fng entry (key 0x6962C0CD) is hash-checked only.

## 3. Differential check of tvg01 against retail-derived evidence (analysis/settings-gui/evidence/json)
For UI_Options, UI_PauseOptions and UI_PC_Customize_Options: object count (234/120/375) and resource count (20/15/18) match; every row_object_slots range and every singletons_present value matches when objects are looked up by name hash. Zero mismatches. (GenericDialog_SMALL not re-checked.) Package sha256/decoded size not recomputed. This confirms the vector against the evidence JSON, not the implementation against the retail files.

## 4. Behaviour versus SPEC_GUI / SPEC 4.4 and the four 10 amendments
Demonstrated by passing tests: row lists and cap omission (amendment 3), Back writes registry once after exit animation and second Back ignored (5.1), Start in game resumes with zero registry writes (5.2, amendment 4), Customize open/back without writes (5.3), immediate re-init request for Light Glow vs apply-only for Crowds (5.4, amendment 1), Default Settings dialog keys/buttons/initial focus Yes, Yes = auto-detect + re-init + rebuild focus row 1 without registry write, No/Back no change (5.6, 6), frontend-only rebuild when the reset lowers resolution (5.5, amendment 2; in-game list not refreshed, matching spec).
Not demonstrable: no message shown on failure (absence only, implementation has no UI for it); all-attempts-fail outcome (spec 11.9 unknown); retries/FSAA=0 sequence belongs to the host reset, not covered by options-menu-test; DIM/UNDIM scripts; pixel/visual parity (implementer Q12: v1 draws natively, not from the packages).

## 5. Divergences / proposed amendments
D1 (test, not behaviour). tvg03 vectors exercise only the list engine. Proposed amendment to test-vectors-gui README: state whether tvg03 must be driven through menu input; if so add vector cases with Display (8 rows) and Customize (22 rows + spacer) built from caps, driven by Up/Down through the screen.
D2. tvg01 non-hash fields have no automated consumer. Proposed: add a retail-gated loader test (skipped when data missing) that checks package sha256/object/resource counts and slot ranges, or mark those fields "evidence only" in the README.
No behavioural divergence from SPEC_GUI was found in what the spec defines.

## 6. QUESTIONS.md review (12 GUI implementer questions) - proposed answers / amendments
1. Exit anim length: the evidence JSON shows the 0x47FF4E7C-related tracks; frontend-style scripts in UI_Options contain len 100/140/180 ms tracks (e.g. 140 and 100). Capture request: read the event-handler object's Forward script length (analysis, static, no runtime needed); amend SPEC 5.1 with the value. 140 ms default is plausible but unconfirmed.
2. Exit animation on Customize->Back and input blocking: SPEC 5.3 is silent on Back. Proposed amendment: analyst to inspect whether Customize has an event-handler object with a Forward/Back script; until then mark as [inferred]. Input blocked during every animation: add as [inferred].
3. Default dialog Left/Right: SPEC 6/11.5 unknown; capture request 12.3 covers it. Dialog button y/extents: analyst should add the button1/button2 positions and sizes from GenericDialog_SMALL.fng (static data, available in evidence/json GLOBALB_BUN_6962c0cd) so pointer support can be specified.
4. Pointer on rows: SPEC 11.2/11.10 unknown. Proposed amendment: extract the arrow and label hit extents from the packages; add a capture request (video of mouse over rows, wheel, scrollbar).
5. Spacer when row 1 absent: spec 4.1 silent. Propose [inferred] statement "no spacer when row 1 omitted"; confirm by static analysis of the Customize row builder or capture with cap 0 (low priority).
6. Slider domains / FSAA unbounded and hidden when caps are "no limit": FSAA hiding under ST_NO_CAP is an implementation choice not in the spec. Propose spec text defining behaviour when caps are unknown, or record as implementation-defined.
7. In-game remembered focus: SPEC 11.4 unknown; capture request: in game open Display, move focus, Back, re-enter, observe.
8. Start outside in-game Display: spec silent; propose explicit "ignored" [inferred].
9. Pause row / Options hub: SPEC 1.2/7 describe UI_PauseOptionsMain; project has no hub. Propose spec statement of the hub's required behaviour or declare the Options pause entry a project-specific stand-in.
10. P=4/5 presets lower resolution to index 1 on modern GPU: this follows SPEC 4.2 table literally; flag to analyst to confirm whether the table's resolution column is capped by the available mode list/CPU+GPU class (capture: run Default Settings on modern hardware, read g_RacingResolution).
11. Registry write failure: spec silent. Propose statement "no UI" [inferred] or capture.
12. Visuals: unverified; parity of layout/animation cannot be claimed. Capture requests 12.1-12.4 remain needed.

Backend QUESTIONS.md items 1-9 (settings backend) and the 8 analyst open questions are unchanged by this run; settings-test passes, but defaults of unknown fields remain guesses (needs SPEC 12.1 capture).

## 7. Not verified
Retail rendering, animation timing, pointer behaviour, dialog geometry, in-game route through a real Options hub, package loader against retail data, all-attempts-fail display reset, host display-reset retry sequence (FSAA 0 x3, resolution step-down).
