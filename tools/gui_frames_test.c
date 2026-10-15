/* gui_frames_test -- coordinate frames, group composition and the anchor read rule of the authored
 * screens (SPEC_GUI_ART_AMEND1 A1-A8, B1-B4) against test-vectors-gui/tvg12_frames.json.
 *
 *   gui_frames_test [VECTOR_DIR]
 *
 * Synthetic data only: the fixtures are in-memory packages that copy the object names, hashes,
 * sizes, parent chains and script names the amendment publishes. No retail package, texture or text
 * is opened. The "real-package-like" fixtures model the hazard behind verifier defect D1: the
 * anchor objects (OPTION_MASTER, TITLE_MASTER, DATA_MASTER) have an Init script that is
 * automatically followed by a second script which hides and moves them, so any read of the anchor
 * after the chain has run returns the wrong position (A6). The suite fails if the anchor is read
 * late, for the carousels and for the list screens, both directly on the scene and through the
 * adapter (tools/pause_menu_test.c). Exit code is nonzero if any check fails. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/gui_timing.h"
#include "../src/gui_view.h"
#include "../src/pause_screen.h"
#include "synth_pkgs.h"
#include "synth_fng.h"

static int g_checks, g_fail;
#define CHECK(cond, ...) do { g_checks++; if (!(cond)) { g_fail++; \
    if (g_fail <= 80) { printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } } while (0)
#define NEAR(a, b, tol) (fabs((double)(a) - (double)(b)) <= (tol))

static const char *g_dir = "docs/cleanroom-settings/test-vectors-gui";
#include "test_json.h"

#define PRESENTATION_HASH 0x8D7E0C53u       /* presentation group of the pause/hub packages */
#define ROW_GROUP_HASH 0x371F1C93u
#define GLOW_HASH 0xDF422345u
#define BACKING_HASH 0x687FDA31u
#define STRIP_HASH 0x5A11CE00u              /* synthetic name of the light strip image (the package leaves it unnamed) */

static double tol_centre = 1.0, tol_size = 1.0, tol_retail = 2.5;

static J *case_by_name(J *cases, const char *name) {
    for (int i = 0; i < cases->n; i++) {
        const char *n = jstrv(cases->kid[i], "name");
        if (n && !strcmp(n, name)) return cases->kid[i];
    }
    return NULL;
}
static double jnum_at(const J *a, int i) { return a && a->t == J_ARR && i < a->n ? a->kid[i]->num : 0; }

/* ---- drawn rectangle of a node in pixels -------------------------------------------------------------------- */
static void node_px(const UgNode *n, GlPxMap m, double c[2], double sz[2]) {
    GlPt p = gl_px_point(&m, n->we, n->wf);
    c[0] = p.x; c[1] = p.y;
    float sx = hypotf(n->wa, n->wb), sy = hypotf(n->wc, n->wd);
    sz[0] = gl_px_len(&m, fabsf(n->props.w[UG_P_SIZE]) * sx);
    sz[1] = gl_px_len(&m, fabsf(n->props.w[UG_P_SIZE + 1]) * sy);
}
/* Compares a node with an "expected_px" object ({"1920x1080": {"centre": [x,y], "size": [w,h]}, ...}). */
static void check_px(const char *what, const UgNode *n, const J *epx, double tol_c) {
    for (int i = 0; epx && i < epx->n; i++) {
        int W = 0, H = 0;
        if (sscanf(epx->key[i], "%dx%d", &W, &H) != 2) continue;
        GlPxMap m = gl_px_map(W, H);
        double c[2], sz[2];
        node_px(n, m, c, sz);
        const J *ec = jget(epx->kid[i], "centre"), *es = jget(epx->kid[i], "size");
        CHECK(ec && NEAR(c[0], jnum_at(ec, 0), tol_c) && NEAR(c[1], jnum_at(ec, 1), tol_c),
              "%s @%s: centre (%.3f, %.3f) px, expected (%.3f, %.3f)", what, epx->key[i], c[0], c[1], jnum_at(ec, 0), jnum_at(ec, 1));
        if (es)
            CHECK(NEAR(sz[0], jnum_at(es, 0), tol_size) && NEAR(sz[1], jnum_at(es, 1), tol_size),
                  "%s @%s: size %.3f x %.3f px, expected %.3f x %.3f", what, epx->key[i], sz[0], sz[1], jnum_at(es, 0), jnum_at(es, 1));
    }
}

/* ---- A1: the mapping -------------------------------------------------------------------------------------------- */
static void test_mapping(J *j) {
    J *res = jget(jget(j, "mapping"), "resolutions");
    CHECK(res && res->n == 3, "three resolutions in the vector");
    for (int i = 0; res && i < res->n; i++) {
        J *r = res->kid[i];
        int W = (int)jnum(r, "W", 0), H = (int)jnum(r, "H", 0);
        GlPxMap m = gl_px_map(W, H);
        CHECK(NEAR(m.s, jnum(r, "s", 0), 1e-6), "%dx%d: s = %f", W, H, m.s);
        J *o = jget(r, "origin");
        CHECK(NEAR(m.ox, jnum_at(o, 0), 1e-4) && NEAR(m.oy, jnum_at(o, 1), 1e-4), "%dx%d: origin (%f, %f)", W, H, m.ox, m.oy);
        /* the renderer's own viewport fit is the same mapping */
        UgView v = ug_view_fit(W, H);
        CHECK(NEAR(v.scale, m.s, 1e-6) && NEAR(v.x0, m.ox - 320.0f * m.s, 1.0) && NEAR(v.y0, m.oy - 240.0f * m.s, 1.0),
              "%dx%d: ug_view_fit agrees (scale %f, x0 %d y0 %d)", W, H, v.scale, v.x0, v.y0);
        float vx, vy;
        ug_view_to_virtual(&v, W / 2, H / 2, &vx, &vy);
        CHECK(NEAR(vx, 0, 0.6) && NEAR(vy, 0, 0.6), "%dx%d: the window centre is the virtual origin", W, H);
    }
    /* A1 worked numbers: pillarbox at 16:9, columns 240..1679 at 1080p; sizes scale on both axes */
    GlPxMap m = gl_px_map(1920, 1080);
    GlPt l = gl_px_point(&m, -320, 0), r = gl_px_point(&m, 320, 0);
    CHECK(NEAR(l.x, 240, 1e-3) && NEAR(r.x, 1680, 1e-3), "pillarbox: content spans pixels 240..1680");
    CHECK(NEAR(gl_px_len(&m, 118), 265.5, 1e-3) && NEAR(gl_px_len(&m, 59), 132.75, 1e-3), "118 x 59 units at 1080p");
    GlPt c = gl_px_point(&m, 0.38835f, -14.29613f);
    CHECK(NEAR(c.x, 960.874, 0.01) && NEAR(c.y, 507.834, 0.01), "the anchor at 1080p (%.3f, %.3f)", c.x, c.y);
    /* below 4:3 the same min rule (guessed, AMEND1 C1) and a degenerate output */
    GlPxMap q = gl_px_map(1280, 1024);
    CHECK(NEAR(q.s, 2.0, 1e-6) && NEAR(q.ox, 640, 1e-4) && NEAR(q.oy, 512, 1e-4), "5:4 output letterboxes with s = W/640");
    GlPxMap z = gl_px_map(0, 0);
    CHECK(z.s == 0, "no output, no scale");
    /* the third component of a position never reaches x/y: a deep child sits at its x/y sum */
    Pkg *p = (Pkg *)calloc(1, sizeof *p);
    UgFngObject *g = add(p, UG_OBJ_GROUP, NULL, 10, 20, 1, 1);
    g->pos[2] = 533;
    UgFngObject *c2 = add(p, UG_OBJ_IMAGE, NULL, 5, 6, 8, 8);
    c2->pos[2] = 583; c2->has_parent = 1; c2->parent_guid = g->guid;
    finish(p);
    UgScene s;
    ug_scene_init(&s, &p->pk, NULL);
    CHECK(NEAR(s.nodes[1].we, 15, 1e-4) && NEAR(s.nodes[1].wf, 26, 1e-4), "z does not change x/y: (%.3f, %.3f)", s.nodes[1].we, s.nodes[1].wf);
    ug_scene_free(&s);
    free(p);
}

