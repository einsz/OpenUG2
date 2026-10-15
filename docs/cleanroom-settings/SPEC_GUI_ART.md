# Authored-art presentation of the pause menu and graphics options screens

Status: reviewed and approved 2026-10-14 (see REVIEW.md). Supplement to `SPEC_GUI.md` (approved), which stays authoritative
except where section 9 below lists a correction. This file says how the retail screens are built
from the retail FEng packages, so that an implementation can evaluate those packages instead of
drawing the screens with native primitives, and it fills in the pause-menu content that was so far
unknown (entry catalogue, Options route, quit confirmation).

Provenance: static examination of the retail PC executable (same binary as SPEC.md / SPEC_GUI.md);
the original 2004 retail data files `GLOBAL/InGameCommon.bun`/`.lzc`, `GLOBAL/GLOBALB.BUN`,
`FRONTEND/FRONTB.LZC.bacc`, `LANGUAGES/English.bin`; the user's retail screenshots already used by
SPEC_GUI.md (Audio page) and the pause-menu work (pause screen in explore mode, frontend quit
dialog); the public FEngLib project (message-response command numbers). The game was not run. All
test vectors are synthetic.

Tags: **[data]** read from retail package/language/texture data; **[static]** determined from the
executable, not run; **[observed]** measured on a retail screenshot; **[inferred]** one reasoning
step from the above; **[guessed]** plausible, untested.

Identifiers. Package names, object names, script names, message hashes, texture keys and
language-label keys below are data lookup keys and are functionally required (flagged as such in
SPEC_GUI.md). Hash: `h = 0xFFFFFFFF; for each byte c: h = h*33 + toupper(c) (mod 2^32)`.
Coordinates: centred 640×480 virtual units, x right, y down. "Centre"/"top-left" refer to an
image's rectangle; for text objects "position" is the text anchor as in the base GUI rules.

---

## 1. Runtime rules shared by all these screens

### 1.1 Message-response commands [data; public FEngLib numbering]

Object and package message responses use these command numbers: 0 run script (by name hash),
1 post message to FEng (to an object guid), 2 post message to the application, 0x100 make object
(guid) the current button, 0x300 if current script == X, 0x301 if current script != X,
0x500 else, 0x501 end-if. Script events (`EV` records: message, target, time) post the message at
the given time; target `0xFFFFFFFF` = the application, `0` = the package's target list for that
message, `0xFFFFFFFC` = every object of the package [data; meaning of 0/0xFFFFFFFC inferred from
the target lists].

### 1.2 Property writes by the application [static]

When the application sets a property of an object (position, size, colour/alpha, text label,
texture), the value is written into the **base value (time −1 key) of that property's track in
every script of that object**; if no script of the object has such a track, it is written into the
object's own stored data. Consequently a script that is started later animates relative to the
application-set value, and authored scripts do not snap the object back. Implementations must use
this rule when they move, resize, recolour or retexture authored objects (rows, icons, slider
parts, scroll tab, carousel items).

Position setters come in two forms: "set centre" and "set top-left" (top-left = centre − size/2 for
images). Both are used below; the form is stated each time.

### 1.3 Visibility [static]

"Hide"/"show" by the application set/clear the object's invisible flag; they do not run scripts.

### 1.4 Event-handler object and transitions [data]

Every package has an event-handler text object, name hash `0x47FF4E7C`, which is never drawn
(it is placed off-screen). Its `Init` script chains to the enter script; the application starts
its exit script by name. Timings per package (vectors `tvg11_transitions.json`):

| Package | Enter script → events | Exit script (start by app) → events | Exit complete |
|---|---|---|---|
| `UI_pause.fng` | `call` 320 ms: `0xBDFD2DF9` to targets at 0; `0x37EDF39B` to app at 320 | `Forward` 320 ms: `0xAADD0187` to targets at 0; `0xE1FDE1D1` to app at 320 | 320 ms |
| `UI_PauseOptionsMain.fng` | `CALL` 320 ms, same events | `Forward` 320 ms, same events | 320 ms |
| `UI_PauseOptions.fng` | `CALL` 320 ms: `0xBDFD2DF9` at 0; `INIT_COMPLETE` (`0x35F8620B`) to app at 320 | `FORWARD` 320 ms: `0xAADD0187` at 0; `0x84378BEF` to app at 0; `0xE1FDE1D1` to app at 320 | 320 ms |
| `UI_Options.fng`, `UI_PC_Customize_Options.fng` | `call` 500 ms: `0xBDFD2DF9` at 0; `INIT_COMPLETE` at 320 | `Forward`/`FORWARD` 500 ms: `0xAADD0187` at 0; `0x84378BEF` at 0; `0xE1FDE1D1` at **140** | 140 ms |
| `UI_InGameDialog.fng`, `GenericDialog_SMALL.fng` | `call` 180 ms: `0xBDFD2DF9` to all at 0; `INIT_COMPLETE` at 180 | `FORWARD`/`Forward` 140 ms: `0xAADD0187` to all at 0; `0xE1FDE1D1` at 140 | 140 ms |

