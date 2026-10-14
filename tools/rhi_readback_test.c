#include "rhi_gl.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    unsigned char pixels[64]; memset(pixels,0xa5,sizeof pixels);
    assert(rhi_read_pixels(0,0,1,1,RHI_READ_RGB8,pixels,0)==RHI_ERR_INVALID);
    assert(SDL_Init(SDL_INIT_VIDEO)==0);
    assert(rhi_prepare_window(RHI_BACKEND_DEFAULT)==RHI_OK);
    SDL_Window *window=SDL_CreateWindow("readback review",0,0,64,64,
        rhi_window_flags(RHI_BACKEND_DEFAULT)|SDL_WINDOW_HIDDEN); assert(window);
    RhiDeviceDesc device={RHI_BACKEND_DEFAULT,0,window};
    assert(rhi_device_create(&device)==RHI_OK);
    assert(rhi_frame_begin(NULL)==RHI_OK);
    glClearColor(0,0,1,1);glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);glScissor(0,32,64,32);
    glClearColor(1,0,0,1);glClear(GL_COLOR_BUFFER_BIT);glDisable(GL_SCISSOR_TEST);
    assert(rhi_read_pixels(1,31,3,2,RHI_READ_RGB8,pixels,13)==RHI_OK);
    for(int i=0;i<3;i++) {
        assert(pixels[i*3]==255 && pixels[i*3+1]==0 && pixels[i*3+2]==0);
        assert(pixels[13+i*3]==0 && pixels[14+i*3]==0 && pixels[15+i*3]==255);
    }
    assert(pixels[9]==0xa5 && pixels[12]==0xa5 && pixels[22]==0xa5);
    memset(pixels,0xa5,sizeof pixels);
    assert(rhi_read_pixels(1,31,3,2,RHI_READ_RGBA8,pixels,16)==RHI_OK);
    for(int i=0;i<3;i++) {
        assert(pixels[i*4]==255 && pixels[i*4+1]==0 && pixels[i*4+2]==0 && pixels[i*4+3]==255);
        assert(pixels[16+i*4]==0 && pixels[17+i*4]==0 && pixels[18+i*4]==255 && pixels[19+i*4]==255);
    }
    assert(pixels[12]==0xa5 && pixels[15]==0xa5 && pixels[28]==0xa5);
    assert(rhi_read_pixels(-1,0,1,1,RHI_READ_RGB8,pixels,0)==RHI_ERR_INVALID);
    assert(rhi_read_pixels(0,0,1,1,(RhiReadFormat)99,pixels,0)==RHI_ERR_INVALID);
    assert(rhi_read_pixels(0,0,1,1,RHI_READ_RGB8,NULL,0)==RHI_ERR_INVALID);
    assert(rhi_read_pixels(63,0,2,1,RHI_READ_RGB8,pixels,0)==RHI_ERR_INVALID);
    assert(rhi_read_pixels(0,0,3,2,RHI_READ_RGB8,pixels,8)==RHI_ERR_INVALID);
    RhiResult result=rhi_read_pixels(0,0,1,2,RHI_READ_RGB8,pixels+16,SIZE_MAX);
    printf("Overflowing stride result=%d\n",result);fflush(stdout);
    assert(result==RHI_ERR_INVALID);
    assert(glGetError()==GL_NO_ERROR);
    assert(rhi_frame_end()==RHI_OK);
    assert(rhi_read_pixels(0,0,1,1,RHI_READ_RGB8,pixels,0)==RHI_ERR_INVALID);
    rhi_device_destroy();SDL_DestroyWindow(window);SDL_Quit();
    puts("readback origin, stride padding, bounds, frame lifetime and overflow PASS");
}
