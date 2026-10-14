/* options_menu.c -- graphics Display / Customize option screens (SPEC_GUI.md). See options_menu.h
 * and docs/cleanroom-settings/IMPL_DESIGN_GUI.md. The state machine part has no GL/SDL dependency;
 * options_menu_draw only appends triangles to a UgCanvas. */
#include "options_menu.h"

#include <string.h>

/* ---- static tables --------------------------------------------------------------------------- */

typedef struct CustRow {         /* SPEC_GUI 4.1, in list order */
    int gfx;
    uint32_t key;
    const char *label;
    OmRowKind kind;
    OmValueText vt;
    int domain_max;              /* sliders: upper bound of the value domain (SPEC 2.3); -1 = the cap only */
    int reinit;                  /* SPEC 4.4 re-initialisation list */
} CustRow;
static const CustRow k_customize[] = {
    { ST_GFX_CAR_ENV_MAP,        0x7D3930E7u, "Car Reflection Update Rate", OM_ROW_SLIDER, OM_VT_NONE,    3, 1 },
    { ST_GFX_CAR_ENV_MAP_UPDATE, 0x8FE9288Eu, "Car Reflection Detail",      OM_ROW_CHOICE, OM_VT_LOWHIGH, 0, 0 },
    { ST_GFX_CAR_SHADOW,         0xA7A683ACu, "Car Shadow / Neon",          OM_ROW_SLIDER, OM_VT_NONE,    2, 0 },
    { ST_GFX_CAR_HEADLIGHT,      0xCFBEDB50u, "Car Headlight",              OM_ROW_CHOICE, OM_VT_ONOFF,   0, 0 },
    { ST_GFX_CAR_LOD,            0x4AC50BCFu, "Car Geometry Detail",        OM_ROW_CHOICE, OM_VT_LOWHIGH, 0, 0 },
    { ST_GFX_CROWD,              0x06CC3750u, "Crowds",                     OM_ROW_CHOICE, OM_VT_ONOFF,   0, 0 },
    { ST_GFX_WORLD_LOD,          0xBDE2E94Bu, "World Detail",               OM_ROW_SLIDER, OM_VT_NONE,    3, 0 },
    { ST_GFX_ROAD_REFLECTION,    0x2A2A4AB3u, "Road Reflection Detail",     OM_ROW_SLIDER, OM_VT_NONE,    3, 1 },
    { ST_GFX_LIGHT_STREAKS,      0x6F2C7417u, "Light Trails",               OM_ROW_CHOICE, OM_VT_ONOFF,   0, 1 },
    { ST_GFX_LIGHT_GLOW,         0x62925361u, "Light Glow",                 OM_ROW_CHOICE, OM_VT_ONOFF,   0, 1 },
    { ST_GFX_PARTICLES,          0x2522A4E9u, "Particle System",            OM_ROW_CHOICE, OM_VT_ONOFF,   0, 0 },
    { ST_GFX_MOTION_BLUR,        0x822E4E9Bu, "Motion Blur",                OM_ROW_CHOICE, OM_VT_ONOFF,   0, 1 },
    { ST_GFX_FOG,                0x95B8F38Du, "Fog",                        OM_ROW_CHOICE, OM_VT_ONOFF,   0, 0 },
    { ST_GFX_DEPTH_OF_FIELD,     0xA9C27DFDu, "Depth Of Field",             OM_ROW_CHOICE, OM_VT_ONOFF,   0, 1 },
    { ST_GFX_FSAA,               0x4CD775ACu, "Full Screen Anti-Aliasing",  OM_ROW_SLIDER, OM_VT_NONE,   -1, 1 },
    { ST_GFX_TINTING,            0x11504D0Eu, "Tinting",                    OM_ROW_CHOICE, OM_VT_ONOFF,   0, 0 },
    { ST_GFX_HORIZON_FOG,        0x4E57D355u, "Horizon Fog",                OM_ROW_CHOICE, OM_VT_ONOFF,   0, 0 },
    { ST_GFX_OVER_BRIGHT,        0xD6F5F38Cu, "Over Bright",                OM_ROW_CHOICE, OM_VT_ONOFF,   0, 1 },
    { ST_GFX_BLEACH_BYPASS,      0x571F6C00u, "Enhanced Contrast",          OM_ROW_CHOICE, OM_VT_ONOFF,   0, 1 },
    { ST_GFX_RAIN,               0x4CDDBEBBu, "Rain Splatter",              OM_ROW_CHOICE, OM_VT_ONOFF,   0, 0 },
    { ST_GFX_TEXTURE_FILTERING,  0x61A50B8Fu, "Texture Filtering",          OM_ROW_SLIDER, OM_VT_NONE,    2, 1 },
    { ST_GFX_VSYNC,              0x5B4AA6E4u, "Vsync",                      OM_ROW_CHOICE, OM_VT_VSYNC,   0, 1 },
};
#define N_CUSTOMIZE ((int)(sizeof k_customize / sizeof k_customize[0]))