/* ---- A2/A3: composition of every chain case of the vector ------------------------------------------------ */
static int has_sub(const char *s, const char *sub) { return s && strstr(s, sub) != NULL; }

static void test_composition(J *cases) {
    int ran = 0;
    for (int ci = 0; ci < cases->n; ci++) {
        J *c = cases->kid[ci], *chain = jget(c, "chain");
        const char *name = jstrv(c, "name");
        if (!chain || chain->t != J_ARR || chain->n < 1) continue;
        Pkg *p = (Pkg *)calloc(1, sizeof *p);
        UgFngObject *prev = NULL;
        for (int k = chain->n - 1; k >= 1; k--) {                    /* top-level group first */
            J *e = chain->kid[k], *loc = jget(e, "local"), *sc = jget(e, "scale");
            UgFngObject *g = add(p, UG_OBJ_GROUP, NULL, (float)jnum_at(loc, 0), (float)jnum_at(loc, 1), (float)jnum_at(sc, 0), (float)jnum_at(sc, 1));
            if (prev) { g->has_parent = 1; g->parent_guid = prev->guid; }
            prev = g;
        }
        J *e0 = chain->kid[0], *loc0 = jget(e0, "local"), *sz0 = jget(e0, "size"), *szv = jget(c, "size_virtual");
        const char *oname = jstrv(e0, "object");
        int is_text = has_sub(oname, "text anchor");
        float w = 1, h = 1;
        if (sz0) { w = (float)jnum_at(sz0, 0); h = (float)jnum_at(sz0, 1); }
        else if (szv && szv->t == J_ARR) { w = (float)jnum_at(szv, 0); h = (float)jnum_at(szv, 1); }
        UgFngObject *leaf = add(p, is_text ? UG_OBJ_TEXT : UG_OBJ_IMAGE, NULL, (float)jnum_at(loc0, 0), (float)jnum_at(loc0, 1), w, h);
        if (has_sub(oname, "rotated 180")) { leaf->rot[0] = 0; leaf->rot[1] = 0; leaf->rot[2] = 1; leaf->rot[3] = 0; }
        if (prev) { leaf->has_parent = 1; leaf->parent_guid = prev->guid; }
        finish(p);
        UgScene s;
        CHECK(ug_scene_init(&s, &p->pk, NULL), "%s: scene init", name);
        const UgNode *n = &s.nodes[p->n - 1];
        J *evc = jget(c, "expected_virtual_centre");
        CHECK(evc && NEAR(n->we, jnum_at(evc, 0), 1e-3) && NEAR(n->wf, jnum_at(evc, 1), 1e-3),
              "%s: composed centre (%.4f, %.4f), expected (%.4f, %.4f)", name, n->we, n->wf, jnum_at(evc, 0), jnum_at(evc, 1));
        if (szv && szv->t == J_ARR) {
            float dw = fabsf(n->props.w[UG_P_SIZE]) * hypotf(n->wa, n->wb), dh = fabsf(n->props.w[UG_P_SIZE + 1]) * hypotf(n->wc, n->wd);
            CHECK(NEAR(dw, jnum_at(szv, 0), 0.01) && NEAR(dh, jnum_at(szv, 1), 0.01), "%s: drawn size %.3f x %.3f", name, dw, dh);
        }
        check_px(name, n, jget(c, "expected_px"), tol_centre);
        /* retail cross-checks are informative, tolerance 2.5 px */
        J *rc = jget(c, "retail_crosscheck");
        if (rc) {
            GlPxMap m = gl_px_map(1920, 1080);
            double cc[2], sz[2];
            node_px(n, m, cc, sz);
            J *line = jget(rc, "1920x1080_focus_frame_line_centre"), *tab = jget(rc, "1920x1080"), *ys = jget(rc, "1920x1080_strip_y_px");
            if (line) CHECK(NEAR(cc[0], jnum_at(line, 0), tol_retail) && NEAR(cc[1], jnum_at(line, 1), tol_retail), "%s: retail frame line (%.2f, %.2f)", name, cc[0], cc[1]);
            if (tab) CHECK(NEAR(cc[0], jnum_at(tab, 0), tol_retail) && NEAR(cc[1], jnum_at(tab, 1), tol_retail), "%s: retail position (%.2f, %.2f)", name, cc[0], cc[1]);
            if (ys) CHECK(NEAR(cc[1] - sz[1] / 2, jnum_at(ys, 0), tol_retail) && NEAR(cc[1] + sz[1] / 2, jnum_at(ys, 1), tol_retail), "%s: retail strip y %.2f..%.2f", name, cc[1] - sz[1] / 2, cc[1] + sz[1] / 2);
        }
        ug_scene_free(&s);
        free(p);
        ran++;
    }
    CHECK(ran >= 17, "the vector has %d composition cases", ran);
}

/* ---- the real-package-like fixtures ------------------------------------------------------------------------ */

/* FadeIn / fadeout / DIM / UnDim of the pause and hub objects (SPEC_GUI_ART 3.4, A4, B2): colour tracks
 * whose keys are deltas on the base alpha 255; with `pos_base_x` the group's scripts carry a position
 * base (the hub's -39.26 against the stored -39.0). */
static void fade_scripts(Pkg *p, UgFngObject *o, uint32_t out_ms, int with_pos, float pos_x, float pos_y) {
    struct { const char *name; uint32_t len; int t[2]; float a[2]; } sc[4] = {
        { "FadeIn", 100, { 0, 100 }, { -255, 0 } },
        { "fadeout", out_ms, { 0, (int)out_ms }, { 0, -255 } },
        { "DIM", 100, { 0, 100 }, { 0, -195 } },
        { "UnDim", 100, { 0, 100 }, { -195, 0 } },
    };
    for (int i = 0; i < 4; i++) {
        UgScript *s = script(p, o, hn(sc[i].name), sc[i].len);
        const float cb[4] = { 255, 255, 255, 255 };
        const float dv[2][4] = { { 0, 0, 0, sc[i].a[0] }, { 0, 0, 0, sc[i].a[1] } };
        track(p, s, UG_VT_COLOR, UG_P_COLOR, cb, 2, sc[i].t, dv);
        if (with_pos) {
            const float pb[4] = { pos_x, pos_y, 1, 0 };
            track(p, s, UG_VT_VEC3, UG_P_POS, pb, 0, NULL, NULL);
        }
    }
    respond(p, o, GT_MSG_ENTER, UG_CMD_START_SCRIPT, hn("FadeIn"));
    respond(p, o, GT_MSG_EXIT, UG_CMD_START_SCRIPT, hn("fadeout"));
    respond(p, o, UG_MSG_DIM, UG_CMD_START_SCRIPT, hn("DIM"));
    respond(p, o, UG_MSG_UNDIM, UG_CMD_START_SCRIPT, hn("UnDim"));
}

