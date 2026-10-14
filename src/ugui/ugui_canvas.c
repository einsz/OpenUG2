/* ugui_canvas.c -- vertex accumulation for procedural 2D shapes. */
#include "ugui_canvas.h"
extern const UgSkin *ug_kit_skin(void);
#include <math.h>
#include <stdlib.h>
#include <string.h>

void ug_cv_init(UgCanvas *cv) {
    memset(cv, 0, sizeof *cv);
    cv->st[0] = (UgXf){1, 0, 0, 1, 0, 0, 1};
}

void ug_cv_free(UgCanvas *cv) {
    free(cv->v);
    memset(cv, 0, sizeof *cv);
}

void ug_cv_clear(UgCanvas *cv) {
    cv->n = 0;
    cv->visual_status=UG_VIS_OK;
    cv->sp = 0;
    cv->st[0] = (UgXf){1, 0, 0, 1, 0, 0, 1};
}

UgVisualStatus ug_cv_visual_status(const UgCanvas *cv){return cv->visual_status;}

void ug_cv_push(UgCanvas *cv) {
    if (cv->sp < 31) { cv->st[cv->sp + 1] = cv->st[cv->sp]; cv->sp++; }
}
void ug_cv_pop(UgCanvas *cv) {
    if (cv->sp > 0) cv->sp--;
}

void ug_cv_translate(UgCanvas *cv, float x, float y) {
    UgXf *m = &cv->st[cv->sp];
    m->e += m->a * x + m->c * y;
    m->f += m->b * x + m->d * y;
}
void ug_cv_scale(UgCanvas *cv, float s) {
    UgXf *m = &cv->st[cv->sp];
    m->a *= s; m->b *= s; m->c *= s; m->d *= s;
}
void ug_cv_rotate(UgCanvas *cv, float r) {
    UgXf *m = &cv->st[cv->sp];
    float co = cosf(r), si = sinf(r);
    float a = m->a * co + m->c * si, b = m->b * co + m->d * si;
    float c = -m->a * si + m->c * co, d = -m->b * si + m->d * co;
    m->a = a; m->b = b; m->c = c; m->d = d;
}
void ug_cv_mul_alpha(UgCanvas *cv, float a) {
    if (a < 0) a = 0;
    cv->st[cv->sp].alpha *= a;
}
float ug_cv_alpha(const UgCanvas *cv) { return cv->st[cv->sp].alpha; }
float ug_cv_scale_factor(const UgCanvas *cv) {
    const UgXf *m = &cv->st[cv->sp];
    return sqrtf(m->a * m->a + m->b * m->b);
}
void ug_cv_point(const UgCanvas *cv, float x, float y, float *ox, float *oy) {
    const UgXf *m = &cv->st[cv->sp];
    *ox = m->a * x + m->c * y + m->e;
    *oy = m->b * x + m->d * y + m->f;
}

static UgVertex *reserve(UgCanvas *cv, int n) {
    if (cv->n + n > cv->cap) {
        int nc = cv->cap ? cv->cap * 2 : 4096;
        while (nc < cv->n + n) nc *= 2;
        UgVertex *nv = (UgVertex *)realloc(cv->v, sizeof(UgVertex) * (size_t)nc);
        if (!nv) return NULL;
        cv->v = nv;
        cv->cap = nc;
    }
    UgVertex *p = cv->v + cv->n;
    cv->n += n;
    return p;
}

static void set_color(UgVertex *v, int n, UgColor c, float alpha) {
    uint8_t a = ug_clamp8((float)c.a * alpha);
    for (int i = 0; i < n; i++) { v[i].r = c.r; v[i].g = c.g; v[i].b = c.b; v[i].a = a; }
}

/* Quad centred at (cx,cy) with half extents (hw,hh) in the current space. */
static void shape(UgCanvas *cv, float cx, float cy, float hw, float hh,
                  float rad, float th, float mode, float soft, UgColor c, float extra_pad) {
    float sf = ug_cv_scale_factor(cv);
    if (sf < 1e-6f) return;
    float pad = (1.5f + extra_pad) / sf;
    float ex = hw + pad, ey = hh + pad;
    UgVertex *v = reserve(cv, 6);
    if (!v) return;
    static const int cx_[6] = {-1, 1, 1, -1, 1, -1};
    static const int cy_[6] = {-1, -1, 1, -1, 1, 1};
    for (int i = 0; i < 6; i++) {
        v[i].tex = 0;
        float px = cx + cx_[i] * ex, py = cy + cy_[i] * ey;
        ug_cv_point(cv, px, py, &v[i].x, &v[i].y);
        v[i].lx = cx_[i] * ex * sf;
        v[i].ly = cy_[i] * ey * sf;
        v[i].hw = hw * sf; v[i].hh = hh * sf;
        v[i].rad = rad * sf; v[i].th = th * sf;
        v[i].mode = mode; v[i].soft = soft * sf;
    }
    set_color(v, 6, c, ug_cv_alpha(cv));
}

