/* ugui_w_list.c -- icon scroller (SPEC 6.4), option list (6.5), table (6.16)
 * and car-select carousel (6.17). */
#include "ugui_ui.h"
#include "ugui_tuning.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

/* ================================================================== */
/* Icon scroller (SPEC 6.4, amendment 3)                               */
/* ================================================================== */
#define ICON_SPACING UG_TUNE_ICON_SPACING

static float lerpf(float a, float b, float t) { return a + (b - a) * t; }

float ug_icons_thumb_xf(int n, float index) {
    float f = n > 1 ? index / (float)(n - 1) : 0.0f;
    return UG_R_THUMB_X0 + UG_R_THUMB_SPAN * f;
}
float ug_icons_thumb_x(int n, int index) { return ug_icons_thumb_xf(n, (float)index); }

void ug_icon_place(float pos, int item, float slot_cx, float slot_cy, UgIconPlace *o) {
    float d = (float)item - pos, ad = fabsf(d), sgn = d < 0 ? -1.0f : 1.0f;
    float t = ad < 1.0f ? ad : 1.0f;
    o->item = item;
    o->cy = slot_cy;
    o->cx = slot_cx + sgn * ICON_SPACING * ad;
    o->w = lerpf(UG_R_ICON_SEL_W, UG_R_ICON_NB_W, t);
    o->h = lerpf(UG_R_ICON_SEL_H, UG_R_ICON_NB_H, t);
    o->alpha = ad <= 1.0f ? 1.0f : clampf(1.0f - (ad - 1.0f) / UG_TUNE_ICON_FADE_DIST, 0, 1);
}

void ug_icons_layout(int n, int sel, UgIconLayout *o) {
    memset(o, 0, sizeof *o);
    UgIconPlace p;
    ug_icon_place((float)sel, sel, 0, UG_R_ICON_CY, &p);
    o->selected = UG_RECT_C(p.cx, p.cy, p.w, p.h);
    for (int side = -1; side <= 1; side += 2) {
        int it = sel + side;
        if (it < 0 || it >= n) continue;
        ug_icon_place((float)sel, it, 0, UG_R_ICON_CY, &p);
        o->nb_item[o->nnb] = it;
        o->nb[o->nnb] = UG_RECT_C(p.cx, p.cy, p.w, p.h);
        o->nnb++;
    }
    o->frame = UG_RECT_C(0, (UG_R_FRAME_TOP + UG_R_FRAME_BOT) * 0.5f, 2.0f * UG_R_FRAME_HW, UG_R_FRAME_BOT - UG_R_FRAME_TOP);
    o->arrow_y = UG_R_ARROW_CY;
    o->arrow_lx = -UG_R_ARROW_CX;
    o->arrow_rx = UG_R_ARROW_CX;
    o->arrow_l = sel > 0;                 /* an arrow is shown only when an item exists on that side (4.5) */
    o->arrow_r = sel < n - 1;
    o->band = (UgRect){UG_R_BAND_X0, UG_R_BAND_Y0, UG_R_BAND_X1 - UG_R_BAND_X0, UG_R_BAND_Y1 - UG_R_BAND_Y0};
    o->track = (UgRect){UG_R_BAND_X0, UG_R_TRACK_Y0, UG_R_BAND_X1 - UG_R_BAND_X0, UG_R_TRACK_Y1 - UG_R_TRACK_Y0};
    o->thumb_x = ug_icons_thumb_x(n, sel);
    o->thumb_y = UG_R_THUMB_Y;
    /* item title: cap top y -95 at scale 0.7; capitals start about 6 units into the cell and the text is
     * centred vertically, so the text centre lies 10 x scale below the cap top (16.5) */
    o->title_y = UG_R_ITEM_TITLE_CAPTOP + 10.0f * UG_R_ITEM_TITLE_SCALE;
}

static void icons_sync_arrows(UgIconScroller *s) {
    s->sb.hide_a = s->sel <= 0;
    s->sb.hide_b = s->sel >= s->n - 1;
}

void ug_icons_select(UgIconScroller *s, int idx) {
    if (idx < 0 || idx >= s->n) return;
    s->sel = idx;
    ug_scroller_set(&s->sb, idx, s->n);
    icons_sync_arrows(s);
}

int ug_icons_add(UgIconScroller *s, const char *label, int symbol, int locked, int id) {
    if (s->n >= UG_MAX_ICONS) return -1;
    UgIconItem *it = &s->items[s->n];
    memset(it, 0, sizeof *it);
    snprintf(it->label, sizeof it->label, "%s", label);
    it->symbol = symbol; it->locked = locked; it->id = id;
    s->n++;
    s->wrap = UG_TUNE_ICON_WRAP_IF_MORE && s->n > UG_TUNE_ICON_VISIBLE;
    ug_scroller_set(&s->sb, s->sel, s->n);
    icons_sync_arrows(s);
    return s->n - 1;
}

static float wrap_diff(float d, int n) {
    float h = (float)n * 0.5f;
    while (d >= h) d -= (float)n;
    while (d < -h) d += (float)n;
    return d;
}

static void icons_set_sel(UgIconScroller *s, UgCtx *c, int idx, int dir) {
    if (idx == s->sel) return;
    s->sel = idx;
    ug_scroller_set(&s->sb, idx, s->n);
    icons_sync_arrows(s);
    if (dir) ug_anim_play(dir < 0 ? &s->blink_l : &s->blink_r, UG_ANIM_BLINK, 0);
    ug_cue(c, UG_CUE_ICON_SCROLLER);
    ug_post(c, UG_EV_CHANGE, s->w.id, s->items[idx].id, idx);
}

static int icons_step(UgIconScroller *s, UgCtx *c, int dir) {
    if (s->n < 2) return 0;
    int ni = s->sel + dir;
    if (ni < 0 || ni >= s->n) {
        /* no wrap (4.5, R): nothing is drawn beyond the last item, so the press is ignored */
        if (!s->wrap) return 1;
        ni = (ni + s->n) % s->n;
    }
    icons_set_sel(s, c, ni, dir);
    return 1;
}

static void icons_activate(UgIconScroller *s, UgCtx *c) {
    if (s->n == 0) return;
    if (s->items[s->sel].locked) {
        ug_cue(c, UG_CUE_WRONG);
        ug_anim_play(&s->wrong, UG_ANIM_BLINK, 0);
        return;
    }
    ug_cue(c, UG_CUE_SELECT);
    ug_post(c, UG_EV_ACTIVATE, s->w.id, s->items[s->sel].id, s->sel);
}

static void icons_update(UgWidget *w, UgCtx *c, float dt) {
    (void)c;
    UgIconScroller *s = (UgIconScroller *)w;
    float d = (float)s->sel - s->pos;
    if (s->wrap) d = wrap_diff(d, s->n);
    float k = 1.0f - expf(-dt / 70.0f);
    s->pos += d * k;
    if (fabsf((float)s->sel - s->pos) < 0.002f && !s->wrap) s->pos = (float)s->sel;
    if (s->wrap) { while (s->pos < 0) s->pos += (float)s->n; while (s->pos >= (float)s->n) s->pos -= (float)s->n; }
    ug_anim_update(&s->breathe, dt);
    ug_anim_update(&s->blink_l, dt);
    ug_anim_update(&s->blink_r, dt);
    ug_anim_update(&s->wrong, dt);
    ug_scroller_update(&s->sb, dt);
}

