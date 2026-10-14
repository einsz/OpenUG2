/* ugui_tex.h -- texture packs (SPEC 15): parsing, pixel decode, key lookup.
 * GL-free; the GL backend uploads the RGBA8 level-0 images produced here. */
#ifndef UGUI_TEX_H
#define UGUI_TEX_H

#include "ugui_bundle.h"

#ifdef __cplusplus
extern "C" {
#endif

/* pixel formats (15.7) */
enum { UG_PF_P8 = 0x08, UG_PF_BGRA8 = 0x20, UG_PF_DXT1 = 0x22, UG_PF_DXT3 = 0x24, UG_PF_DXT5 = 0x26 };

typedef struct {
    uint32_t key;              /* hash of the upper-case full name (15.4) */
    char source[640];          /* supplying runtime file; never copied asset bytes */
    char name[32];             /* stored name, truncated to 23 characters */
    int w, h, fmt, levels, palettized;
    uint32_t data_off;         /* record field: relative to the pixel data base */
    /* where the level-0 bytes live */
    const uint8_t *src;        /* level 0 pixel data */
    size_t src_len;            /* bytes available from src */
    const uint8_t *pal;        /* palette (P8) or NULL */
    /* compressed-slot variant: blob decoded on first use */
    const uint8_t *comp; size_t comp_len; uint32_t dec_size;
    /* decoded RGBA8, w*h*4 (lazy) */
    uint8_t *rgba;
    int failed;
} UgTexture;

typedef struct {
    UgTexture *t; int n, cap;
    int *order;                /* indices sorted by key (after ug_texlib_index) */
} UgTexLib;

void ug_texlib_init(UgTexLib *l);
void ug_texlib_free(UgTexLib *l);

/* Adds every texture of one pack chunk (0xB3300000), table or slot variant.
 * `c->base` must be the start of the bundle buffer (pixel data alignment is
 * counted from it, 15.5, and slot blob offsets are absolute, 15.6). The buffer
 * has to outlive the library. */
int ug_texpack_parse(UgTexLib *l, const UgChunkRef *c, char *err, size_t errlen);

/* Sort for lookup; first occurrence of a key wins. */
void ug_texlib_index(UgTexLib *l);
/* Index of the texture with this key or -1. */
int ug_texlib_find(const UgTexLib *l, uint32_t key);
/* RGBA8 of level 0 (w*h*4), decoded on first call; NULL if undecodable. */
const uint8_t *ug_texlib_pixels(UgTexLib *l, int idx);

/* Decode one level of a surface to RGBA8 (exposed for tests). */
int ug_tex_decode(int fmt, int w, int h, const uint8_t *src, size_t src_len,
                  const uint8_t *pal, uint8_t *out_rgba);

#ifdef __cplusplus
}
#endif
#endif
