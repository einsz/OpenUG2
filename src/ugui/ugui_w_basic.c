/* ugui_w_basic.c -- label, panel, button, slider, scroller, category
 * switcher, stat bars, grid picker, busy indicators, movie placeholder. */
#include "ugui_ui.h"
#include "ugui_tuning.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

/* ------------------------------------------------------------------ */
/* Label and panel                                                     */
/* ------------------------------------------------------------------ */
static void label_draw(UgWidget *w, UgCtx *c, UgCanvas *cv) {
    UgLabel *l = (UgLabel *)w;
    const char *s = l->text;
    if (l->label && c->strings) s = ug_strtab_or(c->strings, ug_hash_upper(l->label), l->text);
    ug_text_draw(cv, s, w->bounds.x, w->bounds.y, l->scale, l->color, l->just, l->wrap, l->leading, c->glyphs);
}
static const UgWidgetVT LABEL_VT = {NULL, NULL, NULL, label_draw, NULL, NULL, NULL};

void ug_label_init(UgLabel *l, int id, const char *text, float x, float y,
                   float scale, UgColor col, unsigned just) {
    memset(l, 0, sizeof *l);
    ug_widget_init(&l->w, &LABEL_VT, "label", id, (UgRect){x, y, 0, 0});
    l->w.flags = UG_W_NOFOCUS;
    snprintf(l->text, sizeof l->text, "%s", text);
    l->scale = scale; l->color = col; l->just = just;
}

static void panel_draw(UgWidget *w, UgCtx *c, UgCanvas *cv) {
    (void)c;
    UgPanel *p = (UgPanel *)w;
    ug_kit_panel(cv, w->bounds, p->light, p->alpha);
}
static const UgWidgetVT PANEL_VT = {NULL, NULL, NULL, panel_draw, NULL, NULL, NULL};

void ug_panel_init(UgPanel *p, int id, UgRect r, int light, float alpha) {
    memset(p, 0, sizeof *p);
    ug_widget_init(&p->w, &PANEL_VT, "panel", id, r);
    p->w.flags = UG_W_NOFOCUS;
    p->light = light; p->alpha = alpha;
}

/* ------------------------------------------------------------------ */
/* Button                                                              */
/* ------------------------------------------------------------------ */
static void button_press(UgButton *b, UgCtx *c) {
    ug_anim_play(&b->press, UG_ANIM_BLINK, 0);
    ug_cue(c, UG_CUE_BUTTON);
    if (b->post_kind) ug_post(c, b->post_kind, b->post_source, b->w.id, 0);
    else ug_post(c, UG_EV_ACTIVATE, b->w.id, b->w.id, 0);
}

static void button_update(UgWidget *w, UgCtx *c, float dt) {
    (void)c;
    UgButton *b = (UgButton *)w;
    ug_anim_update(&b->pulse, dt);
    ug_anim_update(&b->glow, dt);
    ug_anim_update(&b->press, dt);
}

static void button_focus(UgWidget *w, UgCtx *c, int gained) {
    (void)c;
    UgButton *b = (UgButton *)w;
    if (gained) {
        if (b->style == UG_BTN_PILL) {
            ug_anim_play(&b->pulse, UG_ANIM_PILL_FOCUS, 0);
            ug_anim_play(&b->glow, UG_ANIM_PILL_GLOW, 0);
        } else {
            ug_anim_play(&b->pulse, UG_ANIM_FOCUS_PULSE, 0);
        }
    } else {
        ug_anim_stop(&b->pulse);
        ug_anim_stop(&b->glow);
    }
}

static int button_action(UgWidget *w, UgCtx *c, UgAction a) {
    UgButton *b = (UgButton *)w;
    if (a != UG_A_ACCEPT) return 0;
    button_press(b, c);
    return 1;
}

static int button_pointer(UgWidget *w, UgCtx *c, const UgPtr *p) {
    UgButton *b = (UgButton *)w;
    if (p->type == UG_PT_DOWN && p->button == 1) { button_press(b, c); return 1; }
    return p->type == UG_PT_UP;
}

static void button_draw(UgWidget *w, UgCtx *c, UgCanvas *cv) {
    UgButton *b = (UgButton *)w;
    UgRect r = w->bounds;
    float grow = b->press.has_script ? (b->press.props.w[UG_P_SIZE] - b->press.rest.w[UG_P_SIZE]) : 0;
    float cx = r.x + r.w * 0.5f, cy = r.y + r.h * 0.5f;
    r.x -= grow * 0.5f; r.y -= grow * 0.5f; r.w += grow; r.h += grow;
    int foc = w->focused;
    float alpha = (w->flags & UG_W_DISABLED) ? 120.0f / 255.0f : 1.0f;
    ug_cv_push(cv);
    ug_cv_mul_alpha(cv, alpha);
    if (b->style == UG_BTN_DIALOG) {
        /* SPEC 6.9 (R): the focused button has a dark backing, clearly darker than the light inner panel; the
         * unfocused one a light-grey backing close to the panel's tone and a grey (150) label. The 42 x 42 end
         * caps have transparent margins, so the authored extent is wider than the visible one. */
        if (foc) {
            if (ug_kit_skin()) {
                float m = UG_R_DLG_BTN_CAP_MARGIN;
                ug_kit_cap_box(cv, (UgRect){r.x - m, cy - 21.0f, r.w + 2 * m, 42.0f}, 42.0f, UG_WHITE, UG_BORDER, 0);
            }
        } else {
            ug_cv_fill(cv, r, UG_RGBA(255, 255, 255, 105), r.h * 0.5f);
        }
        UgColor base = foc ? UG_TEXT_LIGHT : UG_RGBA(150, 150, 150, 255);
        ug_text_draw_ink(cv,b->label,cx,cy,b->label_scale,base,UG_J_CENTER|UG_J_VCENTER,c->glyphs);
        if(foc)ug_text_draw_ink(cv,b->label,cx,cy,b->label_scale,ug_anim_color(&b->pulse),UG_J_CENTER|UG_J_VCENTER,c->glyphs);
    } else if (b->style == UG_BTN_PILL) {
        UgColor tint = foc ? ug_anim_color(&b->pulse) : UG_RGBA(255, 255, 255, 150);
        if (foc) ug_kit_pill_glow(cv, r, ug_anim_color(&b->glow));
        ug_kit_pill_pc(cv, r, tint);
        ug_kit_text(c, cv, b->label, cx, cy, b->label_scale, foc ? UG_ACCENT : UG_WHITE, UG_J_CENTER | UG_J_VCENTER);
    } else {   /* UG_BTN_HINT: bare text with glyph tokens */
        UgColor base = UG_TEXT_HINT;
        ug_text_draw(cv, b->label, r.x, cy, b->label_scale, base, UG_J_VCENTER, 0, 0, c->glyphs);
        if (foc) ug_text_draw(cv, b->label, r.x, cy, b->label_scale, ug_anim_color(&b->pulse), UG_J_VCENTER, 0, 0, c->glyphs);
    }
    ug_cv_pop(cv);
}