static int icons_action(UgWidget *w, UgCtx *c, UgAction a) {
    UgIconScroller *s = (UgIconScroller *)w;
    switch (a) {
    case UG_A_LEFT:  return icons_step(s, c, -1);
    case UG_A_RIGHT: return icons_step(s, c, +1);
    case UG_A_ACCEPT: icons_activate(s, c); return 1;
    default: return 0;
    }
}

/* position the strip must have for item i to lie wrap_diff() slots from the centre (wrap is off unless forced
 * on through UG_TUNE_ICON_WRAP_IF_MORE) */
static float icon_pos_for(const UgIconScroller *s, int i) {
    return s->wrap ? (float)i - wrap_diff((float)i - s->pos, s->n) : s->pos;
}

static int icon_hit(const UgIconScroller *s, float x, float y) {
    for (int i = 0; i < s->n; i++) {
        UgIconPlace p;
        ug_icon_place(icon_pos_for(s, i), i, 0, UG_R_ICON_CY, &p);
        if (p.alpha < 0.5f) continue;
        if (ug_rect_hit(UG_RECT_C(p.cx, p.cy, p.w, p.h), x, y)) return i;
    }
    return -1;
}

static int icons_pointer(UgWidget *w, UgCtx *c, const UgPtr *p) {
    UgIconScroller *s = (UgIconScroller *)w;
    if (p->type == UG_PT_WHEEL) { icons_step(s, c, p->wheel > 0 ? -1 : 1); return 1; }
    int ni = s->sel;
    int r = ug_scroller_pointer(&s->sb, p, &ni);
    if (r == 2) { icons_set_sel(s, c, ni, ni > s->sel ? 1 : -1); return 1; }
    if (r == 1) return 1;
    if (p->type == UG_PT_MOVE) {
        int i = icon_hit(s, p->x, p->y);
        if (i >= 0 && i != s->sel && (fabsf(p->x - s->last_px) > 0.5f)) {
            icons_set_sel(s, c, i, i < s->sel ? -1 : 1);
        }
        s->last_px = p->x;
        return 0;
    }
    if (p->type == UG_PT_DOWN && p->button == 1) {
        UgIconLayout L;
        ug_icons_layout(s->n, s->sel, &L);
        if (L.arrow_l && ug_rect_hit(UG_RECT_C(L.arrow_lx, L.arrow_y, 34, 34), p->x, p->y)) { icons_step(s, c, -1); return 1; }
        if (L.arrow_r && ug_rect_hit(UG_RECT_C(L.arrow_rx, L.arrow_y, 34, 34), p->x, p->y)) { icons_step(s, c, +1); return 1; }
        int i = icon_hit(s, p->x, p->y);
        if (i >= 0) {
            if (i != s->sel) icons_set_sel(s, c, i, 0);
            icons_activate(s, c);
            return 1;
        }
    }
    return 0;
}

static void icon_draw_one(UgIconScroller *s, UgCanvas *cv, int i, const UgIconPlace *p, int nearest) {
    const UgIconItem *it = &s->items[i];
    ug_cv_push(cv);
    ug_cv_mul_alpha(cv, p->alpha);
    UgRect r = UG_RECT_C(p->cx, p->cy, p->w, p->h);
    UgColor ic = it->locked ? UG_RGBA(150, 150, 150, 255) : UG_WHITE;
    if (it->tex > 0) ug_cv_image_rect(cv, r, it->tex, ic);
    else ug_kit_symbol(cv, it->symbol, p->cx, p->cy - 2, 38.0f * p->w / UG_R_ICON_SEL_W, ic);
    if (it->locked) {
        float grow = (nearest && s->wrong.has_script) ? s->wrong.props.w[UG_P_SIZE] - 24 : 0;
        ug_kit_lock(cv, p->cx + 38.0f * p->w / UG_R_ICON_SEL_W, p->cy + 16, 24 + grow, UG_ACCENT);
    }
    ug_cv_pop(cv);
}

static void icons_draw(UgWidget *w, UgCtx *c, UgCanvas *cv) {
    UgIconScroller *s = (UgIconScroller *)w;
    UgIconLayout L;
    ug_icons_layout(s->n, s->sel, &L);
    ug_kit_panel(cv, w->bounds, 0, 0.95f);
    /* light icon band (x -290..+290, y -165.5..-103.3) */
    ug_kit_box(cv, L.band, 1, 0.78f);
    /* screen title: accent, scale 0.9, right aligned at the top right (R) */
    ug_kit_text(c, cv, s->title, UG_R_SCR_TITLE_RIGHT + 0.8f, UG_R_SCR_TITLE_CAPTOP + 10.0f * UG_R_SCR_TITLE_SCALE,
                UG_R_SCR_TITLE_SCALE, UG_ACCENT, UG_J_RIGHT | UG_J_VCENTER);
    /* scroller: the visible track (8 thick), thumb on the 6.4 formula, arrows only when an item exists on that side */
    ug_scroller_draw(&s->sb, cv, 1.0f);
    /* dark backing at the slot (118 x 59), then the icons, then the frame */
    UgRect slot = UG_RECT_C(0, UG_R_ICON_CY, UG_R_ICON_SEL_W, UG_R_ICON_SEL_H);
    const UgSkin *sk = ug_kit_skin();
    if (sk && sk->icon_back) ug_cv_image_rect(cv, slot, sk->icon_back, UG_RGBA(255, 255, 255, 215));
    else ug_cv_fill(cv, slot, UG_RGBA(0, 0, 0, 215), 10);
    /* far to near: the icon nearest the centre is drawn last */
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < s->n; i++) {
            UgIconPlace p;
            ug_icon_place(icon_pos_for(s, i), i, 0, UG_R_ICON_CY, &p);
            int nearest = fabsf((float)i - icon_pos_for(s, i)) < 0.5f;
            if (p.alpha <= 0.0f || nearest != (pass == 1)) continue;
            icon_draw_one(s, cv, i, &p, nearest);
        }
    }
    float foc = w->focused ? 1.0f : 0.45f;
    UgColor hc = ug_anim_color(&s->breathe);
    if (sk && sk->icon_hl)
        ug_cv_image_rect(cv, UG_RECT_C(0, UG_R_ICON_CY, 2.0f * UG_R_FRAME_HW + 6.0f, UG_R_FRAME_BOT - UG_R_FRAME_TOP + 7.0f),
                         sk->icon_hl, ug_color_alpha(hc, foc));
    else {
        /* line centres on the measured rectangle (x +-60, y -161 / -106), about 2 thick, glow about 3 outside */
        UgRect f = L.frame;
        ug_cv_softring(cv, (UgRect){f.x - 1.5f, f.y - 1.5f, f.w + 3, f.h + 3}, ug_color_alpha(hc, foc * 0.55f), 8.0f, 3.0f, 2.2f);
        ug_cv_outline_centered(cv, f, ug_color_alpha(hc, foc), 6.0f, UG_R_FRAME_LINE);
    }
    if (L.arrow_l) ug_kit_badge(cv, L.arrow_lx, L.arrow_y, s->blink_l.props.w[UG_P_SIZE], 0, ug_anim_color(&s->blink_l), 0);
    if (L.arrow_r) ug_kit_badge(cv, L.arrow_rx, L.arrow_y, s->blink_r.props.w[UG_P_SIZE], 1, ug_anim_color(&s->blink_r), 0);
    if (s->n) {
        ug_kit_text(c, cv, s->items[s->sel].label, 0, L.title_y, UG_R_ITEM_TITLE_SCALE, UG_ACCENT, UG_J_CENTER | UG_J_VCENTER);
        if (s->items[s->sel].locked)
            ug_kit_text(c, cv, "Locked", 0, L.title_y + 16, 0.45f, UG_TEXT_HINT, UG_J_CENTER | UG_J_VCENTER);
    }
}

