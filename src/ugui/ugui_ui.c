/* ugui_ui.c -- context, hints, widget base, screens (focus/navigation) and
 * the screen stack (SPEC 4). */
#include "ugui_ui.h"
#include "ugui_tuning.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* actions, cues, context                                              */
/* ------------------------------------------------------------------ */
uint32_t ug_action_message(UgAction a) {
    switch (a) {
    case UG_A_ACCEPT: return UG_MSG_ACCEPT;
    case UG_A_BACK:   return UG_MSG_BACK;
    case UG_A_UP:     return UG_MSG_UP;
    case UG_A_DOWN:   return UG_MSG_DOWN;
    case UG_A_LEFT:   return UG_MSG_LEFT;
    case UG_A_RIGHT:  return UG_MSG_RIGHT;
    case UG_A_LTRIG:  return UG_MSG_LTRIG;
    case UG_A_RTRIG:  return UG_MSG_RTRIG;
    case UG_A_BTN0: case UG_A_BTN1: case UG_A_BTN2: case UG_A_BTN3:
        return UG_MSG_BUTTON0 + (uint32_t)(a - UG_A_BTN0);
    case UG_A_START:  return UG_MSG_START;
    default: return 0;
    }
}

static const char *ACTION_NAMES[UG_A_COUNT] = {
    "none", "accept", "back", "up", "down", "left", "right", "ltrig", "rtrig",
    "button0", "button1", "button2", "button3", "start", "help",
    "info-wheel", "info-value", "info-tabs", "info-drag", "info-rotate", "quit",
};
const char *ug_action_name(UgAction a) {
    return (a >= 0 && a < UG_A_COUNT) ? ACTION_NAMES[a] : "?";
}

int ug_action_clickable(UgAction a) {
    switch (a) {
    case UG_A_ACCEPT: case UG_A_BACK: case UG_A_HELP: case UG_A_BTN0:
    case UG_A_BTN1: case UG_A_BTN2: case UG_A_BTN3: case UG_A_START:
    case UG_A_LTRIG: case UG_A_RTRIG: case UG_A_QUIT:
        return 1;
    default: return 0;
    }
}

static const char *CUE_NAMES[UG_CUE_COUNT] = {
    "focus", "unfocus", "up", "down", "left", "right", "select", "back", "start",
    "wrong", "slider", "toggle", "scroll bar", "icon scroller", "browser",
    "date entry", "stat change", "text input", "keyboard nav", "keyboard enter",
    "keyboard delete", "trigger left", "trigger right", "install", "button",
    "summon dialog", "summon help", "summon customize",
};
const char *ug_cue_name(UgCue c) { return (c >= 0 && c < UG_CUE_COUNT) ? CUE_NAMES[c] : "?"; }

void ug_ctx_init(UgCtx *c) {
    memset(c, 0, sizeof *c);
    c->font = ug_font();
    c->glyphs = UG_GLYPHS_MOUSE;
    c->pointer_visible = 1;
    c->last_cue = UG_CUE_COUNT;
}

void ug_cue(UgCtx *c, UgCue cue) {
    c->last_cue = cue;
    if (c->cue_fn) c->cue_fn(c->cue_user, cue);
}

void ug_post(UgCtx *c, int kind, int source, int a, int b) {
    int nt = (c->qt + 1) % 64;
    if (nt == c->qh) return;       /* queue full: drop */
    c->q[c->qt] = (UgUiEvent){kind, source, a, b};
    c->qt = nt;
}

int ug_poll(UgCtx *c, UgUiEvent *out) {
    if (c->qh == c->qt) return 0;
    *out = c->q[c->qh];
    c->qh = (c->qh + 1) % 64;
    return 1;
}

/* ------------------------------------------------------------------ */
/* hints                                                               */
/* ------------------------------------------------------------------ */
void ug_hints_clear(UgHintSet *s) { s->n = 0; }

