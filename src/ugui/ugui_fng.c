/* ugui_fng.c -- screen package reader (SPEC 3). */
#include "ugui_fng.h"
#include "ugui_codec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FOURCC(a, b, c, d) ((uint32_t)(uint8_t)(a) | ((uint32_t)(uint8_t)(b) << 8) | \
                            ((uint32_t)(uint8_t)(c) << 16) | ((uint32_t)(uint8_t)(d) << 24))
#define TAG2(a, b) ((uint16_t)((uint8_t)(a) | ((uint8_t)(b) << 8)))

#define C_ROOT  FOURCC('F', 'E', 'n', 0xE7)
#define C_PKHD  FOURCC('P', 'k', 'H', 'd')
#define C_TYPS  FOURCC('T', 'y', 'p', 'S')
#define C_RES   FOURCC('R', 'e', 's', 0xCC)
#define C_RSNM  FOURCC('R', 's', 'N', 'm')
#define C_RSRQ  FOURCC('R', 's', 'R', 'q')
#define C_OBJ   FOURCC('O', 'b', 'j', 0xCC)
#define C_BUTN  FOURCC('B', 'u', 't', 'n')
#define C_FOB   FOURCC('F', 'O', 'b', 0xEA)
#define C_OBJD  FOURCC('O', 'b', 'j', 'D')
#define C_SCRP  FOURCC('S', 'c', 'r', 'p')
#define C_MSGR  FOURCC('M', 's', 'g', 'R')
#define C_PKGR  FOURCC('P', 'k', 'g', 'R')
#define C_TARG  FOURCC('T', 'a', 'r', 'g')
#define C_BUNDLE_HASHED 0x00030210u
#define C_BUNDLE_RAW    0x00030203u

typedef struct {
    UgFngPackage *pk;
    char *err; size_t errlen;
    int failed;
    const uint8_t *names; size_t names_len;
    int found;
} Ctx;

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint32_t rd16(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8); }
static float rdf(const uint8_t *p) {
    uint32_t u = rd32(p);
    float f;
    memcpy(&f, &u, 4);
    return f;
}

static int fail(Ctx *c, const char *msg) {
    if (!c->failed && c->err && c->errlen) snprintf(c->err, c->errlen, "%s", msg);
    c->failed = 1;
    return 0;
}

/* grow-by-one array append; returns the new zeroed element */
static void *push(void **arr, int *n, size_t sz) {
    void *na = realloc(*arr, sz * (size_t)(*n + 1));
    if (!na) return NULL;
    *arr = na;
    void *e = (char *)na + sz * (size_t)(*n);
    memset(e, 0, sz);
    (*n)++;
    return e;
}

static char *dup_n(const uint8_t *s, size_t n) {
    char *d = (char *)malloc(n + 1);
    if (!d) return NULL;
    memcpy(d, s, n);
    d[n] = 0;
    return d;
}
static char *dup_cstr_bounded(const uint8_t *s, size_t maxn) {
    size_t n = 0;
    while (n < maxn && s[n]) n++;
    return dup_n(s, n);
}

/* ---- iteration helpers ---- */
/* chunk: returns 1 and advances, 0 at clean end, -1 on malformed */
static int next_chunk(const uint8_t **p, const uint8_t *end, uint32_t *id,
                      const uint8_t **pl, uint32_t *size) {
    if (*p == end) return 0;
    if (end - *p < 8) return -1;
    *id = rd32(*p);
    *size = rd32(*p + 4);
    if ((size_t)(end - *p - 8) < *size) return -1;
    *pl = *p + 8;
    *p = *pl + *size;
    return 1;
}
static int next_tag(const uint8_t **p, const uint8_t *end, uint16_t *id,
                    const uint8_t **pl, uint32_t *size) {
    if (*p == end) return 0;
    if (end - *p < 4) return -1;
    *id = (uint16_t)rd16(*p);
    *size = rd16(*p + 2);
    if ((size_t)(end - *p - 4) < *size) return -1;
    *pl = *p + 4;
    *p = *pl + *size;
    return 1;
}