/* UI_pause.fng / UI_PauseOptionsMain.fng as AMEND1 describes their frames (A2-A7). */
static void build_carousel_real(Pkg *p, int hub, uint32_t init_len) {
    UgFngObject *m = add(p, UG_OBJ_IMAGE, "OPTION_MASTER", 0, 0, 118, 59);
    anchor_chain(p, m, 0, 0, 0, 0, 0);
    anchor_set(m, hub ? 0.39f : 0.38835f, hub ? -14.30f : -14.29613f, 118, 59, 0, 170, init_len);
    UgFngObject *g = add(p, UG_OBJ_GROUP, NULL, -39.0f, -21.0f, 1, 1);
    g->name_hash = PRESENTATION_HASH;
    fade_scripts(p, g, 100, hub, -39.26f, -21.0f);                  /* hub: scripts carry x -39.26, stored -39.0 (A4) */
    UgFngObject *glow = add(p, UG_OBJ_IMAGE, NULL, 39.2555f, 6.9964f, 126, 62);
    glow->name_hash = GLOW_HASH; glow->has_parent = 1; glow->parent_guid = g->guid;
    UgFngObject *back = add(p, UG_OBJ_IMAGE, NULL, 39.26f, 6.71f, 126, 62);
    back->name_hash = BACKING_HASH; back->has_parent = 1; back->parent_guid = g->guid;
    UgFngObject *row = add(p, UG_OBJ_GROUP, NULL, hub ? 41.0f : 41.29f, hub ? 7.0f : 7.0817f, 1, 1);
    row->name_hash = ROW_GROUP_HASH; row->has_parent = 1; row->parent_guid = g->guid;
    UgFngObject *strip = add(p, UG_OBJ_IMAGE, NULL, 0, 0, 548, 62);
    strip->name_hash = STRIP_HASH; strip->has_parent = 1; strip->parent_guid = row->guid;
    UgFngObject *title = add(p, UG_OBJ_TEXT, "ICON_TITLE", 41.29f, 41.0f, 0.7f, 0.7f);
    title->has_parent = 1; title->parent_guid = g->guid;
    UgFngObject *head = add(p, UG_OBJ_TEXT, "Header_text", -247.71f, -61.29f, 0.75f, 0.75f);
    head->has_parent = 1; head->parent_guid = g->guid;
    add(p, UG_OBJ_TEXT, "Back_button", -280, 200, 1, 1);
    add(p, UG_OBJ_TEXT, "Select_button", 280, 200, 1, 1);
    UgFngObject *rail = add(p, UG_OBJ_IMAGE, "ScrollBar_Backing", 3, -52, hub ? 564.0f : 562.0f, 8);
    fade_scripts(p, rail, 100, 0, 0, 0);
    UgFngObject *tab = add(p, UG_OBJ_IMAGE, "ScrollBar_Handle", 218.03f, -52.2821f, 16, 16);
    fade_scripts(p, tab, 100, 0, 0, 0);
    UgFngObject *l = add(p, UG_OBJ_IMAGE, "ScrollBar_Arrow_1", -250.2907f, -12.7695f, 26, 26);
    l->rot[0] = 0; l->rot[1] = 0; l->rot[2] = 1; l->rot[3] = 0;     /* 180 degrees about z, about its own centre */
    fade_scripts(p, l, 140, 0, 0, 0);
    script(p, l, hn("Blink"), 400); script(p, l, hn("Static"), 0);   /* an object's scripts are one contiguous slice */
    UgFngObject *r = add(p, UG_OBJ_IMAGE, "ScrollBar_Arrow_2", 259.1396f, -12.7695f, 26, 26);
    fade_scripts(p, r, 140, 0, 0, 0);
    script(p, r, hn("Blink"), 400); script(p, r, hn("Static"), 0);
    for (int k = 1; k <= 10; k++) {
        char nm[32];
        snprintf(nm, sizeof nm, "option_%d", k);
        UgFngObject *o = add(p, UG_OBJ_IMAGE, nm, 0, 334.0f + 24 * (k - 1), 118, 59);
        fade_scripts(p, o, 100, 0, 0, 0);
    }
    handler(p, "Forward", 320, 320);
    finish(p);
}

/* UI_Options / Customize / UI_PauseOptions: masters with the Init chain, rows as build_list_pkg. */
typedef struct ListSpec {
    const char *pkg;
    OmContext ctx;
    int customize;
    int slots, backing;
    float tcx, tcy, tw, th, dcx, dcy, dw, dh;       /* authored (stored = Init) masters */
    float tchain[2], dchain[2];                       /* position after the chain: must not be used */
} ListSpec;
static const ListSpec k_lists[3] = {
    { "UI_Options.fng", OM_CTX_FRONTEND, 0, 8, 1, -151, -120, 240, 36, 126, -122, 216, 32, { -132, -120 }, { 109, -122 } },
    { "UI_PC_Customize_Options.fng", OM_CTX_INGAME, 1, 24, 1, -152, -120, 240, 36, 126, -122, 200, 32, { -133, -120 }, { 109, -122 } },
    { "UI_PauseOptions.fng", OM_CTX_INGAME, 0, 11, 0, -150.2189f, -120.9405f, 240, 36, 124.663f, -122.9405f, 231.18f, 32, { -131.22f, -123.94f }, { 107.66f, -125.94f } },
};
static void build_list_real(Pkg *p, const ListSpec *ls, uint32_t init_len) {
    build_list_pkg(p, ls->slots, ls->backing, 0);
    UgFngObject *tm = &p->obj[0], *dm = &p->obj[1];
    anchor_set(tm, ls->tcx, ls->tcy, ls->tw, ls->th, ls->tchain[0], ls->tchain[1], init_len);
    anchor_set(dm, ls->dcx, ls->dcy, ls->dw, ls->dh, ls->dchain[0], ls->dchain[1], init_len);
}

typedef struct LFx {
    OptionsMenu m;
    StGraphics g;
    StGfxCaps caps;
    StRegStore store;
    OmConfig cfg;
} LFx;
static void lfx_open(LFx *f, OmContext ctx, int customize) {
    memset(f, 0, sizeof *f);
    st_gfx_defaults(&f->g);
    st_gfx_caps_full(&f->caps, 5);
    st_reg_open_memory(&f->store);
    om_config_default(&f->cfg, ctx);
    om_open(&f->m, &f->cfg, &f->g, &f->caps, &f->store, 0);
    if (customize) { om_input(&f->m, OM_IN_DOWN); om_input(&f->m, OM_IN_DOWN); om_input(&f->m, OM_IN_ACCEPT); om_update(&f->m, 1.0f); }
}

/* ---- carousel states (tvg12 cases with "slots") ------------------------------------------------------------- */
static const uint32_t k_icon_keys[4] = { 0x62AA0DF6u, 0x520DE4E3u, 0x84D1D864u, 0x4C9E34E6u };

