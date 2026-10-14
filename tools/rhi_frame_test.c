#include "rhi_gl.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* Raises GL_INVALID_ENUM through a call the state tracker does not model, as
 * an earlier unrelated call would. */
static void pending_error(void) { glHint(0x1234, GL_NICEST); }
int main(void) {
    const float green[4]={0,1,0,1};
    assert(SDL_Init(SDL_INIT_VIDEO)==0);
    assert(rhi_prepare_window(RHI_BACKEND_DEFAULT)==RHI_OK);
    SDL_Window *window=SDL_CreateWindow("frame review",0,0,64,64,
        rhi_window_flags(RHI_BACKEND_DEFAULT)|SDL_WINDOW_HIDDEN); assert(window);
    RhiDeviceDesc device={RHI_BACKEND_DEFAULT,0,window};
    assert(rhi_device_create(&device)==RHI_OK);
    const RhiCaps *caps=rhi_caps();
    const int gpu_timer=caps->gpu_timer;
    assert(caps->max_texture_size>=64);

    /* Main-target clear: whole frame, regardless of masks, scissor and viewport. */
    assert(rhi_cmd_clear_main(green,1.0f)==RHI_ERR_INVALID);   /* outside a frame */
    assert(rhi_frame_begin(NULL)==RHI_OK);
    assert(rhi_cmd_clear_main(NULL,1.0f)==RHI_ERR_INVALID);
    glClearColor(0,0,1,1);glClear(GL_COLOR_BUFFER_BIT);
    glColorMask(GL_FALSE,GL_TRUE,GL_FALSE,GL_TRUE);glDepthMask(GL_FALSE);
    glEnable(GL_SCISSOR_TEST);glScissor(0,0,8,8);glViewport(0,0,16,16);
    assert(rhi_cmd_clear_main(green,1.0f)==RHI_OK);
    unsigned char px[64*64*3];
    assert(rhi_read_pixels(0,0,64,64,RHI_READ_RGB8,px,0)==RHI_OK);
    for(int i=0;i<64*64;i++)assert(px[i*3]==0 && px[i*3+1]==255 && px[i*3+2]==0);
    GLboolean cm[4],dm;GLint vp[4];
    gls_get_booleanv(GL_COLOR_WRITEMASK,cm);gls_get_booleanv(GL_DEPTH_WRITEMASK,&dm);
    assert(!cm[0] && cm[1] && !cm[2] && cm[3] && !dm);   /* masks come back */
    gls_get_integerv(GL_VIEWPORT,vp);
    assert(vp[0]==0 && vp[1]==0 && vp[2]==64 && vp[3]==64 && !gls_is_enabled(GL_SCISSOR_TEST));
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glDepthMask(GL_TRUE);
    /* Not inside a pass. */
    RhiTexture target=rhi_texture_create(&(RhiTextureDesc){RHI_TEXTURE_2D,RHI_FORMAT_RGBA8,4,4,
        RHI_TEXTURE_SAMPLED|RHI_TEXTURE_COLOR_TARGET});
    assert(target.id && rhi_texture_upload(target,0,0,NULL,0)==RHI_OK);
    assert(rhi_cmd_begin_pass(&(RhiPassDesc){{target,0},{{0},0},RHI_LOAD_CLEAR,RHI_LOAD_KEEP,{0,0,0,1},1})==RHI_OK);
    assert(rhi_cmd_clear_main(green,1.0f)==RHI_ERR_INVALID);
    rhi_cmd_end_pass();glBindFramebuffer(GL_FRAMEBUFFER,0);
    assert(glGetError()==GL_NO_ERROR);

    /* Upload failures come back as results. */
    pending_error();
    RhiBuffer failed=rhi_buffer_create(&(RhiBufferDesc){RHI_BUFFER_VERTEX,RHI_UPDATE_STATIC,16,NULL});
    assert(!failed.id && glGetError()==GL_NO_ERROR);
    RhiBuffer stat=rhi_buffer_create(&(RhiBufferDesc){RHI_BUFFER_VERTEX,RHI_UPDATE_STATIC,16,NULL});
    assert(stat.id);
    pending_error();assert(rhi_buffer_update(stat,0,4,px)==RHI_ERR_UNSUPPORTED);
    pending_error();assert(rhi_buffer_replace(stat,32,NULL)==RHI_ERR_UNSUPPORTED);
    assert(rhi_buffer_update(stat,0,32,px)==RHI_OK);
    /* STREAM buffers do not check: the error stays for the caller. */
    RhiBuffer stream=rhi_buffer_create(&(RhiBufferDesc){RHI_BUFFER_VERTEX,RHI_UPDATE_STREAM,0,NULL});
    assert(stream.id);
    pending_error();assert(rhi_buffer_replace(stream,16,px)==RHI_OK);
    assert(glGetError()==GL_INVALID_ENUM && glGetError()==GL_NO_ERROR);
    rhi_buffer_destroy(stat);rhi_buffer_destroy(stream);
    /* A level larger than the device allows is rejected by GL. */
    RhiTexture big=rhi_texture_create(&(RhiTextureDesc){RHI_TEXTURE_2D,RHI_FORMAT_RGBA8,
        caps->max_texture_size+1,1,RHI_TEXTURE_SAMPLED});
    assert(big.id && rhi_texture_upload(big,0,0,NULL,0)==RHI_ERR_UNSUPPORTED);
    rhi_texture_destroy(big);
    RhiTexture tex=rhi_texture_create(&(RhiTextureDesc){RHI_TEXTURE_2D,RHI_FORMAT_RGB8,4,4,RHI_TEXTURE_SAMPLED});
    pending_error();assert(rhi_texture_upload(tex,0,0,px,0)==RHI_ERR_UNSUPPORTED);
    assert(rhi_texture_upload(tex,0,0,px,0)==RHI_OK);
    pending_error();assert(rhi_texture_generate_mips(tex)==RHI_ERR_UNSUPPORTED);
    assert(rhi_texture_generate_mips(tex)==RHI_OK);
    rhi_texture_destroy(tex);rhi_texture_destroy(target);
    assert(glGetError()==GL_NO_ERROR);

    /* GPU timers. */
    uint64_t ns=0;
    if(caps->gpu_timer) {
        RhiTimer t=rhi_timer_create(),u=rhi_timer_create();assert(t.id && u.id && t.id!=u.id);
        assert(rhi_cmd_timer_end()==RHI_ERR_INVALID);
        assert(rhi_cmd_timer_begin(t)==RHI_OK);
        assert(rhi_cmd_timer_begin(u)==RHI_ERR_INVALID);   /* one at a time */
        assert(!rhi_timer_read(t,&ns));                     /* still running */
        assert(rhi_cmd_clear_main(green,1.0f)==RHI_OK);
        assert(rhi_cmd_timer_end()==RHI_OK);
        glFinish();
        int ready=0;
        for(int i=0;i<1000 && !ready;i++)ready=rhi_timer_read(t,&ns);
        assert(ready);
        assert(!rhi_timer_read(u,&ns));   /* never measured */
        rhi_timer_destroy(t);assert(!rhi_timer_read(t,&ns));
        assert(rhi_cmd_timer_begin(t)==RHI_ERR_INVALID);
        RhiTimer again=rhi_timer_create();assert(again.id && again.id!=t.id);
        rhi_timer_destroy(again);rhi_timer_destroy(u);
    } else {
        assert(!rhi_timer_create().id);
    }
    assert(glGetError()==GL_NO_ERROR);
    assert(rhi_frame_end()==RHI_OK);
    rhi_device_destroy();SDL_DestroyWindow(window);SDL_Quit();
    printf("rhi_frame_test: PASS (main clear, upload results, texture limit, gpu timer %s)\n",
           gpu_timer?"on":"unavailable");
}
