/* ugui_w_entry.c -- text field, on-screen keyboard (SPEC 6.14), date entry
 * (SPEC 6.15). */
#include "ugui_ui.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* ================================================================== */
/* Text field                                                          */
/* ================================================================== */
void ug_textfield_insert(UgTextField *t, const char *s) {
    for (const char *p = s; *p;) {
        if (*p == '\b') { ug_textfield_backspace(t); p++; continue; }
        const char *q = p;
        ug_utf8_next(&q);
        size_t n = (size_t)(q - p);
        if (q == p) break;
        if ((size_t)t->len + n >= sizeof t->text || t->len + (int)n > t->max) break;
        memmove(t->text + t->caret + n, t->text + t->caret, (size_t)(t->len - t->caret) + 1);
        memcpy(t->text + t->caret, p, n);
        t->len += (int)n;
        t->caret += (int)n;
        p = q;
    }
    t->blink = 0;
}

void ug_textfield_backspace(UgTextField *t) {
    if (t->caret <= 0) return;
    int i = t->caret - 1;
    while (i > 0 && ((unsigned char)t->text[i] & 0xC0) == 0x80) i--;
    int n = t->caret - i;
    memmove(t->text + i, t->text + t->caret, (size_t)(t->len - t->caret) + 1);
    t->len -= n;
    t->caret = i;
    t->blink = 0;
}

void ug_textfield_draw_at(UgTextField *t, UgCtx *c, UgCanvas *cv, UgRect r, float scale, int active) {
    ug_cv_fill(cv, r, UG_RGBA(255, 255, 255, active ? 210 : 150), 8);
    ug_cv_outline(cv, r, active ? UG_ACCENT : UG_RGBA(197, 218, 165, 200), 8, active ? 2.2f : 1.4f);
    float cy = r.y + r.h * 0.5f, x = r.x + 12;
    if (t->len == 0) {
        ug_kit_text(c, cv, active ? "Type here" : "(empty)", x, cy, scale * 0.75f, UG_RGBA(110, 110, 110, 200), UG_J_VCENTER);
    } else {
        char buf[64];
        const char *src = t->text;
        if (t->password) { int n = 0; for (const char *p = t->text; *p; ) { ug_utf8_next(&p); if (n < 62) buf[n++] = '*'; } buf[n] = 0; src = buf; }
        ug_kit_text(c, cv, src, x, cy, scale, UG_TEXT_DARK, UG_J_VCENTER);
    }
    if (active && fmodf(t->blink, 1000.0f) < 560.0f) {
        char pre[64];
        int n = t->caret < 63 ? t->caret : 63;
        memcpy(pre, t->text, (size_t)n);
        pre[n] = 0;
        float w = 0;
        if (n) ug_text_measure(pre, scale, 0, 0, c->glyphs, &w, NULL);
        ug_cv_flat_rect(cv, (UgRect){x + w + 1, cy - 11 * scale, 2, 22 * scale}, UG_RGBA(30, 30, 30, 255));
    }
}

static void tf_update(UgWidget *w, UgCtx *c, float dt) { (void)c; ((UgTextField *)w)->blink += dt; }
static void tf_draw(UgWidget *w, UgCtx *c, UgCanvas *cv) {
    ug_textfield_draw_at((UgTextField *)w, c, cv, w->bounds, 1.0f, w->focused);
}
static int tf_pointer(UgWidget *w, UgCtx *c, const UgPtr *p) {
    (void)c;
    UgTextField *t = (UgTextField *)w;
    if (p->type == UG_PT_DOWN && p->button == 1) { t->caret = t->len; return 1; }
    return 0;
}
static int tf_action(UgWidget *w, UgCtx *c, UgAction a) {
    UgTextField *t = (UgTextField *)w;
    (void)c;
    if (a == UG_A_BTN1) { ug_textfield_backspace(t); return 1; }
    if (a == UG_A_LEFT && t->caret > 0) { t->caret--; while (t->caret > 0 && ((unsigned char)t->text[t->caret] & 0xC0) == 0x80) t->caret--; return 1; }
    if (a == UG_A_RIGHT && t->caret < t->len) { t->caret++; while (t->caret < t->len && ((unsigned char)t->text[t->caret] & 0xC0) == 0x80) t->caret++; return 1; }
    return 0;
}
static void tf_hints(UgWidget *w, UgHintSet *h) {
    (void)w;
    ug_hints_add(h, UG_A_BTN1, "Delete", 85);
}
static const UgWidgetVT TF_VT = {tf_update, tf_action, tf_pointer, tf_draw, NULL, tf_hints, NULL};