void ug_hint_glyphs(UgAction a, UgGlyph *g1, UgGlyph *g2) {
    const char *token=NULL;
    switch(a){
    case UG_A_ACCEPT: token="ACCEPT";break; case UG_A_BACK:token="CANCEL";break;
    case UG_A_HELP:token="HELP";break; case UG_A_LTRIG:token="LTRIGGER";break;case UG_A_RTRIG:token="RTRIGGER";break;
    case UG_A_BTN0:token="BUTTON0";break;case UG_A_BTN1:token="BUTTON1";break;
    case UG_A_BTN2:token="BUTTON2";break;case UG_A_BTN3:token="BUTTON3";break;case UG_A_START:token="START";break;
    case UG_A_INFO_WHEEL:token="PAD_UP";break;case UG_A_INFO_VALUE:token="PAD_LEFT";break;
    default:break;
    }
    *g1=*g2=(UgGlyph){UG_G_NONE,0};
    if(token)ug_glyph_for_token(token,UG_GLYPHS_MOUSE,g1);
}

void ug_hints_remove(UgHintSet *s, UgAction a) {
    for (int i = 0; i < s->n; i++)
        if (s->h[i].action == a) {
            memmove(&s->h[i], &s->h[i + 1], sizeof(UgHint) * (size_t)(s->n - i - 1));
            s->n--;
            return;
        }
}

void ug_hints_pc(UgHintSet *s, UgAction a, const char *label, int skip) {
    for (int i = 0; i < s->n; i++)
        if (s->h[i].action == a) {
            if (label) snprintf(s->h[i].pc_label, sizeof s->h[i].pc_label, "%s", label);
            s->h[i].pc_skip = skip;
        }
}

const UgHint *ug_hints_find(const UgHintSet *s, UgAction a) {
    for (int i = 0; i < s->n; i++) if (s->h[i].action == a) return &s->h[i];
    return NULL;
}

void ug_hints_add(UgHintSet *s, UgAction a, const char *label, int prio) {
    UgHint *h = NULL;
    for (int i = 0; i < s->n; i++) if (s->h[i].action == a) h = &s->h[i];
    if (!h) {
        if (s->n >= UG_MAX_HINTS) return;
        h = &s->h[s->n++];
    }
    memset(h, 0, sizeof *h);
    h->action = a;
    ug_hint_glyphs(a, &h->g1, &h->g2);
    snprintf(h->label, sizeof h->label, "%s", label);
    h->priority = prio;
    h->clickable = ug_action_clickable(a);
}

/* ------------------------------------------------------------------ */
/* widget base                                                         */
/* ------------------------------------------------------------------ */
void ug_widget_init(UgWidget *w, const UgWidgetVT *vt, const char *kind, int id, UgRect b) {
    memset(w, 0, sizeof *w);
    w->vt = vt;
    w->kind = kind;
    w->id = id;
    w->bounds = b;
}

int ug_widget_focusable(const UgWidget *w) {
    return w && !(w->flags & (UG_W_HIDDEN | UG_W_DISABLED | UG_W_NOFOCUS));
}

/* ------------------------------------------------------------------ */
/* screens                                                             */
/* ------------------------------------------------------------------ */
void ug_screen_init(UgScreen *s, const char *title, unsigned flags) {
    memset(s, 0, sizeof *s);
    s->title = title;
    s->flags = flags;
    s->focus = -1;
    s->default_focus = -1;
    ug_anim_init(&s->fade, UG_WHITE, 0, 0);
    ug_anim_init(&s->dim, UG_WHITE, 0, 0);
}

void ug_screen_free(UgScreen *s) {
    for (int i = 0; i < s->n; i++)
        if (s->w[i]->vt && s->w[i]->vt->destroy) s->w[i]->vt->destroy(s->w[i]);
    ug_anim_free(&s->fade);
    ug_anim_free(&s->dim);
    s->n = 0;
}

void ug_screen_add(UgScreen *s, UgWidget *w) {
    if (s->n >= UG_MAX_WIDGETS) return;
    s->w[s->n++] = w;
}

UgWidget *ug_screen_focused(UgScreen *s) {
    return (s->focus >= 0 && s->focus < s->n) ? s->w[s->focus] : NULL;
}

static void first_focus(UgScreen *s, UgCtx *c) {
    if (s->default_focus >= 0 && s->default_focus < s->n && ug_widget_focusable(s->w[s->default_focus])) {
        ug_screen_set_focus(s, c, s->default_focus);
        return;
    }
    for (int i = 0; i < s->n; i++)
        if (ug_widget_focusable(s->w[i])) { ug_screen_set_focus(s, c, i); return; }
}

