/* ugui_anim.c -- tracks, script playback (SPEC 3.5) and the standard
 * animation set (SPEC 5). GL-free. */
#include "ugui_core.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Track evaluation                                                    */
/* ------------------------------------------------------------------ */
int ug_type_ncomp(int vt) {
    switch (vt) {
    case UG_VT_BOOL: case UG_VT_INT: case UG_VT_FLOAT: return 1;
    case UG_VT_VEC2: return 2;
    case UG_VT_VEC3: return 3;
    case UG_VT_QUAT: case UG_VT_COLOR: return 4;
    }
    return 1;
}

static void lerp_vec(const float *a, const float *b, float f, int n, float *o) {
    for (int i = 0; i < n; i++) o[i] = a[i] + (b[i] - a[i]) * f;
}

/* delta value of the track at time t (see SPEC 3.5.3 steps 1-3) */
static void delta_at(const UgTrack *tr, float t, int n, float d[4]) {
    int nk = tr->nkeys;
    memset(d, 0, 4 * sizeof(float));
    if (nk <= 0) return;
    const UgKey *k = tr->keys;
    float t0 = (float)k[0].t, tn = (float)k[nk - 1].t;
    int wrap = (tr->action & UG_ACT_WRAP) && tr->length > 0;

    if (!wrap) {
        if (nk == 1 || t <= t0) { memcpy(d, k[0].v, sizeof(float) * n); return; }
        if (t >= tn) { memcpy(d, k[nk - 1].v, sizeof(float) * n); return; }
        for (int i = 0; i + 1 < nk; i++) {
            float ta = (float)k[i].t, tb = (float)k[i + 1].t;
            if (ta <= t && t < tb) {
                lerp_vec(k[i].v, k[i + 1].v, (t - ta) / (tb - ta), n, d);
                return;
            }
        }
        memcpy(d, k[nk - 1].v, sizeof(float) * n);
        return;
    }

    float L = (float)tr->length;
    t = fmodf(t, L);
    if (t < 0) t += L;
    if (t < t0) {
        float ta = tn - L;
        lerp_vec(k[nk - 1].v, k[0].v, (t - ta) / (t0 - ta), n, d);
        return;
    }
    if (t >= tn) {
        float tb = t0 + L;
        lerp_vec(k[nk - 1].v, k[0].v, (t - tn) / (tb - tn), n, d);
        return;
    }
    for (int i = 0; i + 1 < nk; i++) {
        float ta = (float)k[i].t, tb = (float)k[i + 1].t;
        if (ta <= t && t < tb) {
            lerp_vec(k[i].v, k[i + 1].v, (t - ta) / (tb - ta), n, d);
            return;
        }
    }
    memcpy(d, k[nk - 1].v, sizeof(float) * n);
}

static void quat_mul(const float a[4], const float b[4], float o[4]) {
    /* (x,y,z,w) Hamilton product a*b */
    o[0] = a[3]*b[0] + a[0]*b[3] + a[1]*b[2] - a[2]*b[1];
    o[1] = a[3]*b[1] - a[0]*b[2] + a[1]*b[3] + a[2]*b[0];
    o[2] = a[3]*b[2] + a[0]*b[1] - a[1]*b[0] + a[2]*b[3];
    o[3] = a[3]*b[3] - a[0]*b[0] - a[1]*b[1] - a[2]*b[2];
}

void ug_track_eval(const UgTrack *tr, float t_ms, float out[4]) {
    int n = ug_type_ncomp(tr->type);
    float base[4] = {0, 0, 0, 0}, d[4];
    if (tr->has_base) memcpy(base, tr->base.v, sizeof base);
    if (tr->type == UG_VT_QUAT && !tr->has_base) base[3] = 1.0f;

    if (tr->nkeys == 0) {
        memcpy(out, base, sizeof base);
    } else if (tr->type == UG_VT_QUAT) {
        delta_at(tr, t_ms, 4, d);
        float len = sqrtf(d[0]*d[0] + d[1]*d[1] + d[2]*d[2] + d[3]*d[3]);
        if (len > 1e-6f) for (int i = 0; i < 4; i++) d[i] /= len;
        else { d[0] = d[1] = d[2] = 0; d[3] = 1; }
        quat_mul(d, base, out);     /* SPEC 3.5.2 (amendment 2): delta * base */
    } else {
        delta_at(tr, t_ms, n, d);
        for (int i = 0; i < n; i++) out[i] = base[i] + d[i];
    }
    if (tr->type == UG_VT_COLOR) {
        for (int i = 0; i < 4; i++) out[i] = (float)ug_clamp8(out[i]);
    } else if (tr->type == UG_VT_INT) {
        out[0] = floorf(out[0] + 0.5f);
    } else if (tr->type == UG_VT_BOOL) {
        out[0] = out[0] >= 0.5f ? 1.0f : 0.0f;
    }
}

