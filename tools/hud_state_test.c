#include "render.h"
#include "rhi_gl.h"
#include "hud_baseline.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    assert(SDL_Init(SDL_INIT_VIDEO)==0);
    SDL_Window *w=SDL_CreateWindow("hud state",0,0,32,32,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);assert(w);
    SDL_GLContext c=SDL_GL_CreateContext(w);assert(c);
    RProg r=render_program();assert(r.prog);GpuMesh q=make_quad();assert(q.vbo.id);
    HudBaseline *h=hud_baseline_init("build/hud-state-no-assets");assert(h);
    glEnable(GL_CULL_FACE);glEnable(GL_STENCIL_TEST);glColorMask(0,1,0,1);
    HudState state={0};hud_baseline_draw(h,&r,&q,&state,32,32);
    GLboolean mask[4];glGetBooleanv(GL_COLOR_WRITEMASK,mask);
    printf("HUD exit: cull=%d stencil=%d mask=%d%d%d%d\n",glIsEnabled(GL_CULL_FACE),glIsEnabled(GL_STENCIL_TEST),mask[0],mask[1],mask[2],mask[3]);fflush(stdout);
    assert(glIsEnabled(GL_CULL_FACE) && glIsEnabled(GL_STENCIL_TEST) && !mask[0] && mask[1] && !mask[2] && mask[3]);
    hud_baseline_free(&h);rhi_buffer_destroy(q.vbo);rhi_buffer_destroy(q.nbo);rhi_buffer_destroy(q.ibo);rhi_shader_destroy(r.shader);
    SDL_GL_DeleteContext(c);SDL_DestroyWindow(w);SDL_Quit();puts("HUD state PASS");
}
