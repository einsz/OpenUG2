/* ugui_tex.c -- texture pack reader and pixel decoders (SPEC 15). */
#include "ugui_tex.h"
#include "ugui_codec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CH_INFO    0x33310001u
#define CH_KEYS    0x33310002u
#define CH_SLOTS   0x33310003u
#define CH_RECORDS 0x33310004u
#define CH_DATA    0x33320002u
#define REC_SIZE   124u
#define SLOT_SIZE  24u
#define DESC_SIZE  32u

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint32_t rd16(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8); }
static int fail(char *err, size_t n, const char *msg) {
    if (err && n) snprintf(err, n, "%s", msg);
    return 0;
}

void ug_texlib_init(UgTexLib *l) { memset(l, 0, sizeof *l); }

void ug_texlib_free(UgTexLib *l) {
    for (int i = 0; i < l->n; i++) free(l->t[i].rgba);
    free(l->t);
    free(l->order);
    memset(l, 0, sizeof *l);
}

static UgTexture *add_tex(UgTexLib *l) {
    if (l->n == l->cap) {
        int nc = l->cap ? l->cap * 2 : 256;
        UgTexture *nt = (UgTexture *)realloc(l->t, sizeof(UgTexture) * (size_t)nc);
        if (!nt) return NULL;
        l->t = nt;
        l->cap = nc;
    }
    UgTexture *t = &l->t[l->n++];
    memset(t, 0, sizeof *t);
    return t;
}

/* ---- level sizes (15.4 size identity) ---- */
static size_t level_bytes(int fmt, int w, int h) {
    switch (fmt) {
    case UG_PF_DXT1: return (size_t)((w + 3) / 4) * (size_t)((h + 3) / 4) * 8;
    case UG_PF_DXT3: case UG_PF_DXT5: return (size_t)((w + 3) / 4) * (size_t)((h + 3) / 4) * 16;
    case UG_PF_BGRA8: return (size_t)w * (size_t)h * 4;
    case UG_PF_P8: return (size_t)w * (size_t)h;
    }
    return 0;
}

/* ---- decoders (15.7) ---- */
static void rgb565(unsigned c, int *r, int *g, int *b) {
    int r5 = (int)((c >> 11) & 31), g6 = (int)((c >> 5) & 63), b5 = (int)(c & 31);
    *r = (r5 << 3) | (r5 >> 2);
    *g = (g6 << 2) | (g6 >> 4);
    *b = (b5 << 3) | (b5 >> 2);
}

static void put(uint8_t *out, int w, int x, int y, int r, int g, int b, int a) {
    uint8_t *p = out + ((size_t)y * (size_t)w + (size_t)x) * 4;
    p[0] = (uint8_t)r; p[1] = (uint8_t)g; p[2] = (uint8_t)b; p[3] = (uint8_t)a;
}

/* colour block; force4 = DXT3/5 always use 4-colour mode */
static void colour_block(const uint8_t *b, int force4, int col[4][4]) {
    unsigned c0 = rd16(b), c1 = rd16(b + 2);
    int r0, g0, b0, r1, g1, b1;
    rgb565(c0, &r0, &g0, &b0);
    rgb565(c1, &r1, &g1, &b1);
    col[0][0] = r0; col[0][1] = g0; col[0][2] = b0; col[0][3] = 255;
    col[1][0] = r1; col[1][1] = g1; col[1][2] = b1; col[1][3] = 255;
    if (force4 || c0 > c1) {
        col[2][0] = (2 * r0 + r1) / 3; col[2][1] = (2 * g0 + g1) / 3; col[2][2] = (2 * b0 + b1) / 3; col[2][3] = 255;
        col[3][0] = (r0 + 2 * r1) / 3; col[3][1] = (g0 + 2 * g1) / 3; col[3][2] = (b0 + 2 * b1) / 3; col[3][3] = 255;
    } else {
        col[2][0] = (r0 + r1) / 2; col[2][1] = (g0 + g1) / 2; col[2][2] = (b0 + b1) / 2; col[2][3] = 255;
        col[3][0] = col[3][1] = col[3][2] = col[3][3] = 0;      /* transparent black */
    }
}

