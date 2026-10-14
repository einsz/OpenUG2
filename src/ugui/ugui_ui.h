/* ugui_ui.h -- widgets, focus/navigation, screens, hint bar. GL-free.
 *
 * Input model: abstract actions (SPEC 4.3) plus pointer events (SPEC 4.6).
 * Widgets only mutate their own state and draw into a UgCanvas; results are
 * reported through the UgCtx event queue. */
#ifndef UGUI_UI_H
#define UGUI_UI_H

#include "ugui_canvas.h"
#include "ugui_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Actions, cues, events                                               */
/* ------------------------------------------------------------------ */
typedef enum {
    UG_A_NONE = 0,
    UG_A_ACCEPT, UG_A_BACK, UG_A_UP, UG_A_DOWN, UG_A_LEFT, UG_A_RIGHT,
    UG_A_LTRIG, UG_A_RTRIG, UG_A_BTN0, UG_A_BTN1, UG_A_BTN2, UG_A_BTN3,
    UG_A_START, UG_A_HELP,
    /* informational pseudo actions used only by the hint bar */
    UG_A_INFO_WHEEL, UG_A_INFO_VALUE, UG_A_INFO_TABS, UG_A_INFO_DRAG,
    UG_A_INFO_ROTATE,
    /* pseudo action of the PC help bar's "Quit Game" pill (SPEC 7.4.1): reported to the
     * application as UG_EV_QUIT, never dispatched to widgets */
    UG_A_QUIT,
    UG_A_COUNT
} UgAction;

uint32_t ug_action_message(UgAction a);          /* SPEC 4.3 hashes (0 = none) */
const char *ug_action_name(UgAction a);
int ug_action_clickable(UgAction a);             /* usable as a click target */

/* Sound cue hooks, SPEC 9 */
typedef enum {
    UG_CUE_FOCUS, UG_CUE_UNFOCUS, UG_CUE_UP, UG_CUE_DOWN, UG_CUE_LEFT,
    UG_CUE_RIGHT, UG_CUE_SELECT, UG_CUE_BACK, UG_CUE_START, UG_CUE_WRONG,
    UG_CUE_SLIDER, UG_CUE_TOGGLE, UG_CUE_SCROLLBAR, UG_CUE_ICON_SCROLLER,
    UG_CUE_BROWSER, UG_CUE_DATE_ENTRY, UG_CUE_STAT_CHANGE, UG_CUE_TEXT_INPUT,
    UG_CUE_KEY_NAV, UG_CUE_KEY_ENTER, UG_CUE_KEY_DELETE, UG_CUE_TRIGGER_L,
    UG_CUE_TRIGGER_R, UG_CUE_INSTALL, UG_CUE_BUTTON, UG_CUE_SUMMON_DIALOG,
    UG_CUE_SUMMON_HELP, UG_CUE_SUMMON_CUSTOMIZE, UG_CUE_COUNT
} UgCue;
const char *ug_cue_name(UgCue c);

typedef enum {
    UG_EV_NONE = 0,
    UG_EV_ACTIVATE,   /* a = item/row id, b = index */
    UG_EV_CHANGE,     /* a = item/row id, b = new value (slider: 0..1000) */
    UG_EV_BACK,       /* widget/screen asks to go back */
    UG_EV_HINT,       /* a = UgAction clicked on the hint bar */
    UG_EV_DIALOG,     /* a = button index chosen, source = dialog id */
    UG_EV_TEXT_DONE,  /* keyboard Done */
    UG_EV_DATE,       /* a = yyyymmdd */
    UG_EV_CUSTOMIZE,  /* car select Button1 */
    UG_EV_TOGGLE_HINTS,
    UG_EV_QUIT        /* PC help bar "Quit Game" pill clicked (the application asks for confirmation) */
} UgUiEventKind;

typedef struct { int kind, source, a, b; } UgUiEvent;

typedef enum { UG_PT_MOVE, UG_PT_DOWN, UG_PT_UP, UG_PT_WHEEL } UgPtrType;
typedef struct {
    int type;
    float x, y;          /* virtual coordinates */
    int button;          /* 1 left, 2 right */
    int wheel;           /* +1 up, -1 down */
} UgPtr;

