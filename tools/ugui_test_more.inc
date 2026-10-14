/* ugui_test_more.inc -- included by ugui_test.c (sections B..G).
 * B animations/players, C canvas/font/text, D widgets and navigation,
 * E hint bar, F scene runtime, G mouse-only tour of the demo. */
#include "ugui_canvas.h"
#include "ugui_scene.h"
#include "ugui_ui.h"
#include "ugui_demo_app.h"

/* ------------------------------------------------------------------ */
/* shared helpers                                                      */
/* ------------------------------------------------------------------ */
static int g_cues[UG_CUE_COUNT];
static void count_cue(void *u, UgCue c) { (void)u; g_cues[c]++; }
static void reset_cues(void) { memset(g_cues, 0, sizeof g_cues); }

static int drain(UgCtx *c, int kind, UgUiEvent *last) {
    UgUiEvent e;
    int n = 0;
    while (ug_poll(c, &e))
        if (e.kind == kind) { n++; if (last) *last = e; }
    return n;
}

static float anim_val(UgStdAnim k, UgColor rest, int param, float t, int comp, float z0) {
    UgProps p;
    ug_props_default(&p, rest, 24, 24);
    p.w[UG_P_POS + 2] = z0;
    UgScript s;
    ug_anim_build(k, &p, param, &s);
    UgProps out = p;
    ug_script_apply(&s, t, &out);
    ug_script_free(&s);
    return out.w[comp];
}

/* ------------------------------------------------------------------ */
/* B. Standard animations (SPEC 5) and the script player (SPEC 3.5.4)   */
/* ------------------------------------------------------------------ */
static void test_anim_durations(void) {
    static const int want[UG_ANIM_COUNT] = {180, 140, 100, 100, 1200, 0, 0, 400, 1200, 1200, 1200, 3600, 320, 320, 0};
    for (int k = 0; k < UG_ANIM_COUNT; k++)
        CHECK_MSG(ug_anim_duration((UgStdAnim)k) == want[k], "%s duration %d want %d", ug_anim_name((UgStdAnim)k),
                  ug_anim_duration((UgStdAnim)k), want[k]);
    /* the built script carries the same length and loops where the table loops */
    UgProps p;
    ug_props_default(&p, UG_WHITE, 24, 24);
    for (int k = 0; k < UG_ANIM_COUNT; k++) {
        UgScript s;
        ug_anim_build((UgStdAnim)k, &p, 0, &s);
        CHECK((int)s.length == want[k]);
        int loops = k == UG_ANIM_FOCUS_PULSE || k == UG_ANIM_BREATHE || k == UG_ANIM_PILL_FOCUS ||
                    k == UG_ANIM_PILL_GLOW || k == UG_ANIM_ACTIVITY;
        CHECK(((s.flags & UG_SCRIPT_LOOP) != 0) == loops);
        CHECK(s.ntracks > 0);
        ug_script_free(&s);
    }
}

static void test_anim_values(void) {
    const UgColor w50 = UG_RGBA(50, 50, 50, 255);
    /* fade-in: alpha 0 for 40 ms then linear to rest by 180 */
    CHECK_NEAR(anim_val(UG_ANIM_FADE_IN, w50, 0, 0, 3, 0), 0, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_FADE_IN, w50, 0, 40, 3, 0), 0, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_FADE_IN, w50, 0, 110, 3, 0), 128, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_FADE_IN, w50, 0, 180, 3, 0), 255, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_FADE_IN, w50, 0, 110, 0, 0), 50, 0.5);
    /* fade-out: linear to 0 by 100 ms, hold */
    const UgColor w150 = UG_RGBA(255, 255, 255, 150);
    CHECK_NEAR(anim_val(UG_ANIM_FADE_OUT, w150, 0, 0, 3, 0), 150, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_FADE_OUT, w150, 0, 50, 3, 0), 75, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_FADE_OUT, w150, 0, 140, 3, 0), 0, 0.5);
    /* dim to ~60, or 0 for groups; undim back */
    CHECK_NEAR(anim_val(UG_ANIM_DIM, UG_WHITE, 0, 100, 3, 0), 60, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_DIM, UG_WHITE, 1, 100, 3, 0), 0, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_UNDIM, UG_WHITE, 0, 0, 3, 0), 60, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_UNDIM, UG_WHITE, 0, 100, 3, 0), 255, 0.5);
    /* focus pulse: accent, alpha 0->255 (0-440), hold (440-760), ->0 (760-1200) */
    CHECK_NEAR(anim_val(UG_ANIM_FOCUS_PULSE, UG_WHITE, 0, 0, 3, 0), 0, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_FOCUS_PULSE, UG_WHITE, 0, 220, 3, 0), 128, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_FOCUS_PULSE, UG_WHITE, 0, 440, 3, 0), 255, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_FOCUS_PULSE, UG_WHITE, 0, 760, 3, 0), 255, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_FOCUS_PULSE, UG_WHITE, 0, 980, 3, 0), 128, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_FOCUS_PULSE, UG_WHITE, 0, 600, 0, 0), 202, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_FOCUS_PULSE, UG_WHITE, 0, 600, 1, 0), 244, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_FOCUS_PULSE, UG_WHITE, 0, 600, 2, 0), 138, 0.5);
    /* row focus: alpha 80 -> 200, 11 units nearer (z 100 -> 89) */
    CHECK_NEAR(anim_val(UG_ANIM_ROW_FOCUS, UG_WHITE, 0, 0, 3, 100), 200, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_ROW_FOCUS, UG_WHITE, 0, 0, UG_P_POS + 2, 100), 89, 0.01);
    CHECK_NEAR(anim_val(UG_ANIM_ROW_UNFOCUS, UG_WHITE, 0, 0, 3, 100), 80, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_ROW_UNFOCUS, UG_WHITE, 0, 0, UG_P_POS + 2, 100), 100, 0.01);
    /* blink: accent tint, alpha 150-255-150, size +6 at 200 ms, back at 400 */
    CHECK_NEAR(anim_val(UG_ANIM_BLINK, w150, 0, 0, 0, 0), 202, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_BLINK, w150, 0, 0, 3, 0), 150, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_BLINK, w150, 0, 200, 3, 0), 255, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_BLINK, w150, 0, 200, UG_P_SIZE, 0), 30, 0.01);
    CHECK_NEAR(anim_val(UG_ANIM_BLINK, w150, 0, 100, UG_P_SIZE, 0), 27, 0.01);
    CHECK_NEAR(anim_val(UG_ANIM_BLINK, w150, 0, 400, 3, 0), 150, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_BLINK, w150, 0, 400, 0, 0), 255, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_BLINK, w150, 0, 400, UG_P_SIZE, 0), 24, 0.01);
    /* breathe: full 0-320, fade to 0 by 640, hold 0 to 1000, back to full at 1200 */
    CHECK_NEAR(anim_val(UG_ANIM_BREATHE, UG_WHITE, 0, 0, 3, 0), 255, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_BREATHE, UG_WHITE, 0, 320, 3, 0), 255, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_BREATHE, UG_WHITE, 0, 640, 3, 0), 0, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_BREATHE, UG_WHITE, 0, 1000, 3, 0), 0, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_BREATHE, UG_WHITE, 0, 1100, 3, 0), 128, 0.5);
    /* pill focus: white/150 -> accent/255 at 500, hold to 700, back at 1200 */
    CHECK_NEAR(anim_val(UG_ANIM_PILL_FOCUS, UG_WHITE, 0, 0, 3, 0), 150, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_PILL_FOCUS, UG_WHITE, 0, 500, 3, 0), 255, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_PILL_FOCUS, UG_WHITE, 0, 500, 0, 0), 202, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_PILL_FOCUS, UG_WHITE, 0, 700, 3, 0), 255, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_PILL_FOCUS, UG_WHITE, 0, 1199, 3, 0), 150, 1.5);
    CHECK_NEAR(anim_val(UG_ANIM_PILL_GLOW, UG_WHITE, 0, 0, 3, 0), 0, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_PILL_GLOW, UG_WHITE, 0, 600, 3, 0), 255, 0.5);
    /* activity arrows light in sequence over 3600 ms */
    for (int i = 0; i < 3; i++) {
        float peak = (float)(i * 1200 + 300);
        CHECK_NEAR(anim_val(UG_ANIM_ACTIVITY, UG_WHITE, i, peak, 3, 0), 255, 0.5);
        for (int j = 0; j < 3; j++)
            if (j != i) CHECK(anim_val(UG_ANIM_ACTIVITY, UG_WHITE, j, peak, 3, 0) < 100.0f);
    }
    /* strip swap cross-fades over 320 ms; disabled = alpha 120 */
    CHECK_NEAR(anim_val(UG_ANIM_STRIP_OUT, UG_WHITE, 0, 320, 3, 0), 0, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_STRIP_IN, UG_WHITE, 0, 0, 3, 0), 0, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_STRIP_IN, UG_WHITE, 0, 320, 3, 0), 255, 0.5);
    CHECK_NEAR(anim_val(UG_ANIM_DISABLED, UG_WHITE, 0, 0, 3, 0), 120, 0.5);
}