/* ---- text ---- */
static char *utf16le_to_utf8(const uint8_t *p, size_t nbytes) {
    char *out = (char *)malloc(nbytes * 2 + 4), *o = out;
    if (!out) return NULL;
    size_t nu = nbytes / 2;
    for (size_t i = 0; i < nu; i++) {
        uint32_t u = rd16(p + 2 * i);
        if (u == 0) break;
        if (u >= 0xD800 && u < 0xDC00 && i + 1 < nu) {
            uint32_t lo = rd16(p + 2 * (i + 1));
            if (lo >= 0xDC00 && lo < 0xE000) {
                u = 0x10000 + ((u - 0xD800) << 10) + (lo - 0xDC00);
                i++;
            }
        }
        o += ug_utf8_encode(u, o);
    }
    *o = 0;
    return out;
}

/* ---- SA block ---- */
static void parse_sa(UgFngObject *o, const uint8_t *p, uint32_t size) {
    if (size >= 16) {
        /* file order B,G,R,A (i32) -> R,G,B,A */
        o->color.b = ug_clamp8((float)(int32_t)rd32(p));
        o->color.g = ug_clamp8((float)(int32_t)rd32(p + 4));
        o->color.r = ug_clamp8((float)(int32_t)rd32(p + 8));
        o->color.a = ug_clamp8((float)(int32_t)rd32(p + 12));
    }
    if (size >= 68) {
        for (int i = 0; i < 3; i++) o->pivot[i] = rdf(p + 16 + 4 * i);
        for (int i = 0; i < 3; i++) o->pos[i] = rdf(p + 28 + 4 * i);
        for (int i = 0; i < 4; i++) o->rot[i] = rdf(p + 40 + 4 * i);
        for (int i = 0; i < 3; i++) o->size[i] = rdf(p + 56 + 4 * i);
    }
    if (size >= 84) {
        for (int i = 0; i < 4; i++) o->uv[i] = rdf(p + 68 + 4 * i);
        o->has_uv = 1;
    }
    if (o->type == UG_OBJ_GRADIENT && size >= 148) {
        for (int c = 0; c < 4; c++) {
            const uint8_t *q = p + 84 + 16 * c;
            o->corners[c].b = ug_clamp8((float)(int32_t)rd32(q));
            o->corners[c].g = ug_clamp8((float)(int32_t)rd32(q + 4));
            o->corners[c].r = ug_clamp8((float)(int32_t)rd32(q + 8));
            o->corners[c].a = ug_clamp8((float)(int32_t)rd32(q + 12));
        }
        o->has_corners = 1;
    } else if (o->type == UG_OBJ_MULTIIMAGE && size >= 84 + 60) {
        for (int i = 0; i < 15; i++) o->extra[i] = rdf(p + 84 + 4 * i);
        o->nextra = 15;
    }
}

static int parse_objd(Ctx *c, UgFngObject *o, const uint8_t *p, const uint8_t *end) {
    uint16_t id; const uint8_t *pl; uint32_t sz; int r;
    const uint8_t *sa = NULL; uint32_t sa_size = 0;
    while ((r = next_tag(&p, end, &id, &pl, &sz)) > 0) {
        switch (id) {
        case TAG2('O', 't'): if (sz >= 4) o->type = rd32(pl); break;
        case TAG2('O', 'h'): if (sz >= 4) o->name_hash = rd32(pl); break;
        case TAG2('O', 'P'):
            if (sz < 16) return fail(c, "fng: short OP tag");
            o->guid = rd32(pl);
            /* pl+4 is the name hash again; trust Oh unless absent */
            if (!o->name_hash) o->name_hash = rd32(pl + 4);
            o->flags = rd32(pl + 8);
            o->res_index = (int32_t)rd32(pl + 12);
            break;
        case TAG2('P', 'A'):
            if (sz >= 4) { o->parent_guid = rd32(pl); o->has_parent = 1; }
            break;
        case TAG2('I', 'f'): if (sz >= 4) o->image_flags = rd32(pl); break;
        case TAG2('S', 'b'): if (sz >= 4) { o->buffer_len = rd32(pl); o->is_text = 1; } break;
        case TAG2('S', 't'):
            free(o->text);
            o->text = utf16le_to_utf8(pl, sz);
            o->is_text = 1;
            break;
        case TAG2('S', 'j'): if (sz >= 4) o->justify = rd32(pl); break;
        case TAG2('S', 'l'): if (sz >= 4) o->leading = (int32_t)rd32(pl); break;
        case TAG2('S', 'w'): if (sz >= 4) o->max_width = rd32(pl); break;
        case TAG2('S', 'H'): if (sz >= 4) { o->label_hash = rd32(pl); o->has_label_hash = 1; } break;
        case TAG2('S', 'A'): sa = pl; sa_size = sz; break;
        default: break;   /* SL, On, M1..Mc and unknown tags are skipped */
        }
    }
    if (r < 0) return fail(c, "fng: tag runs past ObjD chunk");
    if (o->type == UG_OBJ_TEXT) o->is_text = 1;
    o->size[0] = o->size[1] = o->size[2] = 1.0f;
    o->rot[3] = 1.0f;
    if (sa) parse_sa(o, sa, sa_size);
    return 1;
}