typedef struct UgCtx {
    const UgFont *font;
    float time_ms;
    UgGlyphMode glyphs;
    const UgStrTab *strings;
    void (*cue_fn)(void *user, UgCue cue);
    void *cue_user;
    UgUiEvent q[64];
    int qh, qt;
    float px, py;
    int pointer_visible;
    UgCue last_cue;
} UgCtx;

void ug_ctx_init(UgCtx *c);
void ug_cue(UgCtx *c, UgCue cue);
void ug_post(UgCtx *c, int kind, int source, int a, int b);
int  ug_poll(UgCtx *c, UgUiEvent *out);

/* ------------------------------------------------------------------ */
/* Hints (SPEC 7.6)                                                    */
/* ------------------------------------------------------------------ */
typedef struct {
    UgAction action;
    UgGlyph g1, g2;          /* g2.id == NONE: single glyph */
    char label[28];
    int priority;            /* higher = kept longer */
    int clickable;
    int pc_skip;             /* PC help bar (7.4.1): never shown as a pill (tool / library-only hints) */
    char pc_label[24];       /* PC pill label; empty = derived (Accept "Continue", Back "Back", others the label) */
} UgHint;

#define UG_MAX_HINTS 12
typedef struct { UgHint h[UG_MAX_HINTS]; int n; } UgHintSet;

void ug_hints_clear(UgHintSet *s);
/* adds a hint using the standard mouse/key glyph pair of `action`; a hint
 * with an action already present is replaced */
void ug_hints_add(UgHintSet *s, UgAction action, const char *label, int priority);
void ug_hints_remove(UgHintSet *s, UgAction action);
void ug_hint_glyphs(UgAction a, UgGlyph *g1, UgGlyph *g2);
const UgHint *ug_hints_find(const UgHintSet *s, UgAction a);
/* PC help bar mapping of one hint (SPEC 7.4.1): `label` NULL keeps the derived label; skip = 1
 * keeps the hint off the PC bar (it stays on the mouse hint strip). */
void ug_hints_pc(UgHintSet *s, UgAction a, const char *label, int skip);

/* ------------------------------------------------------------------ */
/* Widget base                                                         */
/* ------------------------------------------------------------------ */
typedef struct UgWidget UgWidget;
typedef struct UgWidgetVT {
    void (*update)(UgWidget *, UgCtx *, float dt_ms);
    int  (*action)(UgWidget *, UgCtx *, UgAction);          /* 1 = consumed */
    int  (*pointer)(UgWidget *, UgCtx *, const UgPtr *);    /* 1 = consumed */
    void (*draw)(UgWidget *, UgCtx *, UgCanvas *);
    void (*focus)(UgWidget *, UgCtx *, int gained);
    void (*hints)(UgWidget *, UgHintSet *);
    void (*destroy)(UgWidget *);
} UgWidgetVT;

#define UG_W_HIDDEN      0x01u
#define UG_W_DISABLED    0x02u
#define UG_W_NOREACH     0x04u   /* not reachable by directional movement */
#define UG_W_NOFOCUS     0x08u   /* never takes focus (decor) */
#define UG_W_CAPTURE     0x10u   /* wants move/up events after a press */
#define UG_W_NOHOVERFOCUS 0x20u /* horizontal lists retain deliberate focus (20.10) */

struct UgWidget {
    const UgWidgetVT *vt;
    UgRect bounds;
    unsigned flags;
    int id;
    const char *kind;
    int focused;
};

int  ug_widget_focusable(const UgWidget *w);
void ug_widget_init(UgWidget *w, const UgWidgetVT *vt, const char *kind, int id, UgRect b);

/* shared drawing helpers (SPEC 5 visual kit) */
void ug_kit_text(UgCtx *c, UgCanvas *cv, const char *s, float x, float y,
                 float scale, UgColor col, unsigned just);
/* screen title of the options / car-select style pages (SPEC 6.5, 6.17, R): accent, scale 0.8,
 * left aligned, ink left x -288, cap top y -214 */
void ug_kit_title(UgCtx *c, UgCanvas *cv, const char *s);
void ug_kit_textw(UgCtx *c, UgCanvas *cv, const char *s, float x, float y,
                  float scale, UgColor col, unsigned just, float wrap);
