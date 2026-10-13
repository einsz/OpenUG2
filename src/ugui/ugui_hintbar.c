/* ugui_hintbar.c -- mouse-oriented hint bar (SPEC 7.6), the compact pill grid
 * (SPEC 7.4), the original PC help bar as observed (SPEC 7.4.1) and the player
 * strip (SPEC 6.18). */
#include "ugui_ui.h"
#include "ugui_tuning.h"
#include <stdio.h>
#include <string.h>

#define STRIP_Y     206.0f
#define HELP_X      212.0f
#define AREA_LEFT   UG_TUNE_HINT_AREA_LEFT   /* dark box starts at -107 (SPEC 7.1) */
#define GLYPH_W     18.0f
#define GLYPH_GAP   6.0f
#define ITEM_GAP    UG_TUNE_HINT_ITEM_GAP
#define SLASH_W     10.0f

/* slot centres of the 3 x 2 pill grid (SPEC 7.4) */
static const float PILL_SLOT[6][2] = {
    {223, 226}, {223, 203}, {76, 203}, {-72, 203}, {76, 226}, {-72, 226}};

/* ------------------------------------------------------------------ */
/* PC help bar model (SPEC 7.4.1, amendment 3, R)                      */
/* ------------------------------------------------------------------ */
int ug_pc_slot_center(int index, float *x, float *y) {
    if (index < 0 || index >= 6) return 0;
    *x = PILL_SLOT[index][0];
    *y = PILL_SLOT[index][1];
    return 1;
}

void ug_pc_strip_geom(UgPcStripGeom *g) {
    memset(g, 0, sizeof *g);
    g->top = UG_R_PC_STRIP_TOP;
    g->bottom = UG_R_PC_STRIP_BOT;
    float h = g->bottom - g->top, k = 16.0f;
    /* rounded ends: a 16 x 16 corner on top of a 16-wide box (left x -300.4..-284.4, right +281.6..+297.6) */
    g->left_corner = (UgRect){UG_R_PC_LEFT_X0, g->top, k, k};
    g->left_box = (UgRect){UG_R_PC_LEFT_X0, g->top + k, k, h - k};
    g->player = (UgRect){UG_R_PC_PLAYER_X0, g->top, UG_R_PC_PLAYER_X1 - UG_R_PC_PLAYER_X0, h};
    g->hint = (UgRect){UG_R_PC_HINT_X0, g->top, UG_R_PC_HINT_X1 - UG_R_PC_HINT_X0, h};
    g->right_corner = (UgRect){UG_R_PC_RIGHT_X1 - k, g->top, k, k};
    g->right_box = (UgRect){UG_R_PC_RIGHT_X1 - k, g->top + k, k, h - k};
    g->player_cx = UG_R_PC_PLAYER_CX; g->player_cy = UG_R_PC_PLAYER_CY;
    /* the art is the black 60% box (final alpha 153 x object alpha / 255): (128,128,128) at 215 for the
     * player part, white at 150 for the hint part. Modulate keeps both black, the player part darker (Q17) */
    g->player_alpha = UG_R_PC_PLAYER_ALPHA * 153 / 255;
    g->hint_alpha = UG_R_PC_HINT_ALPHA * 153 / 255;
}

void ug_pc_pill_geom(int index, UgRect *outline, float *label_cx, float *label_cy) {
    float x = 0, y = 0;
    ug_pc_slot_center(index, &x, &y);
    if (outline) *outline = UG_RECT_C(x, y + 0.3f, UG_R_PC_PILL_W, UG_R_PC_PILL_H);
    if (label_cx) *label_cx = x;
    if (label_cy) *label_cy = y + UG_R_PC_LABEL_DY;
}

static void pc_add(UgPcPills *o, UgAction a, const char *label) {
    if (o->n >= 6) return;
    o->p[o->n].action = a;
    snprintf(o->p[o->n].label, sizeof o->p[o->n].label, "%s", label);
    o->n++;
}

/* Slot rule observed on four screens (7.4.1, I): slot 1 is always the quit entry, slot 2 holds the screen's
 * forward action (Continue) when it has one, else Back, the next free slot holds Back or the extra action. */
