/* rhi_gl.c - OpenGL backend of rhi.h: desktop GL 2.1, or OpenGL ES 2.0 when
 * built with -DN2_GLES. The CPU-side GL state tracker (rhi_gl_state.c) is part
 * of this backend; the context is created through it so it starts from the GL
 * defaults. */
#include "rhi.h"
#include "rhi_gl_state.h"   /* GL headers; SDL_GL_*Context go through the tracker */
#include <stdio.h>
#include <string.h>

#ifdef N2_GLES
#  define RHI_GL_BUILD_BACKEND RHI_BACKEND_GLES2
#else
#  define RHI_GL_BUILD_BACKEND RHI_BACKEND_GL
#endif

#define RHI_GL_MAX_TEXTURE_MAX_ANISOTROPY 0x84FF   /* EXT_texture_filter_anisotropic */

static struct {
    SDL_Window   *window;
    SDL_GLContext context;
    RhiCaps       caps;
    uint64_t      frame_index;
} g_rhi;

static RhiBackend resolve(RhiBackend backend) {
    return backend == RHI_BACKEND_DEFAULT ? RHI_GL_BUILD_BACKEND : backend;
}

RhiResult rhi_prepare_window(RhiBackend backend) {
    if (resolve(backend) != RHI_GL_BUILD_BACKEND) return RHI_ERR_UNSUPPORTED;
#ifdef N2_GLES
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
#endif
    /* No core profile is requested: macOS then gives the legacy 2.1 context. */
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    return RHI_OK;
}

uint32_t rhi_window_flags(RhiBackend backend) {
    return resolve(backend) == RHI_GL_BUILD_BACKEND ? (uint32_t)SDL_WINDOW_OPENGL : 0u;
}

static int has_extension(const char *ext, const char *name) {
    return ext && strstr(ext, name) != NULL;
}

static void query_caps(RhiCaps *c) {
    GLint limit[2] = {0, 0};
    const char *ext = (const char *)glGetString(GL_EXTENSIONS);
    memset(c, 0, sizeof *c);
    c->backend = RHI_GL_BUILD_BACKEND;
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, limit);
    c->max_viewport[0] = limit[0];
    c->max_viewport[1] = limit[1];
    /* DXT1-only drivers expose texture_compression_dxt1 (GLES). */
    c->tex_bc2 = c->tex_bc3 = (uint8_t)has_extension(ext, "GL_EXT_texture_compression_s3tc");
    c->tex_bc1 = (uint8_t)(c->tex_bc3 || has_extension(ext, "texture_compression_dxt1"));
    c->max_anisotropy = 1.0f;
    if (has_extension(ext, "GL_EXT_texture_filter_anisotropic")) {
        GLfloat mx = 1.0f;
        glGetFloatv(RHI_GL_MAX_TEXTURE_MAX_ANISOTROPY, &mx);
        if (mx > 1.0f) c->max_anisotropy = mx;
    }
}

RhiResult rhi_device_create(const RhiDeviceDesc *desc) {
    if (!desc || !desc->window || g_rhi.context) return RHI_ERR_INVALID;
    if (resolve(desc->backend) != RHI_GL_BUILD_BACKEND) return RHI_ERR_UNSUPPORTED;
    SDL_Window *win = (SDL_Window *)desc->window;
    SDL_GLContext ctx = SDL_GL_CreateContext(win);
    if (!ctx) { fprintf(stderr, "GL ctx: %s\n", SDL_GetError()); return RHI_ERR_UNSUPPORTED; }
    g_rhi.window = win;
    g_rhi.context = ctx;
    g_rhi.frame_index = 0;
    /* Readbacks are tightly packed rows, including odd widths. */
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    query_caps(&g_rhi.caps);
    SDL_GL_SetSwapInterval((desc->flags & RHI_DEVICE_VSYNC) ? 1 : 0);
    return RHI_OK;
}

void rhi_device_destroy(void) {
    if (!g_rhi.context) return;
    SDL_GL_DeleteContext(g_rhi.context);
    memset(&g_rhi, 0, sizeof g_rhi);
}

void rhi_drawable_size(int32_t *width, int32_t *height) {
    int w = 0, h = 0;
    if (g_rhi.window) SDL_GL_GetDrawableSize(g_rhi.window, &w, &h);
    if (width) *width = w;
    if (height) *height = h;
}

const RhiCaps *rhi_caps(void) {
    return &g_rhi.caps;
}

RhiResult rhi_frame_begin(RhiFrameInfo *out) {
    if (!g_rhi.context) return RHI_ERR_INVALID;
    if (out) {
        rhi_drawable_size(&out->width, &out->height);
        out->frame_index = g_rhi.frame_index;
    }
    return RHI_OK;
}

RhiResult rhi_frame_end(void) {
    if (!g_rhi.context) return RHI_ERR_INVALID;
    SDL_GL_SwapWindow(g_rhi.window);
    g_rhi.frame_index++;
    return RHI_OK;
}
