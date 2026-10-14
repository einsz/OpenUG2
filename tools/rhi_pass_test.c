/* Include the backend to inject a zero-name framebuffer allocation. */
#include "rhi_gl.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int fail_fbo;
static void test_gen_framebuffers(GLsizei n, GLuint *names) {
    if(fail_fbo)memset(names,0,(size_t)n*sizeof *names);
#if defined(__APPLE__) && !defined(N2_GLES)
    else glGenFramebuffersEXT(n,names);
#else
    else glGenFramebuffers(n,names);
#endif
}
#if defined(__APPLE__) && !defined(N2_GLES)
#define glGenFramebuffersEXT test_gen_framebuffers
#else
#define glGenFramebuffers test_gen_framebuffers
#endif
#include "../src/rhi_gl.c"
#if defined(__APPLE__) && !defined(N2_GLES)
#undef glGenFramebuffersEXT
#else
#undef glGenFramebuffers
#endif
int main(void) {
    assert(SDL_Init(SDL_INIT_VIDEO)==0);
    SDL_Window *w=SDL_CreateWindow("rhi pass",0,0,32,32,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);assert(w);
    SDL_GLContext ctx=SDL_GL_CreateContext(w);assert(ctx);
    RhiTexture color=rhi_texture_create(&(RhiTextureDesc){RHI_TEXTURE_2D,RHI_FORMAT_RGBA8,4,4,RHI_TEXTURE_SAMPLED|RHI_TEXTURE_COLOR_TARGET});assert(color.id);
    assert(rhi_texture_upload(color,0,0,NULL,0)==RHI_OK);
    RhiTexture a=rhi_texture_create(&(RhiTextureDesc){RHI_TEXTURE_2D,RHI_FORMAT_D16,4,4,RHI_TEXTURE_DEPTH_TARGET});assert(a.id);
    RhiTexture b=rhi_texture_create(&(RhiTextureDesc){RHI_TEXTURE_2D,RHI_FORMAT_D16,4,4,RHI_TEXTURE_DEPTH_TARGET});assert(b.id);
    /* Texture and renderbuffer names can coincide in separate GL namespaces. */
    RhiPassDesc pass={{color,0},{b,0},RHI_LOAD_CLEAR,RHI_LOAD_CLEAR,{1,0,0,1},1};
    glColorMask(0,1,0,1);glDepthMask(GL_FALSE);
    assert(rhi_cmd_begin_pass(&pass)==RHI_OK);
    GLboolean mask[4],dm;glGetBooleanv(GL_COLOR_WRITEMASK,mask);glGetBooleanv(GL_DEPTH_WRITEMASK,&dm);
    assert(!mask[0] && mask[1] && !mask[2] && mask[3] && !dm);
    unsigned char cleared[4];glReadPixels(0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,cleared);
    assert(cleared[0]==255 && !cleared[1] && !cleared[2] && cleared[3]==255);
    float cleared_depth=0;glReadPixels(0,0,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&cleared_depth);assert(cleared_depth>.999f);
    glColorMask(1,1,1,1);glDepthMask(GL_TRUE);
    assert(rhi_cmd_begin_pass(&pass)==RHI_ERR_INVALID); /* No nesting. */
    GLint before,after;glGetIntegerv(GL_FRAMEBUFFER_BINDING,&before);assert(before);rhi_cmd_end_pass();
    rhi_texture_destroy(a);glGetIntegerv(GL_FRAMEBUFFER_BINDING,&after);assert(after==before);
    pass.color_load=(RhiLoadOp)2;assert(rhi_cmd_begin_pass(&pass)==RHI_ERR_INVALID);
    pass.color_load=RHI_LOAD_KEEP;pass.depth.face=1;assert(rhi_cmd_begin_pass(&pass)==RHI_ERR_INVALID);
    rhi_texture_destroy(b);rhi_texture_destroy(color);
    RhiTexture sampled=rhi_texture_create(&(RhiTextureDesc){RHI_TEXTURE_2D,RHI_FORMAT_RGBA8,4,4,RHI_TEXTURE_SAMPLED});assert(sampled.id);
    assert(rhi_texture_upload(sampled,0,0,NULL,0)==RHI_OK);
    pass=(RhiPassDesc){{sampled,0},{{0},0},RHI_LOAD_CLEAR,RHI_LOAD_KEEP,{1,0,0,1},1};
    assert(rhi_cmd_begin_pass(&pass)==RHI_ERR_INVALID);rhi_texture_destroy(sampled);
    color=rhi_texture_create(&(RhiTextureDesc){RHI_TEXTURE_2D,RHI_FORMAT_RGBA8,4,4,RHI_TEXTURE_COLOR_TARGET});assert(color.id);
    assert(rhi_texture_upload(color,0,0,NULL,0)==RHI_OK);
    rhi_cmd_bind_texture(0,color);GLint bound;glGetIntegerv(GL_TEXTURE_BINDING_2D,&bound);assert(!bound);
    glClearColor(0,1,0,1);glClear(GL_COLOR_BUFFER_BIT);fail_fbo=1;
    pass=(RhiPassDesc){{color,0},{{0},0},RHI_LOAD_CLEAR,RHI_LOAD_KEEP,{1,0,0,1},1};
    assert(rhi_cmd_begin_pass(&pass)==RHI_ERR_UNSUPPORTED);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING,&bound);assert(!bound);
    unsigned char pixel[4];glReadPixels(0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);assert(!pixel[0] && pixel[1]==255);
    fail_fbo=0;assert(rhi_cmd_begin_pass(&pass)==RHI_OK);rhi_cmd_end_pass();rhi_texture_destroy(color);
    assert(glGetError()==GL_NO_ERROR);
    SDL_GL_DeleteContext(ctx);SDL_DestroyWindow(w);SDL_Quit();puts("rhi_pass_test: PASS (ownership, roles, validation, allocation rollback and recovery)");
}