void ug_pc_assign(const UgHintSet *s, UgPcPills *out) {
    memset(out, 0, sizeof *out);
    pc_add(out, UG_A_QUIT, "Quit Game");
    const UgHint *fwd = NULL, *back = NULL;
    for (int i = 0; i < s->n; i++) {
        const UgHint *h = &s->h[i];
        if (h->pc_skip) continue;
        if (h->action == UG_A_ACCEPT && !fwd) fwd = h;
        else if (h->action == UG_A_BACK && !back) back = h;
    }
    if (fwd) pc_add(out, UG_A_ACCEPT, fwd->pc_label[0] ? fwd->pc_label : "Continue");
    if (back) pc_add(out, UG_A_BACK, back->pc_label[0] ? back->pc_label : "Back");
    for (int i = 0; i < s->n; i++) {         /* further actions: the screen's extra buttons and trigger pages */
        const UgHint *h = &s->h[i];
        if (h->pc_skip) continue;
        switch (h->action) {
        case UG_A_BTN0: case UG_A_BTN1: case UG_A_BTN2: case UG_A_BTN3: case UG_A_LTRIG: case UG_A_RTRIG:
            pc_add(out, h->action, h->pc_label[0] ? h->pc_label : h->label);
            break;
        default: break;                      /* Help has no pill on PC; informational hints are not clickable */
        }
    }
}

void ug_hintbar_init(UgHintBar *h) {
    memset(h, 0, sizeof *h);
    h->hover = -1;
    h->mode = UG_HINTS_PC;
    h->alpha = 1;
    snprintf(h->player, sizeof h->player, "PLAYER");
    snprintf(h->cash, sizeof h->cash, "$ 0");
    ug_anim_init(&h->hover_pulse, UG_RGBA(255,255,255,150), 0, 0);
    ug_anim_init(&h->hover_glow, UG_WHITE, 0, 0);
    ug_anim_init(&h->swap_name, UG_RGBA(230, 230, 230, 255), 0, 0);
    ug_anim_init(&h->swap_cash, UG_RGBA(230, 230, 230, 255), 0, 0);
    h->swap_cash.props.w[3] = 0;
    h->showing_name = 1;
}

void ug_hintbar_free(UgHintBar *h) {
    ug_anim_free(&h->hover_pulse); ug_anim_free(&h->hover_glow);
    ug_anim_free(&h->swap_name); ug_anim_free(&h->swap_cash);
}

/* Help first (rightmost), then by descending priority */
void ug_hintbar_set(UgHintBar *h, const UgHintSet *s) {
    UgHintSet o;
    memset(&o, 0, sizeof o);
    for (int i = 0; i < s->n; i++) if (s->h[i].action == UG_A_HELP) o.h[o.n++] = s->h[i];
    for (;;) {
        int best = -1;
        for (int i = 0; i < s->n; i++) {
            if (s->h[i].action == UG_A_HELP) continue;
            int used = 0;
            for (int k = 0; k < o.n; k++) if (o.h[k].action == s->h[i].action) used = 1;
            if (used) continue;
            if (best < 0 || s->h[i].priority > s->h[best].priority) best = i;
        }
        if (best < 0) break;
        o.h[o.n++] = s->h[best];
    }
    /* keep hover state stable across identical sets */
    if (o.n != h->set.n || memcmp(&o, &h->set, sizeof o) != 0) h->hover = -1;
    h->set = o;
    ug_pc_assign(&h->set, &h->pc);
}

static float label_w(UgCtx *c, const char *s) {
    float w;
    ug_text_measure(s, 0.7f, 0, 0, c->glyphs, &w, NULL);
    return w;
}

static float item_w(UgCtx *c, const UgHint *hn, int use2) {
    float w = GLYPH_W;
    if (use2 && hn->g2.id != UG_G_NONE) w += SLASH_W + GLYPH_W;
    return w + GLYPH_GAP + label_w(c, hn->label);
}

/* lay the items out right-to-left from the Help slot; returns the leftmost x */
static float strip_layout(UgHintBar *h, UgCtx *c, const int *idx, int n, int use2, float *xs) {
    float x = HELP_X;
    for (int k = 0; k < n; k++) {
        float w = item_w(c, &h->set.h[idx[k]], use2);
        if (k > 0) x = xs[k - 1] - ITEM_GAP - w;
        xs[k] = x;
    }
    return n ? xs[n - 1] : HELP_X;
}