void ug_kit_flat(UgCanvas *cv, UgRect r, UgColor c);
void ug_kit_corner(UgCanvas *cv, float cx, float cy, float sx, float sy, UgColor c);
void ug_kit_box(UgCanvas *cv, UgRect r, int light, float alpha01);    /* 60% box */
void ug_kit_panel(UgCanvas *cv, UgRect r, int light, float alpha01);  /* composite rounded panel */
void ug_kit_pill(UgCanvas *cv, UgRect r, UgColor fill, UgColor rim, float rim_w);
/* PC pill button (SPEC 5, 6.8): the original 256x32 art tinted by `tint`; missing art is unavailable */
void ug_kit_pill_pc(UgCanvas *cv, UgRect r, UgColor tint);
void ug_kit_pill_glow(UgCanvas *cv, UgRect r, UgColor tint);
/* black body with an end cap on each side (dialog buttons, strips); `tint` carries the alpha */
void ug_kit_cap_box(UgCanvas *cv, UgRect r, float cap_w, UgColor tint, UgColor fallback_rim, float rim_w);
/* scroll-arrow badge: dir 0 left 1 right 2 up 3 down */
void ug_kit_badge(UgCanvas *cv, float cx, float cy, float size, int dir, UgColor c, int solid);
void ug_kit_frame(UgCanvas *cv, UgRect r, UgColor c, float alpha01);  /* highlight frame */
void ug_kit_lock(UgCanvas *cv, float cx, float cy, float size, UgColor c);
void ug_kit_symbol(UgCanvas *cv, int symbol, float cx, float cy, float size, UgColor c);
void ug_kit_cursor(UgCanvas *cv, float x, float y);
void ug_kit_slider_track(UgCanvas *cv, UgRect r);
#define UG_SYMBOLS 12

/* ------------------------------------------------------------------ */
/* Basic widgets                                                       */
/* ------------------------------------------------------------------ */
typedef struct {
    UgWidget w;
    char text[200];
    float scale;
    UgColor color;
    unsigned just;
    float wrap, leading;
    const char *label;       /* when set, looked up in ctx->strings by hash */
} UgLabel;
void ug_label_init(UgLabel *l, int id, const char *text, float x, float y,
                   float scale, UgColor c, unsigned just);

typedef struct {
    UgWidget w;
    int light, mirrored_corners;
    float alpha;
} UgPanel;
void ug_panel_init(UgPanel *p, int id, UgRect r, int light, float alpha);

typedef enum { UG_BTN_DIALOG, UG_BTN_PILL, UG_BTN_HINT } UgButtonStyle;
typedef struct {
    UgWidget w;
    int style;
    char label[64];
    UgAnim pulse;    /* focus pulse (dialog) / pill tint (pill) */
    UgAnim glow;     /* pill glow overlay */
    UgAnim press;    /* blink on press */
    float label_scale;
    int pressed;
    int post_kind;              /* 0: UG_EV_ACTIVATE (source = id) */
    int post_source;
} UgButton;
void ug_button_init(UgButton *b, int id, int style, UgRect r, const char *label);

typedef struct {
    UgWidget w;
    float value;       /* 0..1 */
    int vertical;
    int steps;         /* keyboard step = 1/steps */
    int dragging;
    UgAnim pulse;
} UgSlider;
void ug_slider_init(UgSlider *s, int id, UgRect track, float value);
void ug_slider_set(UgSlider *s, float v);
float ug_slider_value_at(const UgSlider *s, float x, float y);   /* pointer -> value */
void ug_slider_draw_at(UgCanvas *cv, UgRect track, float value, int vertical,
                       int focused, float alpha01, const UgAnim *pulse);
/* same with the green fill height and the margin between the track end and the fill start / knob
 * travel start (option-row slider of SPEC 6.6: fill 7 high; the fill starts at track.x + margin) */
void ug_slider_draw_ex(UgCanvas *cv, UgRect track, float value, int vertical, int focused,
                       float alpha01, const UgAnim *pulse, float fill_h, float margin);

