/* Original-resource visual kit, SPEC 20.1. Missing resources are omitted. */
#include "ugui_ui.h"
#include <math.h>
#include <string.h>
#include "ugui_tuning.h"

#define PI_F 3.14159265f

/* ---- skin: original art for the kit roles when a texture library is loaded ---- */
static UgSkin g_skin;
static int g_has_skin;
void ug_kit_set_skin(const UgSkin *s) { if (s) { g_skin = *s; g_has_skin = 1; } else g_has_skin = 0; }
const UgSkin *ug_kit_skin(void) { return g_has_skin ? &g_skin : NULL; }

/* The original box and corner textures already carry 60% alpha (SPEC 1.4 table); the
 * kit's colours were written as final alpha, so convert them to a tint alpha. */
static UgColor tint_from_final(UgColor c) {
    int a = (int)c.a * 255 / 153;
    c.a = (uint8_t)(a > 255 ? 255 : a);
    return c;
}

void ug_kit_text(UgCtx *c, UgCanvas *cv, const char *s, float x, float y,
                 float scale, UgColor col, unsigned just) {
    ug_text_draw(cv, s, x, y, scale, col, just, 0, 0, c->glyphs);
}

void ug_kit_textw(UgCtx *c, UgCanvas *cv, const char *s, float x, float y,
                  float scale, UgColor col, unsigned just, float wrap) {
    ug_text_draw(cv, s, x, y, scale, col, just, wrap, 0, c->glyphs);
}

void ug_kit_flat(UgCanvas *cv, UgRect r, UgColor c) { ug_cv_flat_rect(cv, r, c); }

void ug_kit_title(UgCtx *c, UgCanvas *cv, const char *s) {
    /* capitals start about 6 units into the cell and the text is centred, so the centre lies 10 x scale
     * below the cap top (16.5) */
    ug_text_draw(cv, s, UG_R_TITLE_INK_LEFT - 0.5f, UG_R_TITLE_CAPTOP + 10.0f * UG_R_TITLE_SCALE, UG_R_TITLE_SCALE,
                 UG_ACCENT, UG_J_VCENTER, 0, 0, c->glyphs);
}

/* Native bottom-right corner. Signed geometry retains its UV association. */
static void skin_corner(UgCanvas *cv,float cx,float cy,float sx,float sy,UgColor tint){
    ug_cv_image(cv,cx-sx/2,cy-sy/2,cx+sx/2,cy+sy/2,0,0,1,1,g_skin.corner_w,tint);
}

void ug_kit_corner(UgCanvas *cv, float cx, float cy, float sx, float sy, UgColor c) {
    if (g_has_skin && g_skin.corner_w) { skin_corner(cv, cx, cy, sx, sy, tint_from_final(c)); return; }
    cv->visual_status=UG_VIS_UNAVAILABLE;
}

void ug_kit_box(UgCanvas *cv, UgRect r, int light, float alpha01) {
    if (g_has_skin && g_skin.box_w) {
        UgColor t = light ? UG_RGBA(255, 255, 255, 255) : UG_RGBA(0, 0, 0, 255);
        t.a = ug_clamp8(255.0f * alpha01);
        ug_cv_image_rect(cv, r, g_skin.box_w, t);
        return;
    }
    cv->visual_status=UG_VIS_UNAVAILABLE;
}

void ug_kit_panel(UgCanvas *cv, UgRect r, int light, float alpha01) {
    if (g_has_skin && g_skin.box_w && g_skin.corner_w) {
        UgColor t = light ? UG_RGBA(255, 255, 255, 255) : UG_RGBA(0, 0, 0, 255);
        t.a = ug_clamp8(255.0f * alpha01);
        float k = 16.0f;
        if (r.w < 2 * k || r.h < 2 * k) { ug_kit_corner(cv, r.x + r.w / 2, r.y + r.h / 2, r.w, r.h, tint_from_final(ug_color_alpha(t, 0.6f))); return; }
        float x1 = r.x + r.w - k, y1 = r.y + r.h - k;
        int bx = g_skin.box_w;
        ug_cv_image_rect(cv, (UgRect){r.x + k, r.y + k, r.w - 2 * k, r.h - 2 * k}, bx, t);
        ug_cv_image_rect(cv, (UgRect){r.x + k, r.y, r.w - 2 * k, k}, bx, t);
        ug_cv_image_rect(cv, (UgRect){r.x + k, y1, r.w - 2 * k, k}, bx, t);
        ug_cv_image_rect(cv, (UgRect){r.x, r.y + k, k, r.h - 2 * k}, bx, t);
        ug_cv_image_rect(cv, (UgRect){x1, r.y + k, k, r.h - 2 * k}, bx, t);
        skin_corner(cv, r.x + k / 2, r.y + k / 2, -k, -k, t);
        skin_corner(cv, x1 + k / 2, r.y + k / 2, k, -k, t);
        skin_corner(cv, x1 + k / 2, y1 + k / 2, k, k, t);
        skin_corner(cv, r.x + k / 2, y1 + k / 2, -k, k, t);
        return;
    }
    cv->visual_status=UG_VIS_UNAVAILABLE;
}

