/* gui_layout.c -- see gui_layout.h. Rules: AMEND1 A1 (pixel mapping), SPEC_GUI_ART 4.2 (row placement), 4.3 (slider drawing),
 * 4.5 (vertical scrollbar tab). */
#include "gui_layout.h"

GlAnchors gl_anchors_from_centres(float tcx, float tcy, float tw, float th,
                                  float dcx, float dcy, float dw, float dh) {
    GlAnchors a = { tcx - tw / 2, tcy - th / 2, tw, th, dcx - dw / 2, dcy - dh / 2, dw, dh };
    return a;
}

GlPxMap gl_px_map(int w, int h) {
    GlPxMap m = { 0, 0, 0 };
    if (w <= 0 || h <= 0) return m;
    float sx = (float)w / 640.0f, sy = (float)h / 480.0f;
    m.s = sx < sy ? sx : sy;
    m.ox = (float)w * 0.5f;
    m.oy = (float)h * 0.5f;
    return m;
}
GlPt gl_px_point(const GlPxMap *m, float x, float y) {
    GlPt p = { m->ox + m->s * x, m->oy + m->s * y };
    return p;
}
float gl_px_len(const GlPxMap *m, float v) { return m->s * v; }

float gl_row_pitch(const GlAnchors *a) { return a->th; }
float gl_row_top(const GlAnchors *a, int k) { return a->ty + (float)k * a->th; }
float gl_data_top(const GlAnchors *a, int k) { return a->dy + (float)k * a->th; }
float gl_data_cx(const GlAnchors *a) { return a->dx + a->dw / 2; }
GlPt gl_label_tl(const GlAnchors *a, int k) { GlPt p = { a->tx, gl_row_top(a, k) }; return p; }
float gl_label_centre_x(const GlAnchors *a) { return a->tx + a->tw / 2; }
GlPt gl_backing_pos(const GlAnchors *a, int k) {
    GlPt p = { a->tx + GL_BACKING_DX, gl_row_top(a, k) + GL_BACKING_DY };
    return p;
}
GlPt gl_left_arrow_tl(const GlAnchors *a, int k, float arrow_w) {
    GlPt p = { a->dx - arrow_w, gl_data_top(a, k) };
    return p;
}
GlPt gl_right_arrow_tl(const GlAnchors *a, int k) {
    GlPt p = { a->dx + a->dw, gl_data_top(a, k) };
    return p;
}
GlPt gl_slider_base_tl(const GlAnchors *a, int k, float base_w) {
    GlPt p = { gl_data_cx(a) - base_w / 2, gl_data_top(a, k) };
    return p;
}

GlSlider gl_slider(GlPt base_tl, float bw, float bh, float kh, float f) {
    GlSlider s;
    s.usable_w = bw - 2 * GL_SLIDER_MARGIN;
    s.knob_c.x = base_tl.x + GL_SLIDER_MARGIN + f * s.usable_w;
    s.knob_c.y = base_tl.y + bh / 2 - 1.0f;
    s.fill_tl.x = base_tl.x + GL_SLIDER_MARGIN;
    s.fill_tl.y = base_tl.y + bh / 2 - kh / 2 + 1.0f;
    s.fill_w = f * s.usable_w;
    return s;
}

float gl_scroll_tab_y(float rail_cy, float rail_h, int c, int n) {
    float top = rail_cy - rail_h / 2;
    if (n <= 1) return top + 8.0f;
    return top + 8.0f + (rail_h - 16.0f) * (float)(c - 1) / (float)(n - 1);
}
int gl_scroll_visible(int n) { return n > 1; }
