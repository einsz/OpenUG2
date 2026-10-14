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
#include <limits.h>

#if defined(__APPLE__) && !defined(N2_GLES)
/* The legacy macOS GL 2.1 context has framebuffer objects only as the EXT
   extension. Bind/Delete Framebuffers/Renderbuffers go through
   rhi_gl_state.c, which maps them to the EXT entry points itself. */
#define glGenFramebuffers glGenFramebuffersEXT
#define glFramebufferTexture2D glFramebufferTexture2DEXT
#define glCheckFramebufferStatus glCheckFramebufferStatusEXT
#define glGenRenderbuffers glGenRenderbuffersEXT
#define glRenderbufferStorage glRenderbufferStorageEXT
#define glFramebufferRenderbuffer glFramebufferRenderbufferEXT
#endif

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
    size_t limit = (size_t)PTRDIFF_MAX;
    if ((size_t)w > limit / 4 / (size_t)h) return RHI_ERR_INVALID;
    size_t row_bytes = (size_t)w * bpp;
    if (!stride) stride = row_bytes;
    if (stride < row_bytes || (size_t)(h - 1) > (limit - row_bytes) / stride)
        return RHI_ERR_INVALID;
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
    /* Retire the slot before its generation wraps and revives stale handles. */
    if (b->generation == UINT8_MAX) return;
    b->generation++;
    g_buffers.free_list[g_buffers.free_count++] = buf.id & RHI_GL_SLOT_MASK;
}

/* ------------------------------------------------------------------------------ */
/* Textures                                                                       */
/* ------------------------------------------------------------------------------ */
/* Same handle scheme as buffers, separate table. GL calls go through the state
 * tracker macros, so the binding they leave behind is tracked. */
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#  define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#  define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#  define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

static const RhiSamplerDesc k_default_sampler = {
    RHI_FILTER_NEAREST, RHI_FILTER_LINEAR, RHI_MIP_LINEAR, RHI_WRAP_REPEAT, RHI_WRAP_REPEAT, 1.0f
};

typedef struct {
    GLuint    name;     /* 0 = free slot */
    GLenum    target;   /* GL_TEXTURE_2D / GL_TEXTURE_CUBE_MAP */
    RhiFormat format;
    uint32_t  usage;
    int32_t   width, height;
    uint8_t   generation;
    RhiSampler sampler;
    GLint     wrap_s, wrap_t, min_filter, mag_filter;   /* parameters GL has now */
    float     anisotropy;
} RhiGlTexture;

static struct {
    RhiGlTexture *slots;
    uint32_t      count, capacity;
    uint32_t     *free_list;
    uint32_t      free_count;
    unsigned      stale_logged;
} g_textures;

static void rhi_gl_forget_framebuffers(RhiTexture tex);   /* passes, below */

static RhiGlTexture *texture_find(RhiTexture tex) {
    uint32_t slot = tex.id & RHI_GL_SLOT_MASK;
    if (!slot || slot > g_textures.count) return NULL;
    RhiGlTexture *t = &g_textures.slots[slot - 1];
    return t->name && t->generation == (uint8_t)(tex.id >> RHI_GL_SLOT_BITS) ? t : NULL;
}

static RhiGlTexture *texture_lookup(RhiTexture tex, const char *what) {
    RhiGlTexture *t = texture_find(tex);
    if (!t && tex.id && g_textures.stale_logged < 8) {
        g_textures.stale_logged++;
        fprintf(stderr, "rhi: %s: stale or invalid texture handle 0x%08x\n",
                what, (unsigned)tex.id);
    }
    return t;
}

GLuint rhi_gl_texture_name(RhiTexture tex) {
    const RhiGlTexture *t = texture_lookup(tex, "bind");
    return t ? t->name : 0;
}

int rhi_texture_is_live(RhiTexture tex) {
    return texture_find(tex) != NULL;
}

static uint32_t texture_slot_alloc(void) {
    if (g_textures.free_count) return g_textures.free_list[--g_textures.free_count];
    if (g_textures.count == g_textures.capacity) {
        uint32_t cap = g_textures.capacity ? g_textures.capacity * 2u : 1024u;
        if (cap > RHI_GL_SLOT_MASK) return 0;
        RhiGlTexture *slots = (RhiGlTexture *)realloc(g_textures.slots, cap * sizeof *slots);
        if (!slots) return 0;
        g_textures.slots = slots;
        uint32_t *list = (uint32_t *)realloc(g_textures.free_list, cap * sizeof *list);
        if (!list) return 0;
        g_textures.free_list = list;
        g_textures.capacity = cap;
    }
    memset(&g_textures.slots[g_textures.count], 0, sizeof g_textures.slots[0]);
    return ++g_textures.count;
}

static int format_compressed(RhiFormat f) {
    return f == RHI_FORMAT_BC1 || f == RHI_FORMAT_BC2 || f == RHI_FORMAT_BC3;
}

