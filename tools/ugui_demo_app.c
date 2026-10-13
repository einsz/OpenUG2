/* ugui_demo_app.c -- demo pages and application logic (GL-free).
 * See ugui_demo_app.h. Every page is built from library widgets; the only
 * widgets defined here are the animation gallery and the .fng scene viewer,
 * which demonstrate extending the library through UgWidgetVT. */
#include "ugui_demo_app.h"
#include "ugui_tuning.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define R(cx, cy, w, h) UG_RECT_C((cx), (cy), (w), (h))

/* ------------------------------------------------------------------ */
/* page names and descriptions                                         */
/* ------------------------------------------------------------------ */
static const char *PAGE_NAME[DP_COUNT] = {
    "Original Screens", "Panels & Text", "Option List", "Controls", "Keyboard", "Date Entry", "Table",
    "Car Select", "Dialogs", "Animations", "Package (.fng)", "Hint Glyphs", "PC Controls", "Locked Item"};

static const char *PAGE_DESC[DP_COUNT] = {
    "Browse every screen package found in the game install and view it rendered with the original textures and fonts; enter, exit and dim animations play from the package's own scripts.",
    "Dark and light rounded panels, mirrored corner pieces, text in eight justification modes, wrapped text, localised text from a string table, inline glyph tokens.",
    "Selector, toggle, slider and action rows, a disabled row, a vertical scroller and more rows than fit. Hover a row, click arrows, drag the slider, use the wheel.",
    "Stat bars, category switcher, grid picker, sliders and scrollers, busy indicators, unavailable movie content and original button art.",
    "On-screen keyboard with explicit key links, caps and symbols, and a text field with caret. Physical typing also works.",
    "Month, day and year fields: wheel or arrow badges cycle the numbers, click a field to select it, click Accept to confirm.",
    "Multi-column table with a selected-row marker, vertical scroller and detail activation.",
    "Car carousel with manufacturer box, name badge, filter switcher, stat bars and original-resource UI.",
    "One, two and three button dialogs in small, medium and large sizes; the page beneath dims while a dialog is open. Also the help popup.",
    "Every standard animation of the visual kit, running from real tracks. Click a cell to replay it.",
    "A synthetic .fng screen package parsed and run: scripts, events, messages, focus and the right-click Back mapping.",
    "Choose PC text-only pills, genuine PS2 glyphs or genuine Xbox glyphs from a supplied source.",
    "The PC keyboard-controls page: a dark header band, seven rows at a 36 pitch, name text at scale 0.5 with mouse input and the original PC pill strip.",
    "This entry is locked: Accept plays the error cue and flashes the lock."};

const char *demo_page_name(int i) { return (i >= 0 && i < DP_COUNT) ? PAGE_NAME[i] : "?"; }

/* ------------------------------------------------------------------ */
/* helpers                                                             */
/* ------------------------------------------------------------------ */
static void page_title(UgCtx *c, UgCanvas *cv, const char *t) {
    ug_kit_panel(cv, (UgRect){-304,-228,608,40}, 0, 1);
    ug_kit_title(c, cv, t);
}

static void caption(UgCtx *c, UgCanvas *cv, const char *t, float x, float y) {
    ug_kit_text(c, cv, t, x, y, 0.5f, UG_TEXT_DARK, UG_J_VCENTER);
}

static DemoApp *app_of(UgScreen *s) { return (DemoApp *)s->user; }

/* ------------------------------------------------------------------ */
/* animation gallery widget (extends the library through UgWidgetVT)   */
/* ------------------------------------------------------------------ */
#define GAL_COLS 2
#define GAL_CW 175.0f
#define GAL_CH 36.0f
static UgRect gal_cell(int i) {
    int col = i % GAL_COLS, row = i / GAL_COLS;
    return R(-207.5f + (float)col * GAL_CW, -148.0f + (float)row * 39, GAL_CW - 6, GAL_CH);
}

static void gal_rest(UgAnim *a, int kind) {
    UgColor c = UG_WHITE;
    float sz = 24;
    switch (kind) {
    case UG_ANIM_BLINK: c = UG_RGBA(255, 255, 255, 150); break;
    case UG_ANIM_ROW_FOCUS: case UG_ANIM_ROW_UNFOCUS: c = UG_RGBA(255, 255, 255, 80); break;
    default: break;
    }
    ug_anim_init(a, c, sz, sz);
    if (kind == UG_ANIM_ROW_FOCUS || kind == UG_ANIM_ROW_UNFOCUS) a->rest.w[UG_P_POS + 2] = 100;
    a->props = a->rest;
}

static void gal_play(DemoGallery *g, int k) {
    ug_anim_play(&g->a[k], (UgStdAnim)k, 0);
    if (k == UG_ANIM_ACTIVITY)
        for (int i = 0; i < 3; i++) ug_anim_play(&g->act[i], UG_ANIM_ACTIVITY, i);
    g->idle[k] = 0;
}

static void gal_update(UgWidget *w, UgCtx *c, float dt) {
    (void)c;
    DemoGallery *g = (DemoGallery *)w;
    for (int k = 0; k < GAL_N; k++) {
        ug_anim_update(&g->a[k], dt);
        if (ug_anim_duration((UgStdAnim)k) > 0 && !ug_anim_running(&g->a[k])) {
            g->idle[k] += dt;
            if (g->idle[k] > 700) gal_play(g, k);          /* one-shots replay by themselves */
        } else if (ug_anim_duration((UgStdAnim)k) == 0) {
            g->idle[k] += dt;
            if (g->idle[k] > 1000) {                         /* zero-length states toggle */
                g->idle[k] = 0;
                if (k == UG_ANIM_ROW_FOCUS || k == UG_ANIM_ROW_UNFOCUS) {
                    int other = (g->a[k].has_script && g->a[k].kind == UG_ANIM_ROW_FOCUS) ? UG_ANIM_ROW_UNFOCUS : UG_ANIM_ROW_FOCUS;
                    ug_anim_play(&g->a[k], (UgStdAnim)other, 0);
                } else if (g->a[k].has_script) ug_anim_stop(&g->a[k]);
                else ug_anim_play(&g->a[k], (UgStdAnim)k, 0);
            }
        }
    }
    for (int i = 0; i < 3; i++) ug_anim_update(&g->act[i], dt);
}

static int gal_hit(const DemoGallery *g, float x, float y) {
    for (int i = 0; i < GAL_N; i++) if (ug_rect_hit(gal_cell(i), x, y)) return i;
    (void)g;
    return -1;
}

static int gal_action(UgWidget *w, UgCtx *c, UgAction a) {
    DemoGallery *g = (DemoGallery *)w;
    int nx = g->sel;
    switch (a) {
    case UG_A_LEFT: nx--; break;
    case UG_A_RIGHT: nx++; break;
    case UG_A_UP: nx -= GAL_COLS; break;
    case UG_A_DOWN: nx += GAL_COLS; break;
    case UG_A_ACCEPT: gal_play(g, g->sel); ug_cue(c, UG_CUE_SELECT); return 1;
    default: return 0;
    }
    if (nx < 0 || nx >= GAL_N) return 0;
    g->sel = nx;
    ug_cue(c, UG_CUE_BROWSER);
    return 1;
}