typedef struct {
    UgRect track;
    int vertical;
    int count, index;
    int page;               /* click on track jumps by this many */
    int with_arrows;
    int dragging;
    UgAnim blink_a, blink_b;
    float arrow_gap;        /* arrows sit this far beyond the track ends */
    float arrow_size;
    float inset;            /* thumb centre travels from track start + inset to end - inset (default 8) */
    int has_range;          /* explicit thumb centre range along the axis (SPEC 6.4: -273.7 .. +273.8) */
    float lo, hi;
    int has_cross;          /* explicit thumb centre across the axis (SPEC 6.4: y -171) */
    float cross;
    int hide_a, hide_b;     /* arrow at the start / end hidden (no item on that side, 4.5) */
} UgScroller;
void ug_scroller_init(UgScroller *s, UgRect track, int vertical, int arrows);
void ug_scroller_set(UgScroller *s, int index, int count);
void ug_scroller_update(UgScroller *s, float dt);
void ug_scroller_draw(UgScroller *s, UgCanvas *cv, float alpha01);
/* returns 1 and sets *new_index when the pointer changes the index */
int  ug_scroller_pointer(UgScroller *s, const UgPtr *p, int *new_index);
void ug_scroller_free(UgScroller *s);
void ug_scroller_blink(UgScroller *s, int toward_end);
float ug_scroller_thumb_pos(const UgScroller *s);     /* centre along the track */
/* centres of the start / end arrow badges (up/left, down/right) */
void ug_scroller_arrows(const UgScroller *s, float *ax, float *ay, float *bx, float *by);

typedef struct {
    UgWidget w;
    UgScroller sc;
    int id_base;
} UgScrollerWidget;
void ug_scroller_widget_init(UgScrollerWidget *w, int id, UgRect track, int vertical, int count);

#define UG_MAX_CATS 8
typedef struct {
    UgWidget w;
    int light; /* demo light-panel text palette */
    char cats[UG_MAX_CATS][24];
    int n, index, wrap;
    UgAnim blink_l, blink_r;
    float text_dx;          /* caption offset from the centre (SPEC 6.17: caption x 185 between arrows 95 / 268) */
} UgSwitcher;
void ug_switcher_init(UgSwitcher *s, int id, float cx, float cy, float width,
                      const char *const *cats, int n);
int  ug_switcher_step(UgSwitcher *s, UgCtx *c, int dir);

typedef struct {
    UgWidget w;
    const char *labels[3];
    float target[3], shown[3];
    float from[3], t[3];
    int n;
    float spacing;
    int absolute;           /* 1: SPEC 6.11 measured positions (labels / fills fixed on the screen) */
} UgStatBars;
void ug_statbars_init(UgStatBars *s, int id, float cx, float cy, const char *const *labels, int n);
/* the measured car-select layout of SPEC 6.11 / 6.17 (labels at y ~162.5, fills at y 176) */
void ug_statbars_init_measured(UgStatBars *s, int id, const char *const *labels, int n);
typedef struct { float label_x, label_y, fill_x0, fill_y, fill_len, fill_h; } UgStatBarGeom;
void ug_statbars_geom(const UgStatBars *s, int i, UgStatBarGeom *g);
void ug_statbars_set(UgStatBars *s, const float *values);   /* animates 300 ms */

typedef struct {
    UgWidget w;
    int cols, rows;
    UgColor *colors;      /* cols*rows */
    int sel;
    float size[64];       /* current drawn cell size */
    float start[64], t[64];
    float hx, hy;         /* highlight centre (animated) */
    UgAnim breathe;
    float pitch;          /* cell pitch */
    float px, py;         /* origin of cell 0 centre */
} UgGrid;
void ug_grid_init(UgGrid *g, int id, float cx, float cy, int cols, int rows, float pitch);
void ug_grid_select(UgGrid *g, int idx);

typedef struct {
    UgWidget w;
    UgAnim arrows[3];
    float spin;           /* turns */
    int show_spinner, show_arrows;
} UgBusy;
void ug_busy_init(UgBusy *b, int id, float cx, float cy);

void ug_movie_draw(UgCanvas *cv, UgCtx *c, UgRect r, const char *label);