RhiTexture rhi_texture_create(const RhiTextureDesc *desc) {
    RhiTexture out = {0};
    int depth = desc && desc->format == RHI_FORMAT_D16;
    if (!desc || (desc->type != RHI_TEXTURE_2D && desc->type != RHI_TEXTURE_CUBE) ||
        desc->format < RHI_FORMAT_RGB8 || desc->format > RHI_FORMAT_D16 ||
        desc->width <= 0 || desc->height <= 0 || !desc->usage ||
        (desc->usage & ~(uint32_t)(RHI_TEXTURE_SAMPLED | RHI_TEXTURE_COLOR_TARGET |
                                   RHI_TEXTURE_DEPTH_TARGET)) ||
        (depth && (desc->type != RHI_TEXTURE_2D || desc->usage != RHI_TEXTURE_DEPTH_TARGET)) ||
        (!depth && (desc->usage & RHI_TEXTURE_DEPTH_TARGET)) ||
        (desc->type == RHI_TEXTURE_CUBE && desc->width != desc->height)) {
        fprintf(stderr, "rhi: texture_create: invalid description\n");
        return out;
    }
    RhiSampler sampler = rhi_sampler_create(&k_default_sampler);
    if (!sampler.id) return out;
    uint32_t slot = texture_slot_alloc();
    if (!slot) { fprintf(stderr, "rhi: texture_create: out of handles\n"); return out; }
    RhiGlTexture *t = &g_textures.slots[slot - 1];
    GLuint name = 0;
    if (depth) glGenRenderbuffers(1, &name);   /* depth target, not sampled */
    else glGenTextures(1, &name);
    if (!name) {
        g_textures.free_list[g_textures.free_count++] = slot;
        fprintf(stderr, "rhi: texture_create: glGenTextures failed\n");
        return out;
    }
    t->name = name;
    t->target = depth ? GL_RENDERBUFFER
              : desc->type == RHI_TEXTURE_CUBE ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
    t->format = desc->format;
    t->usage = desc->usage;
    t->width = desc->width;
    t->height = desc->height;
    t->sampler = sampler;
    t->wrap_s = t->wrap_t = GL_REPEAT;               /* GL's initial parameters */
    t->min_filter = GL_NEAREST_MIPMAP_LINEAR;
    t->mag_filter = GL_LINEAR;
    t->anisotropy = 1.0f;
    out.id = ((uint32_t)t->generation << RHI_GL_SLOT_BITS) | slot;
    if (depth) {
        glBindRenderbuffer(GL_RENDERBUFFER, name);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, desc->width, desc->height);
        return out;
    }
    glBindTexture(t->target, name);
    return out;
}