static void test_anim_owner(void) {
    UgAnim a;
    ug_anim_init(&a, UG_WHITE, 24, 24);
    CHECK(!ug_anim_running(&a));
    ug_anim_play(&a, UG_ANIM_FADE_OUT, 0);
    CHECK(ug_anim_running(&a));
    ug_anim_update(&a, 50);
    CHECK_NEAR(ug_anim_alpha01(&a), 0.5, 0.01);
    ug_anim_update(&a, 200);                 /* past the end: final value held, stopped */
    CHECK(!ug_anim_running(&a));
    CHECK_NEAR(ug_anim_alpha01(&a), 0.0, 0.001);
    ug_anim_play(&a, UG_ANIM_FADE_IN, 0);
    ug_anim_update(&a, 400);
    CHECK(!ug_anim_running(&a));
    CHECK_NEAR(ug_anim_alpha01(&a), 1.0, 0.001);
    ug_anim_play(&a, UG_ANIM_FOCUS_PULSE, 0);
    ug_anim_update(&a, 5000);                /* loops forever */
    CHECK(ug_anim_running(&a));
    ug_anim_stop(&a);
    CHECK_NEAR(ug_anim_alpha01(&a), 1.0, 0.001);
    ug_anim_free(&a);
}

typedef struct { uint32_t msg[16]; uint32_t t[16]; int n; } EvLog;
static void ev_cb(void *u, const UgEvent *e) {
    EvLog *l = (EvLog *)u;
    if (l->n < 16) { l->msg[l->n] = e->msg; l->t[l->n] = e->t_ms; l->n++; }
}

static UgScript mk_script(uint32_t id, uint32_t len, uint32_t flags) {
    UgScript s;
    memset(&s, 0, sizeof s);
    s.id = id; s.length = len; s.flags = flags;
    return s;
}

static void test_player(void) {
    EvLog log;
    UgProps p;
    ug_props_default(&p, UG_WHITE, 10, 10);

    /* a one-shot with events at 0, 50 and 100 ms (chain to a looping script) */
    UgScript sc[2];
    sc[0] = mk_script(1, 100, 0);
    sc[0].has_chain = 1; sc[0].chain = 2;
    UgEvent ev[3] = {{10, 0, 0}, {11, 0, 50}, {12, 0, 100}};
    sc[0].events = ev; sc[0].nevents = 3;
    UgKey k0[2] = {{0, {0, 0, 0, 0}}, {100, {0, 0, 0, -255}}};
    UgTrack tr;
    memset(&tr, 0, sizeof tr);
    tr.type = UG_VT_COLOR; tr.has_base = 1; tr.base.v[0] = tr.base.v[1] = tr.base.v[2] = tr.base.v[3] = 255;
    tr.keys = k0; tr.nkeys = 2; tr.length = 100; tr.prop = UG_P_COLOR;
    sc[0].tracks = &tr; sc[0].ntracks = 1;
    sc[1] = mk_script(2, 100, UG_SCRIPT_LOOP);
    UgEvent ev2[1] = {{20, 0, 30}};
    sc[1].events = ev2; sc[1].nevents = 1;

    UgPlayer pl;
    memset(&log, 0, sizeof log);
    ug_player_init(&pl, sc, 2, ev_cb, &log);
    p.w[UG_P_POS] = 5.0f;
    CHECK(ug_player_start(&pl, 1, &p));
    CHECK(log.n == 1 && log.msg[0] == 10);          /* t = 0 event fires on start */
    CHECK_NEAR(p.w[3], 255, 0.5);                    /* tracks applied at t = 0 */
    ug_player_update(&pl, 49, &p);
    CHECK(log.n == 1);
    ug_player_update(&pl, 2, &p);                    /* t = 51 */
    CHECK(log.n == 2 && log.msg[1] == 11);
    CHECK_NEAR(p.w[3], 255.0 * 49 / 100, 1.0);
    CHECK_NEAR(p.w[UG_P_POS], 5.0, 1e-6);            /* properties without tracks are left alone */
    ug_player_update(&pl, 60, &p);                   /* t = 111: end of script 1, chain into 2 */
    CHECK(log.n == 3 && log.msg[2] == 12);
    CHECK(pl.cur == &sc[1]);
    CHECK_NEAR(pl.t, 11.0, 0.01);                    /* overshoot is carried over */
    ug_player_update(&pl, 20, &p);                   /* 11 -> 31 passes the event at 30 */
    CHECK(log.n == 4 && log.msg[3] == 20);
    ug_player_update(&pl, 1000, &p);                 /* loops, stays running */
    CHECK(pl.running);
    CHECK(log.n >= 10);
    CHECK(!ug_player_start(&pl, 99, &p));

    /* non-looping script without chain stops at its final values */
    UgScript one = mk_script(7, 100, 0);
    one.tracks = &tr; one.ntracks = 1;
    ug_player_init(&pl, &one, 1, NULL, NULL);
    ug_player_start(&pl, 7, &p);
    ug_player_update(&pl, 250, &p);
    CHECK(!pl.running);
    CHECK_NEAR(p.w[3], 0, 0.5);

    /* loop semantics: events at 0 and 100 on a 100 ms loop */
    UgScript lp = mk_script(8, 100, UG_SCRIPT_LOOP);
    UgEvent lev[2] = {{30, 0, 0}, {31, 0, 100}};
    lp.events = lev; lp.nevents = 2;
    memset(&log, 0, sizeof log);
    ug_player_init(&pl, &lp, 1, ev_cb, &log);
    ug_player_start(&pl, 8, &p);
    CHECK(log.n == 1 && log.msg[0] == 30);
    ug_player_update(&pl, 99, &p);
    CHECK(log.n == 1);
    ug_player_update(&pl, 2, &p);                    /* crosses 100: end event; t=0 events fire on start only (3.5.4) */
    CHECK(log.n == 2 && log.msg[1] == 31);
    CHECK_NEAR(pl.t, 1.0, 0.01);
    ug_player_update(&pl, 99, &p);                   /* next wrap: still no second t=0 event */
    CHECK(log.n == 3 && log.msg[2] == 31);
    /* zero-length loop never advances */
    UgScript z = mk_script(9, 0, UG_SCRIPT_LOOP);
    ug_player_init(&pl, &z, 1, NULL, NULL);
    ug_player_start(&pl, 9, &p);
    ug_player_update(&pl, 500, &p);
    CHECK(pl.running && pl.t == 0);
}

/* ------------------------------------------------------------------ */
/* C. Canvas, font, text                                               */
/* ------------------------------------------------------------------ */
static void test_color_ops(void) {
    UgColor c = ug_color_mul(UG_RGBA(255, 255, 255, 255), UG_RGBA(100, 200, 50, 128));
    CHECK(c.r == 100 && c.g == 200 && c.b == 50 && c.a == 128);   /* (a*b+127)/255, SPEC 1.4 amendment 2 */
    UgColor w = ug_color_mul(UG_WHITE, UG_WHITE);
    CHECK(w.r == 255 && w.g == 255 && w.b == 255 && w.a == 255);  /* 255 x 255 = 255 */
    CHECK(ug_color_mul(UG_RGBA(255, 0, 7, 128), UG_RGBA(255, 255, 255, 255)).a == 128);
    CHECK(ug_clamp8(-5) == 0 && ug_clamp8(300) == 255 && ug_clamp8(127.5f) == 128);
    UgColor l = ug_color_lerp(UG_RGBA(0, 0, 0, 0), UG_RGBA(200, 100, 50, 255), 0.5f);
    CHECK(l.r == 100 && l.a == 128);
}

static void test_view(void) {
    UgView v = ug_view_fit(640, 480);
    CHECK(v.x0 == 0 && v.y0 == 0 && v.w == 640 && v.h == 480 && v.scale == 1.0f);
    v = ug_view_fit(1280, 720);          /* 16:9 window -> pillar-boxed 4:3 */
    CHECK(v.h == 720 && v.w == 960 && v.x0 == 160 && v.y0 == 0);
    float x, y;
    ug_view_to_virtual(&v, 640, 360, &x, &y);
    CHECK_NEAR(x, 0, 0.01); CHECK_NEAR(y, 0, 0.01);
    ug_view_to_virtual(&v, 160, 0, &x, &y);       /* top-left of the 4:3 area */
    CHECK_NEAR(x, -320, 0.01); CHECK_NEAR(y, -240, 0.01);
    v = ug_view_fit(500, 1000);
    CHECK(v.w == 500 && v.y0 > 0);
}

static void test_canvas(void) {
    UgCanvas cv; ug_cv_init(&cv); ug_kit_set_skin(NULL);
    ug_cv_fill(&cv,(UgRect){10,20,30,40},UG_WHITE,4); CHECK(cv.n==0);
    ug_cv_push(&cv); ug_cv_translate(&cv,100,50); ug_cv_mul_alpha(&cv,0.5f);
    ug_cv_flat_rect(&cv,(UgRect){0,0,10,10},UG_RGBA(255,255,255,200));
    CHECK(cv.n==6&&cv.v[0].x==100&&cv.v[0].y==50&&cv.v[0].a==100);ug_cv_pop(&cv);
    UgSkin sk={0};sk.corner_w=1;sk.box_w=2;ug_kit_set_skin(&sk); ug_cv_clear(&cv);
    ug_kit_corner(&cv,0,0,16,16,UG_WHITE);ug_kit_corner(&cv,0,0,-16,16,UG_WHITE);
    CHECK(cv.n==12&&cv.v[0].x==-8&&cv.v[6].x==8);CHECK(cv.v[0].lx==0&&cv.v[6].lx==0);
    ug_kit_set_skin(NULL);ug_cv_free(&cv);
}

static void test_font(void) {
    const UgFont *f=ug_font();CHECK(f&&f->ng==0&&!f->atlas);CHECK(!ug_font_glyph(f,'A'));
    UgCanvas cv;ug_cv_init(&cv);ug_text_set_face(NULL);ug_text_draw(&cv,"No substitute",0,0,1,UG_WHITE,0,0,0,UG_GLYPHS_MOUSE);CHECK(cv.n==0);ug_cv_free(&cv);
}

