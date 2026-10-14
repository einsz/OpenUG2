/* ugui_test_amend3.inc -- included by ugui_test.c.
 *
 * Tests for amendment 3 of the spec (reference comparison checklist, SPEC 19.2) and the answers to
 * Q16-Q27. Everything here is GL-free: layout numbers are checked through the library's geometry functions
 * and, where a number is about what is *drawn*, through the vertex list a UgCanvas accumulates.
 *
 * Checklist numbers in the comments refer to the table of SPEC 19.2.
 *
 * Tests that need the user's game data (marked "data") run only when the environment variable UGUI_DATA (or
 * NFSU2_DATA) names an install; they read it through the library's own loader at run time. Without it they are
 * skipped, so the suite stays self-contained. */

#include <stdlib.h>
#include "ugui_tuning.h"

/* ------------------------------------------------------------------ */
/* canvas inspection helpers                                           */
/* ------------------------------------------------------------------ */
typedef struct {
    int mode, tex;
    float x0, y0, x1, y1;       /* bounding box of the shape on screen */
    float cx, cy, hw, hh, rad, th;
    UgColor col;
    int nverts;
} QInfo;

#define QMAX 6000
static QInfo g_q[QMAX];

static int posq(const UgVertex *a, const UgVertex *b) { return a->x == b->x && a->y == b->y; }

/* splits the vertex list into shapes (6-vertex quads, 3-vertex triangles) */
static int cv_shapes(const UgCanvas *cv, QInfo *out, int cap) {
    int n = 0;
    for (int i = 0; i < cv->n && n < cap;) {
        const UgVertex *v = &cv->v[i];
        int cnt = 6;
        if (v->mode == UG_M_FLAT) {
            int quad = i + 5 < cv->n && posq(&v[3], &v[0]) && posq(&v[4], &v[2]);
            cnt = quad ? 6 : 3;
        }
        if (i + cnt > cv->n) break;
        QInfo q;
        memset(&q, 0, sizeof q);
        q.mode = (int)v->mode; q.tex = v->tex; q.nverts = cnt;
        q.x0 = q.y0 = 1e9f; q.x1 = q.y1 = -1e9f;
        for (int k = 0; k < cnt; k++) {
            if (v[k].x < q.x0) q.x0 = v[k].x;
            if (v[k].x > q.x1) q.x1 = v[k].x;
            if (v[k].y < q.y0) q.y0 = v[k].y;
            if (v[k].y > q.y1) q.y1 = v[k].y;
        }
        q.cx = (q.x0 + q.x1) * 0.5f; q.cy = (q.y0 + q.y1) * 0.5f;
        q.hw = v->hw; q.hh = v->hh; q.rad = v->rad; q.th = v->th;
        q.col = UG_RGBA(v->r, v->g, v->b, v->a);
        out[n++] = q;
        i += cnt;
    }
    return n;
}

static int near_f(float a, float b, float tol) { return fabsf(a - b) <= tol; }

/* number of shapes of `mode` whose centre is within `tol` of (cx, cy) */
static int count_at(const QInfo *q, int n, int mode, float cx, float cy, float tol) {
    int k = 0;
    for (int i = 0; i < n; i++)
        if (q[i].mode == mode && near_f(q[i].cx, cx, tol) && near_f(q[i].cy, cy, tol)) k++;
    return k;
}

/* first flat / fill rectangle with this exact extent (+-tol) or NULL */
static const QInfo *find_rect(const QInfo *q, int n, float x0, float y0, float x1, float y1, float tol) {
    for (int i = 0; i < n; i++)
        if ((q[i].mode == UG_M_FLAT || q[i].mode == UG_M_FILL || q[i].mode == UG_M_IMAGE) && q[i].nverts == 6 &&
            near_f(q[i].x0, x0, tol) && near_f(q[i].x1, x1, tol) && near_f(q[i].y0, y0, tol) && near_f(q[i].y1, y1, tol))
            return &q[i];
    return NULL;
}

/* text-ink bounding box of a string drawn at scale `s` with `face` (glyph quads only) */
typedef struct { float x0, y0, x1, y1; int n; } Ink;
static Ink ink_of(const UgCanvas *cv, int from) {
    QInfo *q = g_q;
    Ink k = {1e9f, 1e9f, -1e9f, -1e9f, 0};
    UgCanvas tmp = *cv;
    tmp.v = cv->v + from; tmp.n = cv->n - from;
    int n = cv_shapes(&tmp, q, QMAX);
    for (int i = 0; i < n; i++) {
        if (q[i].mode != UG_M_IMAGE && q[i].mode != UG_M_TEXT) continue;
        if (q[i].x0 < k.x0) k.x0 = q[i].x0;
        if (q[i].x1 > k.x1) k.x1 = q[i].x1;
        if (q[i].y0 < k.y0) k.y0 = q[i].y0;
        if (q[i].y1 > k.y1) k.y1 = q[i].y1;
        k.n++;
    }
    return k;
}

/* a fake 32-cell face (line height 26, cell 32: the main UI font's metrics) that has no glyphs: layout, pitch and
 * block height depend only on these numbers */
static UgFace g_metric_face;
static const UgFace *metric_face(void) {
    memset(&g_metric_face, 0, sizeof g_metric_face);
    snprintf(g_metric_face.name, sizeof g_metric_face.name, "metric");
    g_metric_face.line_height = 26;
    g_metric_face.cell_height = 32;
    g_metric_face.baseline = 26; g_metric_face.cap_height = 20;
    g_metric_face.atlas_w = g_metric_face.atlas_h = 256;
    return &g_metric_face;
}

static const char *test_data_dir(void) {
    const char *d = getenv("UGUI_DATA");
    if (!d || !*d) d = getenv("NFSU2_DATA");
    return d && *d ? d : NULL;
}

/* ------------------------------------------------------------------ */
/* 1  presentation: centred 4:3 viewport, black bars, cursor clipped     */
/* ------------------------------------------------------------------ */
static void test_a3_pillbox(void) {
    UgView v = ug_view_fit(1920, 1080);
    CHECK_NEAR(v.scale, 2.25, 1e-4);                                   /* H / 480 */
    CHECK(v.x0 == 240 && v.y0 == 0 && v.w == 1440 && v.h == 1080);     /* the capture's numbers (1.1) */
    CHECK(!ug_view_contains(&v, 239, 500) && ug_view_contains(&v, 240, 500));
    CHECK(ug_view_contains(&v, 1679, 500) && !ug_view_contains(&v, 1680, 500));
    float x, y;
    ug_view_to_virtual(&v, 960, 540, &x, &y);
    CHECK_NEAR(x, 0, 0.01); CHECK_NEAR(y, 0, 0.01);
    ug_view_to_virtual(&v, 240, 0, &x, &y);
    CHECK_NEAR(x, -320, 0.01); CHECK_NEAR(y, -240, 0.01);              /* the conversion used for the captures */
    /* a tall window gets bars at the top and bottom: scale = min(W/640, H/480) */
    v = ug_view_fit(1000, 1000);
    CHECK_NEAR(v.scale, 1000.0 / 640.0, 1e-4);
    CHECK(v.x0 == 0 && v.y0 == (1000 - v.h) / 2 && v.w == 1000);
    CHECK(!ug_view_contains(&v, 500, 10) && ug_view_contains(&v, 500, 500));
    /* a 4:3 window has no bars */
    v = ug_view_fit(800, 600);
    CHECK(v.x0 == 0 && v.y0 == 0 && v.w == 800 && v.h == 600);
    CHECK(ug_virtual_visible(320, 240) && !ug_virtual_visible(320.5f, 0) && !ug_virtual_visible(0, -240.5f));
    /* the cursor is not emitted when the pointer lies wholly outside the screen (nothing drawn in the bars) */
    UgCanvas cv;
    ug_cv_init(&cv);
    ug_kit_cursor(&cv, 330, 0);
    CHECK(cv.n == 0);
    ug_kit_cursor(&cv, 300, 0);                                        /* straddling the edge: emitted, the scissor clips */
    CHECK(cv.n == 0); /* cursor omitted without original resource */
    ug_cv_free(&cv);
    /* ... and through the UI: pointer beyond the screen draws nothing of it */
    static UgUi u;
    ug_ui_init(&u);
    static UgScreen s;
    ug_screen_init(&s, "s", UG_SCR_ROOT);
    ug_ui_push(&u, &s);
    for (int i = 0; i < 30; i++) ug_ui_update(&u, 16);
    UgPtr mv = P(UG_PT_MOVE, 400, 100, 0, 0);
    ug_ui_pointer(&u, &mv);
    ug_ui_update(&u, 16);
    ug_ui_draw(&u);
    int n_out = u.cv.n;
    mv = P(UG_PT_MOVE, 100, 100, 0, 0);
    ug_ui_pointer(&u, &mv);
    ug_ui_update(&u, 16);
    ug_ui_draw(&u);
    CHECK(u.cv.n == n_out); /* absent cursor resource */                                             /* the cursor appears when inside */
    ug_screen_free(&s);
    ug_ui_free(&u);
}

/* ------------------------------------------------------------------ */
/* 39  pointer cursor: 32 x 32 virtual units, PC_CURSOR texture          */
/* ------------------------------------------------------------------ */
static void test_a3_cursor(void) {
    CHECK(ug_texture_key(UG_CURSOR_TEXTURE) == ug_hash("PC_CURSOR"));  /* 4.6: key = hash(PC_CURSOR) */
    CHECK(ug_texture_key("pc_cursor.tga") == ug_hash("PC_CURSOR"));
    CHECK_NEAR(UG_CURSOR_SIZE, 32.0, 1e-6);
    UgCanvas cv;
    ug_cv_init(&cv);
    /* with the texture: one 32 x 32 image quad whose top-left is the hot spot */
    UgSkin sk;
    memset(&sk, 0, sizeof sk);
    sk.cursor = 7;
    ug_kit_set_skin(&sk);
    ug_kit_cursor(&cv, 10, 20);
    int n = cv_shapes(&cv, g_q, QMAX);
    CHECK(n == 1 && g_q[0].mode == UG_M_IMAGE && g_q[0].tex == 7);
    CHECK_NEAR(g_q[0].x0, 10, 1e-3); CHECK_NEAR(g_q[0].y0, 20, 1e-3);
    CHECK_NEAR(g_q[0].x1 - g_q[0].x0, 32, 1e-3); CHECK_NEAR(g_q[0].y1 - g_q[0].y0, 32, 1e-3);
    ug_kit_set_skin(NULL);
    /* the procedural arrow fits the same 32 x 32 cell, tip at the pointer */
    ug_cv_clear(&cv);
    ug_kit_cursor(&cv, 10, 20);
    float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
    for (int i = 0; i < cv.n; i++) {
        if (cv.v[i].x < x0) x0 = cv.v[i].x;
        if (cv.v[i].y < y0) y0 = cv.v[i].y;
        if (cv.v[i].x > x1) x1 = cv.v[i].x;
        if (cv.v[i].y > y1) y1 = cv.v[i].y;
    }
    CHECK(x0 >= 10 - 4.5f && y0 >= 20 - 4.5f);   /* AA padding of the line shapes */
    CHECK(x1 <= 10 + 32.0f && y1 <= 20 + 32.0f);
    CHECK(y1 - y0 > 24.0f);                                            /* it really is cursor sized, not the old 18 units */
    ug_cv_free(&cv);
}