/* ---- scripts ---- */
static const uint32_t POSITIONAL_PROP[7] = {UG_P_COLOR, UG_P_PIVOT, UG_P_POS,
                                            UG_P_ROT, UG_P_SIZE, UG_P_UV0, UG_P_UV1};

static int parse_script(Ctx *c, UgScript *s, const uint8_t *p, const uint8_t *end) {
    uint16_t id; const uint8_t *pl; uint32_t sz; int r;
    UgTrack *cur = NULL;
    int track_index = -1;
    while ((r = next_tag(&p, end, &id, &pl, &sz)) > 0) {
        switch (id) {
        case TAG2('S', 'h'):
            if (sz < 16) return fail(c, "fng: short Sh tag");
            s->id = rd32(pl); s->length = rd32(pl + 4); s->flags = rd32(pl + 8);
            break;
        case TAG2('S', 'c'):
            if (sz >= 4) { s->chain = rd32(pl); s->has_chain = 1; }
            break;
        case TAG2('F', 'I'): {
            if (sz < 8) return fail(c, "fng: short FI tag");
            UgTrack *t = (UgTrack *)push((void **)&s->tracks, &s->ntracks, sizeof(UgTrack));
            if (!t) return fail(c, "fng: out of memory");
            track_index++;
            t->type = pl[0]; t->vsize = pl[1]; t->interp = pl[2]; t->action = pl[3];
            t->length = rd32(pl + 4) & 0x00FFFFFFu;
            t->prop = track_index < 7 ? POSITIONAL_PROP[track_index] : UG_NPROPS;
            cur = &s->tracks[s->ntracks - 1];
            break;
        }
        case TAG2('T', 'o'):
            if (!cur || sz < 4) return fail(c, "fng: To tag without track");
            cur->prop = rd32(pl);
            break;
        case TAG2('K', 'd'): {
            if (!cur) return fail(c, "fng: Kd tag without track");
            int n = ug_type_ncomp(cur->type);
            uint32_t stride = 4u + cur->vsize;
            if (cur->type > UG_VT_COLOR || cur->vsize < (uint32_t)n * 4)
                return fail(c, "fng: bad track value size");
            if (sz % stride) return fail(c, "fng: Kd size not a multiple of key size");
            uint32_t cnt = sz / stride;
            for (uint32_t k = 0; k < cnt; k++) {
                const uint8_t *q = pl + k * stride;
                UgKey key;
                memset(&key, 0, sizeof key);
                key.t = (int32_t)rd32(q);
                const uint8_t *v = q + 4;
                if (cur->type == UG_VT_COLOR) {
                    key.v[0] = (float)(int32_t)rd32(v + 8);   /* R */
                    key.v[1] = (float)(int32_t)rd32(v + 4);   /* G */
                    key.v[2] = (float)(int32_t)rd32(v);       /* B */
                    key.v[3] = (float)(int32_t)rd32(v + 12);  /* A */
                } else if (cur->type == UG_VT_INT || cur->type == UG_VT_BOOL) {
                    key.v[0] = (float)(int32_t)rd32(v);
                } else {
                    for (int i = 0; i < n; i++) key.v[i] = rdf(v + 4 * i);
                }
                if (k == 0 && key.t == -1) {
                    cur->base = key;
                    cur->has_base = 1;
                } else {
                    UgKey *nk = (UgKey *)realloc(cur->keys, sizeof(UgKey) * (size_t)(cur->nkeys + 1));
                    if (!nk) return fail(c, "fng: out of memory");
                    cur->keys = nk;
                    cur->keys[cur->nkeys++] = key;
                }
            }
            break;
        }
        case TAG2('E', 'V'): {
            uint32_t cnt = sz / 12;
            for (uint32_t k = 0; k < cnt; k++) {
                UgEvent *e = (UgEvent *)push((void **)&s->events, &s->nevents, sizeof(UgEvent));
                if (!e) return fail(c, "fng: out of memory");
                e->msg = rd32(pl + 12 * k);
                e->target = rd32(pl + 12 * k + 4);
                e->t_ms = rd32(pl + 12 * k + 8);
            }
            break;
        }
        default: break;   /* Sn and unknown tags */
        }
    }
    if (r < 0) return fail(c, "fng: tag runs past Scrp chunk");
    return 1;
}

