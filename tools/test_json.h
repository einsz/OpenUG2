/* test_json.h -- minimal JSON DOM shared by the vector-driven tests (options_menu_test,
 * gui_art_test). The includer defines `static const char *g_dir;` (vector directory) and
 * `static int g_fail;` before including this file. */
#ifndef TEST_JSON_H
#define TEST_JSON_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- minimal JSON DOM ------------------------------------------------------------ */
enum { J_NULL, J_BOOL, J_NUM, J_STR, J_ARR, J_OBJ };
typedef struct J { int t; double num; char *s; struct J **kid; char **key; int n, cap; } J;
static const char *jp;
static void jws(void) { while (*jp == ' ' || *jp == '\n' || *jp == '\r' || *jp == '\t') jp++; }
static J *jnew(int t) { J *j = (J *)calloc(1, sizeof *j); j->t = t; return j; }
static void jadd(J *c, char *key, J *v) {
    if (c->n == c->cap) {
        c->cap = c->cap ? c->cap * 2 : 8;
        c->kid = (J **)realloc(c->kid, (size_t)c->cap * sizeof *c->kid);
        c->key = (char **)realloc(c->key, (size_t)c->cap * sizeof *c->key);
    }
    c->kid[c->n] = v; c->key[c->n] = key; c->n++;
}
static char *jstr(void) {
    char *out = (char *)malloc(strlen(jp) + 1), *w = out;
    jp++;
    while (*jp && *jp != '"') {
        if (*jp == '\\' && jp[1]) { jp++; *w++ = *jp == 'n' ? '\n' : *jp == 't' ? '\t' : *jp; jp++; }
        else *w++ = *jp++;
    }
    if (*jp == '"') jp++;
    *w = 0;
    return out;
}
static J *jparse(void) {
    jws();
    if (*jp == '{' || *jp == '[') {
        int obj = *jp == '{';
        J *c = jnew(obj ? J_OBJ : J_ARR);
        char close = obj ? '}' : ']';
        jp++; jws();
        while (*jp && *jp != close) {
            char *k = NULL;
            if (obj) { jws(); k = jstr(); jws(); if (*jp == ':') jp++; }
            jadd(c, k, jparse());
            jws();
            if (*jp == ',') jp++;
            jws();
        }
        if (*jp == close) jp++;
        return c;
    }
    if (*jp == '"') { J *j = jnew(J_STR); j->s = jstr(); return j; }
    if (!strncmp(jp, "null", 4)) { jp += 4; return jnew(J_NULL); }
    if (!strncmp(jp, "true", 4)) { jp += 4; J *j = jnew(J_BOOL); j->num = 1; return j; }
    if (!strncmp(jp, "false", 5)) { jp += 5; return jnew(J_BOOL); }
    J *j = jnew(J_NUM);
    char *e;
    j->num = strtod(jp, &e);
    if (e == jp) e++;                       /* never loop on garbage */
    jp = e;
    return j;
}
static J *jload(const char *name) {
    char path[1024];
    snprintf(path, sizeof path, "%s/%s", g_dir, name);
    FILE *f = fopen(path, "rb");
    if (!f) { printf("FAIL cannot read %s\n", path); g_fail++; return NULL; }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *d = (char *)malloc((size_t)n + 1);
    size_t got = fread(d, 1, (size_t)n, f);
    fclose(f);
    d[got] = 0;
    jp = d;
    J *j = jparse();
    free(d);
    return j;
}
static J *jget(const J *o, const char *k) {
    if (!o || o->t != J_OBJ) return NULL;
    for (int i = 0; i < o->n; i++) if (!strcmp(o->key[i], k)) return o->kid[i];
    return NULL;
}
static const char *jstrv(const J *o, const char *k) { J *v = jget(o, k); return v && v->t == J_STR ? v->s : NULL; }
static double jnum(const J *o, const char *k, double def) { J *v = jget(o, k); return v && v->t == J_NUM ? v->num : def; }
static int jbool(const J *o, const char *k) { J *v = jget(o, k); return v && v->t == J_BOOL && v->num != 0; }
static void jfree(J *j) {
    if (!j) return;
    for (int i = 0; i < j->n; i++) { jfree(j->kid[i]); free(j->key ? j->key[i] : NULL); }
    free(j->kid); free(j->key); free(j->s); free(j);
}
static uint32_t hexkey(const char *s) { return (uint32_t)strtoul(s, NULL, 16); }

#endif