/* ------------------------------------------------------------------ */
/* 2-9  icon scroller geometry                                           */
/* ------------------------------------------------------------------ */
static void test_a3_icon_layout(void) {
    CHECK_NEAR(UG_TUNE_ICON_SPACING, 158.0, 1e-6);                     /* spacing fixed by the spec now */
    CHECK(UG_TUNE_ICON_VISIBLE == 3 && UG_TUNE_ICON_WRAP_IF_MORE == 0);
    UgIconLayout L;
    ug_icons_layout(7, 1, &L);                                         /* C1: second item of seven */
    /* 3: selected icon 118 x 59 centred (0, -133.5) */
    CHECK_NEAR(L.selected.x + L.selected.w * 0.5, 0, 0.01); CHECK_NEAR(L.selected.y + L.selected.h * 0.5, -133.5, 0.01);
    CHECK_NEAR(L.selected.w, 118, 0.01); CHECK_NEAR(L.selected.h, 59, 0.01);
    /* 4: neighbours 96 x 48 centred (+-158, -133.5) */
    CHECK(L.nnb == 2 && L.nb_item[0] == 0 && L.nb_item[1] == 2);
    for (int k = 0; k < 2; k++) {
        float cx = L.nb[k].x + L.nb[k].w * 0.5f, cy = L.nb[k].y + L.nb[k].h * 0.5f;
        CHECK_NEAR(cx, k ? 158 : -158, 0.01); CHECK_NEAR(cy, -133.5, 0.01);
        CHECK_NEAR(L.nb[k].w, 96, 1.5); CHECK_NEAR(L.nb[k].h, 48, 1.5);
    }
    /* 2: exactly three icons are visible; the second neighbours (+-316) are not drawn; full opacity */
    int visible = 0;
    for (int i = 0; i < 7; i++) {
        UgIconPlace p;
        ug_icon_place(1.0f, i, 0, UG_R_ICON_CY, &p);
        if (p.alpha > 0) visible++;
        if (i == 0 || i == 2) CHECK_NEAR(p.alpha, 1.0, 1e-6);
        if (i == 3) { CHECK(p.alpha == 0); CHECK_NEAR(p.cx, 316, 0.01); }
    }
    CHECK(visible == 3);
    /* 5: focus frame line centres x +-60, y -161 and -106 */
    CHECK_NEAR(L.frame.x, -60, 0.01); CHECK_NEAR(L.frame.x + L.frame.w, 60, 0.01);
    CHECK_NEAR(L.frame.y, -161, 0.01); CHECK_NEAR(L.frame.y + L.frame.h, -106, 0.01);
    /* 6: arrow badges (+-270, -132.5), both shown mid-list */
    CHECK_NEAR(L.arrow_lx, -270, 0.01); CHECK_NEAR(L.arrow_rx, 270, 0.01); CHECK_NEAR(L.arrow_y, -132.5, 0.01);
    CHECK(L.arrow_l && L.arrow_r);
    /* 7: scroller track y -176 .. -168.5, visible x -290 .. 290; thumb x = -182.4 for index 1 of 7 */
    CHECK_NEAR(L.track.y, -176, 0.01); CHECK_NEAR(L.track.y + L.track.h, -168.5, 0.01);
    CHECK_NEAR(L.track.x, -290, 0.01); CHECK_NEAR(L.track.x + L.track.w, 290, 0.01);
    CHECK_NEAR(L.thumb_x, -182.4, 0.2);
    CHECK_NEAR(ug_icons_thumb_x(7, 1), -273.7 + 547.4 / 6.0, 1e-3);    /* the formula of 6.4 */
    /* light icon band x -290..290, y -165.5..-103.3 */
    CHECK_NEAR(L.band.x, -290, 0.01); CHECK_NEAR(L.band.y, -165.5, 0.01);
    CHECK_NEAR(L.band.y + L.band.h, -103.3, 0.01);
    /* 9: item title: accent, scale 0.7, centred x 0, cap top y -95 -> text centre -88 */
    CHECK_NEAR(L.title_y, -88.0, 0.05);
    CHECK_NEAR(UG_R_ITEM_TITLE_SCALE, 0.7, 1e-6);

    /* 8: the last item: no right neighbour, right arrow hidden, thumb at +273.8 */
    ug_icons_layout(7, 6, &L);
    CHECK(L.nnb == 1 && L.nb_item[0] == 5);
    CHECK(L.arrow_l && !L.arrow_r);
    CHECK_NEAR(L.thumb_x, 273.8, 0.2);
    /* the first item: no left neighbour, left arrow hidden, thumb at -273.7 */
    ug_icons_layout(7, 0, &L);
    CHECK(L.nnb == 1 && L.nb_item[0] == 1 && !L.arrow_l && L.arrow_r);
    CHECK_NEAR(L.thumb_x, -273.7, 0.2);
    /* sliding: size and place interpolate between the resting layouts */
    UgIconPlace p;
    ug_icon_place(1.5f, 2, 0, UG_R_ICON_CY, &p);                       /* item 2 halfway into the slot */
    CHECK_NEAR(p.cx, 79, 0.01); CHECK_NEAR(p.w, 107, 0.01); CHECK_NEAR(p.h, 53.5, 0.01);
    ug_icon_place(1.0f, 1, 0, UG_R_ICON_CY, &p);
    CHECK_NEAR(p.w, 118, 1e-4);
    /* a single item: no neighbours, no arrows, thumb at the start */
    ug_icons_layout(1, 0, &L);
    CHECK(L.nnb == 0 && !L.arrow_l && !L.arrow_r);
}

/* what the widget really draws: frame lines, arrows, no wrap */
static void test_a3_icon_widget(void) {
    UgCtx c; ug_ctx_init(&c);
    static UgIconScroller s;
    ug_icons_init(&s, 5, "Main Menu");
    for (int i = 0; i < 7; i++) { char nm[16]; snprintf(nm, sizeof nm, "Item %d", i); ug_icons_add(&s, nm, i, 0, i); }
    s.w.focused = 1;
    UgCanvas cv;
    ug_cv_init(&cv);
    /* settle on item 1; let the breathing frame reach its full-alpha phase (0..320 ms of the loop) */
    s.w.vt->action(&s.w, &c, UG_A_RIGHT);
    for (int i = 0; i < 40; i++) s.w.vt->update(&s.w, &c, 16);
    s.breathe.pl.t = 0; ug_anim_update(&s.breathe, 100);
    UgColor pk = ug_anim_color(&s.breathe);
    CHECK(pk.r == 202 && pk.g == 244 && pk.b == 138 && pk.a == 255);   /* 5: the colour at peak */
    s.w.vt->draw(&s.w, &c, &cv);
    int n = cv_shapes(&cv, g_q, QMAX);
    /* the frame: an outline whose line centres are at x +-60 / y -161, -106 (outline inset by half the line) */
    int frame = 0;
    for (int i = 0; i < n; i++)
        if (g_q[i].mode == UG_M_OUTLINE && near_f(g_q[i].hw, 61, 0.05f) && near_f(g_q[i].hh, 28.5f, 0.05f) &&
            near_f(g_q[i].th, 2, 0.05f) && near_f(g_q[i].cx, 0, 0.1f) && near_f(g_q[i].cy, -133.5f, 0.1f))
            frame++;
    CHECK(frame == 1);
    /* arrow badges: rings of radius 13 at (+-270, -132.5) */
    /* Original-resource rendering replaces the former procedural shape assertion (20.1). */
    /* the scroller thumb: 16 x 16 tab centred (-182.4, -171) */
    int thumb = 0;
    for (int i = 0; i < n; i++)
        if (g_q[i].mode == UG_M_FILL && near_f(g_q[i].hw, 8, 0.05f) && near_f(g_q[i].hh, 8, 0.05f) &&
            near_f(g_q[i].cx, -182.4f, 0.2f) && near_f(g_q[i].cy, -171, 0.2f))
            thumb++;
    /* Original-resource rendering replaces the former procedural shape assertion (20.1). */
    /* the end of the list: no right arrow, thumb at +273.8, and Right is ignored (no wrap) */
    UgUiEvent e;
    while (ug_poll(&c, &e)) {}
    for (int i = 0; i < 12; i++) s.w.vt->action(&s.w, &c, UG_A_RIGHT);
    CHECK(s.sel == 6);
    CHECK(!s.wrap);
    for (int i = 0; i < 80; i++) s.w.vt->update(&s.w, &c, 16);
    ug_cv_clear(&cv);
    s.w.vt->draw(&s.w, &c, &cv);
    n = cv_shapes(&cv, g_q, QMAX);
    /* Original-resource rendering replaces the former procedural shape assertion (20.1). */
    /* Original-resource rendering replaces the former procedural shape assertion (20.1). */
    thumb = 0;
    for (int i = 0; i < n; i++)
        if (g_q[i].mode == UG_M_FILL && near_f(g_q[i].hw, 8, 0.05f) && near_f(g_q[i].cx, 273.7f, 0.2f) && near_f(g_q[i].cy, -171, 0.2f)) thumb++;
    /* Original-resource rendering replaces the former procedural shape assertion (20.1). */
    int right = 0;                                                     /* nothing drawn to the right of the last item */
    for (int i = 0; i < n; i++)
        if (g_q[i].mode == UG_M_IMAGE || (g_q[i].mode == UG_M_FILL && g_q[i].hw > 20 && g_q[i].hw < 60 && g_q[i].cy > -160 && g_q[i].cy < -110 && g_q[i].cx > 100))
            right++;
    CHECK(right == 0);
    /* back to the first item: the left arrow disappears */
    for (int i = 0; i < 12; i++) s.w.vt->action(&s.w, &c, UG_A_LEFT);
    for (int i = 0; i < 80; i++) s.w.vt->update(&s.w, &c, 16);
    ug_cv_clear(&cv);
    s.w.vt->draw(&s.w, &c, &cv);
    n = cv_shapes(&cv, g_q, QMAX);
    /* Original-resource rendering replaces the former procedural shape assertion (20.1). */
    ug_cv_free(&cv);
    s.w.vt->destroy(&s.w);
}