void ug_cv_fill(UgCanvas *cv, UgRect r, UgColor c, float radius) {
    const UgSkin *sk=ug_kit_skin();
    if(!sk||!sk->box_w){cv->visual_status=UG_VIS_UNAVAILABLE;return;}
    if(radius<=0){ug_cv_image_rect(cv,r,sk->box_w,c);return;}
    if(!sk->corner_w){cv->visual_status=UG_VIS_UNAVAILABLE;return;}
    float k=radius; if(k>r.w/2)k=r.w/2;if(k>r.h/2)k=r.h/2;
    ug_cv_image_rect(cv,(UgRect){r.x+k,r.y,r.w-2*k,r.h},sk->box_w,c);
    ug_cv_image_rect(cv,(UgRect){r.x,r.y+k,k,r.h-2*k},sk->box_w,c);
    ug_cv_image_rect(cv,(UgRect){r.x+r.w-k,r.y+k,k,r.h-2*k},sk->box_w,c);
    ug_cv_image(cv,r.x+k,r.y+k,r.x,r.y,0,0,1,1,sk->corner_w,c);
    ug_cv_image(cv,r.x+r.w-k,r.y+k,r.x+r.w,r.y,0,0,1,1,sk->corner_w,c);
    ug_cv_image(cv,r.x+r.w-k,r.y+r.h-k,r.x+r.w,r.y+r.h,0,0,1,1,sk->corner_w,c);
    ug_cv_image(cv,r.x+k,r.y+r.h-k,r.x,r.y+r.h,0,0,1,1,sk->corner_w,c);
}

void ug_cv_outline(UgCanvas *cv, UgRect r, UgColor c, float radius, float thickness) {
    float hw = r.w * 0.5f, hh = r.h * 0.5f;
    float m = hw < hh ? hw : hh;
    (void)m; (void)radius;
    shape(cv, r.x + hw, r.y + hh, hw, hh, 0, thickness, UG_M_OUTLINE, 0, c, 0);
}

void ug_cv_outline_centered(UgCanvas *cv, UgRect r, UgColor c, float radius, float thickness) {
    float h = thickness * 0.5f;
    ug_cv_outline(cv, (UgRect){r.x - h, r.y - h, r.w + thickness, r.h + thickness}, c, radius + h, thickness);
}

void ug_cv_softring(UgCanvas *cv, UgRect r, UgColor c, float radius, float thickness, float soft) {
    (void)cv; (void)r; (void)c; (void)radius; (void)thickness; (void)soft; /* no substitute art */
}

void ug_cv_glow(UgCanvas *cv, float cx, float cy, float hw, float hh, UgColor c) {
    (void)cv; (void)cx; (void)cy; (void)hw; (void)hh; (void)c; /* no substitute art */
}

void ug_cv_disc(UgCanvas *cv, float cx, float cy, float r, UgColor c) {
    (void)cv; (void)cx; (void)cy; (void)r; (void)c; /* no substitute art */
}

void ug_cv_ring(UgCanvas *cv, float cx, float cy, float r, float th, UgColor c) {
    (void)cv; (void)cx; (void)cy; (void)r; (void)th; (void)c; /* no substitute art */
}

void ug_cv_line(UgCanvas *cv, float x0, float y0, float x1, float y1, float th, UgColor c) {
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    ug_cv_push(cv);
    ug_cv_translate(cv, (x0 + x1) * 0.5f, (y0 + y1) * 0.5f);
    ug_cv_rotate(cv, atan2f(dy, dx));
    ug_cv_fill(cv, UG_RECT_C(0, 0, len + th, th), c, th * 0.5f);
    ug_cv_pop(cv);
}

void ug_cv_flat_rect(UgCanvas *cv, UgRect r, UgColor col) {
    UgVertex *v = reserve(cv, 6);
    if (!v) return;
    float xs[4] = {r.x, r.x + r.w, r.x + r.w, r.x};
    float ys[4] = {r.y, r.y, r.y + r.h, r.y + r.h};
    static const int idx[6] = {0, 1, 2, 0, 2, 3};
    for (int i = 0; i < 6; i++) {
        memset(&v[i], 0, sizeof v[i]);
        ug_cv_point(cv, xs[idx[i]], ys[idx[i]], &v[i].x, &v[i].y);
        v[i].mode = UG_M_FLAT;
    }
    set_color(v, 6, col, ug_cv_alpha(cv));
}

void ug_cv_quarter(UgCanvas *cv, UgRect r, int corner, UgColor c) {
    (void)cv; (void)r; (void)corner; (void)c; /* no substitute art */
}