void ug_screen_set_focus(UgScreen *s, UgCtx *c, int idx) {
    if (idx == s->focus) return;
    UgWidget *old = ug_screen_focused(s);
    if (old) {
        old->focused = 0;
        if (old->vt && old->vt->focus) old->vt->focus(old, c, 0);
    }
    s->focus = idx;
    UgWidget *nw = ug_screen_focused(s);
    if (nw) {
        nw->focused = 1;
        if (nw->vt && nw->vt->focus) nw->vt->focus(nw, c, 1);
        if (old) ug_cue(c, UG_CUE_FOCUS);
    }
}

void ug_screen_focus_widget(UgScreen *s, UgCtx *c, UgWidget *w) {
    for (int i = 0; i < s->n; i++)
        if (s->w[i] == w) { ug_screen_set_focus(s, c, i); return; }
}

int ug_screen_nav_target(const UgScreen *s, UgAction dir) {
    if (s->focus < 0 || s->focus >= s->n) return -1;
    const UgWidget *cur = s->w[s->focus];
    float cx = cur->bounds.x + cur->bounds.w * 0.5f, cy = cur->bounds.y + cur->bounds.h * 0.5f;
    int best = -1;
    float best_score = 1e30f;
    for (int i = 0; i < s->n; i++) {
        const UgWidget *w = s->w[i];
        if (i == s->focus || !ug_widget_focusable(w) || (w->flags & UG_W_NOREACH)) continue;
        float dx = w->bounds.x + w->bounds.w * 0.5f - cx, dy = w->bounds.y + w->bounds.h * 0.5f - cy;
        float fwd, lat;
        switch (dir) {
        case UG_A_UP:    fwd = -dy; lat = fabsf(dx); break;
        case UG_A_DOWN:  fwd = dy;  lat = fabsf(dx); break;
        case UG_A_LEFT:  fwd = -dx; lat = fabsf(dy); break;
        case UG_A_RIGHT: fwd = dx;  lat = fabsf(dy); break;
        default: return -1;
        }
        if (fwd <= 1.0f || lat > fwd * UG_TUNE_NAV_CONE) continue;     /* +-45 degree cone */
        float score = fwd + UG_TUNE_NAV_LATERAL_WEIGHT * lat;
        if (score < best_score) { best_score = score; best = i; }
    }
    return best;
}

int ug_screen_action(UgScreen *s, UgCtx *c, UgAction a) {
    if (s->focus < 0) first_focus(s, c);
    UgWidget *f = ug_screen_focused(s);
    if (f && f->vt && f->vt->action && f->vt->action(f, c, a)) return 1;
    if (s->on_action && s->on_action(s, c, a)) return 1;
    if (a == UG_A_UP || a == UG_A_DOWN || a == UG_A_LEFT || a == UG_A_RIGHT) {
        int t = ug_screen_nav_target(s, a);
        if (t >= 0) { ug_screen_set_focus(s, c, t); return 1; }
        return 0;
    }
    if (a == UG_A_BACK && !(s->flags & UG_SCR_ROOT)) {
        ug_cue(c, UG_CUE_BACK);
        ug_post(c, UG_EV_BACK, 0, 0, 0);
        return 1;
    }
    return 0;
}

static UgWidget *hit_widget(UgScreen *s, float x, float y) {
    for (int i = s->n - 1; i >= 0; i--) {
        UgWidget *w = s->w[i];
        if (w->flags & UG_W_HIDDEN) continue;
        if (ug_rect_hit(w->bounds, x, y)) return w;
    }
    return NULL;
}

int ug_screen_pointer(UgScreen *s, UgCtx *c, const UgPtr *p) {
    if (p->type == UG_PT_DOWN && p->button != 1) return 0;
    if (s->capture) {
        UgWidget *w = s->capture;
        int r = w->vt && w->vt->pointer ? w->vt->pointer(w, c, p) : 0;
        if (p->type == UG_PT_UP) s->capture = NULL;
        return r;
    }
    UgWidget *hit = hit_widget(s, p->x, p->y);
    if (!hit) return 0;
    if (((p->type == UG_PT_MOVE && UG_TUNE_HOVER_FOCUS && !(hit->flags & UG_W_NOHOVERFOCUS)) || p->type == UG_PT_DOWN) && ug_widget_focusable(hit))
        ug_screen_focus_widget(s, c, hit);
    if (hit->flags & UG_W_DISABLED) return 0;
    int r = hit->vt && hit->vt->pointer ? hit->vt->pointer(hit, c, p) : 0;
    if (p->type == UG_PT_DOWN && r && (hit->flags & UG_W_CAPTURE)) s->capture = hit;
    return r;
}