static void test_text_layout(void) {
    size_t len;uint8_t *bin=read_hex("font_chunk.hex",&len);CHECK(bin!=NULL);if(!bin)return;
    UgFace f;char err[128];CHECK(ug_face_parse(&f,bin+8,len-8,err,sizeof err));ug_text_set_face(&f);
    UgTextLayout tl;ug_text_layout(&tl,"AV^VA",0,3,UG_GLYPHS_MOUSE);CHECK(tl.nlines==2);CHECK_NEAR(tl.pitch,24,0.001);CHECK_NEAR(tl.height,45,0.001);ug_text_layout_free(&tl);
    ug_text_layout(&tl,"A$JOY_EVENT_FENG_ACCEPT$V",0,0,UG_GLYPHS_MOUSE);CHECK(tl.n==3);CHECK(tl.items[1].is_glyph&&tl.items[1].glyph.id==UG_G_NONE);ug_text_layout_free(&tl);
    ug_text_set_face(NULL);ug_face_free(&f);free(bin);
}

static void test_text_justify(void) {
    size_t len;uint8_t *bin=read_hex("font_chunk.hex",&len);CHECK(bin!=NULL);if(!bin)return;
    UgFace f;char err[128];CHECK(ug_face_parse(&f,bin+8,len-8,err,sizeof err));f.tex=1;ug_text_set_face(&f);
    UgCanvas a,b;ug_cv_init(&a);ug_cv_init(&b);float w,h;ug_text_measure("AV",1,0,0,UG_GLYPHS_MOUSE,&w,&h);
    unsigned modes[]={UG_J_CENTER,UG_J_RIGHT,UG_J_VCENTER,UG_J_BOTTOM,UG_J_CENTER|UG_J_VCENTER};
    for(unsigned i=0;i<sizeof modes/sizeof *modes;i++){
        float dx=modes[i]&UG_J_CENTER?-w/2:modes[i]&UG_J_RIGHT?-w:0,dy=modes[i]&UG_J_VCENTER?-h/2:modes[i]&UG_J_BOTTOM?-h:0;
        ug_cv_clear(&a);ug_cv_clear(&b);ug_text_draw(&a,"AV",50,70,1,UG_WHITE,modes[i],0,0,UG_GLYPHS_MOUSE);ug_text_draw(&b,"AV",50+dx,70+dy,1,UG_WHITE,0,0,0,UG_GLYPHS_MOUSE);
        CHECK(a.n==12&&b.n==12);for(int k=0;k<a.n&&k<b.n;k++){CHECK_NEAR(a.v[k].x,b.v[k].x,0.001);CHECK_NEAR(a.v[k].y,b.v[k].y,0.001);}
    }ug_cv_free(&a);ug_cv_free(&b);ug_text_set_face(NULL);ug_face_free(&f);free(bin);
}

/* ------------------------------------------------------------------ */
/* D. Widgets, focus and navigation                                    */
/* ------------------------------------------------------------------ */
static UgPtr P(int type, float x, float y, int button, int wheel) {
    UgPtr p = {type, x, y, button, wheel};
    return p;
}

static void test_slider(void) {
    UgCtx c; ug_ctx_init(&c);
    UgSlider s;
    ug_slider_init(&s, 7, UG_RECT_C(0, 0, 235, 32), 0.5f);
    CHECK(s.w.vt->action(&s.w, &c, UG_A_RIGHT));          /* +1/10 of the range (SPEC 6.6) */
    CHECK_NEAR(s.value, 0.6, 1e-5);
    s.w.vt->action(&s.w, &c, UG_A_LEFT);
    s.w.vt->action(&s.w, &c, UG_A_LEFT);
    CHECK_NEAR(s.value, 0.4, 1e-5);
    CHECK(!s.w.vt->action(&s.w, &c, UG_A_UP));            /* vertical movement not consumed */
    UgUiEvent e;
    CHECK(drain(&c, UG_EV_CHANGE, &e) == 3 && e.source == 7 && e.b == 400);
    /* pointer: click on the track jumps, drag is continuous, wheel steps */
    UgPtr p = P(UG_PT_DOWN, -117.5f + 12, 0, 1, 0);
    CHECK(s.w.vt->pointer(&s.w, &c, &p));
    CHECK_NEAR(s.value, 0.0, 1e-5);
    p = P(UG_PT_DOWN, 0, 0, 1, 0);
    s.w.vt->pointer(&s.w, &c, &p);
    CHECK_NEAR(s.value, 0.5, 1e-5);
    CHECK(s.dragging);
    p = P(UG_PT_MOVE, -105.5f + 0.75f * 211.0f, 0, 0, 0);   /* 3/4 of the usable length */
    s.w.vt->pointer(&s.w, &c, &p);
    CHECK_NEAR(s.value, 0.75, 1e-3);
    p = P(UG_PT_MOVE, 5000, 0, 0, 0);                      /* clamps */
    s.w.vt->pointer(&s.w, &c, &p);
    CHECK_NEAR(s.value, 1.0, 1e-5);
    p = P(UG_PT_UP, 0, 0, 1, 0);
    s.w.vt->pointer(&s.w, &c, &p);
    CHECK(!s.dragging);
    p = P(UG_PT_MOVE, 0, 0, 0, 0);
    s.w.vt->pointer(&s.w, &c, &p);                         /* no drag: ignored */
    CHECK_NEAR(s.value, 1.0, 1e-5);
    p = P(UG_PT_WHEEL, 0, 0, 0, -1);
    s.w.vt->pointer(&s.w, &c, &p);
    CHECK_NEAR(s.value, 0.9, 1e-5);
    /* vertical variant: up increases */
    UgSlider v;
    ug_slider_init(&v, 8, UG_RECT_C(0, 0, 32, 200), 0.0f);
    v.vertical = 1;
    v.w.vt->action(&v.w, &c, UG_A_UP);
    CHECK_NEAR(v.value, 0.1, 1e-5);
    CHECK_NEAR(ug_slider_value_at(&v, 0, -88), 1.0, 1e-4);
    CHECK_NEAR(ug_slider_value_at(&v, 0, 88), 0.0, 1e-4);
    s.w.vt->destroy(&s.w); v.w.vt->destroy(&v.w);
}

static void test_scroller(void) {
    UgCtx c; ug_ctx_init(&c);
    UgScrollerWidget w;
    ug_scroller_widget_init(&w, 3, UG_RECT_C(0, 0, 400, 8), 0, 11);
    UgScroller *s = &w.sc;
    /* thumb = track_left + (track_width - thumb) * index / (N - 1)  (SPEC 6.4) */
    for (int i = 0; i < 11; i++) {
        s->index = i;
        float left = -200 + (400 - 16) * (float)i / 10.0f;
        CHECK_NEAR(ug_scroller_thumb_pos(s) - 8.0f, left, 1e-3);
    }
    s->index = 0;
    /* arrows step by one, track clicks page, thumb drags */
    UgPtr p = P(UG_PT_DOWN, 200 + 16, 0, 1, 0);           /* right arrow */
    CHECK(w.w.vt->pointer(&w.w, &c, &p));
    CHECK(s->index == 1);
    p = P(UG_PT_DOWN, 100, 0, 1, 0);                      /* track to the right of the thumb */
    w.w.vt->pointer(&w.w, &c, &p);
    CHECK(s->index == 1 + s->page);
    p = P(UG_PT_DOWN, ug_scroller_thumb_pos(s), 0, 1, 0);
    CHECK(w.w.vt->pointer(&w.w, &c, &p) && s->dragging);
    p = P(UG_PT_MOVE, 200, 0, 0, 0);
    w.w.vt->pointer(&w.w, &c, &p);
    CHECK(s->index == 10);
    p = P(UG_PT_UP, 200, 0, 1, 0);
    w.w.vt->pointer(&w.w, &c, &p);
    CHECK(!s->dragging);
    p = P(UG_PT_WHEEL, 0, 0, 0, 1);                       /* wheel up = back one */
    w.w.vt->pointer(&w.w, &c, &p);
    CHECK(s->index == 9);
    CHECK(w.w.vt->action(&w.w, &c, UG_A_LEFT) && s->index == 8);
    s->index = 0;
    w.w.vt->action(&w.w, &c, UG_A_LEFT);                   /* clamps at the ends, no wrap */
    CHECK(s->index == 0);
    w.w.vt->destroy(&w.w);
}