typedef struct Variant { const char *name; uint32_t init_len; int late_ms; int roundtrip; } Variant;
static const Variant k_variants[] = {
    { "chain runs at the first update", 0, 0, 0 },
    { "chain runs 40 ms in, bound 100 ms after creation", 40, 100, 0 },
    { "serialized and re-parsed, bound 100 ms after creation", 0, 100, 1 },
};

static void rect_in(const UgNode *inner, const UgNode *outer, const char *what) {
    float iw = fabsf(inner->props.w[UG_P_SIZE]) * hypotf(inner->wa, inner->wb), ih = fabsf(inner->props.w[UG_P_SIZE + 1]) * hypotf(inner->wc, inner->wd);
    float ow = fabsf(outer->props.w[UG_P_SIZE]) * hypotf(outer->wa, outer->wb), oh = fabsf(outer->props.w[UG_P_SIZE + 1]) * hypotf(outer->wc, outer->wd);
    CHECK(inner->we - iw / 2 >= outer->we - ow / 2 - 1e-3 && inner->we + iw / 2 <= outer->we + ow / 2 + 1e-3 &&
          inner->wf - ih / 2 >= outer->wf - oh / 2 - 1e-3 && inner->wf + ih / 2 <= outer->wf + oh / 2 + 1e-3,
          "%s: slot rectangle (%.2f, %.2f) %.1fx%.1f lies inside the light strip (%.2f, %.2f) %.0fx%.0f", what,
          inner->we, inner->wf, iw, ih, outer->we, outer->wf, ow, oh);
}

static void run_carousel_case(J *c, const Variant *var) {
    const char *name = jstrv(c, "name"), *pkgname = jstrv(c, "package");
    int hub = pkgname && strstr(pkgname, "PauseOptionsMain") != NULL;
    char label[160];
    snprintf(label, sizeof label, "%s [%s]", name, var->name);
    Fake f;
    fake_init(&f, k_icon_keys, 4);
    Pkg *p = (Pkg *)calloc(1, sizeof *p);
    build_carousel_real(p, hub, var->init_len);
    UgFngPackage parsed;
    const UgFngPackage *use = &p->pk;
    uint8_t *bin = NULL;
    if (var->roundtrip) {
        size_t len = 0;
        char err[160] = "";
        bin = synth_fng(&p->pk, pkgname, &len);
        CHECK(ug_fng_parse(bin, len, &parsed, err, sizeof err), "%s: parse: %s", label, err);
        use = &parsed;
    }
    GvScreen s;
    CHECK(gv_screen_attach(&s, use, &f.a, pkgname), "%s: attach: %s", label, s.status);
    /* the hazard is real in this fixture: once the chain has run, the master evaluates to the wrong place */
    int mi = ug_scene_find_name(&s.scene, hn("OPTION_MASTER"));
    if (var->late_ms) gv_screen_update(&s, (float)var->late_ms);
    CHECK(mi >= 0 && NEAR(s.scene.nodes[mi].wf, 170, 1e-3), "%s: fixture hazard: the evaluated master is at y %.3f after the chain", label, mi >= 0 ? s.scene.nodes[mi].wf : 0);
    GuiCarousel car;
    gc_init(&car, hub ? 564 : 562);
    GvCarousel v;
    CHECK(gv_carousel_bind(&v, &s, &car), "%s: bind: %s", label, s.status);
    J *ea = jget(c, "anchor");
    CHECK(NEAR(car.master_x, jnum_at(ea, 0), 1e-3) && NEAR(car.master_y, jnum_at(ea, 1), 1e-3),
          "%s: cached anchor (%.4f, %.4f), authored (%.4f, %.4f) (A6)", label, car.master_x, car.master_y, jnum_at(ea, 0), jnum_at(ea, 1));
    CHECK(!NEAR(car.master_y, 170, 1.0) && !NEAR(car.master_y, 922.5, 1.0), "%s: the chained position was not used", label);

    J *slots = jget(c, "slots");
    int N = (int)jnum(c, "N", 4), focused = (int)jnum(c, "focused", 1);
    int disabled[GC_MAX] = { 0 };
    for (int i = 0; i < slots->n && i < GC_MAX; i++) disabled[i] = jbool(slots->kid[i], "disabled");
    gc_set_entries(&car, N, disabled);
    if (!strncmp(name, "hub_tween", 9)) { gc_snap_to(&car, 0); gc_step(&car, 1); gc_update(&car, 0.15f); }
    else gc_snap_to(&car, focused - 1);
    CHECK(NEAR(gc_scroll_value(&car), jnum(c, "scroll_offset_s", 0), 1e-3), "%s: scroll offset %.3f", label, gc_scroll_value(&car));
    GvEntry e[GC_MAX];
    for (int i = 0; i < N && i < 4; i++) { e[i].tex = gv_texture(&f.a, k_icon_keys[i]); e[i].label = "x"; }
    gv_screen_update(&s, 400);                                    /* the enter fades are over */
    gv_carousel_apply(&v, &car, e, N, "Pause", 0, 0);
    gv_screen_update(&s, 0);
    gv_carousel_apply(&v, &car, e, N, "Pause", 0, 0);

    const UgScene *sc = &s.scene;
    int strip = ug_scene_find_name(sc, STRIP_HASH), glow = ug_scene_find_name(sc, GLOW_HASH);
    for (int i = 0; i < slots->n; i++) {
        J *sl = slots->kid[i];
        const UgNode *n = &sc->nodes[v.slot[i]];
        J *vc = jget(sl, "virtual_centre");
        int visible = jbool(sl, "visible");
        CHECK(NEAR(n->we, jnum_at(vc, 0), 0.01) && NEAR(n->wf, jnum_at(vc, 1), 0.01),
              "%s: %s centre (%.4f, %.4f), expected (%.4f, %.4f)", label, jstrv(sl, "slot"), n->we, n->wf, jnum_at(vc, 0), jnum_at(vc, 1));
        J *sv = jget(sl, "size_virtual");
        CHECK(NEAR(n->props.w[UG_P_SIZE], jnum_at(sv, 0), 1e-3) && NEAR(n->props.w[UG_P_SIZE + 1], jnum_at(sv, 1), 1e-3),
              "%s: %s size %.2f x %.2f", label, jstrv(sl, "slot"), n->props.w[UG_P_SIZE], n->props.w[UG_P_SIZE + 1]);
        CHECK((!n->app_hidden) == (visible != 0), "%s: %s visible %d, expected %d", label, jstrv(sl, "slot"), !n->app_hidden, visible);
        if (visible) {
            check_px(jstrv(sl, "slot"), n, jget(sl, "expected_px"), tol_centre);
            if (jbool(sl, "inside_panel_row") && strip >= 0) rect_in(n, &sc->nodes[strip], label);
            /* B3: icons are never stretched or skewed: 2:1 like the 128x64 texture, full UV, no rotation */
            CHECK(NEAR(n->props.w[UG_P_SIZE] / n->props.w[UG_P_SIZE + 1], 2.0, 1e-4), "%s: %s keeps the 2:1 aspect", label, jstrv(sl, "slot"));
            CHECK(n->props.w[UG_P_UV0] == 0 && n->props.w[UG_P_UV0 + 1] == 0 && n->props.w[UG_P_UV1] == 1 && n->props.w[UG_P_UV1 + 1] == 1, "%s: %s full UV", label, jstrv(sl, "slot"));
            CHECK(NEAR(n->wb, 0, 1e-6) && NEAR(n->wc, 0, 1e-6) && NEAR(fabsf(n->wa), fabsf(n->wd), 1e-6), "%s: %s is not skewed", label, jstrv(sl, "slot"));
            if (n->tex_override > 0 && i == focused - 1)
                CHECK(!NEAR(n->wf, 170, 1.0), "%s: the focused slot is not at the chained y", label);
        }
    }
    /* the strip of the vector, the glow and the backing coincide with the focused slot (A7) */
    J *ps = jget(c, "panel_row_strip");
    if (ps && ps->t == J_OBJ && strip >= 0) {
        J *pc = jget(ps, "centre"), *pz = jget(ps, "size");
        const UgNode *n = &sc->nodes[strip];
        CHECK(NEAR(n->we, jnum_at(pc, 0), 0.01) && NEAR(n->wf, jnum_at(pc, 1), 0.01) && NEAR(n->props.w[UG_P_SIZE], jnum_at(pz, 0), 1e-3),
              "%s: light strip at (%.4f, %.4f), expected (%.4f, %.4f)", label, n->we, n->wf, jnum_at(pc, 0), jnum_at(pc, 1));
    }
    if (glow >= 0 && !strncmp(name, "pause_explore_rest_c1", 21)) {
        const UgNode *g = &sc->nodes[glow], *sl0 = &sc->nodes[v.slot[0]];
        CHECK(NEAR(g->we, 0.2555, 1e-3) && NEAR(g->wf, -14.0036, 1e-3), "%s: glow at (%.4f, %.4f)", label, g->we, g->wf);
        CHECK(NEAR(g->we, sl0->we, 0.4) && NEAR(g->wf, sl0->wf, 0.4), "%s: glow coincides with the focused slot within 0.4 units", label);
    }
    if (glow >= 0 && !strncmp(name, "hub_rest_c1", 11)) {
        const UgNode *g = &sc->nodes[glow], *sl0 = &sc->nodes[v.slot[0]];
        CHECK(NEAR(g->we, -0.0045, 1e-3) && NEAR(g->wf, -14.0036, 1e-3), "%s: hub glow after the enter fade at (%.4f, %.4f) (A4)", label, g->we, g->wf);
        CHECK(NEAR(g->we, sl0->we, 0.4) && NEAR(g->wf, sl0->wf, 0.4), "%s: hub glow coincides with the focused slot within 0.4 units", label);
    }
    /* A5: everything the application moved is top-level, so its position is its screen position */
    for (int i = 0; i < slots->n; i++) {
        const UgNode *n = &sc->nodes[v.slot[i]];
        CHECK(n->parent < 0 && NEAR(n->props.w[UG_P_POS], n->we, 1e-4) && NEAR(n->props.w[UG_P_POS + 1], n->wf, 1e-4), "%s: slot %d is written in its own (screen) frame", label, i + 1);
    }
    const int moved[3] = { v.tab, v.arrow_l, v.arrow_r };
    for (int i = 0; i < 3; i++) CHECK(sc->nodes[moved[i]].parent < 0, "%s: scroll objects are top-level", label);
    /* the tab: x from the application, y stays the authored value (A7) */
    if (N > 1) {
        const UgNode *tab = &sc->nodes[v.tab];
        CHECK(NEAR(tab->wf, -52.2821, 1e-3), "%s: tab y %.4f untouched", label, tab->wf);
        if (focused == 1 && !strncmp(name, "pause_explore_rest_c1", 21)) CHECK(NEAR(tab->we, -270, 1e-3), "%s: tab x %.3f", label, tab->we);
    }
    gv_screen_free(&s);
    if (var->roundtrip) { ug_fng_free(&parsed); free(bin); }
    free(p);
    fake_free(&f);
}

