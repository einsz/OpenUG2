/* ugui_gl.h -- OpenGL backend: renders a UgCanvas. GL 2.1 / GLES2 compatible
 * (one shader, one vertex buffer, the built-in 8-bit font atlas, plus library textures). */
#ifndef UGUI_GL_H
#define UGUI_GL_H

#include "ugui_canvas.h"
#include "ugui_tex.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UgGL UgGL;

UgGL *ug_gl_create(void);                 /* needs a current GL context */
void  ug_gl_destroy(UgGL *g);
/* Draws the canvas into the window's letterboxed 4:3 area. The caller clears
 * the window and swaps buffers. */
void  ug_gl_render(UgGL *g, const UgCanvas *cv, int win_w, int win_h);
/* Uniform exact 640x480 transform over an existing full-viewport scene.
 * Avoid integer viewport rounding changing the UI aspect ratio. */
void  ug_gl_render_overlay(UgGL *g, const UgCanvas *cv, int win_w, int win_h);
/* Library whose textures UG_M_IMAGE quads refer to (uploaded on first use,
 * decoded lazily). The library must outlive the backend. */
void  ug_gl_set_texlib(UgGL *g, UgTexLib *lib);
/* Drains glGetError, prints each code with `tag`, returns how many. */
int   ug_gl_errors(const char *tag);

#ifdef __cplusplus
}
#endif
#endif
