/* hud_caller_state_test -- hud_draw ("Restores every GL state and uniform it touches", hud.h) must hand the caller's vertex-attribute
 * descriptions and element-buffer binding back unchanged. Asset-free: hud_test_white_widget() (compiled with -DHUD_TEST_HOOKS) builds a
 * one-node authored package, so the real hud_draw runs. The caller configures attribute 0 as 3 floats, stride 12, offset 12 in its own buffer,
 * enables attribute 5, and binds an element buffer; all of it must survive the call. */
#include "render.h"
#include "rhi_gl.h"
#include "hud.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

static GLint geti(GLenum p) { GLint v = -1; glGetIntegerv(p, &v); return v; }
static GLint attri(GLuint i, GLenum p) { GLint v = -1; glGetVertexAttribiv(i, p, &v); return v; }
int main(void) {
    assert(SDL_Init(SDL_INIT_VIDEO) == 0);
    SDL_Window *w = SDL_CreateWindow("hud caller state", 0, 0, 128, 128, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN); assert(w);
    SDL_GLContext c = SDL_GL_CreateContext(w); assert(c);
    RProg r = render_program(); assert(r.prog);
    Hud *h = hud_test_white_widget(); assert(h);

    GLuint buf[2]; glGenBuffers(2, buf);
    glBindBuffer(GL_ARRAY_BUFFER, buf[0]); glBufferData(GL_ARRAY_BUFFER, 256, NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf[1]); glBufferData(GL_ELEMENT_ARRAY_BUFFER, 64, NULL, GL_STATIC_DRAW);
    glEnableVertexAttribArray(5);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, (const void *)12);
    const GLint want_array = (GLint)buf[0], want_elem = (GLint)buf[1];
    assert(glGetError() == GL_NO_ERROR);

    glViewport(0, 0, 128, 128); glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT);
    HudState s = {0}; s.career_context = 1; s.hide_map = 1; s.hide_gauge = 1;
    hud_draw(h, NULL, NULL, &s, 128, 128);

    unsigned char centre[4] = {0}; glReadPixels(64, 64, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, centre);
    const GLint a0size = attri(0, GL_VERTEX_ATTRIB_ARRAY_SIZE), a0buf = attri(0, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING);
    const GLint a0stride = attri(0, GL_VERTEX_ATTRIB_ARRAY_STRIDE), a5 = attri(5, GL_VERTEX_ATTRIB_ARRAY_ENABLED);
    void *a0ptr = NULL; glGetVertexAttribPointerv(0, GL_VERTEX_ATTRIB_ARRAY_POINTER, &a0ptr);
    const GLint array_after = geti(GL_ARRAY_BUFFER_BINDING), elem_after = geti(GL_ELEMENT_ARRAY_BUFFER_BINDING);
    const GLenum err = glGetError();
    printf("hud_draw drew the widget: centre pixel %u,%u,%u (expect 255,255,255)\n", centre[0], centre[1], centre[2]);
    printf("ARRAY_BUFFER:         %d -> %d\n", want_array, array_after);
    printf("attribute 5 enabled:  1 -> %d\n", a5);
    printf("attribute 0 size:     3 -> %d\n", a0size);
    printf("attribute 0 stride:   12 -> %d\n", a0stride);
    printf("attribute 0 offset:   12 -> %lu\n", (unsigned long)(uintptr_t)a0ptr);
    printf("attribute 0 buffer:   %d -> %d\n", want_array, a0buf);
    printf("ELEMENT_ARRAY_BUFFER: %d -> %d\n", want_elem, elem_after);
    printf("GL error: 0x%x\n", err);
    int ok = centre[0] == 255 && centre[1] == 255 && centre[2] == 255 && array_after == want_array && a5 == 1 && a0size == 3 &&
             a0stride == 12 && (uintptr_t)a0ptr == 12 && a0buf == want_array && elem_after == want_elem && err == GL_NO_ERROR;
    hud_free(&h); rhi_shader_destroy(r.shader);
    SDL_GL_DeleteContext(c); SDL_DestroyWindow(w); SDL_Quit();
    puts(ok ? "HUD caller state PASS" : "HUD caller state FAIL");
    return ok ? 0 : 1;
}