static const char *const k_resolution[6] = {            /* SPEC_GUI 3.1 (functional text) */
    "640x480", "800x600", "1024x768", "1280x960", "1280x1024", "1600x1200"
};

/* Layout, SPEC_GUI 2.3 (measured on the Audio page; centred virtual units). */
#define OM_ROW0_Y (-127.0f)
#define OM_BACK_X0 (-286.2f)
#define OM_BACK_X1 261.8f
#define OM_LABEL_X (-268.9f)
#define OM_VALUE_X 127.8f
#define OM_ARROW_LX 6.2f
#define OM_ARROW_RX 246.2f
#define OM_ARROW_HIT 14.0f           /* half width of an arrow's click area */
#define OM_SLIDER_X0 54.6f
#define OM_SLIDER_KNOB_X1 196.6f
#define OM_SLIDER_TRACK_X1 202.9f

/* ---- packages (SPEC_GUI 1.2) ------------------------------------------------------------------- */

const char *om_package_name(OmContext ctx, OmScreen screen) {
    if (screen == OM_SCREEN_CUSTOMIZE) return "UI_PC_Customize_Options.fng";
    if (screen == OM_SCREEN_DISPLAY) return ctx == OM_CTX_INGAME ? "UI_PauseOptions.fng" : "UI_Options.fng";
    return NULL;
}
uint32_t om_package_key(OmContext ctx, OmScreen screen) {
    if (screen == OM_SCREEN_CUSTOMIZE) return 0xC4ACC9A5u;
    if (screen == OM_SCREEN_DISPLAY) return ctx == OM_CTX_INGAME ? 0xAEA9078Fu : 0x551CA351u;
    return 0;
}
const char *om_hub_package_name(OmContext ctx) {
    return ctx == OM_CTX_INGAME ? "UI_PauseOptionsMain.fng" : "UI_OptionsMain.fng";
}
const char *om_dialog_package_name(void) { return "GenericDialog_SMALL.fng"; }

/* ---- list engine (SPEC_GUI 2.5) ----------------------------------------------------------------- */

static void list_fit_window(OmList *l) {
    while (l->focus >= l->top + OM_VISIBLE_ROWS) l->top++;
    while (l->focus < l->top) l->top--;
}
void om_list_init(OmList *l, int n, const int *selectable, int initial_focus) {
    memset(l, 0, sizeof *l);
    if (n > OM_MAX_ROWS) n = OM_MAX_ROWS;
    if (n < 0) n = 0;
    l->n = n;
    for (int i = 0; i < n; i++) l->selectable[i] = selectable ? selectable[i] : 1;
    if (initial_focus < 0) initial_focus = 0;
    if (n > 0 && initial_focus > n - 1) initial_focus = n - 1;
    l->focus = initial_focus;
    if (n <= OM_VISIBLE_ROWS) l->top = 0;
    else l->top = initial_focus <= n - OM_VISIBLE_ROWS ? initial_focus : n - OM_VISIBLE_ROWS;
}
int om_first_selectable(const OmList *l) {
    for (int i = 0; i < l->n; i++) if (l->selectable[i]) return i;
    return 0;
}
int om_list_move(OmList *l, int dir) {
    if (l->n == 0 || dir == 0) return 0;
    int step = dir < 0 ? -1 : 1, last = step > 0 ? l->n - 1 : 0, k = l->focus;
    if (k == last) return 0;
    k += step;
    while (k != last && !l->selectable[k]) k += step;      /* stop at a selectable row or at the end */
    l->focus = k;
    list_fit_window(l);
    return 1;
}

