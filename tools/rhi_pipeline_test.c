/* Ordinary command rejection and optional-color checks in a native GL context. */
#include "rhi_gl_state.h"
#include "rhi.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>

static void pixel(unsigned char out[4]) { glReadPixels(8,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,out); }
static void rejected_draw(void (*draw)(uint32_t,uint32_t),uint32_t count,uint32_t first) {
    unsigned char out[4];GLint before[3],after[3];
    const GLenum names[]={GL_CURRENT_PROGRAM,GL_ARRAY_BUFFER_BINDING,GL_ELEMENT_ARRAY_BUFFER_BINDING};
    for(int n=0;n<3;n++)glGetIntegerv(names[n],&before[n]);
    GLint pos=gls_attrib_enabled(0),color=gls_attrib_enabled(1);glClear(GL_COLOR_BUFFER_BIT);
    draw(count,first);pixel(out);
    assert(glGetError()==GL_NO_ERROR && out[0]==0 && out[1]==0 && out[2]==0);
    for(int n=0;n<3;n++){glGetIntegerv(names[n],&after[n]);assert(before[n]==after[n]);}
    assert(pos==gls_attrib_enabled(0) && color==gls_attrib_enabled(1));
}
static RhiShader shader(void) {
    const char *names[]={"aPos","aColor"};
    RhiShaderDesc d={"attribute vec2 aPos;attribute vec4 aColor;varying vec4 c;void main(){c=aColor;gl_Position=vec4(aPos,0.0,1.0);}",
        "varying vec4 c;void main(){gl_FragColor=c;}",names,2,"pipeline test",NULL,0};
    RhiShader s=rhi_shader_create(&d);assert(s.id);return s;
}
int main(void) {
    assert(SDL_Init(SDL_INIT_VIDEO)==0);
    assert(rhi_prepare_window(RHI_BACKEND_DEFAULT)==RHI_OK);
    SDL_Window *w=SDL_CreateWindow("pipeline test",0,0,16,16,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);assert(w);
    SDL_GLContext ctx=SDL_GL_CreateContext(w);assert(ctx);
    glViewport(0,0,16,16);glClearColor(0,0,0,1);
    RhiShader s=shader();RhiPipelineDesc d,bad;rhi_pipeline_desc_init(&d);d.shader=s;
    d.vertex.attrib_count=2;
    d.vertex.attribs[0]=(RhiVertexAttrib){0,0,0,RHI_VERTEX_FLOAT2};
    d.vertex.attribs[1]=(RhiVertexAttrib){1,1,0,RHI_VERTEX_UNORM8X4};
    d.vertex.stride[0]=8;d.vertex.stride[1]=4;
    bad=d;bad.vertex.attribs[1].location=0;assert(!rhi_pipeline_create(&bad).id);
    bad=d;bad.vertex.attribs[1].location=RHI_MAX_VERTEX_ATTRIBS;assert(!rhi_pipeline_create(&bad).id);
    bad=d;bad.vertex.stride[0]=(uint32_t)INT_MAX+1u;assert(!rhi_pipeline_create(&bad).id);
    bad=d;bad.vertex.attribs[0].offset=4;assert(!rhi_pipeline_create(&bad).id);
    RhiPipeline p=rhi_pipeline_create(&d);assert(p.id && rhi_pipeline_create(&d).id==p.id);
    bad=d;bad.vertex.stride[0]=bad.vertex.stride[1]=0;RhiPipeline packed=rhi_pipeline_create(&bad);assert(packed.id);
    bad=d;bad.vertex.attribs[1].format=RHI_VERTEX_FLOAT1;assert(rhi_pipeline_create(&bad).id);
    const float pos[]={-1,-1,1,-1,0,1};const uint16_t indices[]={0,1,2};
    RhiBuffer v=rhi_buffer_create(&(RhiBufferDesc){RHI_BUFFER_VERTEX,RHI_UPDATE_STATIC,sizeof pos,pos});
    RhiBuffer i=rhi_buffer_create(&(RhiBufferDesc){RHI_BUFFER_INDEX,RHI_UPDATE_STATIC,sizeof indices,indices});
    assert(v.id && i.id);
    /* Pipelines have no depth bias: binding one turns a caller's polygon offset off. */
    glEnable(GL_POLYGON_OFFSET_FILL);rhi_cmd_bind_pipeline(p);
    assert(!glIsEnabled(GL_POLYGON_OFFSET_FILL) && !gls_is_enabled(GL_POLYGON_OFFSET_FILL));
    /* Blend pipelines force the ADD equation; a snapshot brings the caller's equation,
       array-buffer binding and enabled attribute arrays back. */
    {
        bad=d;bad.blend=1;bad.src_rgb=bad.src_alpha=RHI_BLEND_SRC_ALPHA;
        bad.dst_rgb=bad.dst_alpha=RHI_BLEND_ONE_MINUS_SRC_ALPHA;
        RhiPipeline blended=rhi_pipeline_create(&bad);assert(blended.id);
        GLuint scratch;glGenBuffers(1,&scratch);
        glBlendEquationSeparate(GL_FUNC_SUBTRACT,GL_FUNC_REVERSE_SUBTRACT);
        glBindBuffer(GL_ARRAY_BUFFER,scratch);glEnableVertexAttribArray(5);
        RhiStateSnapshot saved;rhi_cmd_save_state(&saved);
        rhi_cmd_bind_pipeline(blended);rhi_cmd_bind_vertex_buffer(0,v,0);rhi_cmd_bind_vertex_buffer(1,(RhiBuffer){0},0);
        GLint eq[2];gls_get_integerv(GL_BLEND_EQUATION_RGB,&eq[0]);gls_get_integerv(GL_BLEND_EQUATION_ALPHA,&eq[1]);
        assert(eq[0]==GL_FUNC_ADD && eq[1]==GL_FUNC_ADD);
        glGetIntegerv(GL_BLEND_EQUATION_RGB,&eq[0]);glGetIntegerv(GL_BLEND_EQUATION_ALPHA,&eq[1]);
        assert(eq[0]==GL_FUNC_ADD && eq[1]==GL_FUNC_ADD);
        rhi_cmd_draw(3,0);   /* rebinds the array buffer, switches attribute 5 off */
        assert(!gls_attrib_enabled(5));
        rhi_cmd_restore_state(&saved);
        GLint buf,on;
        gls_get_integerv(GL_BLEND_EQUATION_RGB,&eq[0]);gls_get_integerv(GL_BLEND_EQUATION_ALPHA,&eq[1]);
        assert(eq[0]==GL_FUNC_SUBTRACT && eq[1]==GL_FUNC_REVERSE_SUBTRACT);
        glGetIntegerv(GL_BLEND_EQUATION_RGB,&eq[0]);glGetIntegerv(GL_BLEND_EQUATION_ALPHA,&eq[1]);
        assert(eq[0]==GL_FUNC_SUBTRACT && eq[1]==GL_FUNC_REVERSE_SUBTRACT);
        gls_get_integerv(GL_ARRAY_BUFFER_BINDING,&buf);assert((GLuint)buf==scratch);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&buf);assert((GLuint)buf==scratch);
        glGetVertexAttribiv(5,GL_VERTEX_ATTRIB_ARRAY_ENABLED,&on);assert(on && gls_attrib_enabled(5));
        assert(glGetError()==GL_NO_ERROR);
        glDisableVertexAttribArray(5);glBindBuffer(GL_ARRAY_BUFFER,0);glDeleteBuffers(1,&scratch);
        glBlendEquation(GL_FUNC_ADD);rhi_cmd_bind_pipeline(p);
    }
    rhi_cmd_bind_vertex_buffer(0,v,0);rhi_cmd_bind_index_buffer(i);
    /* A null color stream disables its array and uses the caller's constant. */
    glEnableVertexAttribArray(1);glVertexAttrib4f(1,1,1,1,1);rhi_cmd_bind_vertex_buffer(1,(RhiBuffer){0},0);
    glClear(GL_COLOR_BUFFER_BIT);rhi_cmd_draw_indexed(3,0);unsigned char out[4];pixel(out);
    assert(glGetError()==GL_NO_ERROR && out[0]==255 && out[1]==255 && out[2]==255);
    assert(!gls_attrib_enabled(1));
    /* Non-indexed draws need no index buffer, and include first/count in every stream extent. */
    rhi_cmd_bind_index_buffer((RhiBuffer){0});glClear(GL_COLOR_BUFFER_BIT);rhi_cmd_draw(3,0);pixel(out);
    assert(glGetError()==GL_NO_ERROR && out[0]==255);
    rejected_draw(rhi_cmd_draw,0,0);rejected_draw(rhi_cmd_draw,4,0);rejected_draw(rhi_cmd_draw,3,1);
    rejected_draw(rhi_cmd_draw,(uint32_t)INT_MAX+1u,0);rejected_draw(rhi_cmd_draw,1,UINT32_MAX);
    rhi_cmd_bind_pipeline(packed);rejected_draw(rhi_cmd_draw,3,1);
    glClear(GL_COLOR_BUFFER_BIT);rhi_cmd_draw(3,0);pixel(out);assert(out[0]==255 && glGetError()==GL_NO_ERROR);
    rhi_cmd_bind_pipeline(p);rhi_cmd_bind_vertex_buffer(0,v,8);rejected_draw(rhi_cmd_draw,3,0);
    /* Nonzero attribute offsets and the last vertex exactly at the allocation end. */
    const float padded[]={0,0,-1,-1,0,0,1,-1,0,0,0,1};
    RhiBuffer interleaved=rhi_buffer_create(&(RhiBufferDesc){RHI_BUFFER_VERTEX,RHI_UPDATE_STATIC,sizeof padded,padded});assert(interleaved.id);
    bad=d;bad.vertex.attribs[0].offset=8;bad.vertex.stride[0]=16;
    RhiPipeline offset=rhi_pipeline_create(&bad);assert(offset.id);rhi_cmd_bind_pipeline(offset);rhi_cmd_bind_vertex_buffer(0,interleaved,0);
    glClear(GL_COLOR_BUFFER_BIT);rhi_cmd_draw(3,0);pixel(out);assert(out[0]==255 && glGetError()==GL_NO_ERROR);
    rejected_draw(rhi_cmd_draw,3,1);rhi_buffer_destroy(interleaved);rhi_cmd_bind_pipeline(p);
    /* FLOAT1 uses four bytes per packed vertex, including a later stream's range. */
    const float red[]={1,1,1};RhiBuffer scalar=rhi_buffer_create(&(RhiBufferDesc){RHI_BUFFER_VERTEX,RHI_UPDATE_STATIC,sizeof red,red});assert(scalar.id);
    bad=d;bad.vertex.attribs[1].format=RHI_VERTEX_FLOAT1;bad.vertex.stride[1]=0;
    RhiPipeline single=rhi_pipeline_create(&bad);assert(single.id);rhi_cmd_bind_pipeline(single);
    rhi_cmd_bind_vertex_buffer(0,v,0);rhi_cmd_bind_vertex_buffer(1,scalar,0);
    glClear(GL_COLOR_BUFFER_BIT);rhi_cmd_draw(3,0);pixel(out);assert(out[0]==255 && out[1]==0 && out[2]==0 && glGetError()==GL_NO_ERROR);
    assert(rhi_buffer_replace(scalar,2*sizeof(float),red)==RHI_OK);rejected_draw(rhi_cmd_draw,3,0);
    rhi_buffer_destroy(scalar);rhi_cmd_bind_vertex_buffer(1,(RhiBuffer){0},0);rhi_cmd_bind_pipeline(p);
    rhi_cmd_bind_vertex_buffer(0,v,0);rhi_cmd_bind_index_buffer(i);
    rejected_draw(rhi_cmd_draw_indexed,4,0);rejected_draw(rhi_cmd_draw_indexed,3,1);rejected_draw(rhi_cmd_draw_indexed,1,UINT32_MAX);rejected_draw(rhi_cmd_draw_indexed,(uint32_t)INT_MAX+1u,0);
    rhi_cmd_bind_index_buffer((RhiBuffer){0});rejected_draw(rhi_cmd_draw_indexed,3,0);
    rhi_cmd_bind_index_buffer(v);rejected_draw(rhi_cmd_draw_indexed,3,0);rhi_cmd_bind_index_buffer(i);
    rhi_cmd_bind_vertex_buffer(0,i,0);rejected_draw(rhi_cmd_draw_indexed,3,0);rejected_draw(rhi_cmd_draw,3,0);
    rhi_cmd_bind_vertex_buffer(0,v,sizeof pos);rejected_draw(rhi_cmd_draw_indexed,3,0);
    rhi_cmd_bind_vertex_buffer(0,v,sizeof pos-4);rejected_draw(rhi_cmd_draw_indexed,3,0);
    RhiBuffer stale_v=rhi_buffer_create(&(RhiBufferDesc){RHI_BUFFER_VERTEX,RHI_UPDATE_STATIC,sizeof pos,pos});
    rhi_cmd_bind_vertex_buffer(0,stale_v,0);rhi_buffer_destroy(stale_v);rejected_draw(rhi_cmd_draw_indexed,3,0);rejected_draw(rhi_cmd_draw,3,0);
    rhi_cmd_bind_vertex_buffer(0,v,0);
    RhiBuffer stale_i=rhi_buffer_create(&(RhiBufferDesc){RHI_BUFFER_INDEX,RHI_UPDATE_STATIC,sizeof indices,indices});
    rhi_cmd_bind_index_buffer(stale_i);rhi_buffer_destroy(stale_i);rejected_draw(rhi_cmd_draw_indexed,3,0);rhi_cmd_bind_index_buffer(i);
    rhi_shader_destroy(s);rejected_draw(rhi_cmd_draw,3,0);rejected_draw(rhi_cmd_draw_indexed,3,0);
    RhiShader next=shader();d.shader=next;RhiPipeline live=rhi_pipeline_create(&d);assert(live.id);
    rhi_cmd_bind_pipeline(live);GLint before,after;gls_get_integerv(GL_CURRENT_PROGRAM,&before);
    rhi_cmd_bind_pipeline(p);gls_get_integerv(GL_CURRENT_PROGRAM,&after);assert(before==after);rejected_draw(rhi_cmd_draw_indexed,3,0);
    rhi_buffer_destroy(v);rhi_buffer_destroy(i);rhi_shader_destroy(next);
    SDL_GL_DeleteContext(ctx);SDL_DestroyWindow(w);SDL_Quit();
    puts("rhi_pipeline_test: PASS (layouts, resource lifetime, usage/ranges, optional color, indexed/nonindexed)");return 0;
}