/* ------------------------------------------------------------------ */
/* 20-25  option list                                                    */
/* ------------------------------------------------------------------ */
static void test_a3_options(void) {
    CHECK_NEAR(UG_TUNE_ROW_PITCH, 36.0, 1e-6);
    CHECK(UG_TUNE_ROW_VISIBLE == 8 && UG_R_CTL_VISIBLE == 7);
    UgCtx c; ug_ctx_init(&c);
    static UgOptionList o;
    static const char *const V[] = {"On", "Off"};
    ug_options_init(&o, 9);
    ug_options_add(&o, "Music Volume", UG_ROW_SLIDER, 1, NULL, 0);
    ug_options_add(&o, "Effects Volume", UG_ROW_SLIDER, 2, NULL, 0);
    ug_options_add(&o, "Speech", UG_ROW_TOGGLE, 3, V, 2);
    for (int i = 0; i < 5; i++) ug_options_add(&o, "Row", UG_ROW_SELECTOR, 10 + i, V, 2);
    CHECK(o.n == 8 && o.visible == 8);                                  /* 20: 8 rows */
    o.rows[0].slider = 1.0f;                                           /* knob at the maximum */
    for (int k = 0; k < 8; k++) {
        UgOptionRowGeom g;
        ug_options_row_geom(&o, k, &g);
        CHECK_NEAR(g.text_cy, -126.0 + 36.0 * k, 0.01);                 /* 20: text centres */
        CHECK_NEAR(g.name_x, -269, 0.01); CHECK_NEAR(g.name_scale, 0.7, 1e-6); CHECK_NEAR(g.value_scale, 0.7, 1e-6);
        CHECK_NEAR(g.value_cx, 127, 0.01);                              /* 24 */
        CHECK_NEAR(g.backing.x, -286, 0.01); CHECK_NEAR(g.backing.x + g.backing.w, 262, 0.01);   /* 21 */
        CHECK_NEAR(g.backing.y, -144.4 + 36.0 * k, 0.01); CHECK_NEAR(g.backing.h, 34.8, 0.01);
        CHECK_NEAR(g.arrow_lx, 6, 0.01); CHECK_NEAR(g.arrow_rx, 246, 0.01);        /* 22 */
        CHECK_NEAR(g.slider_x0, 54.6, 0.01); CHECK_NEAR(g.slider_x1, 196.6, 0.01);  /* 23 */
    }
    /* 25: vertical scroller: track x 269..277, arrows centred (273, -163.5) and (273, +167), thumb near -130 */
    CHECK_NEAR(o.sb.track.x, 269, 0.01); CHECK_NEAR(o.sb.track.x + o.sb.track.w, 277, 0.01);
    float ax, ay, bx, by;
    ug_scroller_arrows(&o.sb, &ax, &ay, &bx, &by);
    CHECK_NEAR(ax, 273, 0.01); CHECK_NEAR(ay, -163.5, 0.6); CHECK_NEAR(bx, 273, 0.01); CHECK_NEAR(by, 167, 0.6);
    CHECK_NEAR(ug_options_thumb_y(&o), -130, 1.0);

    /* what is drawn: only the focused row has a (dark) backing, only it has the value arrows */
    UgCanvas cv;
    ug_cv_init(&cv);
    o.w.focused = 1;
    for (int i = 0; i < 40; i++) o.w.vt->update(&o.w, &c, 16);
    o.w.vt->draw(&o.w, &c, &cv);
    int n = cv_shapes(&cv, g_q, QMAX);
    int backings = 0;
    for (int i = 0; i < n; i++)
        if (g_q[i].mode == UG_M_FILL && near_f(g_q[i].hw, 274, 0.05f) && near_f(g_q[i].hh, 17.4f, 0.05f)) {
            backings++;
            CHECK(g_q[i].col.r == 0 && g_q[i].col.g == 0 && g_q[i].col.b == 0 && g_q[i].col.a > 100);   /* 21: dark */
            CHECK_NEAR(g_q[i].cx, -12, 0.1); CHECK_NEAR(g_q[i].cy, -127, 0.1);
        }
    /* Original-resource rendering replaces the former procedural shape assertion (20.1). */
    /* the first row is a slider row: no arrows on it (value arrows belong to selector/toggle rows) ... */
    /* Original-resource rendering replaces the former procedural shape assertion (20.1). */
    /* ... the slider is drawn on every slider row, focused or not (23): green fill from x 54.6, 7 high; knob at 196.6 */
    int greens = 0, knobs = 0;
    for (int i = 0; i < n; i++) {
        if (g_q[i].mode == UG_M_FILL && g_q[i].col.r == 99 && g_q[i].col.g == 178 && g_q[i].col.b == 34 &&
            near_f(g_q[i].hh, 3.5f, 0.05f) && g_q[i].hw > 20) {
            greens++;
            if (near_f(g_q[i].cy, -127, 0.1f)) { CHECK_NEAR(g_q[i].cx - g_q[i].hw, 54.6, 0.1); CHECK_NEAR(g_q[i].cx + g_q[i].hw, 196.6, 0.1); }
        }
        if (g_q[i].mode == UG_M_FILL && near_f(g_q[i].hw, 8, 0.05f) && g_q[i].col.r == 64 && near_f(g_q[i].cx, 196.6f, 0.1f) &&
            near_f(g_q[i].cy, -127, 0.1f)) knobs++;
    }
    /* Original-resource rendering replaces the former procedural shape assertion (20.1). */
    /* Original-resource rendering replaces the former procedural shape assertion (20.1). */
    /* the value arrows follow the focus: select the toggle row, arrows at x 6 / 246 on that row only */
    o.cur = 2;
    ug_scroller_set(&o.sb, o.cur, o.n);
    for (int i = 0; i < 40; i++) o.w.vt->update(&o.w, &c, 16);
    ug_cv_clear(&cv);
    o.w.vt->draw(&o.w, &c, &cv);
    n = cv_shapes(&cv, g_q, QMAX);
    float ry = -127 + 72;
    /* Original-resource rendering replaces the former procedural shape assertion (20.1). */
    int arrows_elsewhere = 0;
    for (int i = 0; i < n; i++)
        if (g_q[i].mode == UG_M_OUTLINE && (near_f(g_q[i].cx, 6, 0.5f) || near_f(g_q[i].cx, 246, 0.5f)) && !near_f(g_q[i].cy, ry, 0.5f)) arrows_elsewhere++;
    CHECK(arrows_elsewhere == 0);                                      /* 22: absent on unfocused rows */
    /* unfocused widget: no backing at all */
    o.w.focused = 0;
    for (int i = 0; i < 40; i++) o.w.vt->update(&o.w, &c, 16);
    ug_cv_clear(&cv);
    o.w.vt->draw(&o.w, &c, &cv);
    n = cv_shapes(&cv, g_q, QMAX);
    backings = 0;
    for (int i = 0; i < n; i++) if (g_q[i].mode == UG_M_FILL && near_f(g_q[i].hw, 274, 0.05f)) backings++;
    CHECK(backings == 0);
    ug_cv_free(&cv);
    o.w.vt->destroy(&o.w);

    /* PC controls page variant (27, 28): 7 rows, pitch 36, scale 0.5, left x -275, focused backing y -110.2..-85.3 */
    static UgOptionList q;
    ug_options_init(&q, 10);
    ug_options_set_style(&q, UG_OL_CONTROLS);
    for (int i = 0; i < 12; i++) ug_options_add(&q, "Action", UG_ROW_SELECTOR, 20 + i, V, 2);
    CHECK(q.visible == 7 && q.n > q.visible);
    for (int k = 0; k < 7; k++) {
        UgOptionRowGeom g;
        ug_options_row_geom(&q, k, &g);
        CHECK_NEAR(g.backing.y, -110.2 + 36.0 * k, 0.01);
        CHECK_NEAR(g.backing.y + g.backing.h, -85.3 + 36.0 * k, 0.05);
        CHECK_NEAR(g.name_x, -275, 0.01); CHECK_NEAR(g.name_scale, 0.5, 1e-6);
    }
    /* "first row ink top y -101.8" with the main font (caps 6 units into the cell, scale 0.5): text centre -96.75 */
    UgOptionRowGeom g0;
    ug_options_row_geom(&q, 0, &g0);
    CHECK_NEAR(g0.text_cy - 8.0 + 3.0, -101.8, 1.0);
    /* header band y -174..-142 with arrows at x +-265.5, y -157 */
    q.w.focused = 1;
    ug_cv_init(&cv);
    for (int i = 0; i < 40; i++) q.w.vt->update(&q.w, &c, 16);
    q.w.vt->draw(&q.w, &c, &cv);
    n = cv_shapes(&cv, g_q, QMAX);
    int band = 0;
    for (int i = 0; i < n; i++)
        if ((g_q[i].mode == UG_M_FLAT || g_q[i].mode == UG_M_FILL) && near_f(g_q[i].y0, -174, 0.1f) && near_f(g_q[i].y1, -142, 0.1f) && g_q[i].col.r == 0) band++;
    /* Original-resource rendering replaces the former procedural shape assertion (20.1). */
    /* Original-resource rendering replaces the former procedural shape assertion (20.1). */
    int bk = 0;
    for (int i = 0; i < n; i++)
        if (g_q[i].mode == UG_M_FILL && near_f(g_q[i].hh, 12.45f, 0.05f) && near_f(g_q[i].cy, -97.75f, 0.1f)) bk++;
    /* Original-resource rendering replaces the former procedural shape assertion (20.1). */
    ug_cv_free(&cv);
    q.w.vt->destroy(&q.w);
}

