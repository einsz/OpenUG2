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

#ifdef __cplusplus
}
#endif
#endif /* RHI_GL_H */