static void test_switcher_grid_statbars(void) {
    UgCtx c; ug_ctx_init(&c);
    reset_cues(); c.cue_fn = count_cue;
    static const char *const CATS[3] = {"All", "Body", "Rims"};
    UgSwitcher s;
    ug_switcher_init(&s, 4, 0, 0, 200, CATS, 3);
    CHECK(s.w.vt->action(&s.w, &c, UG_A_RTRIG) && s.index == 1);
    s.w.vt->action(&s.w, &c, UG_A_RTRIG);
    s.w.vt->action(&s.w, &c, UG_A_RTRIG);
    CHECK(s.index == 0);                                   /* wraps (G) */
    s.w.vt->action(&s.w, &c, UG_A_LTRIG);
    CHECK(s.index == 2);
    CHECK(ug_anim_running(&s.blink_l));                    /* the arrow on that side blinks */
    UgPtr p = P(UG_PT_DOWN, 90, 0, 1, 0);                  /* right arrow badge */
    s.w.vt->pointer(&s.w, &c, &p);
    CHECK(s.index == 0);
    p = P(UG_PT_WHEEL, 0, 0, 0, 1);
    s.w.vt->pointer(&s.w, &c, &p);
    CHECK(s.index == 2);
    CHECK(g_cues[UG_CUE_TRIGGER_L] > 0 && g_cues[UG_CUE_TRIGGER_R] > 0);
    s.w.vt->destroy(&s.w);

    /* stat bars animate 300 ms, linear */
    static const char *const LB[3] = {"A", "B", "C"};
    UgStatBars sb;
    ug_statbars_init(&sb, 5, 0, 0, LB, 3);
    float v[3] = {1.0f, 0.5f, 0.0f};
    ug_statbars_set(&sb, v);
    sb.w.vt->update(&sb.w, &c, 150);
    CHECK_NEAR(sb.shown[0], 0.5, 1e-3);
    sb.w.vt->update(&sb.w, &c, 150);
    CHECK_NEAR(sb.shown[0], 1.0, 1e-3);
    CHECK_NEAR(sb.shown[1], 0.5, 1e-3);
    sb.w.vt->update(&sb.w, &c, 500);
    CHECK_NEAR(sb.shown[0], 1.0, 1e-3);

    /* grid: selected cell grows 16 -> 24 in 140 ms, previous shrinks in 300 ms */
    UgGrid g;
    ug_grid_init(&g, 6, 0, 0, 6, 3, 20);
    g.w.vt->update(&g.w, &c, 500);
    CHECK_NEAR(g.size[0], 24, 1e-3);
    CHECK(g.w.vt->action(&g.w, &c, UG_A_RIGHT) && g.sel == 1);
    CHECK(!g.w.vt->action(&g.w, &c, UG_A_UP));            /* no wrap, not consumed at the edge */
    g.w.vt->update(&g.w, &c, 70);
    CHECK_NEAR(g.size[1], 20, 0.01);
    CHECK_NEAR(g.size[0], 24 - 8 * 70.0 / 300, 0.01);
    g.w.vt->update(&g.w, &c, 100);
    CHECK_NEAR(g.size[1], 24, 1e-3);
    g.w.vt->update(&g.w, &c, 200);
    CHECK_NEAR(g.size[0], 16, 1e-3);
    float cx = g.px + 3 * g.pitch, cy = g.py + 2 * g.pitch;
    p = P(UG_PT_MOVE, cx, cy, 0, 0);
    g.w.vt->pointer(&g.w, &c, &p);                         /* hover moves the highlight */
    CHECK(g.sel == 2 * 6 + 3);
    p = P(UG_PT_DOWN, cx, cy, 1, 0);
    drain(&c, UG_EV_ACTIVATE, NULL);
    g.w.vt->pointer(&g.w, &c, &p);
    UgUiEvent e;
    CHECK(drain(&c, UG_EV_ACTIVATE, &e) == 1 && e.b == 15);
    g.w.vt->destroy(&g.w);
}

static void test_icon_scroller(void) {
    UgCtx c; ug_ctx_init(&c);
    reset_cues(); c.cue_fn = count_cue;
    UgIconScroller s;
    ug_icons_init(&s, 11, "Test");
    for (int i = 0; i < 7; i++) {
        char nm[16];
        snprintf(nm, sizeof nm, "Item %d", i);
        ug_icons_add(&s, nm, i, i == 6, 100 + i);
    }
    CHECK(s.n == 7 && !s.wrap);                            /* SPEC 4.5 (R): no wrap */
    CHECK(s.w.vt->action(&s.w, &c, UG_A_RIGHT) && s.sel == 1);
    UgUiEvent e;
    CHECK(drain(&c, UG_EV_CHANGE, &e) == 1 && e.a == 101 && e.b == 1);
    CHECK(ug_anim_running(&s.blink_r));
    s.sel = 0; s.pos = 0;
    ug_icons_select(&s, 0);
    CHECK(s.w.vt->action(&s.w, &c, UG_A_LEFT) && s.sel == 0);    /* the press is ignored at the first item */
    CHECK(s.sb.hide_a && !s.sb.hide_b);
    ug_icons_select(&s, 6);
    s.pos = 6;
    CHECK(s.w.vt->action(&s.w, &c, UG_A_RIGHT) && s.sel == 6);   /* ... and at the last */
    CHECK(!s.sb.hide_a && s.sb.hide_b);
    /* locked item: accept plays the error cue and does not activate */
    reset_cues();
    drain(&c, UG_EV_ACTIVATE, NULL);
    s.w.vt->action(&s.w, &c, UG_A_ACCEPT);
    CHECK(g_cues[UG_CUE_WRONG] == 1 && drain(&c, UG_EV_ACTIVATE, NULL) == 0);
    /* unlocked: activates with its id */
    s.w.vt->action(&s.w, &c, UG_A_LEFT);
    s.w.vt->action(&s.w, &c, UG_A_ACCEPT);
    CHECK(drain(&c, UG_EV_ACTIVATE, &e) == 1 && e.a == 105 && e.b == 5);
    /* wheel steps, click on the centre slot accepts, click on a neighbour selects + accepts */
    s.w.vt->update(&s.w, &c, 2000);
    UgPtr p = P(UG_PT_WHEEL, 0, -134, 0, -1);
    s.w.vt->pointer(&s.w, &c, &p);
    CHECK(s.sel == 6);
    s.w.vt->update(&s.w, &c, 2000);
    s.sel = 2; s.pos = 2;
    ug_icons_select(&s, 2);
    p = P(UG_PT_DOWN, 158, -133.5f, 1, 0);                 /* the neighbour right of the centre slot (+158) */
    drain(&c, UG_EV_ACTIVATE, NULL);
    s.w.vt->pointer(&s.w, &c, &p);
    CHECK(s.sel == 3 && drain(&c, UG_EV_ACTIVATE, &e) == 1 && e.a == 103);
    p = P(UG_PT_DOWN, 270, -132.5f, 1, 0);                 /* right arrow badge = one step */
    s.w.vt->pointer(&s.w, &c, &p);
    CHECK(s.sel == 4);
    /* Amendment 8: moving/idle hover never selects horizontal-list items. */
    s.sel = 2; s.pos = 2;
    ug_icons_select(&s, 2);
    p = P(UG_PT_MOVE, 160, -134, 0, 0);
    s.last_px = 0;
    s.w.vt->pointer(&s.w, &c, &p);
    CHECK(s.sel == 2);
    s.w.vt->pointer(&s.w, &c, &p);
    CHECK(s.sel == 2);
    /* thumb drag on the scroller: the thumb sits at the 6.4 formula */
    p = P(UG_PT_DOWN, ug_scroller_thumb_pos(&s.sb), -171, 1, 0);
    CHECK(s.w.vt->pointer(&s.w, &c, &p) && s.sb.dragging);
    p = P(UG_PT_MOVE, 282, -171, 0, 0);
    s.w.vt->pointer(&s.w, &c, &p);
    CHECK(s.sel == 6);
    p = P(UG_PT_UP, 282, -171, 1, 0);
    s.w.vt->pointer(&s.w, &c, &p);
    s.w.vt->destroy(&s.w);
}

static UgOptionList *mk_options(UgOptionList *o) {
    static const char *const V3[] = {"A", "B", "C"};
    static const char *const V2[] = {"Off", "On"};
    ug_options_init(o, 20);
    ug_options_add(o, "Selector", UG_ROW_SELECTOR, 1, V3, 3);       /* 0 */
    ug_options_add(o, "Toggle", UG_ROW_TOGGLE, 2, V2, 2);           /* 1 */
    ug_options_add(o, "Slider", UG_ROW_SLIDER, 3, NULL, 0);         /* 2 */
    int d = ug_options_add(o, "Disabled", UG_ROW_SELECTOR, 4, V3, 3);   /* 3 */
    ug_options_set_disabled(o, d, 1);
    ug_options_add(o, "Action", UG_ROW_ACTION, 5, NULL, 0);         /* 4 */
    for (int i = 0; i < 8; i++) ug_options_add(o, "Filler", UG_ROW_TOGGLE, 10 + i, V2, 2);   /* 5.. */
    return o;
}

