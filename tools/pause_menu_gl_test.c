/* Synthetic GL transform/state test. No install, asset decoder or retail data. */
#include "../src/rhi_gl_state.h"
#include "../src/ugui/ugui_gl.h"
#include "../src/pause_menu_layout.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    assert(!SDL_Init(SDL_INIT_VIDEO));
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
    SDL_Window *win=SDL_CreateWindow("Synthetic pause GL test",0,0,1919,1079,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
    assert(win);
    SDL_GLContext context=SDL_GL_CreateContext(win);assert(context);
    UgGL *gl=ug_gl_create();assert(gl);
    UgCanvas cv;ug_cv_init(&cv);
    /* Test geometry only; these primitives never enter the pause compositor. */
    ug_cv_flat_rect(&cv,UG_RECT_C(-200,-40,10,10),UG_RGBA(255,0,0,255));
    int sizes[][2]={{1920,1080},{1280,960},{1280,720},{640,480},{960,600},{1919,1079}};
    for(size_t i=0;i<sizeof sizes/sizeof sizes[0];i++) {
        int w=sizes[i][0],h=sizes[i][1];
        SDL_SetWindowSize(win,w,h);
        SDL_PumpEvents();
        int dw,dh;SDL_GL_GetDrawableSize(win,&dw,&dh);assert(dw==w && dh==h);
        glDisable(GL_SCISSOR_TEST);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
        glViewport(13,17,100,105);glScissor(9,11,120,121);glEnable(GL_SCISSOR_TEST);
        glBlendFuncSeparate(GL_ONE,GL_ZERO,GL_ZERO,GL_ONE);
        glActiveTexture(GL_TEXTURE1);
        ug_gl_render_overlay(gl,&cv,w,h);
        int vp[4],sc[4],active,srca,dsta;
        glGetIntegerv(GL_VIEWPORT,vp);glGetIntegerv(GL_SCISSOR_BOX,sc);
        glGetIntegerv(GL_ACTIVE_TEXTURE,&active);
        glGetIntegerv(GL_BLEND_SRC_ALPHA,&srca);glGetIntegerv(GL_BLEND_DST_ALPHA,&dsta);
        assert(vp[0]==13 && vp[1]==17 && vp[2]==100 && vp[3]==105);
        assert(sc[0]==9 && sc[1]==11 && sc[2]==120 && sc[3]==121);
        assert(glIsEnabled(GL_SCISSOR_TEST) && active==GL_TEXTURE1 && srca==GL_ZERO && dsta==GL_ONE);
        unsigned char *pixels=malloc((size_t)w*h*4);assert(pixels);
        glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
        int minx=w,miny=h,maxx=-1,maxy=-1;
        for(int y=0;y<h;y++)for(int x=0;x<w;x++)if(pixels[((size_t)y*w+x)*4]>200) {
            if(x<minx)minx=x;
            if(x>maxx)maxx=x;
            if(y<miny)miny=y;
            if(y>maxy)maxy=y;
        }
        PauseTransform t=pause_menu_transform(w,h);
        /* Raster edges are determined by pixel-centre coverage. */
        assert(abs(minx-(int)ceil(t.ox+115*t.scale-.5))<=1);
        assert(abs(maxx-((int)ceil(t.ox+125*t.scale-.5)-1))<=1);
        assert(abs(miny-(int)ceil(h-(t.oy+205*t.scale)-.5))<=1);
        assert(abs(maxy-((int)ceil(h-(t.oy+195*t.scale)-.5)-1))<=1);
        free(pixels);
        assert(!ug_gl_errors("synthetic pause overlay"));
    }
    ug_cv_free(&cv);ug_gl_destroy(gl);SDL_GL_DeleteContext(context);SDL_DestroyWindow(win);SDL_Quit();
    puts("synthetic pause GL transform/state: PASS (6 viewports)");
    return 0;
}
