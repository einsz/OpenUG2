# Graphics / display options screens: structural and behavioural specification

Status: draft for review. Companion to `SPEC.md`, which stays authoritative for value domains,
caps, presets, persistence and the meaning of every option (SPEC §2.3, §4). This document covers
only what the screens that edit the graphics options look like and how they behave: which authored
packages are used, how rows are composed and laid out, focus, scrolling, input, dialogs and
transitions. It does not repeat SPEC §4 except where a GUI fact amends it (§10).

Provenance: static examination of the retail PC executable (sha256 `f9dd86c0…24168a7`, the same
binary as SPEC.md); retail data files from the user's own install (only the original 2004 files:
`FRONTEND/FRONTB.LZC.bacc`, `GLOBAL/InGameCommon.bun`/`.lzc`, `GLOBAL/GLOBALB.BUN`,
`LANGUAGES/English.bin`); one user-supplied retail screenshot of the *Audio* page, which uses the
same screen package and list engine as the frontend Display page; the public FEngLib project
(object-type and flag enumerations). The game was not run. Test vectors are synthetic.

Confidence tags:

* **[data]** read from the retail package/language data.
* **[static]** determined by examining the executable; not confirmed at runtime.
* **[observed]** measured on the retail screenshot.
* **[inferred]** one reasoning step from the above.
* **[guessed]** plausible, untested.

Functional identifiers. Package lookup names (`UI_Options.fng` …), object names inside packages
(`OPTION_NAME_3`, `TITLE_MASTER` …), script names (`Highlight` …), FEng message hashes, texture
names and language-label keys are **data lookup keys**. An implementation that drives the retail
packages has to use them byte-exactly; they are flagged as functionally required and are not
original code identifiers. Hash function for every key below (package keys, object names, script
names, messages, texture names): `h = 0xFFFFFFFF; for each byte c: h = h*33 + toupper(c) mod 2^32`.
Label keys are the language-file keys of SPEC.md (English text given for convenience).

Coordinates: centred 640×480 virtual units, x right, y down, origin at screen centre (add
(320, 240) for top-left coordinates). Colours are RGBA; package data stores B,G,R,A.

---

## 1. Screen graph

### 1.1 Overview [static]

There are **two** list screens for graphics options, not one:

| Screen | Purpose | Rows |
|---|---|---|
| **Display** | top-level graphics page | 4 rows: Resolution, Level Of Detail, Customize Display Settings, Default Settings |
| **Customize** | the individual graphics options | up to 22 rows (one per capable option, §4) |

Every one of the 24 options of SPEC §2.3/§4.4 is edited on exactly one of them: Resolution and
Level Of Detail on Display, the other 22 on Customize.

### 1.2 Packages and routes

| Context | Display package | Customize package | Parent screen (Back target) |
|---|---|---|---|
| Front end (main menu → Options) | `UI_Options.fng` (key `0x551CA351`) | `UI_PC_Customize_Options.fng` (`0xC4ACC9A5`) | `UI_OptionsMain.fng` |
| In game (pause → Options) | `UI_PauseOptions.fng` (`0xAEA9078F`) | `UI_PC_Customize_Options.fng` (same) | `UI_PauseOptionsMain.fng` |

[static for routes; data for the keys]

* Front end: the options hub (`UI_OptionsMain.fng`) sets the "current options page" to *Display*
  (page index 1; Audio is 0, Gameplay 2, Player 3) and switches to `UI_Options.fng`. The same
  package serves all option pages; the page index selects title and rows. [static]
* In game: the pause options hub (`UI_PauseOptionsMain.fng`) switches to `UI_PauseOptions.fng` with
  page index 1. [static]
* From either Display screen, *Customize Display Settings* switches to
  `UI_PC_Customize_Options.fng`. Back from Customize returns to `UI_PauseOptions.fng` when the game
  is in its in-game state, otherwise to `UI_Options.fng`. [static]
