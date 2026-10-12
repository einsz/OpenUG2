/* gl_state.h — CPU-side GL state tracker.
 *
 * Renderer-boundary migration step 2 (issue #14). Helpers used to
 * save/restore the caller's state by reading it back from GL (glIsEnabled,
 * glGetIntegerv, glGetBooleanv, glGetFloatv). A Vulkan backend has no
 * equivalent, so the state is shadowed on the CPU instead:
 *
 *  - Each state-setting entry point below is redirected by a function-like
 *    macro to a gls* shim that records the value and then issues exactly the
 *    same GL call. The GL command stream (calls, order, values) is unchanged;
 *    only the queries disappear.
 *  - gls_get_* answer from the shadow copy, which starts from the GL spec
 *    defaults whenever the current SDL GL context changes. They never query
 *    GL, except for a pname the tracker does not model (counted below).
 *
 * Every TU that includes render.h (or this header) is tracked. Code that
 * changes this state through raw GL (the ImGui GL2 backend) must restore it
 * before returning, as it already does.
 *
 * Build with -DGLS_VERIFY to cross-check every getter against the real GL
 * query and abort on the first mismatch. */
#ifndef OPENUG2_GL_STATE_H
#define OPENUG2_GL_STATE_H

#include <SDL.h>
#ifdef N2_GLES
#  include <SDL_opengles2.h>
#elif defined(__APPLE__)
#  ifndef GL_SILENCE_DEPRECATION
#    define GL_SILENCE_DEPRECATION 1
#  endif
#  include <OpenGL/gl.h>
#  include <OpenGL/glext.h>
#else
#  ifndef GL_GLEXT_PROTOTYPES
#    define GL_GLEXT_PROTOTYPES 1
#  endif
#  include <SDL_opengl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ---- recording shims (reached through the macros below) ---- */
SDL_GLContext glsCreateContext(SDL_Window *window);   /* resets the tracker */
void glsDeleteContext(SDL_GLContext ctx);
void glsEnable(GLenum cap);
void glsDisable(GLenum cap);
void glsBlendFunc(GLenum src, GLenum dst);
void glsBlendFuncSeparate(GLenum src_rgb, GLenum dst_rgb, GLenum src_a, GLenum dst_a);
void glsDepthFunc(GLenum func);
void glsDepthMask(GLboolean mask);
void glsColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a);
void glsClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
#ifdef N2_GLES
void glsClearDepthf(GLfloat depth);
#else
void glsClearDepth(GLdouble depth);
#endif
void glsViewport(GLint x, GLint y, GLsizei w, GLsizei h);
void glsUseProgram(GLuint program);
void glsBindFramebuffer(GLenum target, GLuint fbo);
void glsBindRenderbuffer(GLenum target, GLuint rb);
void glsDeleteFramebuffers(GLsizei n, const GLuint *ids);
void glsDeleteRenderbuffers(GLsizei n, const GLuint *ids);
void glsActiveTexture(GLenum unit);
void glsBindTexture(GLenum target, GLuint texture);
void glsDeleteTextures(GLsizei n, const GLuint *ids);

/* ---- tracked-state getters: drop-in for the glIsEnabled / glGet* forms ----
 * Modelled caps: DEPTH_TEST BLEND CULL_FACE SCISSOR_TEST STENCIL_TEST DITHER
 * POLYGON_OFFSET_FILL. Modelled pnames: BLEND_{SRC,DST}[_RGB|_ALPHA],
 * DEPTH_FUNC, VIEWPORT, CURRENT_PROGRAM, FRAMEBUFFER_BINDING,
 * RENDERBUFFER_BINDING, ACTIVE_TEXTURE, TEXTURE_BINDING_2D (active unit),
 * DEPTH_WRITEMASK, COLOR_WRITEMASK, COLOR_CLEAR_VALUE,
 * DEPTH_CLEAR_VALUE. */
GLboolean gls_is_enabled(GLenum cap);
void gls_get_integerv(GLenum pname, GLint *out);
void gls_get_booleanv(GLenum pname, GLboolean *out);
void gls_get_floatv(GLenum pname, GLfloat *out);

/* Reads that had to go to GL because the pname/cap/location is not modelled
 * (diagnostic; 0 in every converted path). */
unsigned gls_fallback_queries(void);

#ifdef __cplusplus
}
#endif

#ifndef GLS_IMPLEMENTATION
#  define SDL_GL_CreateContext(w)         glsCreateContext(w)
#  define SDL_GL_DeleteContext(c)         glsDeleteContext(c)
#  define glEnable(c)                     glsEnable(c)
#  define glDisable(c)                    glsDisable(c)
#  define glBlendFunc(s,d)                glsBlendFunc(s,d)
#  define glBlendFuncSeparate(a,b,c,d)    glsBlendFuncSeparate(a,b,c,d)
#  define glDepthFunc(f)                  glsDepthFunc(f)
#  define glDepthMask(m)                  glsDepthMask(m)
#  define glColorMask(r,g,b,a)            glsColorMask(r,g,b,a)
#  define glClearColor(r,g,b,a)           glsClearColor(r,g,b,a)
#  ifdef N2_GLES
#    define glClearDepthf(d)              glsClearDepthf(d)
#  else
#    define glClearDepth(d)               glsClearDepth(d)
#  endif
#  define glViewport(x,y,w,h)             glsViewport(x,y,w,h)
#  define glUseProgram(p)                 glsUseProgram(p)
#  define glBindFramebuffer(t,f)          glsBindFramebuffer(t,f)
#  define glBindRenderbuffer(t,r)         glsBindRenderbuffer(t,r)
#  define glDeleteFramebuffers(n,p)       glsDeleteFramebuffers(n,p)
#  define glDeleteRenderbuffers(n,p)      glsDeleteRenderbuffers(n,p)
#  define glActiveTexture(u)              glsActiveTexture(u)
#  define glBindTexture(t,x)              glsBindTexture(t,x)
#  define glDeleteTextures(n,p)           glsDeleteTextures(n,p)
#endif

#endif /* OPENUG2_GL_STATE_H */