void ug_textfield_init(UgTextField *t, int id, UgRect r, int max) {
    memset(t, 0, sizeof *t);
    ug_widget_init(&t->w, &TF_VT, "textfield", id, r);
    t->max = max > 47 ? 47 : max;
}

/* ================================================================== */
/* Keyboard                                                            */
/* ================================================================== */
static const char *KROWS[4] = {"1234567890", "QWERTYUIOP", "ASDFGHJKL'", "ZXCVBNM,.-"};
static const char *KSYM[4] = {"!@#$%^&*()", "[]{}<>\\|~`", "-_=+;:\"?!.", "()_,.@/:;?"};

static void kb_add(UgKeyboard *k, int kind, const char *lo, const char *up, const char *sy, UgRect r) {
    if (k->nkeys >= UG_MAX_KEYS) return;
    UgKbKey *key = &k->keys[k->nkeys++];
    memset(key, 0, sizeof *key);
    key->kind = kind;
    snprintf(key->lower, sizeof key->lower, "%s", lo);
    snprintf(key->upper, sizeof key->upper, "%s", up);
    snprintf(key->sym, sizeof key->sym, "%s", sy);
    key->r = r;
    for (int i = 0; i < 4; i++) key->link[i] = -1;
}

static void kb_layout(UgKeyboard *k, float cx, float top) {
    k->nkeys = 0;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 10; c++) {
            char lo[2] = {(char)((KROWS[r][c] >= 'A' && KROWS[r][c] <= 'Z') ? KROWS[r][c] + 32 : KROWS[r][c]), 0};
            char up[2] = {KROWS[r][c], 0}, sy[2] = {KSYM[r][c], 0};
            kb_add(k, UG_KB_CHAR, lo, up, sy,
                   UG_RECT_C(cx + ((float)c - 4.5f) * 40.0f, top + (float)r * 40.0f + 16.0f, 36, 32));
        }
    static const struct { int kind; const char *label; float w; } CTL[5] = {
        {UG_KB_SYMBOLS, "Symbols", 90}, {UG_KB_CAPS, "Caps", 90}, {UG_KB_SPACE, "Space", 200},
        {UG_KB_BACKSPACE, "Delete", 90}, {UG_KB_DONE, "Done", 90}};
    float total = 24;
    for (int i = 0; i < 5; i++) total += CTL[i].w;
    float x = cx - total * 0.5f + 0;
    for (int i = 0; i < 5; i++) {
        float w = CTL[i].w;
        kb_add(k, CTL[i].kind, CTL[i].label, CTL[i].label, CTL[i].label,
               UG_RECT_C(x + w * 0.5f, top + 4 * 40.0f + 16.0f, w, 32));
        x += w + 6;
    }
    /* explicit per-key links up,down,left,right (SPEC 4.5, 6.14) */
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 10; c++) {
            UgKbKey *key = &k->keys[r * 10 + c];
            key->link[0] = r > 0 ? (r - 1) * 10 + c : -1;
            key->link[1] = r < 3 ? (r + 1) * 10 + c : -1;
            key->link[2] = r * 10 + (c + 9) % 10;
            key->link[3] = r * 10 + (c + 1) % 10;
        }
    for (int i = 0; i < 5; i++) {
        UgKbKey *key = &k->keys[40 + i];
        float kx = key->r.x + key->r.w * 0.5f;
        int near_up = 30, ib = 1e9;
        for (int c = 0; c < 10; c++) {
            const UgKbKey *u = &k->keys[30 + c];
            int d = (int)fabsf(u->r.x + u->r.w * 0.5f - kx);
            if (d < ib) { ib = d; near_up = 30 + c; }
        }
        key->link[0] = near_up;
        key->link[1] = -1;
        key->link[2] = 40 + (i + 4) % 5;
        key->link[3] = 40 + (i + 1) % 5;
    }
    for (int c = 0; c < 10; c++) {          /* bottom character row -> nearest control key */
        UgKbKey *key = &k->keys[30 + c];
        float kx = key->r.x + key->r.w * 0.5f;
        int best = 40, bd = 1 << 30;
        for (int i = 0; i < 5; i++) {
            const UgKbKey *u = &k->keys[40 + i];
            int d = (int)fabsf(u->r.x + u->r.w * 0.5f - kx);
            if (d < bd) { bd = d; best = 40 + i; }
        }
        key->link[1] = best;
    }
}