/* ---- responses ---- */
static int parse_responses(Ctx *c, UgFngResponse **arr, int *n,
                           const uint8_t *p, const uint8_t *end) {
    uint16_t id; const uint8_t *pl; uint32_t sz; int r;
    UgFngResponse *cur = NULL;
    UgFngCmd *cmd = NULL;
    while ((r = next_tag(&p, end, &id, &pl, &sz)) > 0) {
        switch (id) {
        case TAG2('M', 'i'):
            if (sz < 4) return fail(c, "fng: short Mi tag");
            if (!push((void **)arr, n, sizeof(UgFngResponse))) return fail(c, "fng: out of memory");
            cur = &(*arr)[*n - 1];
            cur->msg = rd32(pl);
            cmd = NULL;
            break;
        case TAG2('M', 'C'): break;   /* count is implied by the entries */
        case TAG2('R', 'i'):
            if (!cur || sz < 4) return fail(c, "fng: Ri without Mi");
            if (!push((void **)&cur->cmds, &cur->ncmds, sizeof(UgFngCmd))) return fail(c, "fng: out of memory");
            cmd = &cur->cmds[cur->ncmds - 1];
            cmd->id = rd32(pl);
            break;
        case TAG2('R', 'u'): if (cmd && sz >= 4) cmd->arg = rd32(pl); break;
        case TAG2('R', 's'):
            if (cmd) { free(cmd->str); cmd->str = dup_cstr_bounded(pl, sz); }
            break;
        case TAG2('R', 't'): if (cmd && sz >= 4) cmd->target = rd32(pl); break;
        default: break;
        }
    }
    if (r < 0) return fail(c, "fng: tag runs past response chunk");
    return 1;
}

static int parse_fob(Ctx *c, const uint8_t *p, const uint8_t *end) {
    UgFngPackage *pk = c->pk;
    UgFngObject *o = (UgFngObject *)push((void **)&pk->obj, &pk->nobj, sizeof(UgFngObject));
    if (!o) return fail(c, "fng: out of memory");
    int idx = pk->nobj - 1;
    uint32_t id, sz; const uint8_t *pl; int r;
    while ((r = next_chunk(&p, end, &id, &pl, &sz)) > 0) {
        o = &pk->obj[idx];
        if (id == C_OBJD) {
            if (!parse_objd(c, o, pl, pl + sz)) return 0;
        } else if (id == C_SCRP) {
            if (!push((void **)&o->scripts, &o->nscripts, sizeof(UgScript))) return fail(c, "fng: out of memory");
            if (!parse_script(c, &o->scripts[o->nscripts - 1], pl, pl + sz)) return 0;
        } else if (id == C_MSGR) {
            if (!parse_responses(c, &o->resp, &o->nresp, pl, pl + sz)) return 0;
        }
    }
    if (r < 0) return fail(c, "fng: chunk runs past FOb container");
    return 1;
}

static int parse_targ(Ctx *c, const uint8_t *p, const uint8_t *end) {
    UgFngPackage *pk = c->pk;
    uint16_t id; const uint8_t *pl; uint32_t sz; int r;
    while ((r = next_tag(&p, end, &id, &pl, &sz)) > 0) {
        if (id == TAG2('T', 'c') && sz >= 4) {
            pk->target_count = rd32(pl);
            pk->has_targets = 1;
        } else if (id == TAG2('M', 't') && sz >= 4) {
            UgFngTarget *t = (UgFngTarget *)push((void **)&pk->targets, &pk->ntargets, sizeof(UgFngTarget));
            if (!t) return fail(c, "fng: out of memory");
            t->msg = rd32(pl);
            t->nguids = (int)(sz / 4 - 1);
            t->guids = (uint32_t *)malloc(sizeof(uint32_t) * (size_t)(t->nguids ? t->nguids : 1));
            if (!t->guids) return fail(c, "fng: out of memory");
            for (int i = 0; i < t->nguids; i++) t->guids[i] = rd32(pl + 4 + 4 * i);
        }
    }
    if (r < 0) return fail(c, "fng: tag runs past Targ chunk");
    return 1;
}