* Each switch replaces the current screen package; Display is rebuilt from scratch when it is
  re-entered (§5.6).

### 1.3 Where the packages are [data]

| Package | Container(s) | Decoded bytes | Objects | Resources |
|---|---|---:|---:|---:|
| `UI_Options.fng` | `FRONTEND/FRONTB.LZC.bacc` (chunk `0x00030210`, JDLZ) | 207 908 | 234 | 20 |
| `UI_PauseOptions.fng` | `GLOBAL/InGameCommon.bun` and `.lzc`, **uncompressed** chunk `0x00030203`; both copies byte-identical | 119 840 | 120 | 15 |
| `UI_PC_Customize_Options.fng` | `FRONTEND/FRONTB.LZC.bacc` and `GLOBAL/InGameCommon.bun`/`.lzc` (chunk `0x00030210`); all copies byte-identical | 377 952 | 375 | 18 |
| `GenericDialog_SMALL.fng` | `GLOBAL/GLOBALB.BUN`, `GLOBALB.LZC.bacc`, `FRONTEND/FRONTB.LZC.bacc`; identical | 18 112 | 55 | 12 |

SHA-256 values are in `test-vectors-gui/tvg01_packages.json`. Note for loaders: the in-game
options packages (`UI_PauseOptions.fng`, `UI_PauseOptionsMain.fng`) are stored as chunk
`0x00030203` (raw package, no key word, no compression), unlike the `0x00030210` records. A loader
that indexes only `0x00030210` records will not find them. The chunk payload starts with the
`FEn\xE7` package header; the key is the hash of the name in the package header. [data]

The 2023-dated `FrontB.lzc` / `GlobalB.lzc` in the user's install are modified files and were not
used.

## 2. The option-list engine (shared by all three screens)

### 2.1 Row kinds [static]

| Kind | Objects bound (n = row slot, §2.2) | Shows | Responds to |
|---|---|---|---|
| **choice** (two-state toggle or multi-value) | `OPTION_NAME_n`, `OPTION_DATA_n`, `LEFT_ARROW_n`, `RIGHT_ARROW_n`, `OPTION_BACKING_n` | label; value text; left/right arrows | Left, Right; toggles also Accept (§2.6) |
| **slider** | `OPTION_NAME_n`, `SLIDER_BASE_n`, `SLIDER_KNOB_n`, `SLIDER_HANDLE_n`, `SLIDER_BEG_n`, `SLIDER_END_n`, `LEFT_ARROW_n`, `RIGHT_ARROW_n`, `OPTION_BACKING_n` | label; slider bar; arrows. **No value text.** | Left, Right only |
| **button** | `OPTION_NAME_n`, `OPTION_BACKING_n` | label only | Accept |
| **spacer** | `OPTION_NAME_n`, `OPTION_BACKING_n` | nothing (blank row); not selectable | — |

Object names are case-insensitive hashes (the packages mix case, e.g. `Left_arrow_2`,
`LEFT_ARROW_2`). An object missing from the package is simply absent (e.g. `UI_PauseOptions.fng`
has no `OPTION_BACKING_n`, so in game the focused row has no backing box). [static; data]

Arrow artwork is assigned by the screen, not the package: for every graphics row both arrows use the
texture whose name hash is `0x9943D584` (`U2_MENU_ARROW_WHITE`); the left arrow is the same image
rotated 180° by its authored rotation track. (Other pages use `U2_MENU_ARROW` `0x367B3E04`, or
`U2_MENU_MINUS`/`U2_MENU_PLUS` `0x374F6465`/`0x760BDC9D`.) [static; data]

### 2.2 Row slots

Rows are numbered 1…N in list order. Row *n* binds the objects whose names end in the decimal
number *n* (`OPTION_NAME_` + n, etc.). Binding is by list position, not by visible line: a package
must contain one object set per row. Slots available [data]:

| Package | NAME/DATA | BACKING | arrows | slider parts |
|---|---|---|---|---|
| `UI_Options.fng` | 1–13 | 1–15 | 1–12 | 1–11 |
| `UI_PauseOptions.fng` | 1–11 | none | 1–11 | 1–7 |
| `UI_PC_Customize_Options.fng` | 1–25 | 1–27 | 1–25 | 1–25 |

All Display rows (4) and all Customize rows (≤ 23 with the spacer) fit. [inferred]

### 2.3 Layout [static, with observed numbers]

* Two invisible anchor objects define the grid: `TITLE_MASTER` (label column) and `DATA_MASTER`
  (value column). The row pitch equals the **height of `TITLE_MASTER`**; the first visible row is
  placed at the y of `TITLE_MASTER`. [static]
* Visible row k (k = 0 … V−1, counted from the top of the window) is placed at
  `y = TITLE_MASTER.y + k × pitch`. Rows outside the window are hidden (and parked far off-screen). [static]
* V (visible rows) = **8** on all three screens. [static]

Authored anchors (absolute centres, size) [data]:

| Package | `TITLE_MASTER` | `DATA_MASTER` |
|---|---|---|
| `UI_Options.fng` | (−151, −120), 240×36 | (126, −122), 216×32 |
| `UI_PauseOptions.fng` | (−150.2, −120.9), 240×36 | (124.7, −122.9), 231.18×32 |
| `UI_PC_Customize_Options.fng` | (−152, −120), 240×36 | (126, −122), 200×32 |

Pitch is therefore 36 everywhere. Measured on the retail Audio page (same package and engine as the
frontend Display page) [observed]:

* row pitch 36.0; first-row label cap centre y ≈ −126.2; labels left-aligned at x ≈ −268.9;
  label scale 0.7;
* focused-row backing x −286.2…+261.8, y −144.4…−109.6 (≈ 35 high, centred ≈ −127), grey ≈ (65,65,65);
  unfocused rows show no backing;
* value arrows on the focused row only, centres x ≈ +6.2 (left) and +246.2 (right), visible disc ≈ 21.5;
* value text centred at x ≈ 127.8 (≈ `DATA_MASTER` x);
* slider fill starts x ≈ 54.6, fill height ≈ 7, colour (98,178,34); knob centre at full value x ≈ 196.6;
  track to ≈ 202.9.

The in-game and Customize packages are assumed to follow the same rule relative to their own
anchors. [inferred]

### 2.4 Row visual states (authored scripts) [static for which script is run; data for the content]

| Event | Scripts run on the row's objects |
|---|---|
| row gains focus | `Highlight` on name, data, left arrow, right arrow and backing |
| row loses focus | `UnHighlight` on the same objects |
| row made non-selectable | `Disable`/`disabled` (name/data drawn at alpha 120 in `UI_Options.fng`) |
| row made selectable again | `UnHighlight` |
| Left/Right pressed on a row | the corresponding arrow plays `Blink` (400 ms) [inferred] |
| screen enters / leaves | `FadeIn` / `FadeOut` (frontend: 180 ms / 140 ms; in game data/name: 220 ms / 100 ms) |
| dialog over the screen | `DIM` / `UNDIM` (100 ms) |

Authored effect of these scripts (data, `UI_Options.fng`): arrows `Highlight` → alpha 150,
`UnHighlight` → fade to alpha 0 within 100 ms (so arrows are visible on the focused row only);
backing `Highlight` alpha 200, `UnHighlight` alpha 0; name/data `Highlight` is a 1 200 ms looping
lime pulse, `UnHighlight` returns them to dark grey (50,50,50). In `UI_PauseOptions.fng` the
name/data base colour is lime (202,244,138) on a black panel. Implementations that evaluate the
authored tracks get these automatically; values are listed only as acceptance hints.

### 2.5 Focus and scrolling [static]

Navigation is **non-wrapping**:

* Down: if the focused row is the last row, nothing happens. Otherwise move to the next row,
  continuing past non-selectable rows; stop at the first selectable row or at the last row.