static void icons_hints(UgWidget *w, UgHintSet *h) {
    UgIconScroller *s = (UgIconScroller *)w;
    ug_hints_add(h, UG_A_INFO_WHEEL, "Browse", 75);
    if (s->n && !s->items[s->sel].locked) ug_hints_add(h, UG_A_ACCEPT, "Select", 90);
}

static void icons_destroy(UgWidget *w) {
    UgIconScroller *s = (UgIconScroller *)w;
    ug_anim_free(&s->breathe); ug_anim_free(&s->blink_l); ug_anim_free(&s->blink_r);
    ug_anim_free(&s->wrong);
    ug_scroller_free(&s->sb);
}
static const UgWidgetVT ICONS_VT = {icons_update, icons_action, icons_pointer, icons_draw, NULL, icons_hints, icons_destroy};

void ug_icons_init(UgIconScroller *s, int id, const char *title) {
    memset(s, 0, sizeof *s);
    ug_widget_init(&s->w, &ICONS_VT, "icons", id, UG_RECT_C(0, -146, 594, 152));      /* 562 + the 16-wide strips: x -297..297 */
    s->w.flags |= UG_W_CAPTURE;
    snprintf(s->title, sizeof s->title, "%s", title);
    ug_scroller_init(&s->sb, UG_RECT_C(0, (UG_R_TRACK_Y0 + UG_R_TRACK_Y1) * 0.5f, UG_R_BAND_X1 - UG_R_BAND_X0, UG_R_TRACK_Y1 - UG_R_TRACK_Y0), 0, 0);
    s->sb.has_range = 1; s->sb.lo = UG_R_THUMB_X0; s->sb.hi = UG_R_THUMB_X0 + UG_R_THUMB_SPAN;   /* 6.4 thumb formula */
    s->sb.has_cross = 1; s->sb.cross = UG_R_THUMB_Y;
    ug_anim_init(&s->breathe, UG_ACCENT, 0, 0);
    ug_anim_play(&s->breathe, UG_ANIM_BREATHE, 0);
    ug_anim_init(&s->blink_l, UG_RGBA(255, 255, 255, 150), 26, 26);
    ug_anim_init(&s->blink_r, UG_RGBA(255, 255, 255, 150), 26, 26);
    ug_anim_init(&s->wrong, UG_WHITE, 24, 24);
    icons_sync_arrows(s);
}

/* ================================================================== */
/* Option list (SPEC 6.5, 6.6, amendment 3)                            */
/* ================================================================== */
#define OL_PITCH  UG_TUNE_ROW_PITCH

void ug_options_row_geom(const UgOptionList *o, int k, UgOptionRowGeom *g) {
    float dy = OL_PITCH * (float)k;
    memset(g, 0, sizeof *g);
    if (o->style == UG_OL_CONTROLS) {       /* PC controls page (checklist 27): scale 0.5, 24.9-high backing */
        g->backing = (UgRect){UG_R_ROW_BACK_X0, UG_R_CTL_BACK_Y0 + dy, UG_R_ROW_BACK_X1 - UG_R_ROW_BACK_X0, UG_R_CTL_BACK_H};
        g->name_x = UG_R_CTL_NAME_X;
        g->name_scale = g->value_scale = 0.5f;
        g->text_cy = g->backing.y + g->backing.h * 0.5f + 1.0f;      /* the text sits 1 below the backing centre, as on the audio page */
    } else {                                 /* plain options page (checklist 20-25) */
        g->backing = (UgRect){UG_R_ROW_BACK_X0, UG_R_ROW_BACK_Y0 + dy, UG_R_ROW_BACK_X1 - UG_R_ROW_BACK_X0, UG_R_ROW_BACK_H};
        g->name_x = UG_R_ROW_NAME_X;
        g->name_scale = g->value_scale = 0.7f;
        g->text_cy = UG_R_ROW_TEXT_Y0 + dy;
    }
    g->value_cx = UG_R_ROW_VALUE_X;
    g->arrow_lx = UG_R_ROW_ARROW_LX;
    g->arrow_rx = UG_R_ROW_ARROW_RX;
    g->arrow_cy = g->backing.y + g->backing.h * 0.5f;
    g->slider_x0 = UG_R_SLIDER_X0;
    g->slider_x1 = UG_R_SLIDER_X1;
    g->slider_cy = g->arrow_cy;
    /* ug_slider_draw_ex keeps `margin` between the track end and the fill start / knob travel */
    g->slider_track = UG_RECT_C((UG_R_SLIDER_X0 + UG_R_SLIDER_X1) * 0.5f, g->slider_cy,
                                (UG_R_SLIDER_X1 - UG_R_SLIDER_X0) + 24.0f, 32.0f);
}

static float ol_row_y(const UgOptionList *o, int row) {
    UgOptionRowGeom g;
    ug_options_row_geom(o, row - o->first, &g);
    return g.text_cy;
}

float ug_options_thumb_y(const UgOptionList *o) { return ug_scroller_thumb_pos(&o->sb); }

static void ol_scroll_into_view(UgOptionList *o) {
    if (o->cur < o->first) o->first = o->cur;
    if (o->cur >= o->first + o->visible) o->first = o->cur - o->visible + 1;
    if (o->first < 0) o->first = 0;
    ug_scroller_set(&o->sb, o->cur, o->n);
}

static int ol_next(const UgOptionList *o, int from, int dir) {
    for (int i = from + dir; i >= 0 && i < o->n; i += dir)
        if (!o->rows[i].disabled) return i;
    return -1;
}

static void ol_changed(UgOptionList *o, UgCtx *c, int row) {
    UgOptionRow *r = &o->rows[row];
    int v = r->kind == UG_ROW_SLIDER ? (int)(r->slider * 1000.0f + 0.5f) : r->sel;
    ug_cue(c, r->kind == UG_ROW_SLIDER ? UG_CUE_SLIDER : UG_CUE_TOGGLE);
    ug_post(c, UG_EV_CHANGE, o->w.id, r->id, v);
}