static int parse_root(Ctx *c, const uint8_t *p, const uint8_t *end) {
    UgFngPackage *pk = c->pk;
    uint32_t id, sz; const uint8_t *pl; int r;
    const uint8_t *rq = NULL; uint32_t rq_size = 0;
    while ((r = next_chunk(&p, end, &id, &pl, &sz)) > 0) {
        switch (id) {
        case C_PKHD: {
            if (sz < 24) return fail(c, "fng: short PkHd");
            for (int i = 0; i < 6; i++) pk->header_words[i] = rd32(pl + 4 * i);
            uint32_t n1 = pk->header_words[4], n2 = pk->header_words[5];
            if ((uint64_t)24 + n1 + n2 > sz) return fail(c, "fng: PkHd strings run past chunk");
            pk->name = dup_cstr_bounded(pl + 24, n1);
            pk->path = dup_cstr_bounded(pl + 24 + n1, n2);
            break;
        }
        case C_TYPS: {
            uint32_t n = sz / 8;
            pk->type_sizes = (uint32_t (*)[2])malloc(sizeof(uint32_t[2]) * (size_t)(n ? n : 1));
            if (!pk->type_sizes) return fail(c, "fng: out of memory");
            pk->ntypes = (int)n;
            for (uint32_t i = 0; i < n; i++) {
                pk->type_sizes[i][0] = rd32(pl + 8 * i);
                pk->type_sizes[i][1] = rd32(pl + 8 * i + 4);
            }
            break;
        }
        case C_RES: {
            const uint8_t *q = pl, *qe = pl + sz;
            uint32_t cid, csz; const uint8_t *cpl; int rr;
            while ((rr = next_chunk(&q, qe, &cid, &cpl, &csz)) > 0) {
                if (cid == C_RSNM) { c->names = cpl; c->names_len = csz; }
                else if (cid == C_RSRQ) { rq = cpl; rq_size = csz; }
            }
            if (rr < 0) return fail(c, "fng: chunk runs past Res container");
            break;
        }
        case C_OBJ: {
            const uint8_t *q = pl, *qe = pl + sz;
            uint32_t cid, csz; const uint8_t *cpl; int rr;
            while ((rr = next_chunk(&q, qe, &cid, &cpl, &csz)) > 0) {
                if (cid == C_BUTN && csz >= 4) {
                    pk->button_count = rd32(cpl);
                    pk->has_button_count = 1;
                } else if (cid == C_FOB) {
                    if (!parse_fob(c, cpl, cpl + csz)) return 0;
                }
            }
            if (rr < 0) return fail(c, "fng: chunk runs past Obj container");
            break;
        }
        case C_PKGR:
            if (!parse_responses(c, &pk->pkg_resp, &pk->npkg_resp, pl, pl + sz)) return 0;
            break;
        case C_TARG:
            if (!parse_targ(c, pl, pl + sz)) return 0;
            break;
        default: break;
        }
    }
    if (r < 0) return fail(c, "fng: chunk runs past package root");

    /* resolve resource requests now that names are known */
    if (rq) {
        if (rq_size < 4) return fail(c, "fng: short RsRq");
        uint32_t cnt = rd32(rq);
        if ((uint64_t)4 + (uint64_t)cnt * 24 > rq_size) return fail(c, "fng: RsRq count runs past chunk");
        pk->res = (UgFngResource *)calloc(cnt ? cnt : 1, sizeof(UgFngResource));
        if (!pk->res) return fail(c, "fng: out of memory");
        pk->nres = (int)cnt;
        for (uint32_t i = 0; i < cnt; i++) {
            const uint8_t *q = rq + 4 + 24 * i;
            UgFngResource *res = &pk->res[i];
            res->id = rd32(q);
            res->name_off = rd32(q + 4);
            res->kind = rd32(q + 8);
            if (c->names && res->name_off < c->names_len)
                res->name = dup_cstr_bounded(c->names + res->name_off, c->names_len - res->name_off);
            else
                res->name = dup_n((const uint8_t *)"", 0);
        }
    }
    c->found = 1;
    return 1;
}

static int parse_stream(Ctx *c, const uint8_t *p, const uint8_t *end, int depth);