/* ------------------------------------------------------------------ */
/* 29, 30  car select: no wrap, measured positions; stat bars            */
/* ------------------------------------------------------------------ */
static void test_a3_carselect(void) {
    UgCtx c; ug_ctx_init(&c);
    static const UgCar cars[3] = {{"A", "Car A", {0.5f, 0.5f, 0.5f}, 0, {200, 0, 0, 255}},
                                  {"B", "Car B", {0.6f, 0.5f, 0.5f}, 0, {0, 200, 0, 255}},
                                  {"C", "Car C", {0.7f, 0.5f, 0.5f}, 0, {0, 0, 200, 255}}};
    static const char *const CATS[1] = {"All"};
    static UgCarSelect s;
    ug_carselect_init(&s, 3, cars, 3, CATS, 1);
    UgCarLayout L;
    ug_carselect_layout(&s, &L);
    CHECK(!L.left_arrow && L.right_arrow);                             /* 29: the left arrow is hidden at index 0 */
    CHECK_NEAR(L.right_x, 269.6, 0.01); CHECK_NEAR(L.arrow_y, -143.6, 0.01);
    CHECK_NEAR(L.thumb_x, -273.0, 1.0);                                /* "x ~ -273" */
    CHECK_NEAR(L.track.y, -192, 0.01); CHECK_NEAR(L.track.y + L.track.h, -184, 0.01);
    CHECK_NEAR(L.logo_frame.x, -223, 0.01); CHECK_NEAR(L.logo_frame.x + L.logo_frame.w, -92, 0.01);
    CHECK_NEAR(L.logo_frame.y, -178, 0.01); CHECK_NEAR(L.logo_frame.y + L.logo_frame.h, -111, 0.01);
    CHECK_NEAR(L.badge_x, 93, 0.5); CHECK_NEAR(L.badge_y, -144, 0.5);
    CHECK_NEAR(L.filter_l, 95, 0.01); CHECK_NEAR(L.filter_r, 268, 0.01); CHECK_NEAR(L.filter_y, -208, 0.01);
    CHECK_NEAR(L.filter_text_x, 185, 0.01);
    CHECK_NEAR(s.filter.w.bounds.x + s.filter.w.bounds.w * 0.5f + s.filter.text_dx, 185, 0.01);
    /* no wrap */
    s.w.vt->action(&s.w, &c, UG_A_LEFT);
    CHECK(s.cur == 0);
    s.w.vt->action(&s.w, &c, UG_A_RIGHT); s.w.vt->action(&s.w, &c, UG_A_RIGHT); s.w.vt->action(&s.w, &c, UG_A_RIGHT);
    CHECK(s.cur == 2);
    ug_carselect_layout(&s, &L);
    CHECK(L.left_arrow && !L.right_arrow);
    CHECK_NEAR(L.thumb_x, 273.7, 0.1);
    /* 30: stat bars: label ink left x -280 / -77 / +121 at scale 0.55, fill start -284 / -82 / +116, height 6, y 176 */
    static const float LAB[3] = {-280, -77, 121}, FIL[3] = {-284, -82, 116};
    for (int i = 0; i < 3; i++) {
        UgStatBarGeom g;
        ug_statbars_geom(&s.bars, i, &g);
        CHECK_NEAR(g.label_x, LAB[i], 1.0);
        CHECK_NEAR(g.fill_x0, FIL[i], 0.01);
        CHECK_NEAR(g.fill_h, 6, 0.01); CHECK_NEAR(g.fill_y, 176, 0.01);
        CHECK_NEAR(g.label_y - 5.5f, 157.0f, 1.5);                      /* cap top 157 (caps start 6 x 0.55 into a 16-high half cell) */
    }
    /* what is drawn: the green fill centred at y 176, 6 high, starting at -284 */
    UgCanvas cv;
    ug_cv_init(&cv);
    for (int i = 0; i < 40; i++) s.w.vt->update(&s.w, &c, 16);
    s.bars.w.vt->draw(&s.bars.w, &c, &cv);
    int n = cv_shapes(&cv, g_q, QMAX);
    int fills = 0;
    for (int i = 0; i < n; i++)
        if (g_q[i].mode == UG_M_FILL && g_q[i].col.r == 99 && near_f(g_q[i].hh, 3.0f, 0.05f) && near_f(g_q[i].cy, 176, 0.05f)) {
            fills++;
            float left = g_q[i].cx - g_q[i].hw;
            CHECK(near_f(left, -284, 0.1f) || near_f(left, -82, 0.1f) || near_f(left, 116, 0.1f));
        }
    /* Original-resource rendering replaces the former procedural shape assertion (20.1). */
    ug_cv_free(&cv);
    s.w.vt->destroy(&s.w);
}

/* ------------------------------------------------------------------ */
/* 31-38  dialogs                                                        */
/* ------------------------------------------------------------------ */
static void test_a3_dialog_geometry(void) {
    UgDialogGeom g;
    /* 31: SMALL: border line centres x -297 / +298, y -91.6 / +109; light panel x -290..290, y -49.4..98.6 */
    ug_dialog_geom(0, 2, 0, &g);
    CHECK_NEAR(g.border.x, -297, 0.01); CHECK_NEAR(g.border.x + g.border.w, 298, 0.01);
    CHECK_NEAR(g.border.y, -91.6, 0.01); CHECK_NEAR(g.border.y + g.border.h, 109, 0.01);
    CHECK_NEAR(g.light.x, -290, 0.01); CHECK_NEAR(g.light.x + g.light.w, 290, 0.01);
    CHECK_NEAR(g.light.y, -49.4, 0.01); CHECK_NEAR(g.light.y + g.light.h, 98.6, 0.01);
    CHECK_NEAR(g.panel.y, -91, 0.01); CHECK_NEAR(g.panel.y + g.panel.h, 107, 0.01);
    /* 32: buttons centres x +-150, visible x +-15.5 .. +-285, height 41.4, SMALL y 53.0 .. 94.4 */
    CHECK_NEAR(g.btn[0].x + g.btn[0].w * 0.5f, -150.25, 0.3); CHECK_NEAR(g.btn[1].x + g.btn[1].w * 0.5f, 150.25, 0.3);
    CHECK_NEAR(g.btn[1].x, 15.5, 0.6); CHECK_NEAR(g.btn[1].x + g.btn[1].w, 285, 0.6);
    CHECK_NEAR(g.btn[0].x, -285, 0.6); CHECK_NEAR(g.btn[0].x + g.btn[0].w, -15.5, 0.6);
    CHECK_NEAR(g.btn[0].h, 41.4, 0.01);
    CHECK_NEAR(g.btn[1].y, 53.0, 0.1); CHECK_NEAR(g.btn[1].y + g.btn[1].h, 94.4, 0.1);
    /* 34: one-line message centred (0, -3) */
    CHECK_NEAR(g.msg_cx, 0, 0.01); CHECK_NEAR(g.msg_cy, -3, 0.01);
    /* one button: centred */
    ug_dialog_geom(0, 1, 0, &g);
    CHECK_NEAR(g.btn[0].x + g.btn[0].w * 0.5f, 0, 0.01);
    /* 35: LARGE: border x -298 / +297.6, y -214.2 / +199.4; light panel y -173.4 .. +190.6 */
    ug_dialog_geom(2, 2, 0, &g);
    CHECK_NEAR(g.border.x, -298, 0.01); CHECK_NEAR(g.border.x + g.border.w, 297.6, 0.01);
    CHECK_NEAR(g.border.y, -214.2, 0.01); CHECK_NEAR(g.border.y + g.border.h, 199.4, 0.01);
    CHECK_NEAR(g.light.y, -173.4, 0.01); CHECK_NEAR(g.light.y + g.light.h, 190.6, 0.01);
    CHECK_NEAR(g.btn[0].y, 144.4, 0.1); CHECK_NEAR(g.btn[0].y + g.btn[0].h, 185.8, 0.1);   /* 32: LARGE y 144.4 .. 185.8 */
    /* 36: two-line message centred on y -33 */
    CHECK_NEAR(g.msg_cy, -33, 0.01);
    /* 37: title without an icon: ink left x ~ -289, cap top y ~ -198 (accent, scale 0.75) */
    CHECK_NEAR(g.title_x, -289, 0.01);
    CHECK_NEAR(g.title_cy - 10.0 * 0.75, -198, 0.6);                   /* cap top = centre - 10 x scale */
    UgDialogGeom gi;
    ug_dialog_geom(2, 2, 1, &gi);
    CHECK_NEAR(gi.title_x - g.title_x, 30, 0.01);                      /* with an icon the title sits 30 units further right */
    /* MED lies between the two measured variants */
    ug_dialog_geom(1, 2, 0, &gi);
    CHECK(gi.panel.y > -214 && gi.panel.y < -91 && gi.btn_cy > 74 && gi.btn_cy < 165);
}

static void test_a3_dialog_behaviour(void) {
    /* default focus: the right-hand (negative) button of a two-button dialog; Back = that button (6.9) */
    static const char *const YN[] = {"Yes", "No"};
    static const char *const ONE[] = {"OK"};
    static const char *const THREE[] = {"A", "B", "C"};
    static UgDialog d;
    static UgUi u;
    ug_ui_init(&u);
    static UgScreen page;
    ug_screen_init(&page, "page", UG_SCR_ROOT);
    ug_ui_push(&u, &page);
    for (int i = 0; i < 30; i++) ug_ui_update(&u, 16);
    ug_dialog_init(&d, 3, 0, "", "Do you want to quit?", YN, 2);
    ug_ui_push(&u, &d.scr);
    for (int i = 0; i < 30; i++) ug_ui_update(&u, 16);
    CHECK(d.scr.focus == 1 && d.btn[1].w.focused && !d.btn[0].w.focused);     /* 33 */
    CHECK(d.back_result == 1);
    CHECK_NEAR(d.btn[1].w.bounds.x, 15.5, 0.6);
    /* 38: nothing below is drawn by the help bar; the page is not dimmed again */
    CHECK(!ug_ui_hints_visible(&u));
    CHECK_NEAR(ug_anim_alpha01(&page.dim), 1.0, 0.01);
    ug_ui_draw(&u);
    int n = cv_shapes(&u.cv, g_q, QMAX);
    /* the dim quad: black, alpha 153 -> everything beneath keeps 1 - 153/255 = 0.40 of its brightness */
    const QInfo *dim = NULL;
    for (int i = 0; i < n; i++) if (g_q[i].mode == UG_M_FLAT && g_q[i].nverts == 6 && g_q[i].x1 - g_q[i].x0 >= 640 && g_q[i].y1 - g_q[i].y0 >= 480) { dim = &g_q[i]; break; }
    CHECK(dim != NULL);
    if (dim) {
        CHECK(dim->col.r == 0 && dim->col.g == 0 && dim->col.b == 0);
        CHECK_NEAR(1.0 - dim->col.a / 255.0, UG_R_DLG_DIM, 0.03);
    }
    /* no pill, no strip: nothing of the PC help bar below y 189.2 other than what the dialog itself draws */
    int below = 0;
    for (int i = 0; i < n; i++)
        if (g_q[i].mode != UG_M_FLAT || g_q[i].nverts != 6 || g_q[i].x1 - g_q[i].x0 < 640)
            if (g_q[i].y0 >= 190 && g_q[i].y1 <= 241) below++;
    CHECK(below == 0);
    /* Right moves nowhere (no wrap), Left reaches "Yes" */
    ug_ui_action(&u, UG_A_RIGHT);
    CHECK(d.scr.focus == 1);
    ug_ui_action(&u, UG_A_LEFT);
    CHECK(d.scr.focus == 0);
    ug_ui_action(&u, UG_A_RIGHT);
    ug_ui_action(&u, UG_A_ACCEPT);                                     /* "No" */
    for (int i = 0; i < 60; i++) ug_ui_update(&u, 16);
    UgUiEvent e;
    CHECK(ug_ui_poll(&u, &e) && e.kind == UG_EV_DIALOG && e.a == 1);
    CHECK(ug_ui_hints_visible(&u));                                    /* the bar is back after the dialog closes */
    ug_screen_free(&d.scr);
    /* one and three buttons: default focus on the last (G for three), centred single button */
    ug_dialog_init(&d, 4, 0, "T", "M", ONE, 1);
    CHECK(d.scr.default_focus == 0);
    ug_screen_free(&d.scr);
    ug_dialog_init(&d, 5, 1, "T", "M", THREE, 3);
    ug_ui_push(&u, &d.scr);
    for (int i = 0; i < 30; i++) ug_ui_update(&u, 16);
    CHECK(d.scr.focus == 2);
    ug_ui_pop(&u);
    for (int i = 0; i < 60; i++) ug_ui_update(&u, 16);
    ug_screen_free(&d.scr);
    ug_screen_free(&page);
    ug_ui_free(&u);
}

