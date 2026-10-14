/* rhi_gl.h - GL backend internals shared with code that still issues GL calls
 * directly during the migration to rhi.h. Not part of the backend-neutral
 * interface: only GL-specific code may include it. */
#ifndef RHI_GL_H
#define RHI_GL_H

#include "rhi.h"
#include "rhi_gl_state.h"   /* GL headers */

#ifdef __cplusplus
extern "C" {
#endif

/* GL buffer object name behind a handle; 0 for {0} or a destroyed handle. */
GLuint rhi_gl_buffer_name(RhiBuffer buf);

/* GL texture object name behind a handle; 0 for {0} or a destroyed handle.
 * Until samplers and texture binding are part of rhi.h, rhi_texture_create,
 * rhi_texture_upload and rhi_texture_generate_mips leave the texture bound on
 * the active texture unit, so the caller's glTexParameter* calls apply to it. */
GLuint rhi_gl_texture_name(RhiTexture tex);

#ifdef __cplusplus
}
#endif
#endif /* RHI_GL_H */