RhiResult rhi_texture_upload(RhiTexture tex, int32_t face, int32_t level,
                             const void *data, size_t size) {
    RhiGlTexture *t = texture_lookup(tex, "texture_upload");
    int faces = t && t->target == GL_TEXTURE_CUBE_MAP ? 6 : 1;
    if (!t || t->target == GL_RENDERBUFFER || face < 0 || face >= faces || level < 0 || level > 30)
        return RHI_ERR_INVALID;
    GLsizei w = t->width >> level, h = t->height >> level;
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    GLenum target = faces == 6 ? (GLenum)(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face) : GL_TEXTURE_2D;
    glBindTexture(t->target, t->name);
    if (format_compressed(t->format)) {
        if (!data || !size || size > 0x7fffffff) return RHI_ERR_INVALID;
        GLenum fmt = t->format == RHI_FORMAT_BC1 ? GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
                   : t->format == RHI_FORMAT_BC2 ? GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
                                                 : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        glCompressedTexImage2D(target, level, fmt, w, h, 0, (GLsizei)size, data);
    } else {
        GLenum fmt = t->format == RHI_FORMAT_RGB8 ? GL_RGB : GL_RGBA;
        glTexImage2D(target, level, (GLint)fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    }
    return RHI_OK;
}

RhiResult rhi_texture_generate_mips(RhiTexture tex) {
    RhiGlTexture *t = texture_lookup(tex, "texture_generate_mips");
    if (!t || t->target != GL_TEXTURE_2D || format_compressed(t->format)) return RHI_ERR_INVALID;
    glBindTexture(GL_TEXTURE_2D, t->name);
    glGenerateMipmap(GL_TEXTURE_2D);
    return RHI_OK;
}

void rhi_texture_destroy(RhiTexture tex) {
    RhiGlTexture *t = texture_lookup(tex, "texture_destroy");
    if (!t) return;
    rhi_gl_forget_framebuffers(tex);
    if (t->target == GL_RENDERBUFFER) glDeleteRenderbuffers(1, &t->name);
    else glDeleteTextures(1, &t->name);
    t->name = 0;
    /* Retire the slot before its generation can alias an old handle. */
    if (t->generation == UINT8_MAX) return;
    t->generation++;
    g_textures.free_list[g_textures.free_count++] = tex.id & RHI_GL_SLOT_MASK;
}

/* ------------------------------------------------------------------------------ */
/* Samplers                                                                       */
/* ------------------------------------------------------------------------------ */
/* A short list searched by value; handle id = index + 1. The game uses a
 * handful of samplers (wrap x filter x anisotropy setting). */
#define RHI_GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE

static struct {
    RhiSamplerDesc *descs;
    uint32_t        count, capacity;
} g_samplers;

static int sampler_desc_valid(const RhiSamplerDesc *d) {
    return d && (d->min_filter == RHI_FILTER_NEAREST || d->min_filter == RHI_FILTER_LINEAR) &&
           (d->mag_filter == RHI_FILTER_NEAREST || d->mag_filter == RHI_FILTER_LINEAR) &&
           d->mip_filter >= RHI_MIP_NONE && d->mip_filter <= RHI_MIP_LINEAR &&
           (d->wrap_u == RHI_WRAP_REPEAT || d->wrap_u == RHI_WRAP_CLAMP) &&
           (d->wrap_v == RHI_WRAP_REPEAT || d->wrap_v == RHI_WRAP_CLAMP) &&
           d->max_anisotropy >= 1.0f;
}

RhiSampler rhi_sampler_create(const RhiSamplerDesc *desc) {
    RhiSampler out = {0};
    if (!sampler_desc_valid(desc)) {
        fprintf(stderr, "rhi: sampler_create: invalid description\n");
        return out;
    }
    for (uint32_t i = 0; i < g_samplers.count; i++) {
        const RhiSamplerDesc *d = &g_samplers.descs[i];
        if (d->min_filter == desc->min_filter && d->mag_filter == desc->mag_filter &&
            d->mip_filter == desc->mip_filter && d->wrap_u == desc->wrap_u &&
            d->wrap_v == desc->wrap_v && d->max_anisotropy == desc->max_anisotropy) {
            out.id = i + 1;
            return out;
        }
    }
    if (g_samplers.count == g_samplers.capacity) {
        uint32_t cap = g_samplers.capacity ? g_samplers.capacity * 2u : 16u;
        RhiSamplerDesc *descs = (RhiSamplerDesc *)realloc(g_samplers.descs, cap * sizeof *descs);
        if (!descs) { fprintf(stderr, "rhi: sampler_create: out of memory\n"); return out; }
        g_samplers.descs = descs;
        g_samplers.capacity = cap;
    }
    g_samplers.descs[g_samplers.count++] = *desc;
    out.id = g_samplers.count;
    return out;
}

const RhiSamplerDesc *rhi_sampler_desc(RhiSampler sampler) {
    if (!sampler.id || sampler.id > g_samplers.count) return NULL;
    return &g_samplers.descs[sampler.id - 1];
}

RhiSampler rhi_texture_sampler(RhiTexture tex) {
    const RhiGlTexture *t = texture_lookup(tex, "texture_sampler");
    return t ? t->sampler : (RhiSampler){0};
}

static GLint gl_min_filter(const RhiSamplerDesc *d) {
    static const GLint table[2][3] = {
        { GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR },
        { GL_LINEAR,  GL_LINEAR_MIPMAP_NEAREST,  GL_LINEAR_MIPMAP_LINEAR  },
    };
    return table[d->min_filter][d->mip_filter];
}

/* Only parameters that differ from what the texture has are set. Anisotropy
 * is left alone when the driver has no anisotropic filtering. */
RhiResult rhi_texture_set_sampler(RhiTexture tex, RhiSampler sampler) {
    RhiGlTexture *t = texture_lookup(tex, "texture_set_sampler");
    const RhiSamplerDesc *d = rhi_sampler_desc(sampler);
    if (!t || !d || t->target == GL_RENDERBUFFER) return RHI_ERR_INVALID;
    GLint wrap_s = d->wrap_u == RHI_WRAP_CLAMP ? GL_CLAMP_TO_EDGE : GL_REPEAT;
    GLint wrap_t = d->wrap_v == RHI_WRAP_CLAMP ? GL_CLAMP_TO_EDGE : GL_REPEAT;
    GLint min_filter = gl_min_filter(d);
    GLint mag_filter = d->mag_filter == RHI_FILTER_LINEAR ? GL_LINEAR : GL_NEAREST;
    float aniso = 1.0f;
    if (g_rhi.caps.max_anisotropy > 1.0f)
        aniso = d->max_anisotropy < g_rhi.caps.max_anisotropy ? d->max_anisotropy
                                                              : g_rhi.caps.max_anisotropy;
    glBindTexture(t->target, t->name);
    if (wrap_s != t->wrap_s) glTexParameteri(t->target, GL_TEXTURE_WRAP_S, wrap_s);
    if (wrap_t != t->wrap_t) glTexParameteri(t->target, GL_TEXTURE_WRAP_T, wrap_t);
    if (min_filter != t->min_filter) glTexParameteri(t->target, GL_TEXTURE_MIN_FILTER, min_filter);
    if (mag_filter != t->mag_filter) glTexParameteri(t->target, GL_TEXTURE_MAG_FILTER, mag_filter);
    if (aniso != t->anisotropy)
        glTexParameterf(t->target, RHI_GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
    t->wrap_s = wrap_s; t->wrap_t = wrap_t;
    t->min_filter = min_filter; t->mag_filter = mag_filter;
    t->anisotropy = aniso;
    t->sampler = sampler;
    return RHI_OK;
}

/* ------------------------------------------------------------------------------ */
/* Shaders                                                                        */
/* ------------------------------------------------------------------------------ */
/* A handful of programs: a fixed table with generation-checked ids. */
#define RHI_GL_MAX_SHADERS 64u
#define RHI_GL_MAX_PARAM_BLOCKS 4u

/* One parameter block of a program: the game-visible storage, the bytes last
 * uploaded (GL's own values: zero after linking) and a location per field. */
typedef struct {
    const RhiParamLayout *layout;   /* the caller's table, kept by pointer */
    unsigned char        *data, *uploaded;
    GLint                *location;
} RhiGlParamBlock;

static struct {
    GLuint          program[RHI_GL_MAX_SHADERS];   /* 0 = free slot */
    uint8_t         generation[RHI_GL_MAX_SHADERS];
    RhiGlParamBlock block[RHI_GL_MAX_SHADERS][RHI_GL_MAX_PARAM_BLOCKS];
    uint32_t        block_count[RHI_GL_MAX_SHADERS];
} g_shaders;

static void param_blocks_free(int slot) {
    for (uint32_t b = 0; b < g_shaders.block_count[slot]; b++) {
        RhiGlParamBlock *pb = &g_shaders.block[slot][b];
        free(pb->data); free(pb->uploaded); free(pb->location);
        memset(pb, 0, sizeof *pb);
    }
    g_shaders.block_count[slot] = 0;
}

/* ---- std140 checks ---- */
static uint32_t param_align(RhiParamType t) {
    return t == RHI_PARAM_VEC2 ? 8u : (t == RHI_PARAM_VEC3 || t == RHI_PARAM_VEC4 ||
                                       t == RHI_PARAM_MAT4) ? 16u : 4u;
}
static uint32_t param_size(RhiParamType t) {
    switch (t) {
    case RHI_PARAM_VEC2: return 8u;
    case RHI_PARAM_VEC3: return 12u;
    case RHI_PARAM_VEC4: return 16u;
    case RHI_PARAM_MAT4: return 64u;
    default:             return 4u;
    }
}

RhiResult rhi_param_layout_check(const RhiParamLayout *l) {
    const char *name = l && l->name ? l->name : "params";
    if (!l || !l->fields || !l->field_count) {
        fprintf(stderr, "rhi: %s: empty parameter layout\n", name);
        return RHI_ERR_INVALID;
    }
    for (uint32_t i = 0; i < l->field_count; i++) {
        const RhiParamField *f = &l->fields[i];
        if (!f->name || f->type < RHI_PARAM_FLOAT || f->type > RHI_PARAM_INT || !f->count) {
            fprintf(stderr, "rhi: %s: field %u is malformed\n", name, (unsigned)i);
            return RHI_ERR_INVALID;
        }
        if (f->count > 1 && f->type != RHI_PARAM_VEC4 && f->type != RHI_PARAM_MAT4) {
            fprintf(stderr, "rhi: %s: %s: std140 arrays must be vec4 or mat4\n", name, f->name);
            return RHI_ERR_INVALID;
        }
        uint64_t end = (uint64_t)f->offset + (uint64_t)param_size(f->type) * f->count;
        if (f->offset % param_align(f->type) || end > l->size || end < f->offset) {
            fprintf(stderr, "rhi: %s: %s: offset %u breaks std140 alignment or the block size\n",
                    name, f->name, (unsigned)f->offset);
            return RHI_ERR_INVALID;
        }
        for (uint32_t j = 0; j < i; j++) {
            const RhiParamField *g = &l->fields[j];
            uint64_t gend = (uint64_t)g->offset + (uint64_t)param_size(g->type) * g->count;
            if (f->offset < gend && g->offset < end) {
                fprintf(stderr, "rhi: %s: %s overlaps %s\n", name, f->name, g->name);
                return RHI_ERR_INVALID;
            }
        }
    }
    return RHI_OK;
}

static int shader_slot(RhiShader shader) {
    uint32_t slot = shader.id & RHI_GL_SLOT_MASK;
    if (!slot || slot > RHI_GL_MAX_SHADERS || !g_shaders.program[slot - 1] ||
        g_shaders.generation[slot - 1] != (uint8_t)(shader.id >> RHI_GL_SLOT_BITS)) return -1;
    return (int)slot - 1;
}

GLuint rhi_gl_shader_program(RhiShader shader) {
    int i = shader_slot(shader);
    return i < 0 ? 0 : g_shaders.program[i];
}

static GLuint shader_compile(GLenum type, const char *src, const char *label) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, (GLsizei)sizeof log, NULL, log);
        fprintf(stderr, "rhi: %s %s shader: %s\n", label,
                type == GL_VERTEX_SHADER ? "vertex" : "fragment", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

RhiShader rhi_shader_create(const RhiShaderDesc *desc) {
    RhiShader out = {0};
    if (!desc || !desc->vertex_source || !desc->fragment_source ||
        (desc->attribute_count && !desc->attribute_names)) {
        fprintf(stderr, "rhi: shader_create: invalid description\n");
        return out;
    }
    const char *label = desc->label ? desc->label : "unnamed";
    int slot = -1;
    for (uint32_t i = 0; i < RHI_GL_MAX_SHADERS && slot < 0; i++)
        if (!g_shaders.program[i] && g_shaders.generation[i] != UINT8_MAX) slot = (int)i;
    if (slot < 0) { fprintf(stderr, "rhi: shader_create: out of handles\n"); return out; }
    GLuint v = shader_compile(GL_VERTEX_SHADER, desc->vertex_source, label);
    GLuint f = shader_compile(GL_FRAGMENT_SHADER, desc->fragment_source, label);
    if (!v || !f) {
        if (v) glDeleteShader(v);
        if (f) glDeleteShader(f);
        return out;
    }
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    for (uint32_t i = 0; i < desc->attribute_count; i++)
        glBindAttribLocation(p, i, desc->attribute_names[i]);
    glLinkProgram(p);
    glDeleteShader(v);   /* freed with the program */
    glDeleteShader(f);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(p, (GLsizei)sizeof log, NULL, log);
        fprintf(stderr, "rhi: %s shader link: %s\n", label, log);
        glDeleteProgram(p);
        return out;
    }
    if (desc->param_block_count > RHI_GL_MAX_PARAM_BLOCKS ||
        (desc->param_block_count && !desc->param_blocks)) {
        fprintf(stderr, "rhi: %s: too many parameter blocks\n", label);
        glDeleteProgram(p);
        return out;
    }
    for (uint32_t b = 0; b < desc->param_block_count; b++) {
        const RhiParamLayout *l = desc->param_blocks[b];
        RhiGlParamBlock *pb = &g_shaders.block[slot][b];
        g_shaders.block_count[slot] = b + 1;
        if (!l || rhi_param_layout_check(l) != RHI_OK ||
            !(pb->data = (unsigned char *)calloc(1, l->size)) ||
            !(pb->uploaded = (unsigned char *)calloc(1, l->size)) ||
            !(pb->location = (GLint *)malloc(l->field_count * sizeof *pb->location))) {
            fprintf(stderr, "rhi: %s: parameter block %u unusable\n", label, (unsigned)b);
            param_blocks_free(slot);
            glDeleteProgram(p);
            return out;
        }
        pb->layout = l;
        for (uint32_t i = 0; i < l->field_count; i++)   /* -1: unused by the shader */
            pb->location[i] = glGetUniformLocation(p, l->fields[i].name);
    }
    g_shaders.program[slot] = p;
    out.id = ((uint32_t)g_shaders.generation[slot] << RHI_GL_SLOT_BITS) | (uint32_t)(slot + 1);
    return out;
}

void *rhi_shader_params(RhiShader shader, uint32_t block) {
    int i = shader_slot(shader);
    if (i < 0 || block >= g_shaders.block_count[i]) return NULL;
    return g_shaders.block[i][block].data;
}

static void param_upload(const RhiParamField *f, GLint loc, const void *v) {
    GLsizei n = (GLsizei)f->count;
    switch (f->type) {
    case RHI_PARAM_FLOAT: glUniform1fv(loc, n, (const GLfloat *)v); break;
    case RHI_PARAM_VEC2:  glUniform2fv(loc, n, (const GLfloat *)v); break;
    case RHI_PARAM_VEC3:  glUniform3fv(loc, n, (const GLfloat *)v); break;
    case RHI_PARAM_VEC4:  glUniform4fv(loc, n, (const GLfloat *)v); break;
    case RHI_PARAM_MAT4:  glUniformMatrix4fv(loc, n, GL_FALSE, (const GLfloat *)v); break;
    case RHI_PARAM_INT:   glUniform1i(loc, *(const GLint *)v); break;
    }
}

void rhi_gl_flush_params(void) {
    static int last = -1;   /* the shader found last time, usually the same */
    GLint current = 0;
    gls_get_integerv(GL_CURRENT_PROGRAM, &current);
    if (!current) return;
    int slot = -1;
    if (last >= 0 && g_shaders.program[last] == (GLuint)current) slot = last;
    for (uint32_t i = 0; slot < 0 && i < RHI_GL_MAX_SHADERS; i++)
        if (g_shaders.program[i] == (GLuint)current) slot = (int)i;
    if (slot < 0) return;   /* a program not created through rhi_shader_create */
    last = slot;
    for (uint32_t b = 0; b < g_shaders.block_count[slot]; b++) {
        RhiGlParamBlock *pb = &g_shaders.block[slot][b];
        const RhiParamLayout *l = pb->layout;
        for (uint32_t i = 0; i < l->field_count; i++) {
            const RhiParamField *f = &l->fields[i];
            size_t off = f->offset, n = param_size(f->type) * f->count;
            if (pb->location[i] < 0 || !memcmp(pb->data + off, pb->uploaded + off, n)) continue;
            param_upload(f, pb->location[i], pb->data + off);
            memcpy(pb->uploaded + off, pb->data + off, n);
        }
    }
}

void rhi_shader_destroy(RhiShader shader) {
    int i = shader_slot(shader);
    if (i < 0) return;
    glDeleteProgram(g_shaders.program[i]);
    param_blocks_free(i);
    g_shaders.program[i] = 0;
    if (g_shaders.generation[i] != UINT8_MAX) g_shaders.generation[i]++;
}

/* ------------------------------------------------------------------------------ */
/* Pipelines and draws                                                            */
/* ------------------------------------------------------------------------------ */
#define RHI_GL_MAX_PIPELINES 64u

static struct {
    RhiPipelineDesc desc[RHI_GL_MAX_PIPELINES];
    uint32_t        count;
} g_pipelines;

static struct {
    int        pipeline;                          /* index, -1 = none bound */
    RhiBuffer  vertex[RHI_MAX_VERTEX_STREAMS];
    uint32_t   vertex_offset[RHI_MAX_VERTEX_STREAMS];
    RhiBuffer  index;
} g_cmd = { -1, {{0}}, {0}, {0} };

void rhi_pipeline_desc_init(RhiPipelineDesc *d) {
    memset(d, 0, sizeof *d);
    d->primitive = RHI_PRIMITIVE_TRIANGLES;
    d->cull = RHI_CULL_NONE;
    d->depth_test = 0;
    d->depth_write = 1;
    d->depth_compare = RHI_COMPARE_LESS;
    d->blend = 0;
    d->src_rgb = d->src_alpha = RHI_BLEND_ONE;
    d->dst_rgb = d->dst_alpha = RHI_BLEND_ZERO;
    d->color_write = 0xF;
    d->stencil_test = 0;
    d->stencil_compare = RHI_COMPARE_ALWAYS;
    d->stencil_ref = 0;
    d->stencil_read_mask = d->stencil_write_mask = 0xFF;
    d->stencil_fail = d->stencil_depth_fail = d->stencil_pass = RHI_STENCIL_KEEP;
}

static int vertex_layout_equal(const RhiVertexLayout *a, const RhiVertexLayout *b) {
    if (a->attrib_count != b->attrib_count) return 0;
    for (uint32_t i = 0; i < RHI_MAX_VERTEX_STREAMS; i++)
        if (a->stride[i] != b->stride[i]) return 0;
    for (uint32_t i = 0; i < a->attrib_count; i++) {
        const RhiVertexAttrib *x = &a->attribs[i], *y = &b->attribs[i];
        if (x->location != y->location || x->stream != y->stream || x->offset != y->offset ||
            x->format != y->format) return 0;
    }
    return 1;
}

static int pipeline_desc_equal(const RhiPipelineDesc *a, const RhiPipelineDesc *b) {
    return a->shader.id == b->shader.id && vertex_layout_equal(&a->vertex, &b->vertex) &&
           a->primitive == b->primitive && a->cull == b->cull &&
           !a->depth_test == !b->depth_test && !a->depth_write == !b->depth_write &&
           a->depth_compare == b->depth_compare && !a->blend == !b->blend &&
           a->src_rgb == b->src_rgb && a->dst_rgb == b->dst_rgb &&
           a->src_alpha == b->src_alpha && a->dst_alpha == b->dst_alpha &&
           (a->color_write & 0xF) == (b->color_write & 0xF) &&
           !a->stencil_test == !b->stencil_test && a->stencil_compare == b->stencil_compare &&
           a->stencil_ref == b->stencil_ref && a->stencil_read_mask == b->stencil_read_mask &&
           a->stencil_write_mask == b->stencil_write_mask && a->stencil_fail == b->stencil_fail &&
           a->stencil_depth_fail == b->stencil_depth_fail && a->stencil_pass == b->stencil_pass;
}

static uint32_t vertex_format_bytes(RhiVertexFormat format) {
    static const uint32_t bytes[] = {0, 8, 12, 16, 4, 4};
    return format >= RHI_VERTEX_FLOAT2 && format <= RHI_VERTEX_FLOAT1 ? bytes[format] : 0;
}

RhiPipeline rhi_pipeline_create(const RhiPipelineDesc *desc) {
    RhiPipeline out = {0};
    if (!desc || shader_slot(desc->shader) < 0 ||
        desc->vertex.attrib_count > RHI_MAX_VERTEX_ATTRIBS ||
        desc->primitive > RHI_PRIMITIVE_LINES || desc->cull > RHI_CULL_FRONT ||
        desc->depth_compare > RHI_COMPARE_ALWAYS || desc->stencil_compare > RHI_COMPARE_ALWAYS ||
        desc->stencil_fail > RHI_STENCIL_INVERT || desc->stencil_depth_fail > RHI_STENCIL_INVERT ||
        desc->stencil_pass > RHI_STENCIL_INVERT ||
        desc->src_rgb > RHI_BLEND_ONE_MINUS_DST_COLOR || desc->dst_rgb > RHI_BLEND_ONE_MINUS_DST_COLOR ||
        desc->src_alpha > RHI_BLEND_ONE_MINUS_DST_COLOR || desc->dst_alpha > RHI_BLEND_ONE_MINUS_DST_COLOR) {
        fprintf(stderr, "rhi: pipeline_create: invalid description\n");
        return out;
    }
    uint32_t locations = 0;
    for (uint32_t i = 0; i < desc->vertex.attrib_count; i++) {
        const RhiVertexAttrib *a = &desc->vertex.attribs[i];
        uint32_t bytes = vertex_format_bytes(a->format);
        if (a->stream >= RHI_MAX_VERTEX_STREAMS || !bytes ||
            a->location >= RHI_MAX_VERTEX_ATTRIBS || (locations & (1u << a->location)) ||
            desc->vertex.stride[a->stream] > INT_MAX ||
            (uint64_t)a->offset + bytes > (desc->vertex.stride[a->stream] ? desc->vertex.stride[a->stream] : bytes)) {
            fprintf(stderr, "rhi: pipeline_create: invalid vertex attribute %u\n", (unsigned)i);
            return out;
        }
        locations |= 1u << a->location;
    }
    for (uint32_t i = 0; i < g_pipelines.count; i++)
        if (pipeline_desc_equal(&g_pipelines.desc[i], desc)) { out.id = i + 1; return out; }
    if (g_pipelines.count == RHI_GL_MAX_PIPELINES) {
        fprintf(stderr, "rhi: pipeline_create: out of handles\n");
        return out;
    }
    g_pipelines.desc[g_pipelines.count++] = *desc;
    out.id = g_pipelines.count;
    return out;
}

static void set_cap(GLenum cap, int on) {
    if (!gls_is_enabled(cap) != !on) { if (on) glEnable(cap); else glDisable(cap); }
}

static GLenum gl_compare(RhiCompare c) {
    static const GLenum t[] = { GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL,
                                GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS };
    return t[c];
}
static GLenum gl_stencil_op(RhiStencilOp op) {
    static const GLenum t[] = { GL_KEEP, GL_ZERO, GL_REPLACE, GL_INCR, GL_DECR, GL_INVERT };
    return t[op];
}
static GLenum gl_blend(RhiBlendFactor f) {
    static const GLenum t[] = { GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                                GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_SRC_COLOR,
                                GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR };
    return t[f];
}

void rhi_cmd_bind_pipeline(RhiPipeline pipeline) {
    if (!pipeline.id || pipeline.id > g_pipelines.count) {
        fprintf(stderr, "rhi: bind_pipeline: invalid handle %u\n", (unsigned)pipeline.id);
        g_cmd.pipeline = -1;
        return;
    }
    const RhiPipelineDesc *d = &g_pipelines.desc[pipeline.id - 1];
    if (shader_slot(d->shader) < 0) {
        fprintf(stderr, "rhi: bind_pipeline: destroyed shader\n");
        g_cmd.pipeline = -1;
        return;
    }
    g_cmd.pipeline = (int)pipeline.id - 1;
    GLint current = 0;
    GLuint program = rhi_gl_shader_program(d->shader);
    gls_get_integerv(GL_CURRENT_PROGRAM, &current);
    if ((GLuint)current != program) glUseProgram(program);
    /* Only what differs from the tracked GL state is sent. */
    set_cap(GL_DEPTH_TEST, d->depth_test);
    if (d->depth_test) {
        GLint func = 0;
        gls_get_integerv(GL_DEPTH_FUNC, &func);
        if ((GLenum)func != gl_compare(d->depth_compare)) glDepthFunc(gl_compare(d->depth_compare));
    }
    GLboolean mask = GL_FALSE;
    gls_get_booleanv(GL_DEPTH_WRITEMASK, &mask);
    if (!mask != !d->depth_write) glDepthMask(d->depth_write ? GL_TRUE : GL_FALSE);
    set_cap(GL_BLEND, d->blend);
    if (d->blend) {
        GLint f[4];
        gls_get_integerv(GL_BLEND_SRC_RGB, &f[0]); gls_get_integerv(GL_BLEND_DST_RGB, &f[1]);
        gls_get_integerv(GL_BLEND_SRC_ALPHA, &f[2]); gls_get_integerv(GL_BLEND_DST_ALPHA, &f[3]);
        GLenum want[4] = { gl_blend(d->src_rgb), gl_blend(d->dst_rgb),
                           gl_blend(d->src_alpha), gl_blend(d->dst_alpha) };
        if ((GLenum)f[0] != want[0] || (GLenum)f[1] != want[1] ||
            (GLenum)f[2] != want[2] || (GLenum)f[3] != want[3])
            glBlendFuncSeparate(want[0], want[1], want[2], want[3]);
    }
    set_cap(GL_CULL_FACE, d->cull != RHI_CULL_NONE);
    if (d->cull != RHI_CULL_NONE) glCullFace(d->cull == RHI_CULL_BACK ? GL_BACK : GL_FRONT);
    GLboolean cm[4];
    gls_get_booleanv(GL_COLOR_WRITEMASK, cm);
    GLboolean want_cm[4] = { (d->color_write & 1) ? GL_TRUE : GL_FALSE, (d->color_write & 2) ? GL_TRUE : GL_FALSE,
                             (d->color_write & 4) ? GL_TRUE : GL_FALSE, (d->color_write & 8) ? GL_TRUE : GL_FALSE };
    if (memcmp(cm, want_cm, sizeof cm)) glColorMask(want_cm[0], want_cm[1], want_cm[2], want_cm[3]);
    /* Stencil functions are not tracked: set whenever the test is on, left as
       they are when it is off (like the blend factors). */
    set_cap(GL_STENCIL_TEST, d->stencil_test);
    if (d->stencil_test) {
        glStencilMask(d->stencil_write_mask);
        glStencilFunc(gl_compare(d->stencil_compare), d->stencil_ref, d->stencil_read_mask);
        glStencilOp(gl_stencil_op(d->stencil_fail), gl_stencil_op(d->stencil_depth_fail),
                    gl_stencil_op(d->stencil_pass));
    }
}

void rhi_cmd_bind_vertex_buffer(uint32_t stream, RhiBuffer buffer, uint32_t offset) {
    if (stream >= RHI_MAX_VERTEX_STREAMS) return;
    g_cmd.vertex[stream] = buffer;
    g_cmd.vertex_offset[stream] = offset;
}

void rhi_cmd_bind_index_buffer(RhiBuffer buffer) { g_cmd.index = buffer; }

void rhi_cmd_bind_texture(uint32_t slot, RhiTexture texture) {
    const RhiGlTexture *t = texture_lookup(texture, "bind_texture");
    if (t && !(t->usage & RHI_TEXTURE_SAMPLED)) t = NULL;   /* not sampleable */
    GLint active = 0;
    gls_get_integerv(GL_ACTIVE_TEXTURE, &active);
    if ((GLenum)active != GL_TEXTURE0 + slot) glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(t ? t->target : GL_TEXTURE_2D, t ? t->name : 0);
}

/* Vertex setup for a draw: per layout attribute (in layout order) its buffer,
 * enable and pointer -- the GL calls, in the same order, that the draw code
 * issued before rhi.h had draws -- then attributes the layout does not name
 * are switched off, so no stale array is read. */
static const RhiPipelineDesc *draw_setup(const char *what, uint32_t vertex_count, uint32_t first_vertex) {
    if (g_cmd.pipeline < 0) {
        fprintf(stderr, "rhi: %s: no pipeline bound\n", what);
        return NULL;
    }
    const RhiPipelineDesc *d = &g_pipelines.desc[g_cmd.pipeline];
    if (shader_slot(d->shader) < 0) {
        fprintf(stderr, "rhi: %s: destroyed shader\n", what);
        g_cmd.pipeline = -1;
        return NULL;
    }
    GLuint names[RHI_MAX_VERTEX_STREAMS] = {0};
    for (uint32_t i = 0; i < d->vertex.attrib_count; i++) {
        const RhiVertexAttrib *a = &d->vertex.attribs[i];
        if (!g_cmd.vertex[a->stream].id) continue; /* disabled constant attribute */
        const RhiGlBuffer *b = buffer_lookup(g_cmd.vertex[a->stream], what);
        uint32_t bytes = vertex_format_bytes(a->format);
        uint32_t stride = d->vertex.stride[a->stream] ? d->vertex.stride[a->stream] : bytes;
        uint64_t last = vertex_count ? (uint64_t)first_vertex + vertex_count - 1u : 0;
        uint64_t end = (uint64_t)g_cmd.vertex_offset[a->stream] + a->offset + last * stride + bytes;
        if (!b || b->target != GL_ARRAY_BUFFER || end > PTRDIFF_MAX || end > b->size) {
            fprintf(stderr, "rhi: %s: invalid vertex buffer or range\n", what);
            return NULL;
        }
        names[a->stream] = b->name;
    }
    unsigned used = 0;
    int bound_stream = -1;
    for (uint32_t i = 0; i < d->vertex.attrib_count; i++) {
        const RhiVertexAttrib *a = &d->vertex.attribs[i];
        used |= 1u << a->location;
        if (!names[a->stream]) {
            glDisableVertexAttribArray(a->location);
            continue;
        }
        if ((int)a->stream != bound_stream) {
            glBindBuffer(GL_ARRAY_BUFFER, names[a->stream]);
            bound_stream = (int)a->stream;
        }
        static const GLint comps[] = { 0, 2, 3, 4, 4, 1 };
        uintptr_t off = (uintptr_t)g_cmd.vertex_offset[a->stream] + a->offset;
        glEnableVertexAttribArray(a->location);
        glVertexAttribPointer(a->location, comps[a->format],
                              a->format == RHI_VERTEX_UNORM8X4 ? GL_UNSIGNED_BYTE : GL_FLOAT,
                              a->format == RHI_VERTEX_UNORM8X4 ? GL_TRUE : GL_FALSE,
                              (GLsizei)d->vertex.stride[a->stream], (const void *)off);
    }
    for (GLuint loc = 0; loc < RHI_MAX_VERTEX_ATTRIBS; loc++)
        if (!(used & (1u << loc)) && gls_attrib_enabled(loc)) glDisableVertexAttribArray(loc);
    return d;
}

static GLenum gl_primitive(RhiPrimitive p) { return p == RHI_PRIMITIVE_LINES ? GL_LINES : GL_TRIANGLES; }

void rhi_cmd_draw_indexed(uint32_t index_count, uint32_t first_index) {
    if (!index_count) return;
    const RhiGlBuffer *index = buffer_lookup(g_cmd.index, "draw_indexed");
    if (!index || index->target != GL_ELEMENT_ARRAY_BUFFER || index_count > INT_MAX ||
        (uint64_t)first_index * 2u > PTRDIFF_MAX || first_index > index->size / 2u ||
        index_count > index->size / 2u - first_index) {
        fprintf(stderr, "rhi: draw_indexed: invalid index buffer or range\n");
        return;
    }
    const RhiPipelineDesc *d = draw_setup("draw_indexed", 0, 0);
    if (!d) return;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index->name);
    rhi_gl_flush_params();
    glDrawElements(gl_primitive(d->primitive), (GLsizei)index_count, GL_UNSIGNED_SHORT,
                   (const void *)((uintptr_t)first_index * 2u));
}