void ug_script_apply(const UgScript *s, float t_ms, UgProps *p) {
    for (int i = 0; i < s->ntracks; i++) {
        const UgTrack *tr = &s->tracks[i];
        int n = ug_type_ncomp(tr->type);
        if (tr->prop + (uint32_t)n > UG_NPROPS) continue;
        float v[4];
        ug_track_eval(tr, t_ms, v);
        for (int c = 0; c < n; c++) p->w[tr->prop + c] = v[c];
    }
}

void ug_script_free(UgScript *s) {
    if (!s) return;
    for (int i = 0; i < s->ntracks; i++) free(s->tracks[i].keys);
    free(s->tracks);
    free(s->events);
    memset(s, 0, sizeof *s);
}

/* ------------------------------------------------------------------ */
/* Player                                                              */
/* ------------------------------------------------------------------ */
void ug_player_init(UgPlayer *pl, const UgScript *scripts, int n,
                    UgEmitFn emit, void *user) {
    memset(pl, 0, sizeof *pl);
    pl->scripts = scripts;
    pl->nscripts = n;
    pl->emit = emit;
    pl->user = user;
}

const UgScript *ug_player_find(const UgPlayer *pl, uint32_t id) {
    for (int i = 0; i < pl->nscripts; i++)
        if (pl->scripts[i].id == id) return &pl->scripts[i];
    return NULL;
}

/* fire events with lo < t <= hi, in time order */
static void fire(UgPlayer *pl, const UgScript *s, float lo, float hi) {
    if (!pl->emit || s->nevents == 0) return;
    int idx[64], n = 0;
    for (int i = 0; i < s->nevents && n < 64; i++) {
        float t = (float)s->events[i].t_ms;
        if (t > lo && t <= hi) {
            int j = n++;
            while (j > 0 && s->events[idx[j - 1]].t_ms > s->events[i].t_ms) {
                idx[j] = idx[j - 1];
                j--;
            }
            idx[j] = i;
        }
    }
    for (int i = 0; i < n; i++) pl->emit(pl->user, &s->events[idx[i]]);
}

void ug_player_start_script(UgPlayer *pl, const UgScript *s, UgProps *p) {
    pl->cur = s;
    pl->t = 0;
    pl->running = s != NULL;
    if (!s) return;
    ug_script_apply(s, 0, p);
    fire(pl, s, -1.0f, 0.0f);
}

int ug_player_start(UgPlayer *pl, uint32_t id, UgProps *p) {
    const UgScript *s = ug_player_find(pl, id);
    if (!s) return 0;
    ug_player_start_script(pl, s, p);
    return 1;
}

void ug_player_update(UgPlayer *pl, float dt, UgProps *p) {
    if (!pl->cur || !pl->running) return;
    float prev = pl->t, t = prev + dt;
    for (int guard = 0; guard < 16; guard++) {
        const UgScript *s = pl->cur;
        float len = (float)s->length;
        if (t < len) {
            fire(pl, s, prev, t);
            ug_script_apply(s, t, p);
            pl->t = t;
            return;
        }
        fire(pl, s, prev, len);
        if (s->has_chain) {
            const UgScript *nx = ug_player_find(pl, s->chain);
            if (nx) {
                ug_script_apply(s, len, p);
                t -= len;
                pl->cur = nx;
                prev = -1.0f;
                continue;
            }
        }
        if (s->flags & UG_SCRIPT_LOOP) {
            if (len <= 0.0f) { ug_script_apply(s, 0, p); pl->t = 0; return; }
            t -= len;                      /* one cycle at a time so no loop event is skipped */
            prev = 0.0f;                   /* SPEC 3.5.4: t = 0 events fire on start only, not on a wrap */
            continue;
        }
        ug_script_apply(s, len, p);
        pl->t = len;
        pl->running = 0;
        return;
    }
    /* pathological dt (more than 16 transitions): settle on the wrapped time */
    if (pl->cur && (pl->cur->flags & UG_SCRIPT_LOOP) && pl->cur->length > 0) t = fmodf(t, (float)pl->cur->length);
    ug_script_apply(pl->cur, t, p);
    pl->t = t;
}

