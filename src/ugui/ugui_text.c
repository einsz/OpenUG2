/* ugui_text.c -- text layout (SPEC 8.2, 8.4, 7.3, 16.5) and drawing. */
#include "ugui_canvas.h"
#include "ugui_tuning.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ITALIC_SHEAR 0.20f      /* built-in face only: the original atlases are already italic */

static const UgFace *g_face;

void ug_text_set_face(const UgFace *f) { g_face = f; }
const UgFace *ug_text_face(void) { return g_face ? g_face : ug_face_builtin(); }

const UgFace *ug_face_builtin(void) {
    static UgFace f;
    if (!f.procedural) {
        memset(&f, 0, sizeof f);
        snprintf(f.name, sizeof f.name, "builtin");
        f.line_height = (int)UG_LINE_PITCH;
        f.cell_height = (int)UG_LINE_PITCH;
        f.baseline = (int)UG_BASELINE;
        f.cap_height = (int)UG_CAP_HEIGHT;
        f.procedural = 1;
    }
    return &f;
}

/* Missing atlas never selects a substitute face. */
static const UgFace *draw_face(void) { return g_face && g_face->tex>0 && !g_face->procedural ? g_face : NULL; }

static int add_item(UgTextLayout *tl, const UgTextItem *it) {
    if (tl->n == tl->cap) {
        int nc = tl->cap ? tl->cap * 2 : 64;
        UgTextItem *ni = (UgTextItem *)realloc(tl->items, sizeof(UgTextItem) * (size_t)nc);
        if (!ni) return 0;
        tl->items = ni;
        tl->cap = nc;
    }
    tl->items[tl->n++] = *it;
    return 1;
}

static int add_line(UgTextLayout *tl, int first, int count) {
    UgTextLine *nl = (UgTextLine *)realloc(tl->lines, sizeof(UgTextLine) * (size_t)(tl->nlines + 1));
    if (!nl) return 0;
    tl->lines = nl;
    nl[tl->nlines].first = first;
    nl[tl->nlines].count = count;
    nl[tl->nlines].width = 0;
    tl->nlines++;
    return 1;
}

void ug_text_layout_free(UgTextLayout *tl) {
    free(tl->items);
    free(tl->lines);
    memset(tl, 0, sizeof *tl);
}

/* try to read "$NAME$" at s; returns the number of bytes consumed or 0 */
static int read_token(const char *s, char *name, size_t cap) {
    if (*s != '$') return 0;
    size_t n = 0;
    for (const char *p = s + 1; *p && n < cap - 1; p++, n++) {
        if (*p == '$') {
            if (n == 0) return 0;
            name[n] = 0;
            return (int)n + 2;
        }
        char ch = *p;
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_'))
            return 0;
        name[n] = ch;
    }
    return 0;
}

static int is_space(const UgTextItem *it) { return !it->is_glyph && it->cp == ' '; }

/* Layout one paragraph (items [p0,p1)) into lines with greedy word wrap.
 * A word longer than the wrap width stays on its own line (SPEC 16.5, G). */
static int wrap_paragraph(UgTextLayout *tl, int p0, int p1, float wrap_w) {
    UgTextItem *it = tl->items;
    if (p0 == p1) return add_line(tl, p0, 0);
    int start = p0;
    while (start < p1) {
        float w = 0;
        int last_space = -1, end = p1, next = p1, overflow = 0, wrapped = 0;
        for (int i = start; i < p1; i++) {
            float iw = it[i].w + (i > start ? it[i].kern : 0);
            if (is_space(&it[i])) {
                if (overflow) { end = i; next = i + 1; wrapped = 1; break; }
                last_space = i;
            } else if (wrap_w > 0 && w + iw > wrap_w && i > start) {
                if (last_space > start) { end = last_space; next = last_space + 1; wrapped = 1; break; }
                overflow = 1;
            }
            w += iw;
        }
        int e = end;
        while (e > start && is_space(&it[e - 1])) e--;
        if (!add_line(tl, start, e - start)) return 0;
        if (wrapped) while (next < p1 && is_space(&it[next])) next++;
        start = next;
    }
    return 1;
}