static void test_carousel_cases(J *cases) {
    int ran = 0;
    for (int vi = 0; vi < (int)(sizeof k_variants / sizeof k_variants[0]); vi++)
        for (int ci = 0; ci < cases->n; ci++)
            if (jget(cases->kid[ci], "slots")) { run_carousel_case(cases->kid[ci], &k_variants[vi]); ran++; }
    CHECK(ran == 6 * (int)(sizeof k_variants / sizeof k_variants[0]), "six carousel cases in %d variants (%d run)", (int)(sizeof k_variants / sizeof k_variants[0]), ran);
}

/* The anchor rule on its own (A6): a late read is never the cached value, and later layout updates do not consult
 * the master again. */
static void test_anchor_cached(J *cases) {
    J *c = case_by_name(cases, "anchor_read_rule");
    CHECK(c != NULL, "anchor_read_rule is in the vector");
    if (!c) return;
    Fake f;
    fake_init(&f, k_icon_keys, 4);
    Pkg *p = (Pkg *)calloc(1, sizeof *p);
    build_carousel_real(p, 0, 0);
    GvScreen s;
    gv_screen_attach(&s, &p->pk, &f.a, "UI_pause.fng");
    GuiCarousel car;
    gc_init(&car, 562);
    GvCarousel v;
    CHECK(gv_carousel_bind(&v, &s, &car), "bind: %s", s.status);
    J *ea = jget(c, "expected_anchor"), *cp = jget(c, "chained_script_0x6048A18B_position");
    CHECK(NEAR(car.master_x, jnum_at(ea, 0), 1e-3) && NEAR(car.master_y, jnum_at(ea, 1), 1e-3), "the cached anchor is the authored value (%.4f, %.4f)", car.master_x, car.master_y);
    CHECK(NEAR(s.scene.nodes[v.master].we, jnum_at(cp, 0), 1e-3) && NEAR(s.scene.nodes[v.master].wf, jnum_at(cp, 1), 1e-3), "(the master object itself did move: %.3f, %.3f)", s.scene.nodes[v.master].we, s.scene.nodes[v.master].wf);
    GvEntry e[4];
    for (int i = 0; i < 4; i++) { e[i].tex = gv_texture(&f.a, k_icon_keys[i]); e[i].label = "x"; }
    gc_set_entries(&car, 4, NULL);
    for (int round = 0; round < 3; round++) {                       /* many updates, the chain keeps running */
        gv_screen_update(&s, 150);
        gv_carousel_apply(&v, &car, e, 4, "Pause", 0, 0);
        const UgNode *n = &s.scene.nodes[v.slot[0]];
        CHECK(NEAR(n->wf, -14.29613, 1e-3) && NEAR(n->we, 0.38835, 1e-3), "round %d: slot 1 stays at the authored anchor (%.4f, %.4f)", round, n->we, n->wf);
    }
    /* the must-not-equal pixels of the vector at every resolution */
    J *mn = jget(c, "must_not_equal_px");
    const UgNode *n0 = &s.scene.nodes[v.slot[0]];
    for (int i = 0; mn && i < mn->n; i++) {
        int W, H;
        sscanf(mn->key[i], "%dx%d", &W, &H);
        GlPxMap m = gl_px_map(W, H);
        GlPt px = gl_px_point(&m, n0->we, n0->wf);
        J *cc = jget(mn->kid[i], "centre");
        CHECK(!(NEAR(px.x, jnum_at(cc, 0), tol_centre) && NEAR(px.y, jnum_at(cc, 1), tol_centre)), "the focused slot is not at the forbidden pixel (%.1f, %.1f) @%s", jnum_at(cc, 0), jnum_at(cc, 1), mn->key[i]);
        CHECK(px.y < jnum_at(cc, 1) - 100, "the focused slot is far above the forbidden row @%s (y %.1f)", mn->key[i], px.y);
    }
    gv_screen_free(&s);
    free(p);
    fake_free(&f);
}