static void button_destroy(UgWidget *w) {
    UgButton *b = (UgButton *)w;
    ug_anim_free(&b->pulse); ug_anim_free(&b->glow); ug_anim_free(&b->press);
}

static void button_hints(UgWidget *w, UgHintSet *h) {
    (void)w;
    ug_hints_add(h, UG_A_ACCEPT, "Press", 90);
}

static const UgWidgetVT BUTTON_VT = {button_update, button_action, button_pointer,
                                     button_draw, button_focus, button_hints, button_destroy};

void ug_button_init(UgButton *b, int id, int style, UgRect r, const char *label) {
    memset(b, 0, sizeof *b);
    ug_widget_init(&b->w, &BUTTON_VT, "button", id, r);
    b->style = style;
    snprintf(b->label, sizeof b->label, "%s", label);
    b->label_scale = style == UG_BTN_DIALOG ? 0.75f : (style == UG_BTN_PILL ? 0.45f : 0.7f);
    if (style == UG_BTN_PILL) b->w.flags |= UG_W_NOREACH;
    ug_anim_init(&b->pulse, UG_WHITE, 0, 0);
    ug_anim_init(&b->glow, UG_WHITE, 0, 0);
    ug_anim_init(&b->press, UG_RGBA(255, 255, 255, 255), 24, 24);
}

/* ------------------------------------------------------------------ */
/* Slider                                                              */
/* ------------------------------------------------------------------ */
#define SLIDER_MARGIN 12.0f

void ug_slider_draw_at(UgCanvas *cv, UgRect t, float v, int vertical, int focused,
                       float alpha01, const UgAnim *pulse) {
    ug_slider_draw_ex(cv, t, v, vertical, focused, alpha01, pulse, 6.0f, SLIDER_MARGIN);
}

void ug_slider_draw_ex(UgCanvas *cv, UgRect t, float v, int vertical, int focused,
                       float alpha01, const UgAnim *pulse, float fill_h, float margin) {
    const UgSkin *sk = ug_kit_skin();
    ug_cv_push(cv);
    ug_cv_mul_alpha(cv, alpha01);
    if (sk && sk->slider_bg && !vertical) {
        UgRect bt = t;
        bt.y += sk->slider_bg_dy * t.h;                  /* centre the art's bar on the fill */
        ug_cv_image_rect(cv, bt, sk->slider_bg, UG_RGBA(255, 255, 255, 120));
    }

    float len = (vertical ? t.h : t.w) - 2 * margin;
    float cx = t.x + t.w * 0.5f, cy = t.y + t.h * 0.5f;
    float kx, ky, sx, sy;
    if (vertical) { sx = cx; sy = t.y + t.h - margin; kx = cx; ky = sy - v * len; }
    else          { sx = t.x + margin; sy = cy; kx = sx + v * len; ky = cy; }
    float fh = fill_h * 0.5f;
    /* fill bar with rounded end pieces */
    UgColor g = UG_GREEN;
    if (sk && sk->white) {
        if (vertical) ug_cv_image_rect(cv, (UgRect){sx - fh, ky, fill_h, sy - ky}, sk->white, g);
        else          ug_cv_image_rect(cv, (UgRect){sx, sy - fh, kx - sx, fill_h}, sk->white, g);
    }
    if (sk && sk->slider_fill_corner) {
        ug_cv_image_rect(cv, UG_RECT_C(sx, sy, 15, 15), sk->slider_fill_corner, g);
        ug_cv_image_rect(cv, UG_RECT_C(kx, ky, 15, 15), sk->slider_fill_corner, g);
    }
    /* knob */
    if (sk && sk->slider_knob) {
        ug_cv_image_rect(cv, UG_RECT_C(kx, ky, 16, 16), sk->slider_knob, UG_WHITE);
        if (focused) {
            UgColor rim = pulse ? ug_color_lerp(UG_WHITE, UG_ACCENT, 0.6f + 0.4f * ug_anim_alpha01(pulse)) : UG_ACCENT;
            ug_cv_outline(cv, UG_RECT_C(kx, ky, 16, 16), rim, 4, 1.4f);
        }
    }
    ug_cv_pop(cv);
}

float ug_slider_value_at(const UgSlider *s, float x, float y) {
    const UgRect *t = &s->w.bounds;
    float len = (s->vertical ? t->h : t->w) - 2 * SLIDER_MARGIN;
    float v = s->vertical ? ((t->y + t->h - SLIDER_MARGIN) - y) / len : (x - (t->x + SLIDER_MARGIN)) / len;
    return clampf(v, 0, 1);
}

void ug_slider_set(UgSlider *s, float v) { s->value = clampf(v, 0, 1); }