/* ------------------------------------------------------------------ */
/* Lists                                                               */
/* ------------------------------------------------------------------ */
#define UG_MAX_ICONS 24
typedef struct { char label[32]; int symbol; int locked; int id; int tex; } UgIconItem;   /* tex: icon texture (library index + 1), 0 = procedural symbol */
typedef struct {
    UgWidget w;
    UgIconItem items[UG_MAX_ICONS];
    int n, sel;
    float pos;              /* animated selection position */
    int wrap;
    char title[48];
    UgScroller sb;
    UgAnim breathe, blink_l, blink_r, wrong;
    float last_px;
} UgIconScroller;
void ug_icons_init(UgIconScroller *s, int id, const char *title);
int  ug_icons_add(UgIconScroller *s, const char *label, int symbol, int locked, int id);
void ug_icons_select(UgIconScroller *s, int idx);

/* Icon strip geometry (SPEC 6.4, amendment 3, R). `pos` is the (animated) selection position, so
 * item `item` lies d = item - pos slots from the centre. At rest (pos integral) the selected icon is
 * 118 x 59 at the slot, the neighbours (|d| = 1) are 96 x 48 at +-158 at full opacity, nothing else
 * is visible. In between, size and distance interpolate; beyond |d| = 1 the icon slides outwards and
 * fades (UG_TUNE_ICON_FADE_DIST, G). Pure function, shared by the widget and the original-screen
 * viewer. */
typedef struct { int item; float cx, cy, w, h, alpha; } UgIconPlace;
void ug_icon_place(float pos, int item, float slot_cx, float slot_cy, UgIconPlace *out);
typedef struct {
    UgRect selected;                 /* drawn rect of the selected icon */
    int nnb, nb_item[2];             /* neighbours that exist (item index), left first */
    UgRect nb[2];
    UgRect frame;                    /* rectangle through the frame's line centres (120 x 55) */
    float arrow_y, arrow_lx, arrow_rx;
    int arrow_l, arrow_r;            /* 1 = badge shown (an item exists on that side) */
    UgRect band, track;              /* light icon band, scroller track */
    float thumb_x, thumb_y;
    float title_y;                   /* item title text centre (accent, scale 0.7, centred) */
} UgIconLayout;
void ug_icons_layout(int n, int sel, UgIconLayout *out);
/* the scroller thumb formula of 6.4: centre x = -273.7 + 547.4 * index / (n - 1) */
float ug_icons_thumb_x(int n, int index);
float ug_icons_thumb_xf(int n, float index);        /* same, fractional index (animated selection) */

typedef enum { UG_ROW_ACTION, UG_ROW_SELECTOR, UG_ROW_TOGGLE, UG_ROW_SLIDER } UgRowKind;
#define UG_MAX_ROWS 40
#define UG_MAX_VALUES 8
typedef struct {
    char name[40];
    int kind;
    char values[UG_MAX_VALUES][20];
    int nvalues, sel;
    float slider;
    int disabled, id;
    float backing;          /* animated backing alpha 80..200 */
} UgOptionRow;
#define UG_OL_PLAIN    0   /* options page (audio settings): 8 rows, name scale 0.7 */
#define UG_OL_CONTROLS 1   /* PC controls page: header band, 7 rows, name scale 0.5 */
typedef struct {
    UgWidget w;
    UgOptionRow rows[UG_MAX_ROWS];
    int n, cur, first, visible;
    UgScroller sb;
    UgAnim pulse, blink_l, blink_r;
    int drag_row;
    char name_header[24], value_header[24];
    int style;
    char caption[32];       /* controls page: caption in the dark header band */
} UgOptionList;
void ug_options_init(UgOptionList *o, int id);
void ug_options_set_style(UgOptionList *o, int style);
/* Row geometry (SPEC 6.5 / 6.6, R) of the k-th visible row (k = 0 .. visible - 1). */
typedef struct {
    float text_cy;                   /* centre of the name / value text */
    UgRect backing;                  /* the focused row's backing (unfocused rows draw none) */
    float name_x, name_scale;        /* left anchor of the name text */
    float value_cx, value_scale;     /* centre of the value text */
    float arrow_lx, arrow_rx, arrow_cy;   /* value arrows, focused row only */
    UgRect slider_track;             /* rect handed to the slider drawer */
    float slider_x0, slider_x1;      /* fill start and knob centre at the maximum value */
    float slider_cy;
} UgOptionRowGeom;
void ug_options_row_geom(const UgOptionList *o, int k, UgOptionRowGeom *g);
/* the thumb centre y of the vertical scroller for the current row */
float ug_options_thumb_y(const UgOptionList *o);
int  ug_options_add(UgOptionList *o, const char *name, int kind, int id,
                    const char *const *values, int nvalues);