void ug_screen_update(UgScreen *s, UgCtx *c, float dt) {
    ug_anim_update(&s->fade, dt);
    ug_anim_update(&s->dim, dt);
    if (s->update) s->update(s, c, dt);
    for (int i = 0; i < s->n; i++)
        if (s->w[i]->vt && s->w[i]->vt->update) s->w[i]->vt->update(s->w[i], c, dt);
    if (s->focus < 0 || !ug_widget_focusable(ug_screen_focused(s))) {
        if (s->focus >= 0) { UgWidget *o = ug_screen_focused(s); if (o) o->focused = 0; s->focus = -1; }
        first_focus(s, c);
    }
}

void ug_screen_draw(UgScreen *s, UgCtx *c, UgCanvas *cv) {
    float a = ug_anim_alpha01(&s->fade) * ug_anim_alpha01(&s->dim);
    if (a <= 0.0f) return;
    ug_cv_push(cv);
    ug_cv_mul_alpha(cv, a);
    if (s->draw_bg) s->draw_bg(s, c, cv);
    for (int i = 0; i < s->n; i++) {
        UgWidget *w = s->w[i];
        if ((w->flags & UG_W_HIDDEN) || !w->vt || !w->vt->draw) continue;
        w->vt->draw(w, c, cv);
    }
    ug_cv_pop(cv);
}

void ug_screen_collect_hints(UgScreen *s, UgHintSet *out) {
    ug_hints_clear(out);
    UgWidget *f = ug_screen_focused(s);
    if (f && f->vt && f->vt->hints) f->vt->hints(f, out);
    for (int i = 0; i < s->hints.n; i++) {
        const UgHint *h = &s->hints.h[i];
        ug_hints_add(out, h->action, h->label, h->priority);
    }
    if (s->refresh_hints) s->refresh_hints(s, out);
    if (!(s->flags & UG_SCR_MODAL)) {
        if (!(s->flags & (UG_SCR_ROOT | UG_SCR_NOBACKHINT))) ug_hints_add(out, UG_A_BACK, "Back", 80);
        ug_hints_add(out, UG_A_HELP, "Help", 50);
    }
}

/* ------------------------------------------------------------------ */
/* UI stack and transitions                                            */
/* ------------------------------------------------------------------ */
void ug_ui_init(UgUi *u) {
    memset(u, 0, sizeof *u);
    ug_ctx_init(&u->ctx);
    ug_cv_init(&u->cv);
    ug_hintbar_init(&u->hints);
}

void ug_ui_free(UgUi *u) {
    ug_cv_free(&u->cv);
    ug_hintbar_free(&u->hints);
}

UgScreen *ug_ui_top(UgUi *u) { return u->sp ? u->stack[u->sp - 1] : NULL; }
int ug_ui_busy(const UgUi *u) { return u->pend != 0; }

static void app_post(UgUi *u, const UgUiEvent *e) {
    int nt = (u->aqt + 1) % 32;
    if (nt == u->aqh) return;
    u->appq[u->aqt] = *e;
    u->aqt = nt;
}

int ug_ui_poll(UgUi *u, UgUiEvent *out) {
    if (u->aqh == u->aqt) return 0;
    *out = u->appq[u->aqh];
    u->aqh = (u->aqh + 1) % 32;
    return 1;
}

static void enter(UgScreen *s) {
    s->focus = -1;
    ug_anim_stop(&s->dim);
    if (s->flags & UG_SCR_NOFADE) ug_anim_stop(&s->fade);
    else ug_anim_play(&s->fade, UG_ANIM_FADE_IN, 0);
}