/* ---- slider rule (SPEC_GUI 2.7) ------------------------------------------------------------------- */

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
int om_slider_step(int *v, int min, int max, int dir) {
    int cur = clampi(*v, min, max), nv = cur;
    if (dir > 0) nv = cur + 1 > max ? max : cur + 1;
    else if (dir < 0) nv = cur - 1 < min ? min : cur - 1;
    if (nv == cur) return 0;
    *v = nv;
    return 1;
}
float om_slider_fill(int v, int min, int max) {
    if (max <= min) return 0.0f;
    return (float)(clampi(v, min, max) - min) / (float)(max - min);
}

/* ---- rows ----------------------------------------------------------------------------------------- */

void om_config_default(OmConfig *c, OmContext ctx) {
    memset(c, 0, sizeof *c);
    c->ctx = ctx;
    c->cpu_mhz = 3000;           /* CPU score 4, GPU score 5: SPEC 4.5 recommendation (P = 4) */
    c->gpu_score = 5.0;
    c->exit_ms = 140.0f;         /* frontend FadeOut figure of SPEC_GUI 2.4; the Forward script length is not specified */
}

static OmRow *add_row(OptionsMenu *m, OmRowKind kind, OmRowId id, uint32_t key, const char *label) {
    OmRow *r = &m->rows[m->nrows];
    memset(r, 0, sizeof *r);
    r->kind = kind;
    r->id = id;
    r->slot = ++m->nrows;
    r->label_key = key;
    r->label = label;
    r->selectable = kind != OM_ROW_SPACER;
    r->gfx = -1;
    return r;
}

static void refresh_res_text(OptionsMenu *m) {
    int r = m->gfx->v[ST_GFX_RESOLUTION];
    if (r >= 0 && r <= 5) m->res_shown = r;                  /* outside 0..5 the text is left unchanged */
}

static void build_display(OptionsMenu *m) {
    OmRow *r = add_row(m, OM_ROW_CHOICE, OM_ID_RESOLUTION, 0xA9A76BBAu, "Resolution");
    r->gfx = ST_GFX_RESOLUTION; r->vt = OM_VT_RESOLUTION; r->needs_reinit = 1;
    r = add_row(m, OM_ROW_SLIDER, OM_ID_LEVEL, 0xB55E7665u, "Level Of Detail");
    r->gfx = ST_GFX_PERF_LEVEL; r->min = 1; r->max = 5; r->needs_reinit = 1;
    add_row(m, OM_ROW_BUTTON, OM_ID_CUSTOMIZE, OM_KEY_TITLE_CUSTOMIZE, "Customize Display Settings");
    add_row(m, OM_ROW_BUTTON, OM_ID_DEFAULTS, 0x13F0EDEBu, "Default Settings");
}

static void build_customize(OptionsMenu *m) {
    for (int i = 0; i < N_CUSTOMIZE; i++) {
        const CustRow *c = &k_customize[i];
        int max = 1;
        if (c->gfx != ST_GFX_VSYNC) {                       /* Vsync has no cap and is always present */
            int cap = m->caps ? m->caps->cap[c->gfx] : ST_NO_CAP;
            if (cap <= 0) continue;                         /* SPEC_GUI 4.1: zero cap, no row */
            if (c->kind == OM_ROW_SLIDER) {
                if (c->domain_max < 0) { if (cap == ST_NO_CAP) continue; max = cap; }
                else max = cap < c->domain_max ? cap : c->domain_max;
            }
        }
        OmRow *r = add_row(m, c->kind, OM_ID_OPTION, c->key, c->label);
        r->gfx = c->gfx; r->vt = c->vt; r->needs_reinit = c->reinit;
        if (c->kind == OM_ROW_SLIDER) { r->min = 0; r->max = max; }
        if (i == 0 && m->nrows == 1 && m->cfg.row1_label_has_caret)
            add_row(m, OM_ROW_SPACER, OM_ID_SPACER, 0, NULL);   /* SPEC_GUI 4.1: blank row after row 1 */
    }
}