void ug_text_layout(UgTextLayout *tl, const char *s, float wrap_w, float leading,
                    UgGlyphMode mode) {
    const UgFace *face = ug_text_face();
    const UgFont *font = ug_font();
    float glyph_cell = UG_CAP_HEIGHT * UG_TUNE_GLYPH_CELL_PER_CAP;
    memset(tl, 0, sizeof *tl);
    /* 16.5 (amendment 3): line pitch = (cell height + leading) x scale, not the line-height byte */
    tl->pitch = (float)face->cell_height + leading;
    if (!s) s = "";
    int para_start = 0;
    uint32_t prev = 0;
    while (1) {
        char name[64];
        int end = 0;
        if (*s == 0) { end = 1; }
        else if (*s == '\n' || *s == '^') { s++; }
        else if (s[0] == '\\' && s[1] == 'n') { s += 2; }
        else {
            int used = read_token(s, name, sizeof name);
            UgGlyph g;
            if (used) {
                const char *suffix = strncmp(name, "JOY_EVENT_FENG_", 15) ? name : name + 15;
                if (ug_glyph_for_token(suffix, mode, &g)) {
                    UgTextItem it;
                    memset(&it, 0, sizeof it);
                    it.is_glyph = 1; it.glyph = g; it.w = glyph_cell;
                    add_item(tl, &it);
                    prev = 0;
                    s += used;
                    continue;
                }
            }
            uint32_t cp = ug_utf8_next(&s);
            UgTextItem it;
            memset(&it, 0, sizeof it);
            it.cp = cp;
            if (face->procedural) {
                it.w = ug_font_adv(font, cp);
            } else {
                const UgFGlyph *g2 = ug_face_glyph(face, cp);
                if (g2) {
                    it.w = (float)g2->adv;
                    if (prev) it.kern = (float)ug_face_kern(face, g2, prev);
                } else {
                    it.missing = 1;       /* skipped, kerning resets (16.5 step 1) */
                }
            }
            prev = it.missing ? 0 : cp;
            add_item(tl, &it);
            continue;
        }
        prev = 0;
        if (!wrap_paragraph(tl, para_start, tl->n, wrap_w)) break;
        para_start = tl->n;
        if (end) break;
    }
    /* pen positions and widths */
    float maxw = 0;
    for (int l = 0; l < tl->nlines; l++) {
        UgTextLine *ln = &tl->lines[l];
        float x = 0;
        for (int i = 0; i < ln->count; i++) {
            UgTextItem *it = &tl->items[ln->first + i];
            if (i == 0) it->kern = 0;
            x += it->kern;
            it->x = x;
            x += it->w;
        }
        ln->width = x;
        if (x > maxw) maxw = x;
    }
    tl->width = maxw;
    tl->height = tl->nlines > 0 ? (float)(tl->nlines - 1) * tl->pitch + (float)face->cell_height : 0;
}

void ug_text_measure(const char *s, float scale, float wrap_w, float leading,
                     UgGlyphMode mode, float *w, float *h) {
    UgTextLayout tl;
    ug_text_layout(&tl, s, wrap_w, leading, mode);
    if (w) *w = tl.width * scale;
    if (h) *h = tl.height * scale;
    ug_text_layout_free(&tl);
}