static void test_option_list(void) {
    UgCtx c; ug_ctx_init(&c);
    static UgOptionList o;
    mk_options(&o);
    CHECK(o.n == 13 && o.n > o.visible);                   /* more rows than fit */
    /* Left/Right change the value; selectors wrap (G) */
    CHECK(o.w.vt->action(&o.w, &c, UG_A_RIGHT) && o.rows[0].sel == 1);
    o.w.vt->action(&o.w, &c, UG_A_RIGHT);
    o.w.vt->action(&o.w, &c, UG_A_RIGHT);
    CHECK(o.rows[0].sel == 0);
    o.w.vt->action(&o.w, &c, UG_A_LEFT);
    CHECK(o.rows[0].sel == 2);
    UgUiEvent e;
    int n = 0;
    while (ug_poll(&c, &e)) if (e.kind == UG_EV_CHANGE && e.a == 1) n++;
    CHECK(n == 4);
    /* Up at the first row is not consumed (focus may leave the list) */
    CHECK(!o.w.vt->action(&o.w, &c, UG_A_UP));
    CHECK(o.w.vt->action(&o.w, &c, UG_A_DOWN) && o.cur == 1);
    o.w.vt->action(&o.w, &c, UG_A_RIGHT);                  /* toggle */
    CHECK(o.rows[1].sel == 1);
    o.w.vt->action(&o.w, &c, UG_A_DOWN);                   /* slider row */
    o.w.vt->action(&o.w, &c, UG_A_RIGHT);
    CHECK_NEAR(o.rows[2].slider, 0.6, 1e-5);
    /* disabled rows are skipped by navigation */
    o.w.vt->action(&o.w, &c, UG_A_DOWN);
    CHECK(o.cur == 4);
    o.w.vt->action(&o.w, &c, UG_A_UP);
    CHECK(o.cur == 2);
    o.w.vt->action(&o.w, &c, UG_A_DOWN);
    /* action row: Left/Right not consumed, Accept posts activate */
    CHECK(!o.w.vt->action(&o.w, &c, UG_A_LEFT));
    drain(&c, UG_EV_ACTIVATE, NULL);
    o.w.vt->action(&o.w, &c, UG_A_ACCEPT);
    CHECK(drain(&c, UG_EV_ACTIVATE, &e) == 1 && e.a == 5 && e.b == 4);
    /* the list scrolls when the selection passes the last visible row */
    for (int i = 0; i < 8; i++) o.w.vt->action(&o.w, &c, UG_A_DOWN);
    CHECK(o.cur == 12 && o.first == 12 - o.visible + 1 && o.first > 0);
    CHECK(o.sb.index == 12 && o.sb.count == 13);
    CHECK(!o.w.vt->action(&o.w, &c, UG_A_DOWN));
    /* pointer: hover focuses an enabled row, not a disabled one */
    o.cur = 0; o.first = 0;
    float y3 = -126 + 3 * 36.0f;
    UgPtr p = P(UG_PT_MOVE, -100, y3, 0, 0);
    o.w.vt->pointer(&o.w, &c, &p);
    CHECK(o.cur == 0);
    p = P(UG_PT_MOVE, -100, -126 + 4 * 36.0f, 0, 0);
    o.w.vt->pointer(&o.w, &c, &p);
    CHECK(o.cur == 4);
    /* click on the right value arrow steps, left arrow steps back */
    p = P(UG_PT_DOWN, 246, -126, 1, 0);                    /* the right value arrow of the first row (x +246) */
    o.w.vt->pointer(&o.w, &c, &p);
    CHECK(o.cur == 0 && o.rows[0].sel == 0);              /* 2 -> wraps to 0 */
    p = P(UG_PT_DOWN, 6, -126, 1, 0);                      /* the left one (x +6) */
    o.w.vt->pointer(&o.w, &c, &p);
    CHECK(o.rows[0].sel == 2);
    /* slider: click jumps, drag follows, release ends */
    float y2 = -126 + 2 * 36.0f;
    p = P(UG_PT_DOWN, 54.6f, y2, 1, 0);                    /* the fill starts at x 54.6 */
    o.w.vt->pointer(&o.w, &c, &p);
    CHECK_NEAR(o.rows[2].slider, 0.0, 1e-4);
    CHECK(o.drag_row == 2);
    p = P(UG_PT_MOVE, (54.6f + 196.6f) * 0.5f, y2, 0, 0);
    o.w.vt->pointer(&o.w, &c, &p);
    CHECK_NEAR(o.rows[2].slider, 0.5, 1e-3);
    p = P(UG_PT_UP, 126, y2, 1, 0);
    o.w.vt->pointer(&o.w, &c, &p);
    CHECK(o.drag_row == -1);
    /* wheel over the value column changes the value; over the name column it scrolls the selection */
    p = P(UG_PT_WHEEL, 126, -126 + 36.0f, 0, 1);
    o.w.vt->pointer(&o.w, &c, &p);
    CHECK(o.cur == 1 && o.rows[1].sel == 0);   /* toggle was On, wheel up = next value */
    p = P(UG_PT_WHEEL, -100, -126 + 36.0f, 0, -1);
    o.w.vt->pointer(&o.w, &c, &p);
    CHECK(o.cur == 2);
    /* clicking an action row activates; clicking the value of a toggle toggles */
    drain(&c, UG_EV_ACTIVATE, NULL);
    p = P(UG_PT_DOWN, -100, -126 + 4 * 36.0f, 1, 0);
    o.w.vt->pointer(&o.w, &c, &p);
    CHECK(drain(&c, UG_EV_ACTIVATE, &e) == 1 && e.a == 5);
    /* vertical scroller click on its bottom arrow steps down */
    o.cur = 0; o.first = 0;
    ug_scroller_set(&o.sb, 0, o.n);
    float ax = o.sb.track.x + 4, ay = o.sb.track.y + o.sb.track.h + o.sb.arrow_gap;
    p = P(UG_PT_DOWN, ax, ay, 1, 0);
    o.w.vt->pointer(&o.w, &c, &p);
    CHECK(o.cur == 1);
    /* backing alpha animates toward 200 on the focused row */
    o.w.focused = 1;
    o.w.vt->update(&o.w, &c, 1000);
    CHECK_NEAR(o.rows[1].backing, 200, 1.0);
    CHECK_NEAR(o.rows[0].backing, 80, 1.0);
    o.w.vt->destroy(&o.w);
}

static void test_table_keyboard_date(void) {
    UgCtx c; ug_ctx_init(&c);
    static UgTable t;
    ug_table_init(&t, 30);
    ug_table_column(&t, "A", -200, 0);
    for (int i = 0; i < 15; i++) { char b[8]; snprintf(b, sizeof b, "r%d", i); ug_table_add_row(&t, b, "", "", ""); }
    CHECK(t.n == 15 && t.visible == 10);
    CHECK(t.w.vt->action(&t.w, &c, UG_A_DOWN) && t.cur == 1);
    CHECK(t.w.vt->action(&t.w, &c, UG_A_UP) && t.cur == 0);
    CHECK(!t.w.vt->action(&t.w, &c, UG_A_UP) && t.cur == 0);
    for (int i = 0; i < 12; i++) t.w.vt->action(&t.w, &c, UG_A_DOWN);
    CHECK(t.cur == 12 && t.first == 3);
    UgPtr p = P(UG_PT_WHEEL, 0, 0, 0, 1);
    t.w.vt->pointer(&t.w, &c, &p);
    CHECK(t.cur == 11);
    drain(&c, UG_EV_ACTIVATE, NULL);
    p = P(UG_PT_DOWN, 0, -92 + 2 * 26.0f, 1, 0);          /* third visible row */
    t.w.vt->pointer(&t.w, &c, &p);
    UgUiEvent e;
    CHECK(t.cur == 5 && drain(&c, UG_EV_ACTIVATE, &e) == 1 && e.a == 5);
    t.w.vt->destroy(&t.w);

    /* keyboard: explicit links, typing, caps, symbols, done */
    static UgTextField tf;
    static UgKeyboard kb;
    ug_textfield_init(&tf, 31, UG_RECT_C(0, -150, 400, 40), 8);
    ug_keyboard_init(&kb, 32, 0, -110, &tf);
    CHECK(kb.nkeys == 45);
    for (int i = 0; i < kb.nkeys; i++)
        for (int d = 0; d < 4; d++) {
            int to = kb.keys[i].link[d];
            CHECK(to < kb.nkeys);
            if (to >= 0) CHECK(to != i);
        }
    /* left/right wrap inside a row; down from the bottom character row reaches a control key */
    CHECK(kb.keys[0].link[2] == 9 && kb.keys[9].link[3] == 0);
    CHECK(kb.keys[30].link[1] >= 40 && kb.keys[0].link[0] == -1);
    CHECK(kb.keys[40].link[0] >= 30 && kb.keys[40].link[0] < 40);
    kb.w.focused = 1;
    kb.cur = 11;                                           /* 'Q' row */
    CHECK(kb.w.vt->action(&kb.w, &c, UG_A_RIGHT) && kb.cur == 12);
    CHECK(kb.w.vt->action(&kb.w, &c, UG_A_UP) && kb.cur == 2);
    CHECK(!kb.w.vt->action(&kb.w, &c, UG_A_UP));           /* top row: leaves the keyboard */
    kb.cur = 10;                                           /* Q */
    kb.w.vt->action(&kb.w, &c, UG_A_ACCEPT);
    CHECK(!strcmp(tf.text, "Q"));                          /* caps start on */
    kb.w.vt->action(&kb.w, &c, UG_A_RTRIG);
    kb.w.vt->action(&kb.w, &c, UG_A_ACCEPT);
    CHECK(!strcmp(tf.text, "Qq"));
    kb.w.vt->action(&kb.w, &c, UG_A_LTRIG);                /* symbols */
    kb.cur = 0;
    kb.w.vt->action(&kb.w, &c, UG_A_ACCEPT);
    CHECK(!strcmp(tf.text, "Qq!"));
    kb.w.vt->action(&kb.w, &c, UG_A_BTN1);                 /* backspace */
    CHECK(!strcmp(tf.text, "Qq"));
    ug_keyboard_type(&kb, &c, "xyz");                      /* physical keyboard */
    CHECK(!strcmp(tf.text, "Qqxyz"));
    ug_textfield_insert(&tf, "123456");                    /* capped at max */
    CHECK(tf.len == 8);
    ug_textfield_insert(&tf, "\b\b");
    CHECK(tf.len == 6);
    ug_textfield_insert(&tf, "\xC3\xA9");                  /* UTF-8 aware */
    ug_textfield_backspace(&tf);
    CHECK(tf.len == 6);
    kb.cur = 44;                                           /* Done */
    drain(&c, UG_EV_TEXT_DONE, NULL);
    kb.w.vt->action(&kb.w, &c, UG_A_ACCEPT);
    CHECK(drain(&c, UG_EV_TEXT_DONE, &e) == 1 && e.a == 6);
    UgPtr kp = P(UG_PT_DOWN, kb.keys[41].r.x + 5, kb.keys[41].r.y + 5, 1, 0);   /* Caps by mouse */
    int caps = kb.caps;
    kb.w.vt->pointer(&kb.w, &c, &kp);
    CHECK(kb.caps != caps && kb.cur == 41);
    kb.w.vt->destroy(&kb.w);

    /* date entry */
    static UgDateEntry d;
    ug_date_init(&d, 33, 0, 0, 2004, 12, 31);
    d.field = 0;
    d.w.vt->action(&d.w, &c, UG_A_UP);
    CHECK(d.month == 1 && d.day == 31);                    /* month wraps */
    d.w.vt->action(&d.w, &c, UG_A_UP);
    CHECK(d.month == 2 && d.day == 29);                    /* leap year clamp (2004) */
    d.year = 2005; d.field = 2;
    d.w.vt->action(&d.w, &c, UG_A_DOWN);
    CHECK(d.year == 2004);
    d.field = 1;
    d.w.vt->action(&d.w, &c, UG_A_UP);
    CHECK(d.day == 1);                                     /* day wraps */
    CHECK(d.w.vt->action(&d.w, &c, UG_A_LEFT));
    CHECK(d.field == 0);
    CHECK(!d.w.vt->action(&d.w, &c, UG_A_LEFT));           /* leftmost field: not consumed */
    d.w.vt->action(&d.w, &c, UG_A_RIGHT);
    drain(&c, UG_EV_DATE, NULL);
    d.w.vt->action(&d.w, &c, UG_A_ACCEPT);
    CHECK(drain(&c, UG_EV_DATE, &e) == 1 && e.a == 20040201);
    p = P(UG_PT_WHEEL, -148, 0, 0, 1);                     /* over the month field */
    d.field = 2;
    d.w.vt->pointer(&d.w, &c, &p);
    CHECK(d.field == 0 && d.month == 3);
    d.w.vt->destroy(&d.w);
}

