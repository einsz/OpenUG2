#include "rhi_gl_state.h"
#include "ugui/ugui_gl.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    GLint viewport[4],scissor[4],blend[6],active,texture[2],program,buffer,unpack;
    GLboolean enable[4];GLint attr[5][6];void *pointer[5];
} State;
static void state(State *s) {
    memset(s,0,sizeof *s);
    glGetIntegerv(GL_VIEWPORT,s->viewport);glGetIntegerv(GL_SCISSOR_BOX,s->scissor);
    const GLenum blends[]={GL_BLEND_SRC_RGB,GL_BLEND_DST_RGB,GL_BLEND_SRC_ALPHA,GL_BLEND_DST_ALPHA,GL_BLEND_EQUATION_RGB,GL_BLEND_EQUATION_ALPHA};
    const GLenum caps[]={GL_SCISSOR_TEST,GL_BLEND,GL_DEPTH_TEST,GL_CULL_FACE};
    for(int i=0;i<6;i++)glGetIntegerv(blends[i],&s->blend[i]);
    for(int i=0;i<4;i++)s->enable[i]=glIsEnabled(caps[i]);
    glGetIntegerv(GL_ACTIVE_TEXTURE,&s->active);
    glActiveTexture(GL_TEXTURE0);glGetIntegerv(GL_TEXTURE_BINDING_2D,&s->texture[0]);
    glActiveTexture(GL_TEXTURE3);glGetIntegerv(GL_TEXTURE_BINDING_2D,&s->texture[1]);
    glActiveTexture(s->active);glGetIntegerv(GL_CURRENT_PROGRAM,&s->program);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&s->buffer);glGetIntegerv(GL_UNPACK_ALIGNMENT,&s->unpack);
    const GLenum props[]={GL_VERTEX_ATTRIB_ARRAY_ENABLED,GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING,GL_VERTEX_ATTRIB_ARRAY_SIZE,GL_VERTEX_ATTRIB_ARRAY_TYPE,GL_VERTEX_ATTRIB_ARRAY_NORMALIZED,GL_VERTEX_ATTRIB_ARRAY_STRIDE};
    for(int i=0;i<5;i++){for(int j=0;j<6;j++)glGetVertexAttribiv(i,props[j],&s->attr[i][j]);glGetVertexAttribPointerv(i,GL_VERTEX_ATTRIB_ARRAY_POINTER,&s->pointer[i]);}
}
int main(void) {
    assert(SDL_Init(SDL_INIT_VIDEO)==0);
    SDL_Window *w=SDL_CreateWindow("GUI state test",0,0,128,96,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);assert(w);
    SDL_GLContext ctx=SDL_GL_CreateContext(w);assert(ctx);UgGL *g=ug_gl_create();assert(g);
    GLuint buffers[5],textures[2];float data[128]={0};glGenBuffers(5,buffers);glGenTextures(2,textures);
    for(int i=0;i<5;i++){glBindBuffer(GL_ARRAY_BUFFER,buffers[i]);glBufferData(GL_ARRAY_BUFFER,sizeof data,data,GL_STATIC_DRAW);glVertexAttribPointer(i,2+i%3,GL_FLOAT,i%2,32,(void *)(size_t)(i*4));if(i%2)glEnableVertexAttribArray(i);else glDisableVertexAttribArray(i);}
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,textures[0]);
    glActiveTexture(GL_TEXTURE3);glBindTexture(GL_TEXTURE_2D,textures[1]);
    glViewport(7,11,73,50);glScissor(19,23,27,18);
    glEnable(GL_SCISSOR_TEST);glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_ONE,GL_SRC_ALPHA,GL_DST_ALPHA,GL_ZERO);
    glBlendEquationSeparate(GL_FUNC_SUBTRACT,GL_FUNC_REVERSE_SUBTRACT);glPixelStorei(GL_UNPACK_ALIGNMENT,8);
    UgCanvas canvas;ug_cv_init(&canvas);ug_cv_flat_rect(&canvas,UG_RECT_C(0,0,50,20),UG_WHITE);
    State before,after;state(&before);assert(glGetError()==GL_NO_ERROR);
    ug_gl_render(g,&canvas,128,96);state(&after);
    assert(glGetError()==GL_NO_ERROR);assert(!memcmp(&before,&after,sizeof before));
    ug_cv_free(&canvas);ug_gl_destroy(g);glDeleteBuffers(5,buffers);glDeleteTextures(2,textures);
    SDL_GL_DeleteContext(ctx);SDL_DestroyWindow(w);SDL_Quit();puts("ugui_gl_state_test: PASS");return 0;
}