int ug_tex_decode(int fmt, int w, int h, const uint8_t *src, size_t src_len,
                  const uint8_t *pal, uint8_t *out) {
    if (w <= 0 || h <= 0 || !src) return 0;
    if (level_bytes(fmt, w, h) == 0 || src_len < level_bytes(fmt, w, h)) return 0;
    switch (fmt) {
    case UG_PF_BGRA8:
        for (int i = 0; i < w * h; i++) {
            out[4 * i] = src[4 * i + 2]; out[4 * i + 1] = src[4 * i + 1];
            out[4 * i + 2] = src[4 * i]; out[4 * i + 3] = src[4 * i + 3];
        }
        return 1;
    case UG_PF_P8:
        if (!pal) return 0;
        for (int i = 0; i < w * h; i++) {
            const uint8_t *e = pal + 4 * (size_t)src[i];
            out[4 * i] = e[2]; out[4 * i + 1] = e[1]; out[4 * i + 2] = e[0]; out[4 * i + 3] = e[3];   /* B,G,R,A */
        }
        return 1;
    case UG_PF_DXT1: case UG_PF_DXT3: case UG_PF_DXT5: {
        int bw = (w + 3) / 4, bh = (h + 3) / 4, bsz = fmt == UG_PF_DXT1 ? 8 : 16;
        for (int by = 0; by < bh; by++)
            for (int bx = 0; bx < bw; bx++) {
                const uint8_t *blk = src + ((size_t)by * (size_t)bw + (size_t)bx) * (size_t)bsz;
                int col[4][4];
                const uint8_t *cb = blk + (bsz == 8 ? 0 : 8);
                colour_block(cb, fmt != UG_PF_DXT1, col);
                uint32_t idx = rd32(cb + 4);
                int alpha[16];
                if (fmt == UG_PF_DXT3) {
                    for (int i = 0; i < 16; i++) alpha[i] = ((blk[i >> 1] >> ((i & 1) * 4)) & 15) * 17;
                } else if (fmt == UG_PF_DXT5) {
                    int a0 = blk[0], a1 = blk[1], tab[8];
                    tab[0] = a0; tab[1] = a1;
                    if (a0 > a1) for (int i = 1; i < 7; i++) tab[i + 1] = ((7 - i) * a0 + i * a1) / 7;
                    else { for (int i = 1; i < 5; i++) tab[i + 1] = ((5 - i) * a0 + i * a1) / 5; tab[6] = 0; tab[7] = 255; }
                    uint64_t bits = 0;
                    for (int i = 0; i < 6; i++) bits |= (uint64_t)blk[2 + i] << (8 * i);
                    for (int i = 0; i < 16; i++) alpha[i] = tab[(bits >> (3 * i)) & 7];
                } else {
                    for (int i = 0; i < 16; i++) alpha[i] = 255;
                }
                for (int i = 0; i < 16; i++) {
                    int x = bx * 4 + (i & 3), y = by * 4 + (i >> 2);
                    if (x >= w || y >= h) continue;
                    const int *c = col[(idx >> (2 * i)) & 3];
                    int a = fmt == UG_PF_DXT1 ? c[3] : alpha[i];
                    put(out, w, x, y, c[0], c[1], c[2], a);
                }
            }
        return 1;
    }
    }
    return 0;
}

/* ---- pack parsing ---- */
static void set_name(UgTexture *t, const uint8_t *nm) {
    size_t n = 0;
    while (n < 24 && nm[n]) n++;
    if (n > 23) n = 23;
    memcpy(t->name, nm, n);
    t->name[n] = 0;
}