static void slider_changed(UgSlider *s, UgCtx *c) {
    ug_cue(c, UG_CUE_SLIDER);
    ug_post(c, UG_EV_CHANGE, s->w.id, s->w.id, (int)(s->value * 1000.0f + 0.5f));
}

static void slider_step(UgSlider *s, UgCtx *c, int dir) {
    float st = 1.0f / (float)(s->steps > 0 ? s->steps : 10);
    float n = floorf(s->value / st + 0.5f) + (float)dir;
    ug_slider_set(s, n * st);
    slider_changed(s, c);
}

static int slider_action(UgWidget *w, UgCtx *c, UgAction a) {
    UgSlider *s = (UgSlider *)w;
    int dec = s->vertical ? UG_A_DOWN : UG_A_LEFT, inc = s->vertical ? UG_A_UP : UG_A_RIGHT;
    if ((int)a == dec) { slider_step(s, c, -1); return 1; }
    if ((int)a == inc) { slider_step(s, c, +1); return 1; }
    return 0;
}

static int slider_pointer(UgWidget *w, UgCtx *c, const UgPtr *p) {
    UgSlider *s = (UgSlider *)w;
    if (p->type == UG_PT_DOWN && p->button == 1) {
        s->dragging = 1;
        ug_slider_set(s, ug_slider_value_at(s, p->x, p->y));
        slider_changed(s, c);
        return 1;
    }
    if (p->type == UG_PT_MOVE && s->dragging) {
        float old = s->value;
        ug_slider_set(s, ug_slider_value_at(s, p->x, p->y));
        if (old != s->value) slider_changed(s, c);
        return 1;
    }
    if (p->type == UG_PT_UP) { s->dragging = 0; return 1; }
    if (p->type == UG_PT_WHEEL) { slider_step(s, c, p->wheel > 0 ? 1 : -1); return 1; }
    return 0;
}

static void slider_update(UgWidget *w, UgCtx *c, float dt) { (void)c; ug_anim_update(&((UgSlider *)w)->pulse, dt); }
static void slider_focus(UgWidget *w, UgCtx *c, int g) {
    (void)c;
    UgSlider *s = (UgSlider *)w;
    if (g) ug_anim_play(&s->pulse, UG_ANIM_FOCUS_PULSE, 0); else ug_anim_stop(&s->pulse);
}
static void slider_draw(UgWidget *w, UgCtx *c, UgCanvas *cv) {
    (void)c;
    UgSlider *s = (UgSlider *)w;
    ug_slider_draw_at(cv, w->bounds, s->value, s->vertical, w->focused,
                      (w->flags & UG_W_DISABLED) ? 0.47f : 1.0f, &s->pulse);
}
static void slider_hints(UgWidget *w, UgHintSet *h) {
    (void)w;
    ug_hints_add(h, UG_A_INFO_DRAG, "Adjust", 75);
}
static void slider_destroy(UgWidget *w) { ug_anim_free(&((UgSlider *)w)->pulse); }
static const UgWidgetVT SLIDER_VT = {slider_update, slider_action, slider_pointer,
                                     slider_draw, slider_focus, slider_hints, slider_destroy};

void ug_slider_init(UgSlider *s, int id, UgRect track, float value) {
    memset(s, 0, sizeof *s);
    ug_widget_init(&s->w, &SLIDER_VT, "slider", id, track);
    s->w.flags |= UG_W_CAPTURE;
    s->value = clampf(value, 0, 1);
    s->steps = 10;
    ug_anim_init(&s->pulse, UG_WHITE, 0, 0);
}

/* ------------------------------------------------------------------ */
/* Scroller                                                            */
/* ------------------------------------------------------------------ */
void ug_scroller_init(UgScroller *s, UgRect track, int vertical, int arrows) {
    memset(s, 0, sizeof *s);
    s->track = track;
    s->vertical = vertical;
    s->with_arrows = arrows;
    s->count = 1;
    s->page = 3;
    s->arrow_size = 24;
    s->arrow_gap = 16;
    s->inset = 8.0f;
    ug_anim_init(&s->blink_a, UG_RGBA(255, 255, 255, 150), 24, 24);
    ug_anim_init(&s->blink_b, UG_RGBA(255, 255, 255, 150), 24, 24);
}

void ug_scroller_free(UgScroller *s) {
    ug_anim_free(&s->blink_a);
    ug_anim_free(&s->blink_b);
}

void ug_scroller_set(UgScroller *s, int index, int count) {
    s->count = count < 1 ? 1 : count;
    s->index = index < 0 ? 0 : (index >= s->count ? s->count - 1 : index);
}

void ug_scroller_update(UgScroller *s, float dt) {
    ug_anim_update(&s->blink_a, dt);
    ug_anim_update(&s->blink_b, dt);
}

void ug_scroller_blink(UgScroller *s, int toward_end) {
    ug_anim_play(toward_end ? &s->blink_b : &s->blink_a, UG_ANIM_BLINK, 0);
}

/* thumb centre along the axis: proportional to index / (count - 1) (SPEC 6.4, R) */
float ug_scroller_thumb_pos(const UgScroller *s) {
    float f = s->count > 1 ? (float)s->index / (float)(s->count - 1) : 0.0f;
    if (s->has_range) return s->lo + (s->hi - s->lo) * f;
    float len = (s->vertical ? s->track.h : s->track.w) - 2.0f * s->inset;
    float start = s->vertical ? s->track.y : s->track.x;
    return start + s->inset + len * f;
}

void ug_scroller_arrows(const UgScroller *s, float *ax, float *ay, float *bx, float *by) {
    float g = s->arrow_gap;
    if (s->vertical) {
        *ax = *bx = s->track.x + s->track.w * 0.5f;
        *ay = s->track.y - g; *by = s->track.y + s->track.h + g;
    } else {
        *ay = *by = s->track.y + s->track.h * 0.5f;
        *ax = s->track.x - g; *bx = s->track.x + s->track.w + g;
    }
}

