/* ugui_ffont.c -- original font chunk reader (SPEC 16.1-16.3, 16.4). */
#include "ugui_face.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint32_t rd16(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8); }
static int fail(char *err, size_t n, const char *msg) {
    if (err && n) snprintf(err, n, "%s", msg);
    return 0;
}

static void copy_name(char *dst, size_t cap, const uint8_t *src, size_t max, int lower) {
    size_t n = 0;
    while (n < max && src[n] && n + 1 < cap) {
        char c = (char)src[n];
        if (lower && c >= 'A' && c <= 'Z') c = (char)(c + 32);
        dst[n++] = c;
    }
    dst[n] = 0;
}

int ug_face_parse(UgFace *f, const uint8_t *p, size_t size, char *err, size_t errlen) {
    memset(f, 0, sizeof *f);
    if (size < 512 + 128) return fail(err, errlen, "font: chunk too small");
    copy_name(f->name, sizeof f->name, p, 256, 1);
    copy_name(f->atlas_name, sizeof f->atlas_name, p + 256, 256, 0);
    const uint8_t *b = p + 512;
    size_t bsize = size - 512;
    if (memcmp(b, "FNTF", 4)) return fail(err, errlen, "font: missing FNTF block");
    f->version = (int)rd16(b + 8);
    uint32_t ng = rd16(b + 10);
    f->line_height = b[18];
    uint32_t goff = rd32(b + 20), koff = rd32(b + 24), cell = rd32(b + 36);
    f->cell_height = (int)cell;
    if (f->version == 0x19E) {
        uint32_t toff = rd32(b + 48);
        if (toff && (uint64_t)toff + 32 <= bsize) {
            f->atlas_w = (int)rd32(b + toff + 24);
            f->atlas_h = (int)rd32(b + toff + 28);
        }
    }
    if ((uint64_t)goff + (uint64_t)ng * 16 > bsize) return fail(err, errlen, "font: glyph table runs past chunk");
    f->g = (UgFGlyph *)calloc(ng ? ng : 1, sizeof(UgFGlyph));
    if (!f->g) return fail(err, errlen, "font: out of memory");
    f->ng = (int)ng;
    for (uint32_t i = 0; i < ng; i++) {
        const uint8_t *r = b + goff + (size_t)i * 16;
        UgFGlyph *g = &f->g[i];
        g->cp = rd16(r);
        g->w = r[2]; g->h = r[3];
        g->x = (uint16_t)rd16(r + 4); g->y = (uint16_t)rd16(r + 6);
        g->xoff = (int8_t)r[9]; g->yoff = (int8_t)r[10];
        g->nkern = r[11];
        g->kfirst = (uint16_t)rd16(r + 12);
        g->adv = (uint16_t)rd16(r + 14);
    }
    if (koff) {
        if ((uint64_t)koff + 4 > bsize) { ug_face_free(f); return fail(err, errlen, "font: kerning table out of range"); }
        uint32_t nk = rd32(b + koff);
        if ((uint64_t)koff + 4 + (uint64_t)nk * 4 > bsize) { ug_face_free(f); return fail(err, errlen, "font: kerning table runs past chunk"); }
        f->k = (UgKern *)calloc(nk ? nk : 1, sizeof(UgKern));
        if (!f->k) { ug_face_free(f); return fail(err, errlen, "font: out of memory"); }
        f->nk = (int)nk;
        for (uint32_t i = 0; i < nk; i++) {
            const uint8_t *e = b + koff + 4 + (size_t)i * 4;
            f->k[i].prev = (uint16_t)rd16(e);
            f->k[i].adj = (int8_t)e[2];
        }
        for (int i = 0; i < f->ng; i++)
            if ((int)f->g[i].kfirst + (int)f->g[i].nkern > f->nk) f->g[i].nkern = 0;   /* defensive */
    }
    /* glyphs are sorted by code (16.3); sort anyway so lookups never depend on it */
    for (int i = 1; i < f->ng; i++) {
        UgFGlyph v = f->g[i];
        int j = i - 1;
        while (j >= 0 && f->g[j].cp > v.cp) { f->g[j + 1] = f->g[j]; j--; }
        f->g[j + 1] = v;
    }
    const UgFGlyph *h = ug_face_glyph(f, 'H');
    if (!h) h = ug_face_glyph(f, 'A');
    if (h) { f->baseline = h->yoff + h->h; f->cap_height = h->h; }
    else { f->baseline = f->line_height * 4 / 5; f->cap_height = f->line_height * 3 / 4; }
    return 1;
}

void ug_face_free(UgFace *f) {
    free(f->g);
    free(f->k);
    memset(f, 0, sizeof *f);
}

const UgFGlyph *ug_face_glyph(const UgFace *f, uint32_t cp) {
    int lo = 0, hi = f->ng - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (f->g[mid].cp == cp) return &f->g[mid];
        if (f->g[mid].cp < cp) lo = mid + 1; else hi = mid - 1;
    }
    return NULL;
}

int ug_face_kern(const UgFace *f, const UgFGlyph *g, uint32_t prev_cp) {
    if (!g || !g->nkern || !f->k) return 0;
    for (int i = 0; i < g->nkern; i++)
        if (f->k[g->kfirst + i].prev == prev_cp) return f->k[g->kfirst + i].adj;
    return 0;
}