static const char *kb_label(const UgKeyboard *k, const UgKbKey *key) {
    if (key->kind != UG_KB_CHAR) return key->lower;
    return k->symbols ? key->sym : (k->caps ? key->upper : key->lower);
}

static void kb_press(UgKeyboard *k, UgCtx *c, int i) {
    UgKbKey *key = &k->keys[i];
    key->flash = 160;
    switch (key->kind) {
    case UG_KB_CHAR:
        if (k->target) ug_textfield_insert(k->target, kb_label(k, key));
        ug_cue(c, UG_CUE_KEY_ENTER);
        break;
    case UG_KB_SPACE:
        if (k->target) ug_textfield_insert(k->target, " ");
        ug_cue(c, UG_CUE_KEY_ENTER);
        break;
    case UG_KB_CAPS: k->caps = !k->caps; ug_cue(c, UG_CUE_TRIGGER_R); break;
    case UG_KB_SYMBOLS: k->symbols = !k->symbols; ug_cue(c, UG_CUE_TRIGGER_L); break;
    case UG_KB_BACKSPACE:
        if (k->target) ug_textfield_backspace(k->target);
        ug_cue(c, UG_CUE_KEY_DELETE);
        break;
    case UG_KB_DONE:
        ug_cue(c, UG_CUE_SELECT);
        ug_post(c, UG_EV_TEXT_DONE, k->w.id, k->target ? k->target->len : 0, 0);
        break;
    }
}

void ug_keyboard_type(UgKeyboard *k, UgCtx *c, const char *utf8) {
    if (!k->target) return;
    ug_textfield_insert(k->target, utf8);
    ug_cue(c, UG_CUE_TEXT_INPUT);
}

static void kb_update(UgWidget *w, UgCtx *c, float dt) {
    (void)c;
    UgKeyboard *k = (UgKeyboard *)w;
    ug_anim_update(&k->pulse, dt);
    for (int i = 0; i < k->nkeys; i++) if (k->keys[i].flash > 0) k->keys[i].flash -= dt;
}

static int kb_action(UgWidget *w, UgCtx *c, UgAction a) {
    UgKeyboard *k = (UgKeyboard *)w;
    int dir = a == UG_A_UP ? 0 : a == UG_A_DOWN ? 1 : a == UG_A_LEFT ? 2 : a == UG_A_RIGHT ? 3 : -1;
    if (dir >= 0) {
        int to = k->keys[k->cur].link[dir];
        if (to < 0) return 0;
        k->cur = to;
        ug_anim_play(&k->pulse, UG_ANIM_FOCUS_PULSE, 0);
        ug_cue(c, UG_CUE_KEY_NAV);
        return 1;
    }
    switch (a) {
    case UG_A_ACCEPT: kb_press(k, c, k->cur); return 1;
    case UG_A_BTN1: if (k->target) ug_textfield_backspace(k->target); ug_cue(c, UG_CUE_KEY_DELETE); return 1;
    case UG_A_LTRIG: k->symbols = !k->symbols; ug_cue(c, UG_CUE_TRIGGER_L); return 1;
    case UG_A_RTRIG: k->caps = !k->caps; ug_cue(c, UG_CUE_TRIGGER_R); return 1;
    default: return 0;
    }
}

