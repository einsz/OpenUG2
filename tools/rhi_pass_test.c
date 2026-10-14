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
static void copy_tests(SDL_Window *window) {
    glClearColor(1,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
    RhiTextureDesc d={RHI_TEXTURE_2D,RHI_FORMAT_RGBA8,4,4,RHI_TEXTURE_SAMPLED|RHI_TEXTURE_COPY_DST};
    for(int i=0;i<2;i++) {
        RhiTexture copy=rhi_texture_create(&d);assert(copy.id);
        assert(rhi_cmd_copy_main_to_texture(copy,0,0,4,4)==RHI_OK);
        unsigned char pixel[64];glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,pixel);assert(pixel[0]==255 && !pixel[2]);
        rhi_texture_destroy(copy); /* Reuse must allocate new GL storage. */
    }
    RhiTexture copy=rhi_texture_create(&d);assert(copy.id);
    int width,height;SDL_GL_GetDrawableSize(window,&width,&height);
    assert(rhi_cmd_copy_main_to_texture(copy,-1,0,4,4)==RHI_ERR_INVALID);
    assert(rhi_cmd_copy_main_to_texture(copy,0,0,width+1,height)==RHI_ERR_INVALID);
    d.usage=RHI_TEXTURE_SAMPLED;RhiTexture sampled=rhi_texture_create(&d);assert(sampled.id);
    assert(rhi_cmd_copy_main_to_texture(sampled,0,0,4,4)==RHI_ERR_INVALID);rhi_texture_destroy(sampled);
    d.usage=RHI_TEXTURE_SAMPLED|RHI_TEXTURE_COLOR_TARGET|RHI_TEXTURE_COPY_DST;
    RhiTexture target=rhi_texture_create(&d);assert(target.id && rhi_texture_upload(target,0,0,NULL,0)==RHI_OK);
    RhiTexture depth=rhi_texture_create(&(RhiTextureDesc){RHI_TEXTURE_2D,RHI_FORMAT_D16,4,4,RHI_TEXTURE_DEPTH_TARGET});assert(depth.id);
    RhiPassDesc pass={{target,0},{depth,0},RHI_LOAD_CLEAR,RHI_LOAD_CLEAR,{0,0,1,1},1};
    assert(rhi_cmd_begin_pass(&pass)==RHI_OK);
    assert(rhi_cmd_copy_main_to_texture(copy,0,0,4,4)==RHI_ERR_INVALID);
    rhi_cmd_end_pass();GLint before,after;glGetIntegerv(GL_FRAMEBUFFER_BINDING,&before);assert(before);
    assert(rhi_cmd_copy_main_to_texture(copy,0,0,4,4)==RHI_OK);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING,&after);assert(after==before);
    unsigned char pixel[64];glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,pixel);assert(pixel[0]==255 && !pixel[2]);
    assert(rhi_cmd_copy_main_to_texture(target,0,0,8,8)==RHI_OK);
    RhiResult result=rhi_cmd_begin_pass(&pass);
    GLenum status=glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status!=GL_FRAMEBUFFER_COMPLETE)assert(result!=RHI_OK); /* Resize invalidates cached completeness. */
    else assert(result==RHI_OK);
    if(result==RHI_OK)rhi_cmd_end_pass();
    rhi_texture_destroy(depth);rhi_texture_destroy(target);rhi_texture_destroy(copy);
    d.format=RHI_FORMAT_DEPTH;d.usage=RHI_TEXTURE_SAMPLED|RHI_TEXTURE_COPY_DST;
    RhiTexture depth_copy=rhi_texture_create(&d);assert(depth_copy.id);
    assert(rhi_texture_generate_mips(depth_copy)==RHI_ERR_INVALID);rhi_texture_destroy(depth_copy);
    assert(glGetError()==GL_NO_ERROR);
}

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
    /* Frame copies: COPY_DST only, outside passes, inside the frame; the
       texture takes the rectangle's size. */
    RhiTexture copy=rhi_texture_create(&(RhiTextureDesc){RHI_TEXTURE_2D,RHI_FORMAT_RGBA8,4,4,RHI_TEXTURE_SAMPLED|RHI_TEXTURE_COPY_DST});assert(copy.id);
    RhiTexture plain=rhi_texture_create(&(RhiTextureDesc){RHI_TEXTURE_2D,RHI_FORMAT_RGBA8,4,4,RHI_TEXTURE_SAMPLED});assert(plain.id);
    assert(rhi_cmd_copy_main_to_texture(plain,0,0,4,4)==RHI_ERR_INVALID);
    assert(rhi_cmd_copy_main_to_texture(copy,-1,0,4,4)==RHI_ERR_INVALID);
    assert(rhi_cmd_copy_main_to_texture(copy,1,0,INT_MAX,4)==RHI_ERR_INVALID);
    assert(rhi_cmd_copy_main_to_texture(copy,0,0,0,4)==RHI_ERR_INVALID);
    g_rhi.in_frame=1;g_rhi.frame_w=g_rhi.frame_h=32;
    assert(rhi_cmd_copy_main_to_texture(copy,30,0,4,4)==RHI_ERR_INVALID);
    assert(rhi_cmd_copy_main_to_texture(copy,0,0,8,8)==RHI_OK);
    g_rhi.in_frame=0;
    GLint copied_w=0;glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&copied_w);
    assert(copied_w==8 && rhi_cmd_copy_main_to_texture(copy,2,2,8,8)==RHI_OK);
    color=rhi_texture_create(&(RhiTextureDesc){RHI_TEXTURE_2D,RHI_FORMAT_RGBA8,4,4,RHI_TEXTURE_COLOR_TARGET});assert(color.id);
    assert(rhi_texture_upload(color,0,0,NULL,0)==RHI_OK);
    pass=(RhiPassDesc){{color,0},{{0},0},RHI_LOAD_KEEP,RHI_LOAD_KEEP,{0,0,0,0},1};
    assert(rhi_cmd_begin_pass(&pass)==RHI_OK);
    assert(rhi_cmd_copy_main_to_texture(copy,0,0,4,4)==RHI_ERR_INVALID);
    rhi_cmd_end_pass();glBindFramebuffer(GL_FRAMEBUFFER,0);
    RhiTexture depth=rhi_texture_create(&(RhiTextureDesc){RHI_TEXTURE_2D,RHI_FORMAT_DEPTH,4,4,RHI_TEXTURE_SAMPLED|RHI_TEXTURE_COPY_DST});assert(depth.id);