static void ol_step_value(UgOptionList *o, UgCtx *c, int row, int dir) {
    UgOptionRow *r = &o->rows[row];
    if (r->kind == UG_ROW_SLIDER) {
        float n = floorf(r->slider * 10.0f + 0.5f) + (float)dir;
        r->slider = clampf(n / 10.0f, 0, 1);
    } else if (r->kind == UG_ROW_SELECTOR || r->kind == UG_ROW_TOGGLE) {
        if (r->nvalues < 1) return;
        if (UG_TUNE_SELECTOR_WRAP) r->sel = (r->sel + dir + r->nvalues) % r->nvalues;     /* selectors wrap (G) */
        else { int ns = r->sel + dir; if (ns < 0 || ns >= r->nvalues) return; r->sel = ns; }
    } else return;
    if (row == o->cur) ug_anim_play(dir < 0 ? &o->blink_l : &o->blink_r, UG_ANIM_BLINK, 0);
    ol_changed(o, c, row);
}

static void ol_activate(UgOptionList *o, UgCtx *c, int row) {
    UgOptionRow *r = &o->rows[row];
    if (r->kind == UG_ROW_ACTION) {
        ug_cue(c, UG_CUE_SELECT);
        ug_post(c, UG_EV_ACTIVATE, o->w.id, r->id, row);
    } else if (r->kind != UG_ROW_SLIDER) {
        ol_step_value(o, c, row, +1);
    }
}

static void ol_update(UgWidget *w, UgCtx *c, float dt) {
    (void)c;
    UgOptionList *o = (UgOptionList *)w;
    ug_anim_update(&o->pulse, dt);
    ug_anim_update(&o->blink_l, dt);
    ug_anim_update(&o->blink_r, dt);
    ug_scroller_update(&o->sb, dt);
    for (int i = 0; i < o->n; i++) {
        float target = (i == o->cur && w->focused) ? 200.0f : 80.0f;
        o->rows[i].backing += (target - o->rows[i].backing) * (1.0f - expf(-dt / 50.0f));
    }
}

static int ol_action(UgWidget *w, UgCtx *c, UgAction a) {
    UgOptionList *o = (UgOptionList *)w;
    if (o->n == 0) return 0;
    switch (a) {
    case UG_A_UP: case UG_A_DOWN: {
        int dir = a == UG_A_UP ? -1 : 1;
        int nx = ol_next(o, o->cur, dir);
        if (nx < 0 && UG_TUNE_LIST_WRAP)             /* wrap to the other end (G, off by default) */
            nx = ol_next(o, dir > 0 ? -1 : o->n, dir);
        if (nx < 0) return 0;
        o->cur = nx;
        ol_scroll_into_view(o);
        ug_anim_play(&o->pulse, UG_ANIM_FOCUS_PULSE, 0);
        ug_scroller_blink(&o->sb, dir > 0);
        ug_cue(c, dir < 0 ? UG_CUE_UP : UG_CUE_DOWN);
        return 1;
    }
    case UG_A_LEFT: case UG_A_RIGHT: {
        UgOptionRow *r = &o->rows[o->cur];
        if (r->kind == UG_ROW_ACTION) return 0;
        ol_step_value(o, c, o->cur, a == UG_A_LEFT ? -1 : 1);
        return 1;
    }
    case UG_A_ACCEPT:
        ol_activate(o, c, o->cur);
        return 1;
    default: return 0;
    }
}

static int ol_row_at(const UgOptionList *o, float x, float y) {
    for (int v = 0; v < o->visible; v++) {
        int row = o->first + v;
        if (row >= o->n) break;
        UgOptionRowGeom g;
        ug_options_row_geom(o, v, &g);
        UgRect r = {g.backing.x, g.arrow_cy - OL_PITCH * 0.5f, g.backing.w, OL_PITCH};
        if (ug_rect_hit(r, x, y)) return row;
    }
    return -1;
}

static void ol_drag_to(UgOptionList *o, UgCtx *c, int row, float x) {
    UgOptionRowGeom g;
    ug_options_row_geom(o, row - o->first, &g);
    float v = clampf((x - g.slider_x0) / (g.slider_x1 - g.slider_x0), 0, 1);
    if (v != o->rows[row].slider) {
        o->rows[row].slider = v;
        ol_changed(o, c, row);
    }
}

static int ol_pointer(UgWidget *w, UgCtx *c, const UgPtr *p) {
    UgOptionList *o = (UgOptionList *)w;
    if (o->drag_row >= 0) {
        if (p->type == UG_PT_MOVE) { ol_drag_to(o, c, o->drag_row, p->x); return 1; }
        if (p->type == UG_PT_UP) { o->drag_row = -1; return 1; }
    }
    int ni = o->cur;
    int r = ug_scroller_pointer(&o->sb, p, &ni);
    if (r == 2) {
        int t = ni;
        if (o->rows[t].disabled) { int nx = ol_next(o, t, ni >= o->cur ? 1 : -1); if (nx >= 0) t = nx; else return 1; }
        o->cur = t;
        ol_scroll_into_view(o);
        return 1;
    }
    if (r == 1) return 1;

    int row = ol_row_at(o, p->x, p->y);
    if (p->type == UG_PT_MOVE) {
        if (row >= 0 && !o->rows[row].disabled && row != o->cur) {
            o->cur = row;
            ug_anim_play(&o->pulse, UG_ANIM_FOCUS_PULSE, 0);
            ug_cue(c, UG_CUE_FOCUS);
        }
        return row >= 0;
    }
    if (p->type == UG_PT_WHEEL) {
        if (row >= 0 && !o->rows[row].disabled && p->x > 18.0f) {
            o->cur = row;
            ol_step_value(o, c, row, p->wheel > 0 ? 1 : -1);
        } else {
            int nx = ol_next(o, o->cur, p->wheel > 0 ? -1 : 1);
            if (nx >= 0) {
                o->cur = nx;
                ol_scroll_into_view(o);
                ug_anim_play(&o->pulse, UG_ANIM_FOCUS_PULSE, 0);
                ug_scroller_blink(&o->sb, p->wheel < 0);
                ug_cue(c, p->wheel > 0 ? UG_CUE_UP : UG_CUE_DOWN);
            }
        }
        return 1;
    }
    if (p->type == UG_PT_DOWN && p->button == 1 && row >= 0 && !o->rows[row].disabled) {
        UgOptionRow *rw = &o->rows[row];
        UgOptionRowGeom g;
        ug_options_row_geom(o, row - o->first, &g);
        o->cur = row;
        if (rw->kind == UG_ROW_SLIDER && p->x > g.slider_x0 - 12 && p->x < g.slider_x1 + 12) {
            o->drag_row = row;
            ol_drag_to(o, c, row, p->x);
            return 1;
        }
        if (rw->kind == UG_ROW_SELECTOR || rw->kind == UG_ROW_TOGGLE) {
            if (ug_rect_hit(UG_RECT_C(g.arrow_lx, g.arrow_cy, 28, 28), p->x, p->y)) { ol_step_value(o, c, row, -1); return 1; }
            if (ug_rect_hit(UG_RECT_C(g.arrow_rx, g.arrow_cy, 28, 28), p->x, p->y)) { ol_step_value(o, c, row, +1); return 1; }
            if (p->x > 18) { ol_step_value(o, c, row, +1); return 1; }
            return 1;
        }
        if (rw->kind == UG_ROW_ACTION) { ol_activate(o, c, row); return 1; }
        return 1;
    }
    return 0;
}