static void enter_screen(OptionsMenu *m, OmScreen screen, int remembered_focus) {
    m->screen = screen;
    m->nrows = 0;
    m->dialog_open = 0;
    m->dialog_focus = 0;
    m->exiting = 0;
    if (screen == OM_SCREEN_DISPLAY) build_display(m);
    else if (screen == OM_SCREEN_CUSTOMIZE) build_customize(m);
    int sel[OM_MAX_ROWS];
    for (int i = 0; i < m->nrows; i++) sel[i] = m->rows[i].selectable;
    om_list_init(&m->list, m->nrows, sel, 0);
    int focus = om_first_selectable(&m->list);
    if (m->cfg.ctx == OM_CTX_INGAME && screen == OM_SCREEN_DISPLAY && remembered_focus >= 1 &&
        remembered_focus <= m->nrows && m->rows[remembered_focus - 1].selectable)
        focus = remembered_focus - 1;
    om_list_init(&m->list, m->nrows, sel, focus);
    refresh_res_text(m);
}

void om_open(OptionsMenu *m, const OmConfig *cfg, StGraphics *gfx, const StGfxCaps *caps,
             StRegStore *store, int remembered_focus) {
    int apply = m->apply_requested, reinit = m->reinit_requested;   /* pending requests survive */
    int writes = m->registry_writes, status = m->last_write_status;
    memset(m, 0, sizeof *m);
    m->apply_requested = apply; m->reinit_requested = reinit;
    m->registry_writes = writes; m->last_write_status = status;
    m->cfg = *cfg;
    m->gfx = gfx;
    m->caps = caps;
    m->store = store;
    m->res_shown = -1;
    enter_screen(m, OM_SCREEN_DISPLAY, remembered_focus);
}
void om_close(OptionsMenu *m) {
    m->screen = OM_SCREEN_CLOSED;
    m->nrows = 0;
    m->dialog_open = 0;
    m->exiting = 0;
    m->nevents = 0;
}
int om_is_open(const OptionsMenu *m) { return m->screen != OM_SCREEN_CLOSED; }
OmScreen om_screen(const OptionsMenu *m) { return m->screen; }

static void push_event(OptionsMenu *m, OmEvent e) {
    if (m->nevents < (int)(sizeof m->events / sizeof m->events[0])) m->events[m->nevents++] = e;
}
OmEvent om_poll_event(OptionsMenu *m) {
    if (m->nevents == 0) return OM_EV_NONE;
    OmEvent e = m->events[0];
    for (int i = 1; i < m->nevents; i++) m->events[i - 1] = m->events[i];
    m->nevents--;
    return e;
}

/* ---- hook ------------------------------------------------------------------------------------------ */

int om_take_apply_request(OptionsMenu *m) { int v = m->apply_requested; m->apply_requested = 0; return v; }
int om_take_reinit_request(OptionsMenu *m) { int v = m->reinit_requested; m->reinit_requested = 0; return v; }
int om_apply_pending(const OptionsMenu *m) { return m->apply_requested; }
int om_reinit_pending(const OptionsMenu *m) { return m->reinit_requested; }

void om_notify_resolution_lowered(OptionsMenu *m) {
    /* SPEC_GUI 5.5: only the frontend Display list is rebuilt. */
    if (m->screen == OM_SCREEN_DISPLAY && m->cfg.ctx == OM_CTX_FRONTEND && !m->exiting)
        enter_screen(m, OM_SCREEN_DISPLAY, 0);
}

/* ---- queries ----------------------------------------------------------------------------------------- */