* Up: mirror image; stops at the first row.
* After a move the window (first visible row `top`) is adjusted minimally: while
  `focus ≥ top + V`, `top += 1`; while `focus < top`, `top −= 1`.
* Initial window when the screen is built: if the list has ≤ V rows, `top = 1`; else `top = focus`
  when `focus ≤ N − V + 1`, otherwise `top = N − V + 1`.
* Initial focus: the first selectable row (frontend Display and Customize). In game the Display
  screen starts on a row index remembered by the screen framework, falling back to the first
  selectable row. [static; meaning of the remembered index inferred]
* The focus change runs `UnHighlight` on the old row and `Highlight` on the new one, then updates
  the scrollbar.

Vectors: `tvg03_navigation.json`.

### 2.6 Scrollbar [static; geometry data/observed]

* Objects: `ScrollBar_Backing` (8×282 track), tab image (name hash `0x3FBB7A4E`,
  `U2_Scroller_Tab.tga`), `ScrollBar_Arrow_1` (up, y −164) and `ScrollBar_Arrow_2` (down, y +167),
  corner caps; frontend x 273, in game x 275. [data]
* The bar is shown whenever the list has more than one row, even when all rows fit (the retail Audio
  page with 8 rows shows bar, arrows and tab). [static; observed]
* The tab tracks the **focused row**, not the window: its position is linear in the focused row
  index, from the top of the track for row 1 to the bottom for row N. [static for linearity;
  exact end positions inferred]
* Clicking the scrollbar arrows: unknown (§11).

### 2.7 Input [static]

Messages (FEng message hashes, functional): Up `0x72619778`, Down `0x911C0A4B`, Left
`0x9120409E`, Right `0xB5971BF1`, Accept `0x406415E3`, Back `0x911AB364`, Start `0xB5AF2461`,
button activation `0x0C407210`.

| Input | choice row | slider row | button row |
|---|---|---|---|
| Left / Right | change value (SPEC §4.4) | step value (below) | nothing |
| Accept (button activation `0x0C407210`) | toggles: flip (SPEC §4.4); Resolution: nothing | nothing | activate |
| Back | leave the screen (§5) | same | same |

* Slider step (all graphics sliders: min, max, step 1): Right sets `v = min(v + 1, max)`, Left
  `v = max(v − 1, min)`. The setting is applied (and display re-init requested, SPEC §4.4) only
  when the value really moved; pressing towards a limit that is already reached does nothing.
  Ranges: Level Of Detail 1…5; every other slider 0…cap (SPEC §4.3). When a row is built the
  slider position is the stored value clamped into [min, max]; e.g. a stored Level Of Detail of 0
  shows at 1. [static] Vectors: `tvg04_slider.json`.
* Slider drawing: fill fraction `f = (v − min) / (max − min)`; the fill runs from the track start to
  the knob, the knob sits at fraction f of the track. Measured: Audio "Menu Music" 0.7 → knob at
  x 154.5 vs 54.6…196.6 (f = 0.703). [static; observed]
* Mouse: a primary click on the focused row's **left arrow** acts as Left; a click on the **right
  arrow or on the value text** acts as Right. [static] Hover-to-focus and wheel behaviour are
  unknown (§11).
* Each accepted input plays a UI sound chosen from a per-event table; not specified here.

## 3. Display screen

### 3.1 Rows [static]

| Slot | Kind | Label key → English | Value shown | Edits |
|---:|---|---|---|---|
| 1 | choice | `0xA9A76BBA` Resolution | literal `640x480`, `800x600`, `1024x768`, `1280x960`, `1280x1024`, `1600x1200` for index 0…5 (functional text, ASCII) | `g_RacingResolution` |
| 2 | slider 1…5 | `0xB55E7665` Level Of Detail | slider only | `g_PerformanceLevel` |
| 3 | button | `0x381FE1E9` Customize Display Settings | — | opens Customize |
| 4 | button | `0x13F0EDEB` Default Settings | — | opens the confirmation (§6) |