static void ol_draw(UgWidget *w, UgCtx *c, UgCanvas *cv) {
    UgOptionList *o = (UgOptionList *)w;
    if (o->style == UG_OL_CONTROLS) {
        /* header band (checklist 28): dark band y -174..-142, centred caption scale 0.7, arrows at x +-265.5 */
        float bx0 = -286.0f, bx1 = 274.0f;
        ug_kit_box(cv, (UgRect){bx0, UG_R_CTL_BAND_Y0, bx1 - bx0, UG_R_CTL_BAND_Y1 - UG_R_CTL_BAND_Y0}, 0, 1.0f);
        float cy = (UG_R_CTL_BAND_Y0 + UG_R_CTL_BAND_Y1) * 0.5f;
        ug_kit_text(c, cv, o->caption, (bx0 + bx1) * 0.5f, cy, 0.7f, UG_TEXT_LIGHT, UG_J_CENTER | UG_J_VCENTER);
        ug_kit_badge(cv, -UG_R_CTL_BAND_ARROW, UG_R_CTL_BAND_ARROW_Y, 20, 0, UG_RGBA(255, 255, 255, 150), 0);
        ug_kit_badge(cv, UG_R_CTL_BAND_ARROW, UG_R_CTL_BAND_ARROW_Y, 20, 1, UG_RGBA(255, 255, 255, 150), 0);
        ug_kit_panel(cv, (UgRect){-292, -140, 566, 276}, 1, 0.85f);
        ug_kit_text(c, cv, o->name_header, UG_R_CTL_NAME_X, -127, 0.5f, UG_TEXT_DARK, UG_J_VCENTER);
        ug_kit_text(c, cv, o->value_header, UG_R_ROW_VALUE_X, -127, 0.5f, UG_TEXT_DARK, UG_J_CENTER | UG_J_VCENTER);
    } else {
        ug_kit_panel(cv, (UgRect){-292, -150, 566, 300}, 1, 0.85f);     /* light panel behind the rows (G extent) */
    }
    for (int v = 0; v < o->visible; v++) {
        int row = o->first + v;
        if (row >= o->n) break;
        const UgOptionRow *r = &o->rows[row];
        UgOptionRowGeom g;
        ug_options_row_geom(o, v, &g);
        int foc = (row == o->cur) && w->focused && !r->disabled;
        ug_cv_push(cv);
        if (r->disabled) ug_cv_mul_alpha(cv, 120.0f / 255.0f);
        /* unfocused rows show no backing (R); the focused row's is dark grey and fades in 80 -> 200 */
        float fade = clampf((r->backing - 80.0f) / 120.0f, 0, 1);
        if (foc && fade > 0.0f)
            ug_cv_fill(cv, g.backing, UG_RGBA(0, 0, 0, ug_clamp8((float)UG_TUNE_ROW_BACKING_ALPHA * fade)), g.backing.h * 0.5f);
        UgColor base = foc ? UG_TEXT_LIGHT : UG_TEXT_DARK;
        ug_kit_text(c, cv, r->name, g.name_x, g.text_cy, g.name_scale, base, UG_J_VCENTER);
        if (foc) ug_kit_text(c, cv, r->name, g.name_x, g.text_cy, g.name_scale, ug_anim_color(&o->pulse), UG_J_VCENTER);
        if (r->kind == UG_ROW_SELECTOR || r->kind == UG_ROW_TOGGLE) {
            ug_kit_text(c, cv, r->values[r->sel], g.value_cx, g.text_cy, g.value_scale, base, UG_J_CENTER | UG_J_VCENTER);
            if (foc) {
                ug_kit_text(c, cv, r->values[r->sel], g.value_cx, g.text_cy, g.value_scale, ug_anim_color(&o->pulse), UG_J_CENTER | UG_J_VCENTER);
                /* value arrows only on the focused row (R) */
                ug_kit_badge(cv, g.arrow_lx, g.arrow_cy, o->blink_l.props.w[UG_P_SIZE], 0, ug_anim_color(&o->blink_l), 0);
                ug_kit_badge(cv, g.arrow_rx, g.arrow_cy, o->blink_r.props.w[UG_P_SIZE], 1, ug_anim_color(&o->blink_r), 0);
            }
        } else if (r->kind == UG_ROW_SLIDER) {
            /* drawn on every row that has one, focused or not (R) */
            ug_slider_draw_ex(cv, g.slider_track, r->slider, 0, foc, 1.0f, &o->pulse, UG_R_SLIDER_FILL_H, 12.0f);
        }
        ug_cv_pop(cv);
    }
    ug_scroller_draw(&o->sb, cv, 1.0f);
}

static void ol_hints(UgWidget *w, UgHintSet *h) {
    UgOptionList *o = (UgOptionList *)w;
    ug_hints_add(h, UG_A_INFO_WHEEL, "Change Selection", 75);
    if (o->n == 0) return;
    const UgOptionRow *r = &o->rows[o->cur];
    if (r->kind == UG_ROW_SLIDER) ug_hints_add(h, UG_A_INFO_DRAG, "Adjust", 74);
    else if (r->kind == UG_ROW_ACTION) ug_hints_add(h, UG_A_ACCEPT, "Select", 90);
    else ug_hints_add(h, UG_A_INFO_VALUE, "Change Value", 74);
}

static void ol_destroy(UgWidget *w) {
    UgOptionList *o = (UgOptionList *)w;
    ug_anim_free(&o->pulse); ug_anim_free(&o->blink_l); ug_anim_free(&o->blink_r);
    ug_scroller_free(&o->sb);
}
static const UgWidgetVT OL_VT = {ol_update, ol_action, ol_pointer, ol_draw, NULL, ol_hints, ol_destroy};

void ug_options_set_style(UgOptionList *o, int style) {
    o->style = style == UG_OL_CONTROLS ? UG_OL_CONTROLS : UG_OL_PLAIN;
    o->visible = o->style == UG_OL_CONTROLS ? UG_R_CTL_VISIBLE : UG_TUNE_ROW_VISIBLE;
    if (o->style == UG_OL_CONTROLS) {
        /* the header band occupies the top of the page; the scroller sits beside the rows (not measured, G) */
        o->sb.track = (UgRect){UG_R_VSCR_X0, -100.0f, UG_R_VSCR_X1 - UG_R_VSCR_X0, 224.0f};
        o->sb.lo = -84.0f; o->sb.hi = 108.0f;
    }
    ol_scroll_into_view(o);
}