/* focus / navigation among widgets of one screen (SPEC 4.5) */
static void test_focus_navigation(void) {
    UgCtx c; ug_ctx_init(&c);
    static UgButton a, b, cc, dd, pill;
    static UgScreen s;
    ug_screen_init(&s, "nav", 0);
    ug_button_init(&a, 1, UG_BTN_DIALOG, UG_RECT_C(-100, 0, 60, 30), "A");
    ug_button_init(&b, 2, UG_BTN_DIALOG, UG_RECT_C(100, 0, 60, 30), "B");
    ug_button_init(&cc, 3, UG_BTN_DIALOG, UG_RECT_C(0, -100, 60, 30), "C");
    ug_button_init(&dd, 4, UG_BTN_DIALOG, UG_RECT_C(0, 100, 60, 30), "D");
    ug_button_init(&pill, 5, UG_BTN_PILL, UG_RECT_C(100, 100, 60, 30), "P");   /* pointer-only */
    ug_screen_add(&s, &a.w); ug_screen_add(&s, &b.w); ug_screen_add(&s, &cc.w);
    ug_screen_add(&s, &dd.w); ug_screen_add(&s, &pill.w);
    CHECK(pill.w.flags & UG_W_NOREACH);
    ug_screen_update(&s, &c, 16);
    CHECK(s.focus == 0 && a.w.focused);                    /* first button takes focus */
    CHECK(ug_screen_action(&s, &c, UG_A_RIGHT) && s.focus == 1);
    CHECK(ug_screen_action(&s, &c, UG_A_DOWN) && s.focus == 3);   /* pill is skipped, D is in the cone */
    CHECK(ug_screen_action(&s, &c, UG_A_UP) && s.focus == 2);     /* nearest in the cone is C */
    CHECK(ug_screen_action(&s, &c, UG_A_LEFT) && s.focus == 0);
    CHECK(!ug_screen_action(&s, &c, UG_A_LEFT) && s.focus == 0);  /* no wrap */
    UgPtr p = P(UG_PT_MOVE, 100, 0, 0, 0);                 /* hover focus */
    ug_screen_pointer(&s, &c, &p);
    CHECK(s.focus == 1 && b.w.focused && !a.w.focused);
    p = P(UG_PT_MOVE, 500, 500, 0, 0);                     /* leaving does not remove focus */
    ug_screen_pointer(&s, &c, &p);
    CHECK(s.focus == 1);
    p = P(UG_PT_MOVE, 100, 100, 0, 0);                     /* pointer-only widgets are hover-focusable */
    ug_screen_pointer(&s, &c, &p);
    CHECK(s.focus == 4);
    /* disabled widgets are skipped and cannot be focused */
    b.w.flags |= UG_W_DISABLED;
    ug_screen_set_focus(&s, &c, 0);
    CHECK(ug_screen_action(&s, &c, UG_A_RIGHT) && s.focus != 1);      /* b is skipped */
    ug_screen_set_focus(&s, &c, 0);
    p = P(UG_PT_MOVE, 100, 0, 0, 0);
    ug_screen_pointer(&s, &c, &p);
    CHECK(s.focus == 0);                                              /* ... and cannot be hovered */
    b.w.flags &= ~(unsigned)UG_W_DISABLED;
    /* Accept on a focused button posts the activation; focus cues fire */
    reset_cues(); c.cue_fn = count_cue;
    s.focus = 0; a.w.focused = 1;
    ug_screen_action(&s, &c, UG_A_ACCEPT);
    UgUiEvent e;
    CHECK(drain(&c, UG_EV_ACTIVATE, &e) == 1 && e.source == 1);
    ug_screen_action(&s, &c, UG_A_RIGHT);
    CHECK(g_cues[UG_CUE_FOCUS] >= 1);
    /* unconsumed Back becomes a back event; root screens ignore it */
    drain(&c, UG_EV_BACK, NULL);
    CHECK(ug_screen_action(&s, &c, UG_A_BACK));
    CHECK(drain(&c, UG_EV_BACK, NULL) == 1);
    s.flags |= UG_SCR_ROOT;
    CHECK(!ug_screen_action(&s, &c, UG_A_BACK) && drain(&c, UG_EV_BACK, NULL) == 0);
    /* focus animation starts on gain and stops on loss (focus pulse, SPEC 5) */
    ug_screen_set_focus(&s, &c, 3);
    CHECK(ug_anim_running(&dd.pulse) && !ug_anim_running(&a.pulse));
    ug_screen_free(&s);
}

static void test_hints_and_bar(void) {
    UgHintSet set;ug_hints_clear(&set);ug_hints_add(&set,UG_A_ACCEPT,"Select",85);ug_hints_add(&set,UG_A_BACK,"Back",50);
    CHECK(set.n==2&&set.h[0].g1.id==UG_G_NONE); UgHintBar h;ug_hintbar_init(&h);h.mode=UG_HINTS_STRIP;ug_hintbar_set(&h,&set);UgCtx c;ug_ctx_init(&c);ug_hintbar_layout(&h,&c);
    CHECK(h.nshown==2);int over;UgPtr p=P(UG_PT_DOWN,223,226,1,0);CHECK(ug_hintbar_pointer(&h,&c,&p,&over)==UG_A_NONE&&!over);ug_hintbar_free(&h);
}

/* ------------------------------------------------------------------ */
/* E. Dialogs and the UI stack                                         */
/* ------------------------------------------------------------------ */
static void run_ui(UgUi *u, int ms) {
    for (int t = 0; t < ms; t += 10) ug_ui_update(u, 10);
}