`0xBDFD2DF9` makes the listed objects run their `FadeIn` script; `0xAADD0187` makes them run
`FadeOut`/`fadeout`. `0xE1FDE1D1` ("exit complete") is what the application waits for before it
switches package or performs a deferred action. This answers SPEC_GUI §5.1/§11.7 "exit animation
length": 140 ms for the frontend Display and Customize screens, 320 ms in game.

Dim/undim: the application runs script `DIM` (hash `0x00009E99`) or `UNDIM` (`0x03D8EABC`) on the
handler. In `UI_pause.fng`/`UI_PauseOptionsMain.fng` these 0 ms scripts post `0xA301C015` /
`0x9F218E63` to the targets, which run their 100 ms `DIM`/`UnDim` scripts. In `UI_Options.fng` and
Customize the 100 ms `DIM`/`UnDim` scripts post the same messages to all objects, and `UnDim` posts
`0x0153E234` (undim complete) to the application at 100 ms. [data]

Handler responses (all packages): `0xCB1AA27F` → `BACK` (0 ms, no effect); `0xADE14EC3` → exit
script; `0x0C407210` → "if exit script not running: run exit script". `UI_Options`/Customize also
respond to Back `0x911AB364` the same way; `UI_PauseOptionsMain` runs `Forward` on `0x911AB364`
unconditionally. Whether the button-activation message reaches the handler in every case is
discussed in §10 item 1. [data]

### 1.5 Package-level key responses [data]

| Package | Up `0x72619778` | Down `0x911C0A4B` | Left `0x9120409E` | Right `0xB5971BF1` | Back `0x911AB364` | Start `0xB5AF2461` | Accept `0x406415E3` |
|---|---|---|---|---|---|---|---|
| `UI_pause`, `UI_PauseOptionsMain` | — | — | to app + to `ScrollBar_Arrow_1` (plays `Blink`) | to app + to `ScrollBar_Arrow_2` (`Blink`) | to app | to app | — |
| `UI_PauseOptions` | to app + `ScrollBar_Arrow_1` | to app + `ScrollBar_Arrow_2` | — | — | to app + `0x0C407210` to an object | to app | — |
| `UI_Options`, Customize | to app + `ScrollBar_Arrow_1` | to app + `ScrollBar_Arrow_2` | — | — | to app + to handler | — | to app |

`0x14DD0705` is mapped to Back in all of them. `Blink` (400 ms) chains to `Static`/`STATIC`.

## 2. Pause menu (`UI_pause.fng`, key `0x32124083`)

### 2.1 What the screen shows [static; data]

* `Header_text` (`0x42ADB44C`) label set to `0x6C839FBE` "Pause"; `Back_button` to `0x933DD695`,
  `Select_button` to `0x8202C364` (both inside the console-only group, flag `0x40`, not drawn on PC).
* An icon carousel (§3) whose entries are added in the order of §2.2. Entry k is bound to the
  package object `option_k` (k = 1…10). Each bound object's texture is replaced by the entry's icon
  texture key; `ICON_TITLE` shows the label key of the focused entry.
* Initial focus: entry 1 [static] (the remembered index is reset when the screen is built). The
  page index used by the Options hub (§2.5) is reset to 0 (Audio) every time the pause screen is
  created [static].

### 2.2 Entry catalogue and order [static; labels data]

Context values: *mode* (game mode: 1 career; 4 two-player; 8 online; `0x200` LAN; other values
are other single-player modes, e.g. 2) [mode 1 static; others inferred from the labels and
neighbouring front-end code]; *race* (race-activity value: 0 = no race, i.e. free roam / explore;
7 = a special race state whose meaning is unknown; other = race in progress) [static, meaning of 7
unknown]; *session* (online session state; > 2 = connected) [inferred].

**A. No race in progress (race = 0)** — the explore/free-roam pause:

| # | Present when | Icon texture key (name) | Label key → English |
|---|---|---|---|
| 1 | mode = 1 | `0x62AA0DF6` (`INGAME_ICON_EXPLORE`) | `0xC3E5BCDC` Return to Explore Mode |
| 1′ | mode ≠ 1 | `0x62AA0DF6` (`INGAME_ICON_EXPLORE`) | `0xC44A6515` Return to Free Run |
| 2 | mode = 1 and a garage flag (never set, §2.3) | `0xB8038AC0` (`SHOP_ICON_CRIB`) | `0x453890E1` Jump to Garage? |
| 3 | Options condition (§2.3) | `0x520DE4E3` (`INGAME_ICON_OPTIONS`) | `0xB34993C9` Options |
| 4 | mode ∉ {4, 8, `0x200`} | `0x84D1D864` (`INGAME_ICON_REPLAY`) | `0x7840428A` Replay |
| 5 | always | `0x4C9E34E6` (`INGAME_ICON_QUIT_TO_FE`) | mode 8: `0xFF956AF2` Quit to Online Main Menu; `0x200`: `0xF39723A1` Quit to LAN Main Menu; else `0xC7E1420C` **Quit to Main Menu** |