void ug_hintbar_layout(UgHintBar *h, UgCtx *c) {
    int idx[UG_MAX_HINTS], n = 0;
    h->nshown = 0;
    if(h->mode==UG_HINTS_PC) {
        for(int i=0;i<h->pc.n;i++) {
            float x=0,y=0;ug_pc_slot_center(i,&x,&y);
            h->shown[i]=i;h->rect[i]=UG_RECT_C(x,y,144,24);
        }
        h->nshown=h->pc.n;h->use_g2=0;return;
    }
    if(h->mode!=UG_HINTS_STRIP)return; /* historical invented pill layout stays disabled */
    for (int i = 0; i < h->set.n; i++) idx[n++] = i;
    float xs[UG_MAX_HINTS];
    /* 1) both glyphs (mouse / key), 2) mouse glyph only, 3) drop the lowest priority */
    int use2 = 0;
    for (;;) {
        float left = strip_layout(h, c, idx, n, use2, xs);
        if (n <= 1 || left >= AREA_LEFT) break;
        if (use2) { use2 = 0; continue; }
        int worst = n - 1;
        for (int k = 1; k < n; k++)
            if (h->set.h[idx[k]].priority <= h->set.h[idx[worst]].priority) worst = k;
        for (int k = worst; k + 1 < n; k++) idx[k] = idx[k + 1];
        n--;
    }
    h->use_g2 = use2;
    for (int k = 0; k < n; k++) {
        h->shown[k] = idx[k];
        h->rect[k] = (UgRect){xs[k], STRIP_Y - 12, item_w(c, &h->set.h[idx[k]], use2), 24};
    }
    h->nshown = n;
}

void ug_hintbar_update(UgHintBar *h, UgCtx *c, float dt) {
    (void)c;
    ug_anim_update(&h->hover_pulse, dt);
    ug_anim_update(&h->hover_glow, dt);
    ug_anim_update(&h->swap_name, dt);
    ug_anim_update(&h->swap_cash, dt);
    h->swap_timer += dt;
    if (h->swap_timer >= UG_TUNE_STRIP_SWAP_MS) {          /* player name <-> cash, 320 ms cross-fade */
        h->swap_timer = 0;
        UgAnim *out = h->showing_name ? &h->swap_name : &h->swap_cash;
        UgAnim *in = h->showing_name ? &h->swap_cash : &h->swap_name;
        out->props = out->rest;
        in->props = in->rest;
        ug_anim_play(out, UG_ANIM_STRIP_OUT, 0);
        ug_anim_play(in, UG_ANIM_STRIP_IN, 0);
        h->showing_name = !h->showing_name;
    }
}

UgAction ug_hintbar_pointer(UgHintBar *h, UgCtx *c, const UgPtr *p, int *over) {
    int hit=-1;
    if(h->mode==UG_HINTS_PC && !h->no_bg && h->alpha>0.01f)
        for(int k=0;k<h->nshown;k++)if(ug_rect_hit(h->rect[k],p->x,p->y)){hit=k;break;}
    *over=hit>=0;
    if(hit!=h->hover) {
        h->hover=hit;
        if(hit>=0) {
            ug_anim_play(&h->hover_pulse,UG_ANIM_PILL_FOCUS,0);
            ug_anim_play(&h->hover_glow,UG_ANIM_PILL_GLOW,0);
            ug_cue(c,UG_CUE_FOCUS);
        } else {ug_anim_stop(&h->hover_pulse);ug_anim_stop(&h->hover_glow);}
    }
    if(hit>=0 && p->type==UG_PT_DOWN && p->button==1) {
        ug_cue(c,UG_CUE_BUTTON);return h->pc.p[hit].action;
    }
    return UG_A_NONE;
}

/* bottom strip background, SPEC 7.1 (amendment 2): absolute ranges, y 190..222, no overlaps:
 * left end cap x -309..-277 (alpha 215), grey box -277.1..-109.7 (128,128,128, alpha 215),
 * gap, dark box -107..+278 (alpha 150), right end cap +278..+310 (alpha 150). */
static void draw_strip_background(UgCanvas *cv, int with_dark) {
    const float y0 = STRIP_Y - 16.0f;
    /* the data tints the black 60% art (128,128,128) at 215; modulate keeps it dark (Q17, R), so the
     * procedural fallback draws the left box black too, only a little more opaque than the right one */
    UgColor left = UG_RGBA(128, 128, 128, 215), dark = UG_RGBA(0, 0, 0, 150);
    const UgSkin *sk = ug_kit_skin();
    if (sk && sk->cap && sk->box_b) {         /* the original art: black 60% pieces tinted as in the data */
        ug_cv_image(cv, -277, y0, -309, y0 + 32, 0, 0, 1, 1, sk->cap, left);   /* mirrored: arc on the left */
        ug_cv_image_rect(cv, (UgRect){-277.1f, y0, 167.4f, 32}, sk->box_b, left);
        if (!with_dark) return;
        ug_cv_image_rect(cv, (UgRect){-107, y0, 385, 32}, sk->box_b, dark);
        ug_cv_image(cv, 278, y0, 310, y0 + 32, 0, 0, 1, 1, sk->cap, dark);
        return;
    }

}