void ug_scroller_draw(UgScroller *s, UgCanvas *cv, float alpha01) {
    const UgSkin *sk = ug_kit_skin();
    ug_cv_push(cv);
    ug_cv_mul_alpha(cv, alpha01);
    UgRect t = s->track;
    if (sk && sk->box_w) ug_cv_image_rect(cv, t, sk->box_w, UG_RGBA(255, 255, 255, 140));

    float tc = ug_scroller_thumb_pos(s);
    float cx = t.x + t.w * 0.5f, cy = t.y + t.h * 0.5f;
    if (s->has_cross) { if (s->vertical) cx = s->cross; else cy = s->cross; }
    float tcx = t.x + t.w * 0.5f, tcy = t.y + t.h * 0.5f;       /* track centre: the end dots stay on it */
    /* end dots */
    float e0x = s->vertical ? tcx : t.x + 1, e0y = s->vertical ? t.y + 1 : tcy;
    float e1x = s->vertical ? tcx : t.x + t.w - 1, e1y = s->vertical ? t.y + t.h - 1 : tcy;
    if (sk && sk->scr_dot) {
        UgColor d = UG_RGBA(255, 255, 255, 140);
        ug_cv_image_rect(cv, UG_RECT_C(e0x, e0y, 8, 8), sk->scr_dot, d);
        ug_cv_image_rect(cv, UG_RECT_C(e1x, e1y, 8, 8), sk->scr_dot, d);
    }
    /* thumb tab 16x16 */
    UgRect th = s->vertical ? UG_RECT_C(cx, tc, 16, 16) : UG_RECT_C(tc, cy, 16, 16);
    if (sk && sk->scr_tab) ug_cv_image_rect(cv, th, sk->scr_tab, UG_RGBA(255, 255, 255, 200));
    if (s->with_arrows) {
        float ax, ay, bx, by;
        ug_scroller_arrows(s, &ax, &ay, &bx, &by);
        int da = s->vertical ? 2 : 0, db = s->vertical ? 3 : 1;
        if (!s->hide_a) ug_kit_badge(cv, ax, ay, s->blink_a.props.w[UG_P_SIZE], da, ug_anim_color(&s->blink_a), 0);
        if (!s->hide_b) ug_kit_badge(cv, bx, by, s->blink_b.props.w[UG_P_SIZE], db, ug_anim_color(&s->blink_b), 0);
    }
    ug_cv_pop(cv);
}

static void idx_from_pos(const UgScroller *s, float x, float y, int *out) {
    float len = (s->vertical ? s->track.h : s->track.w) - 2.0f * s->inset;
    float start = (s->vertical ? s->track.y : s->track.x) + s->inset;
    if (s->has_range) { start = s->lo; len = s->hi - s->lo; }
    float pos = s->vertical ? y : x;
    float f = len > 0 ? clampf((pos - start) / len, 0, 1) : 0;
    *out = (int)floorf(f * (float)(s->count - 1) + 0.5f);
}

int ug_scroller_pointer(UgScroller *s, const UgPtr *p, int *new_index) {
    if (p->type == UG_PT_UP) { int was = s->dragging; s->dragging = 0; return was ? 1 : 0; }
    if (p->type == UG_PT_MOVE && s->dragging) {
        int idx;
        idx_from_pos(s, p->x, p->y, &idx);
        if (idx != s->index) { *new_index = idx; return 2; }
        return 1;
    }
    if (p->type != UG_PT_DOWN || p->button != 1) return 0;
    if (s->with_arrows) {
        float ax, ay, bx, by, h = s->arrow_size * 0.5f;
        ug_scroller_arrows(s, &ax, &ay, &bx, &by);
        if (!s->hide_a && ug_rect_hit(UG_RECT_C(ax, ay, 2 * h, 2 * h), p->x, p->y)) {
            *new_index = s->index > 0 ? s->index - 1 : 0;
            ug_scroller_blink(s, 0);
            return 2;
        }
        if (!s->hide_b && ug_rect_hit(UG_RECT_C(bx, by, 2 * h, 2 * h), p->x, p->y)) {
            *new_index = s->index < s->count - 1 ? s->index + 1 : s->count - 1;
            ug_scroller_blink(s, 1);
            return 2;
        }
    }
    float tc = ug_scroller_thumb_pos(s);
    float cx = s->track.x + s->track.w * 0.5f, cy = s->track.y + s->track.h * 0.5f;
    if (s->has_cross) { if (s->vertical) cx = s->cross; else cy = s->cross; }
    UgRect th = s->vertical ? UG_RECT_C(cx, tc, 20, 20) : UG_RECT_C(tc, cy, 20, 20);
    if (ug_rect_hit(th, p->x, p->y)) { s->dragging = 1; return 1; }
    UgRect grab = s->track;
    if (s->vertical) { grab.x -= 8; grab.w += 16; } else { grab.y -= 8; grab.h += 16; }
    if (ug_rect_hit(grab, p->x, p->y)) {            /* click on the track = page */
        float pos = s->vertical ? p->y : p->x;
        int dir = pos < tc ? -1 : 1;
        int ni = s->index + dir * s->page;
        *new_index = ni < 0 ? 0 : (ni >= s->count ? s->count - 1 : ni);
        ug_scroller_blink(s, dir > 0);
        return 2;
    }
    return 0;
}