All four rows are always present (no cap condition). Changing Level Of Detail also refreshes the
Resolution row's value text (the preset may change the resolution). For a resolution index outside
0…5 the value text is not updated. [static]

### 3.2 Frontend variant (`UI_Options.fng`)

* Title object `Header_Text` (name hash `0x42ADB44C`, absolute (−289.7, −220), scale 0.8, lime
  (202,244,138)) shows `0x4204D527` "Display". [static; data]
* The screen hides, for this page: `ColorCal_Logo`, `ColorCal_Blurb`,
  `Colour_Calibration_Backing`, `CAR_STATS`, `JOY_EVENT_BUTTON1`, `PLAYER` and the
  "Player Select" text (`0xC90DC4AB`). [static]
* Panels [data]: outer `U2_Menu_Box` 562×412 centred (−1, −17) plus rounded corners; inner light
  panel `U2_Menu_Box_White` 548×364 centred (−1, 1), alpha 200. Measured light panel on the Audio
  page x −291.1…+289.0, y −181.4…+182.4. [observed]
* Help text key for the PC help popup: `0xED586157` (`0xDA850671` while an online session is
  active [inferred]). Both read "Move Highlight Up / Down · Change Selection · … · Back to Previous
  Menu". [static; data]

### 3.3 In-game variant (`UI_PauseOptions.fng`)

* `Header_text` shows `0x1D7BB6C9` "PAUSE OPTIONS" (left, (−287.7, −208.3), scale 0.75);
  `Header_text2` shows `0xEF1BB23B` "DISPLAY" (right-aligned, (288.3, −208.3)); `Back_button` shows
  `0x933DD695` "Back". The code also sets a `Select_button` (`0x8202C364` "Select"), which this
  package does not contain. [static; data]
* `Back_button`, its group and the `$JOY_EVENT_FENG_CANCEL$` glyph text are console-only objects
  (flag `0x40`); on PC they are not drawn. [data; FEngLib flag meaning]
* Panel: black `U2_Menu_Box_Black` 562×436 centred (1, 4) with black rounded corners, light inner
  panel 548×362 at alpha 100, and a lime outline (`U2_Highlight_box`/`U2_Highlight_Corner`, colour
  (197,218,165)). No row backings (§2.1). [data]
* In-game texts (name/value) are lime (202,244,138), scale 0.75. [data]

## 4. Customize screen (`UI_PC_Customize_Options.fng`)

### 4.1 Rows [static]

Rows are added in this fixed order. A row is present **only if the option's capability maximum
(SPEC §4.3) is non-zero**; absent rows leave no gap (later rows move up and take the next slot).
Vsync is always present.

