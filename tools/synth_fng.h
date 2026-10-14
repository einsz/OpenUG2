/* synth_fng.h -- serializes an in-memory UgFngPackage into the raw package container that
 * ug_fng_parse reads (SPEC 3), so tests can hand synthetic packages to code that loads packages from
 * a UgAssets (the pause adapter) without any retail data. Test-only; nothing here is original data.
 *
 *   uint8_t *synth_fng(const UgFngPackage *pk, const char *name, size_t *len);   // malloc'ed
 *
 * Objects, scripts (tracks, base keys, events, chains) and message responses are written; resource
 * tables and targets are not (the fixtures do not use them). */
#ifndef TOOLS_SYNTH_FNG_H
#define TOOLS_SYNTH_FNG_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../src/ugui/ugui_fng.h"

typedef struct SynthBuf { uint8_t *d; size_t n, cap; } SynthBuf;
static void sb_put(SynthBuf *b, const void *p, size_t n) {
    if (b->n + n > b->cap) {
        b->cap = (b->n + n) * 2 + 64;
        b->d = (uint8_t *)realloc(b->d, b->cap);
    }
    memcpy(b->d + b->n, p, n);
    b->n += n;
}
static void sb_u32(SynthBuf *b, uint32_t v) { uint8_t t[4] = { (uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24) }; sb_put(b, t, 4); }
static void sb_u16(SynthBuf *b, uint32_t v) { uint8_t t[2] = { (uint8_t)v, (uint8_t)(v >> 8) }; sb_put(b, t, 2); }
static void sb_f32(SynthBuf *b, float f) { uint32_t u; memcpy(&u, &f, 4); sb_u32(b, u); }
#define SYNTH_FOURCC(a, b, c, d) ((uint32_t)(uint8_t)(a) | ((uint32_t)(uint8_t)(b) << 8) | ((uint32_t)(uint8_t)(c) << 16) | ((uint32_t)(uint8_t)(d) << 24))
/* chunk: id, size, payload (the payload is built in `body`) */
static void sb_chunk(SynthBuf *out, uint32_t id, const SynthBuf *body) { sb_u32(out, id); sb_u32(out, (uint32_t)body->n); sb_put(out, body->d, body->n); }
static void sb_tag(SynthBuf *out, char a, char b, const SynthBuf *body) { sb_u16(out, (uint32_t)((uint8_t)a | ((uint8_t)b << 8))); sb_u16(out, (uint32_t)body->n); sb_put(out, body->d, body->n); }
static void sb_tag_u32(SynthBuf *out, char a, char b, uint32_t v) { SynthBuf t = { 0 }; sb_u32(&t, v); sb_tag(out, a, b, &t); free(t.d); }

static void synth_color(SynthBuf *b, UgColor c) { sb_u32(b, c.b); sb_u32(b, c.g); sb_u32(b, c.r); sb_u32(b, c.a); }   /* file order B,G,R,A */

static void synth_objd(SynthBuf *out, const UgFngObject *o) {
    SynthBuf t = { 0 };
    sb_tag_u32(&t, 'O', 't', o->type);
    sb_tag_u32(&t, 'O', 'h', o->name_hash);
    { SynthBuf q = { 0 }; sb_u32(&q, o->guid); sb_u32(&q, o->name_hash); sb_u32(&q, o->flags); sb_u32(&q, (uint32_t)o->res_index); sb_tag(&t, 'O', 'P', &q); free(q.d); }
    if (o->has_parent) sb_tag_u32(&t, 'P', 'A', o->parent_guid);
    if (o->type == UG_OBJ_TEXT) {
        sb_tag_u32(&t, 'S', 'j', o->justify);
        if (o->has_label_hash) sb_tag_u32(&t, 'S', 'H', o->label_hash);
        if (o->text) {
            SynthBuf q = { 0 };
            for (const char *s = o->text; *s; s++) sb_u16(&q, (uint8_t)*s);
            sb_u16(&q, 0);
            sb_tag(&t, 'S', 't', &q);
            free(q.d);
        }
    }
    SynthBuf sa = { 0 };
    synth_color(&sa, o->color);
    for (int i = 0; i < 3; i++) sb_f32(&sa, o->pivot[i]);
    for (int i = 0; i < 3; i++) sb_f32(&sa, o->pos[i]);
    for (int i = 0; i < 4; i++) sb_f32(&sa, o->rot[i]);
    for (int i = 0; i < 3; i++) sb_f32(&sa, o->size[i]);
    for (int i = 0; i < 4; i++) sb_f32(&sa, o->uv[i]);
    sb_tag(&t, 'S', 'A', &sa);
    free(sa.d);
    sb_chunk(out, SYNTH_FOURCC('O', 'b', 'j', 'D'), &t);
    free(t.d);
}