So the retail career explore pause is: Return to Explore Mode, Options, Replay, Quit to Main Menu
(4 entries; Replay present but not focusable when no replay is available, §2.3). This matches the
user's retail screenshot (explore icon selected, options icon to its right, right arrow visible).
[static; observed consistent]

**B. Race in progress (race ≠ 0)**:

* If session > 2: only the quit entry (as in A.5) [static].
* Otherwise, in this order:

| # | Present when | Icon key (name) | Label → English |
|---|---|---|---|
| 1 | mode = 1 and the race was launched from the career world map [inferred meaning of the flag] | `0x3109913C` (`INGAME_ICON_WORLD_MAP`) | `0x5E670A9E` Return to World Map |
| 2 | always | `0x12BB5EA2` (`INGAME_ICON_RESUME_RACE`) | `0xF6601088` Resume Race |
| 3 | race ≠ 7 and mode ∉ {8, `0x200`} | `0xB295A6B6` (`INGAME_ICON_RESTART_RACE…`, name truncated in the pack) | `0x80833FC2` Restart |
| 4 | mode = 1 and race ≠ 7 | `0x62AA0DF6` (`INGAME_ICON_EXPLORE`) | `0xDB4E3C0D` Return to Explore Mode |
| 5 | Options condition | `0x520DE4E3` | `0xB34993C9` Options |
| 6 | mode ∉ {4, 8, `0x200`} | `0x84D1D864` | `0x7840428A` Replay |
| 7 | always | `0x4C9E34E6` | as A.5 |

Icon textures `INGAME_ICON_*` are in `GLOBAL/InGameCommon.bun`; `SHOP_ICON_CRIB` in
`GLOBAL/GLOBALB.BUN` [data]. Vectors: `tvg07_pause_entries.json`.

### 2.3 Conditions [static]

* **Options condition**: a player record bound to the controller that opened the pause exists, and
  (no race is in progress, or a per-player field of that record is zero). The meaning of that field
  is unknown; in an ordinary single-player session the entry is expected to be present
  [guessed]. In free roam the condition reduces to "a player record for the pausing controller
  exists".
* **Replay entry**: always added when the mode allows it; it is *disabled* (skipped by Left/Right,
  cannot be activated) unless replay data is available (a replay subsystem flag set, recorded
  length > 0, and a replay-blocking global clear). Disabled entries are drawn like the others except
  that, while the edge fade of §3.3 is active, their alpha is capped at 150 instead of 255.
* **Jump to Garage**: guarded by a global with no writer in the executable and a zero initial
  value, so it never appears in the retail PC game [static].

### 2.4 Entry actions [static]

"after exit" = the action runs when the exit script reports `0xE1FDE1D1` (320 ms);
"immediate" = it runs at once when the entry is activated.

| Entry | Timing | Action |
|---|---|---|
| Return to Explore Mode / Free Run (A) | after exit | resume gameplay (free-roam resume request) |
| Options | after exit | switch to `UI_PauseOptionsMain.fng` (§2.5) |
| Replay | after exit | switch to `ui_ReplayControl.fng` (only reachable when enabled) |
| Resume Race | immediate | resume gameplay; the pause package is removed |
| Restart | immediate | confirmation dialog; OK → restart the race |
| Return to Explore Mode (B) | immediate | dialog; OK → leave the race to explore mode |
| Return to World Map | immediate | dialog; OK → leave the race and return to the career front end (start package `UI_CareerCrib.fng`) |
| Jump to Garage | immediate | dialog; OK → same as World Map (never reachable) |
| Quit … | immediate | dialog; OK → quit to the front end |

Back or Start on the pause screen: start the exit script, then resume gameplay at exit-complete.
A second Back/Start/Accept while the exit script runs is ignored. Activation and Back/Start are
ignored while a stored creation tick equals the current tick, i.e. on the frame the screen was
created [static guard; tick meaning inferred].

**Confirmation dialogs** (all of them): package **`UI_InGameDialog.fng`** (key `0xAA469286`,
`InGameCommon`), first button label `0x00000639` **"OK"**, second button label `0x541389E5`
**"Cancel"**, Cancel and Back post `0xB4623F67`, **initial focus = second button (Cancel)** (§6).
Message keys:

| Dialog | Normal | Racing with sub-state 4 [meaning unknown] |
|---|---|---|
| Quit (mode 1) | `0x6F4796A6` "Quit to Main Menu?" | `0xE15B558A` "Quit to Main Menu?^All unsaved race progress will be lost." |
| Quit (other single-player modes) | `0x30ED2368` "Quit to Main Menu?" | `0xE15B558A` |
| Quit (mode 8) | `0xCE50DA14` / with a flag `0xBC25C6B1`; when an online session test holds `0x555BD5B6` / with the flag `0x79A85513` | — |
| Quit (mode `0x200`) | `0xD63B6982`; session test holds: `0x555BD5B6` | — |
| Restart | `0x0972A44B` "Restart Race?" | `0xAA50AA02` |
| Return to Explore Mode | `0x60B42AFB` "Return to Explore Mode?" | `0x07CD17B0` |
| Return to World Map | `0x33C42E03` "Back to the World Map?" | `0xE761C8B8` |
| Jump to Garage | `0xC9BA9E3E` "Jump to Garage?" | — |

OK posts: Quit `0x30ED2368`, Restart `0xE1A57D51`, Explore `0x451E768E`, World Map `0x33C42E03`,
Garage `0x43DA9FD0`. While a dialog is open the pause screen is dimmed (handler `DIM`, §1.4); on
Cancel/Back it is undimmed and accepts input again, and from then on the edge fade (§3.3) is
enabled. The pause screen's own exit script is also started when an immediate entry is activated
[static]; the visual result of the exit fade combined with the dim while the dialog is open was not
determined (§10 item 2). Vectors: `tvg10_dialogs.json`.

The caption "Quit Game" used by earlier application-made dialogs does not exist in the retail pause
flow; the retail pause quit caption (`ICON_TITLE`) is "Quit to Main Menu" (`0xC7E1420C`) in career
and other single-player modes. [static; data]

### 2.5 Options → pause options hub (`UI_PauseOptionsMain.fng`, key `0x73F51C74`) [static; data]

Same carousel template as the pause screen (objects `option_1…10`, `OPTION_MASTER`, `ICON_TITLE`,
`HEADER_TEXT`, scroll objects). `HEADER_TEXT` shows `0x1D7BB6C9` "PAUSE OPTIONS". Entries, in order:

| # | Icon texture key (name, `GLOBALB`) | Label → English | Page index set on activation |
|---|---|---|---|
| 1 | `0xF37AF144` (`OPTIONS_ICON_AUDIO`) | `0xE76CD783` Audio | 0 |
| 2 | `0x8A006328` (`OPTIONS_ICON_DISPLAY`) | `0x4204D527` Display | 1 |
| 3 | `0x4DF98FB2` (`OPTIONS_ICON_GAMEPLAY_S…`) | `0xD0CF6EE1` Gameplay | 2 |
| 4 | `0xD708EFEF` (`OPTIONS_ICON_PLAYER_SET…`) | `0xF760EABE` Player | 3 |

* Initial focus: the entry remembered for this package, 1-based; 0 = entry 1 (Audio). The
  remembered value is updated to the focused entry on every Left/Right move, and reset to 0 when the
  hub is left with Back and when the pause session ends through Start on the Display screen. So
  after Display → Back the hub reopens on Display if the user had moved to it with Left/Right
  [static]. (The pause screen itself resets its remembered value when built, so it always opens on
  entry 1.) All four are "after exit": activation runs the exit (320 ms), the
  entry sets the page index, and on exit-complete the screen switches to `UI_PauseOptions.fng`,
  which builds the rows of that page. Display (index 1) gives the four Display rows of SPEC_GUI §3.1;
  from there "Customize Display Settings" leads to `UI_PC_Customize_Options.fng` (SPEC_GUI §1.2).
* Back: exit, then switch to `UI_pause.fng` (or to the dyno screen `UI_PerformanceDyno_MAIN.fng`
  when a dyno-session flag is set [static; meaning of the flag inferred]). Start: exit, then close
  this package and the pause package and resume gameplay.

## 3. Icon carousel engine (pause menu and pause options hub) [static unless noted]

### 3.1 Bound objects

| Object | Role |
|---|---|
| `option_1` … `option_10` | entry slots in list order; texture replaced by the entry icon |
| `OPTION_MASTER` (untextured magenta, never shown) | anchor: its centre is the selected slot centre (0.388, −14.296); size 118×59 |
| `ICON_TITLE` | caption = label of the focused entry |
| `ScrollBar_Backing` (`0x688A1551`) | horizontal rail: centre (3, −52), 562×8 (`UI_pause`) / 564×8 (hub) |
| `ScrollBar_Handle` (`0x3FBB7A4E`, `U2_Scroller_Tab.tga`) | rail marker (tab), 16×16 |
| `ScrollBar_Arrow_1` (`0x444969FD`) / `ScrollBar_Arrow_2` (`0x444969FE`) | left / right arrows (`U2_Menu_Arrow_White.tga`, 26×26; the left one is rotated by its tracks) |

