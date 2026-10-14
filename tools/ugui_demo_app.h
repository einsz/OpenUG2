/* ugui_demo_app.h -- GL-free application logic of the ugui demo.
 *
 * Builds every page of the SPEC 10 checklist out of the library widgets, so
 * the SDL/OpenGL front-end (ugui_demo.c) and the unit tests (ugui_test.c)
 * drive exactly the same code. */
#ifndef UGUI_DEMO_APP_H
#define UGUI_DEMO_APP_H

#include "ugui_fng.h"
#include "ugui_scene.h"
#include "ugui_ui.h"
#include "ugui_assets.h"

enum {
    DP_ORIGINAL = 0, DP_PANELS, DP_OPTIONS, DP_CONTROLS, DP_ENTRY, DP_DATE, DP_TABLE,
    DP_CARS, DP_DIALOGS, DP_ANIMS, DP_PACKAGE, DP_GLYPHS, DP_PCCTL, DP_LOCKED, DP_COUNT
};

/* widget ids used by tests */
enum {
    DID_ICONS = 100, DID_HINTMODE, DID_OPEN_HELP,
    DID_OPTIONS = 200,
    DID_ROW_DIFFICULTY = 210, DID_ROW_MUSIC, DID_ROW_SFX, DID_ROW_SUBTITLES,
    DID_ROW_LANGUAGE, DID_ROW_HINTBAR, DID_ROW_LOCKED, DID_ROW_INVERT,
    DID_ROW_RES, DID_ROW_RESET, DID_ROW_CREDITS, DID_ROW_APPLY, DID_ROW_DELETE,
    DID_ROW_EXTRA,
    DID_SLIDER = 300, DID_SCROLLER, DID_SWITCHER, DID_GRID, DID_BTN_DIALOG,
    DID_BTN_PILL, DID_BTN_RANDOM, DID_BUSY, DID_STATS,
    DID_KEYBOARD = 400, DID_TEXTFIELD, DID_DATE, DID_TABLE, DID_CARS,
    DID_DLG_BASE = 500, DID_QUIT_DIALOG = 515, DID_HELP = 520, DID_GALLERY = 530, DID_SCENE = 540,
    DID_GLYPHMODE = 550, DID_HINTMODE2 = 560,
    DID_BROWSER = 570, DID_VIEWER = 580
};

typedef struct {
    UgScreen scr;
    UgIconScroller icons;
    UgButton hint_toggle, help_btn;
} DemoRoot;

typedef struct { UgScreen scr; UgLabel hello, ok, price; UgButton glyph_btn; } DemoPanels;
typedef struct { UgScreen scr; UgOptionList list; } DemoOptions;
typedef struct { UgScreen scr; UgOptionList list; } DemoPcControls;
typedef struct {
    UgScreen scr;
    UgStatBars stats;
    UgSwitcher sw;
    UgGrid grid;
    UgSlider slider;
    UgScrollerWidget hscroll, vscroll;
    UgBusy busy;
    UgButton dlg_btn, pill_btn, rand_btn;
    unsigned rnd;
} DemoControls;
typedef struct { UgScreen scr; UgTextField field; UgKeyboard kb; } DemoEntry;
typedef struct { UgScreen scr; UgDateEntry date; int picked; } DemoDate;
typedef struct { UgScreen scr; UgTable table; } DemoTable;
typedef struct { UgScreen scr; UgCarSelect cars; } DemoCars;
typedef struct { UgScreen scr; UgButton b[6]; } DemoDialogs;
typedef struct { UgScreen scr; UgButton mode_btn; } DemoGlyphs;

#define GAL_N (UG_ANIM_COUNT)
typedef struct {
    UgWidget w;
    void *app;
    UgAnim a[GAL_N];
    UgAnim act[3];
    float idle[GAL_N];
    int sel;
} DemoGallery;
typedef struct { UgScreen scr; DemoGallery gal; } DemoAnims;

typedef struct {
    UgWidget w;
    UgFngPackage pk;
    UgScene sc;
    int loaded;
    char status[360];
    uint32_t log[6]; int nlog; int total;
    int example_hits;
} DemoSceneWidget;
typedef struct { UgScreen scr; DemoSceneWidget sw; UgButton example; } DemoPackage;

/* ---- "Original screens" page: browser and viewer (tools/ugui_demo_orig.c) ---- */
#define BR_MAX 512
typedef struct DemoApp DemoApp;

/* Application-side binding of an icon-scroller screen (SPEC 6.4, 15.9): the packages carry a pool of
 * off-screen item buttons, a placeholder slot and a title; the program assigns icons and positions. */