void ug_kit_pill_pc(UgCanvas *cv, UgRect r, UgColor tint) {
    if(g_has_skin && g_skin.pill)ug_cv_image_rect(cv,r,g_skin.pill,tint);
    else cv->visual_status=UG_VIS_UNAVAILABLE;
}

void ug_kit_pill_glow(UgCanvas *cv, UgRect r, UgColor tint) {
    if(g_has_skin && g_skin.pill_glow)ug_cv_image_rect(cv,r,g_skin.pill_glow,tint);
    else cv->visual_status=UG_VIS_UNAVAILABLE;
}

/* dialog / strip button body: black box with an end cap on each side (SPEC 6.8) */
void ug_kit_cap_box(UgCanvas *cv, UgRect r, float cap_w, UgColor tint, UgColor fallback_rim, float rim_w) {
    (void)fallback_rim; (void)rim_w;
    if (g_has_skin && g_skin.cap && g_skin.box_b) {
        /* the cap art has its flat edge on the left and the arc on the right: the left cap is
         * mirrored, the right one native; each is about 2/3 opaque (the rest is clear) */
        ug_cv_image(cv, r.x + cap_w, r.y, r.x, r.y + r.h, 0, 0, 1, 1, g_skin.cap, tint);
        ug_cv_image(cv, r.x + r.w - cap_w, r.y, r.x + r.w, r.y + r.h, 0, 0, 1, 1, g_skin.cap, tint);
        ug_cv_image_rect(cv, (UgRect){r.x + cap_w, r.y, r.w - 2 * cap_w, r.h}, g_skin.box_b, tint);
        return;
    }
    cv->visual_status=UG_VIS_UNAVAILABLE;
}

void ug_kit_pill(UgCanvas *cv, UgRect r, UgColor fill, UgColor rim, float rim_w) {
    ug_cv_fill(cv, r, fill, r.h * 0.5f);
    if (rim_w > 0) ug_cv_outline(cv, r, rim, r.h * 0.5f, rim_w);
}

void ug_kit_slider_track(UgCanvas *cv, UgRect r) {
    ug_cv_fill(cv, r, UG_RGBA(255, 255, 255, 40), r.h * 0.3f);
    ug_cv_outline(cv, r, UG_RGBA(255, 255, 255, 40), r.h * 0.3f, 1.0f);
}

void ug_kit_badge(UgCanvas *cv, float cx, float cy, float size, int dir, UgColor c, int solid) {
    int bt = solid ? g_skin.arrow_w : g_skin.arrow;
    if (g_has_skin && bt) {
        static const float rot[4] = {PI_F, 0.0f, -PI_F * 0.5f, PI_F * 0.5f};
        ug_cv_push(cv);
        ug_cv_translate(cv, cx, cy);
        ug_cv_rotate(cv, rot[dir & 3]);
        ug_cv_image_rect(cv, UG_RECT_C(0, 0, size, size), bt, c);
        ug_cv_pop(cv);
        return;
    }
    cv->visual_status=UG_VIS_UNAVAILABLE;
}

void ug_kit_frame(UgCanvas *cv, UgRect r, UgColor c, float alpha01) {
    UgColor a = ug_color_alpha(c, alpha01);
    ug_cv_softring(cv, r, ug_color_alpha(c, alpha01 * 0.55f), 8.0f, 3.0f, 2.2f);
    ug_cv_outline(cv, r, a, 6.0f, 2.0f);
}

void ug_kit_lock(UgCanvas *cv, float cx, float cy, float size, UgColor c) {
    cv->visual_status=UG_VIS_UNAVAILABLE; (void)c; (void)cx; (void)cy; (void)size;
}

void ug_kit_symbol(UgCanvas *cv, int sym, float cx, float cy, float size, UgColor c) {
    cv->visual_status=UG_VIS_UNAVAILABLE; (void)c; (void)cx; (void)cy; (void)size; (void)sym;
}

void ug_kit_cursor(UgCanvas *cv, float x, float y) {
    /* SPEC 4.6 / 6.21 (R): 32 x 32 virtual units, hot spot at the top-left tip, on top of everything.
     * Clipping to the 4:3 viewport is done by the GL scissor; a cursor that lies wholly outside the
     * screen is not emitted at all. */
    if (x >= UG_SCREEN_W * 0.5f || y >= UG_SCREEN_H * 0.5f || x + UG_CURSOR_SIZE <= -UG_SCREEN_W * 0.5f ||
        y + UG_CURSOR_SIZE <= -UG_SCREEN_H * 0.5f)
        return;
    if (g_has_skin && g_skin.cursor) {
        ug_cv_image_rect(cv, (UgRect){x, y, UG_CURSOR_SIZE, UG_CURSOR_SIZE}, g_skin.cursor, UG_WHITE);
        return;
    }
    cv->visual_status=UG_VIS_UNAVAILABLE;
}