void ug_options_set_disabled(UgOptionList *o, int row, int disabled);

#define UG_MAX_COLS 4
typedef struct {
    UgWidget w;
    char head[UG_MAX_COLS][20];
    float colx[UG_MAX_COLS];
    unsigned colj[UG_MAX_COLS];
    int ncols;
    char cell[32][UG_MAX_COLS][24];
    int n, cur, first, visible;
    UgScroller sb;
    UgAnim pulse;
} UgTable;
void ug_table_init(UgTable *t, int id);
void ug_table_column(UgTable *t, const char *head, float x, unsigned just);
int  ug_table_add_row(UgTable *t, const char *c0, const char *c1, const char *c2, const char *c3);

typedef struct { const char *maker, *name; float stats[3]; int category; UgColor paint; } UgCar;
typedef struct {
    UgWidget w;
    const UgCar *cars; int ncars;
    int list[16], nlist, cur;       /* filtered indices */
    UgSwitcher filter;
    UgStatBars bars;
    UgAnim blink_l, blink_r, pulse;
    UgScroller sb;                  /* horizontal scroller, y -192..-184 */
    float slide;                    /* car art slide animation 0..1 */
    int slide_dir;
} UgCarSelect;
void ug_carselect_init(UgCarSelect *c, int id, const UgCar *cars, int n,
                       const char *const *cats, int ncats);
/* Carousel geometry (SPEC 6.17, R): no wrap, the arrow on a side with no car is hidden. */
typedef struct {
    int left_arrow, right_arrow;     /* 1 = shown */
    float left_x, right_x, arrow_y;  /* badge centres */
    float thumb_x, thumb_y;
    UgRect track;
    UgRect logo_frame;               /* manufacturer frame, outer */
    float badge_x, badge_y;          /* car-name badge centre */
    float filter_l, filter_r, filter_y, filter_text_x;
} UgCarLayout;
void ug_carselect_layout(const UgCarSelect *c, UgCarLayout *out);

/* ------------------------------------------------------------------ */
/* Entry widgets                                                       */
/* ------------------------------------------------------------------ */
typedef struct {
    UgWidget w;
    char text[48];
    int len, max, caret;
    float blink;
    int password;
} UgTextField;
void ug_textfield_init(UgTextField *t, int id, UgRect r, int max);
void ug_textfield_insert(UgTextField *t, const char *utf8);
void ug_textfield_backspace(UgTextField *t);
void ug_textfield_draw_at(UgTextField *t, UgCtx *c, UgCanvas *cv, UgRect r, float scale, int active);

#define UG_MAX_KEYS 64
typedef enum { UG_KB_CHAR, UG_KB_SPACE, UG_KB_CAPS, UG_KB_SYMBOLS, UG_KB_BACKSPACE, UG_KB_DONE } UgKbKind;
typedef struct {
    int kind;
    char lower[12], upper[12], sym[12];
    UgRect r;
    int link[4];              /* explicit up,down,left,right key indices */
    float flash;
} UgKbKey;
typedef struct {
    UgWidget w;
    UgKbKey keys[UG_MAX_KEYS];
    int nkeys, cur;
    int caps, symbols;
    UgTextField *target;
    UgAnim pulse;
} UgKeyboard;
void ug_keyboard_init(UgKeyboard *k, int id, float cx, float top, UgTextField *target);
void ug_keyboard_type(UgKeyboard *k, UgCtx *c, const char *utf8);   /* physical keyboard */

typedef struct {
    UgWidget w;
    int month, day, year;
    int field;                /* 0 month 1 day 2 year */
    UgAnim pulse, blink_u, blink_d;
} UgDateEntry;
void ug_date_init(UgDateEntry *d, int id, float cx, float cy, int y, int m, int day);