static void synth_key(SynthBuf *b, const UgTrack *tr, const UgKey *k) {
    int n = ug_type_ncomp(tr->type);
    sb_u32(b, (uint32_t)k->t);
    if (tr->type == UG_VT_COLOR) {
        sb_u32(b, (uint32_t)(int32_t)k->v[2]); sb_u32(b, (uint32_t)(int32_t)k->v[1]);
        sb_u32(b, (uint32_t)(int32_t)k->v[0]); sb_u32(b, (uint32_t)(int32_t)k->v[3]);
    } else if (tr->type == UG_VT_INT || tr->type == UG_VT_BOOL) {
        sb_u32(b, (uint32_t)(int32_t)k->v[0]);
    } else {
        for (int i = 0; i < n; i++) sb_f32(b, k->v[i]);
    }
}

static void synth_script(SynthBuf *out, const UgScript *s) {
    SynthBuf t = { 0 };
    { SynthBuf q = { 0 }; sb_u32(&q, s->id); sb_u32(&q, s->length); sb_u32(&q, s->flags); sb_u32(&q, 0); sb_tag(&t, 'S', 'h', &q); free(q.d); }
    if (s->has_chain) sb_tag_u32(&t, 'S', 'c', s->chain);
    for (int i = 0; i < s->ntracks; i++) {
        const UgTrack *tr = &s->tracks[i];
        int vsize = ug_type_ncomp(tr->type) * 4;
        { SynthBuf q = { 0 }; sb_put(&q, &(uint8_t[4]){ tr->type, (uint8_t)vsize, tr->interp, tr->action }, 4); sb_u32(&q, tr->length); sb_tag(&t, 'F', 'I', &q); free(q.d); }
        sb_tag_u32(&t, 'T', 'o', tr->prop);
        SynthBuf kd = { 0 };
        if (tr->has_base) synth_key(&kd, tr, &(UgKey){ .t = -1, .v = { tr->base.v[0], tr->base.v[1], tr->base.v[2], tr->base.v[3] } });
        for (int k = 0; k < tr->nkeys; k++) synth_key(&kd, tr, &tr->keys[k]);
        if (kd.n) sb_tag(&t, 'K', 'd', &kd);
        free(kd.d);
    }
    if (s->nevents) {
        SynthBuf ev = { 0 };
        for (int i = 0; i < s->nevents; i++) { sb_u32(&ev, s->events[i].msg); sb_u32(&ev, s->events[i].target); sb_u32(&ev, s->events[i].t_ms); }
        sb_tag(&t, 'E', 'V', &ev);
        free(ev.d);
    }
    sb_chunk(out, SYNTH_FOURCC('S', 'c', 'r', 'p'), &t);
    free(t.d);
}

static void synth_responses(SynthBuf *out, uint32_t chunk_id, const UgFngResponse *r, int n) {
    SynthBuf t = { 0 };
    for (int i = 0; i < n; i++) {
        sb_tag_u32(&t, 'M', 'i', r[i].msg);
        for (int k = 0; k < r[i].ncmds; k++) {
            sb_tag_u32(&t, 'R', 'i', r[i].cmds[k].id);
            sb_tag_u32(&t, 'R', 'u', r[i].cmds[k].arg);
            sb_tag_u32(&t, 'R', 't', r[i].cmds[k].target);
        }
    }
    sb_chunk(out, chunk_id, &t);
    free(t.d);
}

static uint8_t *synth_fng(const UgFngPackage *pk, const char *name, size_t *len) {
    SynthBuf root = { 0 }, out = { 0 };
    SynthBuf hd = { 0 };
    size_t nl = strlen(name) + 1;
    sb_u32(&hd, 0x10); sb_u32(&hd, 0); sb_u32(&hd, 0); sb_u32(&hd, 1); sb_u32(&hd, (uint32_t)nl); sb_u32(&hd, 1);
    sb_put(&hd, name, nl);
    sb_put(&hd, "", 1);
    sb_chunk(&root, SYNTH_FOURCC('P', 'k', 'H', 'd'), &hd);
    free(hd.d);
    SynthBuf obj = { 0 };
    for (int i = 0; i < pk->nobj; i++) {
        const UgFngObject *o = &pk->obj[i];
        SynthBuf fob = { 0 };
        synth_objd(&fob, o);
        for (int s = 0; s < o->nscripts; s++) synth_script(&fob, &o->scripts[s]);
        if (o->nresp) synth_responses(&fob, SYNTH_FOURCC('M', 's', 'g', 'R'), o->resp, o->nresp);
        sb_chunk(&obj, SYNTH_FOURCC('F', 'O', 'b', 0xEA), &fob);
        free(fob.d);
    }
    sb_chunk(&root, SYNTH_FOURCC('O', 'b', 'j', 0xCC), &obj);
    free(obj.d);
    if (pk->npkg_resp) synth_responses(&root, SYNTH_FOURCC('P', 'k', 'g', 'R'), pk->pkg_resp, pk->npkg_resp);
    sb_chunk(&out, SYNTH_FOURCC('F', 'E', 'n', 0xE7), &root);
    free(root.d);
    *len = out.n;
    return out.d;
}
#endif