static void sw_update(UgWidget *w, UgCtx *c, float dt) { (void)c; ug_scroller_update(&((UgScrollerWidget *)w)->sc, dt); }
static void sw_draw(UgWidget *w, UgCtx *c, UgCanvas *cv) {
    (void)c;
    ug_scroller_draw(&((UgScrollerWidget *)w)->sc, cv, (w->flags & UG_W_DISABLED) ? 0.47f : 1.0f);
}
static void sw_changed(UgScrollerWidget *w, UgCtx *c, int idx) {
    if (idx == w->sc.index) return;
    ug_scroller_blink(&w->sc, idx > w->sc.index);
    w->sc.index = idx;
    ug_cue(c, UG_CUE_SCROLLBAR);
    ug_post(c, UG_EV_CHANGE, w->w.id, w->w.id, idx);
}
static int sw_action(UgWidget *w, UgCtx *c, UgAction a) {
    UgScrollerWidget *s = (UgScrollerWidget *)w;
    int dec = s->sc.vertical ? UG_A_UP : UG_A_LEFT, inc = s->sc.vertical ? UG_A_DOWN : UG_A_RIGHT;
    if ((int)a == dec) { if (s->sc.index > 0) sw_changed(s, c, s->sc.index - 1); return 1; }
    if ((int)a == inc) { if (s->sc.index < s->sc.count - 1) sw_changed(s, c, s->sc.index + 1); return 1; }
    return 0;
}
static int sw_pointer(UgWidget *w, UgCtx *c, const UgPtr *p) {
    UgScrollerWidget *s = (UgScrollerWidget *)w;
    if (p->type == UG_PT_WHEEL) {
        int ni = s->sc.index + (p->wheel > 0 ? -1 : 1);
        if (ni >= 0 && ni < s->sc.count) sw_changed(s, c, ni);
        return 1;
    }
    int ni = s->sc.index;
    int r = ug_scroller_pointer(&s->sc, p, &ni);
    if (r == 2) sw_changed(s, c, ni);
    return r != 0;
}
static void sw_hints(UgWidget *w, UgHintSet *h) {
    (void)w;
    ug_hints_add(h, UG_A_INFO_WHEEL, "Scroll", 75);
    ug_hints_add(h, UG_A_INFO_DRAG, "Drag Thumb", 74);
}
static void sw_destroy(UgWidget *w) { ug_scroller_free(&((UgScrollerWidget *)w)->sc); }
static const UgWidgetVT SW_VT = {sw_update, sw_action, sw_pointer, sw_draw, NULL, sw_hints, sw_destroy};

void ug_scroller_widget_init(UgScrollerWidget *w, int id, UgRect track, int vertical, int count) {
    memset(w, 0, sizeof *w);
    UgRect b = track;
    float pad = 40;
    if (vertical) { b.y -= pad; b.h += 2 * pad; b.x -= 12; b.w += 24; }
    else { b.x -= pad; b.w += 2 * pad; b.y -= 12; b.h += 24; }
    ug_widget_init(&w->w, &SW_VT, "scroller", id, b);
    w->w.flags |= UG_W_CAPTURE;
    ug_scroller_init(&w->sc, track, vertical, 1);
    ug_scroller_set(&w->sc, 0, count);
}

/* ------------------------------------------------------------------ */
/* Category switcher                                                   */
/* ------------------------------------------------------------------ */
int ug_switcher_step(UgSwitcher *s, UgCtx *c, int dir) {
    if (s->n < 2) return 0;
    int ni = s->index + dir;
    if (ni < 0 || ni >= s->n) {
        if (!s->wrap) { ug_cue(c, UG_CUE_WRONG); return 0; }
        ni = (ni + s->n) % s->n;
    }
    s->index = ni;
    ug_anim_play(dir < 0 ? &s->blink_l : &s->blink_r, UG_ANIM_BLINK, 0);
    ug_cue(c, dir < 0 ? UG_CUE_TRIGGER_L : UG_CUE_TRIGGER_R);
    ug_post(c, UG_EV_CHANGE, s->w.id, s->w.id, ni);
    return 1;
}

static int sw2_action(UgWidget *w, UgCtx *c, UgAction a) {
    UgSwitcher *s = (UgSwitcher *)w;
    if (a == UG_A_LTRIG || a == UG_A_LEFT) { ug_switcher_step(s, c, -1); return 1; }
    if (a == UG_A_RTRIG || a == UG_A_RIGHT) { ug_switcher_step(s, c, +1); return 1; }
    return 0;
}

static int sw2_pointer(UgWidget *w, UgCtx *c, const UgPtr *p) {
    UgSwitcher *s = (UgSwitcher *)w;
    float cx = w->bounds.x + w->bounds.w * 0.5f, cy = w->bounds.y + w->bounds.h * 0.5f;
    float hx = w->bounds.w * 0.5f - 10.0f;
    if (p->type == UG_PT_WHEEL) { ug_switcher_step(s, c, p->wheel > 0 ? -1 : 1); return 1; }
    if (p->type == UG_PT_DOWN && p->button == 1) {
        if (ug_rect_hit(UG_RECT_C(cx - hx, cy, 26, 26), p->x, p->y)) { ug_switcher_step(s, c, -1); return 1; }
        if (ug_rect_hit(UG_RECT_C(cx + hx, cy, 26, 26), p->x, p->y)) { ug_switcher_step(s, c, +1); return 1; }
        return 1;
    }
    return 0;
}

static void sw2_update(UgWidget *w, UgCtx *c, float dt) {
    (void)c;
    UgSwitcher *s = (UgSwitcher *)w;
    ug_anim_update(&s->blink_l, dt);
    ug_anim_update(&s->blink_r, dt);
}

static void sw2_draw(UgWidget *w, UgCtx *c, UgCanvas *cv) {
    UgSwitcher *s = (UgSwitcher *)w;
    float cx = w->bounds.x + w->bounds.w * 0.5f, cy = w->bounds.y + w->bounds.h * 0.5f;
    float hx = w->bounds.w * 0.5f - 10.0f;
    ug_kit_badge(cv, cx - hx, cy, s->blink_l.props.w[UG_P_SIZE], 0, ug_anim_color(&s->blink_l), 0);
    ug_kit_badge(cv, cx + hx, cy, s->blink_r.props.w[UG_P_SIZE], 1, ug_anim_color(&s->blink_r), 0);
    UgColor col = w->focused ? UG_ACCENT : s->light?UG_TEXT_DARK:UG_TEXT_LIGHT;
    ug_kit_text(c, cv, s->cats[s->index], cx + s->text_dx, cy, s->light?0.7f:0.5f, col, UG_J_CENTER | UG_J_VCENTER);
}

