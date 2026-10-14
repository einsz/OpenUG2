/* ugui_scene.c -- see ugui_scene.h. */
#include "ugui_scene.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "ugui_tuning.h"

#define BROADCAST_ALT 0xFFFFFFFCu
#define TO_APP        0xFFFFFFFFu

static void app_push(UgScene *s, uint32_t msg) {
    int nt = (s->aqt + 1) % 32;
    if (nt == s->aqh) return;
    s->appq[s->aqt] = msg;
    s->aqt = nt;
}

int ug_scene_poll_app(UgScene *s, uint32_t *msg) {
    if (s->aqh == s->aqt) return 0;
    *msg = s->appq[s->aqh];
    s->aqh = (s->aqh + 1) % 32;
    return 1;
}

int ug_scene_find_guid(const UgScene *s, uint32_t guid) {
    for (int i = 0; i < s->n; i++) if (s->nodes[i].obj->guid == guid) return i;
    return -1;
}

uint32_t ug_scene_script_id(const UgScene *s, int node) {
    if (node < 0 || node >= s->n) return 0;
    const UgNode *n = &s->nodes[node];
    /* a script that has run to its end no longer counts as the current one: the "if the current
     * script is not the fade-in" guard (3.6.2) exists to protect a *running* entry animation */
    return (n->pl.cur && n->pl.running) ? n->pl.cur->id : 0;
}

/* ---- message dispatch ---- */
static void deliver(UgScene *s, int node, uint32_t msg);
static void run_commands(UgScene *s, int node, const UgFngResponse *r);

static const UgFngResponse *find_resp(const UgFngResponse *r, int n, uint32_t msg) {
    for (int i = 0; i < n; i++) if (r[i].msg == msg) return &r[i];
    return NULL;
}

static void emit_cb(void *user, const UgEvent *e);

static void deliver(UgScene *s, int node, uint32_t msg) {
    if (s->depth > 8) return;
    const UgFngResponse *r = find_resp(s->nodes[node].obj->resp, s->nodes[node].obj->nresp, msg);
    if (!r) return;
    s->depth++;
    run_commands(s, node, r);
    s->depth--;
}

static void broadcast(UgScene *s, uint32_t msg) {
    /* every object that responds to the message (equivalent to the Targ index) */
    for (int i = 0; i < s->n; i++) deliver(s, i, msg);
}

static void post_to(UgScene *s, uint32_t msg, uint32_t target) {
    if (target == TO_APP) { app_push(s, msg); return; }
    if (target == 0 || target == BROADCAST_ALT) { broadcast(s, msg); return; }
    int i = ug_scene_find_guid(s, target);
    if (i >= 0) deliver(s, i, msg);
}

void ug_scene_post(UgScene *s, uint32_t msg, uint32_t target) { post_to(s, msg, target); }

static void run_pkg_responses(UgScene *s, uint32_t msg);

static void run_commands(UgScene *s, int node, const UgFngResponse *r) {
    int stack[16], sp = 0;      /* each entry: 1 = branch executing */
    int active = 1;
    for (int i = 0; i < r->ncmds; i++) {
        const UgFngCmd *c = &r->cmds[i];
        switch (c->id) {
        case UG_CMD_IF_SCRIPT_IS: case UG_CMD_IF_SCRIPT_NOT: {
            int cur = node >= 0 ? (int)(ug_scene_script_id(s, node) == c->arg) : 0;
            int cond = c->id == UG_CMD_IF_SCRIPT_IS ? cur : !cur;
            if (sp < 16) stack[sp++] = active;
            active = active && cond;
            continue;
        }
        case UG_CMD_ELSE:
            if (sp > 0) active = stack[sp - 1] && !active;
            continue;
        case UG_CMD_ENDIF:
            if (sp > 0) active = stack[--sp];
            continue;
        default: break;
        }
        if (!active) continue;
        switch (c->id) {
        case UG_CMD_START_SCRIPT:
            if (node >= 0) ug_player_start(&s->nodes[node].pl, c->arg, &s->nodes[node].props);
            break;
        case UG_CMD_POST_GUI:
            if (c->target == 0) ug_scene_input_msg(s, c->arg);   /* re-dispatch as input */
            else post_to(s, c->arg, c->target);
            break;
        case UG_CMD_POST_APP:
            app_push(s, c->arg);
            break;
        case UG_CMD_FOCUS: {
            int t = ug_scene_find_guid(s, c->arg);
            if (t >= 0) ug_scene_set_focus(s, t);
            break;
        }
        case UG_CMD_FOCUS_SAVE: s->saved_focus = s->focus; break;
        case UG_CMD_FOCUS_RESTORE: {
            int t = s->saved_focus;
            if (t < 0) t = ug_scene_find_guid(s, c->arg);
            if (t >= 0) ug_scene_set_focus(s, t);
            break;
        }
        default: break;     /* sound, screen stack and routing commands: application level */
        }
    }
}