/* fills dimension/format fields from a 124-byte record */
static void read_record(UgTexture *t, const uint8_t *r) {
    set_name(t, r + 12);
    t->key = rd32(r + 36);
    t->data_off = rd32(r + 48);
    t->w = (int)rd16(r + 68);
    t->h = (int)rd16(r + 70);
    t->fmt = r[74];
    t->palettized = r[77];
    t->levels = r[78];
}

typedef struct {
    const uint8_t *keys; uint32_t nkeys;
    const uint8_t *recs; uint32_t nrecs;
    const uint8_t *slots; uint32_t nslots;
    const uint8_t *data; uint32_t data_size;     /* aligned pixel area */
    int have_data;
} PackCtx;

static int pack_visit(void *u, const UgChunkRef *c) {
    PackCtx *p = (PackCtx *)u;
    switch (c->id) {
    case CH_KEYS: p->keys = c->payload; p->nkeys = c->size / 8; break;
    case CH_RECORDS: p->recs = c->payload; p->nrecs = c->size / REC_SIZE; break;
    case CH_SLOTS: p->slots = c->payload; p->nslots = c->size / SLOT_SIZE; break;
    case CH_DATA: {
        /* data base = next 0x80 boundary counted from the bundle start (15.5) */
        size_t off = (size_t)(c->payload - c->base);
        size_t base = (off + 0x7F) & ~(size_t)0x7F;
        if (base <= off + c->size) {
            p->data = c->base + base;
            p->data_size = (uint32_t)(off + c->size - base);
            p->have_data = 1;
        }
        break;
    }
    default: break;
    }
    return 0;
}

int ug_texpack_parse(UgTexLib *l, const UgChunkRef *c, char *err, size_t errlen) {
    PackCtx p;
    memset(&p, 0, sizeof p);
    if (!ug_chunks_walk_in(c->base, c->base_len, c->payload, c->size, pack_visit, &p, err, errlen))
        return 0;
    if (err && errlen) err[0] = 0;

    if (p.recs && p.nrecs) {
        if (!p.have_data) return fail(err, errlen, "texpack: no pixel data chunk");
        if (p.keys && p.nkeys != p.nrecs) return fail(err, errlen, "texpack: key list and record count differ");
        for (uint32_t i = 0; i < p.nrecs; i++) {
            const uint8_t *r = p.recs + (size_t)i * REC_SIZE;
            UgTexture *t = add_tex(l);
            if (!t) return fail(err, errlen, "texpack: out of memory");
            read_record(t, r);
            /* the key list and the record carry the same key; never recompute it (15.4) */
            if (p.keys) t->key = rd32(p.keys + 8 * (size_t)i);
            uint32_t off = t->data_off;
            size_t lvl0 = level_bytes(t->fmt, t->w, t->h);
            if (off > p.data_size || lvl0 == 0 || lvl0 > p.data_size - off) {
                t->failed = 1;           /* kept in the table; never decodable */
                continue;
            }
            t->src = p.data + off;
            t->src_len = p.data_size - off;
            if (t->fmt == UG_PF_P8) {
                uint32_t po = rd32(r + 52);
                if (po <= p.data_size && 1024u <= p.data_size - po) t->pal = p.data + po;
                else t->failed = 1;
            }
        }
        return 1;
    }
    if (p.slots && p.nslots) {
        for (uint32_t i = 0; i < p.nslots; i++) {
            const uint8_t *s = p.slots + (size_t)i * SLOT_SIZE;
            uint32_t key = rd32(s), off = rd32(s + 4), csz = rd32(s + 8), dsz = rd32(s + 12);
            UgTexture *t = add_tex(l);
            if (!t) return fail(err, errlen, "texpack: out of memory");
            t->key = key;
            t->dec_size = dsz;
            if ((uint64_t)off + csz > c->base_len) { t->failed = 1; continue; }
            t->comp = c->base + off;          /* absolute offset (15.6) */
            t->comp_len = csz;
        }
        return 1;
    }
    return fail(err, errlen, "texpack: pack has neither records nor slots");
}