static int kb_hit(const UgKeyboard *k, float x, float y) {
    for (int i = 0; i < k->nkeys; i++) {
        UgRect r = k->keys[i].r;
        r.x -= 2; r.y -= 4; r.w += 4; r.h += 8;
        if (ug_rect_hit(r, x, y)) return i;
    }
    return -1;
}

static int kb_pointer(UgWidget *w, UgCtx *c, const UgPtr *p) {
    UgKeyboard *k = (UgKeyboard *)w;
    int i = kb_hit(k, p->x, p->y);
    if (p->type == UG_PT_MOVE && i >= 0 && i != k->cur) {
        k->cur = i;
        ug_anim_play(&k->pulse, UG_ANIM_FOCUS_PULSE, 0);
        ug_cue(c, UG_CUE_KEY_NAV);
        return 1;
    }
    if (p->type == UG_PT_DOWN && p->button == 1 && i >= 0) {
        k->cur = i;
        kb_press(k, c, i);
        return 1;
    }
    return 0;
}

static void kb_draw(UgWidget *w, UgCtx *c, UgCanvas *cv) {
    UgKeyboard *k = (UgKeyboard *)w;
    for (int i = 0; i < k->nkeys; i++) {
        const UgKbKey *key = &k->keys[i];
        int foc = (i == k->cur) && w->focused;
        int lit = (key->kind == UG_KB_CAPS && k->caps) || (key->kind == UG_KB_SYMBOLS && k->symbols);
        UgColor fill = UG_RGBA(255, 255, 255, 150);
        if (lit) fill = UG_RGBA(197, 230, 130, 220);
        if (key->flash > 0) fill = UG_RGBA(255, 255, 255, 240);
        ug_cv_fill(cv, key->r, fill, 7);
        float cx = key->r.x + key->r.w * 0.5f, cy = key->r.y + key->r.h * 0.5f;
        float sc = key->kind == UG_KB_CHAR ? 0.8f : 0.6f;
        ug_kit_text(c, cv, kb_label(k, key), cx, cy, sc, UG_TEXT_DARK, UG_J_CENTER | UG_J_VCENTER);
        if (foc) {
            ug_kit_text(c, cv, kb_label(k, key), cx, cy, sc, ug_anim_color(&k->pulse), UG_J_CENTER | UG_J_VCENTER);
            ug_kit_frame(cv, (UgRect){key->r.x - 2, key->r.y - 2, key->r.w + 4, key->r.h + 4}, UG_ACCENT, 1.0f);
        }
    }
}

static void kb_hints(UgWidget *w, UgHintSet *h) {
    (void)w;
    ug_hints_add(h, UG_A_ACCEPT, "Type", 90);
    ug_hints_add(h, UG_A_BTN1, "Delete", 85);
    ug_hints_add(h, UG_A_LTRIG, "Symbols", 72);
    ug_hints_add(h, UG_A_RTRIG, "Caps", 71);
}
static void kb_destroy(UgWidget *w) { ug_anim_free(&((UgKeyboard *)w)->pulse); }
static const UgWidgetVT KB_VT = {kb_update, kb_action, kb_pointer, kb_draw, NULL, kb_hints, kb_destroy};

void ug_keyboard_init(UgKeyboard *k, int id, float cx, float top, UgTextField *target) {
    memset(k, 0, sizeof *k);
    ug_widget_init(&k->w, &KB_VT, "keyboard", id, (UgRect){cx - 300, top - 4, 600, 5 * 40 + 8});
    k->target = target;
    k->caps = 1;
    kb_layout(k, cx, top);
    ug_anim_init(&k->pulse, UG_ACCENT, 0, 0);
    ug_anim_play(&k->pulse, UG_ANIM_FOCUS_PULSE, 0);
}

