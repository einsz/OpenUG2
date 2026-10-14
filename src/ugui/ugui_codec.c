/* ugui_codec.c -- JDLZ and HUFF decoders written from SPEC 13. */
#include "ugui_codec.h"
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

UgCodec ug_codec_detect(const uint8_t *d, size_t len) {
    if (len < 16) return UG_CODEC_NONE;
    if (!memcmp(d, "JDLZ", 4)) return UG_CODEC_JDLZ;
    if (!memcmp(d, "HUFF", 4)) return UG_CODEC_HUFF;
    return UG_CODEC_NONE;
}

/* ------------------------------------------------------------------ */
/* JDLZ (13.2)                                                         */
/* ------------------------------------------------------------------ */
int ug_jdlz_decode(const uint8_t *in, size_t len, uint8_t **out, size_t *out_len,
                   char *err, size_t errlen) {
    if (len < 16 || memcmp(in, "JDLZ", 4)) return fail(err, errlen, "jdlz: bad header");
    uint32_t dec = rd32(in + 8), enc = rd32(in + 12);
    if (dec > UG_CODEC_MAX_OUT) return fail(err, errlen, "jdlz: absurd decoded size");
    size_t end = len;
    if (enc >= 16 && enc < len) end = enc;            /* ignore trailing padding */
    uint8_t *o = (uint8_t *)malloc(dec ? dec : 1);
    if (!o) return fail(err, errlen, "jdlz: out of memory");
    size_t pos = 16, n = 0;
    unsigned fa = 1, fb = 1;                          /* flag registers; 1 = empty */
    while (n < dec) {
        if (fa == 1) { if (pos >= end) goto trunc; fa = (unsigned)in[pos++] + 256u; }
        if (fb == 1) { if (pos >= end) goto trunc; fb = (unsigned)in[pos++] + 256u; }
        unsigned bit_a = fa & 1u;
        fa >>= 1;
        if (!bit_a) {
            if (pos >= end) goto trunc;
            o[n++] = in[pos++];
        } else {
            if (pos + 2 > end) goto trunc;
            unsigned p = in[pos], q = in[pos + 1];
            pos += 2;
            unsigned bit_b = fb & 1u;
            fb >>= 1;
            size_t dist, cnt;
            if (bit_b) { dist = (p & 15u) + 1u; cnt = (size_t)((p >> 4) * 256u + q) + 3u; }
            else { dist = (size_t)((p >> 5) * 256u + q) + 17u; cnt = (p & 31u) + 3u; }
            if (dist > n) { free(o); return fail(err, errlen, "jdlz: match distance before start of output"); }
            if (cnt > dec - n) { free(o); return fail(err, errlen, "jdlz: copy runs past decoded size"); }
            for (size_t i = 0; i < cnt; i++, n++) o[n] = o[n - dist];
        }
    }
    *out = o;
    *out_len = n;
    return 1;
trunc:
    free(o);
    return fail(err, errlen, "jdlz: input ends before decoded size is reached");
}

/* ------------------------------------------------------------------ */
/* HUFF (13.3)                                                         */
/* ------------------------------------------------------------------ */
typedef struct { const uint8_t *p; size_t nbits, pos; int over; } Bits;

static unsigned bit_get(Bits *b) {
    if (b->pos >= b->nbits) { b->over = 1; return 0; }
    unsigned v = (b->p[b->pos >> 3] >> (7 - (b->pos & 7))) & 1u;
    b->pos++;
    return v;
}
static uint32_t bits_get(Bits *b, int n) {
    uint32_t v = 0;
    for (int i = 0; i < n; i++) v = (v << 1) | bit_get(b);
    return v;
}
/* variable-length number: z zero bits then a 1; read z+2 bits v; v + 2^(z+2) - 4 */
static uint32_t varnum(Bits *b) {
    int z = 0;
    while (!bit_get(b)) {
        if (b->over || ++z > 24) { b->over = 1; return 0; }
    }
    uint32_t v = bits_get(b, z + 2);
    return v + (1u << (z + 2)) - 4u;
}