static void run_pkg_responses(UgScene *s, uint32_t msg) {
    const UgFngResponse *r = find_resp(s->pk->pkg_resp, s->pk->npkg_resp, msg);
    if (!r) return;
    if (s->depth > 8) return;
    s->depth++;
    run_commands(s, -1, r);
    s->depth--;
}

int ug_scene_input_msg(UgScene *s, uint32_t msg) {
    int handled = 0;
    if (s->focus >= 0 &&
        find_resp(s->nodes[s->focus].obj->resp, s->nodes[s->focus].obj->nresp, msg)) {
        deliver(s, s->focus, msg);
        handled = 1;
    }
    if (find_resp(s->pk->pkg_resp, s->pk->npkg_resp, msg)) {
        run_pkg_responses(s, msg);
        handled = 1;
    }
    if (msg == UG_MSG_ACCEPT && s->focus >= 0) {
        deliver(s, s->focus, UG_MSG_PRESSED);
        handled = 1;
    }
    return handled;
}

static void emit_cb(void *user, const UgEvent *e) {
    UgNode *n = (UgNode *)user;
    post_to(n->scene, e->msg, e->target);
}

/* ---- layout ---- */
static void node_local(const UgNode *n, float m[12]) {
    const float *p = n->props.w;
    float x=p[UG_P_ROT], y=p[UG_P_ROT+1], z=p[UG_P_ROT+2], w=p[UG_P_ROT+3];
    float norm=sqrtf(x*x+y*y+z*z+w*w);
    if (norm > 1e-8f) { x/=norm; y/=norm; z/=norm; w/=norm; } else { x=y=z=0; w=1; }
    float r[9]={1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w),
                2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w),
                2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)};
    for(int row=0;row<3;row++) {
        float shift=p[UG_P_POS+row]+p[UG_P_PIVOT+row];
        for(int col=0;col<3;col++) {
            float scale=n->obj->type==UG_OBJ_GROUP ? p[UG_P_SIZE+col] : 1;
            m[row*4+col]=r[row*3+col]*scale;
            shift-=r[row*3+col]*p[UG_P_PIVOT+col];
        }
        m[row*4+3]=shift;
    }
}