static void draw_player(UgHintBar *h, UgCtx *c, UgCanvas *cv) {
    UgColor a = ug_anim_color(&h->swap_name), b = ug_anim_color(&h->swap_cash);
    ug_kit_text(c, cv, h->player, -193.4f, STRIP_Y, 0.65f, a, UG_J_CENTER | UG_J_VCENTER);
    ug_kit_text(c, cv, h->cash, -193.4f, STRIP_Y, 0.65f, b, UG_J_CENTER | UG_J_VCENTER);
}

/* PC help-bar pieces are genuine runtime textures, with native bottom-right
 * corners reflected to the two top ends (SPEC 20.2, 7.4.1). */
static void draw_pc(UgHintBar *h,UgCtx *c,UgCanvas *cv) {
    UgPcStripGeom g;ug_pc_strip_geom(&g);const UgSkin *sk=ug_kit_skin();
    if(sk && sk->corner_b && sk->box_b) {
        UgColor left=UG_RGBA(128,128,128,215),right=UG_RGBA(255,255,255,150);
        UgRect r=g.left_corner;
        ug_cv_image(cv,r.x+r.w,r.y+r.h,r.x,r.y,0,0,1,1,sk->corner_b,left);
        ug_cv_image_rect(cv,g.left_box,sk->box_b,left);ug_cv_image_rect(cv,g.player,sk->box_b,left);
        ug_cv_image_rect(cv,g.hint,sk->box_b,right);r=g.right_corner;
        ug_cv_image(cv,r.x,r.y+r.h,r.x+r.w,r.y,0,0,1,1,sk->corner_b,right);
        ug_cv_image_rect(cv,g.right_box,sk->box_b,right);
    } else cv->visual_status=UG_VIS_UNAVAILABLE;
    ug_kit_text(c,cv,h->player,g.player_cx,g.player_cy,0.5f,UG_TEXT_LIGHT,UG_J_CENTER|UG_J_VCENTER);
    for(int k=0;k<h->pc.n;k++) {
        float x=0,y=0;ug_pc_slot_center(k,&x,&y);UgRect r=UG_RECT_C(x,y,144,24);
        int focus=k==h->hover;
        UgColor tint=focus?ug_anim_color(&h->hover_pulse):UG_RGBA(255,255,255,150);
        if(focus)ug_kit_pill_glow(cv,r,ug_anim_color(&h->hover_glow));
        ug_kit_pill_pc(cv,r,tint);
        ug_kit_text(c,cv,h->pc.p[k].label,x,y+UG_R_PC_LABEL_DY,0.45f,
                    focus?UG_ACCENT:UG_WHITE,UG_J_CENTER|UG_J_VCENTER);
    }
}

void ug_hintbar_draw(UgHintBar *h, UgCtx *c, UgCanvas *cv) {
    if(h->alpha<=0.01f || h->no_bg)return; /* authored console strip owns its labels/player too */
    ug_cv_push(cv);ug_cv_mul_alpha(cv,h->alpha);
    if(h->mode==UG_HINTS_PC){draw_pc(h,c,cv);ug_cv_pop(cv);return;}
    if(h->mode!=UG_HINTS_STRIP){ug_cv_pop(cv);return;}
    if(!h->no_bg)draw_strip_background(cv,1);
    if(!h->no_bg)draw_player(h,c,cv);
    for(int k=0;k<h->nshown;k++) {
        const UgHint *hn=&h->set.h[h->shown[k]]; UgRect r=h->rect[k];
        UgGlyph g,unused;ug_hint_glyphs(hn->action,&g,&unused);
        ug_glyph_draw(cv,g,r.x+GLYPH_W*0.5f,STRIP_Y,GLYPH_W,UG_WHITE);
        ug_kit_text(c,cv,hn->label,r.x+GLYPH_W+GLYPH_GAP,STRIP_Y,0.7f,UG_TEXT_HINT,UG_J_VCENTER);
    }
    ug_cv_pop(cv);
}