/* ------------------------------------------------------------------ */
/* Standard animations                                                 */
/* ------------------------------------------------------------------ */
static const struct { const char *name; int dur; } ANIM_INFO[UG_ANIM_COUNT] = {
    {"Fade-in", 180}, {"Fade-out", 140}, {"Dim", 100}, {"Undim", 100},
    {"Focus pulse", 1200}, {"Row focus", 0}, {"Row unfocus", 0},
    {"Blink", 400}, {"Icon breathe", 1200}, {"Pill focus", 1200},
    {"Pill glow", 1200}, {"Activity arrows", 3600}, {"Strip swap out", 320},
    {"Strip swap in", 320}, {"Disabled", 0},
};

const char *ug_anim_name(UgStdAnim k) {
    return (k >= 0 && k < UG_ANIM_COUNT) ? ANIM_INFO[k].name : "?";
}
int ug_anim_duration(UgStdAnim k) {
    return (k >= 0 && k < UG_ANIM_COUNT) ? ANIM_INFO[k].dur : 0;
}

void ug_props_default(UgProps *p, UgColor c, float w, float h) {
    memset(p, 0, sizeof *p);
    p->w[0] = c.r; p->w[1] = c.g; p->w[2] = c.b; p->w[3] = c.a;
    p->w[UG_P_ROT + 3] = 1.0f;
    p->w[UG_P_SIZE] = w; p->w[UG_P_SIZE + 1] = h; p->w[UG_P_SIZE + 2] = 1.0f;
    p->w[UG_P_UV1] = 1.0f; p->w[UG_P_UV1 + 1] = 1.0f;
}

static UgTrack *add_track(UgScript *s, int type, uint32_t prop, int action,
                          const float *base, int nkeys) {
    s->tracks = (UgTrack *)realloc(s->tracks, sizeof(UgTrack) * (s->ntracks + 1));
    UgTrack *t = &s->tracks[s->ntracks++];
    memset(t, 0, sizeof *t);
    t->type = (uint8_t)type;
    t->vsize = (uint8_t)(ug_type_ncomp(type) * 4);
    t->interp = UG_INTERP_LINEAR;
    t->action = (uint8_t)action;
    t->length = s->length;
    t->prop = prop;
    t->has_base = 1;
    t->base.t = -1;
    memcpy(t->base.v, base, sizeof(float) * 4);
    t->nkeys = nkeys;
    t->keys = (UgKey *)calloc((size_t)nkeys, sizeof(UgKey));
    return t;
}

static void key(UgTrack *t, int i, int32_t time, float a, float b, float c, float d) {
    t->keys[i].t = time;
    t->keys[i].v[0] = a; t->keys[i].v[1] = b; t->keys[i].v[2] = c; t->keys[i].v[3] = d;
}