static void layout(UgScene *s) {
    for (int i = 0; i < s->n; i++) {
        UgNode *n = &s->nodes[i];
        float m[12];
        node_local(n, m);
        UgColor c = UG_RGBA(ug_clamp8(n->props.w[0]), ug_clamp8(n->props.w[1]),
                            ug_clamp8(n->props.w[2]), ug_clamp8(n->props.w[3]));
        int vis = !(n->obj->flags & UG_OF_INVISIBLE);
        if (s->pc_build) {
            if ((n->obj->flags & UG_OF_CONSOLE) && !s->console_strip) vis = 0;
        } else if (n->obj->flags & UG_OF_PC_ONLY) vis = 0;
        if (s->hide_hint_text && n->obj->type == UG_OBJ_TEXT && (n->obj->flags & 0x4u)) vis = 0;
        if (n->app_hidden) vis = 0;
        if (n->parent >= 0) {
            const UgNode *pn = &s->nodes[n->parent];
            for(int row=0;row<3;row++) for(int col=0;col<4;col++) {
                float sum=col==3 ? pn->world[row*4+3] : 0;
                for(int k=0;k<3;k++) sum+=pn->world[row*4+k]*m[k*4+col];
                n->world[row*4+col]=sum;
            }
            n->wcolor = ug_color_mul(pn->wcolor, c);
            vis = vis && pn->visible;
        } else {
            memcpy(n->world,m,sizeof m);
            n->wcolor = c;
        }
        n->wa=n->world[0]; n->wc=n->world[1]; n->we=n->world[3];
        n->wb=n->world[4]; n->wd=n->world[5]; n->wf=n->world[7]; n->wz=n->world[11];
        /* text sitting in the bottom strip band (player name, cash, hint labels) is the strip's
         * content, which the mouse hint bar replaces (SPEC 7.1, 7.6) */
        if (s->hide_hint_text && n->obj->type == UG_OBJ_TEXT && n->wf > 188.0f && n->wf < 226.0f && fabsf(n->we) < 316.0f) vis = 0;
        n->visible = vis;
    }
    /* back to front: larger z first, file order breaks ties (SPEC 1.3) */
    int cnt = 0;
    for (int i = 0; i < s->n; i++) s->order[cnt++] = i;
    for (int i = 1; i < cnt; i++) {      /* stable insertion sort */
        int v = s->order[i], j = i - 1;
        /* equal depth: file order (earlier = behind) when UG_TUNE_TIE_LIST_ORDER, else reverse */
        while (j >= 0 && (s->nodes[s->order[j]].wz < s->nodes[v].wz ||
                          (!UG_TUNE_TIE_LIST_ORDER && s->nodes[s->order[j]].wz == s->nodes[v].wz))) { s->order[j + 1] = s->order[j]; j--; }
        s->order[j + 1] = v;
    }
}

void ug_scene_relayout(UgScene *s) { layout(s); }

void ug_scene_update(UgScene *s, float dt) {
    for (int i = 0; i < s->n; i++) ug_player_update(&s->nodes[i].pl, dt, &s->nodes[i].props);
    layout(s);
}

/* ---- init ---- */
static void init_props(UgProps *p, const UgFngObject *o) {
    memset(p, 0, sizeof *p);
    p->w[0] = o->color.r; p->w[1] = o->color.g; p->w[2] = o->color.b; p->w[3] = o->color.a;
    for (int i = 0; i < 3; i++) { p->w[UG_P_PIVOT + i] = o->pivot[i]; p->w[UG_P_POS + i] = o->pos[i]; p->w[UG_P_SIZE + i] = o->size[i]; }
    for (int i = 0; i < 4; i++) { p->w[UG_P_ROT + i] = o->rot[i]; p->w[UG_P_UV0 + i] = o->uv[i]; }
    if (o->has_corners)
        for (int c = 0; c < 4; c++) {
            p->w[UG_P_CORNERS + 4 * c] = o->corners[c].r; p->w[UG_P_CORNERS + 4 * c + 1] = o->corners[c].g;
            p->w[UG_P_CORNERS + 4 * c + 2] = o->corners[c].b; p->w[UG_P_CORNERS + 4 * c + 3] = o->corners[c].a;
        }
}