/* ---- list screens: masters read before their chain ran (A6, A8) -------------------------------------------- */
static void test_list_cases(J *cases) {
    J *c = case_by_name(cases, "list_masters_pre_chain_UI_Options_row1");
    CHECK(c != NULL, "list case in the vector");
    static const uint32_t keys[] = { 0x9943D584u };
    for (int vi = 0; vi < 3; vi++) {
        const Variant *var = &k_variants[vi];
        for (int li = 0; li < 3; li++) {
            const ListSpec *ls = &k_lists[li];
            char label[200];
            snprintf(label, sizeof label, "%s [%s]", ls->pkg, var->name);
            Fake f;
            fake_init(&f, keys, 1);
            Pkg *p = (Pkg *)calloc(1, sizeof *p);
            build_list_real(p, ls, var->init_len);
            UgFngPackage parsed;
            const UgFngPackage *use = &p->pk;
            uint8_t *bin = NULL;
            if (var->roundtrip) {
                size_t len = 0;
                char err[160] = "";
                bin = synth_fng(&p->pk, ls->pkg, &len);
                CHECK(ug_fng_parse(bin, len, &parsed, err, sizeof err), "%s: parse: %s", label, err);
                use = &parsed;
            }
            GvScreen s;
            CHECK(gv_screen_attach(&s, use, &f.a, ls->pkg), "%s: attach", label);
            if (var->late_ms) gv_screen_update(&s, (float)var->late_ms);
            int tmi = ug_scene_find_name(&s.scene, hn("TITLE_MASTER")), dmi = ug_scene_find_name(&s.scene, hn("DATA_MASTER"));
            CHECK(NEAR(s.scene.nodes[tmi].we, ls->tchain[0], 1e-3) && NEAR(s.scene.nodes[dmi].we, ls->dchain[0], 1e-3),
                  "%s: fixture hazard: the evaluated masters sit at x %.2f / %.2f after the chain", label, s.scene.nodes[tmi].we, s.scene.nodes[dmi].we);
            LFx x;
            lfx_open(&x, ls->ctx, ls->customize);
            GvList v;
            CHECK(gv_list_bind(&v, &s, &x.m), "%s: bind: %s", label, s.status);
            float tx = ls->tcx - ls->tw / 2, ty = ls->tcy - ls->th / 2, dx = ls->dcx - ls->dw / 2, dy = ls->dcy - ls->dh / 2;
            CHECK(NEAR(v.anchors.tx, tx, 1e-3) && NEAR(v.anchors.ty, ty, 1e-3) && NEAR(v.anchors.dx, dx, 1e-3) && NEAR(v.anchors.dy, dy, 1e-3) &&
                  NEAR(v.anchors.tw, ls->tw, 1e-3) && NEAR(v.anchors.dw, ls->dw, 1e-3) && NEAR(v.anchors.th, ls->th, 1e-3) && NEAR(v.anchors.dh, ls->dh, 1e-3),
                  "%s: anchors (%.3f, %.3f) / (%.3f, %.3f), authored (%.3f, %.3f) / (%.3f, %.3f)", label, v.anchors.tx, v.anchors.ty, v.anchors.dx, v.anchors.dy, tx, ty, dx, dy);
            gv_screen_update(&s, 400);
            gv_list_sync(&v, &x.m, 0);
            const UgNode *la = &s.scene.nodes[v.row[1].left], *ra = &s.scene.nodes[v.row[1].right], *bg = &s.scene.nodes[v.row[1].backing >= 0 ? v.row[1].backing : v.row[1].left];
            float row1_y = dy + 12;
            CHECK(NEAR(la->we, dx - 12, 1e-3) && NEAR(la->wf, row1_y, 1e-3) && NEAR(ra->we, dx + ls->dw + 12, 1e-3) && NEAR(ra->wf, row1_y, 1e-3),
                  "%s: row-1 arrows at (%.3f, %.3f) / (%.3f, %.3f), expected x %.3f / %.3f", label, la->we, la->wf, ra->we, ra->wf, dx - 12, dx + ls->dw + 12);
            CHECK(la->parent < 0 && ra->parent < 0 && NEAR(la->props.w[UG_P_POS], la->we, 1e-4) && NEAR(la->props.w[UG_P_POS + 1], la->wf, 1e-4), "%s: arrows are written in their own frame", label);
            if (ls->backing) CHECK(NEAR(bg->we, tx + 276, 1e-3) && NEAR(bg->wf, ty + 10, 1e-3), "%s: row-1 backing origin (%.3f, %.3f)", label, bg->we, bg->wf);
            const UgNode *nm = &s.scene.nodes[v.row[1].name];
            CHECK(nm->parent < 0 && NEAR(nm->we, tx, 0.01), "%s: row-1 name x %.3f", label, nm->we);
            if (li == 0 && c) {
                CHECK(!NEAR(la->we, -11, 1.0) && !NEAR(ra->we, 229, 1.0), "%s: not the chained DATA_MASTER (-11 / 229)", label);
                J *l = jget(c, "row1_left_arrow"), *r = jget(c, "row1_right_arrow");
                check_px("row1_left_arrow", la, jget(l, "expected_px"), tol_centre);
                check_px("row1_right_arrow", ra, jget(r, "expected_px"), tol_centre);
                J *rl = jget(l, "retail_crosscheck_virtual"), *rr = jget(r, "retail_crosscheck_virtual");
                GlPxMap m = gl_px_map(1920, 1080);
                CHECK(NEAR(la->we * m.s, jnum_at(rl, 0) * m.s, 0.45 * m.s) && NEAR(ra->we * m.s, jnum_at(rr, 0) * m.s, 0.45 * m.s), "%s: retail arrows within 0.45 units", label);
                J *org = jget(c, "row1_backing_group_origin");
                CHECK(NEAR(bg->we, jnum_at(org, 0), 1e-3) && NEAR(bg->wf, jnum_at(org, 1), 1e-3), "%s: backing group origin (5, -128)", label);
                /* A8: the backing's children keep their local offsets: box at origin + (-17.7, 1) */
            }
            gv_screen_free(&s);
            st_reg_close(&x.store);
            if (var->roundtrip) { ug_fng_free(&parsed); free(bin); }
            free(p);
            fake_free(&f);
        }
    }
}

/* A8: the backing group's children are drawn at origin + local offset. */
static void test_backing_children(void) {
    Pkg *p = (Pkg *)calloc(1, sizeof *p);
    UgFngObject *g = add(p, UG_OBJ_GROUP, "OPTION_BACKING_1", 0, 0, 1, 1);
    UgFngObject *box = add(p, UG_OBJ_IMAGE, NULL, -17.7f, 1.0f, 500.81f, 36);
    box->has_parent = 1; box->parent_guid = g->guid;
    UgFngObject *cap1 = add(p, UG_OBJ_IMAGE, NULL, -268.3f, 1.0f, 36, 36);
    cap1->has_parent = 1; cap1->parent_guid = g->guid;
    UgFngObject *cap2 = add(p, UG_OBJ_IMAGE, NULL, 260.7f, 1.0f, 36, 36);
    cap2->has_parent = 1; cap2->parent_guid = g->guid;
    finish(p);
    UgScene s;
    ug_scene_init(&s, &p->pk, NULL);
    float v[2] = { 5, -128 };
    ug_scene_app_write(&s, 0, UG_P_POS, 2, v);                      /* the application sets the group origin only */
    ug_scene_relayout(&s);
    CHECK(NEAR(s.nodes[1].we, -12.7, 1e-3) && NEAR(s.nodes[1].wf, -127, 1e-3), "box at (%.3f, %.3f)", s.nodes[1].we, s.nodes[1].wf);
    CHECK(NEAR(s.nodes[2].we, -263.3, 1e-3) && NEAR(s.nodes[3].we, 265.7, 1e-3), "caps follow the origin: %.2f / %.2f", s.nodes[2].we, s.nodes[3].we);
    ug_scene_free(&s);
    free(p);
}

