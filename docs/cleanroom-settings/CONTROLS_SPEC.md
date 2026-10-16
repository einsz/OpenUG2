# Controls (PC controller configuration) screen — behavioural specification

Status: draft for review. Companion to `SPEC.md` (settings storage), `SPEC_GUI.md`,
`SPEC_GUI_ART.md` and `SPEC_GUI_ART_AMEND1.md` (option-screen engine, authored-package
evaluation, dialogs; the last three live on branch `settings-gui`). This document covers the
retail PC **Controls** screen: how it is reached, what it shows, how it is driven, the bindable
action list, default bindings, how bindings are captured, displayed and stored, and where an
OpenUG2 implementation hooks in. It identifies every visual element and text by an original
resource key so that **no new art, font, string or layout is needed**.

Provenance: static examination of the retail executable `speed2.exe` (sha256
`f9dd86c0…24168a7`, 4 800 512 bytes, same binary as `SPEC.md`); the original 2004 retail data
files `FRONTEND/FRONTB.LZC.bacc`, `GLOBAL/GLOBALB.BUN`/`GLOBALB.LZC.bacc`,
`GLOBAL/InGameCommon.bun`, `LANGUAGES/English.bin`, `LANGUAGES/Labels.bin`; public DirectInput 8
documentation (key codes, joystick state offsets, action-mapping records); public community
reports on keyboard-binding persistence (cited in §6.6). **The game was not run** (standing user
instruction from the earlier GUI work); there are no runtime captures, and the test vectors are
synthetic (README in `test-vectors/controls/`).

Confidence tags: **[data]** read from retail data files; **[static]** determined by examining
the executable, not confirmed at runtime; **[public]** from public documentation;
**[inferred]** one reasoning step from the above; **[guessed]** plausible, untested.

Identifiers. Package names (`UI_Options_PC_Controller.fng` …), object names and name hashes,
script names, FEng message hashes, texture names/keys, language label keys, action ids and
DirectInput key codes are **data lookup keys** and are flagged as functionally required. The few
literal display strings quoted (`---`, `Left Shift`, `Left Alt`, `Left Ctrl`, `Button `) are
functionally required display text that the executable itself produces (§6.2). No original
code identifiers are used.

Hashes. Object, script, message, package and texture keys: `h = 0xFFFFFFFF; for each byte c:
h = h*33 + toupper(c) (mod 2^32)` (as in SPEC_GUI). Language label keys: the same recurrence
**without** case folding over the label *name* (`SPEC.md` header). `LANGUAGES/Labels.bin`
holds the label names; hashing a name gives the key under which `English.bin` stores the text
[data: 4 048 of 4 519 names in `Labels.bin` resolve this way]. An implementation only needs the
key → text table of `English.bin` (already loaded by `src/ugui` as `UgStrTab`).

Coordinates: centred 640×480 virtual units, origin at the screen centre, x right, y down
(SPEC_GUI_ART_AMEND1 A1). "Top-left" and "centre" as defined there.

### Normative resource rule (reviewer-added; binds the implementation)

**R1.** The implementation MUST NOT create any new resource for this screen: no new art, texture,
font, layout, package, string, sound or localisation entry. Every visual element and every text
MUST come from the original retail resources identified in this document by key or file name
(`UI_Options_PC_Controller.fng` and its textures and font, `GenericDialog_SMALL.fng`,
`UI_PC_Help_Bar.fng`, `OPTIONS_ICON_CONTROLS`, and `English.bin`/other language files by label key).
**R2.** The only literals the implementation may produce itself are the functionally required
display strings listed under "Identifiers" above (`---`, `Left Shift`, `Left Alt`, `Left Ctrl`,
`Button n`) and platform-provided key/button names (D2). **R3.** If an original resource is
missing at run time the feature MUST be unavailable (entry hidden/disabled), never substituted by a
newly authored one. **R4.** Retail asset bytes MUST NOT be copied into the repository; they are read
from the user's own game files at run time.

---

## 1. What OpenUG2 has today and what is missing

Baseline on the current branch (`settings-controls`, = `development`) [observed in the tree]:

* `src/settings.h` / `settings.c`: profile container and payload, including the six 181-byte
  input-device records (`StInputDevice`: GUID, four axis ids, four directions, four "assigned"
  flags, combined-pedal mode, `binding[32]`) and `st_input_find()`. No defaults for bindings, no
  runtime use.
* `src/pause_menu.c` + `src/ugui/*`: authored-package evaluation (`UgFngPackage`, `UgScene`,
  `ug_scene_*`, `UgAssets`, `ug_assets_package()`, `ug_assets_package_by_key()`,
  `ug_strtab_get()`), loading `FRONTEND/FRONTB.LZC.bacc`, `GLOBAL/GLOBALB.*`,
  `LANGUAGES/<lang>.bin` and, for in-game use, `GLOBAL/InGameCommon.*`
  (`ug_assets_open_ingame()`).
* `src/main.c`: driving input is hard-wired to SDL scancodes (W/S throttle/brake, A/D steer,
  Space handbrake, N nitrous, B look back, C camera, Escape pause) read through
  `SDL_GetKeyboardState()` in the per-tick driving block and through the `SDL_KEYDOWN` dispatch.
  No binding layer exists.

On branch `settings-gui` (not merged into this branch) [observed: headers and docs only]:
`src/options_menu.h` (`OptionsMenu`, `om_open_hub()`, `OmHubEntry`, `OM_HUB_ENTRIES 4`, list
engine `OmList`/`om_list_move()`, dialogs `GuiDialog`), `src/gui_view.*`, `src/gui_carousel.*`,
`src/gui_dialog.*`, `src/pause_screen.*`: the in-game options hub (Audio, Display, Gameplay,
Player) and the Display/Customize screens.

Missing for Controls: the Controls entry/route, the three-column binding screen, the device
selector, the capture ("press a key") state, keyboard and joystick binding models with retail
defaults, the reset-to-defaults flow, display names of keys/buttons/axes, persistence, and a
binding layer between SDL input and game actions.

## 2. Screen graph and packages

### 2.1 Retail route [static]

```
UI_Main.fng ──Options──▶ UI_OptionsMain.fng (front-end options hub, icon carousel) [inferred link]
                            │ entry "Controls" (Accept / click)
                            ▼  (hub exit script, then switch at exit-complete)
                         UI_Options_PC_Controller.fng   ◀── the Controls screen
                            │ Back
                            ▼  (immediately, §3.9)
                         UI_OptionsMain.fng
```