static void sw2_hints(UgWidget *w, UgHintSet *h) {
    (void)w;
    ug_hints_add(h, UG_A_INFO_TABS, "Change Category", 72);
}
static void sw2_destroy(UgWidget *w) {
    UgSwitcher *s = (UgSwitcher *)w;
    ug_anim_free(&s->blink_l); ug_anim_free(&s->blink_r);
}
static const UgWidgetVT SWITCHER_VT = {sw2_update, sw2_action, sw2_pointer, sw2_draw, NULL, sw2_hints, sw2_destroy};

void ug_switcher_init(UgSwitcher *s, int id, float cx, float cy, float width,
                      const char *const *cats, int n) {
    memset(s, 0, sizeof *s);
    ug_widget_init(&s->w, &SWITCHER_VT, "switcher", id, UG_RECT_C(cx, cy, width, 26));
    s->n = n > UG_MAX_CATS ? UG_MAX_CATS : n;
    for (int i = 0; i < s->n; i++) snprintf(s->cats[i], sizeof s->cats[i], "%s", cats[i]);
    s->wrap = 1;
    ug_anim_init(&s->blink_l, UG_RGBA(255, 255, 255, 150), 20, 20);
    ug_anim_init(&s->blink_r, UG_RGBA(255, 255, 255, 150), 20, 20);
}

/* ------------------------------------------------------------------ */
/* Stat bars                                                           */
/* ------------------------------------------------------------------ */
static void sb_update(UgWidget *w, UgCtx *c, float dt) {
    (void)c;
    UgStatBars *s = (UgStatBars *)w;
    for (int i = 0; i < s->n; i++) {
        if (s->t[i] < UG_TUNE_STAT_ANIM_MS) {
            s->t[i] += dt;
            float f = clampf(s->t[i] / UG_TUNE_STAT_ANIM_MS, 0, 1);
            s->shown[i] = s->from[i] + (s->target[i] - s->from[i]) * f;
        } else s->shown[i] = s->target[i];
    }
}

/* Stat bar geometry. Original layout (SPEC 6.11 amendment 3, R): labels upper-case at scale 0.55 with
 * ink left edges x -280 / -77 / +121 and text centre y 162.5 (caps y 157..168); green fills of height 6
 * centred on y 176 starting at x -284 / -82 / +116 (about 4 units left of their label). The generic
 * (relative) layout of earlier rounds is kept for the demo's Controls page. */
static const float STAT_LABEL_X[3] = {-280.5f, -77.5f, 120.5f};
static const float STAT_FILL_X0[3] = {-284.0f, -82.0f, 116.0f};

void ug_statbars_geom(const UgStatBars *s, int i, UgStatBarGeom *g) {
    if (s->absolute) {
        g->label_x = STAT_LABEL_X[i]; g->label_y = UG_R_STAT_LABEL_Y;
        g->fill_x0 = STAT_FILL_X0[i]; g->fill_y = UG_R_STAT_FILL_Y;
        g->fill_len = UG_R_STAT_TRACK_LEN; g->fill_h = UG_R_STAT_FILL_H;
        return;
    }
    float cx = s->w.bounds.x + s->w.bounds.w * 0.5f, cy = s->w.bounds.y + s->w.bounds.h * 0.5f;
    float bx = cx + ((float)i - (float)(s->n - 1) * 0.5f) * s->spacing;
    g->label_x = bx; g->label_y = cy - 22;
    g->fill_x0 = bx - 128.0f + 42.0f; g->fill_y = cy + 8; g->fill_len = 256.0f - 84.0f; g->fill_h = 6;
}

static void sb_draw(UgWidget *w, UgCtx *c, UgCanvas *cv) {
    UgStatBars *s = (UgStatBars *)w;
    for (int i = 0; i < s->n; i++) {
        UgStatBarGeom g;
        ug_statbars_geom(s, i, &g);
        /* SPEC 6.11 (amendment 2): 256 x 32 backing at alpha 35; the 256-wide backings really overlap at
         * 200 spacing. The fill starts at its end cap, 42 units from the backing's left edge. */
        UgRect tr = UG_RECT_C(g.fill_x0 - 42.0f + 128.0f, g.fill_y, 256.0f, 32);
        if (s->absolute) ug_kit_text(c, cv, s->labels[i], g.label_x, g.label_y, UG_R_STAT_LABEL_SCALE, UG_TEXT_HINT, UG_J_VCENTER);
        else ug_kit_text(c, cv, s->labels[i], g.label_x, g.label_y, 0.55f, UG_TEXT_DARK, UG_J_CENTER | UG_J_VCENTER);
        const UgSkin *sk = ug_kit_skin();
        if (sk && sk->slider_bg) {
            UgRect bt = tr;
            bt.y += sk->slider_bg_dy * tr.h;
            ug_cv_image_rect(cv, bt, sk->slider_bg, UG_RGBA(255, 255, 255, 35));
        }

        float x0 = g.fill_x0, vx = x0 + g.fill_len * clampf(s->shown[i], 0, 1), y = g.fill_y;
        if (sk && sk->white && sk->slider_fill_corner && sk->slider_knob) {
            ug_cv_image_rect(cv, (UgRect){x0, y - g.fill_h * 0.5f, vx - x0, g.fill_h}, sk->white, UG_GREEN);
            ug_cv_image_rect(cv, UG_RECT_C(x0, y, 15, 15), sk->slider_fill_corner, UG_GREEN);
            ug_cv_image_rect(cv, UG_RECT_C(vx, y, 15, 15), sk->slider_fill_corner, UG_GREEN);
            ug_cv_image_rect(cv, UG_RECT_C(vx, y, 16, 16), sk->slider_knob, UG_WHITE);
        }
    }
}
static const UgWidgetVT STATBARS_VT = {sb_update, NULL, NULL, sb_draw, NULL, NULL, NULL};

