/* rhi.h - backend-neutral rendering interface (issue #14).
 *
 * Rules for this header:
 *   - C99, depends on <stdint.h>/<stddef.h> only: no SDL, GL or Vulkan types.
 *     The window is passed as void*.
 *   - Nothing game-specific: no scene or gameplay concepts, no shader or uniform
 *     names.
 *   - Single thread, one device per process.
 *
 * Covered so far: device/context lifetime, capabilities, the frame begin/end and
 * vertex/index buffers. Textures, samplers, shaders, pipelines, passes, draws
 * and readback are added by later steps; until then the game still issues GL
 * calls for them directly. Backend: rhi_gl.c (desktop GL 2.1 or OpenGL ES 2.0, chosen
 * at build time). */
#ifndef RHI_H
#define RHI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Result codes. */
typedef enum RhiResult {
    RHI_OK = 0,
    RHI_ERR_UNSUPPORTED = -1,   /* backend not compiled in or not available */
    RHI_ERR_INVALID = -2,       /* bad argument or wrong call order */
    RHI_ERR_DEVICE_LOST = -4,
    RHI_ERR_SKIP_FRAME = -5     /* nothing can be presented this frame: skip drawing */
} RhiResult;

/* ------------------------------------------------------------------------------ */
/* Device / context lifetime                                                      */
/* ------------------------------------------------------------------------------ */
typedef enum RhiBackend {
    RHI_BACKEND_DEFAULT = 0, /* the backend this build was compiled for */
    RHI_BACKEND_GL,          /* desktop GL 2.1 compatibility (macOS: legacy 2.1 context) */
    RHI_BACKEND_GLES2        /* OpenGL ES 2.0 */
} RhiBackend;

typedef enum RhiDeviceFlags {
    RHI_DEVICE_VSYNC = 1u << 0  /* present synchronised to the display refresh */
} RhiDeviceFlags;

typedef struct RhiDeviceDesc {
    RhiBackend backend;
    uint32_t   flags;      /* RhiDeviceFlags */
    void      *window;     /* the SDL_Window, created with rhi_window_flags() */
} RhiDeviceDesc;

/* Window setup happens before the window exists, so it is split from device
 * creation: rhi_prepare_window() sets what the backend needs on the windowing
 * system (GL: context attributes) and rhi_window_flags() returns the
 * SDL_WindowFlags bits to create the window with.
 * RHI_ERR_UNSUPPORTED / 0 if the backend is not compiled in. */
RhiResult rhi_prepare_window(RhiBackend backend);
uint32_t  rhi_window_flags(RhiBackend backend);

/* Creates the device for desc->window (GL: the context, made current) and fills
 * the capabilities. Logs the reason to stderr on failure. */
RhiResult rhi_device_create(const RhiDeviceDesc *desc);
/* Destroys the device. The window stays owned by the caller. */
void      rhi_device_destroy(void);

/* Size of the main target in pixels (the window's drawable size, which can
 * differ from the window size on high-DPI displays). */
void      rhi_drawable_size(int32_t *width, int32_t *height);

/* ------------------------------------------------------------------------------ */
/* Capabilities                                                                   */
/* ------------------------------------------------------------------------------ */
typedef struct RhiCaps {
    RhiBackend backend;          /* never RHI_BACKEND_DEFAULT */
    int32_t    max_viewport[2];  /* largest main target / viewport, pixels */
    float      max_anisotropy;   /* 1.0 = anisotropic filtering unavailable */
    uint8_t    tex_bc1, tex_bc2, tex_bc3;  /* S3TC DXT1/3/5 uploads supported */
} RhiCaps;

const RhiCaps *rhi_caps(void);   /* valid after rhi_device_create */

/* ------------------------------------------------------------------------------ */
/* Frame lifecycle                                                                */
/* ------------------------------------------------------------------------------ */
typedef struct RhiFrameInfo {
    int32_t  width, height;      /* main target size for this frame */
    uint64_t frame_index;        /* 0 for the first frame */
} RhiFrameInfo;

/* Begins a frame. RHI_ERR_SKIP_FRAME: draw nothing and do not call
 * rhi_frame_end for it (the GL backend never returns it). */
RhiResult rhi_frame_begin(RhiFrameInfo *out);
/* Ends the frame and presents it (GL: SDL_GL_SwapWindow). */
RhiResult rhi_frame_end(void);

/* ------------------------------------------------------------------------------ */
/* Buffers                                                                        */
/* ------------------------------------------------------------------------------ */
/* Opaque handle: slot index + generation, id 0 = no buffer. A destroyed handle
 * is detected (and logged) instead of reaching the GPU API. */
typedef struct RhiBuffer { uint32_t id; } RhiBuffer;

typedef enum RhiBufferUsage {
    RHI_BUFFER_VERTEX = 1u << 0,
    RHI_BUFFER_INDEX  = 1u << 1   /* 16-bit indices */
} RhiBufferUsage;

typedef enum RhiBufferUpdate {
    RHI_UPDATE_STATIC = 0,  /* written at creation; rare partial updates between
                               draws (rhi_buffer_update) */
    RHI_UPDATE_STREAM = 1   /* contents replaced every frame with
                               rhi_buffer_replace; draws issued before a replace
                               keep using the old contents */
} RhiBufferUpdate;

typedef struct RhiBufferDesc {
    uint32_t        usage;  /* RhiBufferUsage: exactly one bit */
    RhiBufferUpdate update;
    size_t          size;   /* bytes; STREAM may be 0 = no storage until the first
                               rhi_buffer_replace */
    const void     *data;   /* size bytes, or NULL = undefined contents */
} RhiBufferDesc;

/* Returns {0} on failure (logged to stderr). */
RhiBuffer rhi_buffer_create(const RhiBufferDesc *desc);
/* Writes size bytes at offset into the existing storage; the range must lie
 * inside the buffer. */
RhiResult rhi_buffer_update(RhiBuffer buf, size_t offset, size_t size, const void *data);
/* Gives the buffer fresh storage of size bytes (data may be NULL = undefined
 * contents). Earlier draws are unaffected (GL: buffer orphaning). */
RhiResult rhi_buffer_replace(RhiBuffer buf, size_t size, const void *data);
/* Releases the buffer; {0} is ignored. The handle becomes invalid. */
void      rhi_buffer_destroy(RhiBuffer buf);

#ifdef __cplusplus
}
#endif
#endif /* RHI_H */