void ug_anim_build(UgStdAnim k, const UgProps *rest, int param, UgScript *out) {
    memset(out, 0, sizeof *out);
    out->id = (uint32_t)k;
    out->length = (uint32_t)ug_anim_duration(k);
    float rc[4] = {rest->w[0], rest->w[1], rest->w[2], rest->w[3]};
    float A = rc[3];
    const float ac[4] = {UG_ACCENT.r, UG_ACCENT.g, UG_ACCENT.b, 255};
    UgTrack *t;
    switch (k) {
    case UG_ANIM_FADE_IN:        /* alpha 0 for 40 ms, then linear to rest */
        t = add_track(out, UG_VT_COLOR, UG_P_COLOR, 0, rc, 2);
        key(t, 0, 40, 0, 0, 0, -A);
        key(t, 1, 180, 0, 0, 0, 0);
        break;
    case UG_ANIM_FADE_OUT:       /* linear to 0 by 100 ms, hold */
        t = add_track(out, UG_VT_COLOR, UG_P_COLOR, 0, rc, 2);
        key(t, 0, 0, 0, 0, 0, 0);
        key(t, 1, 100, 0, 0, 0, -A);
        break;
    case UG_ANIM_DIM: case UG_ANIM_UNDIM: {
        float target = param ? 0.0f : 60.0f;
        float d = (A > target ? target - A : 0.0f);
        t = add_track(out, UG_VT_COLOR, UG_P_COLOR, 0, rc, 2);
        if (k == UG_ANIM_DIM) { key(t, 0, 0, 0, 0, 0, 0); key(t, 1, 100, 0, 0, 0, d); }
        else                  { key(t, 0, 0, 0, 0, 0, d); key(t, 1, 100, 0, 0, 0, 0); }
        break;
    }
    case UG_ANIM_FOCUS_PULSE:    /* accent, alpha 0->255->hold->0, loop */
        out->flags = UG_SCRIPT_LOOP;
        t = add_track(out, UG_VT_COLOR, UG_P_COLOR, UG_ACT_WRAP, ac, 4);
        key(t, 0, 0, 0, 0, 0, -255);
        key(t, 1, 440, 0, 0, 0, 0);
        key(t, 2, 760, 0, 0, 0, 0);
        key(t, 3, 1200, 0, 0, 0, -255);
        break;
    case UG_ANIM_ROW_FOCUS: case UG_ANIM_ROW_UNFOCUS: {
        float cb[4] = {rc[0], rc[1], rc[2], 80};
        float pb[4] = {rest->w[UG_P_POS], rest->w[UG_P_POS + 1], 100, 0};
        int focus = (k == UG_ANIM_ROW_FOCUS);
        t = add_track(out, UG_VT_COLOR, UG_P_COLOR, UG_ACT_CONSTANT, cb, focus ? 1 : 0);
        if (focus) key(t, 0, 0, 0, 0, 0, 120);       /* alpha 80 -> 200 */
        t = add_track(out, UG_VT_VEC3, UG_P_POS, UG_ACT_CONSTANT, pb, focus ? 1 : 0);
        if (focus) key(t, 0, 0, 0, 0, -11, 0);       /* z 100 -> 89 */
        break;
    }
    case UG_ANIM_BLINK: {        /* tint accent, alpha 150-255-150, size +6 */
        float dr = ac[0] - rc[0], dg = ac[1] - rc[1], db = ac[2] - rc[2];
        t = add_track(out, UG_VT_COLOR, UG_P_COLOR, 0, rc, 3);
        key(t, 0, 0, dr, dg, db, 150 - A);
        key(t, 1, 200, dr, dg, db, 255 - A);
        key(t, 2, 400, 0, 0, 0, 0);
        float sb[4] = {rest->w[UG_P_SIZE], rest->w[UG_P_SIZE + 1], rest->w[UG_P_SIZE + 2], 0};
        t = add_track(out, UG_VT_VEC3, UG_P_SIZE, 0, sb, 3);
        key(t, 0, 0, 0, 0, 0, 0);
        key(t, 1, 200, 6, 6, 0, 0);
        key(t, 2, 400, 0, 0, 0, 0);
        break;
    }
    case UG_ANIM_BREATHE:        /* full 0-320, fade by 640, hold 0 to 1000 */
        out->flags = UG_SCRIPT_LOOP;
        t = add_track(out, UG_VT_COLOR, UG_P_COLOR, UG_ACT_WRAP, ac, 5);
        key(t, 0, 0, 0, 0, 0, 0);
        key(t, 1, 320, 0, 0, 0, 0);
        key(t, 2, 640, 0, 0, 0, -255);
        key(t, 3, 1000, 0, 0, 0, -255);
        key(t, 4, 1200, 0, 0, 0, 0);
        break;
    case UG_ANIM_PILL_FOCUS: {   /* white/150 -> accent/255 at 500, hold 700 */
        float cb[4] = {255, 255, 255, 150};
        out->flags = UG_SCRIPT_LOOP;
        t = add_track(out, UG_VT_COLOR, UG_P_COLOR, UG_ACT_WRAP, cb, 4);
        key(t, 0, 0, 0, 0, 0, 0);
        key(t, 1, 500, ac[0] - 255, ac[1] - 255, ac[2] - 255, 105);
        key(t, 2, 700, ac[0] - 255, ac[1] - 255, ac[2] - 255, 105);
        key(t, 3, 1200, 0, 0, 0, 0);
        break;
    }
    case UG_ANIM_PILL_GLOW: {
        float cb[4] = {255, 255, 255, 0};
        out->flags = UG_SCRIPT_LOOP;
        t = add_track(out, UG_VT_COLOR, UG_P_COLOR, UG_ACT_WRAP, cb, 4);
        key(t, 0, 0, 0, 0, 0, 0);
        key(t, 1, 500, 0, 0, 0, 255);
        key(t, 2, 700, 0, 0, 0, 255);
        key(t, 3, 1200, 0, 0, 0, 0);
        break;
    }
    case UG_ANIM_ACTIVITY: {     /* three badges light in sequence */
        float cb[4] = {255, 255, 255, 60};
        int a = (param % 3) * 1200;
        out->flags = UG_SCRIPT_LOOP;
        t = add_track(out, UG_VT_COLOR, UG_P_COLOR, UG_ACT_WRAP, cb, 3);
        key(t, 0, a, 0, 0, 0, 0);
        key(t, 1, a + 300, 0, 0, 0, 195);
        key(t, 2, a + 900, 0, 0, 0, 0);
        break;
    }
    case UG_ANIM_STRIP_OUT: case UG_ANIM_STRIP_IN:
        t = add_track(out, UG_VT_COLOR, UG_P_COLOR, 0, rc, 3);
        if (k == UG_ANIM_STRIP_OUT) {
            key(t, 0, 0, 0, 0, 0, 0); key(t, 1, 160, 0, 0, 0, -A); key(t, 2, 320, 0, 0, 0, -A);
        } else {
            key(t, 0, 0, 0, 0, 0, -A); key(t, 1, 160, 0, 0, 0, -A); key(t, 2, 320, 0, 0, 0, 0);
        }
        break;
    case UG_ANIM_DISABLED:       /* alpha 120, zero length */
        t = add_track(out, UG_VT_COLOR, UG_P_COLOR, 0, rc, 1);
        key(t, 0, 0, 0, 0, 0, 120 - A);
        break;
    default: break;
    }
    for (int i = 0; i < out->ntracks; i++) out->tracks[i].length = out->length;
}

