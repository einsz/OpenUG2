/* rhi_gl.c - OpenGL backend of rhi.h: desktop GL 2.1, or OpenGL ES 2.0 when
 * built with -DN2_GLES. The CPU-side GL state tracker (rhi_gl_state.c) is part
 * of this backend; the context is created through it so it starts from the GL
 * defaults. */
#include "rhi.h"
#include "rhi_gl.h"
#include "rhi_gl_state.h"   /* GL headers; SDL_GL_*Context go through the tracker */
#include <stdio.h>
#include <stdlib.h>
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
    int32_t       frame_w, frame_h;   /* main target size of the open frame */
    int           in_frame;
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
    rhi_drawable_size(&g_rhi.frame_w, &g_rhi.frame_h);
    g_rhi.in_frame = 1;
    if (out) {
        out->width = g_rhi.frame_w;
        out->height = g_rhi.frame_h;
        out->frame_index = g_rhi.frame_index;
    }
    return RHI_OK;
}

RhiResult rhi_frame_end(void) {
    if (!g_rhi.context) return RHI_ERR_INVALID;
    SDL_GL_SwapWindow(g_rhi.window);
    g_rhi.frame_index++;
    g_rhi.in_frame = 0;
    return RHI_OK;
}

/* GL_RGBA + GL_UNSIGNED_BYTE is the one combination GLES2 guarantees, so both
 * GL flavours read RGBA and convert; GL rows are bottom-up, so they are flipped
 * here. PACK_ALIGNMENT is 1 (rhi_device_create). */