static void do_push(UgUi *u, UgScreen *s) {
    if (u->sp >= UG_STACK_MAX) return;
    if (s->flags & UG_SCR_MODAL) {
        /* SPEC 6.9 (R): the frame beneath a dialog is darkened to 0.40 by the full-screen quad alone;
         * no extra dim fade was measurable (UG_TUNE_DIM_BELOW = 0). */
        UgScreen *below = ug_ui_top(u);
        if (below && UG_TUNE_DIM_BELOW) ug_anim_play(&below->dim, UG_ANIM_DIM, 0);
    }
    u->stack[u->sp++] = s;
    enter(s);
}

void ug_ui_push(UgUi *u, UgScreen *s) {
    if (u->pend || !s) return;
    UgScreen *top = ug_ui_top(u);
    if (!top || (s->flags & UG_SCR_MODAL)) {
        do_push(u, s);
        if (s->flags & UG_SCR_MODAL) ug_cue(&u->ctx, UG_CUE_SUMMON_DIALOG);
        return;
    }
    if (!(top->flags & UG_SCR_NOFADE)) ug_anim_play(&top->fade, UG_ANIM_FADE_OUT, 0);
    u->pend = 1;
    u->pend_scr = s;
    u->pend_t = 0;
}

void ug_ui_pop(UgUi *u) {
    if (u->pend || u->sp == 0) return;
    if (u->sp == 1 && !(u->stack[0]->flags & UG_SCR_MODAL)) return;   /* never pop the root */
    if (!(u->stack[u->sp - 1]->flags & UG_SCR_NOFADE)) ug_anim_play(&u->stack[u->sp - 1]->fade, UG_ANIM_FADE_OUT, 0);
    u->pend = 2;
    u->pend_t = 0;
}

static void finish_transition(UgUi *u) {
    if (u->pend == 1) {
        do_push(u, u->pend_scr);
    } else if (u->pend == 2) {
        UgScreen *gone = u->stack[--u->sp];
        UgScreen *top = ug_ui_top(u);
        ug_anim_stop(&gone->fade);
        if (top) {
            if (gone->flags & UG_SCR_MODAL) {
                if (UG_TUNE_DIM_BELOW) ug_anim_play(&top->dim, UG_ANIM_UNDIM, 0);
                else ug_anim_stop(&top->dim);
            }
            else {
                ug_anim_stop(&top->dim);
                if (top->flags & UG_SCR_NOFADE) ug_anim_stop(&top->fade);
                else ug_anim_play(&top->fade, UG_ANIM_FADE_IN, 0);
            }
        }
    }
    u->pend = 0;
    u->pend_scr = NULL;
}

static int drawn_from(const UgUi *u) {
    int i = u->sp - 1;
    while (i > 0 && (u->stack[i]->flags & UG_SCR_MODAL)) i--;
    return i < 0 ? 0 : i;
}

void ug_ui_action(UgUi *u, UgAction a) {
    UgScreen *top = ug_ui_top(u);
    if (!top || u->pend) return;
    if (a == UG_A_QUIT) {            /* PC help bar "Quit Game" pill: the application confirms */
        UgUiEvent e = {UG_EV_QUIT, 0, 0, 0};
        app_post(u, &e);
        return;
    }
    u->ctx.pointer_visible = 0;
    if (a == UG_A_BACK && (top->flags & UG_SCR_MODAL) == 0 && (top->flags & UG_SCR_ROOT)) {
        /* root page: Back does nothing, but widgets may still use it */
        ug_screen_action(top, &u->ctx, a);
        return;
    }
    if (!ug_screen_action(top, &u->ctx, a)) {
        if (a == UG_A_HELP || a == UG_A_START || (a >= UG_A_BTN0 && a <= UG_A_BTN3) ||
            a == UG_A_ACCEPT || a == UG_A_LTRIG || a == UG_A_RTRIG) {
            UgUiEvent e = {UG_EV_HINT, 0, (int)a, 0};
            app_post(u, &e);
        }
    }
}