int om_row_count(const OptionsMenu *m) { return m->nrows; }
const OmRow *om_row(const OptionsMenu *m, int i) { return i >= 0 && i < m->nrows ? &m->rows[i] : NULL; }
int om_focus(const OptionsMenu *m) { return m->list.focus; }
int om_top(const OptionsMenu *m) { return m->list.top; }
int om_row_visible(const OptionsMenu *m, int i) {
    return i >= m->list.top && i < m->list.top + OM_VISIBLE_ROWS && i < m->nrows;
}
int om_row_value(const OptionsMenu *m, int i) {
    const OmRow *r = om_row(m, i);
    if (!r || r->gfx < 0) return 0;
    int v = m->gfx->v[r->gfx];
    return r->kind == OM_ROW_SLIDER ? clampi(v, r->min, r->max) : v;
}
float om_row_fill(const OptionsMenu *m, int i) {
    const OmRow *r = om_row(m, i);
    return r && r->kind == OM_ROW_SLIDER ? om_slider_fill(om_row_value(m, i), r->min, r->max) : 0.0f;
}
const char *om_row_value_text(const OptionsMenu *m, int i, uint32_t *key) {
    const OmRow *r = om_row(m, i);
    uint32_t k = 0;
    const char *t = NULL;
    if (r) {
        int v = om_row_value(m, i);
        switch (r->vt) {
        case OM_VT_RESOLUTION: if (m->res_shown >= 0) t = k_resolution[m->res_shown]; break;
        case OM_VT_ONOFF:   k = v ? OM_KEY_ON : OM_KEY_OFF; t = v ? "On" : "Off"; break;
        case OM_VT_LOWHIGH: k = v ? OM_KEY_HIGH : OM_KEY_LOW; t = v ? "High" : "Low"; break;
        case OM_VT_VSYNC:   k = v ? OM_KEY_ON : OM_KEY_VSYNC_OFF; t = v ? "On" : "Off"; break;
        default: break;
        }
    }
    if (key) *key = k;
    return t;
}
int om_scrollbar_visible(const OptionsMenu *m) { return m->nrows > 1; }
float om_scroll_tab(const OptionsMenu *m) {
    return m->nrows > 1 ? (float)m->list.focus / (float)(m->nrows - 1) : 0.0f;
}
const char *om_title(const OptionsMenu *m, uint32_t *key) {
    uint32_t k = 0;
    const char *t = NULL;
    if (m->screen == OM_SCREEN_CUSTOMIZE) { k = OM_KEY_TITLE_CUSTOMIZE; t = "Customize Display Settings"; }
    else if (m->screen == OM_SCREEN_DISPLAY) {
        if (m->cfg.ctx == OM_CTX_INGAME) { k = OM_KEY_TITLE_PAUSE_DISPLAY; t = "DISPLAY"; }
        else { k = OM_KEY_TITLE_DISPLAY; t = "Display"; }
    }
    if (key) *key = k;
    return t;
}
int om_dialog_is_open(const OptionsMenu *m) { return m->dialog_open; }
int om_dialog_focus(const OptionsMenu *m) { return m->dialog_focus; }
uint32_t om_dialog_button_key(int i) { return i == 0 ? OM_KEY_YES : OM_KEY_NO; }
float om_row_center_y(int k) { return OM_ROW0_Y + OM_ROW_PITCH * (float)k; }

/* ---- input ----------------------------------------------------------------------------------------------- */

static void changed(OptionsMenu *m, const OmRow *r) {
    m->apply_requested = 1;
    if (r->needs_reinit) m->reinit_requested = 1;
    refresh_res_text(m);
}

static void begin_exit(OptionsMenu *m, OmScreen to);

static void row_adjust(OptionsMenu *m, int dir) {
    const OmRow *r = &m->rows[m->list.focus];
    StGraphics *g = m->gfx;
    if (r->kind == OM_ROW_CHOICE && r->id == OM_ID_RESOLUTION) {
        int res = g->v[ST_GFX_RESOLUTION];
        if (res >= 0 && res <= 5 && m->caps && st_resolution_step(g, m->caps, dir)) changed(m, r);
    } else if (r->kind == OM_ROW_CHOICE) {
        g->v[r->gfx] = g->v[r->gfx] ? 0 : 1;               /* SPEC 4.4: flip 0 <-> 1 */
        changed(m, r);
    } else if (r->kind == OM_ROW_SLIDER) {
        int v = g->v[r->gfx];
        if (!om_slider_step(&v, r->min, r->max, dir)) return;
        if (r->id == OM_ID_LEVEL && m->caps) st_gfx_set_level(g, m->caps, v, m->cfg.gpu_score);
        else g->v[r->gfx] = v;
        changed(m, r);
    }
}

static void row_accept(OptionsMenu *m) {
    const OmRow *r = &m->rows[m->list.focus];
    if (r->kind == OM_ROW_CHOICE && r->id == OM_ID_OPTION) {
        m->gfx->v[r->gfx] = m->gfx->v[r->gfx] ? 0 : 1;
        changed(m, r);
    } else if (r->id == OM_ID_CUSTOMIZE) {
        begin_exit(m, OM_SCREEN_CUSTOMIZE);
    } else if (r->id == OM_ID_DEFAULTS) {
        m->dialog_open = 1;
        m->dialog_focus = 0;                                /* SPEC_GUI 6: first button (Yes) */
    }
}

static void write_registry(OptionsMenu *m) {
    if (!m->store) return;
    m->last_write_status = st_gfx_save(m->store, m->gfx);
    m->registry_writes++;
}