void ug_text_draw(UgCanvas *cv, const char *s, float x, float y, float scale,
                  UgColor c, unsigned just, float wrap_w, float leading,
                  UgGlyphMode mode) {
    if (!s || c.a == 0) return;
    const UgFace *saved = g_face;
    const UgFace *face = draw_face();
    if (!face) {cv->visual_status=UG_VIS_UNAVAILABLE;return;}
    g_face = face;                         /* layout and draw must use the same face */
    UgTextLayout tl;
    ug_text_layout(&tl, s, wrap_w, leading, mode);
    g_face = saved;

    float top = y;
    if (just & UG_J_BOTTOM) top = y - tl.height * scale;
    else if (just & UG_J_VCENTER) top = y - tl.height * scale * 0.5f;

    for (int l = 0; l < tl.nlines; l++) {
        const UgTextLine *ln = &tl.lines[l];
        float lw = ln->width * scale;
        float lx = x;
        if (just & UG_J_CENTER) lx = x - lw * 0.5f;
        else if (just & UG_J_RIGHT) lx = x - lw;
        float ltop = top + (float)l * tl.pitch * scale;
        float base = ltop + (float)face->baseline * scale;

        for (int i = 0; i < ln->count; i++) {
            const UgTextItem *it = &tl.items[ln->first + i];
            float px = lx + it->x * scale;
            if (it->is_glyph) {
                if (it->glyph.id != UG_G_SPACER)
                    ug_glyph_draw(cv, it->glyph, px + it->w * scale * 0.5f,
                                  base - (float)face->cap_height * 0.5f * scale, it->w * scale, c);
                continue;
            }
            if (it->missing || it->cp == ' ') continue;
            {
                const UgFGlyph *g = ug_face_glyph(face, it->cp);
                if (!g || !g->w || !g->h || face->atlas_w <= 0 || face->atlas_h <= 0) continue;
                float aw = (float)face->atlas_w, ah = (float)face->atlas_h;
                float x0 = px + (float)g->xoff * scale, y0 = ltop + (float)g->yoff * scale;
                ug_cv_image(cv, x0, y0, x0 + (float)g->w * scale, y0 + (float)g->h * scale,
                            (float)g->x / aw, (float)g->y / ah,
                            (float)(g->x + g->w) / aw, (float)(g->y + g->h) / ah, face->tex, c);
            }
        }
    }
    ug_text_layout_free(&tl);
}

/* ---- quad listing used by tests and tools (SPEC 16.5, top-left anchor) ---- */
void ug_text_quads(const UgFace *face, const char *s, float scale, UgQuadFn fn, void *user,
                   float *last_pen) {
    const UgFace *saved = g_face;
    g_face = face;
    UgTextLayout tl;
    ug_text_layout(&tl, s, 0, 0, UG_GLYPHS_MOUSE);
    g_face = saved;
    for (int l = 0; l < tl.nlines; l++) {
        const UgTextLine *ln = &tl.lines[l];
        float ltop = (float)l * tl.pitch * scale;
        for (int i = 0; i < ln->count; i++) {
            const UgTextItem *it = &tl.items[ln->first + i];
            if (it->is_glyph || it->missing) continue;
            const UgFGlyph *g = ug_face_glyph(face, it->cp);
            if (!g || !g->w || !g->h) continue;
            UgQuad q;
            q.cp = it->cp;
            q.x0 = (it->x + (float)g->xoff) * scale;
            q.y0 = ltop + (float)g->yoff * scale;
            q.x1 = q.x0 + (float)g->w * scale;
            q.y1 = q.y0 + (float)g->h * scale;
            q.u0 = (float)g->x / (float)face->atlas_w;
            q.v0 = (float)g->y / (float)face->atlas_h;
            q.u1 = (float)(g->x + g->w) / (float)face->atlas_w;
            q.v1 = (float)(g->y + g->h) / (float)face->atlas_h;
            fn(user, &q);
        }
    }
    if (last_pen) *last_pen = tl.nlines ? tl.lines[tl.nlines - 1].width : 0;
    ug_text_layout_free(&tl);
}

/* DEMO_DESIGN D2/D3: enabled labels retain filled ink from the original
 * raster alpha, independent of atlas RGB decorations. Geometry/UVs are unchanged. */
void ug_text_draw_ink(UgCanvas *cv,const char *s,float x,float y,float scale,UgColor c,unsigned just,UgGlyphMode mode) {
    int first=cv->n;
    ug_text_draw(cv,s,x,y,scale,c,just,0,0,mode);
    const UgFace *f=draw_face();
    if(f)for(int i=first;i<cv->n;i++)if(cv->v[i].tex==f->tex)cv->v[i].mode=UG_M_TEXT;
}
