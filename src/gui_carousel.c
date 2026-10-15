/* gui_carousel.c -- see gui_carousel.h. Rules: SPEC_GUI_ART 3.2 (layout, tween, navigation, tab,
 * arrows) and 3.3 (edge fade). */
#include "gui_carousel.h"

#include <math.h>
#include <string.h>

void gc_init(GuiCarousel *c, float rail_w) {
    memset(c, 0, sizeof *c);
    c->master_x = 0.388f;            /* OPTION_MASTER centre, 3.1 */
    c->master_y = -14.296f;
    c->rail_cx = 3.0f;               /* ScrollBar_Backing centre */
    c->rail_cy = -52.0f;
    c->rail_w = rail_w > 0 ? rail_w : GC_RAIL_PAUSE;
}
void gc_set_master(GuiCarousel *c, float x, float y) { c->master_x = x; c->master_y = y; }

static float rest_offset(int i) { return -(float)i * GC_PITCH; }          /* s = -x_focus */

void gc_snap_to(GuiCarousel *c, int i) {
    if (i < 0) i = 0;
    if (c->n > 0 && i > c->n - 1) i = c->n - 1;
    c->cur = i;
    c->scroll = c->to = c->from = rest_offset(i);
    c->vel = 0;
    c->t = 0;
    c->tweening = 0;
}

void gc_set_entries(GuiCarousel *c, int n, const int *disabled) {
    if (n < 0) n = 0;
    if (n > GC_MAX) n = GC_MAX;
    c->n = n;
    for (int i = 0; i < GC_MAX; i++) c->disabled[i] = i < n && disabled ? disabled[i] != 0 : 0;
    int first = 0;
    while (first < n - 1 && c->disabled[first]) first++;
    gc_snap_to(c, first);
}

/* d/dt of the Hermite curve, in units per second. Only used to carry the velocity into a retarget. */
static float tween_value(const GuiCarousel *c, float t, float *vel) {
    float u = t / GC_TWEEN_S;
    if (u < 0) u = 0;
    if (u > 1) u = 1;
    float h00 = 2 * u * u * u - 3 * u * u + 1, h10 = u * u * u - 2 * u * u + u;
    float h01 = -2 * u * u * u + 3 * u * u, h11 = u * u * u - u * u;
    float m0 = c->vel * GC_TWEEN_S;                       /* tangent in units over the whole tween */
    if (vel) {
        float d00 = 6 * u * u - 6 * u, d10 = 3 * u * u - 4 * u + 1, d01 = -6 * u * u + 6 * u, d11 = 3 * u * u - 2 * u;
        *vel = (d00 * c->from + d10 * m0 + d01 * c->to + d11 * 0.0f) / GC_TWEEN_S;
    }
    return h00 * c->from + h10 * m0 + h01 * c->to + h11 * 0.0f;
}

int gc_step(GuiCarousel *c, int dir) {
    if (c->n <= 0 || dir == 0) return 0;
    int step = dir < 0 ? -1 : 1, k = c->cur + step;
    while (k >= 0 && k < c->n && c->disabled[k]) k += step;
    if (k < 0 || k >= c->n) return 0;                      /* no wrap: nothing happens at the ends */
    float v = 0;
    float s = c->tweening ? tween_value(c, c->t, &v) : c->scroll;
    if (!c->tweening) v = 0;
    c->cur = k;
    c->from = s;
    c->vel = v;
    c->to = rest_offset(k);
    c->t = 0;
    c->scroll = s;
    c->tweening = 1;
    return 1;
}

void gc_update(GuiCarousel *c, float seconds) {
    if (!c->tweening) return;
    c->t += seconds > 0 ? seconds : 0;
    if (c->t >= GC_TWEEN_S) {
        c->scroll = c->to;
        c->tweening = 0;
        c->vel = 0;
        c->t = 0;
        return;
    }
    c->scroll = tween_value(c, c->t, NULL);
}
int gc_tweening(const GuiCarousel *c) { return c->tweening; }
float gc_scroll_value(const GuiCarousel *c) { return c->scroll; }

void gc_enable_fade(GuiCarousel *c, int on) { c->fade = on != 0; }

GcSlot gc_slot(const GuiCarousel *c, int i) {
    GcSlot s = { 0, 0, 0, 0, 0 };
    if (i < 0 || i >= c->n) return s;
    float k = i == c->cur ? 1.0f : GC_SMALL;
    s.cx = c->master_x + (float)i * GC_PITCH + c->scroll;
    s.cy = c->master_y;
    s.w = GC_ITEM_W * k;
    s.h = GC_ITEM_H * k;
    s.visible = fabsf(s.cx - c->master_x) <= GC_WINDOW;
    return s;
}

float gc_fade_factor(const GuiCarousel *c, int i) {
    if (!c->fade) return 1.0f;
    GcSlot s = gc_slot(c, i);
    float L = c->master_x - GC_WINDOW, R = c->master_x + GC_WINDOW, x = s.cx;
    if (x < L || x > R) return 0.0f;
    if (x < L + GC_FADE_W) return (x - L) / GC_FADE_W;
    if (x > R - GC_FADE_W) return (R - x) / GC_FADE_W;
    return 1.0f;
}
int gc_alpha(const GuiCarousel *c, int i) {
    if (!c->fade) return 255;
    float a = i >= 0 && i < c->n && c->disabled[i] ? 150.0f : 255.0f;
    return (int)(gc_fade_factor(c, i) * a);                /* trunc */
}

int gc_tab_visible(const GuiCarousel *c) { return c->n > 1; }
float gc_tab_x(const GuiCarousel *c) {
    float left = c->rail_cx - c->rail_w / 2;
    if (c->n <= 1) return left + 8.0f;                      /* hidden; rests at the first position */
    return left + 8.0f + (c->rail_w - 16.0f) * (float)c->cur / (float)(c->n - 1);
}
int gc_left_arrow_visible(const GuiCarousel *c) { return c->n > 1 && c->cur > 0; }
int gc_right_arrow_visible(const GuiCarousel *c) { return c->n > 1 && c->cur < c->n - 1; }

int gc_arrow_at(float x, float y) {
    const float h = GC_ARROW_SIZE / 2 + 0.002f;           /* edges inclusive, with float slack for the 3-decimal box edges */
    if (y < GC_ARROW_Y - h || y > GC_ARROW_Y + h) return 0;
    if (x >= GC_ARROW_L_X - h && x <= GC_ARROW_L_X + h) return -1;
    if (x >= GC_ARROW_R_X - h && x <= GC_ARROW_R_X + h) return 1;
    return 0;
}
int gc_slot_hit(const GuiCarousel *c, float x, float y) {
    return fabsf(x - c->master_x) <= GC_ITEM_W / 2 && fabsf(y - c->master_y) <= GC_ITEM_H / 2;
}
GcPtAction gc_pointer_press(const GuiCarousel *c, GcPointer *p, float x, float y) {
    int a = gc_arrow_at(x, y);
    p->armed = 0;
    if (a) {
        if (a < 0 && gc_left_arrow_visible(c)) return GC_PT_LEFT;
        if (a > 0 && gc_right_arrow_visible(c)) return GC_PT_RIGHT;
        return GC_PT_NONE;                                   /* a hidden arrow is inert */
    }
    p->armed = c->n > 0 && gc_slot_hit(c, x, y);
    return GC_PT_NONE;
}
GcPtAction gc_pointer_release(const GuiCarousel *c, GcPointer *p, float x, float y) {
    int armed = p->armed;
    p->armed = 0;
    return armed && gc_slot_hit(c, x, y) ? GC_PT_ACTIVATE : GC_PT_NONE;
}