static void finish_exit(OptionsMenu *m) {
    OmScreen to = m->exit_to;
    m->exiting = 0;
    if (to == OM_SCREEN_CLOSED) {                           /* SPEC_GUI 5.1 */
        write_registry(m);
        om_close(m);
        push_event(m, OM_EV_LEAVE_TO_HUB);
    } else {
        enter_screen(m, to, 0);                             /* fresh build, focus on the first selectable row */
    }
}
static void begin_exit(OptionsMenu *m, OmScreen to) {
    if (m->exiting) return;                                 /* a second Back while exiting is ignored */
    m->exiting = 1;
    m->exit_to = to;
    m->exit_left_ms = m->cfg.exit_ms;
    if (m->exit_left_ms <= 0) finish_exit(m);
}

static void defaults_yes(OptionsMenu *m) {
    int level = st_perf_level(m->cfg.cpu_mhz, m->cfg.gpu_score);
    if (m->caps) st_gfx_autodetect(m->gfx, m->caps, level, m->cfg.gpu_score);
    m->apply_requested = 1;
    m->reinit_requested = 1;
    enter_screen(m, OM_SCREEN_DISPLAY, 0);                  /* SPEC_GUI 5.6: rebuild, focus row 1 */
}

static void dialog_input(OptionsMenu *m, OmInput in) {
    switch (in) {
    case OM_IN_LEFT:  m->dialog_focus = 0; break;           /* wrapping unknown (SPEC_GUI 11.5): clamped */
    case OM_IN_RIGHT: m->dialog_focus = 1; break;
    case OM_IN_ACCEPT:
        m->dialog_open = 0;
        if (m->dialog_focus == 0) defaults_yes(m);
        break;
    case OM_IN_BACK: m->dialog_open = 0; break;
    default: break;                                          /* Up/Down/Start never reach the dialog */
    }
}

void om_input(OptionsMenu *m, OmInput in) {
    if (!om_is_open(m) || m->exiting || m->nrows == 0) return;
    if (m->dialog_open) { dialog_input(m, in); return; }
    switch (in) {
    case OM_IN_UP:    om_list_move(&m->list, -1); break;
    case OM_IN_DOWN:  om_list_move(&m->list, 1); break;
    case OM_IN_LEFT:  row_adjust(m, -1); break;
    case OM_IN_RIGHT: row_adjust(m, 1); break;
    case OM_IN_ACCEPT: row_accept(m); break;
    case OM_IN_BACK:
        begin_exit(m, m->screen == OM_SCREEN_DISPLAY ? OM_SCREEN_CLOSED : OM_SCREEN_DISPLAY);
        break;
    case OM_IN_START:                                       /* SPEC_GUI 5.2: in-game Display only, no write */
        if (m->cfg.ctx == OM_CTX_INGAME && m->screen == OM_SCREEN_DISPLAY) {
            om_close(m);
            push_event(m, OM_EV_RESUME_GAME);
        }
        break;
    default: break;
    }
}

void om_pointer_click(OptionsMenu *m, float x, float y) {
    if (!om_is_open(m) || m->exiting || m->dialog_open) return;      /* the dialog is modal */
    for (int k = 0; k < OM_VISIBLE_ROWS; k++) {
        int i = m->list.top + k;
        if (i >= m->nrows) break;
        float cy = om_row_center_y(k);
        if (y < cy - OM_ROW_PITCH / 2 || y >= cy + OM_ROW_PITCH / 2) continue;
        const OmRow *r = &m->rows[i];
        if (r->kind == OM_ROW_BUTTON) {                      /* label click activates (SPEC_GUI 11.10, inferred) */
            if (x >= OM_BACK_X0 && x <= OM_BACK_X1) { m->list.focus = i; row_accept(m); }
        } else if (i == m->list.focus && (r->kind == OM_ROW_CHOICE || r->kind == OM_ROW_SLIDER)) {
            if (x >= OM_ARROW_LX - OM_ARROW_HIT && x <= OM_ARROW_LX + OM_ARROW_HIT) row_adjust(m, -1);
            else if (x >= OM_ARROW_RX - OM_ARROW_HIT && x <= OM_ARROW_RX + OM_ARROW_HIT) row_adjust(m, 1);
            else if (r->kind == OM_ROW_CHOICE && x > OM_ARROW_LX + OM_ARROW_HIT && x < OM_ARROW_RX - OM_ARROW_HIT)
                row_adjust(m, 1);                            /* value text acts as Right */
        }
        return;
    }
}

