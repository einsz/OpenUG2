/* ugui_face.h -- font faces: original bitmap fonts (SPEC 16) and the built-in
 * fallback. GL-free. */
#ifndef UGUI_FACE_H
#define UGUI_FACE_H

#include "ugui_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t cp;
    uint8_t  w, h;         /* glyph size in atlas texels */
    uint16_t x, y;         /* top-left in the atlas */
    int8_t   xoff, yoff;   /* pen -> left edge; line top -> top edge */
    uint8_t  nkern;
    uint16_t kfirst;
    uint16_t adv;          /* advance, unscaled units */
} UgFGlyph;

typedef struct { uint16_t prev; int8_t adj; } UgKern;

typedef struct UgFace {
    char name[64];         /* lower-case font name, e.g. "conduitmditc_tt21i" */
    char atlas_name[64];
    int version;           /* 0x19E / 0x16E */
    int line_height;       /* line pitch at scale 1 (16.2) */
    int cell_height;       /* full height of a line incl. descenders (16.2) */
    int atlas_w, atlas_h;
    int baseline, cap_height;   /* derived from the glyph 'H' (or 'A'), for inline glyph art */
    UgFGlyph *g; int ng;   /* sorted by code point */
    UgKern *k; int nk;
    int tex;               /* texture library index + 1; 0 = atlas not available */
    int procedural;        /* 1 = the built-in pixel face (ug_face_builtin) */
} UgFace;

/* Parses the payload of a font chunk 0x00030201 (16.1-16.3). */
int  ug_face_parse(UgFace *f, const uint8_t *payload, size_t size, char *err, size_t errlen);
void ug_face_free(UgFace *f);
const UgFGlyph *ug_face_glyph(const UgFace *f, uint32_t cp);
/* Adjustment applied before `g` when `prev_cp` precedes it (0 when none). */
int  ug_face_kern(const UgFace *f, const UgFGlyph *g, uint32_t prev_cp);

/* Unavailable-face metadata sentinel, never rendered as a substitute. */
const UgFace *ug_face_builtin(void);

/* One glyph quad in virtual units relative to the text origin (16.5). */
typedef struct { uint32_t cp; float x0, y0, x1, y1, u0, v0, u1, v1; } UgQuad;
typedef void (*UgQuadFn)(void *user, const UgQuad *q);
/* Lays out `utf8` on the top-left anchor with this face (no wrap, newlines and
 * '^' break lines); calls `fn` per drawn glyph. *last_pen = unscaled width of
 * the last line (kerning included). */
void ug_text_quads(const UgFace *f, const char *utf8, float scale, UgQuadFn fn, void *user,
                   float *last_pen);

/* Face used by ug_text_draw / ug_text_measure; NULL = unavailable. */
void ug_text_set_face(const UgFace *f);
const UgFace *ug_text_face(void);

#ifdef __cplusplus
}
#endif
#endif