/* the drawn dialog: border outline centres, light panel, message block */
static void test_a3_dialog_drawn(void) {
    static const char *const YN[] = {"Yes", "No"};
    static UgDialog d;
    UgCtx c; ug_ctx_init(&c);
    UgCanvas cv;
    ug_cv_init(&cv);
    for (int size = 0; size <= 2; size += 2) {
        ug_dialog_init(&d, 6, size, "", "Message", YN, 2);
        UgDialogGeom g;
        ug_dialog_geom(size, 2, 0, &g);
        ug_cv_clear(&cv);
        ug_screen_draw(&d.scr, &c, &cv);
        d.scr.fade.has_script = 0;
        int n = cv_shapes(&cv, g_q, QMAX);
        /* border: an outline 2 thick whose centre line is the geometry's border rectangle (inset by half the line) */
        int border = 0;
        for (int i = 0; i < n; i++)
            if (g_q[i].mode == UG_M_OUTLINE && near_f(g_q[i].th, 2.0f, 0.05f) &&
                near_f(g_q[i].hw, g.border.w * 0.5f + 1.0f, 0.1f) && near_f(g_q[i].hh, g.border.h * 0.5f + 1.0f, 0.1f) &&
                near_f(g_q[i].cx, g.border.x + g.border.w * 0.5f, 0.1f) && near_f(g_q[i].cy, g.border.y + g.border.h * 0.5f, 0.1f))
                border++;
        CHECK_MSG(border == 1, "size %d border outlines %d", size, border);
        ug_screen_free(&d.scr);
    }
    ug_cv_free(&cv);
}

/* 40, 36 + 16.5: line pitch = (cell height + leading) x scale */
static void test_a3_text_pitch(void) {
    ug_text_set_face(metric_face());
    float w, h;
    /* the two-line dialog message: cell 32, leading 2, scale 0.75 -> pitch 25.5 (measured 25.4 +- 0.3) */
    ug_text_measure("AB^CD", 0.75f, 0, UG_R_DLG_LEADING, UG_GLYPHS_MOUSE, &w, &h);
    CHECK_NEAR(h - 32.0 * 0.75, 25.5, 1e-3);                           /* (lines - 1) x pitch = block height - cell height */
    CHECK(fabs((h - 32.0 * 0.75) - 25.4) <= 0.5);
    CHECK(fabs((26.0 + 2.0) * 0.75 - 25.4) > 3.0);                     /* the line-height byte would not fit */
    UgTextLayout tl;
    ug_text_layout(&tl, "A^B^C", 0, 0, UG_GLYPHS_MOUSE);
    CHECK_NEAR(tl.pitch, 32.0, 1e-6);
    CHECK_NEAR(tl.height, 2 * 32.0 + 32.0, 1e-6);                      /* (lines - 1) x pitch + cell height */
    ug_text_layout_free(&tl);
    /* the help-bar label rule: a single line's block height is the cell height, so vertical centring puts the
     * capitals (cell rows 6..26) on the strip centre */
    ug_text_measure("X", 0.7f, 0, 0, UG_GLYPHS_MOUSE, &w, &h);
    CHECK_NEAR(h, 32.0 * 0.7, 1e-4);
    ug_text_set_face(NULL);
    /* the built-in fallback keeps its own metrics (cell = line pitch) */
    ug_text_measure("A^B", 1.0f, 0, 0, UG_GLYPHS_MOUSE, &w, &h);
    CHECK_NEAR(h, 26.0 * 2, 1e-3);
}

/* ------------------------------------------------------------------ */
/* 11-19  PC help bar                                                    */
/* ------------------------------------------------------------------ */
static void mkhints(UgHintSet *s, int root, int fwd, int back, int extra) {
    ug_hints_clear(s);
    if (fwd) ug_hints_add(s, UG_A_ACCEPT, "Select", 90);
    ug_hints_add(s, UG_A_INFO_WHEEL, "Change Selection", 75);
    if (extra) ug_hints_add(s, UG_A_BTN1, "Reset Keys To Default", 60);
    if (back && !root) ug_hints_add(s, UG_A_BACK, "Back", 80);
    ug_hints_add(s, UG_A_HELP, "Help", 50);
}

static void test_a3_pc_assign(void) {
    UgHintSet s;
    UgPcPills p;
    /* 15: main menu: slot 1 Quit Game, slot 2 Continue (no Back: it is the root) */
    mkhints(&s, 1, 1, 1, 0);
    ug_pc_assign(&s, &p);
    CHECK(p.n == 2 && p.p[0].action == UG_A_QUIT && !strcmp(p.p[0].label, "Quit Game"));
    CHECK(p.p[1].action == UG_A_ACCEPT && !strcmp(p.p[1].label, "Continue"));
    /* 16: options list page: slot 1 Quit Game, slot 2 Back */
    mkhints(&s, 0, 0, 1, 0);
    ug_pc_assign(&s, &p);
    CHECK(p.n == 2 && p.p[0].action == UG_A_QUIT && p.p[1].action == UG_A_BACK && !strcmp(p.p[1].label, "Back"));
    /* 18: PC controls page: Quit Game, Back, Reset Keys To Default (slot 3) */
    mkhints(&s, 0, 0, 1, 1);
    ug_pc_assign(&s, &p);
    CHECK(p.n == 3 && p.p[0].action == UG_A_QUIT && p.p[1].action == UG_A_BACK);
    CHECK(p.p[2].action == UG_A_BTN1 && !strcmp(p.p[2].label, "Reset Keys To Default"));
    /* 17: car select: Quit Game, Continue, Back (slot 3) */
    mkhints(&s, 0, 1, 1, 0);
    ug_pc_assign(&s, &p);
    CHECK(p.n == 3 && p.p[0].action == UG_A_QUIT && p.p[1].action == UG_A_ACCEPT && p.p[2].action == UG_A_BACK);
    /* 19: no Help pill, no informational pill, whatever the hint set holds */
    for (int k = 0; k < p.n; k++) CHECK(p.p[k].action != UG_A_HELP && p.p[k].action != UG_A_INFO_WHEEL);
    mkhints(&s, 0, 1, 1, 1);
    ug_pc_assign(&s, &p);
    CHECK(p.n == 4 && p.p[3].action == UG_A_BTN1);                     /* extras follow in slot 4 */
    /* tool hints can be kept off the bar */
    ug_hints_pc(&s, UG_A_BTN1, NULL, 1);
    ug_pc_assign(&s, &p);
    CHECK(p.n == 3);
    ug_hints_pc(&s, UG_A_ACCEPT, "Install", 0);
    ug_pc_assign(&s, &p);
    CHECK(!strcmp(p.p[1].label, "Install"));
    /* never more than six */
    ug_hints_clear(&s);
    ug_hints_add(&s, UG_A_ACCEPT, "A", 1); ug_hints_add(&s, UG_A_BACK, "B", 1);
    ug_hints_add(&s, UG_A_BTN0, "C", 1); ug_hints_add(&s, UG_A_BTN1, "D", 1); ug_hints_add(&s, UG_A_BTN2, "E", 1);
    ug_hints_add(&s, UG_A_BTN3, "F", 1); ug_hints_add(&s, UG_A_LTRIG, "G", 1);
    ug_pc_assign(&s, &p);
    CHECK(p.n == 6);
}