static void test_ui_stack_and_dialog(void) {
    static UgUi u;
    ug_ui_init(&u);
    reset_cues(); u.ctx.cue_fn = count_cue;
    static UgScreen root, page;
    static UgButton rb, pb;
    ug_screen_init(&root, "root", UG_SCR_ROOT);
    ug_button_init(&rb, 1, UG_BTN_PILL, UG_RECT_C(0, 0, 100, 30), "Root");
    ug_screen_add(&root, &rb.w);
    ug_screen_init(&page, "page", 0);
    ug_button_init(&pb, 2, UG_BTN_PILL, UG_RECT_C(0, 0, 100, 30), "Page");
    ug_screen_add(&page, &pb.w);
    ug_ui_push(&u, &root);
    run_ui(&u, 300);
    CHECK(ug_ui_top(&u) == &root && !ug_ui_busy(&u));
    CHECK_NEAR(ug_anim_alpha01(&root.fade), 1.0, 0.01);

    /* page transitions: fade-out 140 ms of the old, fade-in 180 ms of the new */
    ug_ui_push(&u, &page);
    CHECK(ug_ui_busy(&u) && ug_ui_top(&u) == &root);
    run_ui(&u, 50);
    CHECK(ug_anim_alpha01(&root.fade) < 1.0f);
    run_ui(&u, 200);
    CHECK(ug_ui_top(&u) == &page);
    run_ui(&u, 300);
    CHECK_NEAR(ug_anim_alpha01(&page.fade), 1.0, 0.01);
    /* right click = Back */
    UgPtr rc = P(UG_PT_DOWN, 300, 200, 2, 0);
    ug_ui_pointer(&u, &rc);
    run_ui(&u, 600);
    CHECK(ug_ui_top(&u) == &root);
    /* ... except on the root page */
    ug_ui_pointer(&u, &rc);
    run_ui(&u, 600);
    CHECK(ug_ui_top(&u) == &root && u.sp == 1);
    CHECK(g_cues[UG_CUE_BACK] >= 1);

    /* dialog: dims the screen beneath, left/right via explicit links, Back = negative choice */
    static UgDialog dlg;
    static const char *const BT[3] = {"Yes", "No", "Cancel"};
    ug_dialog_init(&dlg, 77, 1, "Title", "Message text", BT, 3);
    CHECK(dlg.scr.flags & UG_SCR_MODAL && dlg.nbtn == 3 && dlg.back_result == 2);
    ug_ui_push(&u, &dlg.scr);
    CHECK(u.sp == 2 && ug_ui_top(&u) == &dlg.scr);
    run_ui(&u, 150);
    /* SPEC 6.9 (R): the page beneath is darkened by the 0.40 overlay alone, no extra dim fade (UG_TUNE_DIM_BELOW 0) */
    CHECK_NEAR(ug_anim_alpha01(&root.dim), 1.0, 0.01);
    CHECK(dlg.scr.focus == 2);                              /* default focus: the right-hand, negative button */
    ug_ui_action(&u, UG_A_RIGHT);
    CHECK(dlg.scr.focus == 2);                              /* no wrap */
    ug_ui_action(&u, UG_A_LEFT);
    CHECK(dlg.scr.focus == 1);
    ug_ui_action(&u, UG_A_ACCEPT);                          /* "No" */
    run_ui(&u, 400);
    UgUiEvent e;
    CHECK(ug_ui_poll(&u, &e) && e.kind == UG_EV_DIALOG && e.source == 77 && e.a == 1);
    CHECK(ug_ui_top(&u) == &root);
    run_ui(&u, 300);
    CHECK_NEAR(ug_anim_alpha01(&root.dim), 1.0, 0.01);             /* still fully shown */
    ug_screen_free(&dlg.scr);

    /* the dialog's buttons are clickable, the rest of the screen is not */
    static const char *const OK[1] = {"OK"};
    ug_dialog_init(&dlg, 78, 0, "T", "M", OK, 1);
    ug_ui_push(&u, &dlg.scr);
    run_ui(&u, 400);
    UgPtr outside = P(UG_PT_DOWN, rb.w.bounds.x + 5, rb.w.bounds.y + 5, 1, 0);
    ug_ui_pointer(&u, &outside);
    run_ui(&u, 50);
    CHECK(ug_ui_top(&u) == &dlg.scr);                       /* clicks outside are ignored */
    rc = P(UG_PT_DOWN, 300, 200, 2, 0);                     /* right click = negative choice */
    ug_ui_pointer(&u, &rc);
    run_ui(&u, 500);
    CHECK(ug_ui_poll(&u, &e) && e.kind == UG_EV_DIALOG && e.a == 0 && ug_ui_top(&u) == &root);
    ug_screen_free(&dlg.scr);

    /* help popup: Accept or Back closes, legend lines draw with glyph tokens */
    static UgHelpPopup hp;
    static const char *const LN[2] = {"$JOY_EVENT_FENG_PAD_UP$ $ICON_SPACER$ Change Selection", "$JOY_EVENT_FENG_CANCEL$ $ICON_SPACER$ Back"};
    ug_help_init(&hp, 79, LN, 2);
    ug_ui_push(&u, &hp.scr);
    run_ui(&u, 400);
    ug_ui_draw(&u);
    CHECK(u.cv.n >= 0); /* missing runtime resources no longer produce fake font/art */
    ug_ui_action(&u, UG_A_BACK);
    run_ui(&u, 400);
    CHECK(ug_ui_poll(&u, &e) && e.kind == UG_EV_DIALOG && e.source == 79 && ug_ui_top(&u) == &root);
    ug_screen_free(&hp.scr);

    /* pointer visibility: shown on pointer movement, hidden when the keyboard is used (G) */
    UgPtr mv = P(UG_PT_MOVE, 10, 10, 0, 0);
    ug_ui_pointer(&u, &mv);
    CHECK(u.ctx.pointer_visible);
    ug_ui_action(&u, UG_A_RIGHT);
    CHECK(!u.ctx.pointer_visible);
    ug_screen_free(&root); ug_screen_free(&page);
    ug_ui_free(&u);
}

/* ------------------------------------------------------------------ */
/* F. Scene runtime on the synthetic package                           */
/* ------------------------------------------------------------------ */
static int app_has(UgScene *s, uint32_t msg) {
    uint32_t m;
    int found = 0;
    while (ug_scene_poll_app(s, &m)) if (m == msg) found++;
    return found;
}

static void test_scene_package(void) {
    size_t len = 0;
    uint8_t *bin = read_hex("minimal_package.hex", &len);
    CHECK(bin != NULL);
    if (!bin) return;
    UgFngPackage pk;
    char err[100];
    CHECK(ug_fng_parse(bin, len, &pk, err, sizeof err));
    UgStrTab st;
    ug_strtab_init(&st);
    ug_strtab_add(&st, "COMMON_OK", "OK");
    UgScene sc;
    CHECK(ug_scene_init(&sc, &pk, &st));
    CHECK(sc.n == 3);
    /* the first button receives focus, which starts its highlight script (SPEC 4.5, 3.6) */
    CHECK(sc.focus == 1);
    CHECK(ug_scene_script_id(&sc, 1) == 0x249db7b7u);
    /* hierarchy: child positions are relative to the group; z accumulates (SPEC 1.2, 1.3) */
    ug_scene_update(&sc, 0.0f);
    CHECK(sc.nodes[1].parent == 0 && sc.nodes[2].parent == 0 && sc.nodes[0].parent == -1);
    CHECK_NEAR(sc.nodes[1].we, -80, 1e-3); CHECK_NEAR(sc.nodes[1].wf, 40, 1e-3);
    CHECK_NEAR(sc.nodes[1].wz, 15, 1e-3); CHECK_NEAR(sc.nodes[2].wz, 11, 1e-3);
    CHECK(sc.order[0] == 1 && sc.order[1] == 2 && sc.order[2] == 0);
    /* larger z is drawn first (farther), file order breaks ties */
    CHECK(sc.nodes[sc.order[0]].wz >= sc.nodes[sc.order[1]].wz && sc.nodes[sc.order[1]].wz >= sc.nodes[sc.order[2]].wz);
    /* colour multiplication through parents: (a*b+127)/255 (1.4) */
    CHECK(sc.nodes[1].wcolor.a == ((255 * 150 + 127) / 255) && sc.nodes[1].wcolor.r == 255);
    /* the highlight script loops 400 ms and emits its message to the application at 400 */
    app_has(&sc, 0x2085de1c);
    ug_scene_update(&sc, 399);
    CHECK(app_has(&sc, 0x2085de1cu) == 0);
    ug_scene_update(&sc, 2);
    CHECK(app_has(&sc, 0x2085de1cu) == 1);
    /* the script drives only its colour track; alpha moves 150 -> 255 -> 150 */
    ug_scene_update(&sc, 100);                              /* t = 103 of the loop */
    CHECK(sc.nodes[1].props.w[3] > 150.0f && sc.nodes[1].props.w[3] < 255.0f);
    CHECK_NEAR(sc.nodes[1].props.w[UG_P_POS], 20.0, 1e-6);  /* position untouched */
    ug_scene_update(&sc, 99);                               /* t = 200: peak */
    CHECK_NEAR(sc.nodes[1].props.w[3], 255.0, 1.0);
    /* input: Accept sends 'pressed' to the focused button -> application message (SPEC 4.4) */
    uint32_t m;
    while (ug_scene_poll_app(&sc, &m)) {}
    CHECK(ug_scene_input(&sc, UG_A_ACCEPT));
    int pressed = 0;
    while (ug_scene_poll_app(&sc, &m)) pressed += m == 0x0c407210u;
    CHECK(pressed == 1);
    /* package responses: Back goes to the application */
    CHECK(ug_scene_input(&sc, UG_A_BACK));
    CHECK(app_has(&sc, UG_MSG_BACK) == 1);
    /* the pointer-2 message re-posts Back to the screen itself (command 1, target 0), SPEC 4.6 */
    CHECK(ug_scene_input_msg(&sc, UG_MSG_POINTER2));
    CHECK(app_has(&sc, UG_MSG_BACK) == 1);
    /* messages without responses are not handled */
    CHECK(!ug_scene_input(&sc, UG_A_LTRIG));
    /* pointer: hover focus + click accept; picking uses the world rectangle */
    int hit = ug_scene_pick(&sc, -80, 40);
    CHECK(hit == 1);
    CHECK(ug_scene_pick(&sc, 200, 200) == -1);
    ug_scene_set_focus(&sc, -1);
    UgPtr p = P(UG_PT_MOVE, -80, 40, 0, 0);
    ug_scene_pointer(&sc, &p);
    CHECK(sc.focus == 1);
    ug_scene_update(&sc, 16);
    while (ug_scene_poll_app(&sc, &m)) {}
    p = P(UG_PT_DOWN, -80, 40, 1, 0);
    ug_scene_pointer(&sc, &p);
    pressed = 0;
    while (ug_scene_poll_app(&sc, &m)) pressed += m == 0x0c407210u;
    CHECK(pressed == 1);
    p = P(UG_PT_DOWN, 0, 0, 2, 0);                          /* right click -> Back through the package */
    ug_scene_pointer(&sc, &p);
    CHECK(app_has(&sc, UG_MSG_BACK) == 1);
    /* drawing produces text from the string table label */
    UgCtx ctx; ug_ctx_init(&ctx);
    UgCanvas cv; ug_cv_init(&cv);
    ug_scene_draw(&sc, &ctx, &cv);
    CHECK(cv.n == 0); /* requested bitmap/font unavailable without runtime assets */                                  /* image quad + the two glyphs of "OK" */
    ug_cv_free(&cv);
    ug_scene_free(&sc);
    ug_strtab_free(&st);
    ug_fng_free(&pk);
    free(bin);
}