int ug_huff_decode(const uint8_t *in, size_t len, uint8_t **out, size_t *out_len,
                   char *err, size_t errlen) {
    if (len < 16 || memcmp(in, "HUFF", 4)) return fail(err, errlen, "huff: bad header");
    uint32_t dec_hdr = rd32(in + 8), blen = rd32(in + 12);
    size_t avail = len - 16;
    if (blen && blen < avail) avail = blen;
    Bits b = {in + 16, avail * 8, 0, 0};

    uint32_t method = bits_get(&b, 16);
    if ((method & 0xFF) != 0xFB || (method >> 8) < 0x30 || (method >> 8) > 0x35)
        return fail(err, errlen, "huff: unsupported method");
    if (method & 0x0100) bits_get(&b, 24);
    uint32_t dec = bits_get(&b, 24);
    unsigned esc = bits_get(&b, 8);
    if (dec == 0) dec = dec_hdr;                       /* tolerate a zero stream size */
    if (dec > UG_CODEC_MAX_OUT) return fail(err, errlen, "huff: absurd decoded size");

    /* code-length histogram */
    uint32_t cnt[17] = {0};
    uint64_t c = 0;
    int maxw = 0;
    for (int w = 1; w <= 16; w++) {
        uint32_t nw = varnum(&b);
        if (b.over || nw > 256) return fail(err, errlen, "huff: bad code length histogram");
        cnt[w] = nw;
        c = c * 2 + nw;
        maxw = w;
        if ((c << (16 - w)) >= 65536u) break;
    }
    if ((c << (16 - maxw)) != 65536u) return fail(err, errlen, "huff: code space is not full");
    int nsym = 0;
    for (int w = 1; w <= 16; w++) nsym += (int)cnt[w];
    if (nsym > 256) return fail(err, errlen, "huff: too many symbols");

    /* symbol list */
    uint8_t syms[256];
    uint8_t used[256];
    memset(used, 0, sizeof used);
    unsigned prev = 255;
    for (int i = 0; i < nsym; i++) {
        uint32_t k = varnum(&b);
        if (b.over || k >= (uint32_t)(256 - i)) return fail(err, errlen, "huff: bad symbol list");
        unsigned cur = prev;
        for (uint32_t s = 0; s <= k; s++) {
            do { cur = (cur + 1) & 255u; } while (used[cur]);
        }
        used[cur] = 1;
        syms[i] = (uint8_t)cur;
        prev = cur;
    }

    /* canonical codes: per width first code value and first symbol index */
    uint32_t first_code[17], first_idx[17];
    {
        uint32_t acc = 0;
        int idx = 0;
        for (int w = 1; w <= 16; w++) {
            first_code[w] = acc >> (16 - w);
            first_idx[w] = (uint32_t)idx;
            acc += cnt[w] << (16 - w);
            idx += (int)cnt[w];
        }
    }

    uint8_t *o = (uint8_t *)malloc(dec ? dec : 1);
    if (!o) return fail(err, errlen, "huff: out of memory");
    size_t n = 0;
    int ended = 0;
    while (n < dec && !ended) {
        uint32_t code = 0;
        int sym = -1;
        for (int w = 1; w <= 16; w++) {
            code = (code << 1) | bit_get(&b);
            if (cnt[w] && code >= first_code[w] && code - first_code[w] < cnt[w]) {
                sym = syms[first_idx[w] + (code - first_code[w])];
                break;
            }
        }
        if (sym < 0 || b.over) { free(o); return fail(err, errlen, "huff: corrupt or truncated stream"); }
        if ((unsigned)sym != esc) { o[n++] = (uint8_t)sym; continue; }
        uint32_t r = varnum(&b);
        if (b.over) { free(o); return fail(err, errlen, "huff: truncated stream"); }
        if (r > 0) {
            if (n == 0 || r > dec - n) { free(o); return fail(err, errlen, "huff: bad run"); }
            memset(o + n, o[n - 1], r);
            n += r;
        } else if (bit_get(&b)) {
            ended = 1;
        } else {
            if (n >= dec) break;
            o[n++] = (uint8_t)bits_get(&b, 8);
        }
    }
    if (n != dec) { free(o); return fail(err, errlen, "huff: stream ended before decoded size"); }
    /* methods 0x32FB / 0x34FB: running-sum passes (not used by frontend data, 13.3 step 7) */
    unsigned m = method >> 8;
    int passes = (m == 0x32 || m == 0x34) ? (m == 0x34 ? 2 : 1) : 0;
    for (int p = 0; p < passes; p++)
        for (size_t i = 1; i < n; i++) o[i] = (uint8_t)(o[i] + o[i - 1]);
    *out = o;
    *out_len = n;
    return 1;
}

int ug_decompress(const uint8_t *in, size_t len, uint8_t **out, size_t *out_len,
                  char *err, size_t errlen) {
    switch (ug_codec_detect(in, len)) {
    case UG_CODEC_JDLZ: return ug_jdlz_decode(in, len, out, out_len, err, errlen);
    case UG_CODEC_HUFF: return ug_huff_decode(in, len, out, out_len, err, errlen);
    default: return 2;
    }
}