#define BIND_MAX 24
typedef struct {
    int active;
    int pool[BIND_MAX], npool;           /* node indices of the item pool */
    int slot, title, thumb, track;       /* node indices or -1 */
    int arrow_l, arrow_r;                /* scroll-arrow badge nodes or -1 (hidden when no item on that side) */
    float slot_x, slot_y;                /* the slot's authored centre (world) */
    int tex[BIND_MAX];                   /* icon textures (library index + 1) */
    char label[BIND_MAX][40];
    int n, sel;
    float pos;                           /* animated selection position */
    float last_px;
    char family[32];
} IconBind;

typedef struct {
    UgWidget w;
    DemoApp *app;
    int tab, sel, first_row;
    int list[BR_MAX], n;          /* indices into the assets' sorted package list */
    UgScroller sb;
    UgAnim pulse;
} DemoBrowser;
typedef struct { UgScreen scr; DemoBrowser br; } DemoOrigBrowser;

typedef struct {
    UgWidget w;
    DemoApp *app;
    UgFngPackage pk;
    UgScene sc;
    int loaded, index;            /* index into the assets' package list */
    int suppressed;              /* deliberate console-profile omission, never a load failure */
    char name[80], err[200];
    int objects, missing_tex;
    int closing; float close_t;   /* waiting for the exit animation before popping */
    int forwarding; float fwd_t;
    uint32_t last_msg; int n_msgs;
    int has_strip;
    IconBind bind;
} DemoViewer;
typedef struct { UgScreen scr; DemoViewer vw; } DemoOrigViewer;

struct DemoApp {
    UgUi ui;
    UgStrTab strings;
    DemoRoot root;
    DemoPanels panels;
    DemoOptions options;
    DemoPcControls pcctl;
    DemoControls controls;
    DemoEntry entry;
    DemoDate date;
    DemoTable table;
    DemoCars cars;
    DemoDialogs dialogs;
    DemoAnims anims;
    DemoPackage package;
    DemoGlyphs glyphs;
    UgDialog dlg;
    UgHelpPopup help;
    int dlg_live, help_live;
    char last_result[96];
    char vectors_dir[256];
    int diagnostics;              /* F3: separately identified tooling view */
    int log_cues;
    char cue_log[48];
    float cue_age;
    int dialog_results;
    UgHintMode hint_mode;
    int orig_hint_mode;            /* hint bar of the original-screen viewer (default: the PC help bar, 7.4.1) */
    int saved_hint_mode, orig_active_prev;
    int quit_requested;            /* the quit confirmation was answered "Yes" */
    /* original assets (loaded at run time from the user's install) */
    UgController controller;
    int faithful_ready;
    UgAssets assets;
    int have_assets;
    char assets_msg[200];
    DemoOrigBrowser orig;
    DemoOrigViewer viewer;
};

void demo_init(DemoApp *a, const char *vectors_dir);
/* Same, then loads the original assets from `data_dir` (NULL = $UGUI_DATA, ./). */
/* Language file of the install (default English); call before demo_init_data. */
void demo_set_language(const char *name);
void demo_init_data(DemoApp *a, const char *vectors_dir, const char *data_dir);
/* Opens the original screen at `index` in the assets' package list (viewer). */
int  demo_open_original(DemoApp *a, int index);
int  demo_open_original_named(DemoApp *a, const char *file_name);
/* orig: implemented in ugui_demo_orig.c */
void orig_init(DemoApp *a);
void orig_free(DemoApp *a);
void orig_assets_changed(DemoApp *a);
int  orig_open(DemoApp *a, int index);
void orig_post_frame(DemoApp *a);
int  orig_viewer_active(const DemoApp *a);
/* icon-strip binding of a viewer (SPEC 6.4); exposed so the tests can drive it on a hand-built scene */
int  orig_bind_detect(DemoViewer *v);
void orig_bind_set_items(DemoViewer *v, int n, const int *tex, const char (*labels)[40]);
void orig_bind_apply(DemoViewer *v, float dt);
int  orig_bind_step(DemoViewer *v, UgCtx *c, int dir);
void demo_cycle_hint_mode(DemoApp *a);
UgVisualStatus demo_select_controller(DemoApp *a, UgControllerFamily family);
void demo_free(DemoApp *a);
void demo_update(DemoApp *a, float dt_ms);
void demo_draw(DemoApp *a);
/* Opens page `index` (DP_*) from the root page as a click on its icon would. */
int  demo_open_page(DemoApp *a, int index);
const char *demo_page_name(int index);
UgScreen *demo_page_screen(DemoApp *a, int index);

#endif