/* ------------------------------------------------------------------ */
/* Screens, dialogs and the UI stack                                   */
/* ------------------------------------------------------------------ */
#define UG_MAX_WIDGETS 56
typedef struct UgScreen UgScreen;
struct UgScreen {
    const char *title;
    UgWidget *w[UG_MAX_WIDGETS];
    int n, focus;
    UgWidget *capture;
    unsigned flags;
    UgHintSet hints;
    void *user;
    void (*draw_bg)(UgScreen *, UgCtx *, UgCanvas *);
    void (*update)(UgScreen *, UgCtx *, float dt);
    int  (*on_action)(UgScreen *, UgCtx *, UgAction);   /* unconsumed actions */
    void (*on_event)(UgScreen *, UgCtx *, const UgUiEvent *);
    void (*refresh_hints)(UgScreen *, UgHintSet *);     /* page hints (after widget hints) */
    UgAnim fade, dim;
    int dim_state;
    int default_focus;     /* widget index that takes focus on entry; -1 = the first focusable (dialogs: the
                              negative / right button, SPEC 6.9) */
};
#define UG_SCR_NOBACKHINT 0x8u   /* no Back entry in the hints (PC main menu: Back does nothing, 7.4.1) */
#define UG_SCR_MODAL 0x1u
#define UG_SCR_ROOT  0x2u     /* Back does nothing (SPEC 10.10) */
#define UG_SCR_NOFADE 0x4u    /* the page animates itself: no screen-level fade-in / fade-out */

void ug_screen_init(UgScreen *s, const char *title, unsigned flags);
void ug_screen_free(UgScreen *s);
void ug_screen_add(UgScreen *s, UgWidget *w);
UgWidget *ug_screen_focused(UgScreen *s);
void ug_screen_set_focus(UgScreen *s, UgCtx *c, int index);
void ug_screen_focus_widget(UgScreen *s, UgCtx *c, UgWidget *w);
int  ug_screen_action(UgScreen *s, UgCtx *c, UgAction a);
int  ug_screen_pointer(UgScreen *s, UgCtx *c, const UgPtr *p);
void ug_screen_update(UgScreen *s, UgCtx *c, float dt);
void ug_screen_draw(UgScreen *s, UgCtx *c, UgCanvas *cv);
void ug_screen_collect_hints(UgScreen *s, UgHintSet *out);
/* directional focus movement (SPEC 4.5); returns the new index or -1 */
int  ug_screen_nav_target(const UgScreen *s, UgAction dir);

typedef struct {
    UgScreen scr;
    UgButton btn[3];
    int nbtn, size;           /* 0 small 1 medium 2 large */
    int back_result, id;
    char title[48], message[256];
    int icon;                 /* UG_SYMBOLS index or -1 */
    UgRect panel;
} UgDialog;
void ug_dialog_init(UgDialog *d, int id, int size, const char *title, const char *message,
                    const char *const *buttons, int nbtn);
/* Dialog geometry (SPEC 6.9, amendment 3, R): size 0 SMALL, 1 MED, 2 LARGE. */
typedef struct {
    UgRect panel;            /* dark panel, outer box (x -297 .. +297) */
    UgRect light;            /* light inner panel (x -290 .. +290) */
    UgRect border;           /* rectangle through the border line's centres (line ~2 thick) */
    float btn_cy;            /* button row centre */
    int nbtn;
    UgRect btn[3];           /* visible button extents */
    float msg_cx, msg_cy;    /* message block centre */
    float title_x, title_cy; /* title anchor (left), text centre y */
    float icon_cx, icon_cy;  /* 32 x 32 icon slot (when shown) */
} UgDialogGeom;
void ug_dialog_geom(int size, int nbtn, int has_icon, UgDialogGeom *g);

typedef struct {
    UgScreen scr;
    UgButton back;
    const char *lines[10];
    int nlines;
    UgRect panel;
} UgHelpPopup;
void ug_help_init(UgHelpPopup *h, int id, const char *const *lines, int n);

/* hint bar */
/* STRIP = console hint strip (7.1, y +206); PILLS = disabled historical layout;
 * PC = the original PC help bar as observed (7.4.1): 51-high strip, Quit / Continue / Back pills. */
typedef enum { UG_HINTS_STRIP = 0, UG_HINTS_PILLS = 1, UG_HINTS_PC = 2 } UgHintMode;

