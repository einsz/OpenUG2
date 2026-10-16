/* tire_marks_render_test -- GL regression for the tire-mark pass (tire-marks SPEC TM-15).
 *
 * Draws mark quads through the production mark pass (render_mark_begin: glass pipeline +
 * ug2 shader, uVista = 2) into a hidden 32x32 window and reads the pixels back. The expected
 * values are the reference law of src/tire_marks.c (tm_mark_*), which tire_marks_test checks
 * against test-vectors/render_state.json; composites additionally compare with the vector
 * rows. Needs an OpenGL context, no game data. GL output is 8-bit, so colour tolerance is
 * +-3/255 (render_state README: +-1/255 per rasterised layer, 4 layers).
 *
 *   tire_marks_render_test [VECTOR_DIR]
 */
#include "render.h"
#include "rhi_gl.h"
#include "tire_marks.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *g_dir = "docs/cleanroom-driving-physics/tire-marks/test-vectors";
static int g_fail, g_checks;
#include "test_json.h"
#define CHECK(cond, ...) do { g_checks++; if (!(cond)) { g_fail++; \
    printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static RProg g_r;
static GpuMesh g_mesh;
static unsigned char g_cols[16];

/* Unit quad v0 (0,0) v1 (1,0) v2 (1,1) v3 (0,1) at depth z = 0 (NDC depth 0.5), TM-11.1 UVs. */
static void set_alpha(int a) {
    for (int i = 0; i < 4; i++) { g_cols[i * 4] = g_cols[i * 4 + 1] = g_cols[i * 4 + 2] = TM_VERTEX_RGB; g_cols[i * 4 + 3] = (unsigned char)a; }
    rhi_buffer_replace(g_mesh.cbo, sizeof g_cols, g_cols);
}
static void mesh_init(void) {
    static const float vb[] = { 0,0,0,0,0, 1,0,0,1,0, 1,1,0,1,1, 0,1,0,0,1 };
    static const float nb[] = { 0,0,1, 0,0,1, 0,0,1, 0,0,1 };
    static const uint16_t ib[] = { 0,1,2, 0,2,3 };
    memset(&g_mesh, 0, sizeof g_mesh);
    g_mesh.vbo = rhi_buffer_create(&(RhiBufferDesc){ RHI_BUFFER_VERTEX, RHI_UPDATE_STATIC, sizeof vb, vb });
    g_mesh.nbo = rhi_buffer_create(&(RhiBufferDesc){ RHI_BUFFER_VERTEX, RHI_UPDATE_STATIC, sizeof nb, nb });
    g_mesh.cbo = rhi_buffer_create(&(RhiBufferDesc){ RHI_BUFFER_VERTEX, RHI_UPDATE_STREAM, 0, NULL });
    g_mesh.ibo = rhi_buffer_create(&(RhiBufferDesc){ RHI_BUFFER_INDEX, RHI_UPDATE_STATIC, sizeof ib, ib });
    set_alpha(127);
}
/* Flat 64x32 texture like the mark textures: P8-style RGB plane + separate alpha plane. */
static RhiTexture make_tex(int r, int g, int b, int a) {
    static unsigned char rgb[64 * 32 * 3], al[64 * 32];
    for (int i = 0; i < 64 * 32; i++) { rgb[i * 3] = (unsigned char)r; rgb[i * 3 + 1] = (unsigned char)g; rgb[i * 3 + 2] = (unsigned char)b; al[i] = (unsigned char)a; }
    N2Tex t; memset(&t, 0, sizeof t); t.w = 64; t.h = 32; t.rgb = rgb; t.alpha = al; t.afmt = 8;
    return upload_tex(&t);
}
static void clear_to(float grey) {
    glDepthMask(GL_TRUE); glClearDepth(1); glClearColor(grey, grey, grey, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
static void draw_mark(RhiTexture tex, const float mvp[16]) {
    RMarkSaved saved;
    render_mark_begin(&g_r, &saved);
    ug2_copy(g_r.p->uMVP, mvp, 16);
    rhi_cmd_bind_vertex_buffer(0, g_mesh.vbo, 0); rhi_cmd_bind_vertex_buffer(1, g_mesh.nbo, 0);
    rhi_cmd_bind_vertex_buffer(2, g_mesh.cbo, 0); rhi_cmd_bind_index_buffer(g_mesh.ibo);
    rhi_cmd_bind_texture(0, tex);
    rhi_cmd_draw_indexed(6, 0);
    render_mark_end(&g_r, &saved);
}
static void pixel(int x, int y, int out[3]) {
    unsigned char p[4]; glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p);
    out[0] = p[0]; out[1] = p[1]; out[2] = p[2];
}
static int near_byte(int got, double want01, int tol) { return abs(got - (int)floor(want01 * 255.0 + 0.5)) <= tol; }

static const float MVP[16] = { 2,0,0,0, 0,2,0,0, 0,0,1,0, -1,-1,0,1 };
static const float MVP_MIRROR[16] = { -2,0,0,0, 0,2,0,0, 0,0,1,0, 1,-1,0,1 };   /* reverses the winding */

int main(int argc, char **argv) {
    if (argc > 1 && argv[1][0]) g_dir = argv[1];
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { printf("tire_marks_render_test: no video (%s)\n", SDL_GetError()); return 2; }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_Window *w = SDL_CreateWindow("tire-marks-render-test", 0, 0, 32, 32, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!w) { printf("tire_marks_render_test: no window\n"); return 2; }
    SDL_GLContext ctx = SDL_GL_CreateContext(w);
    if (!ctx) { printf("tire_marks_render_test: no GL context\n"); return 2; }
    g_r = render_program(); glUseProgram(g_r.prog);
    mesh_init();
    g_r.p->uFogDensity = 0; ug2_vec3(g_r.p->uFogColor, 1, 0, 0);
    ug2_vec3(g_r.p->uCamPos, 0, 0, 3);
    g_r.p->uAmbient = 0.2f; g_r.p->uDiffuse = 0.9f;   /* lighting must not matter */
    ug2_vec3(g_r.p->uColor, 0, 1, 0);                 /* tint must not matter */
    g_r.p->uAlpha = 0.3f;                             /* extra alpha scale must not matter */
    glViewport(0, 0, 32, 32);

    /* ---- composite vectors through the production pipeline ------------------------ */
    J *root = jload("render_state.json");
    CHECK(root != NULL, "render_state.json missing");
    const J *cp = root ? jget(root, "composite") : NULL;
    for (int i = 0; cp && i < cp->n; i++) {
        const J *c = cp->kid[i], *tr = jget(c, "texel_rgb"), *res = jget(c, "result_rgb");
        int tex_rgb[3]; for (int j = 0; j < 3; j++) tex_rgb[j] = (int)floor(tr->kid[j]->num + 0.5);
        RhiTexture tex = make_tex(tex_rgb[0], tex_rgb[1], tex_rgb[2], (int)jnum(c, "texel_alpha", 0));
        set_alpha(tm_vertex_alpha((int)jnum(c, "Ib", 0), 0));
        clear_to(0.5f);
        for (int l = 0; l < (int)jnum(c, "layers", 1); l++) draw_mark(tex, MVP);
        int got[3]; pixel(16, 16, got);
        for (int j = 0; j < 3; j++)
            CHECK(near_byte(got[j], res->kid[j]->num, 3), "composite '%s' ch%d: got %d want %.1f", jstrv(c, "name"), j,
                  got[j], res->kid[j]->num * 255.0);
        /* the same pixel from the reference law with the actual 8-bit texel */
        float t[3], src[3], dst[3] = { 0.5f, 0.5f, 0.5f }, out[3]; const unsigned char vc[3] = { 128, 128, 128 };
        for (int j = 0; j < 3; j++) t[j] = tex_rgb[j] / 255.0f;
        tm_mark_source(t, vc, src);
        float a = tm_mark_opacity((int)jnum(c, "texel_alpha", 0) / 255.0f, tm_vertex_alpha((int)jnum(c, "Ib", 0), 0));
        for (int l = 0; l < (int)jnum(c, "layers", 1); l++) { tm_mark_blend(dst, src, a, out); memcpy(dst, out, sizeof dst); }
        for (int j = 0; j < 3; j++) CHECK(near_byte(got[j], dst[j], 2), "composite '%s' reference ch%d: got %d want %.2f",
                                          jstrv(c, "name"), j, got[j], dst[j] * 255.0);
        rhi_texture_destroy(tex);
    }
    jfree(root);

    /* ---- effective alpha: vertex alpha 127 is 0.498, not doubled (TM-15.2) ----------- */
    { RhiTexture tex = make_tex(255, 255, 255, 255);   /* white, opaque texel: result = a over dst 0 */
      set_alpha(127); clear_to(0); draw_mark(tex, MVP);
      int g[3]; pixel(16, 16, g);
      CHECK(near_byte(g[0], 127.0 / 255.0, 2) && near_byte(g[1], 127.0 / 255.0, 2) && near_byte(g[2], 127.0 / 255.0, 2),
            "white mark over black = 127/255 (got %d,%d,%d)", g[0], g[1], g[2]);
      /* colour doubled with the 128 vertex byte is unity, and clamps at 1: grey 200 stays 200*256/255 */
      rhi_texture_destroy(tex); tex = make_tex(200, 100, 255, 255);
      set_alpha(127); clear_to(0); draw_mark(tex, MVP); pixel(16, 16, g);
      CHECK(near_byte(g[0], 127.0 / 255.0 * 200.0 / 255.0 * 256.0 / 255.0, 2) && near_byte(g[1], 127.0 / 255.0 * 100.0 / 255.0 * 256.0 / 255.0, 2) &&
            near_byte(g[2], 127.0 / 255.0, 2), "colour x256/255 (got %d,%d,%d)", g[0], g[1], g[2]);
      /* texture alpha is used as is: texel alpha 127 with vertex alpha 127 */
      rhi_texture_destroy(tex); tex = make_tex(255, 255, 255, 127);
      set_alpha(127); clear_to(0); draw_mark(tex, MVP); pixel(16, 16, g);
      CHECK(near_byte(g[0], 127.0 / 255.0 * 127.0 / 255.0, 2), "texel alpha x vertex alpha = 0.248 (got %d)", g[0]);
      /* no alpha test / discard: a = 40/255*12/255 = 0.0074 still lands (about 2 of 255) */
      rhi_texture_destroy(tex); tex = make_tex(255, 255, 255, 40);
      set_alpha(12); clear_to(0); draw_mark(tex, MVP); pixel(16, 16, g);
      CHECK(g[0] >= 1 && g[0] <= 3, "faint pixel not discarded (got %d)", g[0]);
      /* zero vertex alpha (puddle) draws nothing */
      set_alpha(0); clear_to(0.5f); draw_mark(tex, MVP); pixel(16, 16, g);
      CHECK(near_byte(g[0], 0.5, 1), "alpha 0 leaves the background (got %d)", g[0]);

      /* lighting, tint, uAlpha did not matter above (set non-neutral); fog affects colour only */
      rhi_texture_destroy(tex); tex = make_tex(255, 255, 255, 255);
      g_r.p->uFogDensity = 10.0f;   /* exp(-(w*10)^2): fully fogged */
      set_alpha(127); clear_to(0); draw_mark(tex, MVP); pixel(16, 16, g);
      CHECK(near_byte(g[0], 127.0 / 255.0, 2) && g[1] <= 1 && g[2] <= 1, "fog replaces colour (fog red), alpha unchanged (got %d,%d,%d)", g[0], g[1], g[2]);
      g_r.p->uFogDensity = 0;

      /* both windings are drawn (no culling) */
      set_alpha(127); clear_to(0); draw_mark(tex, MVP_MIRROR); pixel(16, 16, g);
      CHECK(near_byte(g[0], 127.0 / 255.0, 2), "reversed winding drawn (got %d)", g[0]);

      /* depth: test passes at <= (the same depth twice), write stays off */
      float z; set_alpha(127); clear_to(0); draw_mark(tex, MVP);
      glReadPixels(16, 16, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);
      CHECK(z > 0.999f, "depth not written (z %.3f)", z);
      glDepthMask(GL_TRUE); glClear(GL_DEPTH_BUFFER_BIT); glClearDepth(1);
      { /* an opaque occluder at the same depth as the mark: LEQUAL lets the mark through */
        float mvp_eq[16]; memcpy(mvp_eq, MVP, sizeof mvp_eq);
        glClearDepth(0.5); glClear(GL_DEPTH_BUFFER_BIT); glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT); glClearDepth(1);
        draw_mark(tex, mvp_eq); pixel(16, 16, g);
        CHECK(near_byte(g[0], 127.0 / 255.0, 2), "equal depth passes (<=) (got %d)", g[0]);
        float farther[16]; memcpy(farther, MVP, sizeof farther); farther[14] = 0.5f;   /* mark depth 0.75 vs stored 0.25 */
        glClearDepth(0.25); glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT); glClearDepth(1);
        draw_mark(tex, farther); pixel(16, 16, g);
        CHECK(g[0] <= 1, "farther than stored depth is rejected (got %d)", g[0]);
      }
      rhi_texture_destroy(tex); }

    /* ---- texture sampler of the mark textures: wrap, linear mips (TM-15.3) ----------- */
    { RhiTexture tex = make_tex(8, 8, 8, 127);
      const RhiSamplerDesc *d = rhi_sampler_desc(rhi_texture_sampler(tex));
      CHECK(d && d->wrap_u == RHI_WRAP_REPEAT && d->wrap_v == RHI_WRAP_REPEAT, "wrap addressing in U and V");
      CHECK(d && d->mip_filter == RHI_MIP_LINEAR && d->min_filter == RHI_FILTER_LINEAR, "linear filtering with linear mips");
      rhi_texture_destroy(tex); }

    /* ---- state restored by render_mark_end ------------------------------------------- */
    { float before[5] = { g_r.p->uUnlit, g_r.p->uUseTex, g_r.p->uAlpha, g_r.p->uVColor, g_r.p->uVista };
      RMarkSaved s; render_mark_begin(&g_r, &s);
      CHECK(g_r.p->uVista == 2.0f && g_r.p->uUseTex == 1.0f && g_r.p->uAlpha == 1.0f && g_r.p->uVColor == 0.0f &&
            g_r.p->uUnlit == 0.0f, "mark pass shader state");
      render_mark_end(&g_r, &s);
      float after[5] = { g_r.p->uUnlit, g_r.p->uUseTex, g_r.p->uAlpha, g_r.p->uVColor, g_r.p->uVista };
      CHECK(!memcmp(before, after, sizeof before), "state restored"); }

    /* ---- the three mark textures as the host decodes them (TM-15.5), when game data is present ---- */
    { const char *root_dir = getenv("TM_DATA_DIR");
      char path[1024]; snprintf(path, sizeof path, "%s/GLOBAL/InGameCommon.bun",
                                root_dir && *root_dir ? root_dir : "/home/dq/Games/Need for Speed Underground 2");
      long n = 0; FILE *f = fopen(path, "rb"); unsigned char *d = NULL;
      if (f) { fseek(f, 0, SEEK_END); n = ftell(f); rewind(f); d = malloc((size_t)n); if (d && fread(d, 1, (size_t)n, f) != (size_t)n) { free(d); d = NULL; } fclose(f); }
      if (!d) printf("tire_marks_render_test: no game data (%s), texture statistics skipped\n", path);
      else {
          N2Tpk tpk = n2_tpk_open(d, n);
          for (int b = 0; b < tpk.nblk; b++) tpk.blk[b].dbase = tm_payload_base(d, tpk.blk[b].dbase);
          const double want_max[3] = { 127, 102, 102 }, want_mean[3] = { 114.16, 90.22, 90.22 };
          for (int t = TM_TEX_ROAD; t < TM_TEX_COUNT; t++) {
              N2Tex tx; int ok = n2_tpk_decode(d, n, tpk, tm_texture_hash(t), &tx);
              CHECK(ok && tx.w == 64 && tx.h == 32 && tx.alpha && tx.usage == 2 && tx.blend == 1, "mark texture %d decodes (64x32, usage 2, blend 1)", t);
              if (ok && tx.alpha) {
                  int mx = 0; double sum = 0; for (int p = 0; p < 64 * 32; p++) { if (tx.alpha[p] > mx) mx = tx.alpha[p]; sum += tx.alpha[p]; }
                  CHECK(mx == (int)want_max[t - 1] && fabs(sum / (64 * 32) - want_mean[t - 1]) < 0.01, "texture %d alpha max %d mean %.2f (want %.0f / %.2f)", t, mx, sum / (64 * 32), want_max[t - 1], want_mean[t - 1]);
              }
              free(tx.rgb); free(tx.alpha); free(tx.dxt);
          }
          free(tpk.blk); free(d);
      } }

    CHECK(glGetError() == GL_NO_ERROR, "GL error");
    rhi_buffer_destroy(g_mesh.vbo); rhi_buffer_destroy(g_mesh.nbo); rhi_buffer_destroy(g_mesh.cbo); rhi_buffer_destroy(g_mesh.ibo);
    rhi_shader_destroy(g_r.shader); SDL_GL_DeleteContext(ctx); SDL_DestroyWindow(w); SDL_Quit();
    printf("tire_marks_render_test: %d checks, %d failures\n", g_checks, g_fail);
    return g_fail ? 1 : 0;
}