/* decode a possibly compressed package body and parse it */
static int parse_body(Ctx *c, const uint8_t *body, const uint8_t *end, int depth) {
    if (end - body >= 16 && ug_codec_detect(body, (size_t)(end - body)) != UG_CODEC_NONE) {
        uint8_t *dec = NULL;
        size_t dl = 0;
        char msg[96] = "";
        if (!ug_decompress(body, (size_t)(end - body), &dec, &dl, msg, sizeof msg)) {
            char full[160];
            snprintf(full, sizeof full, "fng: compressed package: %s", msg);
            return fail(c, full);
        }
        int ok = parse_stream(c, dec, dec + dl, depth + 1);
        free(dec);
        return ok;
    }
    return parse_stream(c, body, end, depth + 1);
}

static int parse_stream(Ctx *c, const uint8_t *p, const uint8_t *end, int depth) {
    uint32_t id, sz; const uint8_t *pl; int r;
    while (!c->found && (r = next_chunk(&p, end, &id, &pl, &sz)) > 0) {
        if (id == C_ROOT) {
            if (!parse_root(c, pl, pl + sz)) return 0;
        } else if (id == C_BUNDLE_RAW && depth < 4) {
            if (!parse_body(c, pl, pl + sz, depth)) return 0;
        } else if (id == C_BUNDLE_HASHED && depth < 4) {
            if (sz < 4) return fail(c, "fng: short hashed bundle chunk");
            if (!parse_body(c, pl + 4, pl + sz, depth)) return 0;
        }
    }
    if (!c->found && r < 0) return fail(c, "fng: chunk runs past its parent");
    return 1;
}

int ug_fng_parse(const uint8_t *data, size_t len, UgFngPackage *out,
                 char *err, size_t errlen) {
    Ctx c;
    memset(out, 0, sizeof *out);
    memset(&c, 0, sizeof c);
    c.pk = out; c.err = err; c.errlen = errlen;
    if (err && errlen) err[0] = 0;
    if (parse_body(&c, data, data + len, -1) && !c.found) fail(&c, "fng: no package root chunk found");
    if (c.failed) { ug_fng_free(out); return 0; }
    return 1;
}

char *ug_fng_peek_name(const uint8_t *data, size_t len) {
    const uint8_t *p = data, *end = data + len;
    uint32_t id, sz; const uint8_t *pl; int r;
    while ((r = next_chunk(&p, end, &id, &pl, &sz)) > 0) {
        if (id != C_ROOT) continue;
        const uint8_t *q = pl, *qe = pl + sz;
        uint32_t cid, csz; const uint8_t *cpl;
        while (next_chunk(&q, qe, &cid, &cpl, &csz) > 0) {
            if (cid == C_PKHD && csz >= 24) {
                uint32_t n1 = rd32(cpl + 16);
                if (24 + (uint64_t)n1 > csz) return NULL;
                return dup_cstr_bounded(cpl + 24, n1);
            }
        }
        return NULL;
    }
    return NULL;
}

void ug_fng_free(UgFngPackage *p) {
    if (!p) return;
    free(p->name); free(p->path); free(p->type_sizes);
    for (int i = 0; i < p->nres; i++) free(p->res[i].name);
    free(p->res);
    for (int i = 0; i < p->nobj; i++) {
        UgFngObject *o = &p->obj[i];
        free(o->text);
        for (int s = 0; s < o->nscripts; s++) ug_script_free(&o->scripts[s]);
        free(o->scripts);
        for (int r = 0; r < o->nresp; r++) {
            for (int k = 0; k < o->resp[r].ncmds; k++) free(o->resp[r].cmds[k].str);
            free(o->resp[r].cmds);
        }
        free(o->resp);
    }
    free(p->obj);
    for (int r = 0; r < p->npkg_resp; r++) {
        for (int k = 0; k < p->pkg_resp[r].ncmds; k++) free(p->pkg_resp[r].cmds[k].str);
        free(p->pkg_resp[r].cmds);
    }
    free(p->pkg_resp);
    for (int i = 0; i < p->ntargets; i++) free(p->targets[i].guids);
    free(p->targets);
    memset(p, 0, sizeof *p);
}

const UgFngObject *ug_fng_find_guid(const UgFngPackage *p, uint32_t guid) {
    for (int i = 0; i < p->nobj; i++)
        if (p->obj[i].guid == guid) return &p->obj[i];
    return NULL;
}