/* PC help bar model (SPEC 7.4, 7.4.1). Slot k of the 3 x 2 grid; slot 1 (index 0) is always the quit
 * entry, then the screen's forward action, then Back, then further actions. */
typedef struct { UgAction action; char label[28]; } UgPcPill;
typedef struct { UgPcPill p[6]; int n; } UgPcPills;
void ug_pc_assign(const UgHintSet *s, UgPcPills *out);
int  ug_pc_slot_center(int index, float *x, float *y);       /* index 0 = slot 1 */
typedef struct {
    float top, bottom;
    UgRect left_corner, left_box;           /* rounded end: 16 x 16 corner over a 16-wide box */
    UgRect player, hint;                    /* dark player box, hint box */
    UgRect right_corner, right_box;
    float player_cx, player_cy;             /* player name centre, scale 0.5 */
    int player_alpha, hint_alpha;           /* alpha of the black 60% art after the object's tint */
} UgPcStripGeom;
void ug_pc_strip_geom(UgPcStripGeom *g);
/* visible outline of pill `index` and the label centre (slot y - 2) */
void ug_pc_pill_geom(int index, UgRect *outline, float *label_cx, float *label_cy);

typedef struct {
    UgHintSet set;
    UgPcPills pc;                  /* PC mode: pills in slot order */
    int mode;
    UgRect rect[UG_MAX_HINTS];
    int shown[UG_MAX_HINTS];       /* hint index per rect, -1 unused */
    int nshown;
    int use_g2;                    /* mouse+key glyph pairs fit */
    int hover;                     /* rect index or -1 */
    UgAnim hover_pulse, hover_glow;
    char player[24], cash[24];
    UgAnim swap_name, swap_cash;
    float swap_timer;
    int showing_name;
    float alpha;
    int no_bg;                     /* the page draws its own bottom strip (original screens) */
} UgHintBar;
void ug_hintbar_init(UgHintBar *h);
void ug_hintbar_free(UgHintBar *h);
void ug_hintbar_set(UgHintBar *h, const UgHintSet *s);
void ug_hintbar_layout(UgHintBar *h, UgCtx *c);
void ug_hintbar_update(UgHintBar *h, UgCtx *c, float dt);
void ug_hintbar_draw(UgHintBar *h, UgCtx *c, UgCanvas *cv);
/* returns the clicked action or UG_A_NONE; *over = 1 when pointer is on a hint */
UgAction ug_hintbar_pointer(UgHintBar *h, UgCtx *c, const UgPtr *p, int *over);

#define UG_STACK_MAX 8
typedef struct UgUi UgUi;
struct UgUi {
    UgCtx ctx;
    UgCanvas cv;
    UgScreen *stack[UG_STACK_MAX];
    int sp;
    UgHintBar hints;
    /* pending transition */
    int pend;                      /* 0 none, 1 push, 2 pop */
    UgScreen *pend_scr;
    float pend_t;
    int hide_hints;
    UgUiEvent appq[32]; int aqh, aqt;
    /* optional hooks: background before everything, overlay before the cursor */
    void (*draw_back)(UgUi *u, void *user);
    void (*draw_front)(UgUi *u, void *user);
    void *hook_user;
};


void ug_ui_init(UgUi *u);
void ug_ui_free(UgUi *u);
void ug_ui_push(UgUi *u, UgScreen *s);   /* modal screens dim what is below */
void ug_ui_pop(UgUi *u);
UgScreen *ug_ui_top(UgUi *u);
int  ug_ui_busy(const UgUi *u);          /* transition in progress */
/* 1 when the bar is drawn: it is hidden while a modal dialog or popup is open (SPEC 6.9, 7.4.1) */
int  ug_ui_hints_visible(const UgUi *u);
void ug_ui_action(UgUi *u, UgAction a);
void ug_ui_pointer(UgUi *u, const UgPtr *p);
void ug_ui_text(UgUi *u, const char *utf8);       /* physical keyboard text */
void ug_ui_update(UgUi *u, float dt);
void ug_ui_draw(UgUi *u);                          /* fills u->cv */
/* events not consumed by the framework (activations, changes, dialogs ...) */
int  ug_ui_poll(UgUi *u, UgUiEvent *out);

#ifdef __cplusplus
}
#endif
#endif