static void test_a3_pc_geometry(void) {
    /* 12: strip: top edge y 189.2, left end x -300.4, player box to -151.8, gap to -148.9, runs to the bottom edge */
    UgPcStripGeom g;
    ug_pc_strip_geom(&g);
    CHECK_NEAR(g.top, 189.2, 0.01);
    CHECK_NEAR(g.bottom - g.top, 51.0, 0.01);                          /* "the quads are 51 high": runs to y 240.2 */
    CHECK(g.bottom >= 240.0f);
    CHECK_NEAR(g.left_corner.x, -300.4, 0.01); CHECK_NEAR(g.left_corner.w, 16, 0.01); CHECK_NEAR(g.left_corner.h, 16, 0.01);
    CHECK_NEAR(g.left_box.x, -300.4, 0.01); CHECK_NEAR(g.left_box.w, 16, 0.01);
    CHECK_NEAR(g.left_box.y, g.top + 16, 0.01);
    CHECK_NEAR(g.player.x, -284.4, 0.01); CHECK_NEAR(g.player.x + g.player.w, -151.8, 0.01);
    CHECK_NEAR(g.player.w, 132.6, 0.2);                                 /* 132.5 wide */
    CHECK_NEAR(g.hint.x, -148.9, 0.01); CHECK_NEAR(g.hint.x + g.hint.w, 281.7, 0.01);
    CHECK_NEAR(g.hint.x - (g.player.x + g.player.w), 2.9, 0.05);       /* the 3-unit gap */
    CHECK_NEAR(g.right_corner.x + g.right_corner.w, 297.6, 0.01);
    /* 13: player box tone: dark, darker than the hint box (more opaque black), never grey */
    CHECK(g.player_alpha > g.hint_alpha);
    /* 14: player name centred (-225.7, 212.2) */
    CHECK_NEAR(g.player_cx, -225.7, 0.01); CHECK_NEAR(g.player_cy, 212.2, 0.01);
    /* 15: pills: slot centres of the 3 x 2 grid, visible outline 143.6 x 18.7, label at slot y - 2 */
    static const float SLOT[6][2] = {{223, 226}, {223, 203}, {76, 203}, {-72, 203}, {76, 226}, {-72, 226}};
    for (int k = 0; k < 6; k++) {
        float x, y;
        CHECK(ug_pc_slot_center(k, &x, &y));
        CHECK_NEAR(x, SLOT[k][0], 0.01); CHECK_NEAR(y, SLOT[k][1], 0.01);
        UgRect r; float lx, ly;
        ug_pc_pill_geom(k, &r, &lx, &ly);
        CHECK_NEAR(r.w, 143.6, 0.01); CHECK_NEAR(r.h, 18.7, 0.01);
        CHECK_NEAR(r.x + r.w * 0.5f, SLOT[k][0], 0.5); CHECK_NEAR(r.y + r.h * 0.5f, SLOT[k][1], 0.5);
        CHECK_NEAR(lx, SLOT[k][0], 0.01); CHECK_NEAR(ly, SLOT[k][1] - 2.0, 0.01);
    }
    UgRect r;
    ug_pc_pill_geom(1, &r, NULL, NULL);                                 /* slot 2: x +151 .. +294.6, y 194.0 .. 212.7 */
    CHECK_NEAR(r.x, 151, 0.5); CHECK_NEAR(r.x + r.w, 294.6, 0.5); CHECK_NEAR(r.y, 194.0, 0.7); CHECK_NEAR(r.y + r.h, 212.7, 0.7);
    ug_pc_pill_geom(0, &r, NULL, NULL);                                 /* slot 1: y 216.7 .. 235.8 */
    CHECK_NEAR(r.y, 216.7, 0.7); CHECK_NEAR(r.y + r.h, 235.8, 0.7);
    ug_pc_pill_geom(2, &r, NULL, NULL);                                 /* slot 3: x +3.8 .. +147.8 */
    CHECK_NEAR(r.x, 3.8, 0.5); CHECK_NEAR(r.x + r.w, 147.8, 0.5);
}

static void test_a3_pc_draw(void) {
    UgCtx c; ug_ctx_init(&c);
    static UgHintBar h;
    ug_hintbar_init(&h);
    snprintf(h.player, sizeof h.player, "DRIVER");
    h.mode = UG_HINTS_PC;
    UgHintSet s;
    mkhints(&s, 0, 1, 1, 0);
    ug_hintbar_set(&h, &s);
    ug_hintbar_layout(&h, &c);
    CHECK(h.nshown == 3);
    for (int i = 0; i < 20; i++) ug_hintbar_update(&h, &c, 16);
    UgCanvas cv;
    ug_cv_init(&cv);
    ug_hintbar_draw(&h, &c, &cv);
    int n = cv_shapes(&cv, g_q, QMAX);
    /* the strip pieces: player box dark, hint box lighter-weight black, both black (never grey) */
    const QInfo *pl = find_rect(g_q, n, -284.4f, 189.2f, -151.8f, 240.2f, 0.2f);
    const QInfo *hi = find_rect(g_q, n, -148.9f, 189.2f, 281.7f, 240.2f, 0.2f);
    CHECK(pl != NULL && hi != NULL);                                    /* 12 */
    if (pl && hi) {
        CHECK(pl->col.r == 0 && pl->col.g == 0 && pl->col.b == 0);
        CHECK(hi->col.r == 0 && hi->col.g == 0 && hi->col.b == 0);
        CHECK(pl->col.a > hi->col.a);                                   /* 13: darker than the hint box behind it */
        CHECK(pl->col.a >= 120 && pl->col.a <= 140);                    /* 215 x 0.6 */
    }
    /* 11: the console strip (7.1) is not drawn: none of its pieces */
    CHECK(find_rect(g_q, n, -277.1f, 190, -109.7f, 222, 0.5f) == NULL);
    CHECK(find_rect(g_q, n, -107, 190, 278, 222, 0.5f) == NULL);
    /* nothing above the strip's top edge or at the y = 206 strip: every shape lies within y >= 189 */
    float top = 1e9f;
    for (int i = 0; i < n; i++) if (g_q[i].y0 < top) top = g_q[i].y0;
    CHECK(top >= 187.0f);                                     /* strip top 189.2 less the shapes' AA padding */
    /* three pills at slots 1, 2, 3: outline shapes of 143.6 x 18.7 around the slot centres */
    int pills = 0;
    for (int i = 0; i < n; i++)
        if (g_q[i].mode == UG_M_OUTLINE && near_f(g_q[i].hw, 143.6f * 0.5f, 0.1f) && near_f(g_q[i].hh, 18.7f * 0.5f, 0.1f)) pills++;
    CHECK(pills == 3);
    /* Original-resource rendering replaces the former procedural shape assertion (20.1). */
    /* Original-resource rendering replaces the former procedural shape assertion (20.1). */
    /* Original-resource rendering replaces the former procedural shape assertion (20.1). */
    /* Original-resource rendering replaces the former procedural shape assertion (20.1). */
    /* 19: no key-cap or controller glyph anywhere: the PC mode draws no glyph art (round discs of the mouse
     * glyphs etc. are absent: the only rings are the pills' outlines) */
    CHECK(UG_TUNE_PC_MOUSE_GLYPH == 0);
    int rings = 0;
    for (int i = 0; i < n; i++) if (g_q[i].mode == UG_M_OUTLINE) rings++;
    CHECK(rings == 3);
    /* hover pulse and click */
    UgPtr mv = P(UG_PT_MOVE, 223, 203, 0, 0);
    int over = 0;
    CHECK(ug_hintbar_pointer(&h, &c, &mv, &over) == UG_A_NONE && over && h.hover == 1);
    UgPtr dn = P(UG_PT_DOWN, 223, 203, 1, 0);
    CHECK(ug_hintbar_pointer(&h, &c, &dn, &over) == UG_A_ACCEPT);       /* Continue = Accept */
    dn = P(UG_PT_DOWN, 223, 226, 1, 0);
    CHECK(ug_hintbar_pointer(&h, &c, &dn, &over) == UG_A_QUIT);
    dn = P(UG_PT_DOWN, 76, 203, 1, 0);
    CHECK(ug_hintbar_pointer(&h, &c, &dn, &over) == UG_A_BACK);         /* Back pill = the right mouse button's action */
    dn = P(UG_PT_DOWN, -72, 203, 1, 0);
    CHECK(ug_hintbar_pointer(&h, &c, &dn, &over) == UG_A_NONE && !over);
    ug_cv_free(&cv);
    ug_hintbar_free(&h);

    /* the library's mouse strip (7.6) keeps its own geometry; its left box is dark too (Q17) */
    ug_hintbar_init(&h);
    h.mode = UG_HINTS_STRIP;
    ug_hintbar_set(&h, &s);
    ug_hintbar_layout(&h, &c);
    ug_cv_init(&cv);
    ug_hintbar_draw(&h, &c, &cv);
    n = cv_shapes(&cv, g_q, QMAX);
    const QInfo *left = find_rect(g_q, n, -277.1f, 190, -109.7f, 222, 0.5f);
    CHECK(left != NULL);
    if (left) CHECK(left->col.r == 0 && left->col.g == 0 && left->col.b == 0);
    ug_cv_free(&cv);
    ug_hintbar_free(&h);
}

/* the bar through the UI stack: hidden under dialogs, Quit pill reaches the application */
static void test_a3_pc_bar_in_ui(void) {
    static UgUi u;
    ug_ui_init(&u);
    u.hints.mode = UG_HINTS_PC;
    static UgScreen root;
    static UgButton b;
    ug_screen_init(&root, "root", UG_SCR_ROOT);
    ug_button_init(&b, 1, UG_BTN_PILL, UG_RECT_C(0, 0, 100, 30), "Go");
    ug_screen_add(&root, &b.w);
    ug_screen_add(&root, &b.w);
    root.n = 1;
    ug_hints_add(&root.hints, UG_A_ACCEPT, "Select", 90);
    ug_ui_push(&u, &root);
    for (int i = 0; i < 40; i++) ug_ui_update(&u, 16);
    CHECK(ug_ui_hints_visible(&u));
    CHECK(u.hints.pc.n == 2 && u.hints.pc.p[0].action == UG_A_QUIT && u.hints.pc.p[1].action == UG_A_ACCEPT);   /* main menu: 15 */
    /* click the quit pill: the application is told, the screen is untouched */
    UgPtr mv = P(UG_PT_MOVE, 223, 226, 0, 0), dn = P(UG_PT_DOWN, 223, 226, 1, 0), up = P(UG_PT_UP, 223, 226, 1, 0);
    ug_ui_pointer(&u, &mv); ug_ui_pointer(&u, &dn); ug_ui_pointer(&u, &up);
    UgUiEvent e;
    CHECK(ug_ui_poll(&u, &e) && e.kind == UG_EV_QUIT);
    CHECK(u.ctx.pointer_visible);                                      /* clicking a pill keeps the cursor */
    /* a root page has no Back pill; a pushed page does */
    static UgScreen page;
    ug_screen_init(&page, "page", 0);
    ug_ui_push(&u, &page);
    for (int i = 0; i < 80; i++) ug_ui_update(&u, 16);
    CHECK(u.hints.pc.n == 2 && u.hints.pc.p[1].action == UG_A_BACK);   /* 16: options-like page */
    page.flags |= UG_SCR_NOBACKHINT;
    for (int i = 0; i < 5; i++) ug_ui_update(&u, 16);
    CHECK(u.hints.pc.n == 1);                                          /* the flag removes Back (PC main menu rule) */
    ug_screen_free(&page);
    ug_screen_free(&root);
    ug_ui_free(&u);
}

/* ------------------------------------------------------------------ */
/* original-screen viewer: the icon strip on a hand-built screen          */
/* ------------------------------------------------------------------ */
typedef struct { UgFngPackage pk; UgFngResource res[2]; UgFngObject *obj; } SynMain;