void om_update(OptionsMenu *m, float seconds) {
    if (!om_is_open(m) || !m->exiting) return;
    m->exit_left_ms -= seconds > 0 ? seconds * 1000.0f : 0.0f;
    if (m->exit_left_ms <= 0) finish_exit(m);
}

/* ---- drawing -------------------------------------------------------------------------------------------- */

static void draw_arrow(UgCanvas *cv, float cx, float cy, int dir, UgColor c) {
    float s = 8.0f;
    ug_cv_tri(cv, cx + dir * s, cy, cx - dir * s * 0.6f, cy - s, cx - dir * s * 0.6f, cy + s, c);
}
static void draw_vtri(UgCanvas *cv, float cx, float cy, int dir, UgColor c) {   /* dir -1 up, +1 down */
    float s = 6.0f;
    ug_cv_tri(cv, cx, cy + dir * s, cx - s, cy - dir * s * 0.6f, cx + s, cy - dir * s * 0.6f, c);
}

void options_menu_draw(const OptionsMenu *m, UgCanvas *cv) {
    if (!om_is_open(m)) return;
    int ingame = m->cfg.ctx == OM_CTX_INGAME;
    UgColor lime = UG_ACCENT, dark = UG_TEXT_DARK, white = UG_WHITE;
    UgColor text = ingame ? lime : dark;
    float scale = ingame ? 0.75f : 0.7f;

    ug_cv_fill(cv, (UgRect){-320, -240, 640, 480}, UG_RGBA(0, 0, 0, 120), 0);          /* dim the scene */
    if (ingame) {
        ug_cv_fill(cv, UG_RECT_C(1, 4, 562, 436), UG_RGBA(0, 0, 0, 235), 14);
        ug_cv_fill(cv, UG_RECT_C(1, 9, 548, 362), UG_RGBA(255, 255, 255, 25), 8);
        ug_cv_outline(cv, UG_RECT_C(1, 4, 562, 436), UG_BORDER, 14, 2);
    } else {
        ug_cv_fill(cv, UG_RECT_C(-1, -17, 562, 412), UG_RGBA(0, 0, 0, 220), 14);
        ug_cv_fill(cv, UG_RECT_C(-1, 1, 548, 364), UG_RGBA(235, 235, 235, 200), 8);
    }

    /* title (SPEC_GUI 3.2, 3.3) */
    uint32_t tk;
    const char *title = om_title(m, &tk);
    if (ingame && m->screen == OM_SCREEN_DISPLAY) {
        ug_text_draw(cv, "PAUSE OPTIONS", -287.7f, -208.3f, 0.75f, lime, UG_J_VCENTER, 0, 0, UG_GLYPHS_KEYS);
        ug_text_draw(cv, title, 288.3f, -208.3f, 0.75f, lime, UG_J_RIGHT | UG_J_VCENTER, 0, 0, UG_GLYPHS_KEYS);
    } else if (title) {
        ug_text_draw(cv, title, -289.7f, ingame ? -208.3f : -220.0f, ingame ? 0.75f : 0.8f, lime,
                     UG_J_VCENTER, 0, 0, UG_GLYPHS_KEYS);
    }

    /* rows; they dim behind the confirmation dialog */
    ug_cv_push(cv);
    if (m->dialog_open) ug_cv_mul_alpha(cv, 0.4f);
    for (int k = 0; k < OM_VISIBLE_ROWS; k++) {
        int i = m->list.top + k;
        if (i >= m->nrows) break;
        const OmRow *r = &m->rows[i];
        if (r->kind == OM_ROW_SPACER) continue;
        float cy = om_row_center_y(k);
        int focused = i == m->list.focus;
        UgColor c = text;
        if (focused) {
            if (!ingame) {
                ug_cv_fill(cv, (UgRect){OM_BACK_X0, cy - 17.5f, OM_BACK_X1 - OM_BACK_X0, 35.0f}, UG_RGBA(65, 65, 65, 200), 4);
                c = lime;
            } else {
                c = white;                                   /* no backing in game (SPEC_GUI 2.1) */
            }
        }
        ug_text_draw(cv, r->label, OM_LABEL_X, cy, scale, c, UG_J_VCENTER, 0, 0, UG_GLYPHS_KEYS);
        if (r->kind == OM_ROW_CHOICE) {
            const char *vt = om_row_value_text(m, i, NULL);
            if (vt) ug_text_draw(cv, vt, OM_VALUE_X, cy, scale, c, UG_J_CENTER | UG_J_VCENTER, 0, 0, UG_GLYPHS_KEYS);
        } else if (r->kind == OM_ROW_SLIDER) {
            float f = om_row_fill(m, i), kx = OM_SLIDER_X0 + f * (OM_SLIDER_KNOB_X1 - OM_SLIDER_X0);
            ug_cv_fill(cv, (UgRect){OM_SLIDER_X0, cy - 3.5f, OM_SLIDER_TRACK_X1 - OM_SLIDER_X0, 7.0f}, UG_RGBA(30, 30, 30, 200), 3);
            if (kx > OM_SLIDER_X0) ug_cv_fill(cv, (UgRect){OM_SLIDER_X0, cy - 3.5f, kx - OM_SLIDER_X0, 7.0f}, UG_RGBA(98, 178, 34, 255), 3);
            ug_cv_disc(cv, kx, cy, 6.0f, UG_RGBA(230, 230, 230, 255));
        }
        if (focused && (r->kind == OM_ROW_CHOICE || r->kind == OM_ROW_SLIDER)) {   /* arrows on the focused row only */
            UgColor a = UG_RGBA(255, 255, 255, 150);
            draw_arrow(cv, OM_ARROW_LX, cy, -1, a);
            draw_arrow(cv, OM_ARROW_RX, cy, 1, a);
        }
    }
    ug_cv_pop(cv);

    /* scrollbar: shown whenever there is more than one row; the tab follows the focused row (2.6) */
    if (om_scrollbar_visible(m)) {
        float sx = ingame ? 275.0f : 273.0f, sy = ingame ? 10.0f : 2.0f;
        float top = sy - 141.0f + 8.0f, span = 282.0f - 16.0f;
        ug_cv_fill(cv, UG_RECT_C(sx, sy, 8, 282), UG_RGBA(20, 20, 20, 170), 3);
        ug_cv_fill(cv, UG_RECT_C(sx, top + om_scroll_tab(m) * span, 12, 16), ingame ? lime : UG_RGBA(98, 178, 34, 255), 4);
        draw_vtri(cv, sx, ingame ? -154.0f : -164.0f, -1, text);
        draw_vtri(cv, sx, ingame ? 175.0f : 167.0f, 1, text);
    }

    /* hint line (SPEC_GUI 7: the PC help bar proper is the host's) */
    ug_text_draw(cv, ingame ? "Up/Down: Move   Left/Right: Change   Enter: Select   Esc: Back   Start: Resume"
                            : "Up/Down: Move   Left/Right: Change   Enter: Select   Esc: Back",
                 0, ingame ? 205.0f : 196.0f, 0.4f, ingame ? lime : dark, UG_J_CENTER | UG_J_VCENTER, 0, 0, UG_GLYPHS_KEYS);

    /* confirmation dialog (SPEC_GUI 6): Yes at x -150, No at x +150, first button focused */
    if (m->dialog_open) {
        ug_cv_fill(cv, UG_RECT_C(0, 0, 460, 130), UG_RGBA(0, 0, 0, 245), 12);
        ug_cv_outline(cv, UG_RECT_C(0, 0, 460, 130), UG_BORDER, 12, 2);
        ug_text_draw(cv, "Reset to default settings?", 0, -28.0f, 0.7f, lime, UG_J_CENTER | UG_J_VCENTER, 0, 0, UG_GLYPHS_KEYS);
        for (int b = 0; b < 2; b++) {
            float bx = b ? 150.0f : -150.0f;
            int sel = b == m->dialog_focus;
            ug_cv_fill(cv, UG_RECT_C(bx, 28, 130, 34), sel ? UG_RGBA(202, 244, 138, 230) : UG_RGBA(60, 60, 60, 230), 8);
            ug_text_draw(cv, b ? "No" : "Yes", bx, 28, 0.7f, sel ? UG_BLACK : lime, UG_J_CENTER | UG_J_VCENTER, 0, 0, UG_GLYPHS_KEYS);
        }
    }
}