static int gal_pointer(UgWidget *w, UgCtx *c, const UgPtr *p) {
    DemoGallery *g = (DemoGallery *)w;
    int i = gal_hit(g, p->x, p->y);
    if (i < 0) return 0;
    if (p->type == UG_PT_MOVE) { g->sel = i; return 1; }
    if (p->type == UG_PT_DOWN && p->button == 1) { g->sel = i; gal_play(g, i); ug_cue(c, UG_CUE_SELECT); return 1; }
    return 0;
}

static void gal_draw(UgWidget *w, UgCtx *c, UgCanvas *cv) {
    DemoGallery *g=(DemoGallery *)w;
    DemoApp *app=(DemoApp *)g->app;
    ug_kit_panel(cv,(UgRect){-300,-176,370,328},1,1);
    for(int k=0;k<GAL_N;k++) {
        UgRect r=gal_cell(k);
        if(k==g->sel)ug_kit_panel(cv,r,0,1);
        ug_kit_text(c,cv,ug_anim_name((UgStdAnim)k),r.x+8,r.y+r.h/2,0.55f,k==g->sel?UG_ACCENT:UG_TEXT_DARK,UG_J_VCENTER);
    }
    ug_kit_panel(cv,(UgRect){88,-176,212,328},1,1);
    const UgAnim *a=&g->a[g->sel];UgColor col=ug_anim_color(a);
    ug_kit_text(c,cv,g->sel==UG_ANIM_DISABLED?"Disabled sample":"Preview",194,-148,0.7f,UG_TEXT_DARK,UG_J_CENTER|UG_J_VCENTER);
    if(g->sel==UG_ANIM_PILL_FOCUS||g->sel==UG_ANIM_PILL_GLOW) {
        ug_kit_pill_pc(cv,R(194,-55,144,24),col);
        if(g->sel==UG_ANIM_PILL_GLOW)ug_kit_pill_glow(cv,R(194,-55,144,24),col);
    } else if(g->sel==UG_ANIM_BLINK||g->sel==UG_ANIM_ACTIVITY) {
        ug_kit_badge(cv,194,-55,32,1,col,1);
    } else {
        ug_kit_panel(cv,R(194,-55,160,54),0,col.a/255.0f);
        ug_kit_text(c,cv,"Sample",194,-55,0.7f,col,UG_J_CENTER|UG_J_VCENTER);
    }
    ug_kit_text(c,cv,"Select a state to replay",194,24,0.5f,UG_TEXT_DARK,UG_J_CENTER|UG_J_VCENTER);
    if(app->diagnostics) {
        char b[96];snprintf(b,sizeof b,"%d ms  time %.0f",ug_anim_duration((UgStdAnim)g->sel),a->has_script?a->pl.t:0);
        ug_kit_text(c,cv,b,194,76,0.5f,UG_TEXT_DARK,UG_J_CENTER|UG_J_VCENTER);
    }
}

static void gal_hints(UgWidget *w, UgHintSet *h) {
    (void)w;
    ug_hints_add(h, UG_A_ACCEPT, "Replay", 90);
}

static void gal_destroy(UgWidget *w) {
    DemoGallery *g = (DemoGallery *)w;
    for (int k = 0; k < GAL_N; k++) ug_anim_free(&g->a[k]);
    for (int i = 0; i < 3; i++) ug_anim_free(&g->act[i]);
}

static const UgWidgetVT GAL_VT = {gal_update, gal_action, gal_pointer, gal_draw, NULL, gal_hints, gal_destroy};

static void gal_init(DemoGallery *g) {
    memset(g, 0, sizeof *g);
    ug_widget_init(&g->w, &GAL_VT, "gallery", DID_GALLERY, (UgRect){-300, -172, 600, 360});
    for (int i = 0; i < 3; i++) ug_anim_init(&g->act[i], UG_WHITE, 24, 24);
    for (int k = 0; k < GAL_N; k++) { gal_rest(&g->a[k], k); gal_play(g, k); }
}

/* ------------------------------------------------------------------ */
/* .fng scene viewer widget                                            */
/* ------------------------------------------------------------------ */
#define SCN_SCALE 2.0f
#define SCN_OX 110.0f
#define SCN_OY (-40.0f)