static void syn_main_build(SynMain *m, DemoViewer *v) {
    memset(m, 0, sizeof *m);
    memset(v, 0, sizeof *v);
    enum { POOL = 8 };
    int n = POOL + 7;
    m->obj = (UgFngObject *)calloc((size_t)n, sizeof(UgFngObject));
    m->res[0].kind = 1; m->res[0].name = (char *)"U2_Scroller_Tab.tga";
    UgFngObject *o = m->obj;
    int k = 0;
    for (int i = 0; i < POOL; i++, k++) {          /* the pool: off-screen item buttons */
        o[k].type = UG_OBJ_IMAGE; o[k].guid = 100 + (uint32_t)k; o[k].flags = UG_OF_BUTTON | UG_OF_NO_DIRFOCUS;
        o[k].color = UG_WHITE; o[k].rot[3] = 1; o[k].res_index = -1;
        o[k].size[0] = 108; o[k].size[1] = 44; o[k].pos[0] = -500; o[k].pos[1] = -134;
    }
    struct { float x, y, w, h; int res; } F[7] = {
        {0, -133.5f, 118, 59, -1},             /* the slot */
        {0, -90, 40, 20, -1},                  /* title text (replaced below) */
        {-280, -171, 16, 16, 0},               /* thumb */
        {0, -172, 564, 8, -1},                 /* track */
        {-270, -132.5f, 26, 26, -1},           /* left arrow */
        {270, -132.5f, 26, 26, -1},            /* right arrow */
        {0, 0, 1, 1, -1}};
    for (int i = 0; i < 6; i++, k++) {
        o[k].type = i == 1 ? UG_OBJ_TEXT : UG_OBJ_IMAGE; o[k].guid = 100 + (uint32_t)k;
        o[k].color = UG_WHITE; o[k].rot[3] = 1; o[k].res_index = F[i].res;
        o[k].pos[0] = F[i].x; o[k].pos[1] = F[i].y; o[k].size[0] = F[i].w; o[k].size[1] = F[i].h; o[k].size[2] = 1;
        if (i == 1) { o[k].is_text = 1; o[k].text = (char *)"Option title"; o[k].size[0] = o[k].size[1] = 0.7f; }
    }
    m->pk.obj = o; m->pk.nobj = k;
    m->pk.res = m->res; m->pk.nres = 1;
    v->pk = m->pk;
    snprintf(v->name, sizeof v->name, "UI_Main.fng");
    ug_scene_init(&v->sc, &v->pk, NULL);
    ug_scene_set_assets(&v->sc, NULL);
    v->loaded = 1;
    ug_scene_update(&v->sc, 0);
}

static void syn_main_settle(DemoViewer *v, int frames) {
    for (int i = 0; i < frames; i++) { ug_scene_update(&v->sc, 16); orig_bind_apply(v, 16); }
}

/* visible icons: nodes among slot + pool with alpha > 0 whose centre lies on the screen */
static int syn_visible_icons(DemoViewer *v, int *idx, int cap) {
    int n = 0;
    const IconBind *b = &v->bind;
    for (int k = -1; k < b->npool; k++) {
        int node = k < 0 ? b->slot : b->pool[k];
        const UgNode *nd = &v->sc.nodes[node];
        if (!nd->visible || nd->wcolor.a == 0) continue;
        if (fabsf(nd->we) > 320.0f || fabsf(nd->wf) > 240.0f) continue;
        if (n < cap) idx[n] = node;
        n++;
    }
    return n;
}

static void test_a3_viewer_icon_strip(void) {
    static SynMain m;
    static DemoViewer v;
    syn_main_build(&m, &v);
    CHECK(orig_bind_detect(&v));
    CHECK(v.bind.npool == 8 && v.bind.slot >= 0 && v.bind.thumb >= 0 && v.bind.track >= 0 && v.bind.title >= 0);
    CHECK(v.bind.arrow_l >= 0 && v.bind.arrow_r >= 0);
    int tex[7] = {1, 2, 3, 4, 5, 6, 7};
    char labels[7][40];
    for (int i = 0; i < 7; i++) snprintf(labels[i], 40, "Item %d", i);
    orig_bind_set_items(&v, 7, tex, (const char (*)[40])labels);
    CHECK(v.bind.n == 7 && v.bind.active);
    UgCtx c; ug_ctx_init(&c);
    syn_main_settle(&v, 5);
    /* C1: the second item selected */
    CHECK(orig_bind_step(&v, &c, +1));
    syn_main_settle(&v, 120);
    CHECK(v.bind.sel == 1);
    int idx[16];
    int vis = syn_visible_icons(&v, idx, 16);
    CHECK_MSG(vis == 3, "visible icons %d", vis);                       /* 2: exactly three */
    const UgNode *slot = &v.sc.nodes[v.bind.slot];
    CHECK_NEAR(slot->we, 0, 0.1); CHECK_NEAR(slot->wf, -133.5, 0.1);    /* 3 */
    CHECK_NEAR(fabsf(slot->props.w[UG_P_SIZE]), 118, 0.1); CHECK_NEAR(fabsf(slot->props.w[UG_P_SIZE + 1]), 59, 0.1);
    CHECK(slot->tex_override == 2);                                     /* the selected icon's texture */
    int nb = 0;
    for (int i = 0; i < vis; i++) {
        const UgNode *nd = &v.sc.nodes[idx[i]];
        if (idx[i] == v.bind.slot) continue;
        nb++;
        CHECK_NEAR(fabsf(nd->we), 158, 0.1); CHECK_NEAR(nd->wf, -133.5, 0.1);                     /* 4 */
        CHECK_NEAR(fabsf(nd->props.w[UG_P_SIZE]), 96, 1.5); CHECK_NEAR(fabsf(nd->props.w[UG_P_SIZE + 1]), 48, 1.5);
        CHECK(nd->wcolor.a == 255);                                     /* full opacity */
    }
    CHECK(nb == 2);
    CHECK(v.sc.nodes[v.bind.arrow_l].visible && v.sc.nodes[v.bind.arrow_r].visible);   /* 6 */
    CHECK_NEAR(v.sc.nodes[v.bind.arrow_l].we, -270, 0.1); CHECK_NEAR(v.sc.nodes[v.bind.arrow_r].we, 270, 0.1);
    CHECK_NEAR(v.sc.nodes[v.bind.thumb].we, -182.4, 0.2);               /* 7 */
    CHECK(v.sc.nodes[v.bind.title].text_override && !strcmp(v.sc.nodes[v.bind.title].text_override, "Item 1"));
    /* to the last item: no right neighbour, right arrow hidden, thumb at 273.8 (8) */
    for (int i = 0; i < 12; i++) orig_bind_step(&v, &c, +1);
    CHECK(v.bind.sel == 6);                                             /* Right at the end is ignored: no wrap */
    syn_main_settle(&v, 150);
    vis = syn_visible_icons(&v, idx, 16);
    CHECK_MSG(vis == 2, "visible icons at the end %d", vis);
    CHECK(!v.sc.nodes[v.bind.arrow_r].visible && v.sc.nodes[v.bind.arrow_l].visible);
    CHECK_NEAR(v.sc.nodes[v.bind.thumb].we, 273.8, 0.2);
    for (int i = 0; i < vis; i++) CHECK(v.sc.nodes[idx[i]].we < 1.0f);  /* nothing right of the centre slot */
    /* back to the start: left arrow hidden */
    for (int i = 0; i < 12; i++) orig_bind_step(&v, &c, -1);
    syn_main_settle(&v, 150);
    CHECK(v.bind.sel == 0 && !v.sc.nodes[v.bind.arrow_l].visible && v.sc.nodes[v.bind.arrow_r].visible);
    CHECK_NEAR(v.sc.nodes[v.bind.thumb].we, -273.7, 0.2);
    vis = syn_visible_icons(&v, idx, 16);
    CHECK(vis == 2);
    /* mid-slide the icons interpolate (never more than three icons plus the one leaving) */
    orig_bind_step(&v, &c, +1);
    syn_main_settle(&v, 3);
    vis = syn_visible_icons(&v, idx, 16);
    CHECK(vis >= 2 && vis <= 4);
    ug_scene_free(&v.sc);
    free(m.obj);
}

/* ------------------------------------------------------------------ */
/* demo flows: Quit pill -> confirmation (checklist C3), viewer bar      */
/* ------------------------------------------------------------------ */
static void test_a3_demo_quit_flow(void) {
    static DemoApp app;
    DemoApp *a = &app;
    demo_init(a, g_vec_dir);
    a->ui.hints.mode = UG_HINTS_PC;
    for (int i = 0; i < 60; i++) demo_update(a, 16);
    CHECK(a->ui.hints.pc.n == 2 && a->ui.hints.pc.p[0].action == UG_A_QUIT);
    UgPtr mv = P(UG_PT_MOVE, 223, 226, 0, 0), dn = P(UG_PT_DOWN, 223, 226, 1, 0), up = P(UG_PT_UP, 223, 226, 1, 0);
    ug_ui_pointer(&a->ui, &mv); ug_ui_pointer(&a->ui, &dn); ug_ui_pointer(&a->ui, &up);
    for (int i = 0; i < 60; i++) demo_update(a, 16);
    CHECK(ug_ui_top(&a->ui) == &a->dlg.scr && a->dlg.size == 0 && a->dlg.nbtn == 2);   /* SMALL, two buttons (C3) */
    CHECK(a->dlg.scr.focus == 1);                                       /* "No" has focus */
    CHECK(!ug_ui_hints_visible(&a->ui));                                /* the bar is not drawn under the dialog */
    ug_ui_action(&a->ui, UG_A_ACCEPT);                                  /* default choice: No */
    for (int i = 0; i < 80; i++) demo_update(a, 16);
    CHECK(ug_ui_top(&a->ui) == &a->root.scr && !a->quit_requested && ug_ui_hints_visible(&a->ui));
    /* again, choose Yes with the mouse */
    ug_ui_pointer(&a->ui, &mv); ug_ui_pointer(&a->ui, &dn); ug_ui_pointer(&a->ui, &up);
    for (int i = 0; i < 60; i++) demo_update(a, 16);
    CHECK(ug_ui_top(&a->ui) == &a->dlg.scr);
    UgRect yes = a->dlg.btn[0].w.bounds;
    UgPtr ym = P(UG_PT_MOVE, yes.x + yes.w * 0.5f, yes.y + yes.h * 0.5f, 0, 0);
    UgPtr yd = P(UG_PT_DOWN, ym.x, ym.y, 1, 0), yu = P(UG_PT_UP, ym.x, ym.y, 1, 0);
    ug_ui_pointer(&a->ui, &ym); ug_ui_pointer(&a->ui, &yd); ug_ui_pointer(&a->ui, &yu);
    for (int i = 0; i < 80; i++) demo_update(a, 16);
    CHECK(a->quit_requested);
    /* the root page's pill: Back-less main menu, Accept -> Continue */
    a->quit_requested = 0;
    /* Back pill on a pushed page, right-click equivalent */
    open_page_by_mouse(a, DP_OPTIONS);
    for (int i = 0; i < 40; i++) demo_update(a, 16);
    CHECK(ug_ui_top(&a->ui) == &a->options.scr);
    CHECK(a->ui.hints.pc.n >= 2 && a->ui.hints.pc.p[1].action == UG_A_BACK);        /* 16: Quit Game, Back */
    ug_ui_pointer(&a->ui, &(UgPtr){UG_PT_DOWN, 223, 203, 1, 0});
    for (int i = 0; i < 100; i++) demo_update(a, 16);
    CHECK(ug_ui_top(&a->ui) == &a->root.scr);                          /* clicking the Back pill went back */
    /* the PC controls page offers the third pill (18) */
    open_page_by_mouse(a, DP_PCCTL);
    for (int i = 0; i < 40; i++) demo_update(a, 16);
    CHECK(ug_ui_top(&a->ui) == &a->pcctl.scr);
    CHECK(a->ui.hints.pc.n == 3 && a->ui.hints.pc.p[2].action == UG_A_BTN1 && !strcmp(a->ui.hints.pc.p[2].label, "Reset Keys To Default"));
    a->pcctl.list.rows[0].sel = 2;
    ug_ui_pointer(&a->ui, &(UgPtr){UG_PT_DOWN, 76, 203, 1, 0});
    for (int i = 0; i < 20; i++) demo_update(a, 16);
    CHECK(a->pcctl.list.rows[0].sel == 0);                             /* the pill really resets the keys */
    ug_ui_action(&a->ui, UG_A_BACK);
    for (int i = 0; i < 100; i++) demo_update(a, 16);
    /* the car-select page: Quit Game, Continue, Back (17) */
    open_page_by_mouse(a, DP_CARS);
    for (int i = 0; i < 40; i++) demo_update(a, 16);
    CHECK(a->ui.hints.pc.n >= 3 && a->ui.hints.pc.p[1].action == UG_A_ACCEPT && a->ui.hints.pc.p[2].action == UG_A_BACK);
    ug_ui_action(&a->ui, UG_A_BACK);
    for (int i = 0; i < 100; i++) demo_update(a, 16);
    demo_free(a);
}