Unused slots (`option_N+1…10`) are never positioned by the application; they keep their authored
off-screen positions (y ≥ 334) and are therefore invisible even though they receive the enter
fade. [data; static]

### 3.2 Layout

* Entry i has a strip offset `x_i = (i − 1) × 158` (slot width 118 + spacing 40; offsets accumulate
  each slot's own width) [static; data].
* Each update sets every bound slot's **centre** to (`OPTION_MASTER.x + x_i + s`, `OPTION_MASTER.y`),
  where s is the scroll offset. The focused slot is sized 118×59; every other slot is sized
  0.8× (94.4×47.2) about the same centre.
* A slot is shown only while its centre x is within ±240 of `OPTION_MASTER.x`; otherwise hidden.
  At rest that leaves the focused slot and its immediate neighbours (centres ±158).
* At rest s = −x_focus. When focus changes, s moves from its current value to −x_focus with a cubic
  Hermite curve over 0.3 s (start and end velocity 0 when starting from rest:
  `s(t) = s0 + (s1 − s0)(3u² − 2u³)`, u = t/0.3) [static; time unit seconds inferred]. The focus
  (full size, caption, tab, arrows) changes immediately; only s is animated. On first build s
  snaps without animation.
* Navigation: Left/Right move to the previous/next entry, skipping disabled entries; **no wrap**
  (nothing happens at the ends). Up/Down are ignored.
* Tab (marker) centre x = `rail_left + 8 + (rail_w − 16) × (c − 1)/(N − 1)` for focused entry c of N,
  `rail_left = 3 − rail_w/2`; y unchanged (−52.3). The rail, tab and arrows are hidden when N ≤ 1.
  Check: N = 4, c = 1 → x −270, matching the retail screenshot marker at (−270, −51.6) [observed].
* Arrows: `ScrollBar_Arrow_1` hidden when the first entry is focused, `ScrollBar_Arrow_2` hidden
  when the last is focused, both shown otherwise. On a move to the right the left arrow plays
  `Static` and the right arrow `Blink` (unless already playing); the package's own key responses
  also play `Blink` on the arrow of the pressed direction (§1.5).

Vectors: `tvg08_icon_carousel.json`.

### 3.3 Edge fade [static]

Disabled when the screen is built; enabled from the first time a dialog over the screen is closed
(undim), and disabled again (all slot colours restored to white, alpha 255) when a dim starts or
when an entry is activated / Back is pressed. While enabled, each slot's alpha is
`trunc(f × A)` with A = 255 (150 for a disabled entry) and
`f = (x − L)/94.4` for L ≤ x < L + 94.4, `(R − x)/94.4` for R − 94.4 < x ≤ R, 1 between, 0 outside;
L = `OPTION_MASTER.x − 240`, R = `OPTION_MASTER.x + 240`, x = slot centre, 94.4 = 0.8 × slot width.
The alpha is written as the base colour (§1.2), so authored fades scale from it.

### 3.4 Authored per-slot behaviour [data]

`option_n`: `Init` → `hide` (alpha 0); `FadeIn` 220 ms (alpha 0 until 40 ms, then to 255);
`fadeout` 100 ms; `DIM` 100 ms (to alpha 60); `UnDim` 100 ms; script `0xA5CEBA13` 100 ms (fade out,
started by message `0x3F3C398F`). Activation posts `0x0C407210` to the application. Selected-slot
glow `0xDF422345` loops `Animate` (1000 ms), backing `0x687FDA31` static; neither is moved by the
application (they stay at the authored centre ≈ (39.3, 7.0) inside the presentation group, i.e. at
the selected slot) [data; inferred placement].

## 4. Option-list screens: binding the authored rows

The list engine and row semantics of SPEC_GUI §2 apply; this section adds what is needed to drive
the authored objects.

### 4.1 Objects per row slot n [static; data]

Name templates (case-insensitive hashes of the name + decimal n): `OPTION_NAME_`, `OPTION_DATA_`,
`OPTION_IMAGE_`, `OPTION_BACKING_`, `LEFT_ARROW_`, `RIGHT_ARROW_`, `SLIDER_BASE_`, `SLIDER_KNOB_`,
`SLIDER_VALUE_`, `SLIDER_HANDLE_`, `SLIDER_BEG_`, `SLIDER_END_`; anchors `TITLE_MASTER`,
`DATA_MASTER`; optional `DONE_BUTTON` (none of the three packages has one); scrollbar
`ScrollBar_Backing`, `ScrollBar_Handle`, `ScrollBar_Arrow_1/2`, `ScrollBar_Backing_End_1/2`
(Customize only).

* Text: `OPTION_NAME_n` label = row label key; `OPTION_DATA_n` label = value key (or literal
  resolution text) for choice rows. `SLIDER_VALUE_n` (absent in all three packages) would show the
  value as a decimal integer.
* Arrows: `LEFT_ARROW_n`/`RIGHT_ARROW_n` are untextured in the packages; the application sets their
  texture key (SPEC_GUI §2.1: `0x9943D584` for graphics rows).
* Slider parts: base = `SLIDER_BASE_n` (`U2_temp_sliderBG.tga`, 234.92×32), knob = `SLIDER_KNOB_n`
  (`U2_SliderBar_Tab.tga`, 16×16), fill = `SLIDER_HANDLE_n` (white, 16×5.75, green tint
  (99,178,34)); `SLIDER_BEG_n`/`SLIDER_END_n` are bound but never repositioned and stay at their
  authored off-screen positions (not visible).

### 4.2 Row placement [static; observed cross-check]

Let T = `TITLE_MASTER` top-left (tx, ty) and size (tw, th); D = `DATA_MASTER` top-left (dx, dy) and
size (dw, dh). Row pitch = th (36). For the row shown at visible line k (0-based):
row top `ry = ty + k·th`, data top `dy_k = dy + k·th`, data centre x `dcx = dx + dw/2`.

| Object | Placement (top-left unless stated) |
|---|---|
| name text | (tx, ry); if the text object is centre-justified, its centre x = tx + tw/2 |
| value text (choice) | centre x = dcx, top = dy_k |
| left arrow | top-left (dx − arrow_w, dy_k) → right edge on the data column's left edge |
| right arrow | top-left (dx + dw, dy_k) |
| slider base | top-left (dcx − base_w/2, dy_k) |
| backing group | position (tx + 276, ry + 10) (group children keep their authored offsets) |

Rows outside the window are hidden and moved to y = 6969. Cross-check, frontend Display/Audio
package: arrows centre x 6.0 / 246.0 (observed 6.2 / 246.2); backing children span
x −299…+273.7 including the end caps' transparent margins (observed visible ink −286.2…+261.8),
y −145…−109 (observed −144.4…−109.6). Vectors: `tvg09_rows_sliders.json`.

### 4.3 Slider drawing [static; observed cross-check]

With base top-left (bx, by), base size (bw, bh), knob size (kw, kh), margin m = 46, usable width
W = bw − 2m, and f = (v − min)/(max − min):

* knob centre = (bx + m + f·W, by + bh/2 − 1);
* fill: width = f·W, top-left = (bx + m, by + bh/2 − kh/2 + 1) (its height stays the authored 5.75);
* `SLIDER_VALUE_n`, if present, shows trunc(v) as decimal.

Frontend numbers: bx = 8.54, W = 142.92 → fill start 54.54 (observed 54.6); f = 0.703 → knob
155.0 (observed 154.5); f = 1 → 197.5 (observed 196.6).

### 4.4 Focus and state scripts [static for which script; data for content]

Focus gained: `Highlight` on name, data, left arrow, right arrow, backing (via the backing's
`Button_Highlight` response in the frontend packages) and, for slider rows, on base, knob, value,
fill, beg and end; the row's name object becomes the current FEng button. Focus lost: `UnHighlight`
on the same set. Disable/enable as in SPEC_GUI §2.4. Arrow `Blink` (400 ms) on Left/Right.
In-game durations (`UI_PauseOptions.fng`): name/data `Highlight` 1200 ms looping, `FadeIn` 220 ms,
`FadeOut` 100 ms; arrows `Highlight` 180 ms, `UnHighlight` 140 ms; slider parts `FadeIn` 220 ms,
`FadeOut` 80–100 ms. Frontend (`UI_Options`/Customize): `FadeIn` 180 ms, `FadeOut` 140 ms, `DIM`/
`UNDIM` 100 ms, name/data `HIGHLIGHT` 1200 ms. All scripts of row slots start at `Init → Hide`
(alpha 0) and become visible through the enter broadcast `0xBDFD2DF9`. [data]

### 4.5 Scrollbar (vertical lists) [static]

Tab centre y = `rail_top + 8 + (rail_h − 16)·(c − 1)/(N − 1)` where c = focused row (1-based), N =
number of rows, rail = `ScrollBar_Backing` (frontend/Customize centre (273, 2), 8×282 → top −139;
in game (275, 10) → top −131); tab x unchanged. With N ≤ 1 the tab and arrows are hidden. When N > 1
the backing, arrows and backing ends are shown. This answers SPEC_GUI §11.2 (tab end positions and
size: 16×16, centred 8 units inside each end). In game, `ScrollBar_Arrow_1/2` are hidden on the
Audio, Display and Gameplay pages (shown only on the Player page) [static].

### 4.6 Pointer hit extents [static]

A pointer position hits an object when it lies inside the object's current on-screen rectangle,
**edges inclusive**. On a choice/slider row the left arrow is tested first (acts as Left), then the
right arrow and then the value text (both act as Right). On a button row a hit on the name object
activates it (posts `0x0C407210`). This answers SPEC_GUI §11.10 for button rows (yes: a click on
the label activates). Hover-to-focus and wheel remain unknown.

### 4.7 Initial focus [static]

In game, `UI_PauseOptions.fng` starts on a row index remembered per package by the screen
framework, but every path that leaves that screen (Back, Customize, Start) resets the remembered
index to 0, and the Options hub passes 0 when switching in. With 0 the first selectable row gets
focus (a `DONE_BUTTON` would take precedence; none exists). So the in-game Display screen always
opens on row 1, like the frontend. This resolves SPEC_GUI §11.4.

## 5. Display / Customize / hub screens: what the application drives [static]

| Screen | On enter | Header text | Accept/route | Back | Exit length |
|---|---|---|---|---|---|
| `UI_Options.fng` (Display) | rows built (SPEC_GUI §3), focus row 1 | `Header_Text` (`0x42ADB44C`) ← `0x4204D527` | Customize row: run exit script, at exit-complete switch to Customize; Default row: dialog (§6) | run exit script; at exit-complete write registry, switch to `UI_OptionsMain.fng` | 140 ms |
| `UI_PauseOptions.fng` (page 1) | rows built, focus row 1 | `Header_text` ← `0x1D7BB6C9`, `Header_text2` ← `0xEF1BB23B` | as above; target `UI_PC_Customize_Options.fng` | run `FORWARD`; at exit-complete switch to `UI_PauseOptionsMain.fng` | 320 ms |
| `UI_PC_Customize_Options.fng` | rows built (SPEC_GUI §4.1), focus row 1 | `Header_Text` ← `0x381FE1E9` | toggles per SPEC_GUI §2.7 (see §10 item 1) | exit, then the Display package of the context | 140 ms |
| `UI_PauseOptionsMain.fng` | 4 carousel entries (§2.5) | `HEADER_TEXT` ← `0x1D7BB6C9` | entry → page index, exit, `UI_PauseOptions.fng` | exit, `UI_pause.fng` | 320 ms |

Texture/resource sets per package (all keys = hash of the upper-case base name; all resolve in
`GLOBALB` except the `INGAME_ICON_*` set) are listed in the analyst evidence and in each package's
resource table; the implementation must load them from the package's own resource list rather than
from a hand-made list. [data]

## 6. Generic two-button dialogs [static; data]

Applies to `UI_InGameDialog.fng` (pause confirmations) and `GenericDialog_SMALL.fng` (Default
Settings), and to every dialog opened through the same mechanism.

* Objects: message `message_blurb` (`0x1E2640FA`), buttons `button1` (`0xB8A7C6CC`, centre
  (−151, 74) in `UI_InGameDialog`, (−150, 74) in `GenericDialog_SMALL`) and `Button2`
  (`0xB8A7C6CD`, (151, 74) / (150, 74)), button labels `0xDA473D2F` (first) and `0xDA473D30`
  (second) at (−158, 63)/(140, 63) resp. (−157, 63)/(145, 63); the third button `button3`
  (`0xB8A7C6CE`) and its label `0xDA473D31` are hidden and disabled for two-button dialogs.
  Each button group is a 214×42 box (`U2_Menu_Box_Layer1.tga`) with two 42×42 end caps
  (`U2_Menu_Text_Corner.tga`) at ±128 → authored extent x ±(128+21) around the button centre,
  y 53…95. This answers SPEC_GUI §11 "dialog button geometry".
* **Initial focus: the second button** (`Button2`) for every two-button dialog [static]. The
  dialog-call argument that SPEC_GUI §6 read as "default index 0" is not used for focus. This
  agrees with the user's retail frontend quit-dialog screenshot (focus on the right button) and
  with the authored entry colours (second label lime, first grey).
* Navigation (authored responses): Right on `button1` → focus `Button2`; Left on `Button2` →
  focus `button1`; the outward directions do nothing — **no wrap**. Up/Down: nothing. [data]
* Activation of a button posts `0x0C407210` with that button; the application then plays the
  dialog's exit (140 ms) and delivers the button's message (first or second) to the screen that
  opened the dialog. Back posts the cancel message (the second argument for pause dialogs
  `0xB4623F67`; for Default Settings Back = No `0x34DC1BCF`) [static].
* Focus visuals [data]: focused button group alpha 225, unfocused 80; focused label 1200 ms looping
  lime pulse, unfocused label grey (192,192,192).

## 7. Resolved SPEC_GUI §11 items

| §11 item | Resolution | Tag |
|---|---|---|
| 1 row offsets | §4.2/§4.3 formulas; cross-checked on the Audio screenshot | static + observed |
| 2 scrollbar tab ends/size | §4.5 | static |
| 2 click on scrollbar arrows, wheel, hover | not resolved | — |
| 4 in-game initial focus | row 1 (§4.7) | static |
| 5 dialog default focus / wrap | second button; no wrap (§6) | static + data |
| 7 exit length | §1.4 table | data |
| 10 click on row label | activates button rows (§4.6) | static |

## 8. Implementation-facing checklist (what "authored rendering" requires)

1. Load the package (SPEC_GUI §1.3), its resources (textures by key, font
   `ConduitMdITC_TT21i.ffn`), run `Init` scripts and chains, evaluate tracks every frame.
2. Implement message responses (§1.1), script events, package key responses (§1.5) and the target
   lists.
3. Apply application property writes with the base-key rule (§1.2).
4. Drive: row binding and placement (§4), carousel (§3), header labels (§5), exit/enter via the
   handler (§1.4), dialogs (§6).
5. Never draw native boxes/text for these screens; objects with flag `0x40` (console-only) are not
   drawn on PC (SPEC_GUI §3.3).

## 9. Corrections to earlier documents (accepted in review; SPEC_GUI.md carries supersession notes)

1. SPEC_GUI §6 "Initial focus: the first button (Yes)" → the **second button ("No")** is focused
   when the Default Settings dialog opens (§6). [static; consistent with retail frontend dialog
   observation]
2. SPEC_GUI §5.1 "Back starts the exit animation … script `Forward`": the exit lasts 140 ms
   (frontend) / 320 ms (in game) until `0xE1FDE1D1` (§1.4). Additive.
3. Pause-menu QUIT_DIALOG (earlier pause-menu work, analyst default Q1.2/Q4.2): the retail pause
   quit confirmation uses **`UI_InGameDialog.fng`** with buttons **"OK" (`0x0639`) / "Cancel"
   (`0x541389E5`)** and message `0x6F4796A6` "Quit to Main Menu?" (career), not
   `IG_GenericDialog_SMALL.fng` with Yes/No. Default focus Cancel (agrees with the user decision
   there). [static]
4. Pause-menu BINDING §3 "catalogue unknown": resolved by §2.2; the slot-to-entry association is
   list order (entry k ↔ `option_k`) [static].

## 10. Unknowns / ambiguities

1. Delivery of the button-activation message `0x0C407210` to the event-handler object. The hub
   requires it (the application never starts the hub's exit itself, yet waits for exit-complete), so
   in the carousel packages activation starts the exit through the authored handler response
   [inferred]. The list packages have the same response; if it fired on every row activation, Accept
   on a Customize toggle row would also fade the screen out. Whether retail does this is unknown —
   capture request 1. Implementations should follow SPEC_GUI §2.7 for list rows until captured.
2. Pause screen appearance while a confirmation dialog is open (exit fade and dim are both
   triggered; final alpha unknown) and after Cancel (undim restores alpha) — capture request 2.
3. Meaning of mode values other than 1/4/8/`0x200`, of race-activity value 7, of the "racing
   sub-state 4" that selects the "All unsaved race progress will be lost" texts, of the per-player
   field in the Options condition, and of the extra online flag.
4. Difference between the two resume requests (Return to Explore Mode from free roam vs Resume
   Race); both resume gameplay.
5. Time unit of the carousel animation (seconds assumed).
6. Text-object anchor details (glyph placement within the anchor) follow the base text layout and
   were not re-derived here.
7. Hover-to-focus, mouse wheel and clicks on scrollbar arrows (SPEC_GUI §11.2) remain unknown.
8. Help-bar content in game (SPEC_GUI §11.3) unchanged.

## 11. Capture requests

1. Customize screen: focus a toggle row (e.g. Fog), press Enter; record whether the value flips and
   whether the screen fades.
2. Pause in a career race → Quit; screenshot after the dialog has fully appeared, then after Cancel.
3. Pause in a career race: screenshot of each carousel position (confirms §2.2 B order).
4. Pause in free roam: move to Replay (expect it to be skipped) and to Quit; screenshot.

## 12. Test vectors

`test-vectors-gui/tvg07_pause_entries.json` (catalogue per context), `tvg08_icon_carousel.json`
(layout, tween, edge fade, tab), `tvg09_rows_sliders.json` (row and slider placement),
`tvg10_dialogs.json` (dialog packages, texts, buttons, focus), `tvg11_transitions.json`
(enter/exit timing). All synthetic; see the README.
