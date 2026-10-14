/* options_menu.h -- graphics Display and Customize option screens (state machine + native draw).
 *
 * Implements docs/cleanroom-settings/SPEC_GUI.md (section numbers in comments refer to it), on top
 * of the settings backend of SPEC.md (src/settings.h). The state machine is GL-free and SDL-free:
 * the host feeds semantic inputs and a frame time, polls events, and reads/writes the settings it
 * lent to the menu. Drawing (options_menu_draw) only appends to a UgCanvas; the host renders it.
 *
 * Two list screens (SPEC_GUI 1.1): Display (4 rows) and Customize (up to 22 rows + optional spacer).
 * Re-initialisation hook (SPEC_GUI 5.4): the menu never touches the renderer. Changes raise
 * "apply requested" / "display re-init requested" flags immediately; the host consumes them with
 * om_take_apply_request()/om_take_reinit_request() at the start of its next frame.
 * Design log: docs/cleanroom-settings/IMPL_DESIGN_GUI.md. */
#ifndef OPENUG2_OPTIONS_MENU_H
#define OPENUG2_OPTIONS_MENU_H

#include <stdint.h>
#include "settings.h"
#include "ugui/ugui_canvas.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OM_VISIBLE_ROWS 8       /* SPEC_GUI 2.3: V = 8 on all three screens */
#define OM_MAX_ROWS 24          /* 22 rows + spacer, with headroom (SPEC_GUI 2.2: <= 23) */
#define OM_ROW_PITCH 36.0f      /* SPEC_GUI 2.3 */

/* ---- list engine (SPEC_GUI 2.5), pure and shared by every screen ---------------------------- */
typedef struct OmList {
    int n;                       /* number of rows */
    int selectable[OM_MAX_ROWS];
    int focus;                   /* 0-based */
    int top;                     /* 0-based first visible row */
} OmList;
/* Initial window: <= V rows -> top 0; else top = focus when focus <= n-V, else n-V (0-based). */
void om_list_init(OmList *l, int n, const int *selectable, int initial_focus);
/* dir < 0 up, dir > 0 down. Non-wrapping; skips non-selectable rows; adjusts the window minimally.
 * Returns 1 when the focus moved. */
int om_list_move(OmList *l, int dir);
int om_first_selectable(const OmList *l);   /* 0-based; 0 when there is none */

/* ---- slider rule (SPEC_GUI 2.7) --------------------------------------------------------------- */
/* Steps *v by `dir` (<0 left, >0 right) within [min,max]. Returns 1 when the value really moved. */
int om_slider_step(int *v, int min, int max, int dir);
/* Fill fraction (v-min)/(max-min), 0 when the range is empty; v is clamped into the range first. */
float om_slider_fill(int v, int min, int max);

/* ---- identifiers and data keys ------------------------------------------------------------------ */
typedef enum { OM_CTX_FRONTEND = 0, OM_CTX_INGAME } OmContext;           /* SPEC_GUI 1.2 */
typedef enum { OM_SCREEN_CLOSED = 0, OM_SCREEN_DISPLAY, OM_SCREEN_CUSTOMIZE } OmScreen;
typedef enum { OM_ROW_CHOICE = 0, OM_ROW_SLIDER, OM_ROW_BUTTON, OM_ROW_SPACER } OmRowKind;  /* 2.1 */
typedef enum {                   /* what the row does */
    OM_ID_OPTION = 0,            /* an ordinary graphics option, `gfx` names its slot */
    OM_ID_RESOLUTION, OM_ID_LEVEL, OM_ID_CUSTOMIZE, OM_ID_DEFAULTS, OM_ID_SPACER
} OmRowId;
typedef enum {                   /* how the value text is produced (SPEC_GUI 3.1, 4.1) */
    OM_VT_NONE = 0, OM_VT_RESOLUTION, OM_VT_ONOFF, OM_VT_LOWHIGH, OM_VT_VSYNC
} OmValueText;

typedef struct OmRow {
    OmRowKind kind;
    OmRowId id;
    int slot;                    /* 1-based object slot n (SPEC_GUI 2.2) */
    uint32_t label_key;          /* language label key; 0 for a spacer */
    const char *label;           /* English text of that key */
    int selectable;
    int gfx;                     /* ST_GFX_* slot edited, or -1 */
    int min, max;                /* sliders */
    OmValueText vt;
    int needs_reinit;            /* a change raises the display re-init request (SPEC 4.4) */
} OmRow;

/* Label keys, SPEC_GUI 9 */
#define OM_KEY_ON 0x0000063Cu
#define OM_KEY_OFF 0x0000CCFAu
#define OM_KEY_VSYNC_OFF 0x88576CFFu
#define OM_KEY_LOW 0xB55E7678u
#define OM_KEY_HIGH 0x612AF8A6u
#define OM_KEY_TITLE_DISPLAY 0x4204D527u
#define OM_KEY_TITLE_PAUSE 0x1D7BB6C9u
#define OM_KEY_TITLE_PAUSE_DISPLAY 0xEF1BB23Bu
#define OM_KEY_TITLE_CUSTOMIZE 0x381FE1E9u
#define OM_KEY_DEFAULTS_MESSAGE 0x2BBCCA6Bu
#define OM_KEY_YES 0x0000F770u
#define OM_KEY_NO 0x0000061Cu