void ug_options_init(UgOptionList *o, int id) {
    memset(o, 0, sizeof *o);
    ug_widget_init(&o->w, &OL_VT, "options", id, (UgRect){-292, -150, 566 + 14, 300});
    o->w.flags |= UG_W_CAPTURE;
    o->visible = UG_TUNE_ROW_VISIBLE;
    o->drag_row = -1;
    snprintf(o->name_header, sizeof o->name_header, "Action");
    snprintf(o->value_header, sizeof o->value_header, "Setting");
    snprintf(o->caption, sizeof o->caption, "Controls");
    /* vertical scroller: track x 269..277, y -147..+151, arrows 16.25 beyond the track ends (centres
     * -163.25 / +167.25 for -163.5 / +167 measured), thumb at -130 with the first row selected */
    ug_scroller_init(&o->sb, (UgRect){UG_R_VSCR_X0, UG_R_VSCR_Y0, UG_R_VSCR_X1 - UG_R_VSCR_X0, UG_R_VSCR_Y1 - UG_R_VSCR_Y0}, 1, 1);
    o->sb.arrow_gap = 16.25f;
    o->sb.has_range = 1; o->sb.lo = UG_R_VSCR_THUMB_TOP; o->sb.hi = UG_TUNE_VSCR_THUMB_END;
    ug_anim_init(&o->pulse, UG_ACCENT, 0, 0);
    ug_anim_play(&o->pulse, UG_ANIM_FOCUS_PULSE, 0);
    ug_anim_init(&o->blink_l, UG_RGBA(255, 255, 255, 150), 24, 24);
    ug_anim_init(&o->blink_r, UG_RGBA(255, 255, 255, 150), 24, 24);
}

int ug_options_add(UgOptionList *o, const char *name, int kind, int id,
                   const char *const *values, int nvalues) {
    if (o->n >= UG_MAX_ROWS) return -1;
    UgOptionRow *r = &o->rows[o->n];
    memset(r, 0, sizeof *r);
    snprintf(r->name, sizeof r->name, "%s", name);
    r->kind = kind; r->id = id;
    r->backing = 80;
    for (int i = 0; i < nvalues && i < UG_MAX_VALUES; i++) {
        snprintf(r->values[i], sizeof r->values[i], "%s", values[i]);
        r->nvalues++;
    }
    if (kind == UG_ROW_SLIDER) r->slider = 0.5f;
    o->n++;
    ug_scroller_set(&o->sb, o->cur, o->n);
    return o->n - 1;
}

void ug_options_set_disabled(UgOptionList *o, int row, int disabled) {
    if (row >= 0 && row < o->n) o->rows[row].disabled = disabled;
}

/* ================================================================== */
/* Table                                                               */
/* ================================================================== */
#define TB_ROW0  (-92.0f)
#define TB_PITCH 26.0f

static void tb_into_view(UgTable *t) {
    if (t->cur < t->first) t->first = t->cur;
    if (t->cur >= t->first + t->visible) t->first = t->cur - t->visible + 1;
    if (t->first < 0) t->first = 0;
    ug_scroller_set(&t->sb, t->cur, t->n);
}

static void tb_move(UgTable *t, UgCtx *c, int row) {
    if (row < 0 || row >= t->n || row == t->cur) return;
    t->cur = row;
    tb_into_view(t);
    ug_anim_play(&t->pulse, UG_ANIM_FOCUS_PULSE, 0);
    ug_cue(c, UG_CUE_BROWSER);
}

static void tb_update(UgWidget *w, UgCtx *c, float dt) {
    (void)c;
    UgTable *t = (UgTable *)w;
    ug_anim_update(&t->pulse, dt);
    ug_scroller_update(&t->sb, dt);
}

static int tb_action(UgWidget *w, UgCtx *c, UgAction a) {
    UgTable *t = (UgTable *)w;
    if (a == UG_A_UP) { if (t->cur <= 0) return 0; tb_move(t, c, t->cur - 1); ug_scroller_blink(&t->sb, 0); return 1; }
    if (a == UG_A_DOWN) { if (t->cur >= t->n - 1) return 0; tb_move(t, c, t->cur + 1); ug_scroller_blink(&t->sb, 1); return 1; }
    if (a == UG_A_ACCEPT && t->n) {
        ug_cue(c, UG_CUE_SELECT);
        ug_post(c, UG_EV_ACTIVATE, w->id, t->cur, t->cur);
        return 1;
    }
    return 0;
}

static int tb_row_at(const UgTable *t, float x, float y) {
    if (x < -285 || x > 255) return -1;
    for (int v = 0; v < t->visible; v++) {
        int row = t->first + v;
        if (row >= t->n) break;
        if (fabsf(y - (TB_ROW0 + (float)v * TB_PITCH)) <= TB_PITCH * 0.5f) return row;
    }
    return -1;
}

static int tb_pointer(UgWidget *w, UgCtx *c, const UgPtr *p) {
    UgTable *t = (UgTable *)w;
    int ni = t->cur;
    int r = ug_scroller_pointer(&t->sb, p, &ni);
    if (r == 2) { tb_move(t, c, ni); return 1; }
    if (r == 1) return 1;
    int row = tb_row_at(t, p->x, p->y);
    if (p->type == UG_PT_MOVE) { if (row >= 0) tb_move(t, c, row); return row >= 0; }
    if (p->type == UG_PT_WHEEL) { tb_move(t, c, t->cur + (p->wheel > 0 ? -1 : 1)); return 1; }
    if (p->type == UG_PT_DOWN && p->button == 1 && row >= 0) {
        tb_move(t, c, row);
        ug_cue(c, UG_CUE_SELECT);
        ug_post(c, UG_EV_ACTIVATE, w->id, row, row);
        return 1;
    }
    return 0;
}

static void tb_draw(UgWidget *w, UgCtx *c, UgCanvas *cv) {
    UgTable *t = (UgTable *)w;
    ug_kit_panel(cv,(UgRect){-300,-164,600,338},1,1);
    ug_kit_panel(cv,(UgRect){-290,-140,568,32},0,1);
    for (int k = 0; k < t->ncols; k++) {
        ug_kit_text(c, cv, t->head[k], t->colx[k], -118, 0.55f, UG_ACCENT, t->colj[k] | UG_J_VCENTER);
    }
    ug_cv_flat_rect(cv, (UgRect){-272, -108, 528, 1.5f}, UG_RGBA(197, 218, 165, 160));
    for (int v = 0; v < t->visible; v++) {
        int row = t->first + v;
        if (row >= t->n) break;
        float y = TB_ROW0 + (float)v * TB_PITCH;
        int sel = row == t->cur;
        if (sel && w->focused) {
            ug_cv_fill(cv,UG_RECT_C(-8,y,528,24),UG_RGBA(0,0,0,230),12);
            ug_kit_frame(cv, UG_RECT_C(-8, y, 530, 25), UG_ACCENT, 0.9f);
            /* white triangle marks the selected row (SPEC 6.16) */
            ug_cv_tri(cv, -284, y - 7, -284, y + 7, -272, y, UG_WHITE);
        } else if ((row & 1) == 0) {
            ug_cv_fill(cv, UG_RECT_C(-8, y, 528, 24), UG_RGBA(255, 255, 255, 14), 12);
        }
        for (int k = 0; k < t->ncols; k++) {
            ug_kit_text(c, cv, t->cell[row][k], t->colx[k], y, 0.62f, sel && w->focused ? UG_TEXT_LIGHT : UG_TEXT_DARK, t->colj[k] | UG_J_VCENTER);
            if (sel && w->focused)
                ug_kit_text(c, cv, t->cell[row][k], t->colx[k], y, 0.62f, ug_anim_color(&t->pulse), t->colj[k] | UG_J_VCENTER);
        }
    }
    ug_scroller_draw(&t->sb, cv, 1.0f);
}