| Order | Kind | Label key → English | Present if cap of | Values |
|---:|---|---|---|---|
| 1 | slider 0…cap | `0x7D3930E7` Car Reflection Update Rate | CarEnvironmentMap | — |
| (1a) | spacer | — | inserted directly after row 1 when the localized row-1 label contains `^` (forced line break) | — |
| 2 | choice | `0x8FE9288E` Car Reflection Detail | CarEnvironmentMapUpdateData | 0 `0xB55E7678` Low, ≠0 `0x612AF8A6` High |
| 3 | slider | `0xA7A683AC` Car Shadow / Neon | CarShadow | — |
| 4 | choice | `0xCFBEDB50` Car Headlight | CarHeadlight | Off/On |
| 5 | choice | `0x4AC50BCF` Car Geometry Detail | CarLod | Low/High |
| 6 | choice | `0x06CC3750` Crowds | Crowd | Off/On |
| 7 | slider | `0xBDE2E94B` World Detail | WorldLod | — |
| 8 | slider | `0x2A2A4AB3` Road Reflection Detail | RoadReflection | — |
| 9 | choice | `0x6F2C7417` Light Trails | LightStreaks | Off/On |
| 10 | choice | `0x62925361` Light Glow | LightGlow | Off/On |
| 11 | choice | `0x2522A4E9` Particle System | ParticleSystem | Off/On |
| 12 | choice | `0x822E4E9B` Motion Blur | MotionBlur | Off/On |
| 13 | choice | `0x95B8F38D` Fog | Fog | Off/On |
| 14 | choice | `0xA9C27DFD` Depth Of Field | DepthOfField | Off/On |
| 15 | slider | `0x4CD775AC` Full Screen Anti-Aliasing | FSAA | — |
| 16 | choice | `0x11504D0E` Tinting | Tinting | Off/On |
| 17 | choice | `0x4E57D355` Horizon Fog | HorizonFog | Off/On |
| 18 | choice | `0xD6F5F38C` Over Bright | OverBright | Off/On |
| 19 | choice | `0x571F6C00` Enhanced Contrast | BleachByPass | Off/On |
| 20 | choice | `0x4CDDBEBB` Rain Splatter | Rain | Off/On |
| 21 | slider | `0x61A50B8F` Texture Filtering | TextureFiltering | — |
| 22 | choice | `0x5B4AA6E4` Vsync | always | 0 `0x88576CFF` Off, ≠0 `0x0000063C` On |

"Off/On" = 0 → `0x0000CCFA` "Off", ≠0 → `0x0000063C` "On". Fog uses the same pair. On a fully
capable adapter all 22 rows are shown, needing 22 > 8 visible rows, so this screen scrolls.
Vectors: `tvg02_rows.json`, `tvg05_value_text.json`.

The spacer case: in English the label has no `^`, so no spacer. When present, the spacer uses the
next slot, is non-selectable (skipped by Up/Down), and row 1's label is offset to make room for two
text lines. [static]

### 4.2 Other content

* `Header_Text` shows `0x381FE1E9` "Customize Display Settings". `PLAYER` is hidden. [static]
* No colour-calibration objects exist in this package; panel art and anchors as in
  `UI_Options.fng` (§2.3). [data]
* The same package is used in the front end and in game (§1.3). [data; static]

## 5. Leaving the screens, persistence, re-initialisation

### 5.1 Back from Display [static]

Back starts the exit animation of the screen's event-handler object (name hash `0x47FF4E7C`,
script `Forward`); when it completes:

1. the graphics values are written to the registry (SPEC §4.4), then
2. the screen switches to `UI_OptionsMain.fng` (front end) or `UI_PauseOptionsMain.fng` (in game).

A second Back while the exit animation runs is ignored.

### 5.2 Start in game [static]

On the in-game Display screen, Start closes `UI_PauseOptions.fng`, `UI_PauseOptionsMain.fng` and
`UI_Pause.fng` and resumes the game **without** writing the registry. Values changed in this visit
stay in effect for the session but are not persisted by this path (they are persisted by any later
Back from a Display screen or the other write paths of SPEC §4.4).

### 5.3 Customize [static]

Accept on "Customize Display Settings" plays the same exit animation and switches to Customize.
Back from Customize returns to the Display package of the current context (§1.2). Neither
transition writes the registry; writing happens when Display is left with Back.

### 5.4 Display re-initialisation timing [static] — amends SPEC §4.4

Every change that SPEC §4.4 lists as needing re-initialisation raises a flag immediately in the
row handler; the renderer services the flag **at the start of the next frame**, i.e. while the
options screen is still open. It is not deferred to leaving the screen. Changes that do not need
re-initialisation take effect when the next frame is drawn.

### 5.5 Re-initialisation failure [static; data]

