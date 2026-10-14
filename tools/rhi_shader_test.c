/* Native shader handles must never identify a replacement after destruction. */
#include "rhi_gl_state.h"
#include "rhi_gl.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    assert(SDL_Init(SDL_INIT_VIDEO) == 0);
    SDL_Window *window = SDL_CreateWindow("rhi-shader-test", 0, 0, 16, 16,
                                         SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    assert(window);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    assert(context);
    const RhiShaderDesc desc = {
        .vertex_source = "void main(){gl_Position=vec4(0.0);}",
        .fragment_source = "void main(){gl_FragColor=vec4(1.0);}",
        .label = "handle test"
    };
    assert(!rhi_gl_shader_program((RhiShader){0}));
    RhiShader stale = rhi_shader_create(&desc);
    assert(stale.id);
    rhi_shader_destroy(stale);
    /* Cross the 8-bit generation boundary twice, including stale destroy. */
    for (int i = 0; i < 512; i++) {
        RhiShader live = rhi_shader_create(&desc);
        assert(live.id && rhi_gl_shader_program(live));
        assert(!rhi_gl_shader_program(stale));
        rhi_shader_destroy(stale);
        assert(rhi_gl_shader_program(live));
        rhi_shader_destroy(live);
        assert(!rhi_gl_shader_program(live));
    }
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    puts("rhi_shader_test: PASS (retired generations, stale lookup/destroy)");
    return 0;
}