int ug_scene_init(UgScene *s, const UgFngPackage *pk, const UgStrTab *strings) {
    memset(s, 0, sizeof *s);
    s->pk = pk;
    s->strings = strings;
    s->pc_build = UG_TUNE_SCENE_PC_BUILD;
    s->hide_hint_text = 0;
    s->focus = s->saved_focus = -1;
    s->n = pk->nobj;
    s->nodes = (UgNode *)calloc((size_t)(s->n ? s->n : 1), sizeof(UgNode));
    s->order = (int *)calloc((size_t)(s->n ? s->n : 1), sizeof(int));
    if (!s->nodes || !s->order) { ug_scene_free(s); return 0; }
    for (int i = 0; i < s->n; i++) {
        UgNode *n = &s->nodes[i];
        const UgFngObject *o = &pk->obj[i];
        n->obj = o; n->scene = s; n->index = i;
        n->parent = -1;
        if (o->has_parent) {
            for (int k = 0; k < i; k++)
                if (pk->obj[k].guid == o->parent_guid && pk->obj[k].type == UG_OBJ_GROUP) { n->parent = k; break; }
        }
        init_props(&n->props, o);
        ug_player_init(&n->pl, o->scripts, o->nscripts, emit_cb, n);
    }
    layout(s);
    /* initial scripts (SPEC 3.5) then the first button receives focus */
    for (int i = 0; i < s->n; i++) {
        UgNode *n = &s->nodes[i];
        ug_player_start(&n->pl, UG_SCRIPT_INITIAL, &n->props);
    }
    layout(s);
    for (int i = 0; i < s->n; i++)
        if (s->nodes[i].obj->flags & UG_OF_BUTTON) { ug_scene_set_focus(s, i); break; }
    return 1;
}

void ug_scene_free(UgScene *s) {
    free(s->nodes);
    free(s->order);
    memset(s, 0, sizeof *s);
}

void ug_scene_set_focus(UgScene *s, int node) {
    if (node == s->focus || node < -1 || node >= s->n) return;
    int old = s->focus;
    s->focus = node;
    if (old >= 0) deliver(s, old, UG_MSG_FOCUS_LOST);
    if (node >= 0) deliver(s, node, UG_MSG_FOCUS_GAINED);
}

void ug_scene_set_world_pos(UgScene *s, int node, float x, float y) {
    if (node < 0 || node >= s->n) return;
    UgNode *n = &s->nodes[node];
    float lx = x, ly = y;
    if (n->parent >= 0) {
        const UgNode *p = &s->nodes[n->parent];
        float det = p->wa * p->wd - p->wb * p->wc;
        float dx = x - p->we, dy = y - p->wf;
        if (fabsf(det) > 1e-6f) {
            lx = (p->wd * dx - p->wc * dy) / det;
            ly = (-p->wb * dx + p->wa * dy) / det;
        }
    }
    n->props.w[UG_P_POS] = lx;
    n->props.w[UG_P_POS + 1] = ly;
}

void ug_scene_refocus(UgScene *s) {
    if (s->focus >= 0) deliver(s, s->focus, UG_MSG_FOCUS_GAINED);
}

/* ---- picking / navigation ---- */
static UgRect node_rect(const UgNode *n) {
    float w = fabsf(n->props.w[UG_P_SIZE]), h = fabsf(n->props.w[UG_P_SIZE + 1]);
    if (n->obj->type != UG_OBJ_IMAGE && n->obj->type != UG_OBJ_GRADIENT && n->obj->type != UG_OBJ_MOVIE) {
        w = 40.0f * n->props.w[UG_P_SIZE]; h = 26.0f * n->props.w[UG_P_SIZE + 1];
    }
    float sa = sqrtf(n->wa * n->wa + n->wb * n->wb), sd = sqrtf(n->wc * n->wc + n->wd * n->wd);
    return UG_RECT_C(n->we, n->wf, w * sa, h * sd);
}

int ug_scene_pick(const UgScene *s, float x, float y) {
    for (int k = s->n - 1; k >= 0; k--) {          /* nearest to the viewer first */
        const UgNode *n = &s->nodes[s->order[k]];
        if (!n->visible || !(n->obj->flags & UG_OF_BUTTON) || n->wcolor.a == 0) continue;
        if (ug_rect_hit(node_rect(n), x, y)) return n->index;
    }
    return -1;
}