If resetting the display fails, the game sets FSAA to 0, retries up to 3 times, then lowers the
resolution index one step at a time until a reset succeeds or index 0 is reached (SPEC §4.4).
When a lowered resolution succeeds it asks `UI_Options.fng` to rebuild its rows (so the Resolution
row shows the new value). **No failure dialog or text is shown**: no message is posted to the user
and the language file has no such text. What happens when every attempt fails is not specified
here (§11). The rebuild request names only the frontend package, so the in-game Display screen is not
refreshed by it (its Resolution text may be stale until it is re-entered). [static]

### 5.6 Rebuild [static]

Default Settings confirmation (§6) and the failure path rebuild the Display list: all rows are
recreated from the current values, focus goes to row 1, `top = 1`. Entering Display from
Customize also builds it fresh (frontend focus row 1).

## 6. "Default Settings" confirmation [static; data]

* Opened by Accept (or click) on the Default Settings row.
* Dialog package: `GenericDialog_SMALL.fng` (key `0x6962C0CD`, global, available in both
  contexts). Message `0x2BBCCA6B` "Reset to default settings?". Buttons: first `0x0000F770` "Yes"
  (object `button1`, left, centre x −150), second `0x0000061C` "No" (`button2`, right, x +150).
  Initial focus: the **first button ("Yes")**. [static: default index 0; data: button positions]
* Yes: run auto-detect (SPEC §4.2), request display re-initialisation (§5.4), rebuild the Display
  list (§5.6). The registry is not written at this point (only on Back, §5.1).
* No, or cancel/Back: nothing changes; the Display screen keeps its focus.
* The list engine plays `UNDIM` on the screen when a dialog closes; the screen behind is assumed
  to play `DIM` while the dialog is open (authored 100 ms scripts exist). [static for UNDIM;
  inferred for DIM]
* The dialog's own presentation and input (button layout, focus movement, cancel) are those of the
  generic small dialog; see the pause-menu QUIT_DIALOG work for that package family. Whether Left/Right
  wraps between the two buttons was not examined.

## 7. Hint bar and help

* Front end: the screens show the shared PC help bar (`UI_PC_Help_Bar.fng`, in `GLOBALB`). On the
  retail Audio page (same screen and package as frontend Display) it shows two pills: lower
  "Quit Game", upper "Back"; no Help pill and no key glyphs. [observed] The Display and Customize
  screens are expected to show the same two pills. [inferred]
* In game: the help bar has a dedicated `PAUSE_OPTIONS` layout script (moves pills 1–3; others
  hidden), selected by help-bar message `0xABACCCC5`. Which labels the pills carry in game, and
  what selects that layout, are unknown. [data; trigger unknown]
* The `Help`, `$JOY_EVENT_FENG_HELP$`, "Player Select" and `$JOY_EVENT_FENG_BUTTON1$` objects in the
  packages are console-only (flag `0x40`) and not drawn on PC. [data]
* Help popup text keys: Display front end `0xED586157` (§3.2); others unknown.

## 8. Static object inventory (non-row) [data]

Absolute centres from the authored `Init` scripts (parent offsets summed). Full lists in analyst
evidence; the main items:

| Object | `UI_Options.fng` | `UI_PauseOptions.fng` | `UI_PC_Customize_Options.fng` |
|---|---|---|---|
| title text | `Header_Text` (−289.7,−220) s0.8 | `Header_text` (−287.7,−208.3) s0.75 + `Header_text2` (288.3,−208.3) | `Header_Text` (−289.7,−220) s0.8 |
| outer panel | 562×412 at (−1,−17), `U2_Menu_Box` | 562×436 at (1,4), `U2_Menu_Box_Black` | 562×412 at (−0.7,−18.3) |
| inner panel | 548×364 at (−1,1), white, α200 | 548×362 at (1,9), α100 | 548×364 at (−1,1), α200 |
| scroll track | x 273, y 2, 8×282 | x 275, y 10, 8×282 | x 273, y 2, 8×282 |
| scroll arrows | (273,−164) / (273,167), `U2_Menu_Arrow.tga` | (275,−154) / (275,175), `U2_Menu_Arrow_White.tga` | (273,−164) / (273,167) |
| bottom strip (name/cash, `U2_Menu_Box`) | y 306 | — | y 306 |