static void tb_hints(UgWidget *w, UgHintSet *h) {
    (void)w;
    ug_hints_add(h, UG_A_INFO_WHEEL, "Change Selection", 75);
    ug_hints_add(h, UG_A_ACCEPT, "View Details", 90);
}
static void tb_destroy(UgWidget *w) {
    UgTable *t = (UgTable *)w;
    ug_anim_free(&t->pulse);
    ug_scroller_free(&t->sb);
}
static const UgWidgetVT TB_VT = {tb_update, tb_action, tb_pointer, tb_draw, NULL, tb_hints, tb_destroy};

void ug_table_init(UgTable *t, int id) {
    memset(t, 0, sizeof *t);
    ug_widget_init(&t->w, &TB_VT, "table", id, (UgRect){-290, -142, 574 + 20, 324});
    t->w.flags |= UG_W_CAPTURE;
    t->visible = 10;
    ug_scroller_init(&t->sb, (UgRect){269, -92, 8, 234}, 1, 1);
    t->sb.arrow_gap = 18;
    ug_anim_init(&t->pulse, UG_ACCENT, 0, 0);
    ug_anim_play(&t->pulse, UG_ANIM_FOCUS_PULSE, 0);
}

void ug_table_column(UgTable *t, const char *head, float x, unsigned just) {
    if (t->ncols >= UG_MAX_COLS) return;
    snprintf(t->head[t->ncols], sizeof t->head[0], "%s", head);
    t->colx[t->ncols] = x;
    t->colj[t->ncols] = just;
    t->ncols++;
}

int ug_table_add_row(UgTable *t, const char *c0, const char *c1, const char *c2, const char *c3) {
    if (t->n >= 32) return -1;
    const char *v[4] = {c0, c1, c2, c3};
    for (int k = 0; k < UG_MAX_COLS; k++) snprintf(t->cell[t->n][k], sizeof t->cell[0][0], "%s", v[k] ? v[k] : "");
    t->n++;
    ug_scroller_set(&t->sb, t->cur, t->n);
    return t->n - 1;
}

/* ================================================================== */
/* Car select (SPEC 6.17, amendment 3)                                 */
/* ================================================================== */
static const UgCar *cs_car(const UgCarSelect *s) {
    return s->nlist ? &s->cars[s->list[s->cur]] : NULL;
}

void ug_carselect_layout(const UgCarSelect *s, UgCarLayout *o) {
    memset(o, 0, sizeof *o);
    o->left_arrow = s->cur > 0;                      /* no wrap (4.5, R): hidden at the first car */
    o->right_arrow = s->cur < s->nlist - 1;
    o->left_x = -UG_R_CAR_ARROW_RX;                  /* the left arrow mirrors the measured right one (G) */
    o->right_x = UG_R_CAR_ARROW_RX;
    o->arrow_y = UG_R_CAR_ARROW_RY;
    o->track = (UgRect){UG_R_BAND_X0, UG_R_CAR_THUMB_Y - 4.0f, UG_R_BAND_X1 - UG_R_BAND_X0, 8.0f};   /* y -192 .. -184 */
    o->thumb_x = ug_icons_thumb_x(s->nlist, s->cur);
    o->thumb_y = UG_R_CAR_THUMB_Y;
    o->logo_frame = (UgRect){UG_R_CAR_LOGO_X0, UG_R_CAR_LOGO_Y0, UG_R_CAR_LOGO_X1 - UG_R_CAR_LOGO_X0, UG_R_CAR_LOGO_Y1 - UG_R_CAR_LOGO_Y0};
    o->badge_x = UG_R_CAR_BADGE_X; o->badge_y = UG_R_CAR_BADGE_Y;
    o->filter_l = UG_R_CAR_FILTER_ARROW_L; o->filter_r = UG_R_CAR_FILTER_ARROW_R;
    o->filter_y = UG_R_CAR_FILTER_Y; o->filter_text_x = UG_R_CAR_FILTER_TEXT_X;
}

static void cs_sync(UgCarSelect *s) {
    s->sb.hide_a = s->cur <= 0;
    s->sb.hide_b = s->cur >= s->nlist - 1;
    ug_scroller_set(&s->sb, s->cur, s->nlist);
}

static void cs_refresh(UgCarSelect *s, int dir) {
    const UgCar *car = cs_car(s);
    cs_sync(s);
    if (!car) return;
    ug_statbars_set(&s->bars, car->stats);
    s->slide = 0;
    s->slide_dir = dir;
}

static void cs_rebuild(UgCarSelect *s) {
    s->nlist = 0;
    for (int i = 0; i < s->ncars && s->nlist < 16; i++)
        if (s->filter.index == 0 || s->cars[i].category == s->filter.index - 1) s->list[s->nlist++] = i;
    s->cur = 0;
    cs_refresh(s, 0);
}

static void cs_step(UgCarSelect *s, UgCtx *c, int dir) {
    int ni = s->cur + dir;
    if (s->nlist < 2 || ni < 0 || ni >= s->nlist) return;     /* no wrap: the arrow on that side is hidden (4.5, R) */
    s->cur = ni;
    ug_anim_play(dir < 0 ? &s->blink_l : &s->blink_r, UG_ANIM_BLINK, 0);
    ug_cue(c, UG_CUE_BROWSER);
    cs_refresh(s, dir);
    ug_post(c, UG_EV_CHANGE, s->w.id, s->list[s->cur], s->cur);
}

static void cs_filter(UgCarSelect *s, UgCtx *c, int dir) {
    int before = s->filter.index;
    ug_switcher_step(&s->filter, c, dir);
    if (s->filter.index != before) cs_rebuild(s);
}

static void cs_update(UgWidget *w, UgCtx *c, float dt) {
    UgCarSelect *s = (UgCarSelect *)w;
    ug_anim_update(&s->blink_l, dt);
    ug_anim_update(&s->blink_r, dt);
    ug_anim_update(&s->pulse, dt);
    ug_scroller_update(&s->sb, dt);
    s->slide = clampf(s->slide + dt / 260.0f, 0, 1);
    s->filter.w.vt->update(&s->filter.w, c, dt);
    s->bars.w.vt->update(&s->bars.w, c, dt);
}

static int cs_action(UgWidget *w, UgCtx *c, UgAction a) {
    UgCarSelect *s = (UgCarSelect *)w;
    switch (a) {
    case UG_A_LEFT:  cs_step(s, c, -1); return 1;
    case UG_A_RIGHT: cs_step(s, c, +1); return 1;
    case UG_A_LTRIG: cs_filter(s, c, -1); return 1;
    case UG_A_RTRIG: cs_filter(s, c, +1); return 1;
    case UG_A_ACCEPT:
        if (s->nlist) {
            ug_cue(c, UG_CUE_SELECT);
            ug_post(c, UG_EV_ACTIVATE, w->id, s->list[s->cur], s->cur);
        }
        return 1;
    case UG_A_BTN1:
        ug_cue(c, UG_CUE_SUMMON_CUSTOMIZE);
        ug_post(c, UG_EV_CUSTOMIZE, w->id, s->nlist ? s->list[s->cur] : -1, 0);
        return 1;
    default: return 0;
    }
}

