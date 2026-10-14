/* ugui_canvas.h -- GL-free drawing surface, original-resource text layout.
 *
 * Widgets draw into a UgCanvas, which only accumulates triangles. The GL
 * backend renders the accumulated vertex list. Textured UI art requires
 * user-supplied original resources; specified geometry is independent. */
#ifndef UGUI_CANVAS_H
#define UGUI_CANVAS_H

#include "ugui_core.h"
#include "ugui_face.h"
#include "ugui_controller.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { float x, y, w, h; } UgRect;   /* x,y = top-left, +y down */
#define UG_RECT_C(cx, cy, w, h) ((UgRect){(cx) - (w) / 2.0f, (cy) - (h) / 2.0f, (w), (h)})

static inline int ug_rect_hit(UgRect r, float x, float y) {
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

/* virtual screen (SPEC 1.1): origin at the centre, +y down */
#define UG_SCREEN_W 640.0f
#define UG_SCREEN_H 480.0f

/* Presentation of the virtual screen in a window (SPEC 1.1, amendment 3, R): a centred 4:3
 * viewport, scale = min(W/640, H/480); the bars at the sides (or top and bottom) are pure
 * black and nothing is drawn into them, not even the pointer cursor (the GL backend scissors
 * to the viewport; the window is cleared to black first).
 * `scale` = pixels per virtual unit; (x0,y0,w,h) = viewport in pixels. */
typedef struct { float scale; int x0, y0, w, h; } UgView;
UgView ug_view_fit(int win_w, int win_h);
void   ug_view_to_virtual(const UgView *v, int win_x, int win_y, float *x, float *y);
/* 1 when the window pixel lies inside the 4:3 viewport (not in a black bar) */
int    ug_view_contains(const UgView *v, int win_x, int win_y);
/* 1 when a virtual point lies inside the visible screen (|x| <= 320, |y| <= 240) */
int    ug_virtual_visible(float x, float y);

/* ---- vertex format shared with the GL backend ---- */
enum { UG_M_FILL = 0, UG_M_OUTLINE, UG_M_TEXT, UG_M_GLOW, UG_M_SOFTRING,
       UG_M_FLAT, UG_M_SPINNER, UG_M_QUARTER,
       UG_M_IMAGE /* RGBA texture from the texture library x vertex colour */ };
typedef struct {
    float x, y;          /* position (virtual units, after transform) */
    float lx, ly;        /* local coords (uv for UG_M_TEXT) */
    float hw, hh;        /* shape half extents */
    float rad, th;       /* corner radius, outline thickness */
    float mode, soft;    /* UG_M_*, softness / spinner phase */
    uint8_t r, g, b, a;
    int32_t tex;         /* UG_M_IMAGE: texture library index + 1; otherwise 0 */
} UgVertex;

typedef struct { float a, b, c, d, e, f, alpha; } UgXf;

typedef struct {
    UgVertex *v; int n, cap;
    UgVisualStatus visual_status; /* unavailable when an image/font is omitted */
    UgXf st[32]; int sp;
} UgCanvas;

void ug_cv_init(UgCanvas *cv);
void ug_cv_free(UgCanvas *cv);
UgVisualStatus ug_cv_visual_status(const UgCanvas *cv);
void ug_cv_clear(UgCanvas *cv);               /* start of frame */

void ug_cv_push(UgCanvas *cv);
void ug_cv_pop(UgCanvas *cv);
void ug_cv_translate(UgCanvas *cv, float x, float y);
void ug_cv_scale(UgCanvas *cv, float s);
void ug_cv_rotate(UgCanvas *cv, float radians);
void ug_cv_mul_alpha(UgCanvas *cv, float a01);
float ug_cv_alpha(const UgCanvas *cv);
float ug_cv_scale_factor(const UgCanvas *cv);
void ug_cv_point(const UgCanvas *cv, float x, float y, float *ox, float *oy);

void ug_cv_fill(UgCanvas *cv, UgRect r, UgColor c, float radius);
void ug_cv_outline(UgCanvas *cv, UgRect r, UgColor c, float radius, float thickness);
/* outline whose line *centre* runs along the edges of `r` (the measured dialog border and focus
 * frame of SPEC 19.2 give line centres) */
void ug_cv_outline_centered(UgCanvas *cv, UgRect r, UgColor c, float radius, float thickness);
/* blurry outline, used for highlight glow */
void ug_cv_softring(UgCanvas *cv, UgRect r, UgColor c, float radius, float thickness, float soft);
void ug_cv_glow(UgCanvas *cv, float cx, float cy, float hw, float hh, UgColor c);
void ug_cv_disc(UgCanvas *cv, float cx, float cy, float radius, UgColor c);
void ug_cv_ring(UgCanvas *cv, float cx, float cy, float radius, float thickness, UgColor c);
void ug_cv_line(UgCanvas *cv, float x0, float y0, float x1, float y1, float thickness, UgColor c);
/* hard-edged rectangle (no anti-aliased border): seamless when tiled */
void ug_cv_flat_rect(UgCanvas *cv, UgRect r, UgColor c);
/* quarter disc filling a box; the arc faces the given outer corner
 * (0 TL, 1 TR, 2 BR, 3 BL). Straight edges are hard so pieces tile. */
void ug_cv_quarter(UgCanvas *cv, UgRect r, int corner, UgColor c);
void ug_cv_tri(UgCanvas *cv, float x0, float y0, float x1, float y1, float x2, float y2, UgColor c);
/* four corner colours TL TR BR BL (SPEC 6.1 colour-gradient image) */
void ug_cv_gradient(UgCanvas *cv, UgRect r, UgColor tl, UgColor tr, UgColor br, UgColor bl);
/* ring with a fading tail, `phase` 0..1 is the head position */
void ug_cv_spinner(UgCanvas *cv, float cx, float cy, float radius, float thickness,
                   float phase, UgColor c);
/* Quad of a library texture (SPEC 15.9): corners (x0,y0)-(x1,y1) in the current
 * space, uv rectangle (u0,v0)-(u1,v1) with v down; swap uv or corners to mirror.
 * `tex` is the texture library index + 1. The texel is multiplied by `c`. */
void ug_cv_image(UgCanvas *cv, float x0, float y0, float x1, float y1,
                 float u0, float v0, float u1, float v1, int tex, UgColor c);
/* textured quad from the font atlas; shear skews the top edge to the right by
 * shear*(base_y - y). Used by the text renderer. */
void ug_cv_text_quad(UgCanvas *cv, float x0, float y0, float x1, float y1,
                     float u0, float v0, float u1, float v1,
                     float base_y, float shear, UgColor c);

/* ------------------------------------------------------------------ */
/* Skin: original art for the visual kit roles (SPEC 5), resolved from the */
/* user's install at run time; every field is a texture library index + 1, */
/* 0 = unavailable (the affected image is omitted).            */
/* ------------------------------------------------------------------ */
typedef struct {
    int box_b, box_w;           /* 16x16 solid boxes, black / white at 60% */
    int corner_b, corner_w;     /* 16x16 rounded corner pieces */
    int cap;                    /* 32x32 end cap of strips and buttons (black 60%) */
    int arrow, arrow_w, arrow_hl;   /* 32x32 right-pointing badge, white badge, ring */
    int scr_tab, scr_dot;       /* scroller thumb 16x16, end dot 8x8 */
    int icon_hl, icon_back;     /* 128x64 icon highlight and backing */
    int slider_bg, slider_fill_corner, slider_knob, white;   /* slider parts, solid white */
    int pill, pill_glow;        /* PC pill button and its glow (256x32) */
    int save;                   /* 32x32 memory-card icon */
    int cursor;                 /* 32x32 pointer cursor, key hash("PC_CURSOR") (SPEC 4.6, R) */
    float slider_bg_dy;         /* the slider backing's bar lies this fraction of its height below (+) or above (-) the
                                   quad centre (measured on the decoded texture at load); the fill is centred on the bar */
} UgSkin;
#define UG_CURSOR_TEXTURE "PC_CURSOR"
#define UG_CURSOR_SIZE 32.0f    /* virtual units (SPEC 4.6, 6.21, R) */
/* NULL = no resource art available. The pointer's contents are copied. */
void ug_kit_set_skin(const UgSkin *s);
const UgSkin *ug_kit_skin(void);        /* NULL when none */
/* Axis-aligned image of a skin texture over rect r (uv 0..1) with a tint. */
void ug_cv_image_rect(UgCanvas *cv, UgRect r, int tex, UgColor tint);

/* ------------------------------------------------------------------ */
/* Built-in font (original 5x7 pixel design, atlas generated at init)   */
/* ------------------------------------------------------------------ */
#define UG_LINE_PITCH 26.0f    /* SPEC 8.1 at scale 1.0 */
#define UG_CAP_HEIGHT 20.0f
#define UG_BASELINE   22.0f    /* baseline offset from the line top */

typedef struct {
    uint32_t cp;
    int16_t col, row;          /* cell origin in atlas texels */
    uint8_t w, adv;            /* ink width / advance, in font columns */
} UgGlyphInfo;

typedef struct {
    uint8_t *atlas; int aw, ah;       /* 8-bit coverage */
    UgGlyphInfo *g; int ng;           /* sorted by code point */
    int cell_w, cell_h;               /* atlas cell size in texels */
} UgFont;

const UgFont *ug_font(void);                       /* built once, thread-unsafe */
const UgGlyphInfo *ug_font_glyph(const UgFont *f, uint32_t cp);  /* NULL if none */
float ug_font_adv(const UgFont *f, uint32_t cp);   /* units at scale 1 */
#define UG_COL_W 2.45f       /* one font column in units (x) */
#define UG_ROW_H (20.0f / 7.0f)   /* one font row in units (y) */

/* ------------------------------------------------------------------ */
/* Inline glyph tokens rendered via explicit controller profiles (SPEC 20.3) */
/* ------------------------------------------------------------------ */
typedef enum {
    UG_G_NONE = 0, UG_G_SPACER,
    UG_G_MOUSE_LEFT, UG_G_MOUSE_RIGHT, UG_G_MOUSE_WHEEL,
    UG_G_WHEEL_UP, UG_G_WHEEL_DOWN, UG_G_MOUSE_HOVER, UG_G_MOUSE_DRAG,
    UG_G_KEY_ENTER, UG_G_KEY_ESC, UG_G_KEY_BACK, UG_G_KEY_CONTINUE,
    UG_G_KEY_HELP, UG_G_KEY_DELETE, UG_G_KEY_LETTER, UG_G_KEY_PUNCT,
    UG_G_ARROW_LEFT, UG_G_ARROW_RIGHT, UG_G_ARROW_UP, UG_G_ARROW_DOWN,
    UG_G_ARROW_UPDOWN, UG_G_ARROW_LEFTRIGHT, UG_G_SLASH, UG_G_COUNT
} UgGlyphId;

typedef struct { UgGlyphId id; char ch; } UgGlyph;   /* ch for letter/punct caps */

typedef enum { UG_GLYPHS_MOUSE = 0, UG_GLYPHS_KEYS } UgGlyphMode;

/* `name` is the text between the dollar signs, e.g. JOY_EVENT_FENG_SELECT. */
void ug_glyph_set_controller(UgController *p);
const UgController *ug_glyph_controller(void);
int  ug_glyph_for_token(const char *name, UgGlyphMode mode, UgGlyph *out);
void ug_glyph_draw(UgCanvas *cv, UgGlyph g, float cx, float cy, float size, UgColor c);
const char *ug_glyph_name(UgGlyphId id);

/* ------------------------------------------------------------------ */
/* Text (SPEC 8)                                                       */
/* ------------------------------------------------------------------ */
#define UG_J_CENTER   0x1   /* SPEC 8.4 */
#define UG_J_RIGHT    0x2
#define UG_J_VCENTER  0x4
#define UG_J_BOTTOM   0x8

typedef struct {
    int is_glyph;           /* inline glyph token */
    UgGlyph glyph;
    uint32_t cp;
    float x, w;             /* unscaled pen position within the line (after kerning), advance */
    float kern;             /* kerning adjustment applied before this item (16.5), 0 at line start */
    int missing;            /* face has no glyph for cp: skipped, resets kerning (16.5 step 1) */
} UgTextItem;

typedef struct { int first, count; float width; } UgTextLine;

typedef struct {
    UgTextItem *items; int n, cap;
    UgTextLine *lines; int nlines;
    float width, height;    /* unscaled; height = (lines-1) x pitch + cell height (16.5) */
    float pitch;            /* line pitch incl. leading */
} UgTextLayout;

void ug_text_layout(UgTextLayout *tl, const char *utf8, float wrap_w,
                    float leading, UgGlyphMode mode);
void ug_text_layout_free(UgTextLayout *tl);
/* Extent at `scale` (width = widest line, height = block). */
void ug_text_measure(const char *utf8, float scale, float wrap_w, float leading,
                     UgGlyphMode mode, float *w, float *h);
/* Anchor (x,y) per SPEC 8.4 justification bits. */
/* Demo filled ink: original atlas alpha mask with the authored text colour. */
void ug_text_draw_ink(UgCanvas *cv,const char *s,float x,float y,float scale,UgColor c,unsigned just,UgGlyphMode mode);
void ug_text_draw(UgCanvas *cv, const char *utf8, float x, float y, float scale,
                  UgColor c, unsigned just, float wrap_w, float leading,
                  UgGlyphMode mode);

#ifdef __cplusplus
}
#endif
#endif
