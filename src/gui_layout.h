/* gui_layout.h -- placement of the authored option rows and sliders (SPEC_GUI_ART 4.2, 4.3, 4.5,
 * tvg09). Pure geometry, no GL/SDL, no package access. Anchors and object sizes come from the
 * authored package at run time; these functions only apply the rules. Centred virtual units. */
#ifndef OPENUG2_GUI_LAYOUT_H
#define OPENUG2_GUI_LAYOUT_H

#ifdef __cplusplus
extern "C" {
#endif

#define GL_ROW_OFFSCREEN_Y 6969.0f   /* rows outside the window are parked here (4.2) */
#define GL_SLIDER_MARGIN 46.0f       /* knob/fill margin inside the slider base (4.3) */
#define GL_BACKING_DX 276.0f         /* backing group at (tx + 276, ry + 10) */
#define GL_BACKING_DY 10.0f

typedef struct GlPt { float x, y; } GlPt;

/* ---- virtual screen to pixels (SPEC_GUI_ART_AMEND1 A1) --------------------------------------------
 * Every position of the specs is in centred 640x480 units (origin at the screen centre, x right, y
 * down). The mapping to a W x H output is a uniform scale about the centre:
 *   s = min(W/640, H/480), px = W/2 + s*X, py = H/2 + s*Y; sizes scale by s on both axes.
 * 16:9 outputs are pillarboxed; below 4:3 the same `min` rule letterboxes (guessed, AMEND1 C1). The
 * z component of a position never changes x/y (no perspective). No rounding is defined. */
typedef struct GlPxMap { float s, ox, oy; } GlPxMap;
GlPxMap gl_px_map(int w, int h);                       /* s = 0 for a non-positive output */
GlPt gl_px_point(const GlPxMap *m, float x, float y);  /* virtual point -> pixel */
float gl_px_len(const GlPxMap *m, float v);            /* virtual length -> pixels */

/* TITLE_MASTER and DATA_MASTER as top-left + size. */
typedef struct GlAnchors {
    float tx, ty, tw, th;
    float dx, dy, dw, dh;
} GlAnchors;

/* Anchors from object centres and sizes (the authored form is centre + size). */
GlAnchors gl_anchors_from_centres(float tcx, float tcy, float tw, float th,
                                  float dcx, float dcy, float dw, float dh);

float gl_row_pitch(const GlAnchors *a);               /* = TITLE_MASTER height */
float gl_row_top(const GlAnchors *a, int k);          /* ry = ty + k * pitch (k = visible line, 0-based) */
float gl_data_top(const GlAnchors *a, int k);         /* dy_k = dy + k * pitch */
float gl_data_cx(const GlAnchors *a);                 /* dcx = dx + dw/2 */
GlPt gl_label_tl(const GlAnchors *a, int k);          /* name text anchor (tx, ry) */
float gl_label_centre_x(const GlAnchors *a);          /* tx + tw/2 for a centre-justified name */
GlPt gl_backing_pos(const GlAnchors *a, int k);       /* group position (tx + 276, ry + 10) */
GlPt gl_left_arrow_tl(const GlAnchors *a, int k, float arrow_w);   /* (dx - arrow_w, dy_k) */
GlPt gl_right_arrow_tl(const GlAnchors *a, int k);                 /* (dx + dw, dy_k) */
GlPt gl_slider_base_tl(const GlAnchors *a, int k, float base_w);   /* (dcx - base_w/2, dy_k) */

typedef struct GlSlider {
    GlPt knob_c;                 /* knob centre */
    GlPt fill_tl;                /* fill top-left */
    float fill_w;                /* fill width = f * W (its height stays authored) */
    float usable_w;              /* W = bw - 2m */
} GlSlider;
/* f = (v - min)/(max - min), clamped to 0..1 by the caller. */
GlSlider gl_slider(GlPt base_tl, float bw, float bh, float kh, float f);

/* Vertical scrollbar tab centre y (4.5): rail_top + 8 + (rail_h - 16)(c-1)/(N-1); c is 1-based. */
float gl_scroll_tab_y(float rail_cy, float rail_h, int c, int n);
int gl_scroll_visible(int n);    /* tab, arrows and backing exist when N > 1 */

#ifdef __cplusplus
}
#endif
#endif
