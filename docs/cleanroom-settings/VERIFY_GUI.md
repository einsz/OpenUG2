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

# Verification 2026-10-14: authored pause carousel (SPEC_GUI_ART 1-3, tvg07/08/11)

## A. Test results (verbatim summary)
make gui-test rc=0 (gui_art_test 1855 checks 0 failed; gui_view_test 133/0; options_menu_test 1439/0 PASS; pause vectors/supplements/binding/behavior PASS). make settings-test: 2288 checks, 0 failed. settings-boundary-test PASS. ugui-test: 9992 checks, 0 failures. make (nfsu2 up to date) and make menu: build OK (warnings only, pre-existing).
Caveat printed by the suite itself: "pause: UI_pause.fng: reviewed object identities not uniquely resolved; screen not drawn (no native fallback)" and "resolve_authored_pause: unavailable (runtime metadata check deferred)" -- synthetic-fixture tests cannot see the defect below; none of the 12k checks cover real-package placement.

## B. Capture (no src change needed)
Flags in src/main.c: --pause-shot --shot F, --pause-ui-ms MS, --pause-events "t:Name,..." (names open Left Right Up Down Enter Continue Back Escape Start right_click, pointer/press/release/hover t:name:x:y). Replay is absolute-time ms, stderr line "pause capture: ... screen=pause|hub|display|customize selected=N dialog=D caption=...". Captures made (scratchpad): base (pause), hub (events 400:Right,800:Enter; ui 2000), display (+1500:Right,1900:Enter; ui 4000), customize (+3000:Down,3100:Down,3400:Enter; ui 6000), quit dialog (400:Right,800:Right,1200:Right,1600:Enter; ui 3000). All five screens are reachable and report the expected screen state. Hub=PAUSE OPTIONS/Audio, Display page (4 rows), Customize page, and "Quit to Main Menu?" OK/Cancel dialog render.

## C. Behavioural checks that pass in the real capture (career free roam)
* Entry list/behaviour consistent with tvg07: caption "Return to Explore Mode" at start; Right x3 from entry 1 ends on selected=3 (Quit, caption "Quit to Main Menu") skipping disabled Replay (index 2) and not wrapping; Enter on Quit opens the dialog (dialog=1). Options (entry 2) Enter opens hub with caption Audio. Options and Quit icons are present (neighbour at x=1316 px / Quit icon visible dim under dialog). Entry-by-entry caption for Options/Replay/Quit not individually captured (only 1, 4 and hub).
* Horizontal geometry matches 3.2: scale 2.25 px/unit (1920x1080); focused slot centre x=960 (0.388 -> 961); neighbour centre measured ~1315 px = 0.388+158 -> 1316 expected. OK.
* Correction to the report that "only one entry carousel": at rest with focus on entry 1 there is no left neighbour, so 2 icons (focused + entry 2) is the correct count per 3.2 (visible = focused + immediate neighbours). Pre-change pause.png also shows 2, not 3. Not a defect.

## D. Divergence D1 (confirmed defect): icon objects drawn at wrong y (vertical origin)
Observed (pause2.png and base.png, 1920x1080, scale 2.25): the authored panel frame/glow/backing for the selected slot is at pixel (960,508) = centred (0.4,-14.3) as expected, and it is EMPTY. The icon textures (focused explore icon and neighbour Options icon) are drawn at pixel y ~920 (x correct, 960 and ~1315). 920 px = centred y +168.9. Offset from the expected centre y = -14.296 is +183.2 units (= +412 px at this scale); x offset 0. Same offset in hub (Audio icon at (960,920), Display neighbour (1315,920)) and under the quit dialog (dimmed icons at y ~920). pause.png (pre-change) had icons at y 508.
Violated statements:
* SPEC_GUI_ART 3.2 "Each update sets every bound slot's centre to (OPTION_MASTER.x + x_i + s, OPTION_MASTER.y)" with OPTION_MASTER centre (0.388,-14.296) in centred 640x480 units: the slot centre y is -14.296 for all slots; observed ~ +168.9.
* tvg08 rest vectors (N=2,c=1): option_1 centre (0.388,-14.296) size 118x59; option_2 centre (158.388,-14.296) size 94.4x47.2. Observed y wrong; x agrees; sizes appear plausible (focused icon ~ 2.25*? not measured to the pixel).
* SPEC 3.4 note: glow 0xDF422345 / backing 0x687FDA31 sit at the authored centre ~(39.3,7.0) in the presentation group "at the selected slot". The render shows the glow at screen centred y=-14.3 and icons at +168.9, so the spec's two coordinate frames (slot centre in centred screen space vs glow in presentation-group space) are not shown to coincide in the implementation; the spec never states the frame (screen-centred vs presentation-group-local) of OPTION_MASTER or the group's transform. The only evidence tying them is the retail screenshot (icon inside the panel row).
Proposed amendments:
 1. SPEC_GUI_ART 3.1/3.2: state explicitly the coordinate frame of OPTION_MASTER.centre and of the bound slot centres written by the application (centred screen units vs. parent-group-local), and whether the presentation group (which contains glow/backing) has a non-identity translation/scale; if group-local, give the group transform and the resulting screen position.
 2. New vector tvg08 "screen_position_rest": for UI_pause.fng N=4 c=1 at 1920x1080 (centre (960,540), 2.25 px/unit), expected screen-space centre of option_1 = (960.9, 507.8) px, size 265.5x132.75 px; option_2 = (1316.4, 507.8) px, size 212.4x106.2 px; option_3/4 hidden (|x|>240 not exceeded for option_3 at +316 -> hidden). Selected-slot glow centre must coincide with option_1 icon centre within 1 px. Same for UI_PauseOptionsMain (Audio at (960.9,507.8), Display at (1316.4,507.8)).
 3. Capture request / reference render: provide the retail (or analyst-derived) reference image with measured icon centre of the selected entry (the earlier pause.png rendering has it at ~ (960,508)); verifier will diff pixel y of the icon bbox centre vs. the glow centre (tolerance 2 px) at 1920x1080 and 1280x720.
 4. Add an end-to-end test requirement (verification-only, real packages, not synthetic): the offscreen --pause-shot path must assert icon-bbox centre y within the panel row; current synthetic suites pass despite the defect.

## E. Other observations (not asserted as divergences)
* Hub neighbour icon (Display, at 1315,920) looks horizontally stretched/skewed compared with the focused Audio icon; cannot tell if art or scale defect; request pixel-size expectation in the vector D1.2 (neighbour 94.4x47.2 -> 212x106 px) to decide.
* Customize capture: first focused row is "Car Reflection Update Rate" and panel background is lighter/translucent versus Display; SPEC 4.7/tvg09 first-row expectation not checked here (Customize list contents depend on caps).
* Dialog: "Quit to Main Menu?" with OK (focused-looking, bright) and Cancel (dimmed). SPEC 9.3 says default focus Cancel. Appearance suggests OK is the bright/highlighted button and Cancel the dim one: possibly inverted focus rendering or Cancel being the focused (darker text) button; need a capture request: focused-button visual (tvg10) pixel/colour expectation, since the replay state output does not print the dialog focus. The "Return to Explore Mode" caption is faintly visible through the dialog (dim, not fully faded) -- spec 10 item 2 says final alpha unknown.

## F. Not verified
Pixel parity with retail (no retail reference image available for hub/Display/Customize/dialog), animation timing in real packages (tween, 320 ms exit), edge fade, resolution other than 1920x1080, Replay entry enabled state, race/online contexts of tvg07 (only career free roam exercised), pointer replay.