/* ---- dialogs (A8, B1) ---------------------------------------------------------------------------------------- */
static void build_dialog_real(Pkg *p, float b1x, float b2x) {
    UgFngObject *panel = add(p, UG_OBJ_GROUP, "DIALOG_PANEL", -43, -124, 1, 1);
    UgFngObject *dim = add(p, UG_OBJ_IMAGE, NULL, 43, 124, 1070, 650);
    dim->has_parent = 1; dim->parent_guid = panel->guid;
    dim->color.a = 128;
    add(p, UG_OBJ_TEXT, "message_blurb", 0, -28, 0.7f, 0.7f);
    float bx[3] = { b1x, b2x, 0 };
    const char *bn[3] = { "button1", "Button2", "button3" };
    for (int i = 0; i < 3; i++) {
        UgFngObject *b = add(p, UG_OBJ_GROUP, bn[i], bx[i], 74, 1, 1);
        b->flags = UG_OF_BUTTON;
        b->color.a = 80;                                               /* UnHighlight / enter-fade end: 255 - 175 */
        UgScript *hl = script(p, b, hn("Highlight"), 100);
        const float cb[4] = { 255, 255, 255, 225 };
        track(p, hl, UG_VT_COLOR, UG_P_COLOR, cb, 0, NULL, NULL);
        respond(p, b, UG_MSG_FOCUS_GAINED, UG_CMD_START_SCRIPT, hn("Highlight"));
        UgScript *un = script(p, b, hn("UnHighlight"), 100);
        const float ub[4] = { 255, 255, 255, 80 };
        track(p, un, UG_VT_COLOR, UG_P_COLOR, ub, 0, NULL, NULL);
        respond(p, b, UG_MSG_FOCUS_LOST, UG_CMD_START_SCRIPT, hn("UnHighlight"));
        UgFngObject *box = add(p, UG_OBJ_IMAGE, NULL, 0, 0, 214, 42);
        box->has_parent = 1; box->parent_guid = b->guid;
        box->color.a = 200;
        UgFngObject *cap = add(p, UG_OBJ_IMAGE, NULL, 128, 0, 42, 42);
        cap->has_parent = 1; cap->parent_guid = b->guid;
    }
    UgFngObject *l1 = add(p, UG_OBJ_TEXT, NULL, -158, 63, 1, 1); l1->name_hash = 0xDA473D2Fu;
    UgFngObject *l2 = add(p, UG_OBJ_TEXT, NULL, 140, 63, 1, 1); l2->name_hash = 0xDA473D30u;
    UgFngObject *l3 = add(p, UG_OBJ_TEXT, NULL, 0, 63, 1, 1); l3->name_hash = 0xDA473D31u;
    handler(p, "FORWARD", 140, 140);
    finish(p);
}

static void test_dialogs(J *cases) {
    for (int k = 0; k < 2; k++) {
        const char *pkg = k ? "GenericDialog_SMALL.fng" : "UI_InGameDialog.fng";
        Fake f;
        fake_init(&f, NULL, 0);
        Pkg *p = (Pkg *)calloc(1, sizeof *p);
        build_dialog_real(p, k ? -150.0f : -151.0f, k ? 150.0f : 151.0f);
        GuiDialog d;
        GdSpec spec;
        GdContext g = { 1, 0, 0, 0 };
        gd_spec(&spec, k ? GD_DEFAULTS : GD_QUIT, k ? NULL : &g);
        gd_open(&d, &spec);
        GvDialog v;
        CHECK(gv_dialog_attach(&v, &p->pk, &f.a, &d), "%s: attach: %s", pkg, v.scr.status);
        gv_dialog_sync(&v, &d, 200);
        const UgScene *sc = &v.scr.scene;
        const char *names[3] = { "dialog_button1", "dialog_button2", "dialog_button2_right_cap" };
        if (k) names[1] = "small_dialog_button2";
        /* the button boxes: first child of button1 / Button2 */
        int box1 = -1, box2 = -1, cap2 = -1, dimq = -1;
        for (int i = 0; i < sc->n; i++) {
            const UgNode *n = &sc->nodes[i];
            if (n->parent < 0) continue;
            const UgNode *par = &sc->nodes[n->parent];
            if (par->obj->name_hash == hn("button1") && n->obj->size[0] == 214) box1 = i;
            if (par->obj->name_hash == hn("Button2") && n->obj->size[0] == 214) box2 = i;
            if (par->obj->name_hash == hn("Button2") && n->obj->size[0] == 42) cap2 = i;
            if (n->obj->size[0] == 1070) dimq = i;
        }
        CHECK(box1 >= 0 && box2 >= 0 && cap2 >= 0 && dimq >= 0, "%s: dialog parts found", pkg);
        if (box1 < 0 || box2 < 0 || cap2 < 0 || dimq < 0) { free(p); fake_free(&f); continue; }
        if (!k) {
            check_px("dialog_button1", &sc->nodes[box1], jget(case_by_name(cases, names[0]), "expected_px"), tol_centre);
            check_px("dialog_button2_right_cap", &sc->nodes[cap2], jget(case_by_name(cases, names[2]), "expected_px"), tol_centre);
            check_px("dialog_dim_quad", &sc->nodes[dimq], jget(case_by_name(cases, "dialog_dim_quad"), "expected_px"), tol_centre);
        }
        check_px(names[1], &sc->nodes[box2], jget(case_by_name(cases, names[1]), "expected_px"), tol_centre);
        /* the application never moves the dialog's own objects (A8): the buttons stay top-level at their authored centres */
        CHECK(sc->nodes[v.b1].parent < 0 && sc->nodes[v.b2].parent < 0, "%s: button groups are top-level", pkg);
        /* B1: focus on the second button; its group alpha is the Highlight value, the first stays at 80 */
        CHECK(sc->focus == v.b2, "%s: initial focus on the second button", pkg);
        int a1 = sc->nodes[box1].wcolor.a, a2 = sc->nodes[box2].wcolor.a;
        CHECK(a2 > a1 && NEAR(a1, 63, 1) && NEAR(a2, 176, 1), "%s: focused (second) box alpha %d is the dark one, the first %d the light one (B1)", pkg, a2, a1);
        gv_dialog_close(&v);
        free(p);
        fake_free(&f);
    }
}

