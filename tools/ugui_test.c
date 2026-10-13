/* ugui_test.c -- GL-free tests for the ugui widget library.
 *
 * Section A drives the library with the synthetic vectors in
 * tools/ugui_vectors (hash, track evaluation, string table, package
 * parse). Later sections test text layout, canvas, widgets, focus and hints.
 *
 * Usage: ugui_test [vector_dir]   (default: UG_VECTOR_DIR from the Makefile)
 * Exit status is non-zero when any check fails. */
#include "ugui_core.h"
#include "ugui_fng.h"
#include "ugui_ui.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef UG_VECTOR_DIR
#define UG_VECTOR_DIR "tools/ugui_vectors"
#endif

static int g_checks, g_fails;
static const char *g_vec_dir = UG_VECTOR_DIR;

#define CHECK(cond) do { g_checks++; if (!(cond)) { g_fails++; \
    printf("    FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
#define CHECK_MSG(cond, ...) do { g_checks++; if (!(cond)) { g_fails++; \
    printf("    FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
#define CHECK_NEAR(a, b, eps) do { double a_ = (a), b_ = (b); g_checks++; \
    if (!(fabs(a_ - b_) <= (eps))) { g_fails++; printf("    FAIL %s:%d: %s=%g expected %g\n", \
    __FILE__, __LINE__, #a, a_, b_); } } while (0)
#define RUN(fn) do { int f0 = g_fails; printf("  %-44s", #fn); fn(); \
    printf("%s\n", g_fails == f0 ? "ok" : "FAILED"); } while (0)

/* ------------------------------------------------------------------ */
/* file helpers                                                        */
/* ------------------------------------------------------------------ */
static char *read_file(const char *name, size_t *len) {
    char path[512];
    snprintf(path, sizeof path, "%s/%s", g_vec_dir, name);
    FILE *f = fopen(path, "rb");
    if (!f) { printf("    cannot open %s\n", path); return NULL; }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc((size_t)n + 1);
    if (fread(buf, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(buf); return NULL; }
    buf[n] = 0;
    fclose(f);
    if (len) *len = (size_t)n;
    return buf;
}

/* "offset  b0 b1 ..." per line -> bytes */
static uint8_t *read_hex(const char *name, size_t *len) {
    char *txt = read_file(name, NULL);
    if (!txt) return NULL;
    size_t cap = strlen(txt) / 2 + 16, n = 0;
    uint8_t *out = (uint8_t *)malloc(cap);
    char *save = NULL;
    for (char *line = strtok_r(txt, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
        char *tsave = NULL;
        char *tok = strtok_r(line, " \t\r", &tsave);   /* offset */
        if (!tok) continue;
        while ((tok = strtok_r(NULL, " \t\r", &tsave)) != NULL)
            out[n++] = (uint8_t)strtoul(tok, NULL, 16);
    }
    free(txt);
    *len = n;
    return out;
}

/* ------------------------------------------------------------------ */
/* tiny JSON reader (tests only)                                       */
/* ------------------------------------------------------------------ */
typedef struct J J;
enum { J_NULL, J_BOOL, J_NUM, J_STR, J_ARR, J_OBJ };
struct J {
    int t;   /* J_* */
    double num; char *str;
    int n; J **kids; char **keys;
};
static const char *jp;
static void jws(void) { while (*jp == ' ' || *jp == '\n' || *jp == '\t' || *jp == '\r') jp++; }
static J *jnew(int t) { J *j = (J *)calloc(1, sizeof *j); j->t = t; return j; }
static void jadd(J *p, char *key, J *k) {
    p->kids = (J **)realloc(p->kids, sizeof(J *) * (size_t)(p->n + 1));
    p->keys = (char **)realloc(p->keys, sizeof(char *) * (size_t)(p->n + 1));
    p->kids[p->n] = k; p->keys[p->n] = key; p->n++;
}
static char *jstr(void) {
    jp++;  /* opening quote */
    const char *s = jp;
    while (*jp && *jp != '"') { if (*jp == '\\') jp++; jp++; }
    size_t n = (size_t)(jp - s);
    char *r = (char *)malloc(n + 1);
    memcpy(r, s, n); r[n] = 0;
    jp++;
    /* un-escape \\ only */
    char *o = r;
    for (char *i = r; *i; i++) { if (*i == '\\' && i[1]) i++; *o++ = *i; }
    *o = 0;
    return r;
}
static J *jparse(void) {
    jws();
    if (*jp == '{') {
        J *o = jnew(J_OBJ); jp++; jws();
        while (*jp != '}') {
            jws(); char *k = jstr(); jws(); jp++; /* : */
            jadd(o, k, jparse()); jws();
            if (*jp == ',') jp++;
            jws();
        }
        jp++; return o;
    }
    if (*jp == '[') {
        J *a = jnew(J_ARR); jp++; jws();
        while (*jp != ']') {
            jadd(a, NULL, jparse()); jws();
            if (*jp == ',') jp++;
            jws();
        }
        jp++; return a;
    }
    if (*jp == '"') { J *s = jnew(J_STR); s->str = jstr(); return s; }
    if (!strncmp(jp, "true", 4)) { jp += 4; J *b = jnew(J_BOOL); b->num = 1; return b; }
    if (!strncmp(jp, "false", 5)) { jp += 5; return jnew(J_BOOL); }
    if (!strncmp(jp, "null", 4)) { jp += 4; return jnew(J_NULL); }
    J *n = jnew(J_NUM); char *e; n->num = strtod(jp, &e); jp = e; return n;
}
static J *jget(const J *o, const char *k) {
    for (int i = 0; i < o->n; i++) if (o->keys[i] && !strcmp(o->keys[i], k)) return o->kids[i];
    return NULL;
}
static double jnum(const J *o, const char *k) { J *j = jget(o, k); return j ? j->num : -12345.0; }
static const char *jstrv(const J *o, const char *k) { J *j = jget(o, k); return j && j->str ? j->str : ""; }
static void jfree(J *j) {
    if (!j) return;
    for (int i = 0; i < j->n; i++) { jfree(j->kids[i]); free(j->keys[i]); }
    free(j->kids); free(j->keys); free(j->str); free(j);
}

/* ------------------------------------------------------------------ */
/* A. Name hash vectors                                                */
/* ------------------------------------------------------------------ */
static void test_hash_vectors(void) {
    char *txt = read_file("hash_vectors.tsv", NULL);
    CHECK(txt != NULL);
    if (!txt) return;
    int rows = 0;
    char *save = NULL;
    for (char *line = strtok_r(txt, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
        if (line[0] == '#') continue;
        char *tab = strchr(line, '\t');
        if (!tab) continue;
        *tab = 0;
        const char *in = strcmp(line, "<empty>") ? line : "";
        uint32_t want = (uint32_t)strtoul(tab + 1, NULL, 16);
        CHECK_MSG(ug_hash(in) == want, "hash('%s') = %08x, want %08x", in, ug_hash(in), want);
        rows++;
    }
    CHECK(rows >= 10);
    free(txt);
    CHECK(ug_hash("") == 0xFFFFFFFFu);
    CHECK(ug_hash("A") == 0x20u);
    CHECK(ug_hash_upper("Demo_Screen.fng") == ug_hash("DEMO_SCREEN.FNG"));
    CHECK(ug_texture_key("white_box.tga") == ug_hash("WHITE_BOX"));
    CHECK(ug_texture_key("Global\\Art/Panel.bmp") == ug_hash("PANEL"));
}

/* ------------------------------------------------------------------ */
/* A. Track evaluation vectors                                         */
/* ------------------------------------------------------------------ */
static void test_track_vectors(void) {
    char *txt = read_file("track_eval_vectors.json", NULL);
    CHECK(txt != NULL);
    if (!txt) return;
    jp = txt;
    J *root = jparse();
    J *cases = jget(root, "cases");
    CHECK(cases && cases->n >= 7);
    int samples_checked = 0;
    for (int ci = 0; cases && ci < cases->n; ci++) {
        J *c = cases->kids[ci];
        const char *name = jstrv(c, "name");
        int is_col = !strcmp(jstrv(c, "kind"), "color_rgba");
        UgTrack tr;
        memset(&tr, 0, sizeof tr);
        tr.type = is_col ? UG_VT_COLOR : UG_VT_VEC3;
        tr.action = (uint8_t)jnum(c, "loop_action");
        tr.length = (uint32_t)jnum(c, "track_length_ms");
        tr.interp = UG_INTERP_LINEAR;
        J *base = jget(c, "base");
        tr.has_base = 1;
        for (int i = 0; i < base->n; i++) tr.base.v[i] = (float)base->kids[i]->num;
        J *dk = jget(c, "delta_keys");
        tr.nkeys = dk->n;
        tr.keys = (UgKey *)calloc((size_t)(dk->n ? dk->n : 1), sizeof(UgKey));
        for (int k = 0; k < dk->n; k++) {
            tr.keys[k].t = (int32_t)jnum(dk->kids[k], "t_ms");
            J *d = jget(dk->kids[k], "delta");
            for (int i = 0; i < d->n; i++) tr.keys[k].v[i] = (float)d->kids[i]->num;
        }
        J *sm = jget(c, "samples");
        for (int s = 0; s < sm->n; s++) {
            float t = (float)jnum(sm->kids[s], "t_ms"), out[4] = {0};
            J *want = jget(sm->kids[s], "value");
            ug_track_eval(&tr, t, out);
            for (int i = 0; i < want->n; i++) {
                double w = want->kids[i]->num;
                CHECK_MSG(fabs(out[i] - w) < (is_col ? 0.5 : 1e-4),
                          "%s t=%g comp %d: got %g want %g", name, t, i, out[i], w);
            }
            samples_checked++;
        }
        free(tr.keys);
    }
    CHECK(samples_checked >= 30);
    jfree(root);
    free(txt);
}

/* quaternion composition and the spec 3.5.2 base+delta rule */
static void test_track_extra(void) {
    UgKey k[1] = {{0, {0, 0, 0.70710678f, 0.70710678f}}};   /* +90 deg about z */
    UgTrack tr;
    memset(&tr, 0, sizeof tr);
    tr.type = UG_VT_QUAT; tr.interp = 1; tr.has_base = 1;
    tr.base.v[3] = 1.0f;
    tr.keys = k; tr.nkeys = 1;
    float out[4];
    ug_track_eval(&tr, 5, out);
    CHECK_NEAR(out[2], 0.70710678, 1e-4);
    CHECK_NEAR(out[3], 0.70710678, 1e-4);
    /* no delta keys -> base only */
    tr.nkeys = 0;
    ug_track_eval(&tr, 5, out);
    CHECK_NEAR(out[3], 1.0, 1e-6);
    /* float base+delta without clamping; colour clamps */
    UgKey fk[2] = {{0, {0}}, {100, {10}}};
    UgTrack ft;
    memset(&ft, 0, sizeof ft);
    ft.type = UG_VT_FLOAT; ft.has_base = 1; ft.base.v[0] = 300; ft.keys = fk; ft.nkeys = 2;
    ug_track_eval(&ft, 50, out);
    CHECK_NEAR(out[0], 305.0, 1e-4);
    ft.type = UG_VT_COLOR;
    ug_track_eval(&ft, 50, out);
    CHECK_NEAR(out[0], 255.0, 1e-4);
}

/* ------------------------------------------------------------------ */
/* A. String table                                                     */
/* ------------------------------------------------------------------ */
static void test_string_table(void) {
    size_t len = 0;
    uint8_t *bin = read_hex("string_table.hex", &len);
    CHECK(bin && len == 620);
    if (!bin) return;
    UgStrTab t;
    char err[128] = "";
    ug_strtab_init(&t);
    CHECK_MSG(ug_strtab_load(&t, bin, len, err, sizeof err), "%s", err);
    CHECK(t.n == 4);
    CHECK(!strcmp(t.name, "DEMO"));
    char *tsv = read_file("string_table.expected.tsv", NULL);
    CHECK(tsv != NULL);
    int rows = 0;
    char *save = NULL;
    for (char *line = tsv ? strtok_r(tsv, "\n", &save) : NULL; line; line = strtok_r(NULL, "\n", &save)) {
        if (line[0] == '#') continue;
        char *f1 = strchr(line, '\t'); if (!f1) continue; *f1++ = 0;
        char *f2 = strchr(f1, '\t'); if (!f2) continue; *f2++ = 0;
        uint32_t hash = (uint32_t)strtoul(f1, NULL, 16);
        CHECK_MSG(ug_hash_upper(line) == hash, "label hash %s", line);
        const char *got = ug_strtab_get(&t, hash);
        CHECK_MSG(got && !strcmp(got, f2), "text for %s: '%s' want '%s'", line, got ? got : "(null)", f2);
        rows++;
    }
    CHECK(rows == 4);
    CHECK(ug_strtab_get(&t, 0x12345678u) == NULL);
    CHECK(!strcmp(ug_strtab_or(&t, 0x12345678u, "dflt"), "dflt"));
    /* corruption is rejected rather than read out of bounds */
    UgStrTab bad;
    ug_strtab_init(&bad);
    CHECK(!ug_strtab_load(&bad, bin, len - 30, err, sizeof err));
    uint8_t *copy = (uint8_t *)malloc(len);
    memcpy(copy, bin, len);
    copy[8 + 5] = 0xFF;   /* absurd entry count (payload word 1) */
    CHECK(!ug_strtab_load(&bad, copy, len, err, sizeof err));
    free(copy);
    free(tsv);
    ug_strtab_free(&t);
    free(bin);
    /* in-memory build uses upper-cased label hashes */
    UgStrTab m;
    ug_strtab_init(&m);
    ug_strtab_add(&m, "Common_Ok", "OK");
    CHECK(!strcmp(ug_strtab_get(&m, 0x417b2601u), "OK"));
    ug_strtab_free(&m);
}

/* ------------------------------------------------------------------ */
/* A. Package parse vs expected JSON                                   */
/* ------------------------------------------------------------------ */
static void check_color(const UgColor c, const J *a, const char *what) {
    CHECK_MSG(c.r == (int)a->kids[0]->num && c.g == (int)a->kids[1]->num &&
              c.b == (int)a->kids[2]->num && c.a == (int)a->kids[3]->num,
              "%s colour %d,%d,%d,%d", what, c.r, c.g, c.b, c.a);
}
static void check_floats(const float *v, const J *a, const char *what) {
    for (int i = 0; i < a->n; i++)
        CHECK_MSG(fabs(v[i] - a->kids[i]->num) < 1e-5, "%s[%d]=%g want %g", what, i, v[i], a->kids[i]->num);
}
static void check_responses(const UgFngResponse *r, int n, const J *want, const char *mkey,
                            const char *ckey) {
    CHECK(n == want->n);
    for (int i = 0; i < want->n && i < n; i++) {
        const J *w = want->kids[i];
        CHECK((uint32_t)jnum(w, mkey) == r[i].msg || (uint32_t)strtoul(jstrv(w, mkey), NULL, 16) == r[i].msg);
        const J *cm = jget(w, ckey);
        CHECK(cm && cm->n == r[i].ncmds);
        for (int k = 0; cm && k < cm->n && k < r[i].ncmds; k++) {
            CHECK(r[i].cmds[k].id == (uint32_t)jnum(cm->kids[k], "id"));
            CHECK(r[i].cmds[k].arg == (uint32_t)jnum(cm->kids[k], "i"));
            CHECK(r[i].cmds[k].target == (uint32_t)jnum(cm->kids[k], "t"));
        }
    }
}

static void test_package_vs_json(void) {
    size_t len = 0;
    uint8_t *bin = read_hex("minimal_package.hex", &len);
    char *txt = read_file("minimal_package.expected.json", NULL);
    CHECK(bin && txt && len == 1336);
    if (!bin || !txt) { free(bin); free(txt); return; }
    UgFngPackage pk;
    char err[160] = "";
    CHECK_MSG(ug_fng_parse(bin, len, &pk, err, sizeof err), "%s", err);
    jp = txt;
    J *ex = jparse();

    CHECK(pk.name && !strcmp(pk.name, jstrv(ex, "package_name")));
    CHECK(pk.path && !strcmp(pk.path, jstrv(ex, "package_path")));
    J *hw = jget(ex, "header_words");
    for (int i = 0; i < 6; i++) CHECK(pk.header_words[i] == (uint32_t)hw->kids[i]->num);
    J *ts = jget(ex, "type_sizes");
    CHECK(pk.ntypes == ts->n);
    for (int i = 0; i < ts->n && i < pk.ntypes; i++) {
        CHECK(pk.type_sizes[i][0] == (uint32_t)ts->kids[i]->kids[0]->num);
        CHECK(pk.type_sizes[i][1] == (uint32_t)ts->kids[i]->kids[1]->num);
    }
    J *rs = jget(ex, "resources");
    CHECK(pk.nres == rs->n);
    for (int i = 0; i < rs->n && i < pk.nres; i++) {
        CHECK(pk.res[i].id == (uint32_t)jnum(rs->kids[i], "id"));
        CHECK(pk.res[i].name && !strcmp(pk.res[i].name, jstrv(rs->kids[i], "name")));
        CHECK(pk.res[i].kind == (uint32_t)jnum(rs->kids[i], "type"));
    }
    CHECK(pk.has_button_count && pk.button_count == (uint32_t)jnum(ex, "button_count"));

    J *ob = jget(ex, "objects");
    CHECK(pk.nobj == ob->n);
    for (int i = 0; i < ob->n && i < pk.nobj; i++) {
        const J *w = ob->kids[i];
        const UgFngObject *o = &pk.obj[i];
        CHECK(o->type == (uint32_t)jnum(w, "type"));
        CHECK(o->guid == (uint32_t)jnum(w, "guid"));
        CHECK(o->name_hash == (uint32_t)strtoul(jstrv(w, "name_hash"), NULL, 16));
        CHECK(o->flags == (uint32_t)strtoul(jstrv(w, "flags"), NULL, 16));
        CHECK(o->res_index == (int)jnum(w, "resource_index"));
        CHECK(o->parent_guid == (uint32_t)jnum(w, "parent"));
        check_color(o->color, jget(w, "color_rgba"), "object");
        check_floats(o->pivot, jget(w, "pivot"), "pivot");
        check_floats(o->pos, jget(w, "position"), "position");
        check_floats(o->rot, jget(w, "rotation_xyzw"), "rotation");
        check_floats(o->size, jget(w, "size"), "size");
        J *xf = jget(w, "extra_floats");
        if (xf->n) { CHECK(o->has_uv); check_floats(o->uv, xf, "uv"); }
        J *sc = jget(w, "scripts");
        CHECK(sc->n == o->nscripts);
        for (int s = 0; s < sc->n && s < o->nscripts; s++) {
            const J *ws = sc->kids[s];
            const UgScript *us = &o->scripts[s];
            CHECK(us->id == (uint32_t)strtoul(jstrv(ws, "id"), NULL, 16));
            CHECK(us->length == (uint32_t)jnum(ws, "length_ms"));
            CHECK(us->flags == (uint32_t)jnum(ws, "flags"));
            J *tk = jget(ws, "tracks");
            CHECK(tk->n == us->ntracks);
            for (int t = 0; t < tk->n && t < us->ntracks; t++) {
                const J *wt = tk->kids[t];
                const UgTrack *ut = &us->tracks[t];
                CHECK(ut->type == (int)jnum(wt, "param_type"));
                CHECK(ut->interp == (int)jnum(wt, "interp"));
                CHECK(ut->action == (int)jnum(wt, "action"));
                CHECK(ut->length == (uint32_t)jnum(wt, "length"));
                CHECK(ut->prop == (uint32_t)jnum(wt, "property_offset"));
                J *kk = jget(wt, "keys");
                /* expected lists the t=-1 base key first */
                CHECK(ut->has_base && ut->nkeys + 1 == kk->n);
                for (int k = 0; k < kk->n; k++) {
                    const UgKey *key = k == 0 ? &ut->base : (k - 1 < ut->nkeys ? &ut->keys[k - 1] : NULL);
                    if (!key) continue;
                    CHECK(key->t == (int)kk->kids[k]->kids[0]->num);
                    const J *vals = kk->kids[k]->kids[1];
                    for (int c = 0; c < vals->n; c++)
                        CHECK_MSG(fabs(key->v[c] - vals->kids[c]->num) < 1e-5,
                                  "obj %d script %d track %d key %d comp %d", i, s, t, k, c);
                }
            }
            J *ev = jget(ws, "events");
            CHECK(ev->n == us->nevents);
            for (int e = 0; e < ev->n && e < us->nevents; e++) {
                CHECK(us->events[e].msg == (uint32_t)strtoul(ev->kids[e]->kids[0]->str, NULL, 16));
                CHECK(us->events[e].target == (uint32_t)strtoul(ev->kids[e]->kids[1]->str, NULL, 16));
                CHECK(us->events[e].t_ms == (uint32_t)ev->kids[e]->kids[2]->num);
            }
        }
        check_responses(o->resp, o->nresp, jget(w, "responses"), "message", "commands");
        if (jget(w, "text")) {
            CHECK(o->is_text && o->text && !strcmp(o->text, jstrv(w, "text")));
            CHECK(o->buffer_len == (uint32_t)jnum(w, "buffer_len"));
            CHECK(o->justify == (uint32_t)jnum(w, "justify"));
            CHECK(o->leading == (int)jnum(w, "leading"));
            CHECK(o->max_width == (uint32_t)jnum(w, "max_width"));
            CHECK(o->has_label_hash && o->label_hash == (uint32_t)strtoul(jstrv(w, "label_hash"), NULL, 16));
        }
    }
    J *pr = jget(ex, "package_responses");
    CHECK(pk.npkg_resp == pr->n);
    check_responses(pk.pkg_resp, pk.npkg_resp, pr, "msg", "cmds");

    J *tg = jget(ex, "targets");
    CHECK(pk.has_targets && pk.target_count == (uint32_t)jnum(tg->kids[0], "Tc"));
    CHECK(pk.ntargets == tg->n - 1);
    for (int i = 1; i < tg->n && i - 1 < pk.ntargets; i++) {
        const J *mt = jget(tg->kids[i], "Mt");
        const UgFngTarget *t = &pk.targets[i - 1];
        CHECK(t->msg == (uint32_t)mt->kids[0]->num);
        CHECK(t->nguids == mt->n - 1);
        for (int g = 0; g < t->nguids; g++) CHECK(t->guids[g] == (uint32_t)mt->kids[g + 1]->num);
    }

    /* structural facts from SPEC 3.4 / 3.5 */
    const UgFngObject *btn = ug_fng_find_guid(&pk, 0x101);
    CHECK(btn && (btn->flags & UG_OF_BUTTON) && btn->parent_guid == 0x100 && btn->has_parent);
    CHECK(pk.obj[0].has_parent == 0);
    CHECK(pk.obj[0].scripts[0].id == UG_SCRIPT_INITIAL);

    jfree(ex);
    ug_fng_free(&pk);

    /* truncation / overrun is rejected cleanly */
    CHECK(!ug_fng_parse(bin, len - 4, &pk, err, sizeof err));
    CHECK(!ug_fng_parse(bin, 6, &pk, err, sizeof err));
    uint8_t *mut = (uint8_t *)malloc(len);
    memcpy(mut, bin, len);
    mut[4] = 0xFF; mut[5] = 0xFF;              /* root size absurd */
    CHECK(!ug_fng_parse(mut, len, &pk, err, sizeof err));
    free(mut);
    /* a bundle wrapper (0x00030203) around the package is accepted */
    uint8_t *wrap = (uint8_t *)malloc(len + 8);
    uint32_t id = 0x00030203u, sz = (uint32_t)len;
    memcpy(wrap, &id, 4); memcpy(wrap + 4, &sz, 4); memcpy(wrap + 8, bin, len);
    CHECK(ug_fng_parse(wrap, len + 8, &pk, err, sizeof err));
    ug_fng_free(&pk);
    /* a compressed stream that decodes to nothing useful is rejected with a message */
    uint8_t jd[16] = {'J', 'D', 'L', 'Z', 2, 0x10, 0};
    CHECK(!ug_fng_parse(jd, sizeof jd, &pk, err, sizeof err) && err[0]);
    free(wrap);
    free(txt);
    free(bin);
}

#include "ugui_test_more.inc"
#include "ugui_test_assets.inc"
#include "ugui_test_amend3.inc"
#include "ugui_test_amend4.inc"
#include "ugui_test_amend5.inc"
#include "ugui_test_amend6_xbox.inc"
#include "ugui_test_amend6_demo.inc"
#include "ugui_test_amend8.inc"
#include "ugui_test_folder.inc"

int main(int argc, char **argv) {
    if (argc > 1) g_vec_dir = argv[1];
    printf("ugui tests (vectors: %s)\n", g_vec_dir);
    printf("[A] spec test vectors\n");
    RUN(test_hash_vectors);
    RUN(test_track_vectors);
    RUN(test_track_extra);
    RUN(test_string_table);
    RUN(test_package_vs_json);
    run_more_tests();
    run_asset_tests();
    run_amend3_tests();
    run_amend4_tests();
    run_amend5_tests();
    run_amend6_xbox_tests();
    run_amend6_demo_tests();
    run_amend8_tests();
    run_folder_tests();
    if(argc==4&&!strcmp(argv[2],"--test-glyph-folder"))runtime_test_glyph_folder(argv[3]);
    printf("%d checks, %d failures\n", g_checks, g_fails);
    return g_fails ? 1 : 0;
}
