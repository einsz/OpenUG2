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

/* GL program object behind a shader; 0 for {0} or a destroyed handle. Until
 * parameter blocks and pipelines are part of rhi.h, draw code looks up uniform
 * locations and calls glUseProgram on it. */
GLuint rhi_gl_shader_program(RhiShader shader);

/* Uploads the parameter fields of the current program's shader that changed
 * since its last upload (glUniform* only for those). Draw code calls it right
 * before each glDraw* until draws go through rhi.h. */
void rhi_gl_flush_params(void);

/* Pipelines are applied through the state tracker, field by field, every time
 * they are bound, so code that still changes GL state directly stays correct
 * around them. With blending (or the depth test) off, the blend factors (or
 * the depth function) are left as they are: they change no pixels, and code
 * that still enables blending directly may rely on the factors set before. */

#ifdef __cplusplus
}
#endif
#endif /* RHI_GL_H */