void ug_statbars_init(UgStatBars *s, int id, float cx, float cy, const char *const *labels, int n) {
    memset(s, 0, sizeof *s);
    ug_widget_init(&s->w, &STATBARS_VT, "statbars", id, UG_RECT_C(cx, cy, 600, 60));
    s->w.flags = UG_W_NOFOCUS;
    s->n = n > 3 ? 3 : n;
    s->spacing = 200;
    for (int i = 0; i < s->n; i++) { s->labels[i] = labels[i]; s->t[i] = UG_TUNE_STAT_ANIM_MS; }
}

void ug_statbars_init_measured(UgStatBars *s, int id, const char *const *labels, int n) {
    ug_statbars_init(s, id, 0, UG_R_STAT_FILL_Y - 8.0f, labels, n);
    s->absolute = 1;
    s->w.bounds = (UgRect){-300, 150, 600, 40};
}

void ug_statbars_set(UgStatBars *s, const float *v) {
    for (int i = 0; i < s->n; i++) {
        if (v[i] == s->target[i]) continue;
        s->from[i] = s->shown[i];
        s->target[i] = v[i];
        s->t[i] = 0;
    }
}

/* ------------------------------------------------------------------ */
/* Grid picker                                                         */
/* ------------------------------------------------------------------ */
void ug_grid_select(UgGrid *g, int idx) {
    int n = g->cols * g->rows;
    if (idx < 0 || idx >= n || idx == g->sel) return;
    int old = g->sel;
    g->start[old] = g->size[old]; g->t[old] = 0;
    g->start[idx] = g->size[idx]; g->t[idx] = 0;
    g->sel = idx;
}

static void grid_center(const UgGrid *g, int i, float *x, float *y) {
    *x = g->px + (float)(i % g->cols) * g->pitch;
    *y = g->py + (float)(i / g->cols) * g->pitch;
}

static void grid_update(UgWidget *w, UgCtx *c, float dt) {
    (void)c;
    UgGrid *g = (UgGrid *)w;
    int n = g->cols * g->rows;
    for (int i = 0; i < n; i++) {
        float target = (i == g->sel) ? 24.0f : 16.0f, dur = (i == g->sel) ? 140.0f : 300.0f;
        if (g->t[i] < dur) {
            g->t[i] += dt;
            g->size[i] = g->start[i] + (target - g->start[i]) * clampf(g->t[i] / dur, 0, 1);
        } else g->size[i] = target;
    }
    float tx, ty;
    grid_center(g, g->sel, &tx, &ty);
    float k = 1.0f - powf(0.001f, dt / 400.0f);
    g->hx += (tx - g->hx) * k;
    g->hy += (ty - g->hy) * k;
    ug_anim_update(&g->breathe, dt);
}

static int grid_move(UgGrid *g, UgCtx *c, int dx, int dy) {
    int col = g->sel % g->cols, row = g->sel / g->cols;
    int nc = col + dx, nr = row + dy;
    if (nc < 0 || nc >= g->cols || nr < 0 || nr >= g->rows) return 0;
    ug_grid_select(g, nr * g->cols + nc);
    ug_cue(c, UG_CUE_BROWSER);
    return 1;
}

static int grid_action(UgWidget *w, UgCtx *c, UgAction a) {
    UgGrid *g = (UgGrid *)w;
    switch (a) {
    case UG_A_LEFT:  return grid_move(g, c, -1, 0);
    case UG_A_RIGHT: return grid_move(g, c, 1, 0);
    case UG_A_UP:    return grid_move(g, c, 0, -1);
    case UG_A_DOWN:  return grid_move(g, c, 0, 1);
    case UG_A_ACCEPT:
        ug_cue(c, UG_CUE_SELECT);
        ug_post(c, UG_EV_ACTIVATE, w->id, w->id, g->sel);
        return 1;
    default: return 0;
    }
}

static int grid_hit(const UgGrid *g, float x, float y) {
    int n = g->cols * g->rows;
    for (int i = 0; i < n; i++) {
        float cx, cy;
        grid_center(g, i, &cx, &cy);
        if (fabsf(x - cx) <= g->pitch * 0.5f && fabsf(y - cy) <= g->pitch * 0.5f) return i;
    }
    return -1;
}

static int grid_pointer(UgWidget *w, UgCtx *c, const UgPtr *p) {
    UgGrid *g = (UgGrid *)w;
    int i = grid_hit(g, p->x, p->y);
    if (p->type == UG_PT_MOVE && i >= 0) {
        if (i != g->sel) ug_cue(c, UG_CUE_BROWSER);
        ug_grid_select(g, i);
        return 1;
    }
    if (p->type == UG_PT_DOWN && p->button == 1 && i >= 0) {
        ug_grid_select(g, i);
        ug_cue(c, UG_CUE_SELECT);
        ug_post(c, UG_EV_ACTIVATE, w->id, w->id, i);
        return 1;
    }
    return 0;
}

static void grid_draw(UgWidget *w, UgCtx *c, UgCanvas *cv) {
    (void)c;
    UgGrid *g = (UgGrid *)w;
    int n = g->cols * g->rows;
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < n; i++) {
            if ((i == g->sel) != (pass == 1)) continue;
            float cx, cy;
            grid_center(g, i, &cx, &cy);
            float s = g->size[i] * UG_TUNE_GRID_SCALE;      /* 16x16 discs inside a group scaled x2 (6.13) */
            ug_cv_fill(cv, UG_RECT_C(cx, cy, s, s), g->colors[i], s * 0.22f);
            ug_cv_outline(cv, UG_RECT_C(cx, cy, s, s), UG_RGBA(0, 0, 0, 100), s * 0.22f, 1.0f);
        }
    }
    UgColor hc = ug_anim_color(&g->breathe);
    if (!w->focused) hc.a = (uint8_t)(hc.a * 0.4f);
    UgRect hr = UG_RECT_C(g->hx, g->hy, 24.0f * UG_TUNE_GRID_SCALE + 4.0f, 24.0f * UG_TUNE_GRID_SCALE + 4.0f);
    const UgSkin *sk=ug_kit_skin();
    if(sk&&sk->icon_hl)ug_cv_image_rect(cv,hr,sk->icon_hl,hc);else cv->visual_status=UG_VIS_UNAVAILABLE;
}

