/* ugui_bundle.c -- chunk walking and package extraction (SPEC 3.2, 14). */
#include "ugui_bundle.h"
#include "ugui_codec.h"
#include "ugui_fng.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static int fail(char *err, size_t n, const char *msg) {
    if (err && n) snprintf(err, n, "%s", msg);
    return 0;
}

void ug_buf_free(UgBuf *b) { free(b->data); b->data = NULL; b->len = 0; }

int ug_file_read(const char *path, UgBuf *out, char *err, size_t errlen) {
    memset(out, 0, sizeof *out);
    FILE *f = fopen(path, "rb");
    if (!f) { if (err && errlen) snprintf(err, errlen, "cannot open %s", path); return 0; }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 0) { fclose(f); return fail(err, errlen, "cannot size file"); }
    uint8_t *d = (uint8_t *)malloc((size_t)n ? (size_t)n : 1);
    if (!d) { fclose(f); return fail(err, errlen, "out of memory"); }
    if (fread(d, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(d); return fail(err, errlen, "short read"); }
    fclose(f);
    out->data = d;
    out->len = (size_t)n;
    return 1;
}

int ug_bundle_read(const char *path, UgBuf *out, char *err, size_t errlen) {
    UgBuf raw;
    if (!ug_file_read(path, &raw, err, errlen)) return 0;
    uint8_t *dec = NULL;
    size_t dl = 0;
    int r = ug_decompress(raw.data, raw.len, &dec, &dl, err, errlen);
    if (r == 0) { ug_buf_free(&raw); return 0; }
    if (r == 1) { ug_buf_free(&raw); out->data = dec; out->len = dl; }
    else *out = raw;
    return 1;
}

int ug_chunk_is_container(uint32_t id) {
    return (id & 0xF0000000u) == 0x80000000u || id == 0xB3300000u || id == 0xB3310000u ||
           id == 0xB3320000u;
}

static int walk(const uint8_t *base, size_t base_len, const uint8_t *p, const uint8_t *end, int depth,
                UgChunkFn fn, void *user, char *err, size_t errlen) {
    while (p < end) {
        if (end - p < 8) return fail(err, errlen, "bundle: truncated chunk header");
        uint32_t id = rd32(p), size = rd32(p + 4);
        if ((size_t)(end - p - 8) < size) return fail(err, errlen, "bundle: chunk runs past its parent");
        const uint8_t *pl = p + 8;
        if (id != 0) {
            UgChunkRef ref = {id, pl, size, depth, base, base_len, p};
            int handled = fn ? fn(user, &ref) : 0;
            if (!handled && ug_chunk_is_container(id) && depth < 12)
                if (!walk(base, base_len, pl, pl + size, depth + 1, fn, user, err, errlen)) return 0;
        }
        p = pl + size;
    }
    return 1;
}

int ug_chunks_walk(const uint8_t *buf, size_t len, UgChunkFn fn, void *user,
                   char *err, size_t errlen) {
    if (err && errlen) err[0] = 0;
    return walk(buf, len, buf, buf + len, 0, fn, user, err, errlen);
}

int ug_chunks_walk_in(const uint8_t *base, size_t base_len, const uint8_t *buf, size_t len,
                      UgChunkFn fn, void *user, char *err, size_t errlen) {
    if (err && errlen) err[0] = 0;
    return walk(base, base_len, buf, buf + len, 0, fn, user, err, errlen);
}

int ug_package_from_chunk(const UgChunkRef *c, UgPackageBlob *out, char *err, size_t errlen) {
    memset(out, 0, sizeof *out);
    const uint8_t *body = c->payload;
    size_t blen = c->size;
    if (c->id == UG_CHUNK_PKG_HASHED) {
        if (blen < 8) return fail(err, errlen, "bundle: short package chunk");
        body += 4;               /* screen hash prefix: redundant (14.3) */
        blen -= 4;
    } else if (c->id != UG_CHUNK_PKG_RAW) {
        return fail(err, errlen, "bundle: not a package chunk");
    }
    uint8_t *dec = NULL;
    size_t dl = 0;
    int r = ug_decompress(body, blen, &dec, &dl, err, errlen);
    if (r == 0) return 0;
    if (r == 2) {
        dec = (uint8_t *)malloc(blen ? blen : 1);
        if (!dec) return fail(err, errlen, "out of memory");
        memcpy(dec, body, blen);
        dl = blen;
    }
    char *name = ug_fng_peek_name(dec, dl);
    if (!name) { free(dec); return fail(err, errlen, "bundle: package has no readable header"); }
    out->name = name;
    out->key = ug_hash_upper(name);
    out->data = dec;
    out->len = dl;
    return 1;
}

void ug_package_free(UgPackageBlob *p) {
    free(p->name);
    free(p->data);
    memset(p, 0, sizeof *p);
}