void rhi_cmd_draw(uint32_t vertex_count, uint32_t first_vertex) {
    if (!vertex_count) return;
    if (vertex_count > INT_MAX || first_vertex > INT_MAX) {
        fprintf(stderr, "rhi: draw: invalid vertex count or first vertex\n");
        return;
    }
    const RhiPipelineDesc *d = draw_setup("draw", vertex_count, first_vertex);
    if (!d) return;
    rhi_gl_flush_params();
    glDrawArrays(gl_primitive(d->primitive), (GLint)first_vertex, (GLsizei)vertex_count);
}

/* ------------------------------------------------------------------------------ */
/* Passes                                                                         */
/* ------------------------------------------------------------------------------ */
/* One framebuffer object per depth attachment; the colour attachment is
 * re-pointed when a pass names a different one, as the offscreen code did by
 * hand before rhi.h had passes. */
#define RHI_GL_MAX_FRAMEBUFFERS 8u

static struct {
    GLuint     fbo;
    RhiTexture depth;
    GLuint     color_name;     /* attached colour texture, 0 = none */
    GLenum     color_target;   /* GL_TEXTURE_2D or a cube face */
    int        complete;       /* checked since the last attachment change */
} g_fbos[RHI_GL_MAX_FRAMEBUFFERS];
static int g_in_pass;