/* Slot blobs are decoded on first use; the record at decoded_size - 156 supplies the size. */
static int slot_decode(UgTexture *t) {
    uint8_t *dec = NULL;
    size_t dl = 0;
    char err[64];
    if (ug_decompress(t->comp, t->comp_len, &dec, &dl, err, sizeof err) != 1) return 0;
    if (dl < REC_SIZE + DESC_SIZE) { free(dec); return 0; }
    const uint8_t *rec = dec + dl - (REC_SIZE + DESC_SIZE);
    uint32_t key = t->key;
    read_record(t, rec);
    t->key = key;                           /* the slot table key is authoritative */
    size_t lvl0 = level_bytes(t->fmt, t->w, t->h);
    uint8_t *rgba = NULL;
    if (lvl0 && lvl0 <= dl - (REC_SIZE + DESC_SIZE)) {
        rgba = (uint8_t *)malloc((size_t)t->w * (size_t)t->h * 4);
        const uint8_t *pal = NULL;
        if (t->fmt == UG_PF_P8) {
            uint32_t po = rd32(rec + 52);
            if (po <= dl && 1024 <= dl - po) pal = dec + po;
        }
        if (rgba && !ug_tex_decode(t->fmt, t->w, t->h, dec, dl - (REC_SIZE + DESC_SIZE), pal, rgba)) {
            free(rgba);
            rgba = NULL;
        }
    }
    free(dec);
    t->rgba = rgba;
    return rgba != NULL;
}

/* index sort by key, stable so the first occurrence wins */
static const UgTexture *g_sort_base;
static int cmp_key(const void *a, const void *b) {
    const UgTexture *ta = &g_sort_base[*(const int *)a], *tb = &g_sort_base[*(const int *)b];
    if (ta->key != tb->key) return ta->key < tb->key ? -1 : 1;
    return *(const int *)a - *(const int *)b;
}

void ug_texlib_index(UgTexLib *l) {
    free(l->order);
    l->order = (int *)malloc(sizeof(int) * (size_t)(l->n ? l->n : 1));
    if (!l->order) return;
    for (int i = 0; i < l->n; i++) l->order[i] = i;
    g_sort_base = l->t;
    qsort(l->order, (size_t)l->n, sizeof(int), cmp_key);
}

int ug_texlib_find(const UgTexLib *l, uint32_t key) {
    if (!l->order) {
        for (int i = 0; i < l->n; i++) if (l->t[i].key == key) return i;
        return -1;
    }
    int lo = 0, hi = l->n - 1, found = -1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        uint32_t k = l->t[l->order[mid]].key;
        if (k == key) { found = mid; hi = mid - 1; }       /* leftmost = first occurrence */
        else if (k < key) lo = mid + 1;
        else hi = mid - 1;
    }
    return found < 0 ? -1 : l->order[found];
}

const uint8_t *ug_texlib_pixels(UgTexLib *l, int idx) {
    if (idx < 0 || idx >= l->n) return NULL;
    UgTexture *t = &l->t[idx];
    if (t->rgba) return t->rgba;
    if (t->failed) return NULL;
    if (t->comp) {
        if (!slot_decode(t)) t->failed = 1;
        return t->rgba;
    }
    if (!t->src || t->w <= 0 || t->h <= 0) { t->failed = 1; return NULL; }
    t->rgba = (uint8_t *)malloc((size_t)t->w * (size_t)t->h * 4);
    if (!t->rgba) { t->failed = 1; return NULL; }
    if (!ug_tex_decode(t->fmt, t->w, t->h, t->src, t->src_len, t->pal, t->rgba)) {
        free(t->rgba);
        t->rgba = NULL;
        t->failed = 1;
    }
    return t->rgba;
}