/* ================================================================== */
/* Date entry                                                          */
/* ================================================================== */
static int days_in_month(int m, int y) {
    static const int d[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) return 29;
    return d[(m - 1 + 12) % 12];
}

#define DATE_MIN_YEAR 1990
#define DATE_MAX_YEAR 2040

static UgRect date_field_rect(const UgDateEntry *d, int f) {
    static const float W[3] = {96, 96, 140};
    float total = W[0] + W[1] + W[2] + 2 * 30.0f;
    float cx = d->w.bounds.x + d->w.bounds.w * 0.5f, cy = d->w.bounds.y + d->w.bounds.h * 0.5f;
    float x = cx - total * 0.5f;
    for (int i = 0; i < f; i++) x += W[i] + 30.0f;
    return UG_RECT_C(x + W[f] * 0.5f, cy, W[f], 58);
}

static void date_cycle(UgDateEntry *d, UgCtx *c, int dir) {
    if (d->field == 0) d->month = (d->month - 1 + dir + 12) % 12 + 1;
    else if (d->field == 1) { int n = days_in_month(d->month, d->year); d->day = (d->day - 1 + dir + n) % n + 1; }
    else { d->year += dir; if (d->year > DATE_MAX_YEAR) d->year = DATE_MIN_YEAR; if (d->year < DATE_MIN_YEAR) d->year = DATE_MAX_YEAR; }
    int n = days_in_month(d->month, d->year);
    if (d->day > n) d->day = n;
    ug_anim_play(dir > 0 ? &d->blink_u : &d->blink_d, UG_ANIM_BLINK, 0);
    ug_cue(c, UG_CUE_DATE_ENTRY);
    ug_post(c, UG_EV_CHANGE, d->w.id, d->field, d->year * 10000 + d->month * 100 + d->day);
}

static void date_update(UgWidget *w, UgCtx *c, float dt) {
    (void)c;
    UgDateEntry *d = (UgDateEntry *)w;
    ug_anim_update(&d->pulse, dt);
    ug_anim_update(&d->blink_u, dt);
    ug_anim_update(&d->blink_d, dt);
}

static int date_action(UgWidget *w, UgCtx *c, UgAction a) {
    UgDateEntry *d = (UgDateEntry *)w;
    switch (a) {
    case UG_A_UP: date_cycle(d, c, +1); return 1;
    case UG_A_DOWN: date_cycle(d, c, -1); return 1;
    case UG_A_LEFT: if (d->field == 0) return 0; d->field--; ug_cue(c, UG_CUE_LEFT); return 1;
    case UG_A_RIGHT: if (d->field == 2) return 0; d->field++; ug_cue(c, UG_CUE_RIGHT); return 1;
    case UG_A_ACCEPT:
        ug_cue(c, UG_CUE_SELECT);
        ug_post(c, UG_EV_DATE, w->id, d->year * 10000 + d->month * 100 + d->day, 0);
        return 1;
    default: return 0;
    }
}

static int date_pointer(UgWidget *w, UgCtx *c, const UgPtr *p) {
    UgDateEntry *d = (UgDateEntry *)w;
    for (int f = 0; f < 3; f++) {
        UgRect r = date_field_rect(d, f);
        UgRect up = UG_RECT_C(r.x + r.w * 0.5f, r.y - 22, 36, 28), dn = UG_RECT_C(r.x + r.w * 0.5f, r.y + r.h + 22, 36, 28);
        if (p->type == UG_PT_DOWN && p->button == 1) {
            if (ug_rect_hit(up, p->x, p->y)) { d->field = f; date_cycle(d, c, +1); return 1; }
            if (ug_rect_hit(dn, p->x, p->y)) { d->field = f; date_cycle(d, c, -1); return 1; }
            if (ug_rect_hit(r, p->x, p->y)) { d->field = f; return 1; }
        } else if (p->type == UG_PT_MOVE) {
            if (ug_rect_hit(r, p->x, p->y) && d->field != f) { d->field = f; ug_cue(c, UG_CUE_FOCUS); return 1; }
        } else if (p->type == UG_PT_WHEEL) {
            if (ug_rect_hit(r, p->x, p->y) || ug_rect_hit(up, p->x, p->y) || ug_rect_hit(dn, p->x, p->y)) {
                d->field = f;
                date_cycle(d, c, p->wheel > 0 ? +1 : -1);
                return 1;
            }
        }
    }
    return 0;
}