Fonts: all texts use `ConduitMdITC_TT21i.ffn`.

## 9. Label key → widget map (all graphics UI text)

| Key | English | Where |
|---|---|---|
| `0x4204D527` | Display | frontend Display title |
| `0x1D7BB6C9` | PAUSE OPTIONS | in-game title (left) |
| `0xEF1BB23B` | DISPLAY | in-game title (right) |
| `0x381FE1E9` | Customize Display Settings | Display row 3; Customize title |
| `0x13F0EDEB` | Default Settings | Display row 4 |
| `0x2BBCCA6B` | Reset to default settings? | confirmation message |
| `0x0000F770` / `0x0000061C` | Yes / No | confirmation buttons |
| `0x933DD695` | Back | in-game back text (console-only object) |
| `0xA9A76BBA` | Resolution | Display row 1 |
| `0xB55E7665` | Level Of Detail | Display row 2 |
| §4.1 keys | option labels | Customize rows |
| `0x0000063C` / `0x0000CCFA` / `0x88576CFF` | On / Off / Off (Vsync) | toggle values |
| `0xB55E7678` / `0x612AF8A6` | Low / High | detail values |
| `0xED586157` | help text | frontend Display help popup |

## 10. Amendments to SPEC.md (accepted and applied by the reviewer; SPEC_GUI supersedes SPEC §4.4 wherever they differ)

1. §4.4 "it happens when leaving the screen" → re-initialisation is requested immediately and
   serviced at the start of the next frame (§5.4). [static]
2. §4.4 "a failure message is shown" → no message is shown; the frontend Display list is rebuilt to
   show the lowered resolution (§5.5). [static; data]
3. §4.4 "Toggles flip … only if their cap is non-zero" — additionally the whole row is omitted
   when the cap is zero (§4.1), so the condition is not reachable through the menu. [static]
4. §4.4 write paths: leaving the in-game Display screen with Start resumes without writing (§5.2).
   [static]

## 11. Unknowns / ambiguities

1. Exact pixel offsets of name, value text, arrows and slider inside a row relative to the anchors
   (given only as measurements of the frontend Audio page, §2.3); in-game and Customize assumed
   equal.
2. Scrollbar tab end positions and size; whether clicking scrollbar arrows or the track does
   anything; mouse wheel; hover-to-focus on rows.
3. Help-bar pill labels in game and the trigger of its `PAUSE_OPTIONS` layout (§7).
4. Initial focus of the in-game Display screen (remembered index semantics).
5. Whether the Default Settings dialog's Left/Right wraps, and its exact default focus at runtime
   (code passes index 0 = first button).
6. Exact meaning of the "online session" condition that swaps the frontend help text key.
7. Authored animation timings are data; their evaluation (track interpolation) follows the base GUI
   semantics already used for the pause menu and was not re-verified for these packages.
8. Whether any locale's "Car Reflection Update Rate" label contains `^` (spacer row).
9. Outcome when every display-reset attempt fails (resolution already 0).
10. Whether a mouse click on a row label counts as button activation (row labels carry the
    authored button flag, so it is likely). [inferred]

## 12. Capture requests (retail runtime)

1. Stills of frontend Display and Customize (top, scrolled to the bottom), and the in-game
   Display and Customize screens, mouse visible but not over the list.
2. Video: hold Down through Customize from top to bottom and press Down again at the end; then Up at
   the top (confirms §2.5 and the tab motion).
3. Still of the Default Settings dialog right after it opens (default focus) and after Left/Right.
4. Video: change Resolution on the Display page (confirms the immediate re-init, §5.4).
5. In game: press Start on the Display page after a change, quit, and export the registry key
   (confirms §5.2).

## 13. Test vectors

`test-vectors-gui/README.md`. All synthetic except `tvg01` metadata (observed from retail data,
no payload).