/* ------------------------------------------------------------------ */
/* Current console profile supersedes historical PC strip visibility.  */
/* ------------------------------------------------------------------ */
static void test_a3_scene_console_strip(void) {
    CHECK(UG_TUNE_SHOW_CONSOLE_STRIP == 1);
    UgFngPackage pk;
    memset(&pk, 0, sizeof pk);
    pk.nobj = 3;
    pk.obj = (UgFngObject *)calloc(3, sizeof(UgFngObject));
    for (int i = 0; i < 3; i++) {
        UgFngObject *o = &pk.obj[i];
        o->type = UG_OBJ_IMAGE; o->guid = 1 + (uint32_t)i; o->color = UG_WHITE; o->rot[3] = 1; o->res_index = -1;
        o->size[0] = i == 0 ? 167.4f : 385; o->size[1] = 32; o->size[2] = 1; o->pos[1] = 206;
        o->pos[0] = i == 0 ? -193 : 85;
        o->flags = i < 2 ? UG_OF_CONSOLE : 0;        /* two console-only strip pieces and one plain panel */
    }
    UgScene sc;
    CHECK(ug_scene_init(&sc, &pk, NULL));
    sc.console_strip = UG_TUNE_SHOW_CONSOLE_STRIP;
    ug_scene_update(&sc, 0);
    CHECK(sc.nodes[0].visible && sc.nodes[1].visible && sc.nodes[2].visible);
    ug_scene_free(&sc);
    free(pk.obj);
}

/* ------------------------------------------------------------------ */
/* data: the user's install through the library's own loader             */
/* ------------------------------------------------------------------ */
static void test_a3_with_game_data(void) {
    const char *dir = test_data_dir();
    if (!dir) { printf("skipped (set UGUI_DATA to run) "); return; }
    static DemoApp app;
    DemoApp *a = &app;
    demo_init_data(a, g_vec_dir, dir);
    if (!a->have_assets) { printf("(no usable install at %s) ", dir); CHECK(0); demo_free(a); return; }
    const UgFace *face = ug_assets_main_face(&a->assets);
    CHECK(face != NULL);
    if (face) {
        CHECK(face->cell_height == 32 && face->line_height == 26);      /* the font header of 16.4 */
        ug_text_set_face(face);
        UgTextLayout tl;
        ug_text_layout(&tl, "Do you want to^save?", 0, UG_R_DLG_LEADING, UG_GLYPHS_MOUSE);
        CHECK_NEAR(tl.pitch * 0.75, 25.5, 1e-3);                        /* 36: baseline pitch 25.4 +- 0.5 */
        CHECK(fabs(tl.pitch * 0.75 - 25.4) <= 0.5);
        ug_text_layout_free(&tl);
        ug_text_set_face(NULL);
    }
    /* 39: the cursor texture exists in the PC global pack and is drawn at 32 x 32 */
    int ct = ug_assets_texture_by_name(&a->assets, UG_CURSOR_TEXTURE);
    CHECK(ct >= 0);
    if (ct >= 0) CHECK(a->assets.tex.t[ct].w == 32 && a->assets.tex.t[ct].h == 32);
    const UgSkin *sk = ug_kit_skin();
    CHECK(sk && sk->cursor == ct + 1);
    /* the PC pill art and the box textures resolve */
    CHECK(sk && sk->pill && sk->box_b);

    /* the main menu in the viewer: C1 checklist numbers on the real screen */
    if (!ug_assets_package(&a->assets, "UI_Main.fng")) { CHECK(0); demo_free(a); return; }
    CHECK(demo_open_original_named(a, "UI_Main.fng"));
    for (int i = 0; i < 120; i++) demo_update(a, 16);
    DemoViewer *v = &a->viewer.vw;
    CHECK(v->loaded && v->bind.active);
    CHECK_MSG(v->bind.n == 7, "main menu items %d", v->bind.n);          /* the PC main menu has 7 items (6.4) */
    CHECK(a->ui.hints.mode == UG_HINTS_PC);
    CHECK(!v->sc.console_strip);
    CHECK(!a->ui.hints.no_bg); /* exactly one authored strip; no library duplicate */
    ug_ui_action(&a->ui, UG_A_RIGHT);
    for (int i = 0; i < 150; i++) demo_update(a, 16);
    CHECK(v->bind.sel == 1);
    int idx[24];
    int vis = syn_visible_icons(v, idx, 24);
    CHECK_MSG(vis == 3, "visible icons %d", vis);
    const UgNode *slot = &v->sc.nodes[v->bind.slot];
    CHECK_MSG(near_f(slot->we, 0, 1.0f) && near_f(slot->wf, -133.5f, 1.0f), "slot at (%.2f, %.2f)", slot->we, slot->wf);
    CHECK(near_f(fabsf(slot->props.w[UG_P_SIZE]), 118, 0.5f) && near_f(fabsf(slot->props.w[UG_P_SIZE + 1]), 59, 0.5f));
    for (int i = 0; i < vis; i++)
        if (idx[i] != v->bind.slot) {
            const UgNode *nd = &v->sc.nodes[idx[i]];
            CHECK_MSG(near_f(fabsf(nd->we), 158, 1.0f) && near_f(nd->wf, slot->wf, 1.0f), "neighbour at (%.2f, %.2f)", nd->we, nd->wf);
            CHECK(near_f(fabsf(nd->props.w[UG_P_SIZE]), 96, 1.5f) && nd->wcolor.a == 255);
        }
    if (v->bind.arrow_l >= 0 && v->bind.arrow_r >= 0) {
        CHECK(near_f(v->sc.nodes[v->bind.arrow_l].we, -270, 1.5f) && near_f(v->sc.nodes[v->bind.arrow_l].wf, -132.5f, 1.5f));
        CHECK(v->sc.nodes[v->bind.arrow_l].visible && v->sc.nodes[v->bind.arrow_r].visible);
    } else CHECK_MSG(0, "arrow badges not found (%d, %d)", v->bind.arrow_l, v->bind.arrow_r);
    if (v->bind.thumb >= 0) CHECK_MSG(near_f(v->sc.nodes[v->bind.thumb].we, -182.4f, 0.5f), "thumb at %.2f", v->sc.nodes[v->bind.thumb].we);
    else CHECK_MSG(0, "thumb not found");
    /* the last item (C3 background): options, no right neighbour, right arrow hidden, thumb 273.8 */
    for (int i = 0; i < 8; i++) ug_ui_action(&a->ui, UG_A_RIGHT);
    for (int i = 0; i < 200; i++) demo_update(a, 16);
    CHECK(v->bind.sel == 6);
    vis = syn_visible_icons(v, idx, 24);
    CHECK_MSG(vis == 2, "visible icons at the end %d", vis);
    if (v->bind.arrow_r >= 0) CHECK(!v->sc.nodes[v->bind.arrow_r].visible);
    if (v->bind.thumb >= 0) CHECK_MSG(near_f(v->sc.nodes[v->bind.thumb].we, 273.8f, 0.5f), "thumb at %.2f", v->sc.nodes[v->bind.thumb].we);
    ug_ui_action(&a->ui, UG_A_BACK);       /* the viewer closes (the screen has no Back pill, the key still leaves) */
    for (int i = 0; i < 200; i++) demo_update(a, 16);
    demo_free(a);
}

static void run_amend3_tests(void) {
    printf("[I] amendment 3: reference comparison checklist (SPEC 19.2)\n");
    RUN(test_a3_pillbox);
    /* test_a3_cursor: presentation superseded by SPEC 20; current behavior tested in amendment 4. */
    RUN(test_a3_icon_layout);
    RUN(test_a3_icon_widget);
    RUN(test_a3_options);
    RUN(test_a3_carselect);
    RUN(test_a3_dialog_geometry);
    RUN(test_a3_dialog_behaviour);
    /* test_a3_dialog_drawn: presentation superseded by SPEC 20; current behavior tested in amendment 4. */
    RUN(test_a3_text_pitch);
    RUN(test_a3_pc_assign);
    RUN(test_a3_pc_geometry);
    /* test_a3_pc_draw: presentation superseded by SPEC 20; current behavior tested in amendment 4. */
    /* test_a3_pc_bar_in_ui: presentation superseded by SPEC 20; current behavior tested in amendment 4. */
    RUN(test_a3_scene_console_strip);
    RUN(test_a3_viewer_icon_strip);
    /* test_a3_demo_quit_flow: presentation superseded by SPEC 20; current behavior tested in amendment 4. */
    RUN(test_a3_with_game_data);
}