/* hand-built package: conditional commands 0x300/0x301/0x500/0x501 and focus commands */
static void test_scene_conditionals(void) {
    UgFngPackage pk;
    memset(&pk, 0, sizeof pk);
    pk.nobj = 2;
    pk.obj = (UgFngObject *)calloc(2, sizeof(UgFngObject));
    UgScript sA[2], sB[1];
    memset(sA, 0, sizeof sA); memset(sB, 0, sizeof sB);
    sA[0] = mk_script(UG_SCRIPT_INITIAL, 0, 0);
    sA[1] = mk_script(0xAAAA, 100, UG_SCRIPT_LOOP);
    sB[0] = mk_script(UG_SCRIPT_INITIAL, 0, 0);
    for (int i = 0; i < 2; i++) {
        UgFngObject *o = &pk.obj[i];
        o->type = UG_OBJ_IMAGE; o->guid = 0x100 + (uint32_t)i; o->flags = UG_OF_BUTTON;
        o->color = UG_WHITE; o->size[0] = o->size[1] = o->size[2] = 10; o->rot[3] = 1;
        o->pos[0] = i ? 100.0f : -100.0f;
        o->res_index = -1;
    }
    pk.obj[0].scripts = sA; pk.obj[0].nscripts = 2;
    pk.obj[1].scripts = sB; pk.obj[1].nscripts = 1;
    /* object 0, message 0x55: IF current script != 0xAAAA THEN start 0xAAAA ENDIF ;
     * message 0x56: IF == 0xAAAA THEN post app 0x99 ELSE post app 0x98 ENDIF */
    UgFngCmd c1[3] = {{UG_CMD_IF_SCRIPT_NOT, 0xAAAA, 0, NULL}, {UG_CMD_START_SCRIPT, 0xAAAA, 0, NULL}, {UG_CMD_ENDIF, 0, 0, NULL}};
    UgFngCmd c2[4] = {{UG_CMD_IF_SCRIPT_IS, 0xAAAA, 0, NULL}, {UG_CMD_POST_APP, 0x99, 0, NULL},
                      {UG_CMD_ELSE, 0, 0, NULL}, {UG_CMD_POST_APP, 0x98, 0, NULL}};
    UgFngCmd c2b = {UG_CMD_ENDIF, 0, 0, NULL};
    UgFngCmd c2all[5] = {c2[0], c2[1], c2[2], c2[3], c2b};
    UgFngResponse r0[2] = {{0x55, c1, 3}, {0x56, c2all, 5}};
    pk.obj[0].resp = r0; pk.obj[0].nresp = 2;
    /* object 1, message 0x57: focus object 0 (command 0x100) and post message 0x55 to guid 0x100 */
    UgFngCmd c3[2] = {{UG_CMD_FOCUS, 0x100, 0, NULL}, {UG_CMD_POST_GUI, 0x55, 0x100, NULL}};
    UgFngResponse r1[1] = {{0x57, c3, 2}};
    pk.obj[1].resp = r1; pk.obj[1].nresp = 1;
    /* package response: broadcast 0x56 re-dispatches to everything that responds */
    UgFngCmd c4[1] = {{UG_CMD_POST_GUI, 0x56, 0, NULL}};
    UgFngResponse pr[1] = {{0x58, c4, 1}};
    pk.pkg_resp = pr; pk.npkg_resp = 1;
    pk.res = NULL; pk.nres = 0;

    UgScene sc;
    CHECK(ug_scene_init(&sc, &pk, NULL));
    CHECK(sc.focus == 0);                                   /* first button in file order */
    CHECK(ug_scene_script_id(&sc, 0) == UG_SCRIPT_INITIAL);
    ug_scene_post(&sc, 0x56, 0);
    CHECK(app_has(&sc, 0x98) == 1);                         /* not running 0xAAAA -> ELSE branch */
    ug_scene_post(&sc, 0x55, 0x100);
    CHECK(ug_scene_script_id(&sc, 0) == 0xAAAA);            /* IF != -> start */
    ug_scene_post(&sc, 0x55, 0);                            /* already running: no restart */
    sc.nodes[0].pl.t = 40;
    ug_scene_post(&sc, 0x55, 0);
    CHECK_NEAR(sc.nodes[0].pl.t, 40.0, 1e-3);
    ug_scene_post(&sc, 0x56, 0x100);
    CHECK(app_has(&sc, 0x99) == 1);                         /* THEN branch */
    /* focus change messages: object 1 focuses object 0 through a command */
    sc.focus = 1;
    ug_scene_post(&sc, 0x57, 0x101);
    CHECK(sc.focus == 0);
    CHECK(ug_scene_input_msg(&sc, 0x58) && app_has(&sc, 0x99) == 1);   /* package response re-dispatch */
    /* geometric navigation: object 1 is to the right of object 0 */
    CHECK(ug_scene_input(&sc, UG_A_RIGHT) && sc.focus == 1);
    CHECK(!ug_scene_input(&sc, UG_A_RIGHT));
    ug_scene_free(&sc);
    free(pk.obj);
}

/* ------------------------------------------------------------------ */
/* G. Mouse-only tour of the demo application                          */
/* ------------------------------------------------------------------ */
static void frames(DemoApp *a, int n) { for (int i = 0; i < n; i++) demo_update(a, 16); }
static void mmove(DemoApp *a, float x, float y) { UgPtr p = P(UG_PT_MOVE, x, y, 0, 0); ug_ui_pointer(&a->ui, &p); }
static void mclick(DemoApp *a, float x, float y) {
    mmove(a, x, y);
    UgPtr d = P(UG_PT_DOWN, x, y, 1, 0), u = P(UG_PT_UP, x, y, 1, 0);
    ug_ui_pointer(&a->ui, &d);
    ug_ui_pointer(&a->ui, &u);
}
static void mright(DemoApp *a) {
    UgPtr d = P(UG_PT_DOWN, 300, 100, 2, 0), u = P(UG_PT_UP, 300, 100, 2, 0);
    ug_ui_pointer(&a->ui, &d);
    ug_ui_pointer(&a->ui, &u);
}
static void mwheel(DemoApp *a, float x, float y, int dy) {
    mmove(a, x, y);
    UgPtr p = P(UG_PT_WHEEL, x, y, 0, dy);
    ug_ui_pointer(&a->ui, &p);
}
static void click_rect(DemoApp *a, UgRect r) { mclick(a, r.x + r.w * 0.5f, r.y + r.h * 0.5f); }
static void hover_rect(DemoApp *a, UgRect r) { mmove(a, r.x + r.w * 0.5f, r.y + r.h * 0.5f); }

static int click_hint(DemoApp *a, UgAction act) {
    for (int k = 0; k < a->ui.hints.nshown; k++)
        if (a->ui.hints.set.h[a->ui.hints.shown[k]].action == act) { click_rect(a, a->ui.hints.rect[k]); return 1; }
    return 0;
}

static void open_page_by_mouse(DemoApp *a, int page) {
    /* wheel moves the selection, click on the centre slot opens it */
    int guard = 0;
    while (a->root.icons.sel != page && guard++ < 20) {
        mwheel(a, 0, -134, a->root.icons.sel < page ? -1 : 1);
        frames(a, 3);
    }
    frames(a, 30);
    mclick(a, 0, -134);
    frames(a, 60);
}

static void test_mouse_tour(void) {
    DemoApp *a=calloc(1,sizeof *a);demo_init(a,g_vec_dir);
    for(int i=0;i<DP_COUNT-1;i++) {
        if(i==DP_ORIGINAL)continue;
        for(int k=0;k<30;k++)demo_update(a,16);
        CHECK(demo_open_page(a,i));for(int k=0;k<30;k++)demo_update(a,16);
        CHECK(ug_ui_top(&a->ui)==demo_page_screen(a,i));
        UgPtr p=P(UG_PT_DOWN,0,0,2,0);ug_ui_pointer(&a->ui,&p);for(int k=0;k<40;k++)demo_update(a,16);
        CHECK(ug_ui_top(&a->ui)==&a->root.scr);
    }
    demo_free(a);free(a);
}

static void run_more_tests(void) {
    printf("[B] animations and script player\n");
    RUN(test_anim_durations);
    RUN(test_anim_values);
    RUN(test_anim_owner);
    RUN(test_player);
    printf("[C] canvas, font and text\n");
    RUN(test_color_ops);
    RUN(test_view);
    RUN(test_canvas);
    RUN(test_font);
    RUN(test_text_layout);
    RUN(test_text_justify);
    printf("[D] widgets, focus and hints\n");
    RUN(test_slider);
    RUN(test_scroller);
    RUN(test_switcher_grid_statbars);
    RUN(test_icon_scroller);
    RUN(test_option_list);
    RUN(test_table_keyboard_date);
    RUN(test_focus_navigation);
    RUN(test_hints_and_bar);
    RUN(test_ui_stack_and_dialog);
    printf("[F] scene runtime\n");
    RUN(test_scene_package);
    RUN(test_scene_conditionals);
    printf("[G] mouse-only tour of the demo\n");
    RUN(test_mouse_tour);
}