static int cs_pointer(UgWidget *w, UgCtx *c, const UgPtr *p) {
    UgCarSelect *s = (UgCarSelect *)w;
    if (p->type == UG_PT_WHEEL) {
        if (ug_rect_hit(s->filter.w.bounds, p->x, p->y)) cs_filter(s, c, p->wheel > 0 ? -1 : 1);
        else cs_step(s, c, p->wheel > 0 ? -1 : 1);
        return 1;
    }
    if (p->type == UG_PT_DOWN && p->button == 1) {
        UgCarLayout L;
        ug_carselect_layout(s, &L);
        if (ug_rect_hit(s->filter.w.bounds, p->x, p->y)) {
            int before = s->filter.index;
            s->filter.w.vt->pointer(&s->filter.w, c, p);
            if (s->filter.index != before) cs_rebuild(s);
            return 1;
        }
        if (L.left_arrow && ug_rect_hit(UG_RECT_C(L.left_x, L.arrow_y, 34, 34), p->x, p->y)) { cs_step(s, c, -1); return 1; }
        if (L.right_arrow && ug_rect_hit(UG_RECT_C(L.right_x, L.arrow_y, 34, 34), p->x, p->y)) { cs_step(s, c, +1); return 1; }
        if (ug_rect_hit(UG_RECT_C(0, 40, 300, 150), p->x, p->y)) { cs_action(w, c, UG_A_ACCEPT); return 1; }
    }
    return 0;
}

static void cs_draw(UgWidget *w, UgCtx *c, UgCanvas *cv) {
    UgCarSelect *s = (UgCarSelect *)w;
    const UgCar *car = cs_car(s);
    UgCarLayout L;
    ug_carselect_layout(s, &L);
    /* title: same text style as the options pages (ink left -288, cap top -214, scale 0.8) */
    ug_kit_title(c, cv, "Select Car");
    /* manufacturer logo in an accent frame (outer x -223..-92, y -178..-111) and the 256 x 64 name badge */
    ug_kit_panel(cv, L.logo_frame, 0, 0.9f);
    ug_cv_outline(cv, L.logo_frame, UG_ACCENT, 8, 2.0f);
    ug_kit_panel(cv, UG_RECT_C(L.badge_x, L.badge_y, 256, 64), 0, 0.9f);
    float lcx = L.logo_frame.x + L.logo_frame.w * 0.5f, lcy = L.logo_frame.y + L.logo_frame.h * 0.5f;
    if (car) {
        /* No text logo substitute; the demo states which art is omitted. */
        (void)lcx;(void)lcy;
        ug_kit_text(c, cv, car->name, L.badge_x, L.badge_y, 0.9f, UG_ACCENT, UG_J_CENTER | UG_J_VCENTER);
    }
    /* scroller under the title row: track y -192..-184, thumb on the 6.4 formula */
    ug_scroller_draw(&s->sb, cv, 1.0f);
    /* Car model imagery is unavailable to this 2D library; no substitute is drawn. */
    if (L.left_arrow) ug_kit_badge(cv, L.left_x, L.arrow_y, s->blink_l.props.w[UG_P_SIZE], 0, ug_anim_color(&s->blink_l), 0);
    if (L.right_arrow) ug_kit_badge(cv, L.right_x, L.arrow_y, s->blink_r.props.w[UG_P_SIZE], 1, ug_anim_color(&s->blink_r), 0);
    char idx[32];
    snprintf(idx, sizeof idx, "%d / %d", s->nlist ? s->cur + 1 : 0, s->nlist);
    ug_kit_text(c,cv,idx,0,112,0.5f,UG_TEXT_HINT,UG_J_CENTER|UG_J_VCENTER);
    ug_kit_text(c,cv,"Car preview / manufacturer / badge art unavailable",0,78,0.5f,UG_TEXT_HINT,UG_J_CENTER|UG_J_VCENTER);
    s->filter.w.vt->draw(&s->filter.w, c, cv);
    s->bars.w.vt->draw(&s->bars.w, c, cv);
}

static void cs_hints(UgWidget *w, UgHintSet *h) {
    (void)w;
    ug_hints_add(h, UG_A_ACCEPT, "Select Car", 90);
    ug_hints_add(h, UG_A_BTN1, "Customize", 85);
    ug_hints_add(h, UG_A_INFO_WHEEL, "Change Car", 75);
    ug_hints_add(h, UG_A_INFO_TABS, "Change Filter", 72);
}
static void cs_destroy(UgWidget *w) {
    UgCarSelect *s = (UgCarSelect *)w;
    ug_anim_free(&s->blink_l); ug_anim_free(&s->blink_r); ug_anim_free(&s->pulse);
    ug_scroller_free(&s->sb);
    s->filter.w.vt->destroy(&s->filter.w);
}
static const UgWidgetVT CS_VT = {cs_update, cs_action, cs_pointer, cs_draw, NULL, cs_hints, cs_destroy};

void ug_carselect_init(UgCarSelect *s, int id, const UgCar *cars, int n,
                       const char *const *cats, int ncats) {
    memset(s, 0, sizeof *s);
    ug_widget_init(&s->w, &CS_VT, "carselect", id, (UgRect){-300, -232, 600, 432});
    s->cars = cars; s->ncars = n;
    /* PC filter arrows at (95, -208) and (268, -208), caption centred x 185 */
    float fl = UG_R_CAR_FILTER_ARROW_L, fr = UG_R_CAR_FILTER_ARROW_R;
    ug_switcher_init(&s->filter, id + 1, (fl + fr) * 0.5f, UG_R_CAR_FILTER_Y, (fr - fl) + 20.0f, cats, ncats);
    s->filter.text_dx = UG_R_CAR_FILTER_TEXT_X - (fl + fr) * 0.5f;
    static const char *const LBL[3] = {"ACCELERATION", "TOP SPEED", "HANDLING"};
    ug_statbars_init_measured(&s->bars, id + 2, LBL, 3);
    ug_scroller_init(&s->sb, (UgRect){UG_R_BAND_X0, UG_R_CAR_THUMB_Y - 4.0f, UG_R_BAND_X1 - UG_R_BAND_X0, 8.0f}, 0, 0);
    s->sb.has_range = 1; s->sb.lo = UG_R_THUMB_X0; s->sb.hi = UG_R_THUMB_X0 + UG_R_THUMB_SPAN;
    s->sb.has_cross = 1; s->sb.cross = UG_R_CAR_THUMB_Y;
    ug_anim_init(&s->blink_l, UG_RGBA(255, 255, 255, 150), 26, 26);
    ug_anim_init(&s->blink_r, UG_RGBA(255, 255, 255, 150), 26, 26);
    ug_anim_init(&s->pulse, UG_ACCENT, 0, 0);
    s->slide = 1;
    cs_rebuild(s);
}