void ug_ui_pointer(UgUi *u, const UgPtr *p) {
    UgScreen *top = ug_ui_top(u);
    UgCtx *c = &u->ctx;
    if (p->type == UG_PT_MOVE) { c->px = p->x; c->py = p->y; c->pointer_visible = 1; }
    if (!top || u->pend) return;
    if (p->type == UG_PT_DOWN && p->button == 2) {      /* secondary click = Back */
        c->pointer_visible = 1;
        ug_ui_action(u, UG_A_BACK);
        c->pointer_visible = 1;
        return;
    }
    int over = 0;
    if (ug_ui_hints_visible(u) && !top->capture) {
        UgAction hit = ug_hintbar_pointer(&u->hints, c, p, &over);
        if (over) {
            if (hit != UG_A_NONE) { ug_ui_action(u, hit); c->pointer_visible = 1; }
            return;
        }
    } else {
        ug_hintbar_pointer(&u->hints, c, &(UgPtr){UG_PT_MOVE, -9999, -9999, 0, 0}, &over);
    }
    if (ug_screen_pointer(top, c, p)) return;
    /* modal screens swallow clicks outside their widgets */
}

void ug_ui_text(UgUi *u, const char *utf8) {
    UgScreen *top = ug_ui_top(u);
    if (!top || u->pend) return;
    UgWidget *f = ug_screen_focused(top);
    if (f && f->kind && !strcmp(f->kind, "textfield")) {
        UgTextField *t = (UgTextField *)f;
        ug_textfield_insert(t, utf8);
        ug_cue(&u->ctx, UG_CUE_TEXT_INPUT);
        return;
    }
    for (int i = 0; i < top->n; i++) {
        UgWidget *w = top->w[i];
        if (w->kind && !strcmp(w->kind, "keyboard") && !(w->flags & UG_W_HIDDEN)) {
            ug_keyboard_type((UgKeyboard *)w, &u->ctx, utf8);
            return;
        }
    }
}

static void drain_events(UgUi *u) {
    UgUiEvent e;
    while (ug_poll(&u->ctx, &e)) {
        switch (e.kind) {
        case UG_EV_BACK:
            ug_ui_pop(u);
            break;
        case UG_EV_HINT:
            ug_ui_action(u, (UgAction)e.a);
            break;
        case UG_EV_DIALOG:
            ug_ui_pop(u);
            app_post(u, &e);
            break;
        default:
            app_post(u, &e);
            break;
        }
    }
}

void ug_ui_update(UgUi *u, float dt) {
    u->ctx.time_ms += dt;
    for (int i = 0; i < u->sp; i++) ug_screen_update(u->stack[i], &u->ctx, dt);
    if (u->pend) {
        u->pend_t += dt;
        UgScreen *top = ug_ui_top(u);
        if (top && !ug_anim_running(&top->fade)) finish_transition(u);
    }
    drain_events(u);

    UgScreen *top = ug_ui_top(u);
    if (top) {
        int from = drawn_from(u);
        UgScreen *page = u->stack[from];
        UgHintSet hs;
        ug_screen_collect_hints(page, &hs);
        ug_hintbar_set(&u->hints, &hs);
        u->hints.alpha = ug_anim_alpha01(&page->fade) * ug_anim_alpha01(&page->dim);
    }
    ug_hintbar_layout(&u->hints, &u->ctx);
    ug_hintbar_update(&u->hints, &u->ctx, dt);
}

int ug_ui_hints_visible(const UgUi *u) {
    /* SPEC 6.9 / 7.4.1 (R): the PC help bar and its strip are not drawn while a dialog is open */
    return !u->hide_hints && u->sp > 0 && drawn_from(u) == u->sp - 1;
}

void ug_ui_draw(UgUi *u) {
    ug_cv_clear(&u->cv);
    if (u->draw_back) u->draw_back(u, u->hook_user);
    int from = drawn_from(u);
    int bar = ug_ui_hints_visible(u);
    for (int i = from; i < u->sp; i++) {
        ug_screen_draw(u->stack[i], &u->ctx, &u->cv);
        if (i == from && bar) ug_hintbar_draw(&u->hints, &u->ctx, &u->cv);   /* drawn after the page */
    }
    if (u->draw_front) u->draw_front(u, u->hook_user);
    /* the cursor is the last thing drawn; it is clipped to the 4:3 viewport (SPEC 4.6, 6.21, R) */
    if (u->ctx.pointer_visible && ug_virtual_visible(u->ctx.px, u->ctx.py)) ug_kit_cursor(&u->cv, u->ctx.px, u->ctx.py);
}