#ifdef N2_GLES
    assert(rhi_cmd_copy_main_to_texture(depth,0,0,4,4)==RHI_ERR_UNSUPPORTED);
#else
    assert(rhi_cmd_copy_main_to_texture(depth,0,0,4,4)==RHI_OK);
#endif
    rhi_texture_destroy(depth);rhi_texture_destroy(color);rhi_texture_destroy(plain);rhi_texture_destroy(copy);
    copy_tests(w);
    /* Wireframe is a debug command gated by caps. */
    g_rhi.caps.wireframe=0;assert(rhi_cmd_set_wireframe(1)==RHI_ERR_UNSUPPORTED);
#ifndef N2_GLES
    g_rhi.caps.wireframe=1;GLint mode[2];
    assert(rhi_cmd_set_wireframe(1)==RHI_OK);glGetIntegerv(GL_POLYGON_MODE,mode);assert(mode[0]==GL_LINE);
    assert(rhi_cmd_set_wireframe(0)==RHI_OK);glGetIntegerv(GL_POLYGON_MODE,mode);assert(mode[0]==GL_FILL);
#endif
    assert(glGetError()==GL_NO_ERROR);
    SDL_GL_DeleteContext(ctx);SDL_DestroyWindow(w);SDL_Quit();puts("rhi_pass_test: PASS (ownership, roles, validation, allocation rollback and recovery, frame copies, wireframe)");
}