static void rhi_gl_forget_framebuffers(RhiTexture tex) {
    for (uint32_t i = 0; i < RHI_GL_MAX_FRAMEBUFFERS; i++) {
        if (!g_fbos[i].fbo) continue;
        const RhiGlTexture *t = texture_find(tex);
        if (g_fbos[i].depth.id == tex.id || (t && t->target != GL_RENDERBUFFER && g_fbos[i].color_name == t->name)) {
            glDeleteFramebuffers(1, &g_fbos[i].fbo);
            memset(&g_fbos[i], 0, sizeof g_fbos[i]);
        }
    }
}

RhiResult rhi_cmd_begin_pass(const RhiPassDesc *desc) {
    if (!desc || g_in_pass) return RHI_ERR_INVALID;
    const RhiGlTexture *color = texture_lookup(desc->color.texture, "begin_pass");
    const RhiGlTexture *depth = texture_lookup(desc->depth.texture, "begin_pass");
    if (!color || color->format != RHI_FORMAT_RGBA8 || !(color->usage & RHI_TEXTURE_COLOR_TARGET) ||
        (desc->depth.texture.id && (!depth || depth->target != GL_RENDERBUFFER))) return RHI_ERR_INVALID;
    int faces = color->target == GL_TEXTURE_CUBE_MAP ? 6 : 1;
    if (desc->color.face < 0 || desc->color.face >= faces ||
        (depth && desc->depth.face != 0) ||
        (desc->color_load != RHI_LOAD_KEEP && desc->color_load != RHI_LOAD_CLEAR) ||
        (desc->depth_load != RHI_LOAD_KEEP && desc->depth_load != RHI_LOAD_CLEAR)) return RHI_ERR_INVALID;
    GLenum color_target = faces == 6 ? (GLenum)(GL_TEXTURE_CUBE_MAP_POSITIVE_X + desc->color.face)
                                     : GL_TEXTURE_2D;
    int f = -1, free_slot = -1;
    for (uint32_t i = 0; i < RHI_GL_MAX_FRAMEBUFFERS; i++) {
        if (g_fbos[i].fbo && g_fbos[i].depth.id == desc->depth.texture.id) { f = (int)i; break; }
        if (!g_fbos[i].fbo && free_slot < 0) free_slot = (int)i;
    }
    if (f < 0) {
        if (free_slot < 0) { fprintf(stderr, "rhi: begin_pass: out of framebuffers\n"); return RHI_ERR_INVALID; }
        f = free_slot;
        glGenFramebuffers(1, &g_fbos[f].fbo);
        if (!g_fbos[f].fbo) return RHI_ERR_UNSUPPORTED;
        g_fbos[f].depth = desc->depth.texture;
        glBindFramebuffer(GL_FRAMEBUFFER, g_fbos[f].fbo);
        if (depth) glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth->name);
    } else glBindFramebuffer(GL_FRAMEBUFFER, g_fbos[f].fbo);
    if (g_fbos[f].color_name != color->name || g_fbos[f].color_target != color_target) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color_target, color->name, 0);
        g_fbos[f].color_name = color->name;
        g_fbos[f].color_target = color_target;
        g_fbos[f].complete = 0;
    }
    if (!g_fbos[f].complete) {
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) return RHI_ERR_UNSUPPORTED;
        g_fbos[f].complete = 1;
    }
    glViewport(0, 0, color->width, color->height);
    set_cap(GL_SCISSOR_TEST, 0);
    GLbitfield clear = 0;
    GLboolean cm[4] = {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE}, dm = GL_TRUE;
    if (desc->color_load == RHI_LOAD_CLEAR) {
        GLfloat cc[4];
        gls_get_booleanv(GL_COLOR_WRITEMASK, cm);
        if (!cm[0] || !cm[1] || !cm[2] || !cm[3]) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        gls_get_floatv(GL_COLOR_CLEAR_VALUE, cc);
        if (memcmp(cc, desc->clear_color, sizeof cc))
            glClearColor(desc->clear_color[0], desc->clear_color[1], desc->clear_color[2], desc->clear_color[3]);
        clear |= GL_COLOR_BUFFER_BIT;
    }
    if (desc->depth_load == RHI_LOAD_CLEAR && depth) {
        GLfloat cd;
        gls_get_booleanv(GL_DEPTH_WRITEMASK, &dm);
        if (!dm) glDepthMask(GL_TRUE);
        gls_get_floatv(GL_DEPTH_CLEAR_VALUE, &cd);
        if (cd != desc->clear_depth) {
#ifdef N2_GLES
            glClearDepthf(desc->clear_depth);
#else
            glClearDepth(desc->clear_depth);
#endif
        }
        clear |= GL_DEPTH_BUFFER_BIT;
    }
    if (clear) {
        glClear(clear);
        if (!cm[0] || !cm[1] || !cm[2] || !cm[3]) glColorMask(cm[0], cm[1], cm[2], cm[3]);
        if (!dm) glDepthMask(GL_FALSE);
    }
    g_in_pass = 1;
    return RHI_OK;
}

void rhi_cmd_end_pass(void) { g_in_pass = 0; }