/* Package lookup names and keys (SPEC_GUI 1.2); NULL/0 when the screen is closed. */
const char *om_package_name(OmContext ctx, OmScreen screen);
uint32_t om_package_key(OmContext ctx, OmScreen screen);
const char *om_hub_package_name(OmContext ctx);          /* Back target of Display */
const char *om_dialog_package_name(void);                /* GenericDialog_SMALL.fng */

typedef enum {
    OM_IN_NONE = 0, OM_IN_UP, OM_IN_DOWN, OM_IN_LEFT, OM_IN_RIGHT, OM_IN_ACCEPT, OM_IN_BACK, OM_IN_START
} OmInput;

typedef enum {                   /* exactly-once events for the host */
    OM_EV_NONE = 0,
    OM_EV_LEAVE_TO_HUB,          /* Display left with Back: values were written (5.1); go to the hub */
    OM_EV_RESUME_GAME            /* in-game Start: resume without writing (5.2) */
} OmEvent;

typedef struct OmConfig {
    OmContext ctx;
    int row1_label_has_caret;    /* localized "Car Reflection Update Rate" contains '^' (4.1) */
    int cpu_mhz;                 /* inputs of the performance level (SPEC 4.5) */
    double gpu_score;
    float exit_ms;               /* exit animation of the Display screen (5.1) */
} OmConfig;
void om_config_default(OmConfig *c, OmContext ctx);   /* modern hardware: 3000 MHz, GPU score 5 */

typedef struct OptionsMenu {
    OmConfig cfg;
    StGraphics *gfx;             /* lent by the host; edited in place */
    const StGfxCaps *caps;       /* lent by the host */
    StRegStore *store;           /* lent by the host; NULL = nothing is persisted */
    OmScreen screen;
    OmRow rows[OM_MAX_ROWS];
    int nrows;
    OmList list;
    int res_shown;               /* resolution index whose text is displayed, -1 none (3.1) */
    int dialog_open, dialog_focus;
    int exiting;                 /* exit animation running (5.1) */
    OmScreen exit_to;            /* OM_SCREEN_CLOSED = leave to hub */
    float exit_left_ms;
    int apply_requested, reinit_requested;
    int registry_writes, last_write_status;
    OmEvent events[4];
    int nevents;
} OptionsMenu;

/* Opens the Display screen (SPEC_GUI 1.2). `remembered_focus` = 1-based row the screen framework
 * remembered, 0 = none; used in game only (2.5). gfx and caps must outlive the menu. */
void om_open(OptionsMenu *m, const OmConfig *cfg, StGraphics *gfx, const StGfxCaps *caps,
             StRegStore *store, int remembered_focus);
void om_close(OptionsMenu *m);                    /* drops everything without writing anything */
int om_is_open(const OptionsMenu *m);
OmScreen om_screen(const OptionsMenu *m);

void om_input(OptionsMenu *m, OmInput in);
/* Primary click at centred virtual coordinates (origin at the screen centre, 640x480). */
void om_pointer_click(OptionsMenu *m, float x, float y);
/* Advances the exit animation; returns after delivering any completed transition. */
void om_update(OptionsMenu *m, float seconds);
OmEvent om_poll_event(OptionsMenu *m);

/* Hook for the host/renderer (SPEC_GUI 5.4): consume-and-clear. */
int om_take_apply_request(OptionsMenu *m);        /* any graphics value changed */
int om_take_reinit_request(OptionsMenu *m);       /* ... and it needs a display reset */
int om_apply_pending(const OptionsMenu *m);
int om_reinit_pending(const OptionsMenu *m);
/* The host's display reset lowered the resolution (SPEC_GUI 5.5): the frontend Display list is
 * rebuilt so the Resolution row shows the new value; the in-game list is not refreshed. */
void om_notify_resolution_lowered(OptionsMenu *m);

/* Queries for drawing and tests. Rows and focus are 0-based here. */
int om_row_count(const OptionsMenu *m);
const OmRow *om_row(const OptionsMenu *m, int i);
int om_focus(const OptionsMenu *m);
int om_top(const OptionsMenu *m);
int om_row_visible(const OptionsMenu *m, int i);
int om_row_value(const OptionsMenu *m, int i);    /* displayed value (sliders clamped, SPEC_GUI 2.7) */
float om_row_fill(const OptionsMenu *m, int i);
/* Value text: *key = label key when it comes from the language file, else 0 with a literal text.
 * NULL when the row shows none (sliders, buttons, resolution index outside 0..5 before any text). */
const char *om_row_value_text(const OptionsMenu *m, int i, uint32_t *key);
int om_scrollbar_visible(const OptionsMenu *m);   /* more than one row (2.6) */
float om_scroll_tab(const OptionsMenu *m);        /* 0..1 along the track, follows the focused row */
const char *om_title(const OptionsMenu *m, uint32_t *key);

int om_dialog_is_open(const OptionsMenu *m);
int om_dialog_focus(const OptionsMenu *m);        /* 0 = Yes, 1 = No */
uint32_t om_dialog_button_key(int i);

/* Row centre y in centred virtual units for visible row k (SPEC_GUI 2.3). */
float om_row_center_y(int k);

/* Native drawing into centred virtual coordinates. Needs the host's text face to be current. */
void options_menu_draw(const OptionsMenu *m, UgCanvas *cv);

#ifdef __cplusplus
}
#endif
#endif
