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
 * GL 2.1 / ES 2.0 have no sampler objects, so a texture's sampler is applied
 * as texture parameters (only the ones that change). Until texture binding is
 * part of rhi.h, rhi_texture_create, rhi_texture_upload,
 * rhi_texture_generate_mips and rhi_texture_set_sampler leave the texture
 * bound on the active texture unit, as the direct GL code did. */
GLuint rhi_gl_texture_name(RhiTexture tex);

#ifdef __cplusplus
}
#endif
#endif /* RHI_GL_H */