static void date_draw(UgWidget *w, UgCtx *c, UgCanvas *cv) {
    UgDateEntry *d = (UgDateEntry *)w;
    char buf[8];
    int val[3] = {d->month, d->day, d->year};
    static const char *CAP[3] = {"Month", "Day", "Year"};
    for (int f = 0; f < 3; f++) {
        UgRect r = date_field_rect(d, f);
        float cx = r.x + r.w * 0.5f;
        int sel = f == d->field;
        ug_kit_panel(cv, r, 0, sel ? 1.0f : 0.8f);
        snprintf(buf, sizeof buf, f == 2 ? "%04d" : "%02d", val[f]);
        ug_kit_text(c, cv, buf, cx, r.y + r.h * 0.5f, 1.15f, sel ? UG_TEXT_LIGHT : UG_RGBA(170, 170, 170, 255), UG_J_CENTER | UG_J_VCENTER);
        if (sel) {
            if (w->focused) ug_kit_text(c, cv, buf, cx, r.y + r.h * 0.5f, 1.15f, ug_anim_color(&d->pulse), UG_J_CENTER | UG_J_VCENTER);
            ug_kit_frame(cv, r, UG_ACCENT, w->focused ? 1.0f : 0.5f);
            ug_kit_badge(cv, cx, r.y - 22, d->blink_u.props.w[UG_P_SIZE], 2, ug_anim_color(&d->blink_u), 0);
            ug_kit_badge(cv, cx, r.y + r.h + 22, d->blink_d.props.w[UG_P_SIZE], 3, ug_anim_color(&d->blink_d), 0);
        }
        ug_kit_text(c, cv, CAP[f], cx, r.y + r.h + 50, 0.5f, UG_TEXT_HINT, UG_J_CENTER | UG_J_VCENTER);
        if (f < 2) ug_kit_text(c, cv, "/", r.x + r.w + 15, r.y + r.h * 0.5f, 1.0f, UG_TEXT_HINT, UG_J_CENTER | UG_J_VCENTER);
    }
}

static void date_hints(UgWidget *w, UgHintSet *h) {
    (void)w;
    ug_hints_add(h, UG_A_INFO_WHEEL, "Cycle Numbers", 75);
    ug_hints_add(h, UG_A_ACCEPT, "Accept Date", 90);
}
static void date_destroy(UgWidget *w) {
    UgDateEntry *d = (UgDateEntry *)w;
    ug_anim_free(&d->pulse); ug_anim_free(&d->blink_u); ug_anim_free(&d->blink_d);
}
static void date_focus(UgWidget *w, UgCtx *c, int gained) {
    (void)c; (void)gained;
    UgDateEntry *d = (UgDateEntry *)w;
    ug_anim_play(&d->pulse, UG_ANIM_FOCUS_PULSE, 0);
}
static const UgWidgetVT DATE_VT = {date_update, date_action, date_pointer, date_draw, date_focus, date_hints, date_destroy};

void ug_date_init(UgDateEntry *d, int id, float cx, float cy, int y, int m, int day) {
    memset(d, 0, sizeof *d);
    ug_widget_init(&d->w, &DATE_VT, "date", id, UG_RECT_C(cx, cy, 440, 170));
    d->year = y; d->month = m; d->day = day;
    ug_anim_init(&d->pulse, UG_ACCENT, 0, 0);
    ug_anim_play(&d->pulse, UG_ANIM_FOCUS_PULSE, 0);
    ug_anim_init(&d->blink_u, UG_RGBA(255, 255, 255, 150), 22, 22);
    ug_anim_init(&d->blink_d, UG_RGBA(255, 255, 255, 150), 22, 22);
}