void ug_cv_tri(UgCanvas *cv, float x0, float y0, float x1, float y1, float x2, float y2, UgColor c) {
    UgVertex *v = reserve(cv, 3);
    if (!v) return;
    float px[3] = {x0, x1, x2}, py[3] = {y0, y1, y2};
    for (int i = 0; i < 3; i++) {
        memset(&v[i], 0, sizeof v[i]);
        ug_cv_point(cv, px[i], py[i], &v[i].x, &v[i].y);
        v[i].mode = UG_M_FLAT;
    }
    set_color(v, 3, c, ug_cv_alpha(cv));
}

void ug_cv_gradient(UgCanvas *cv, UgRect r, UgColor tl, UgColor tr, UgColor br, UgColor bl) {
    UgVertex *v = reserve(cv, 6);
    if (!v) return;
    float xs[4] = {r.x, r.x + r.w, r.x + r.w, r.x};
    float ys[4] = {r.y, r.y, r.y + r.h, r.y + r.h};
    UgColor cs[4] = {tl, tr, br, bl};
    static const int idx[6] = {0, 1, 2, 0, 2, 3};
    for (int i = 0; i < 6; i++) {
        int k = idx[i];
        memset(&v[i], 0, sizeof v[i]);
        ug_cv_point(cv, xs[k], ys[k], &v[i].x, &v[i].y);
        v[i].mode = UG_M_FLAT;
        set_color(&v[i], 1, cs[k], ug_cv_alpha(cv));
        v[i].r = cs[k].r; v[i].g = cs[k].g; v[i].b = cs[k].b;
    }
}

void ug_cv_spinner(UgCanvas *cv, float cx, float cy, float radius, float th,
                   float phase, UgColor c) {
    (void)cv; (void)cx; (void)cy; (void)radius; (void)th; (void)phase; (void)c; /* no substitute art */
}

void ug_cv_image(UgCanvas *cv, float x0, float y0, float x1, float y1,
                 float u0, float v0, float u1, float v1, int tex, UgColor c) {
    if (tex <= 0) {cv->visual_status=UG_VIS_UNAVAILABLE;return;}
    UgVertex *v = reserve(cv, 6);
    if (!v) return;
    float xs[4] = {x0, x1, x1, x0}, ys[4] = {y0, y0, y1, y1};
    float us[4] = {u0, u1, u1, u0}, vs[4] = {v0, v0, v1, v1};
    static const int idx[6] = {0, 1, 2, 0, 2, 3};
    for (int i = 0; i < 6; i++) {
        int k = idx[i];
        memset(&v[i], 0, sizeof v[i]);
        ug_cv_point(cv, xs[k], ys[k], &v[i].x, &v[i].y);
        v[i].lx = us[k]; v[i].ly = vs[k];
        v[i].mode = UG_M_IMAGE;
        v[i].tex = tex;
    }
    set_color(v, 6, c, ug_cv_alpha(cv));
}

void ug_cv_image_rect(UgCanvas *cv, UgRect r, int tex, UgColor tint) {
    ug_cv_image(cv, r.x, r.y, r.x + r.w, r.y + r.h, 0, 0, 1, 1, tex, tint);
}

void ug_cv_text_quad(UgCanvas *cv, float x0, float y0, float x1, float y1,
                     float u0, float v0, float u1, float v1,
                     float base_y, float shear, UgColor c) {
    (void)cv; (void)x0; (void)y0; (void)x1; (void)y1; (void)u0; (void)v0; (void)u1; (void)v1; (void)base_y; (void)shear; (void)c; /* no substitute art */
}

UgView ug_view_fit(int ww, int wh) {
    UgView v;
    float sx = (float)ww / UG_SCREEN_W, sy = (float)wh / UG_SCREEN_H;
    v.scale = sx < sy ? sx : sy;
    v.w = (int)(UG_SCREEN_W * v.scale + 0.5f);
    v.h = (int)(UG_SCREEN_H * v.scale + 0.5f);
    v.x0 = (ww - v.w) / 2;
    v.y0 = (wh - v.h) / 2;
    return v;
}

int ug_view_contains(const UgView *v, int wx, int wy) {
    return wx >= v->x0 && wx < v->x0 + v->w && wy >= v->y0 && wy < v->y0 + v->h;
}

int ug_virtual_visible(float x, float y) {
    return x >= -UG_SCREEN_W * 0.5f && x <= UG_SCREEN_W * 0.5f && y >= -UG_SCREEN_H * 0.5f && y <= UG_SCREEN_H * 0.5f;
}

void ug_view_to_virtual(const UgView *v, int wx, int wy, float *x, float *y) {
    *x = ((float)(wx - v->x0) / (float)v->w - 0.5f) * UG_SCREEN_W;
    *y = ((float)(wy - v->y0) / (float)v->h - 0.5f) * UG_SCREEN_H;
}