static int nav(UgScene *s, UgAction dir) {
    if (s->focus < 0) return 0;
    UgRect cr = node_rect(&s->nodes[s->focus]);
    float cx = cr.x + cr.w * 0.5f, cy = cr.y + cr.h * 0.5f;
    int best = -1;
    float bs = 1e30f;
    for (int i = 0; i < s->n; i++) {
        const UgNode *n = &s->nodes[i];
        if (i == s->focus || !n->visible || !(n->obj->flags & UG_OF_BUTTON) ||
            (n->obj->flags & UG_OF_NO_DIRFOCUS) || n->wcolor.a == 0) continue;
        UgRect r = node_rect(n);
        float dx = r.x + r.w * 0.5f - cx, dy = r.y + r.h * 0.5f - cy, fwd, lat;
        switch (dir) {
        case UG_A_UP: fwd = -dy; lat = fabsf(dx); break;
        case UG_A_DOWN: fwd = dy; lat = fabsf(dx); break;
        case UG_A_LEFT: fwd = -dx; lat = fabsf(dy); break;
        case UG_A_RIGHT: fwd = dx; lat = fabsf(dy); break;
        default: return 0;
        }
        if (fwd <= 1 || lat > fwd * UG_TUNE_NAV_CONE) continue;
        float sc = fwd + UG_TUNE_NAV_LATERAL_WEIGHT * lat;
        if (sc < bs) { bs = sc; best = i; }
    }
    if (best < 0) return 0;
    ug_scene_set_focus(s, best);
    return 1;
}

int ug_scene_input(UgScene *s, UgAction a) {
    uint32_t msg = ug_action_message(a);
    int handled = msg ? ug_scene_input_msg(s, msg) : 0;
    if (!handled && (a == UG_A_UP || a == UG_A_DOWN || a == UG_A_LEFT || a == UG_A_RIGHT))
        handled = nav(s, a);
    return handled;
}

void ug_scene_pointer(UgScene *s, const UgPtr *p) {
    if (p->type == UG_PT_MOVE) {
        int h = UG_TUNE_HOVER_FOCUS ? ug_scene_pick(s, p->x, p->y) : -1;
        if (h >= 0) ug_scene_set_focus(s, h);
    } else if (p->type == UG_PT_DOWN && p->button == 1) {
        int h = ug_scene_pick(s, p->x, p->y);
        if (h >= 0) { ug_scene_set_focus(s, h); ug_scene_input(s, UG_A_ACCEPT); }
    } else if (p->type == UG_PT_DOWN && p->button == 2) {
        ug_scene_input_msg(s, UG_MSG_POINTER2);
    } else if (p->type == UG_PT_WHEEL && UG_TUNE_WHEEL_AS_UPDOWN) {
        ug_scene_input(s, p->wheel > 0 ? UG_A_UP : UG_A_DOWN);
    }
}

/* ---- assets ---- */
static const UgFngResource *res_of(const UgScene *s, const UgFngObject *o) {
    if (o->res_index < 0 || o->res_index >= s->pk->nres) return NULL;
    return &s->pk->res[o->res_index];
}

void ug_scene_set_assets(UgScene *s, const UgAssets *as) {
    s->assets = as;
    for (int i = 0; i < s->n; i++) {
        UgNode *n = &s->nodes[i];
        const UgFngObject *o = n->obj;
        const UgFngResource *r = res_of(s, o);
        n->tex = 0; n->untextured = 0; n->face = NULL;
        if (o->type == UG_OBJ_IMAGE || o->type == UG_OBJ_SIMPLEIMAGE) {
            if (!r || r->kind == 0 || !strncmp(r->name, "(Untextured", 11)) n->untextured = 1;
            else if (as) {
                int ti = ug_assets_texture_for(as, r->name);
                n->tex = ti >= 0 ? ti + 1 : 0;
            }
        } else if (o->type == UG_OBJ_TEXT && as) {
            if (r && r->kind == 2) n->face = ug_assets_face(as, r->name);
            else n->face = ug_assets_main_face(as);
        }
    }
}