static uint8_t *load_hex_file(const char *path, size_t *len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *txt = (char *)malloc((size_t)n + 1);
    if (fread(txt, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(txt); return NULL; }
    txt[n] = 0;
    fclose(f);
    uint8_t *out = (uint8_t *)malloc((size_t)n / 2 + 16);
    size_t cnt = 0;
    char *save = NULL;
    for (char *line = strtok_r(txt, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
        char *ts = NULL;
        char *tok = strtok_r(line, " \t\r", &ts);
        if (!tok) continue;
        while ((tok = strtok_r(NULL, " \t\r", &ts)) != NULL) out[cnt++] = (uint8_t)strtoul(tok, NULL, 16);
    }
    free(txt);
    *len = cnt;
    return out;
}

static void scn_map(float x, float y, float *ox, float *oy) {
    *ox = (x - SCN_OX) / SCN_SCALE;
    *oy = (y - SCN_OY) / SCN_SCALE;
}

static void scn_note(DemoSceneWidget *s, uint32_t msg) {
    if (s->nlog < 6) s->log[s->nlog++] = msg;
    else { memmove(s->log, s->log + 1, sizeof(uint32_t) * 5); s->log[5] = msg; }
    s->total++;
}

static void scn_drain(DemoSceneWidget *s, UgCtx *c) {
    uint32_t m;
    while (ug_scene_poll_app(&s->sc, &m)) {
        scn_note(s, m);
        if (m == UG_MSG_BACK) ug_post(c, UG_EV_BACK, s->w.id, 0, 0);   /* app reaction to Back */
    }
}

static void scn_update(UgWidget *w, UgCtx *c, float dt) {
    DemoSceneWidget *s = (DemoSceneWidget *)w;
    if (!s->loaded) return;
    ug_scene_update(&s->sc, dt);
    scn_drain(s, c);
}

static int scn_action(UgWidget *w, UgCtx *c, UgAction a) {
    DemoSceneWidget *s = (DemoSceneWidget *)w;
    if (!s->loaded) return 0;
    int r = ug_scene_input(&s->sc, a);
    scn_drain(s, c);
    return r;
}

static int scn_pointer(UgWidget *w, UgCtx *c, const UgPtr *p) {
    DemoSceneWidget *s = (DemoSceneWidget *)w;
    if (!s->loaded) return 0;
    UgPtr q = *p;
    scn_map(p->x, p->y, &q.x, &q.y);
    ug_scene_pointer(&s->sc, &q);
    scn_drain(s, c);
    return p->type != UG_PT_MOVE || ug_scene_pick(&s->sc, q.x, q.y) >= 0;
}

static void scn_draw(UgWidget *w, UgCtx *c, UgCanvas *cv) {
    DemoSceneWidget *s=(DemoSceneWidget *)w;
    ug_kit_panel(cv,(UgRect){-300,-176,600,328},1,1);
    ug_kit_text(c,cv,"Interactive example",0,-140,0.7f,UG_TEXT_DARK,UG_J_CENTER|UG_J_VCENTER);
    ug_kit_text(c,cv,s->example_hits?"Button activated":"Select the button",0,20,0.7f,UG_TEXT_DARK,UG_J_CENTER|UG_J_VCENTER);
    /* The synthetic package is separately inspectable; its unmatched resource
       identifiers never stand in for the interactive example's genuine art. */
}

static void scn_hints(UgWidget *w, UgHintSet *h) {
    (void)w;
    ug_hints_add(h, UG_A_ACCEPT, "Press Button", 90);
}

static void scn_destroy(UgWidget *w) {
    DemoSceneWidget *s = (DemoSceneWidget *)w;
    if (s->loaded) { ug_scene_free(&s->sc); ug_fng_free(&s->pk); s->loaded = 0; }
}

static const UgWidgetVT SCN_VT = {scn_update, scn_action, scn_pointer, scn_draw, NULL, scn_hints, scn_destroy};

static void scn_init(DemoSceneWidget *s, const char *dir, const UgStrTab *strings) {
    memset(s, 0, sizeof *s);
    ug_widget_init(&s->w, &SCN_VT, "scene", DID_SCENE, (UgRect){-300, -170, 600, 250});
    char path[300];
    snprintf(path, sizeof path, "%s/minimal_package.hex", dir);
    size_t len = 0;
    uint8_t *bin = load_hex_file(path, &len);
    char err[96] = "";
    if (!bin) {
        snprintf(s->status, sizeof s->status, "Package not found: %s (run with --vectors DIR)", path);
    } else if (!ug_fng_parse(bin, len, &s->pk, err, sizeof err)) {
        snprintf(s->status, sizeof s->status, "Parse failed: %s", err);
    } else if (!ug_scene_init(&s->sc, &s->pk, strings)) {
        snprintf(s->status, sizeof s->status, "Scene init failed");
        ug_fng_free(&s->pk);
    } else {
        s->loaded = 1;
    }
    free(bin);
}

/* ------------------------------------------------------------------ */
/* page backgrounds                                                    */
/* ------------------------------------------------------------------ */
static void bg_panels(UgScreen *s, UgCtx *c, UgCanvas *cv) {
    DemoApp *a=app_of(s);page_title(c,cv,"Panels & Text");
    ug_kit_panel(cv,(UgRect){-300,-176,600,328},1,1);
    ug_kit_panel(cv,(UgRect){-284,-152,120,56},0,1);
    ug_kit_text(c,cv,"Dark panel",-224,-124,0.7f,UG_TEXT_LIGHT,UG_J_CENTER|UG_J_VCENTER);
    ug_kit_panel(cv,(UgRect){-148,-152,120,56},1,1);
    ug_kit_text(c,cv,"Light panel",-88,-124,0.7f,UG_TEXT_DARK,UG_J_CENTER|UG_J_VCENTER);
    ug_kit_corner(cv,32,-136,26,26,UG_ACCENT);ug_kit_corner(cv,62,-136,-26,26,UG_ACCENT);
    caption(c,cv,"Mirroring",16,-106);
    ug_cv_push(cv);ug_cv_translate(cv,138,-136);ug_cv_rotate(cv,0.4f);
    const UgSkin *sk=ug_kit_skin();if(sk&&sk->box_w)ug_cv_image_rect(cv,R(0,0,56,26),sk->box_w,UG_ACCENT);ug_cv_pop(cv);
    caption(c,cv,"Rotation",110,-106);
    ug_cv_gradient(cv,(UgRect){206,-152,78,40},UG_ACCENT,UG_GREEN,UG_TEXT_DARK,UG_TEXT_HINT);
    caption(c,cv,"Gradient",202,-98);
    static const unsigned mode[8]={0,UG_J_CENTER,UG_J_RIGHT,UG_J_VCENTER,UG_J_CENTER|UG_J_VCENTER,UG_J_RIGHT|UG_J_VCENTER,UG_J_BOTTOM,UG_J_RIGHT|UG_J_BOTTOM};
    static const char *name[8]={"Left top","Centre top","Right top","Left middle","Centre middle","Right middle","Left bottom","Right bottom"};
    for(int i=0;i<8;i++) {
        float x=-279+(i%4)*145,y=-76+(i/4)*42;
        ug_kit_box(cv,(UgRect){x,y,136,34},1,1);
        float ax=(mode[i]&UG_J_CENTER)?x+68:(mode[i]&UG_J_RIGHT)?x+130:x+6;
        float ay=(mode[i]&UG_J_VCENTER)?y+17:(mode[i]&UG_J_BOTTOM)?y+32:y+2;
        ug_kit_text(c,cv,name[i],ax,ay,0.5f,UG_TEXT_DARK,mode[i]);
    }
    ug_kit_panel(cv,(UgRect){-284,20,272,100},0,1);
    ug_kit_textw(c,cv,"Wrapped text fits its own panel and breaks at spaces.",-272,36,0.7f,UG_TEXT_LIGHT,0,244/0.7f);
    caption(c,cv,"Wrapping",-284,134);
    caption(c,cv,"Localized text",8,22);
    if(a->diagnostics)caption(c,cv,"Diagnostics: flags / string-table keys",8,134);
    else caption(c,cv,a->controller.family==UG_CONTROLLER_PC?"PC uses text-only pills":a->controller.status,8,134);
}

static void bg_options(UgScreen *s, UgCtx *c, UgCanvas *cv) { (void)s; page_title(c, cv, "Mixed Settings"); }
static void bg_pcctl(UgScreen *s, UgCtx *c, UgCanvas *cv) { (void)s; page_title(c, cv, "Controls"); }

static void bg_controls(UgScreen *s, UgCtx *c, UgCanvas *cv) {
    (void)s;page_title(c,cv,"Control Samples");
    ug_kit_panel(cv,(UgRect){-300,-176,600,328},1,1);
    caption(c,cv,"Category",-282,-100);
    caption(c,cv,"Slider",42,-100);caption(c,cv,"Swatches",-282,-40);
    caption(c,cv,"Scroller",42,16);caption(c,cv,"Actions",-282,94);
    ug_kit_text(c,cv,"Movie unavailable",-272,56,0.5f,UG_TEXT_DARK,UG_J_VCENTER);
    ug_kit_text(c,cv,app_of(s)->last_result[0]?app_of(s)->last_result:"Action ready",-280,168,0.5f,UG_TEXT_HINT,UG_J_VCENTER);
}

static void bg_entry(UgScreen *s, UgCtx *c, UgCanvas *cv) {
    (void)s;
    page_title(c, cv, "On-screen Keyboard");
    ug_kit_panel(cv,(UgRect){-300,-176,600,328},1,1);
    ug_kit_text(c,cv,"Click keys or type",0,128,0.5f,UG_TEXT_DARK,UG_J_CENTER|UG_J_VCENTER);
}

static void bg_date(UgScreen *s, UgCtx *c, UgCanvas *cv) {
    DemoApp *a = app_of(s);
    page_title(c, cv, "Date Entry");
    ug_kit_panel(cv,(UgRect){-300,-176,600,328},1,1);
    if (a->date.picked) {
        char b[64];
        int d = a->date.picked;
        snprintf(b, sizeof b, "Accepted: %04d-%02d-%02d", d / 10000, (d / 100) % 100, d % 100);
        ug_kit_text(c, cv, b, 0, 110, 0.7f, UG_TEXT_DARK, UG_J_CENTER | UG_J_VCENTER);
    }
    ug_kit_text(c, cv, "Select a field to change", 0, 140, 0.5f, UG_TEXT_DARK, UG_J_CENTER | UG_J_VCENTER);
}

static void bg_table(UgScreen *s, UgCtx *c, UgCanvas *cv) { (void)s; page_title(c, cv, "Rankings"); }
static void bg_cars(UgScreen *s, UgCtx *c, UgCanvas *cv) { (void)s;(void)c;ug_kit_panel(cv,(UgRect){-304,-228,608,40},0,1);ug_kit_panel(cv,(UgRect){-300,-178,600,72},0,1);ug_kit_panel(cv,(UgRect){-300,132,600,40},1,0.3f); }

static void bg_dialogs(UgScreen *s, UgCtx *c, UgCanvas *cv) {
    DemoApp *a = app_of(s);
    page_title(c, cv, "Dialogs");
    ug_kit_panel(cv,(UgRect){-300,-176,600,328},1,1);
    char b[128];
    snprintf(b, sizeof b, "Last result: %s", a->last_result[0] ? a->last_result : "(none yet)");
    ug_kit_text(c, cv, b, 0, 100, 0.7f, UG_ACCENT, UG_J_CENTER | UG_J_VCENTER);
    if(a->diagnostics)ug_kit_text(c,cv,"Diagnostics: modal result",0,130,0.5f,UG_TEXT_DARK,UG_J_CENTER|UG_J_VCENTER);
}

static void bg_anims(UgScreen *s, UgCtx *c, UgCanvas *cv) { (void)s; page_title(c, cv, "Standard Animations"); }
static void bg_package(UgScreen *s, UgCtx *c, UgCanvas *cv) {
    DemoApp *a=app_of(s);page_title(c,cv,"Package Example");
    if(a->diagnostics) {
        char b[120];snprintf(b,sizeof b,"Diagnostics: %d objects / %d resources / %d events",a->package.sw.pk.nobj,a->package.sw.pk.nres,a->package.sw.total);
        ug_kit_text(c,cv,b,-280,130,0.5f,UG_TEXT_DARK,UG_J_VCENTER);
    }
}

static void bg_glyphs(UgScreen *s, UgCtx *c, UgCanvas *cv) {
    DemoApp *a=app_of(s);page_title(c,cv,"Controller Icons");
    ug_kit_panel(cv,(UgRect){-300,-176,600,328},1,1);
    UgController *p=&a->controller;
    if(p->family==UG_CONTROLLER_PC) {
        ug_kit_text(c,cv,"PC: original text-only pills",-280,-140,0.7f,UG_TEXT_DARK,UG_J_VCENTER);
        ug_kit_text(c,cv,"Shared font and panels loaded",-280,-105,0.55f,UG_TEXT_DARK,UG_J_VCENTER);
        ug_kit_text(c,cv,"Select a controller family below",-280,-70,0.55f,UG_TEXT_DARK,UG_J_VCENTER);
    } else if(!ug_controller_family_available(p,p->family)) {
        ug_kit_textw(c,cv,p->status,-280,-140,0.7f,UG_TEXT_DARK,0,560/0.7f);
    } else {
        int n=ug_controller_family_icon_count(p->family),loaded=0;
        for(int i=0;i<n;i++)if(ug_controller_family_icon_texture(p,p->family,i))loaded++;
        char status[80];snprintf(status,sizeof status,"%s glyphs available: %d / %d",p->family==UG_CONTROLLER_PS2?"PS2":"Xbox",loaded,n);
        ug_kit_text(c,cv,status,-280,-171,0.5f,UG_TEXT_DARK,UG_J_VCENTER);
        for(int i=0;i<n;i++) {
            float x=-240+(i%5)*120,y=-134+(i/5)*52;
            int tex=ug_controller_family_icon_texture(p,p->family,i);
            if(tex)ug_cv_image_rect(cv,R(x,y,32,32),tex,UG_WHITE);
            static const char *xb[23]={"Black","D-pad up","Start","Left stick","Left stick L/R","White","Right trigger","Left trigger","Back","D-pad","A","B","X","Y","D-pad up/down","D-pad down","D-pad left","Right stick up","Left stick up","Right stick down","D-pad L/R","Right stick","D-pad right"};
            const char *label=p->family==UG_CONTROLLER_XBOX?xb[i]:ug_controller_family_icon_name(p->family,i)+4;
            ug_kit_text(c,cv,label,x,y+24,0.5f,UG_TEXT_DARK,UG_J_CENTER|UG_J_VCENTER);
        }
    }
    ug_kit_text(c,cv,"F2 selects family",0,170,0.5f,UG_TEXT_DARK,UG_J_CENTER|UG_J_VCENTER);
}

static void bg_root(UgScreen *s, UgCtx *c, UgCanvas *cv) {
    DemoApp *a = app_of(s);
    int sel = a->root.icons.sel;
    int page = a->root.icons.items[sel].id;
    ug_kit_panel(cv, (UgRect){-281, -56, 562, 138}, 0, 0.8f);
    ug_kit_text(c, cv, PAGE_NAME[page], -262, -36, 0.85f, UG_ACCENT, UG_J_VCENTER);
    ug_kit_textw(c,cv,a->diagnostics?PAGE_DESC[page]:"Select an example to explore",-262,-8,0.7f,UG_TEXT_LIGHT,0,520/0.7f);
    ug_kit_text(c, cv, "Choose an example", 0, 105, 0.5f,
                UG_TEXT_HINT, UG_J_CENTER | UG_J_VCENTER);
}

/* ------------------------------------------------------------------ */
/* init                                                                */
/* ------------------------------------------------------------------ */
static void screen_setup(DemoApp *a, UgScreen *s, const char *title, unsigned flags,
                         void (*bg)(UgScreen *, UgCtx *, UgCanvas *)) {
    ug_screen_init(s, title, flags);
    s->user = a;
    s->draw_bg = bg;
}

static void cue_hook(void *user, UgCue cue) {
    DemoApp *a = (DemoApp *)user;
    snprintf(a->cue_log, sizeof a->cue_log, "cue: %s", ug_cue_name(cue));
    a->cue_age = 0;
}

static void front_hook(UgUi *u, void *user) {
    DemoApp *a = (DemoApp *)user;
    UgCtx *c = &u->ctx;
    if(a->diagnostics)ug_kit_text(c,&u->cv,"Diagnostics (F3)",288,-214,0.5f,UG_TEXT_HINT,UG_J_RIGHT);
    if (a->diagnostics && a->cue_age < 1500 && a->cue_log[0]) {
        float al = 1.0f - a->cue_age / 1500.0f;
        ug_kit_text(c, &u->cv, a->cue_log, 312, -228, 0.4f, ug_color_alpha(UG_TEXT_HINT, al), UG_J_RIGHT | UG_J_VCENTER);
    }
}

static void back_hook(UgUi *u, void *user) {
    (void)user;
    UgCanvas *cv = &u->cv;
    (void)cv; /* GL clear supplies neutral background; no invented artwork. */
}

static const UgCar CARS[8] = {
    {"Nissan", "350Z", {0.70f, 0.80f, 0.85f}, 0, {70, 130, 220, 255}},
    {"Mitsubishi", "Lancer Evo", {0.80f, 0.70f, 0.90f}, 0, {220, 60, 50, 255}},
    {"Subaru", "Impreza", {0.75f, 0.70f, 0.80f}, 0, {60, 90, 200, 255}},
    {"Dodge", "Charger", {0.85f, 0.75f, 0.45f}, 1, {240, 150, 30, 255}},
    {"Ford", "Mustang", {0.80f, 0.78f, 0.50f}, 1, {200, 200, 205, 255}},
    {"Lamborghini", "Gallardo", {0.95f, 0.97f, 0.70f}, 2, {240, 220, 40, 255}},
    {"Porsche", "Carrera", {0.92f, 0.90f, 0.80f}, 2, {120, 200, 120, 255}},
    {"Mazda", "RX-8", {0.65f, 0.72f, 0.88f}, 0, {160, 60, 180, 255}},
};

static void build_options(DemoApp *a) {
    DemoOptions *p = &a->options;
    screen_setup(a, &p->scr, "Options", 0, bg_options);
    ug_options_init(&p->list, DID_OPTIONS);
    UgOptionList *o = &p->list;
    const char *diff[] = {"Easy", "Normal", "Hard"};
    const char *onoff[] = {"Off", "On"};
    const char *lang[] = {"English", "Francais", "Deutsch", "Espanol", "Italiano"};
    const char *bar[] = {"PC", "PS2", "Xbox"};
    const char *res[] = {"640x480", "800x600", "1024x768", "1280x960"};
    int r;
    r = ug_options_add(o, "Difficulty", UG_ROW_SELECTOR, DID_ROW_DIFFICULTY, diff, 3); o->rows[r].sel = 1;
    r = ug_options_add(o, "Music Volume", UG_ROW_SLIDER, DID_ROW_MUSIC, NULL, 0); o->rows[r].slider = 0.7f;
    r = ug_options_add(o, "Effects Volume", UG_ROW_SLIDER, DID_ROW_SFX, NULL, 0); o->rows[r].slider = 0.4f;
    r = ug_options_add(o, "Subtitles", UG_ROW_TOGGLE, DID_ROW_SUBTITLES, onoff, 2); o->rows[r].sel = 1;
    ug_options_add(o, "Language", UG_ROW_SELECTOR, DID_ROW_LANGUAGE, lang, 5);
    ug_options_add(o, "Controller Family", UG_ROW_SELECTOR, DID_ROW_HINTBAR, bar, 3);
    r = ug_options_add(o, "Controller Layout (n/a)", UG_ROW_SELECTOR, DID_ROW_LOCKED, diff, 3);
    ug_options_set_disabled(o, r, 1);
    ug_options_add(o, "Invert Mouse Y", UG_ROW_TOGGLE, DID_ROW_INVERT, onoff, 2);
    ug_options_add(o, "Resolution", UG_ROW_SELECTOR, DID_ROW_RES, res, 4);
    ug_options_add(o, "Reset To Defaults", UG_ROW_ACTION, DID_ROW_RESET, NULL, 0);
    ug_options_add(o, "Credits", UG_ROW_ACTION, DID_ROW_CREDITS, NULL, 0);
    ug_options_add(o, "Apply Changes", UG_ROW_ACTION, DID_ROW_APPLY, NULL, 0);
    ug_options_add(o, "Delete Profile", UG_ROW_ACTION, DID_ROW_DELETE, NULL, 0);
    ug_options_add(o, "Extra Row", UG_ROW_TOGGLE, DID_ROW_EXTRA, onoff, 2);
    ug_screen_add(&p->scr, &o->w);
}

static void build_pcctl(DemoApp *a) {
    DemoPcControls *p = &a->pcctl;
    screen_setup(a, &p->scr, "PC Controls", 0, bg_pcctl);
    ug_options_init(&p->list, DID_OPTIONS + 1);
    ug_options_set_style(&p->list, UG_OL_CONTROLS);
    snprintf(p->list.caption, sizeof p->list.caption, "Keyboard");
    snprintf(p->list.value_header,sizeof p->list.value_header,"Binding");
    static const char *const KEYS[] = {"Up Arrow", "W", "Num 8"};
    static const char *const ACT[] = {"Accelerate", "Brake / Reverse", "Steer Left", "Steer Right", "Handbrake", "Nitrous",
                                      "Look Back", "Change Camera", "Reset Car", "Pause", "Shift Up", "Shift Down"};
    for (int i = 0; i < 12; i++) {
        int r = ug_options_add(&p->list, ACT[i], UG_ROW_SELECTOR, DID_OPTIONS + 40 + i, KEYS, 3);
        p->list.rows[r].sel = i % 3;
    }
    ug_screen_add(&p->scr, &p->list.w);
    /* the PC help bar's third pill on this page (SPEC 7.4.1, checklist 18) */
    ug_hints_add(&p->scr.hints, UG_A_BTN1, "Reset Keys To Default", 60);
}

static void build_controls(DemoApp *a) {
    DemoControls *p = &a->controls;
    screen_setup(a, &p->scr, "Controls", 0, bg_controls);
    static const char *const SL[3] = {"Acceleration", "Top Speed", "Handling"};
    ug_statbars_init(&p->stats, DID_STATS, 0, -134, SL, 3);
    float v0[3] = {0.55f, 0.8f, 0.35f};
    ug_statbars_set(&p->stats, v0);
    static const char *const CATS[4] = {"All Parts", "Body Kits", "Spoilers", "Rims"};
    ug_switcher_init(&p->sw, DID_SWITCHER, -170, -76, 220, CATS, 4);
    p->sw.light=1;
    ug_grid_init(&p->grid, DID_GRID, 0, -14, 12, 1, UG_TUNE_SWATCH_PITCH);   /* SPEC 6.13: one row of 12 */
    ug_slider_init(&p->slider, DID_SLIDER, R(160, -76, 220, 32), 0.35f);
    ug_scroller_widget_init(&p->hscroll, DID_SCROLLER, R(140, 44, 160, 8), 0, 12);
    ug_scroller_widget_init(&p->vscroll, DID_SCROLLER + 10, R(275, 48, 8, 54), 1, 9);
    ug_busy_init(&p->busy, DID_BUSY, -210, 52);
    ug_button_init(&p->dlg_btn, DID_BTN_DIALOG, UG_BTN_DIALOG, R(-168, 126, 214, 42), "Dialog Button");
    ug_button_init(&p->pill_btn, DID_BTN_PILL, UG_BTN_HINT, R(130, 112, 144, 24), "Action Button");
    ug_button_init(&p->rand_btn, DID_BTN_RANDOM, UG_BTN_HINT, R(130, 140, 144, 24), "Randomise Stats");
    ug_screen_add(&p->scr, &p->stats.w);
    ug_screen_add(&p->scr, &p->sw.w);
    ug_screen_add(&p->scr, &p->grid.w);
    ug_screen_add(&p->scr, &p->slider.w);
    ug_screen_add(&p->scr, &p->hscroll.w);
    ug_screen_add(&p->scr, &p->vscroll.w);
    ug_screen_add(&p->scr, &p->busy.w);
    ug_screen_add(&p->scr, &p->dlg_btn.w);
    ug_screen_add(&p->scr, &p->pill_btn.w);
    ug_screen_add(&p->scr, &p->rand_btn.w);
}

static void build_pages(DemoApp *a) {
    /* panels page */
    DemoPanels *pp = &a->panels;
    screen_setup(a, &pp->scr, "Panels", 0, bg_panels);
    ug_label_init(&pp->hello, 0, "Hello", 12, 40, 0.7f, UG_TEXT_DARK, 0);
    pp->hello.label = "DEMO_HELLO";
    ug_label_init(&pp->ok, 0, "OK", 12, 88, 0.7f, UG_TEXT_DARK, 0);
    pp->ok.label = "COMMON_OK";
    ug_label_init(&pp->price, 0, "Price", 150, 40, 0.7f, UG_TEXT_DARK, 0);
    pp->price.label = "DEMO_PRICE";
    ug_button_init(&pp->glyph_btn, DID_GLYPHMODE, UG_BTN_HINT, R(160, 92, 200, 24), "Controller Icons");
    ug_screen_add(&pp->scr, &pp->hello.w);
    ug_screen_add(&pp->scr, &pp->ok.w);
    ug_screen_add(&pp->scr, &pp->price.w);
    ug_screen_add(&pp->scr, &pp->glyph_btn.w);

    build_options(a);
    build_pcctl(a);
    build_controls(a);

    /* keyboard page */
    DemoEntry *e = &a->entry;
    screen_setup(a, &e->scr, "Keyboard", 0, bg_entry);
    ug_textfield_init(&e->field, DID_TEXTFIELD, R(0, -142, 400, 40), 20);
    ug_keyboard_init(&e->kb, DID_KEYBOARD, 0, -104, &e->field);
    e->field.w.flags |= UG_W_NOFOCUS;       /* typing always goes to the keyboard's target */
    e->field.w.focused = 1;                 /* ... so the field is always drawn active */
    ug_screen_add(&e->scr, &e->field.w);
    ug_screen_add(&e->scr, &e->kb.w);

    /* date page */
    DemoDate *d = &a->date;
    screen_setup(a, &d->scr, "Date", 0, bg_date);
    ug_date_init(&d->date, DID_DATE, 0, -20, 2004, 11, 8);
    ug_screen_add(&d->scr, &d->date.w);

    /* table page */
    DemoTable *t = &a->table;
    screen_setup(a, &t->scr, "Table", 0, bg_table);
    ug_table_init(&t->table, DID_TABLE);
    ug_table_column(&t->table, "Rank", -266, 0);
    ug_table_column(&t->table, "Player", -206, 0);
    ug_table_column(&t->table, "Car", -20, 0);
    ug_table_column(&t->table, "Time", 244, UG_J_RIGHT);
    static const char *NAMES[] = {"Kaz", "Mira", "Dex", "Juno", "Vale", "Rook", "Sable", "Teo", "Ines", "Orin", "Lux", "Pax"};
    for (int i = 0; i < 24; i++) {
        char r[8], tm[16];
        snprintf(r, sizeof r, "%d", i + 1);
        snprintf(tm, sizeof tm, "%d:%02d.%02d", 1 + i / 12, (37 + i * 7) % 60, (i * 13) % 100);
        ug_table_add_row(&t->table, r, NAMES[i % 12], CARS[i % 8].name, tm);
    }
    ug_screen_add(&t->scr, &t->table.w);

    /* cars page */
    DemoCars *cs = &a->cars;
    screen_setup(a, &cs->scr, "Cars", 0, bg_cars);
    static const char *const FILTERS[4] = {"All", "Tuner", "Muscle", "Exotic"};
    ug_carselect_init(&cs->cars, DID_CARS, CARS, 8, FILTERS, 4);
    ug_screen_add(&cs->scr, &cs->cars.w);

    /* dialogs page */
    DemoDialogs *dg = &a->dialogs;
    screen_setup(a, &dg->scr, "Dialogs", 0, bg_dialogs);
    static const char *LBL[6] = {"One Button (S)", "Two Buttons (M)", "Three Buttons (L)",
                                 "Two Buttons (S)", "Large With Icon", "Help Popup"};
    for (int i = 0; i < 6; i++) {
        ug_button_init(&dg->b[i], DID_DLG_BASE + i, UG_BTN_DIALOG,
                       R(i % 2 ? 125 : -125, -120 + (float)(i / 2) * 58, 214, 42), LBL[i]);
        ug_screen_add(&dg->scr, &dg->b[i].w);
    }

    /* animations page */
    screen_setup(a, &a->anims.scr, "Animations", 0, bg_anims);
    gal_init(&a->anims.gal);a->anims.gal.app=a;
    ug_screen_add(&a->anims.scr, &a->anims.gal.w);

    /* glyph page */
    screen_setup(a, &a->glyphs.scr, "Glyphs", 0, bg_glyphs);
    ug_button_init(&a->glyphs.mode_btn, DID_HINTMODE2, UG_BTN_HINT, R(0, 142, 220, 24), "Controller: PC");
    ug_screen_add(&a->glyphs.scr, &a->glyphs.mode_btn.w);

    /* package page */
    screen_setup(a, &a->package.scr, "Package", 0, bg_package);
    scn_init(&a->package.sw, a->vectors_dir, &a->strings);
    a->package.sw.w.flags|=UG_W_NOFOCUS;
    ug_screen_add(&a->package.scr, &a->package.sw.w);
    ug_button_init(&a->package.example,DID_SCENE+1,UG_BTN_DIALOG,R(0,-55,214,42),"Try Button");
    ug_screen_add(&a->package.scr,&a->package.example.w);
}

void demo_init(DemoApp *a, const char *vectors_dir) {
    memset(a, 0, sizeof *a);
    snprintf(a->vectors_dir, sizeof a->vectors_dir, "%s", vectors_dir ? vectors_dir : "cleanroom/spec/test-vectors");
    ug_ui_init(&a->ui);
    ug_controller_init(&a->controller); ug_glyph_set_controller(&a->controller);
    ug_strtab_init(&a->strings);
    {   /* prefer the binary string table of the test vectors (SPEC 8.3); fall back to an in-memory one */
        char path[300];
        size_t len = 0;
        snprintf(path, sizeof path, "%s/string_table.hex", a->vectors_dir);
        uint8_t *bin = load_hex_file(path, &len);
        if (!bin || !ug_strtab_load(&a->strings, bin, len, NULL, 0)) {
            ug_strtab_add(&a->strings, "COMMON_OK", "OK");
            ug_strtab_add(&a->strings, "COMMON_BACK", "Back");
            ug_strtab_add(&a->strings, "DEMO_HELLO", "Hello^World");
            ug_strtab_add(&a->strings, "DEMO_PRICE", "Caf\xC3\xA9 \xE2\x80\x99 99");
        }
        free(bin);
    }
    a->ui.ctx.strings = &a->strings;
    a->ui.ctx.cue_fn = cue_hook;
    a->ui.ctx.cue_user = a;
    a->ui.draw_back = back_hook;
    a->ui.draw_front = front_hook;
    a->ui.hook_user = a;
    snprintf(a->ui.hints.player, sizeof a->ui.hints.player, "DEMO DRIVER");
    snprintf(a->ui.hints.cash, sizeof a->ui.hints.cash, "$ 12,500");
    a->cue_age = 1e9f;

    /* root page: the icon scroller */
    DemoRoot *r = &a->root;
    screen_setup(a, &r->scr, "Main Menu", UG_SCR_ROOT, bg_root);
    ug_icons_init(&r->icons, DID_ICONS, "Widget Demo");
    static const int SYM[DP_COUNT] = {7, 0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 4, 11};
    for (int i = 0; i < DP_COUNT; i++)
        ug_icons_add(&r->icons, PAGE_NAME[i], SYM[i], i == DP_LOCKED, i);
    ug_button_init(&r->hint_toggle, DID_HINTMODE, UG_BTN_HINT, R(-80, 140, 144, 24), "Controller: PC");
    ug_button_init(&r->help_btn, DID_OPEN_HELP, UG_BTN_HINT, R(80, 140, 144, 24), "Open Help Popup");
    ug_screen_add(&r->scr, &r->icons.w);
    ug_screen_add(&r->scr, &r->hint_toggle.w);
    ug_screen_add(&r->scr, &r->help_btn.w);
    build_pages(a);
    orig_init(a);
    demo_select_controller(a,UG_CONTROLLER_PC);
    ug_ui_push(&a->ui, &r->scr);
}

static char g_language[32];
void demo_set_language(const char *name) { snprintf(g_language, sizeof g_language, "%s", name ? name : ""); }

void demo_init_data(DemoApp *a, const char *vectors_dir, const char *data_dir) {
    demo_init(a, vectors_dir);
    char dir[512];
    const char *found = ug_assets_find_dir(data_dir, dir, sizeof dir);
    if (!found) {
        snprintf(a->assets_msg, sizeof a->assets_msg, "No game data directory given or found (tried %s, $UGUI_DATA, ./).",
                 data_dir && *data_dir ? data_dir : "--data");
        return;
    }
    char msg[200];
    if (ug_assets_open(&a->assets, found, g_language[0] ? g_language : NULL, msg, sizeof msg)) {
        a->have_assets = 1;
        snprintf(a->assets_msg, sizeof a->assets_msg, "%s", msg);
        const UgFace *mf = ug_assets_main_face(&a->assets);
        if (mf && mf->tex>0) ug_text_set_face(mf);
        UgSkin sk;
        ug_skin_resolve(&sk, &a->assets); ug_kit_set_skin(&sk);
        a->faithful_ready=mf && mf->tex>0 && sk.box_w && sk.corner_w && sk.box_b && sk.cap;
        if(!a->faithful_ready)snprintf(a->assets_msg,sizeof a->assets_msg,"Faithful demo unavailable: required original UI font atlas or shared UI kit missing");
        orig_assets_changed(a);
        /* the root menu's icons come from the front-end pack's icon families (SPEC 15.9),
         * looked up by stored name prefix; one icon per page */
        static const char *const ICON[DP_COUNT] = {
            "MAIN_ICON_CAREER", "VISUAL_PART_DECALS", "MAIN_ICON_OPTIONS", "OPTIONS_ICON_CONTROLS",
            "OPTIONS_ICON_PLAYER_SET", "CRIB_ICON_SAVE_LOAD", "ONLINE_ICON_RANKINGS", "CRIB_ICON_CAR_SELECT",
            "ONLINE_ICON_NEWS", "TRACK_ICON_DRIFT", "MAIN_ICON_PROFILE_MANAG", "MAIN_ICON_SPLIT_SCREEN",
            "OPTIONS_ICON_CONTROLS", "CRIB_ICON_REWARDS"};
        for (int i = 0; i < a->root.icons.n && i < DP_COUNT; i++) {
            int one[2];
            if (ug_assets_textures_with_prefix(&a->assets, ICON[a->root.icons.items[i].id], one, 2) > 0)
                a->root.icons.items[i].tex = one[0];
        }
    } else {
        snprintf(a->assets_msg, sizeof a->assets_msg, "%s", msg);
        ug_assets_close(&a->assets);
    }
}

void demo_free(DemoApp *a) {
    ug_glyph_set_controller(NULL);
    ug_screen_free(&a->root.scr);
    ug_screen_free(&a->panels.scr);
    ug_screen_free(&a->options.scr);
    ug_screen_free(&a->pcctl.scr);
    ug_screen_free(&a->controls.scr);
    ug_screen_free(&a->entry.scr);
    ug_screen_free(&a->date.scr);
    ug_screen_free(&a->table.scr);
    ug_screen_free(&a->cars.scr);
    ug_screen_free(&a->dialogs.scr);
    ug_screen_free(&a->anims.scr);
    ug_screen_free(&a->package.scr);
    ug_screen_free(&a->glyphs.scr);
    orig_free(a);
    if (a->dlg_live) ug_screen_free(&a->dlg.scr);
    if (a->help_live) ug_screen_free(&a->help.scr);
    ug_strtab_free(&a->strings);
    ug_ui_free(&a->ui);
    if (a->have_assets) {
        ug_text_set_face(NULL);
        ug_kit_set_skin(NULL);
        ug_assets_close(&a->assets);
        a->have_assets = 0;
    }
}

UgScreen *demo_page_screen(DemoApp *a, int i) {
    switch (i) {
    case DP_ORIGINAL: return &a->orig.scr;
    case DP_PANELS: return &a->panels.scr;
    case DP_OPTIONS: return &a->options.scr;
    case DP_PCCTL: return &a->pcctl.scr;
    case DP_CONTROLS: return &a->controls.scr;
    case DP_ENTRY: return &a->entry.scr;
    case DP_DATE: return &a->date.scr;
    case DP_TABLE: return &a->table.scr;
    case DP_CARS: return &a->cars.scr;
    case DP_DIALOGS: return &a->dialogs.scr;
    case DP_ANIMS: return &a->anims.scr;
    case DP_PACKAGE: return &a->package.scr;
    case DP_GLYPHS: return &a->glyphs.scr;
    default: return NULL;
    }
}

/* ------------------------------------------------------------------ */
/* dialogs                                                             */
/* ------------------------------------------------------------------ */
static void open_dialog(DemoApp *a, int kind, int size, const char *title, const char *msg,
                        const char *const *btns, int nbtn, int icon) {
    if (a->dlg_live) ug_screen_free(&a->dlg.scr);
    ug_dialog_init(&a->dlg, DID_DLG_BASE + 10 + kind, size, title, msg, btns, nbtn);
    a->dlg.icon = icon;
    a->dlg_live = 1;
    ug_ui_push(&a->ui, &a->dlg.scr);
}

static void open_help(DemoApp *a) {
    static const char *const LINES[] = {
        "$JOY_EVENT_FENG_PAD_UP$ $ICON_SPACER$  Change Selection",
        "$JOY_EVENT_FENG_PAD_LEFT$ $ICON_SPACER$  Change Value",
        "$JOY_EVENT_FENG_PAD_RIGHT_ALTERNATE$ $ICON_SPACER$  Rotate Car",
        "$JOY_EVENT_FENG_SELECT$ $ICON_SPACER$  Select Highlighted Option",
        "$JOY_EVENT_FENG_CANCEL$ $ICON_SPACER$  Back to Previous Menu",
        "$JOY_EVENT_FENG_LTRIGGER$ $JOY_EVENT_FENG_RTRIGGER$  Change Category",
    };
    if (a->help_live) ug_screen_free(&a->help.scr);
    ug_help_init(&a->help, DID_HELP, LINES, 6);
    a->help_live = 1;
    ug_ui_push(&a->ui, &a->help.scr);
    ug_cue(&a->ui.ctx, UG_CUE_SUMMON_HELP);
}

static void open_dialog_kind(DemoApp *a, int k) {
    static const char *const OK[] = {"OK"};
    static const char *const YN[] = {"Yes", "No"};
    static const char *const SAVE3[] = {"Save", "Discard", "Cancel"};
    static const char *const AC[] = {"Accept", "Decline"};
    switch (k) {
    case 0: open_dialog(a, 0, 0, "Notice", "Your profile has been saved.", OK, 1, -1); break;
    case 1: open_dialog(a, 1, 1, "Confirm", "Do you want to install this part on the selected car?", YN, 2, 3); break;
    case 2: open_dialog(a, 2, 2, "Unsaved Changes", "You have unsaved changes. Save them before leaving this screen, discard them, or stay here?", SAVE3, 3, 8); break;
    case 3: open_dialog(a, 3, 0, "Delete?", "Delete this persona permanently?", YN, 2, -1); break;
    case 4: open_dialog(a, 4, 2, "Challenge", "A rival wants to race you for pink slips. Accept the challenge and put your car on the line?", AC, 2, 7); break;
    default: open_help(a); break;
    }
}

/* ------------------------------------------------------------------ */
/* events                                                              */
/* ------------------------------------------------------------------ */
int demo_open_page(DemoApp *a, int index) {
    UgScreen *s = demo_page_screen(a, index);
    if (!s || ug_ui_busy(&a->ui)) return 0;
    ug_ui_push(&a->ui, s);
    return 1;
}

static void randomise_stats(DemoApp *a) {
    float v[3];
    a->controls.rnd = a->controls.rnd * 1103515245u + 12345u;
    for (int i = 0; i < 3; i++) {
        a->controls.rnd = a->controls.rnd * 1103515245u + 12345u;
        v[i] = (float)((a->controls.rnd >> 8) & 0xFF) / 255.0f;
    }
    ug_statbars_set(&a->controls.stats, v);
    ug_cue(&a->ui.ctx, UG_CUE_STAT_CHANGE);
}

static void hint_mode_labels(DemoApp *a) {
    const char *name=a->controller.family==UG_CONTROLLER_PC?"PC":a->controller.family==UG_CONTROLLER_PS2?"PS2":"Xbox";
    snprintf(a->root.hint_toggle.label,sizeof a->root.hint_toggle.label,"Controller: %s",name);
    snprintf(a->glyphs.mode_btn.label,sizeof a->glyphs.mode_btn.label,"Controller: %s",name);
    a->options.list.rows[5].sel=a->controller.family==UG_CONTROLLER_PC?0:a->controller.family==UG_CONTROLLER_PS2?1:2;
}

UgVisualStatus demo_select_controller(DemoApp *a, UgControllerFamily family) {
    UgVisualStatus status=ug_controller_select(&a->controller,family);
    ug_glyph_set_controller(&a->controller);
    a->ui.hints.mode=family==UG_CONTROLLER_PC?UG_HINTS_PC:UG_HINTS_STRIP;
    a->hint_mode=(UgHintMode)a->ui.hints.mode;
    a->ui.hints.hover=-1;
    ug_anim_stop(&a->ui.hints.hover_pulse);ug_anim_stop(&a->ui.hints.hover_glow);
    hint_mode_labels(a);
    return status;
}

/* Family selection does not change the mouse/keyboard dispatcher. */
void demo_cycle_hint_mode(DemoApp *a) {
    demo_select_controller(a,a->controller.family==UG_CONTROLLER_PC?UG_CONTROLLER_PS2:a->controller.family==UG_CONTROLLER_PS2?UG_CONTROLLER_XBOX:UG_CONTROLLER_PC);
    fprintf(stderr,"controller: %s\n",a->controller.status);

}

static void handle_event(DemoApp *a, const UgUiEvent *e) {
    char b[96];
    switch (e->kind) {
    case UG_EV_ACTIVATE:
        if (e->source == DID_ICONS) { demo_open_page(a, e->a); }
        else if (e->source == DID_BROWSER) orig_open(a, e->a);
        else if (e->source == DID_HINTMODE || e->source == DID_HINTMODE2) {
            demo_cycle_hint_mode(a);
        } else if (e->source == DID_OPEN_HELP) open_help(a);
        else if (e->source == DID_GLYPHMODE) {
            demo_open_page(a,DP_GLYPHS);
        } else if(e->source==DID_SCENE+1) { a->package.sw.example_hits++;scn_note(&a->package.sw,UG_MSG_ACCEPT);snprintf(a->last_result,sizeof a->last_result,"Example activated"); }
        else if(e->source==DID_BTN_PILL)snprintf(a->last_result,sizeof a->last_result,"Action activated");
        else if (e->source == DID_BTN_RANDOM) randomise_stats(a);
        else if (e->source == DID_BTN_DIALOG) open_dialog_kind(a, 1);
        else if (e->source >= DID_DLG_BASE && e->source < DID_DLG_BASE + 6) open_dialog_kind(a, e->source - DID_DLG_BASE);
        else if (e->source == DID_OPTIONS) {
            if (e->a == DID_ROW_DELETE) open_dialog_kind(a, 3);
            else if (e->a == DID_ROW_RESET) open_dialog_kind(a, 1);
            else if (e->a == DID_ROW_CREDITS) open_dialog_kind(a, 0);
            else if (e->a == DID_ROW_APPLY) open_dialog_kind(a, 2);
        } else if (e->source == DID_DATE) {
            a->date.picked = e->a;
        } else if (e->source == DID_CARS) {
            snprintf(a->last_result, sizeof a->last_result, "selected car %d", e->a);
        }
        break;
    case UG_EV_DATE: a->date.picked = e->a; break;
    case UG_EV_QUIT: {              /* PC help bar "Quit Game" pill (SPEC 7.4.1, checklist C3): a SMALL confirmation */
        static const char *const YN[] = {"Yes", "No"};
        if (!ug_ui_busy(&a->ui) && !(a->dlg_live && ug_ui_top(&a->ui) == &a->dlg.scr))
            open_dialog(a, 5, 0, "", "Do you want to quit the game?", YN, 2, -1);
        break;
    }
    case UG_EV_DIALOG:
        if (e->source == DID_QUIT_DIALOG && e->a == 0) a->quit_requested = 1;
        if (e->source != DID_HELP) {
            snprintf(b, sizeof b, "dialog %d -> button %d", e->source - DID_DLG_BASE - 10, e->a);
            if (e->source != DID_QUIT_DIALOG) snprintf(a->last_result, sizeof a->last_result, "%s", b);
        }
        a->dialog_results++;
        break;
    case UG_EV_CHANGE:
        if (e->source == DID_OPTIONS && e->a == DID_ROW_HINTBAR) {
            int selected=a->options.list.rows[5].sel;
            demo_select_controller(a,selected==0?UG_CONTROLLER_PC:selected==1?UG_CONTROLLER_PS2:UG_CONTROLLER_XBOX);
        }
        break;
    case UG_EV_HINT:
        if (e->a == UG_A_HELP) open_help(a);
        else if (e->a == UG_A_BTN1 && ug_ui_top(&a->ui) == &a->pcctl.scr) {      /* Reset Keys To Default */
            for (int i = 0; i < a->pcctl.list.n; i++) a->pcctl.list.rows[i].sel = 0;
            ug_cue(&a->ui.ctx, UG_CUE_STAT_CHANGE);
        }
        break;
    case UG_EV_TEXT_DONE:
        snprintf(a->last_result, sizeof a->last_result, "text '%s'", a->entry.field.text);
        break;
    case UG_EV_CUSTOMIZE:
        open_dialog_kind(a, 0);
        break;
    default: break;
    }
}

void demo_update(DemoApp *a, float dt) {
    a->cue_age += dt;
    ug_ui_update(&a->ui, dt);
    UgUiEvent e;
    while (ug_ui_poll(&a->ui, &e)) handle_event(a, &e);
    orig_post_frame(a);
}

void demo_draw(DemoApp *a) { if(a->faithful_ready) ug_ui_draw(&a->ui); else ug_cv_clear(&a->ui.cv); }