static void grid_focus(UgWidget *w, UgCtx *c, int gained) {
    (void)c;
    UgGrid *g = (UgGrid *)w;
    if (gained) ug_anim_play(&g->breathe, UG_ANIM_BREATHE, 0); else ug_anim_play(&g->breathe, UG_ANIM_BREATHE, 0);
}
static void grid_hints(UgWidget *w, UgHintSet *h) {
    (void)w;
    ug_hints_add(h, UG_A_ACCEPT, "Select Colour", 90);
}
static void grid_destroy(UgWidget *w) {
    UgGrid *g = (UgGrid *)w;
    ug_anim_free(&g->breathe);
    free(g->colors);
    g->colors = NULL;
}
static const UgWidgetVT GRID_VT = {grid_update, grid_action, grid_pointer, grid_draw, grid_focus, grid_hints, grid_destroy};

static UgColor hsv(float h, float s, float v) {
    float r = 0, g = 0, b = 0;
    int i = (int)floorf(h * 6.0f);
    float f = h * 6.0f - (float)i, p = v * (1 - s), q = v * (1 - f * s), t = v * (1 - (1 - f) * s);
    switch (i % 6) {
    case 0: r = v; g = t; b = p; break; case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break; case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break; default: r = v; g = p; b = q; break;
    }
    return UG_RGBA(ug_clamp8(r * 255), ug_clamp8(g * 255), ug_clamp8(b * 255), 255);
}

void ug_grid_init(UgGrid *g, int id, float cx, float cy, int cols, int rows, float pitch) {
    memset(g, 0, sizeof *g);
    if (cols * rows > 64) rows = 64 / cols;
    g->cols = cols; g->rows = rows; g->pitch = pitch;
    ug_widget_init(&g->w, &GRID_VT, "grid", id,
                   UG_RECT_C(cx, cy, (float)cols * pitch + 14, (float)rows * pitch + 14));
    g->px = cx - ((float)cols - 1) * pitch * 0.5f;
    g->py = cy - ((float)rows - 1) * pitch * 0.5f;
    g->colors = (UgColor *)malloc(sizeof(UgColor) * (size_t)(cols * rows));
    for (int i = 0; i < cols * rows; i++) {
        int col = i % cols, row = i / cols;
        g->colors[i] = row == rows - 1 ? hsv(0, 0, (float)col / (float)(cols - 1 > 0 ? cols - 1 : 1))
                                       : hsv((float)col / (float)cols, 0.85f - 0.18f * (float)row, 0.95f);
        g->size[i] = 16; g->start[i] = 16; g->t[i] = 1000;
    }
    g->hx = g->px; g->hy = g->py;
    ug_anim_init(&g->breathe, UG_ACCENT, 0, 0);
    ug_anim_play(&g->breathe, UG_ANIM_BREATHE, 0);
    g->size[0] = 24; g->start[0] = 24;
}

/* ------------------------------------------------------------------ */
/* Busy indicators                                                     */
/* ------------------------------------------------------------------ */
static void busy_update(UgWidget *w, UgCtx *c, float dt) {
    (void)c;
    UgBusy *b = (UgBusy *)w;
    for (int i = 0; i < 3; i++) ug_anim_update(&b->arrows[i], dt);
    b->spin += dt / 1000.0f;           /* 1 revolution per second */
    if (b->spin > 1000.0f) b->spin -= 1000.0f;
}
static void busy_draw(UgWidget *w, UgCtx *c, UgCanvas *cv) {
    (void)c;
    UgBusy *b = (UgBusy *)w;
    float cx = w->bounds.x + w->bounds.w * 0.5f, cy = w->bounds.y + w->bounds.h * 0.5f;
    if (b->show_arrows)
        for (int i = 0; i < 3; i++) ug_kit_badge(cv, cx - 36 + 36.0f * (float)i, cy, 28, 1, ug_anim_color(&b->arrows[i]), 1);
    if (b->show_spinner) ug_cv_spinner(cv, cx + 90, cy, 14, 4.5f, b->spin - floorf(b->spin), UG_ACCENT);
}
static void busy_destroy(UgWidget *w) {
    UgBusy *b = (UgBusy *)w;
    for (int i = 0; i < 3; i++) ug_anim_free(&b->arrows[i]);
}
static const UgWidgetVT BUSY_VT = {busy_update, NULL, NULL, busy_draw, NULL, NULL, busy_destroy};

void ug_busy_init(UgBusy *b, int id, float cx, float cy) {
    memset(b, 0, sizeof *b);
    ug_widget_init(&b->w, &BUSY_VT, "busy", id, UG_RECT_C(cx, cy, 200, 36));
    b->w.flags = UG_W_NOFOCUS;
    b->show_arrows = b->show_spinner = 1;
    for (int i = 0; i < 3; i++) {
        ug_anim_init(&b->arrows[i], UG_WHITE, 28, 28);
        ug_anim_play(&b->arrows[i], UG_ANIM_ACTIVITY, i);
    }
}

/* ------------------------------------------------------------------ */
/* Movie placeholder                                                   */
/* ------------------------------------------------------------------ */
void ug_movie_draw(UgCanvas *cv, UgCtx *c, UgRect r, const char *label) {
    (void)cv; (void)c; (void)r; (void)label; /* unsupported movie resource: omitted */
}