Front-end options hub entries, in order [static; labels data]. Icon textures are in
`GLOBAL/GLOBALB` (Audio…Controls) or `FRONTEND/FRONTB` (EA TRAX, Credits); the page id is what
the entry stores before the hub's exit script runs, and selects the next package at
exit-complete:

| # | Present | Icon texture key (name) | Label key → English | Page id → next package |
|---|---|---|---|---|
| 1 | always | `0xF37AF144` (`OPTIONS_ICON_AUDIO`) | `0xE76CD783` Audio | 0 → `UI_Options.fng` |
| 2 | always | `0x8A006328` (`OPTIONS_ICON_DISPLAY`) | `0x4204D527` Display | 1 → `UI_Options.fng` |
| 3 | always | `0x4DF98FB2` (`OPTIONS_ICON_GAMEPLAY_S…`) | `0xD0CF6EE1` Gameplay | 2 → `UI_Options.fng` |
| 4 | always | `0xD708EFEF` (`OPTIONS_ICON_PLAYER_SET…`) | `0xF760EABE` Player | 3 → `UI_Options.fng` |
| 5 | always | `0xA04A7B26` (`OPTIONS_ICON_CONTROLS`) | `0x04DC6DB5` **Controls** | 4 → **`UI_Options_PC_Controller.fng`** |
| 6 | condition C | `0xC52CCBF6` (`OPTIONS_ICON_EA_TRAX`) | `0xDCFB6B36` EA GAMES TRAX | 5 → `EA_Trax_Jukebox.fng` |
| 7 | condition C | `0x51009E20` (`OPTIONS_ICON_CREDITS`) | `0x0905101F` Credits | 8 → `Credits.fng` |
| 6′ | not C | `0x83070A37` (`OPTIONS_ICON_ONLINE`) | `0x7D347E05` Online Options (`0x19889765` LAN Options in LAN mode) | 7 → `UI_Options.fng` |