/* ---- B2: the pause screen while a confirmation dialog is open --------------------------------------------- */
static void test_dialog_over_pause(void) {
    Fake f;
    fake_init(&f, k_icon_keys, 4);
    Pkg *p = (Pkg *)calloc(1, sizeof *p);
    build_carousel_real(p, 0, 0);
    GvScreen s;
    gv_screen_attach(&s, &p->pk, &f.a, "UI_pause.fng");
    GuiCarousel car;
    gc_init(&car, 562);
    GvCarousel v;
    CHECK(gv_carousel_bind(&v, &s, &car), "bind: %s", s.status);
    gc_set_entries(&car, 4, NULL);
    GvEntry e[4];
    for (int i = 0; i < 4; i++) { e[i].tex = gv_texture(&f.a, k_icon_keys[i]); e[i].label = "Quit to Main Menu"; }
    gv_screen_update(&s, 400);
    gv_carousel_apply(&v, &car, e, 4, "Pause", 0, 0);
    UgScene *sc = &s.scene;
    int glow = ug_scene_find_name(sc, GLOW_HASH);
    CHECK(sc->nodes[glow].wcolor.a == 255 && sc->nodes[v.slot[0]].wcolor.a == 255 && sc->nodes[v.title].wcolor.a == 255, "the screen is fully shown before the dialog");
    uint32_t forward = hn("Forward");
    gv_screen_dialog(&s, 1);                                       /* "start dim", then the screen's own exit script */
    CHECK(ug_scene_script_id(sc, s.handler) == forward, "the exit script replaced DIM on the handler (B2)");
    CHECK(!s.exit_started, "this is not the screen's real exit: it can still be left afterwards");
    gv_screen_update(&s, 50);
    CHECK(sc->nodes[glow].wcolor.a > 0 && sc->nodes[glow].wcolor.a < 255, "half-way through the 100 ms fade (alpha %d)", sc->nodes[glow].wcolor.a);
    float t_before = sc->nodes[s.handler].pl.t;
    gv_screen_dialog(&s, 1);                                       /* already running: not restarted */
    CHECK(NEAR(sc->nodes[s.handler].pl.t, t_before, 1e-3), "an exit script that is already running is not restarted");
    gv_screen_update(&s, 60);
    int hidden_ok = sc->nodes[glow].wcolor.a == 0 && sc->nodes[v.title].wcolor.a == 0 && sc->nodes[v.slot[0]].wcolor.a == 0 &&
                    sc->nodes[v.slot[1]].wcolor.a == 0 && sc->nodes[v.rail].wcolor.a == 0 && sc->nodes[v.tab].wcolor.a == 0;
    CHECK(hidden_ok, "after 110 ms the caption, glow, icons, rail and tab are at alpha 0 (glow %d, caption %d, slot %d)",
          sc->nodes[glow].wcolor.a, sc->nodes[v.title].wcolor.a, sc->nodes[v.slot[0]].wcolor.a);
    CHECK(sc->nodes[v.arrow_r].wcolor.a > 0, "the arrows take 140 ms");
    gv_screen_update(&s, 40);
    CHECK(sc->nodes[v.arrow_r].wcolor.a == 0, "arrows at alpha 0 after 150 ms");
    gv_screen_update(&s, 200);
    CHECK(s.exit_done, "the exit-complete message still arrives at 320 ms; the screen stays (no pending action)");
    /* Cancel / Back: "stop dim": UnDim starts at alpha 60 and reaches 255 in 100 ms */
    gv_screen_dialog(&s, 0);
    gv_screen_update(&s, 0);
    CHECK(sc->nodes[glow].wcolor.a == 60 && sc->nodes[v.slot[0]].wcolor.a == 60, "UnDim starts at alpha 60 (glow %d)", sc->nodes[glow].wcolor.a);
    gv_screen_update(&s, 100);
    CHECK(sc->nodes[glow].wcolor.a == 255 && sc->nodes[v.title].wcolor.a == 255 && sc->nodes[v.slot[0]].wcolor.a == 255 && sc->nodes[v.rail].wcolor.a == 255,
          "and 255 after 100 ms (glow %d, caption %d)", sc->nodes[glow].wcolor.a, sc->nodes[v.title].wcolor.a);
    CHECK(sc->nodes[v.arrow_r].wcolor.a == 255 || sc->nodes[v.arrow_r].wcolor.a > 0, "arrows are back");
    CHECK(!strcmp(sc->nodes[v.title].text_override, "Quit to Main Menu"), "activating an entry does not change the caption");
    /* a second dialog works the same way, and a genuine exit is still possible afterwards */
    gv_screen_dialog(&s, 1);
    CHECK(ug_scene_script_id(sc, s.handler) == forward, "second dialog: the exit script runs again");
    gv_screen_dialog(&s, 0);
    gv_screen_exit(&s);
    CHECK(s.exit_started && ug_scene_script_id(sc, s.handler) == forward, "a real exit after the dialogs starts the exit script");
    gv_screen_free(&s);
    free(p);
    fake_free(&f);
}

/* ---- B4: Customize's first focused row ----------------------------------------------------------------------- */
static void test_customize_first_row(void) {
    LFx x;
    lfx_open(&x, OM_CTX_INGAME, 1);
    CHECK(om_screen(&x.m) == OM_SCREEN_CUSTOMIZE, "customize open");
    const OmRow *r = om_row(&x.m, om_focus(&x.m));
    CHECK(r && !strcmp(r->label, "Car Reflection Update Rate") && om_focus(&x.m) == 0, "capability present: the first focused row is Car Reflection Update Rate (%s)", r ? r->label : "-");
    st_reg_close(&x.store);
    memset(&x, 0, sizeof x);
    st_gfx_defaults(&x.g);
    st_gfx_caps_full(&x.caps, 5);
    x.caps.cap[ST_GFX_CAR_ENV_MAP] = 0;
    st_reg_open_memory(&x.store);
    om_config_default(&x.cfg, OM_CTX_INGAME);
    om_open(&x.m, &x.cfg, &x.g, &x.caps, &x.store, 0);
    om_input(&x.m, OM_IN_DOWN); om_input(&x.m, OM_IN_DOWN); om_input(&x.m, OM_IN_ACCEPT); om_update(&x.m, 1.0f);
    const OmRow *r2 = om_screen(&x.m) == OM_SCREEN_CUSTOMIZE ? om_row(&x.m, om_focus(&x.m)) : NULL;
    CHECK(r2 && !strcmp(r2->label, "Car Reflection Detail"), "capability 0: the first focused row is Car Reflection Detail (%s)", r2 ? r2->label : "-");
    st_reg_close(&x.store);
}

int main(int argc, char **argv) {
    if (argc > 1) g_dir = argv[1];
    J *j = jload("tvg12_frames.json");
    if (!j) { printf("gui_frames_test: tvg12 vector unavailable\n"); return 1; }
    tol_centre = jnum(jget(j, "tolerance_px"), "centre", 1.0);
    tol_size = jnum(jget(j, "tolerance_px"), "size", 1.0);
    tol_retail = jnum(jget(j, "tolerance_px"), "retail_crosscheck", 2.5);
    J *cases = jget(j, "cases");
    CHECK(cases && cases->n == 25, "tvg12 has %d cases", cases ? cases->n : 0);
    test_mapping(j);
    test_composition(cases);
    test_anchor_cached(cases);
    test_carousel_cases(cases);
    test_list_cases(cases);
    test_backing_children();
    test_dialogs(cases);
    test_dialog_over_pause();
    test_customize_first_row();
    printf("gui_frames_test: %d checks, %d failed\n", g_checks, g_fail);
    jfree(j);
    return g_fail ? 1 : 0;
}