RhiResult rhi_read_pixels(int32_t x, int32_t y, int32_t w, int32_t h,
                          RhiReadFormat format, void *dst, size_t stride) {
    size_t bpp = format == RHI_READ_RGB8 ? 3 : format == RHI_READ_RGBA8 ? 4 : 0;
    if (!g_rhi.in_frame || !dst || !bpp || w <= 0 || h <= 0 || x < 0 || y < 0 ||
        w > g_rhi.frame_w - x || h > g_rhi.frame_h - y) return RHI_ERR_INVALID;
    if (!stride) stride = (size_t)w * bpp;
    else if (stride < (size_t)w * bpp) return RHI_ERR_INVALID;
    unsigned char *rgba = malloc((size_t)w * h * 4);
    if (!rgba) return RHI_ERR_INVALID;
    glReadPixels(x, g_rhi.frame_h - y - h, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    for (int32_t row = 0; row < h; row++) {
        const unsigned char *src = rgba + (size_t)(h - 1 - row) * w * 4;
        unsigned char *out = (unsigned char *)dst + (size_t)row * stride;
        if (bpp == 4) { memcpy(out, src, (size_t)w * 4); continue; }
        for (int32_t i = 0; i < w; i++) {
            out[i*3] = src[i*4]; out[i*3+1] = src[i*4+1]; out[i*3+2] = src[i*4+2];
        }
    }
    free(rgba);
    return RHI_OK;
}

/* ------------------------------------------------------------------------------ */
/* Buffers                                                                        */
/* ------------------------------------------------------------------------------ */
/* Handle id = generation << 24 | (slot + 1). The table is independent of the
 * device so code that creates its own GL context (tests) can use buffers too.
 * Binding goes through the state tracker macros like any other GL caller. */
#define RHI_GL_SLOT_BITS 24u
#define RHI_GL_SLOT_MASK ((1u << RHI_GL_SLOT_BITS) - 1u)

typedef struct {
    GLuint   name;     /* 0 = free slot */
    GLenum   target;   /* GL_ARRAY_BUFFER / GL_ELEMENT_ARRAY_BUFFER */
    GLenum   hint;     /* GL_STATIC_DRAW / GL_STREAM_DRAW */
    uint8_t  generation;
    size_t   size;
} RhiGlBuffer;

static struct {
    RhiGlBuffer *slots;
    uint32_t     count, capacity;
    uint32_t    *free_list;
    uint32_t     free_count;
    unsigned     stale_logged;
} g_buffers;

static RhiGlBuffer *buffer_lookup(RhiBuffer buf, const char *what) {
    uint32_t slot = (buf.id & RHI_GL_SLOT_MASK);
    if (!buf.id) return NULL;
    if (slot && slot <= g_buffers.count) {
        RhiGlBuffer *b = &g_buffers.slots[slot - 1];
        if (b->name && b->generation == (uint8_t)(buf.id >> RHI_GL_SLOT_BITS)) return b;
    }
    if (g_buffers.stale_logged < 8) {
        g_buffers.stale_logged++;
        fprintf(stderr, "rhi: %s: stale or invalid buffer handle 0x%08x\n",
                what, (unsigned)buf.id);
    }
    return NULL;
}

static int buffer_size_ok(size_t size) {
    return size <= (size_t)(((uint64_t)1 << (sizeof(GLsizeiptr) * 8 - 1)) - 1);
}

GLuint rhi_gl_buffer_name(RhiBuffer buf) {
    const RhiGlBuffer *b = buffer_lookup(buf, "bind");
    return b ? b->name : 0;
}

static uint32_t buffer_slot_alloc(void) {
    if (g_buffers.free_count) return g_buffers.free_list[--g_buffers.free_count];
    if (g_buffers.count == g_buffers.capacity) {
        uint32_t cap = g_buffers.capacity ? g_buffers.capacity * 2u : 256u;
        if (cap > RHI_GL_SLOT_MASK) return 0;
        RhiGlBuffer *slots = (RhiGlBuffer *)realloc(g_buffers.slots, cap * sizeof *slots);
        if (!slots) return 0;
        g_buffers.slots = slots;
        uint32_t *list = (uint32_t *)realloc(g_buffers.free_list, cap * sizeof *list);
        if (!list) return 0;
        g_buffers.free_list = list;
        g_buffers.capacity = cap;
    }
    memset(&g_buffers.slots[g_buffers.count], 0, sizeof g_buffers.slots[0]);
    return ++g_buffers.count;
}

RhiBuffer rhi_buffer_create(const RhiBufferDesc *desc) {
    RhiBuffer out = {0};
    if (!desc || (desc->usage != RHI_BUFFER_VERTEX && desc->usage != RHI_BUFFER_INDEX) ||
        (desc->update != RHI_UPDATE_STATIC && desc->update != RHI_UPDATE_STREAM) ||
        !buffer_size_ok(desc->size)) {
        fprintf(stderr, "rhi: buffer_create: invalid description\n");
        return out;
    }
    uint32_t slot = buffer_slot_alloc();
    if (!slot) { fprintf(stderr, "rhi: buffer_create: out of handles\n"); return out; }
    RhiGlBuffer *b = &g_buffers.slots[slot - 1];
    GLuint name = 0;
    glGenBuffers(1, &name);
    if (!name) {
        g_buffers.free_list[g_buffers.free_count++] = slot;
        fprintf(stderr, "rhi: buffer_create: glGenBuffers failed\n");
        return out;
    }
    b->name = name;
    b->target = desc->usage == RHI_BUFFER_INDEX ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
    b->hint = desc->update == RHI_UPDATE_STREAM ? GL_STREAM_DRAW : GL_STATIC_DRAW;
    b->size = 0;
    out.id = ((uint32_t)b->generation << RHI_GL_SLOT_BITS) | slot;
    /* A STREAM buffer without a size gets its storage from the first replace. */
    if (desc->update == RHI_UPDATE_STATIC || desc->size || desc->data) {
        glBindBuffer(b->target, name);
        glBufferData(b->target, (GLsizeiptr)desc->size, desc->data, b->hint);
        b->size = desc->size;
    }
    return out;
}

RhiResult rhi_buffer_update(RhiBuffer buf, size_t offset, size_t size, const void *data) {
    RhiGlBuffer *b = buffer_lookup(buf, "buffer_update");
    if (!b || !data || offset > b->size || size > b->size - offset) return RHI_ERR_INVALID;
    glBindBuffer(b->target, b->name);
    glBufferSubData(b->target, (GLintptr)offset, (GLsizeiptr)size, data);
    return RHI_OK;
}

RhiResult rhi_buffer_replace(RhiBuffer buf, size_t size, const void *data) {
    RhiGlBuffer *b = buffer_lookup(buf, "buffer_replace");
    if (!b || !buffer_size_ok(size)) return RHI_ERR_INVALID;
    glBindBuffer(b->target, b->name);
    glBufferData(b->target, (GLsizeiptr)size, data, b->hint);
    b->size = size;
    return RHI_OK;
}

void rhi_buffer_destroy(RhiBuffer buf) {
    RhiGlBuffer *b = buffer_lookup(buf, "buffer_destroy");
    if (!b) return;
    glDeleteBuffers(1, &b->name);
    b->name = 0;
    b->size = 0;
    b->generation++;
    g_buffers.free_list[g_buffers.free_count++] = buf.id & RHI_GL_SLOT_MASK;
}