int ug_scene_has_bottom_strip(const UgScene *s) {
    for (int i = 0; i < s->n; i++) {
        const UgNode *n = &s->nodes[i];
        if (!n->visible || n->obj->type != UG_OBJ_IMAGE) continue;
        float w = fabsf(n->props.w[UG_P_SIZE]) * sqrtf(n->wa * n->wa + n->wb * n->wb);
        if (w >= 100.0f && n->wf > 190.0f && n->wf < 225.0f) return 1;
    }
    return 0;
}

int ug_scene_missing_textures(const UgScene *s) {
    int m = 0;
    for (int i = 0; i < s->n; i++) {
        const UgNode *n = &s->nodes[i];
        if (n->visible && (n->obj->type == UG_OBJ_IMAGE || n->obj->type == UG_OBJ_SIMPLEIMAGE) &&
            !n->untextured && n->tex == 0) m++;
    }
    return m;
}

/* ---- drawing ---- */
static const char *res_name(const UgScene *s, const UgFngObject *o) {
    const UgFngResource *r = res_of(s, o);
    return r ? r->name : "";
}

void ug_scene_draw(UgScene *s, UgCtx *c, UgCanvas *cv) {
    const UgFace *saved_face = ug_text_face();
    int have_saved = 1;
    for (int k = 0; k < s->n; k++) {
        const UgNode *n = &s->nodes[s->order[k]];
        if (!n->visible || n->wcolor.a == 0) continue;
        const UgFngObject *o = n->obj;
        ug_cv_push(cv);
        /* install the node's world transform on the canvas */
        UgXf *m = &cv->st[cv->sp];
        m->a = n->wa; m->b = n->wb; m->c = n->wc; m->d = n->wd; m->e = n->we; m->f = n->wf;
        float w = n->props.w[UG_P_SIZE], h = n->props.w[UG_P_SIZE + 1];
        UgColor col = n->wcolor;
        switch (o->type) {
        case UG_OBJ_IMAGE: case UG_OBJ_SIMPLEIMAGE: {
            UgRect r = UG_RECT_C(0, 0, w, h);
            int tex = n->tex_override > 0 ? n->tex_override : n->tex;
            if (tex > 0) {
                float u0 = n->props.w[UG_P_UV0], v0 = n->props.w[UG_P_UV0 + 1];
                float u1 = n->props.w[UG_P_UV1], v1 = n->props.w[UG_P_UV1 + 1];
                ug_cv_image(cv, r.x, r.y, r.x + r.w, r.y + r.h, u0, v0, u1, v1, tex, col);
            } else if (n->untextured) {
                ug_cv_flat_rect(cv, r, col); /* explicitly authored plain quad only */
            } else cv->visual_status=UG_VIS_UNAVAILABLE;
            break;
        }
        case UG_OBJ_GRADIENT: {
            UgRect r = UG_RECT_C(0, 0, w, h);
            UgColor cc[4];
            for (int q = 0; q < 4; q++)
                cc[q] = ug_color_mul(o->corners[q], UG_RGBA(255, 255, 255, col.a));
            ug_cv_gradient(cv, r, cc[0], cc[1], cc[2], cc[3]);
            break;
        }
        case UG_OBJ_MOVIE:
            ug_movie_draw(cv, c, UG_RECT_C(0, 0, fabsf(w) > 1 ? fabsf(w) : 160, fabsf(h) > 1 ? fabsf(h) : 90), "MOVIE");
            break;
        case UG_OBJ_TEXT: {
            const char *txt = o->text ? o->text : "";
            if (n->text_override) txt = n->text_override;
            else if (o->has_label_hash) {
                const UgStrTab *tab = s->strings ? s->strings : (s->assets && s->assets->have_strings ? &s->assets->strings : NULL);
                if (tab) txt = ug_strtab_or(tab, o->label_hash, txt);
            }
            ug_text_set_face(n->face);
            ug_text_draw(cv, txt, 0, 0, w, col, o->justify, (float)o->max_width, (float)o->leading, c->glyphs);
            break;
        }
        default: break;
        }
        ug_cv_pop(cv);
    }
    if (have_saved) ug_text_set_face(saved_face);
}