Condition C is a front-end state value below 12 (meaning not determined; inferred "not in an
online/LAN session"). The hub uses the same carousel engine as the pause options hub
(SPEC_GUI_ART §3) with package `UI_OptionsMain.fng` (key `0x1E74C936`, `FRONTB`, objects
`option_1…option_11`, `OPTION_MASTER`, `ICON_TITLE`, `Header_Text` = `0xC7CA52ED` "Options").
Back from the hub: if a profile is loaded and it has unsaved changes (SPEC §3.6) the save
prompt/autosave flow runs, otherwise the screen returns to the screen it was entered from
[static; "entered from" inferred].

### 2.2 In game [static]

There is **no** retail in-game route to the Controls screen: the pause options hub
(`UI_PauseOptionsMain.fng`, SPEC_GUI_ART §2.5) has exactly Audio, Display, Gameplay, Player, and
`UI_Options_PC_Controller.fng` exists only in `FRONTEND/FRONTB.LZC.bacc` (not in
`InGameCommon`). OpenUG2 currently has only the in-game route; see decision D1 (§10).
OpenUG2's in-game asset set already contains `FRONTB` (`ug_assets_open_ingame()` loads it), so
the package and all its textures are available in game [observed in `ugui_assets.c`; data].

### 2.3 Sub-screens and states [static]

The Controls screen is a single package. Everything else is a state of it or a dialog over it:

| State | How entered | Leaves to |
|---|---|---|
| **Browse** (a binding cell is focused) | entry; Down from Selector; end of capture | Selector (Up on row 1), Capture (Accept/click), Dialog, hub (Back) |
| **Selector** (device title row focused, no cell focused) | Up on row 1; pointer over the title | Browse (Down), device change (Left/Right) |
| **Capture** ("press a key / button") | Accept or click on a focused cell | Browse after a key/button/axis/POV or Escape |
| **Reset dialog** (`GenericDialog_SMALL.fng`) | PAD_BUTTON0 message (help-bar pill "Reset Keys To Default") when not capturing | Browse |

There is no separate per-device sub-screen: changing the device rebuilds the rows in place
(§4.2). There is no force-feedback/vibration, deadzone, sensitivity or player-1/player-2 choice on
this screen; those live on the Player options page and in `UI_Wheel_Options.fng` (out of scope).
`UI_OptionsController.fng` (also in `FRONTB`) is a console layout that the PC executable never
names; per-device-class package names the executable carries match no package in the data and
are not reachable. None of these is needed.

### 2.4 Packages used

| Package | Key | Container | Decoded bytes | Role |
|---|---|---|---:|---|
| `UI_Options_PC_Controller.fng` | `0xE0EE65E6` | `FRONTEND/FRONTB.LZC.bacc`, chunk `0x00030210` (JDLZ) | 150 288 | the screen |
| `UI_OptionsMain.fng` | `0x1E74C936` | same | 36 220 | front-end hub (§2.1) |
| `GenericDialog_SMALL.fng` | `0x6962C0CD` | `GLOBAL/GLOBALB.BUN`, `FRONTB` | 18 112 | reset confirmation (§4.6) |
| `UI_PC_Help_Bar.fng` | `0x33AC1CB4` | `GLOBAL/GLOBALB.BUN`, raw chunk `0x00030203` | 50 944 | PC help-bar pills (§3.7) |

[data; sha256 of the decoded Controls package in `tvc09_package.json`.]

## 3. Screen content (`UI_Options_PC_Controller.fng`)

Package resources [data]: `U2_Menu_Box.tga`, `U2_Menu_Text_Corner.tga`, `U2_Menu_Arrow.tga`,
`ConduitMdITC_TT21i.ffn`, `U2_Scroller_Corner.tga`, `U2_Scroller_Tab.tga`,
`U2_Menu_Box_White.tga`, `U2_Menu_RoundedCorner.tga`, `U2_Menu_Corner_white.tga`, plus the
untextured image. All texture keys (hash of the base name) are present in `GLOBAL/GLOBALB`
[data; keys in `tvc09`]. No object carries the console-only flag `0x40` [data]. All drawing is
the authored package; the application only sets labels/text, positions, visibility and starts
scripts (SPEC_GUI_ART §1.2–1.3).

### 3.1 Objects and what the application does with them [data for objects; static for use]

| Object (name or name hash) | Stored centre / size | Application use |
|---|---|---|
| `Header_text` (`0x42ADB44C`, child of group `0x8D7E0C53` at (−42,−157)) | absolute (−289.7,−220), scale 0.8, lime | label := `0x04DC6DB5` "Controls" (authored default `0xC7CA52ED` "Options") |
| `0x4E11CF90` (text) | (−5,−169), scale 0.7, centred | device-selector title, label set per device class (§3.2) |
| `0xD1CDA594` (`U2_Menu_Arrow.tga`) | (−266,−158) 24×24 | selector "previous device" arrow |
| `0x9345E4A7` (`U2_Menu_Arrow.tga`) | (265,−158) 24×24 | selector "next device" arrow |
| `0x666593C4` (text) | (9,−138), scale 0.7 | label := `0x1B15F53C` "Primary" |
| `0x59ABA1E8` (text) | (172,−138), scale 0.7 | label := `0x37E740E0` "Secondary" |
| `Instructions_Label` | (−5,153), scale 0.7, centred | instruction line (§3.5) |
| `TITLE_MASTER` | (−187,−88) 180×36 | anchor of the action-name column (never drawn) |
| `DATA_MASTER` | (11,−90) 150×24 | anchor of the Primary column |
| `DATA_MASTER2` | (173,−90) 150×24 | anchor of the Secondary column |
| `option_name_n`, n = 1…20 | parked off-screen | action label of row n |
| `OPTION_DATA_n`, n = 1…20 | parked | Primary binding text of row n (centre-justified) |
| `OPTION_DATA_2_n`, n = 1…20 | parked | Secondary binding text of row n (centre-justified) |
| `Options_Backing_n` (group, scale (1, 0.8)), n = 1…20 | parked | row backing (`U2_Menu_Box` + two `U2_Menu_Text_Corner` caps) |
| `ScrollBar_Backing` | (273,20) 8×242 | scroll track |
| tab `0x3FBB7A4E` (`U2_Scroller_Tab.tga`) | (273,−50) 16×16 | scroll tab |
| `ScrollBar_Arrow_1` / `ScrollBar_Arrow_2` | (273,−126) / (273,166) 24×24 | scroll arrows |
| event handler `0x47FF4E7C` | off-screen | `CALL` / `FORWARD` scripts (§3.9) |
| panels: group `0x0DE769B5` (outer `U2_Menu_Box` 562×412 at (−1,−18) with rounded corners), group `0x371F1C93` (inner white panel and corners, alpha 200), scroller corner pieces | as authored | none (static decoration) |

Object names are matched case-insensitively (the package mixes `option_data_1` and
`OPTION_DATA_2_1`). A `DONE_BUTTON` object is looked up but absent [data], so the "done" focus
target of the list engine never exists.

### 3.2 Device-selector row [static]

The title `0x4E11CF90` shows the class of the current device:

| Device class | Title label key → English |
|---|---|
| keyboard | `0x5CF08EB7` Keyboard Configuration |
| gamepad / joystick | `0xF91FBA4D` Joypad Configuration |
| wheel | `0x14D4BD3B` Wheel Configuration |

The device's own name is never shown; two gamepads are indistinguishable except by their
bindings. The arrows are hidden when exactly one device exists, otherwise shown and set to their
`Static` script; on a device change the arrow of the direction plays `Blink` [static].

### 3.3 Column headers [static]

`0x666593C4` ← `0x1B15F53C` "Primary"; `0x59ABA1E8` ← `0x37E740E0` "Secondary". Fixed.

### 3.4 Rows [static]

Row n (1-based, list order) binds `option_name_n` (label), `OPTION_DATA_n` (Primary cell),
`OPTION_DATA_2_n` (Secondary cell) and `Options_Backing_n` (backing of the row, driven together
with the label). Row contents per device class: §5. The label text of row n is its label key;
the two cell texts are binding names (§6.2). The label is never focusable; both cells are.

### 3.5 Instruction line [static]

| Moment | `Instructions_Label` label → English |
|---|---|
| screen built | `0x8BAB66EF` Configure your settings |
| capture started | `0x23CF49AA` Press a button to use for this function |
| first frame after a capture ended (assigned or cancelled) | `0xFED0D3DF` Press enter to assign a new key or button |

The third text then stays until the screen is left (it is not reset to the first).

### 3.6 Scrollbar [static; geometry data; formula inferred from SPEC_GUI_ART §4.5]

Same engine as the option lists: shown when the list has more than one row; the tab follows the
focused row, `tab_y = rail_top + 8 + (rail_h − 16)·(c − 1)/(N − 1)` with the rail =
`ScrollBar_Backing` (top −101, height 242) → −93 … +133. N = number of rows, c = focused row.
The tab is not moved while the selector row is focused. Up/Down make the package play `BLINK`
on `ScrollBar_Arrow_1`/`_2` (authored package responses).

### 3.7 PC help bar [static; labels data]

The screen's help-bar flags select three pills of `UI_PC_Help_Bar.fng`, assigned in this order
(pill 1 = lowest, as on the retail Audio page, SPEC_GUI §7):

| Pill | Label key → English | Click posts |
|---|---|---|
| 1 | `0x1C101778` Quit Game | quit-game flow (as on other front-end screens) |
| 2 | `0x1C0787E6` Back | Back `0x911AB364` |
| 3 | `0xB7D0D273` Reset Keys To Default | `0xC519BFBF` (PAD_BUTTON0) → reset dialog (§4.6) |

(The other options screens carry only pills 1–2.) [pill order inferred from bit order; the
retail screen was not observed]

### 3.8 Layout [static; numbers from data]

Anchors are read once when the screen is built, as top-left + size (SPEC_GUI_ART_AMEND1 A6):
T = `TITLE_MASTER` → top-left (−277,−106), 180×36; D1 = `DATA_MASTER` → top-left (−64,−102),
150×24; D2 = `DATA_MASTER2` → top-left (98,−102), 150×24.

* Visible rows V = **7**. Row pitch = T height = **36**.
* Visible line k (0…6) has top `y_k = −106 + 36·k` for **all three columns** (the data columns
  use T's y, not their own anchors' y).
* Label: top-left (−277, y_k). Primary text: centre x = −64 + 75 = **11**, top y_k. Secondary
  text: centre x = 98 + 75 = **173**, top y_k. (Text objects are placed with the "set top-left"
  rule; centre-justified texts get x + width/2.)
* Row backing group position: (−277 + 276, y_k + 8) = (−1, −98 + 36·k); its children keep
  their authored offsets and the group's (1, 0.8) scale (AMEND1 A3).
* Rows outside the window are hidden and moved to y = 6969 (parked).
* Hit rectangle of a cell (pointer): top-left (column x, y_k), size 150×24 (D1's size), edges
  inclusive; hidden cells never hit.

Initial window: top = row 1. With more than 7 rows the window scrolls minimally (§4.4).

### 3.9 Scripts and timing [data; use static]

* Enter: handler `Init` chains to `CALL` (500 ms): `0xBDFD2DF9` (FadeIn broadcast) at 0 ms,
  `INIT_COMPLETE` (`0x35F8620B`) to the application at 320 ms. Objects fade in over 180 ms.
* Exit: handler `FORWARD` (500 ms) posts `0x84378BEF` and exit-complete `0xE1FDE1D1` to the
  application **both at 0 ms**, and no fade-out broadcast. So Back switches to
  `UI_OptionsMain.fng` at once, without a fade-out of this screen. The handler starts `FORWARD`
  on Back (`0x911AB364`) and on button activation (`0x0C407210`) only if `FORWARD` is not
  already running; `0xCB1AA27F` runs `Back` (0 ms, no effect); `0xC2A825E8`/`0x18BFC04B` run
  `Dim`/`UNDIM` (100 ms).
* Cells and labels: `HIGHLIGHT` (1 200 ms looping pulse) on focus, `UnHighlight` on blur, data
  cells also `pulse` (1 200 ms loop, started by the application as `Pulse`) while capturing;
  base text colour dark grey (50,50,50); title base lime (202,244,138). Arrows: `BLINK` 400 ms →
  `STATIC`. All row objects start hidden (`Init → hide`) and appear through the FadeIn broadcast.
* Package key responses [data]: Up/Down → application + `BLINK` on the scroll arrows; Left,
  Right, `PAD_BUTTON0` `0xC519BFBF`, `PAD_BUTTON1` `0xC519BFC0`, `PAD_BUTTON4` `0xC519BFC3` →
  application; Back and `0x14DD0705` → application + handler.

## 4. Behaviour

### 4.1 Entering [static]

1. Build the device list (§4.2). The current-device index is **not** reset (see quirk Q1).
2. Hide or show the selector arrows (§3.2); `UnHighlight` the selector title.
3. Set header, column-header and instruction labels (§3.1, §3.5).
4. Build the **keyboard** rows (§5) regardless of the current-device index; title := Keyboard
   Configuration.
5. Place the window (top = 1), refresh every cell text (§6.2).
6. Focus = row 1, Primary cell (cell and its row label run `Highlight`); scrollbar at row 1.
7. At `INIT_COMPLETE` (320 ms) the focused cell is highlighted again.

### 4.2 Device list and selector [static]

* The list holds every input device the game has opened, in device-slot order; the system
  keyboard is always present and first [inferred: it occupies slot 0]. Classes: keyboard;
  gamepad or joystick (DirectInput types joystick/gamepad); wheel (DirectInput driving type).
* Left / Right while the **selector** is focused: previous / next device, **wrapping** at both
  ends, skipping empty slots. If the device changed: the arrow of that direction `Blink`s and the
  rows are rebuilt for the new device's class (old rows deleted, slot counter reset, rows added
  per §5, window top = 1, all texts refreshed, title set). Focus stays on the selector.
* With one device, Left/Right on the selector do nothing.
* Pointer click on a selector arrow = Left/Right on the selector, whatever is focused [static];
  not during Capture [inferred: menu input is disabled then].

### 4.3 Input map [static]

| Input | Browse (cell focused) | Selector focused |
|---|---|---|
| Up | previous row, same column (§4.4); from row 1 → Selector | nothing |
| Down | next row, same column (§4.4) | → row 1, **Primary** cell |
| Left | Secondary → Primary of the same row; on Primary nothing | previous device |
| Right | Primary → Secondary of the same row; on Secondary nothing | next device |
| Accept (`0x406415E3`) or activation (`0x0C407210`) | start Capture on the focused cell (§4.5) | nothing |
| Back (`0x911AB364`) | leave to `UI_OptionsMain.fng` (§4.7) | same |
| PAD_BUTTON0 (`0xC519BFBF`) | reset dialog unless capturing or in the frame after a capture (§4.6) | same |

Left/Right/Up/Down in Browse play UI sounds (not specified). On PC the keyboard menu keys are
the fixed menu map: arrow keys move, Enter (or keypad Enter) accepts, Escape is Back [static:
menu action table]. Which keyboard key, if any, produces PAD_BUTTON0 is unknown (U5); the
help-bar pill always does.

### 4.4 Focus and window [static]

* Navigation never wraps. Down on the last row and Up/Down within the selector do nothing
  (beyond the sound). All cells are selectable on this screen.
* After a move: if the new row ≥ top + 7, top += 1; if the new row < top, top −= 1.
* Focus change: old cell and its row label/backing `UnHighlight`; new cell and its row
  label/backing `Highlight`; scrollbar updated. Selector focus: the title runs `Highlight`;
  leaving it runs `UnHighlight`.
* Pointer hover over a visible cell focuses it (no scrolling); hover over the title focuses the
  selector. Vectors: `tvc07_navigation.json`.

### 4.5 Capture ("press a key / button") [static]

Start (Accept or click on a focused cell; activation of the cell object posts `0x0C407210`):

1. Remember (device, action of the cell, column 0 = Primary / 1 = Secondary) (§5).
2. Keyboard device: the event buffer is reset, so the key that started the capture is not
   taken [inferred]. Joystick: the axis values are snapshotted.
3. All menu input to the screen and to the help bar is disabled (Back, arrows and the reset
   pill do nothing until the capture ends).
4. The cell starts its `Pulse` script (keyboard Accept path only; a pointer click starts the
   capture without `Pulse`) and the instruction line shows `0x23CF49AA`.

While capturing, each frame (§6.3 keyboard, §6.5 joystick):

* **Escape** on the keyboard cancels: nothing changes. This holds for joystick captures too.
  Escape can therefore never be bound through this screen.
* Otherwise the first qualifying key / button / POV / axis motion is assigned and the capture
  ends.
* There is **no timeout** [static: no timer on this path]. Mouse buttons are not bindable.
* Other devices are ignored (a joystick button does nothing during a keyboard capture and
  keyboard keys other than Escape do nothing during a joystick capture).

End (detected on the screen's per-frame tick): the focused cell is highlighted again, every cell
text is refreshed, and on the **next** frame menu input is re-enabled and the instruction line
becomes `0xFED0D3DF`. Reset (PAD_BUTTON0) is refused while capturing and during that one frame.

### 4.6 Reset to defaults [static; labels data]

PAD_BUTTON0 (help-bar pill "Reset Keys To Default") opens `GenericDialog_SMALL.fng` with
message `0x2BBCCA6B` "Reset to default settings?", first button `0x00000639` "OK", second
button `0x541389E5` "Cancel"; presentation, initial focus (second button) and navigation as the
generic two-button dialog (SPEC_GUI_ART §6). OK → the **current device** is reset (keyboard:
§6.1 defaults; joystick: §6.4 defaults incl. axes) and all cell texts refresh. Cancel/Back →
nothing. Because of Q1 the "current device" can differ from the device whose rows are shown.

### 4.7 Leaving [static]

Back starts the handler exit; exit-complete arrives in the same frame (§3.9) and the screen
switches to `UI_OptionsMain.fng`. Nothing is written here. Joystick bindings live in the profile
(§6.6), so leaving the options hub afterwards triggers the normal unsaved-changes path
(SPEC §3.6, §5.8). If a capture was somehow still active when the screen is built again, it is
cancelled.

## 5. Action list and rows

### 5.1 Action ids [static]

Action ids are shared by the keyboard and joystick binding models and by the profile records.

| Id | Action | Row label key → English | Rows on | Notes |
|---:|---|---|---|---|
| 0 | none | — | — | empty binding |
| 1 | throttle | `0x409607E2` Throttle | K J W | axis-capable |
| 2 | brake / reverse | `0x82EBCC98` Back / Reverse | K J W | axis-capable |
| 3 | steer left | `0x153E7337` Steering (Left) | K J | axis-capable |
| 4 | steer right | `0xBD7BA1AA` Steering (Right) | K J | axis-capable |
| 5 | handbrake | `0xB3D1EA35` Handbrake | K J W | |
| 6 | nitrous | `0xDF5AEF26` Nitrous Boost | K J W | |
| 7 | shift up | `0x3EB318CE` Shift Up | K J W | |
| 8 | shift down | `0xB7D33921` Shift Down | K J W | |
| 9 | look back | `0x35B9F571` Look Back | K J W | |
| 10 | change camera | `0x419CA63A` Camera | K J W | |
| 11 | HUD toggle | — | — | keyboard default only, not on the screen |
| 12 | pause | `0x6F99F62A` Pause | K J W | |
| 14 | reset car | `0x6FC04E6F` Reset Car | K J W | |
| 15 | enter race / shop | `0x4A1341B3` Enter Race / Shop | K J W | |
| 16 | world map | `0x1E7E3391` World Map | K J W | |
| 17 | read SMS | `0x58BAEBBA` Read SMS | K J W | |
| 18 | career statistics | `0x735B2E6C` Career Statistics | K J W | |
| 19 | hydraulics bounce | `0x64730AAC` Hydraulics Bounce | K J W | |
| 22 | hydraulics pressure | `0xA081F0CC` Hydraulics Pressure | K J W | |
| 25 | (unidentified) | — | — | keyboard default only |
| 27–48 | menu actions (up 27, down 28, left 29, right 30, select 31, select-alt 32, help 33, back 34, customise 35, button 2 36, button 3 37, L1 38, R1 39, zoom in 44, zoom out 45, preset 46, quit 47, race options 48) | — | — | fixed menu maps, not on this screen |
| 49–62 | developer camera controls | — | — | keyboard table only, not on this screen |

K = keyboard list, J = joypad list, W = wheel list.

### 5.2 Row order [static]

Keyboard and joypad lists (18 rows): 1 Steering (Left) 3, 2 Steering (Right) 4, 3 Throttle 1,
4 Back / Reverse 2, 5 Shift Up 7, 6 Shift Down 8, 7 Handbrake 5, 8 Nitrous Boost 6, 9 Camera 10,
10 Look Back 9, 11 Enter Race / Shop 15, 12 World Map 16, 13 Read SMS 17, 14 Career Statistics
18, 15 Reset Car 14, 16 Hydraulics Pressure 22, 17 Hydraulics Bounce 19, 18 Pause 12.

Wheel list (16 rows): the same without the two steering rows (rows 3…18 above become 1…16).
The label `0x59C4937B` "Legend" exists in the language file but is not used; action 18 is
labelled "Career Statistics".

### 5.3 Columns [static]

Column 0 = Primary (`OPTION_DATA_n`), column 1 = Secondary (`OPTION_DATA_2_n`). For every cell the
capture target is (row action, column). Joypad and wheel cells are bound to the device that was
current when the rows were built.

### 5.4 Retail quirk Q2 — wheel "Career Statistics" secondary [static]

On the wheel list, row 12 (Career Statistics) Secondary **captures** action 18 column 1 but
**displays** action 17 column 1 (the Read SMS secondary). All other cells capture and display
the same (action, column). Vectors: `tvc02_rows.json`. See decision D5.

## 6. Bindings

### 6.1 Keyboard binding model and defaults [static; key codes public]

The keyboard uses an ordered table of entries, each **(action id, key)**; key 0 = unbound. Keys
are identified by DirectInput keyboard key codes (DIK = PC set-1 scan code, extended keys with
bit 7 set) [public]. The table's order is fixed; every on-screen action has **exactly two
consecutive entries**: the first is Primary, the second Secondary. Default table (state after
"Reset Keys To Default"):

| Action | Primary (DIK, expected English name) | Secondary |
|---|---|---|
| 1 Throttle | `0xC8` Up | `0x11` W |
| 2 Back / Reverse | `0xD0` Down | `0x1F` S |
| 3 Steering (Left) | `0xCB` Left | `0x1E` A |
| 4 Steering (Right) | `0xCD` Right | `0x20` D |
| 7 Shift Up | `0x2A` Left Shift | `0x27` ; |
| 8 Shift Down | `0x1D` Left Ctrl | `0x34` . |
| 5 Handbrake | `0x39` Space | `0xCF` End |
| 6 Nitrous Boost | `0x38` Left Alt | `0xB8` Right Alt |
| 11 (HUD, hidden) | `0x10` Q | `0x0D` = |
| 10 Camera | `0x2E` C | `0xD2` Insert |
| 9 Look Back | `0x30` B | `0x25` K |
| 12 Pause | `0x01` Esc | `0x19` P |
| 14 Reset Car | `0x13` R | `0xC7` Home |
| 15 Enter Race / Shop | `0x1C` Enter | `0x3B` F1 |
| 16 World Map | `0x32` M | `0x3C` F2 |
| 17 Read SMS | `0x0F` Tab | `0x3D` F3 |
| 18 Career Statistics | `0x14` T | `0x3E` F4 |
| 19 Hydraulics Bounce | `0x24` J | `0x16` U |
| 22 Hydraulics Pressure | `0x23` H | `0x15` Y |
| 25 (hidden) | `0x06` 5 | — (single entry) |

Rows appear in the table in the order shown; 14 single entries for actions 49–62 follow
(complete order and codes in `tvc03_keyboard_defaults.json`). Names are the expected English
Windows key names after the substitutions of §6.2 [inferred]; the SDL scancode for each DIK is
listed in `tvc03` [inferred mapping]. Reset algorithm: walk the default entries in order; for
each, take its key K; scanning the table from the start, clear K from every entry until the
target entry (the first, or for the second default of an action the second, entry with that
action) is reached, then store K there. With these defaults (no duplicate keys) the result is
exactly the table above.

Separate fixed keyboard maps exist for the menus and two other contexts (menu: arrows, Enter,
keypad Enter, Escape, and further keys for the remaining menu actions); the Controls screen
neither shows nor edits them, and rebinding a driving key does not change them [static].

### 6.2 How a binding is shown [static]

Cell text for (device, action, column):

* **Keyboard.** Walk the table entries of that action in order. Column 0: the first entry with a
  non-zero key. Column 1: skip the **first entry of the action** (whatever its key), then the
  first remaining entry with a non-zero key. The text is the operating system's name for that
  key (DirectInput object name; on Windows the localized key name). English game language: a name
  equal (case-insensitively) to `Shift`, `Alt` or `Ctrl` is replaced by `Left Shift`,
  `Left Alt`, `Left Ctrl`. A name longer than 64 bytes is cut to its first character. German,
  Swedish and Dutch game languages replace a few further names (space bar, arrows, right
  modifiers, Windows/application keys) with built-in translations [static; strings not
  reproduced here, needed only for those languages].
* **Joystick.** First the axis rule: Steering (Left)/(Right) with the steering axis assigned →
  column 0 shows the steering axis; Throttle with its own axis assigned and combined mode off →
  column 0 shows that axis; Back / Reverse likewise with the brake axis; Throttle or Back /
  Reverse with the combined axis assigned and combined mode on → column 0 shows the combined
  axis. When an axis occupies column 0, column 1 shows the **first** button/POV bound to the
  action. Otherwise column 0 = first, column 1 = second binding, searching buttons 0…15, then POV
  0…3 × direction (down, up, left, right). Names: axis/button/POV object names from the OS
  (a POV shows the POV's name for every direction). English: a name starting (case-insensitively)
  with `Button` followed by a number n is shown as `Button ` + (n + 1) (OS numbering starts at
  0). Korean: a similar renumbering with a Korean prefix.
* **Nothing bound / empty name:** `---` (three hyphen-minus characters).
* The text is converted to the UI text form and at most 23 characters are shown [inferred].

Vectors: `tvc08_display_names.json`, display columns in `tvc04`/`tvc06`.

Retail consequence of the column-1 rule (quirk Q3): when the first entry of an action is empty
but the second is not, both cells show the second key (e.g. after stealing Up from Throttle the
Throttle row shows `W | W`). `tvc04` case "step 1".

### 6.3 Keyboard capture [static]

On the first key press event (key down) during a keyboard capture of (action A, column C):

1. Escape (DIK `0x01`) → cancel, nothing changes.
2. Otherwise let K be the key. Clear K from **every** entry of the table (the key is taken away
   from whatever action had it — no conflict prompt, no swap).
3. Let e0, e1 be the first two entries of action A (Primary, Secondary).
   C = 1: e1 := K. C = 0: e0 := K.
4. Normalise: if e0 is now empty, e0 := e1's key; then if e0 and e1 hold the same key, e1 := empty.
5. Capture ends; the keyboard map is re-applied immediately.

So: assigning a key that the same action already has in the other column moves it (the other
column empties); assigning to Secondary while Primary is empty ends up in Primary. Vectors:
`tvc04_keyboard_capture.json`.

### 6.4 Joystick binding model and defaults [static]

Each joystick device has a 32-entry table indexed by **physical control**, whose value is an
**action id** (0 = none):

* entries 0…15: buttons 0…15;
* entries 16 + 4·p + d: POV hat p (0…3), direction d with **0 = down, 1 = up, 2 = left, 3 = right**.

Plus the axis state of `SPEC.md` §6 (steering, throttle, brake, combined axis ids = byte offsets
into the DirectInput joystick state: 0 X, 4 Y, 8 Z, 12 Rx, 16 Ry, 20 Rz, 24 slider 0,
28 slider 1; directions ±1; four "assigned" flags; combined-pedal mode).

Defaults when a device is opened and on "Reset Keys To Default":

1. Known devices (matched by DirectInput product GUID = USB VID/PID): 9 built-in tables replace
   entries 0…19 (buttons and POV 0); entries 20…31 = 0. VID/PID list and tables in
   `tvc05_joystick_defaults.json` (Logitech 046D:CA03/C216/C21A/C218/C20A, 0B43:0003,
   05FD:3002, 06A3:FF0C, Microsoft 045E:0027).
2. Otherwise by the number of buttons B:
   * B < 6: buttons 0–4 = Nitrous 6, Handbrake 5, Shift Down 8, Shift Up 7, Look Back 9; every
     POV: down = Back/Reverse 2, up = Throttle 1, left = Steer Left 3, right = Steer Right 4.
   * B ≥ 6: button 0 = Back/Reverse 2, button 1 = Throttle 1; B 6–7: buttons 2–6 = 6, 5, 8, 7, 9;
     B ≥ 8: buttons 2–9 = Look Back 9, Camera 10, Nitrous 6, Handbrake 5, Shift Down 8,
     Shift Up 7, Hydraulics Pressure 22, Hydraulics Bounce 19. Every POV: down/up none,
     left = 3, right = 4.
3. Axes (only if the device has at least one axis): steering = X, direction +1, assigned.
   Non-wheel: combined pedals on Y (fewer than 4 axes), else on Rz, except product 0B43:0003
   which uses Z; combined direction +1, assigned, combined mode on. Wheel: combined on slider 0
   when the wheel has slider 0 and Y but no Z; combined on Y when it has Y but neither Z, Rz nor
   slider 0; otherwise (separate pedal axes) no pedal axis is assigned and the user must capture
   Throttle and Back / Reverse. Device-open defaults before this step: throttle/brake directions
   −1, all flags clear, combined axis Y, combined direction +1, combined mode on.
4. A separate fixed menu table per device (not persisted, not shown): buttons 0–4 = select 31,
   back 34, customise 35, button 2 36, button 3 37; every POV down/up/left/right = 28/27/29/30.

Interpretation of "axis exists" from the executable's object queries is [inferred]. On an
Xbox-style pad seen through DirectInput (10 buttons; axes X, Y, Z, Rx, Ry) the rules give: A
Back/Reverse, B Throttle, X Look Back, Y Camera, LB Nitrous, RB Handbrake, Back Shift Down,
Start Shift Up, stick clicks Hydraulics; D-pad left/right steer; X axis steers; pedals on Rz,
which such a pad lacks [inferred]. Vectors: `tvc05`.

### 6.5 Joystick capture [static]

Each frame during a joystick capture of (action A, column C), in this order:

1. Escape on the keyboard → cancel.
2. Buttons 0…15: the first button i that is pressed now and was not pressed in the reference
   state (inferred: the previous frame) → assign to entry i (rule below); capture ends.
3. POVs 0…3: a POV that is not centred → direction d from its angle (centidegrees θ:
   if |sin θ| < |cos θ| then d = up when cos θ > 0 else down; otherwise d = right when sin θ > 0
   else left; exact diagonals count as left/right) → assign to entry 16 + 4p + d. (Checked even
   when a button was taken in the same frame; quirk Q4.)
4. Only for A ∈ {1, 2, 3, 4} and if still capturing: for the axes in the order X, Rx, Y, Ry, Z,
   Rz, slider 0, slider 1, if |value − value in the previous frame| > 1 000 (500 on a wheel) the
   axis is taken with sign s = +1 if it increased, −1 if it decreased (axis rule below).

Button/POV assignment rule for entry i: let keep = true unless an axis occupies column 0 of A
(§6.2 axis rule). For every entry equal to A: if C = 1 and keep and A ∉ {1, 2, 3, 4}, keep that
first one (keep := false); otherwise clear it. Then entry i := A (overwriting whatever action
entry i had — that action loses the control). Consequences: Primary capture removes every
previous binding of A; Secondary keeps the first existing one; for actions 1–4 any capture leaves
only the new control.

Axis rule for axis x, sign s: clear the "assigned" flag of every role currently using x. Then:
A = 3 (steer left): steering = x, direction −s, assigned. A = 4: steering = x, direction +s.
A = 1: throttle = x, direction s; if the brake axis is x with a different direction → combined
axis = x, combined direction = −s, combined assigned, combined mode on; else throttle assigned,
combined mode off. A = 2: symmetric, combined direction = +s. Capture ends. Vectors: `tvc06`.

### 6.6 Persistence and start-up [static unless noted]

* **Joystick devices:** the six device records of the profile payload (`SPEC.md` §6), written
  with every profile save and matched to connected devices by instance GUID on load. Record
  bytes 53…180 hold the 32-entry table of §6.4 (index = control, value = action id). The
  keyboard occupies a slot too (its GUID is the keyboard's DirectInput instance GUID); its
  32-entry table is unused [inferred: no keyboard path writes it].
* **Keyboard:** not stored in the profile, the registry or any game file. The table lives in
  memory; the executable hands it to the DirectInput action mapper and asks it to save the user
  map, but at start-up rebuilds the maps from the built-in defaults. Community reports say
  retail keyboard bindings persist only when the DirectInput mapper component is installed
  ([public] e.g. https://www.supercheats.com/playstation2/questions/needforspeedunderground2/239018/is-there-a-way-to-save-the-con.htm,
  https://forums.ea.com/discussions/need-for-speed-franchise-discussion-en/re-problem-with-controls/9271570).
  Practically: retail keyboard bindings are session-only [inferred]. See decision D4.
* Start-up keyboard state: the default table with the first Throttle entry (Up) flagged
  "not mapped" for the action mapper [static]; whether Up then shows/works before the first
  reset is unknown (U2).

## 7. Resource map (everything the screen needs) [data]

| Element | Resource |
|---|---|
| layout, panels, arrows, scrollbar, row objects | `UI_Options_PC_Controller.fng` (`0xE0EE65E6`) and its 9 textures in `GLOBALB` |
| font | `ConduitMdITC_TT21i.ffn` (main face) |
| header / selector / column / instruction / row labels | `English.bin` keys of §3, §5 (`tvc01_labels.json`) |
| binding texts | platform key/button/axis names (§6.2) and the literal `---` |
| reset dialog | `GenericDialog_SMALL.fng`, keys `0x2BBCCA6B`, `0x00000639`, `0x541389E5` |
| help-bar pills | `UI_PC_Help_Bar.fng`, keys `0x1C101778`, `0x1C0787E6`, `0xB7D0D273` |
| front-end hub entry | icon `0xA04A7B26` (`OPTIONS_ICON_CONTROLS`, `GLOBALB`), label `0x04DC6DB5` |

## 8. OpenUG2 hook points (baseline names)

* **Assets/packages:** `ug_assets_package_by_key(…, 0xE0EE65E6)` (or `ug_assets_package(…,
  "UI_Options_PC_Controller.fng")`), `ug_fng_parse()`, `ug_scene_init()`/`ug_scene_update()`/
  `ug_scene_post()`/`ug_scene_set_focus()`/`ug_scene_set_world_pos()`, `UgNode.text_override`,
  `app_hidden`; labels via `ug_strtab_get(&assets.strings, key)`. Works with
  `ug_assets_open_ingame()` because `FRONTB` is loaded there too.
* **Option-screen framework (branch `settings-gui`):** `OptionsMenu`/`om_open_hub()` and the hub
  table `OmHubEntry` (`OM_HUB_ENTRIES`), `OmScreen`, exit handling in `om_update()`, dialogs via
  `GuiDialog` (generic two-button), the view `gui_view.*`. The Controls screen needs its own
  list model (3 columns, 7 rows, selector row) — `OmList` (8 rows, 1 column) does not fit as is.
* **Persistence:** `StProfile.input[]` / `StInputDevice` (`binding[32]`, `axis[4]`, `dir[4]`,
  `assigned[4]`, `combined_mode`) and `st_input_find()` in `src/settings.h` for joysticks; a
  keyboard store per decision D4 (`StRegStore`/INI is available).
* **Game input:** replace the hard-wired scancode reads in `src/main.c` (per-tick driving block:
  throttle, brake, steer, handbrake, nitrous, look back; `SDL_KEYDOWN` dispatch: camera, pause)
  with action queries over the binding tables; the pause key path (`pause_menu_event()` and the
  Escape/Start handling in `main.c`) must use action 12.
* **Pause menu route:** `src/pause_menu.c` / `pause_screen.*` (`PS_EV_OPTIONS`) if D1 chooses an
  in-game route.

## 9. Amendments to `SPEC.md` (for the reviewer; SPEC.md itself is not edited)

1. §6 "32 × i32 binding table (button / control codes per action slot)" → the table is indexed by
   **physical control** (entries 0–15 buttons, 16 + 4p + d POV p direction d, d: 0 down, 1 up,
   2 left, 3 right) and each value is an **action id** (§5.1, 0 = none). [static]
2. §6 device defaults → replace "combined axis 4 (Y) … (20 or 24 on wheels …)" by §6.4 step 3
   (non-wheel: Y with < 4 axes, else Rz, product 0B43:0003 → Z; wheel: slider 0 / Y / none) and
   add the button/POV/known-device defaults of §6.4. [static]
3. §6 "for the system keyboard, a GUID the game stores itself" → it is the keyboard's DirectInput
   instance GUID; keyboard bindings are not in the record (§6.6). [static; inferred for the GUID value]
4. §11.4 resolved except U1–U3 below; QUESTIONS (analyst) item 4 answered by §5–§6.

## 10. Decisions for the coordinator

* **D1 Route.** Retail has no in-game route (§2.2). Options: (a) add a fifth entry to the pause
  options hub using `OPTIONS_ICON_CONTROLS` `0xA04A7B26` + label `0x04DC6DB5` (all original
  resources; `UI_PauseOptionsMain.fng` has `option_1…10`), Back returning to that hub —
  a deliberate deviation from retail; (b) wait for a front-end `UI_OptionsMain.fng`.
* **D2 Key names.** Retail shows OS key names (§6.2). With SDL the closest equivalent is
  `SDL_GetScancodeName()`/`SDL_GetKeyName()` (platform data, not a created string). Differences
  for the defaults: `Escape` vs `Esc`, `Return` vs `Enter` (`tvc03`). Gamepad: SDL has no
  DirectInput-style object names ("Button 1", "X Axis", POV name). Decide whether platform names
  are acceptable or the Windows naming is to be emulated.
* **D3 Gamepad control indices.** The retail table is indexed by DirectInput button/POV order.
  SDL joystick button/hat indices match that order only on Windows' DirectInput backend.
  Decide: raw SDL joystick indices (record-compatible on Windows), or SDL GameController
  buttons mapped to a fixed DirectInput-like order (e.g. Xbox order A, B, X, Y, LB, RB, Back,
  Start, LS, RS), which keeps retail profiles meaningful for XInput pads.
* **D4 Keyboard persistence.** Retail: session-only (§6.6). Decide whether OpenUG2 persists
  keyboard bindings (e.g. OpenUG2's INI store) — a behaviour improvement, no resource needed.
* **D5 Retail quirks to reproduce or fix:** Q1 stale device index on re-entry (rows show keyboard
  while Left/Right/Reset act relative to the previously selected device); Q2 wheel row 12
  secondary display (§5.4); Q3 column-1 display rule (§6.2); Q4 POV checked after a button
  assignment in the same frame; Escape unbindable; no conflict prompt (steal semantics).
* **D6 Defaults vs current OpenUG2 keys.** Retail defaults (§6.1) differ from today's hard-wired
  keys (nitrous N → Left Alt / Right Alt, steering adds arrow keys, etc.).

## 11. Unknowns / ambiguities

* **U1** Exact OS names DirectInput gives for keys/buttons/axes/POVs on the retail target
  (inferred English Windows names in `tvc03`); runtime capture needed.
* **U2** Keyboard start-up state: effect of the "not mapped" flag on the first Throttle entry
  (does Up accelerate / show before a reset?).
* **U3** Units of the axis-change threshold (1 000 / 500 per frame): the axis range the game sets
  on joysticks was not determined.
* **U4** Whether button activation `0x0C407210` from a cell also reaches the event handler (it
  would start `FORWARD`, whose immediate `0x84378BEF` un-highlights the cell during capture;
  exit-complete is ignored because Back is not pending) — same open point as SPEC_GUI_ART §10.1.
* **U5** Which keyboard key (if any) raises PAD_BUTTON0 on this screen (menu actions 35–37 are
  candidates); pill click is certain.
* **U6** Pill order on the help bar (inferred from bit order).
* **U7** Behaviour when a selector arrow is clicked while a cell is focused (rows are rebuilt
  while the focus reference is not cleared) — recommend moving focus to the selector.
* **U8** Meaning of condition C on the front-end hub (EA TRAX/Credits vs Online entry).
* **U9** How the two non-menu keyboard contexts (tables used in other game states) are chosen;
  they are not edited by this screen.
* **U10** UI sounds per input (ids exist, not specified).

## 12. Capture requests (retail runtime, Windows, English)

1. Screenshots of the Controls screen on entry (keyboard), with focus on Throttle Secondary,
   during capture, and after a capture (instruction line changes).
2. Same screen with a gamepad connected: after Right on the selector (title, rows, axis names).
3. Press Escape, then C, `,` and `.` on the screen (U5); click "Reset Keys To Default" (dialog).
4. Fresh start: what Throttle shows before any reset (U2); rebind W to Nitrous and check the
   Throttle row (Q3).
5. Quit and restart the game: are keyboard changes kept (§6.6)?

## 13. Test vectors

`test-vectors/controls/README.md`: `tvc01_labels` … `tvc09_package`. Synthetic scenarios over
retail-derived defaults; no retail bytes reproduced.