/* ------------------------------------------------------------------ */
/* UgAnim owner                                                        */
/* ------------------------------------------------------------------ */
void ug_anim_init(UgAnim *a, UgColor c, float w, float h) {
    memset(a, 0, sizeof *a);
    ug_props_default(&a->rest, c, w, h);
    a->props = a->rest;
    ug_player_init(&a->pl, &a->sc, 1, NULL, NULL);
}

void ug_anim_free(UgAnim *a) {
    if (a->has_script) ug_script_free(&a->sc);
    a->has_script = 0;
}

void ug_anim_play(UgAnim *a, UgStdAnim k, int param) {
    ug_anim_free(a);
    ug_anim_build(k, &a->rest, param, &a->sc);
    a->has_script = 1;
    a->kind = k;
    a->props = a->rest;
    ug_player_init(&a->pl, &a->sc, 1, NULL, NULL);
    ug_player_start_script(&a->pl, &a->sc, &a->props);
}

void ug_anim_stop(UgAnim *a) {
    ug_anim_free(a);
    a->pl.cur = NULL;
    a->pl.running = 0;
    a->props = a->rest;
}

void ug_anim_update(UgAnim *a, float dt) {
    if (a->has_script) ug_player_update(&a->pl, dt, &a->props);
}

int ug_anim_running(const UgAnim *a) {
    return a->has_script && a->pl.running;
}

UgColor ug_anim_color(const UgAnim *a) {
    return UG_RGBA(ug_clamp8(a->props.w[0]), ug_clamp8(a->props.w[1]),
                   ug_clamp8(a->props.w[2]), ug_clamp8(a->props.w[3]));
}

float ug_anim_alpha01(const UgAnim *a) {
    float v = a->props.w[3] / 255.0f;
    return v < 0 ? 0 : (v > 1 ? 1 : v);
}
