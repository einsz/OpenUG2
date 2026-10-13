/* world_mesh.c — see world_mesh.h. Pure C99, GL 2.1, no VAO. */
#include "world_mesh.h"
#include "rhi_gl.h"   /* GL names behind buffer handles for the draw code */
#include "generated_world_debug_shaders.h"   /* make-embedded shader byte arrays */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>   /* offsetof */

WorldMeshBatch upload_world_mesh_to_gpu(const BatchedVertex *verts, uint32_t vcount,
                                        const uint16_t *indices, uint32_t icount,
                                        const char *section_name, uint32_t chunk_id) {
    WorldMeshBatch b;
    memset(&b, 0, sizeof b);
    b.index_count = icount;
    b.chunk_id    = chunk_id;
    snprintf(b.section_name, sizeof b.section_name, "%s", section_name ? section_name : "");
    if (!verts || !indices || !vcount || !icount) return b;   /* vbo stays 0 -> skipped */

    b.vbo = rhi_buffer_create(&(RhiBufferDesc){RHI_BUFFER_VERTEX, RHI_UPDATE_STATIC,
                                                (size_t)vcount * sizeof(BatchedVertex), verts});
    b.ibo = rhi_buffer_create(&(RhiBufferDesc){RHI_BUFFER_INDEX, RHI_UPDATE_STATIC,
                                                (size_t)icount * sizeof(uint16_t), indices});

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    return b;   /* GL 2.1: no VAO, so attribute pointers are set at draw time */
}

void render_world_map(const WorldMeshBatch *batches, uint32_t n,
                      GLuint program, const float *mvp16, int debug_mode) {
    if (!batches || !program) return;
    glUseProgram(program);

    GLint locMVP  = glGetUniformLocation(program, "u_MVP");
    GLint locMode = glGetUniformLocation(program, "u_DebugMode");
    if (locMVP  >= 0 && mvp16) glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvp16);
    if (locMode >= 0)         glUniform1i(locMode, debug_mode);

    /* Attribute locations (bound to 0..3 by world_debug_program_load; queried so
       an externally-built program still works). Fetched once, not per batch. */
    GLint aPos = glGetAttribLocation(program, "a_Position");
    GLint aNor = glGetAttribLocation(program, "a_Normal");
    GLint aUV  = glGetAttribLocation(program, "a_UV0");
    GLint aCol = glGetAttribLocation(program, "a_Color");
    const GLsizei stride = (GLsizei)sizeof(BatchedVertex);   /* 36 B */

#ifndef N2_GLES
    if (debug_mode == 2) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);   /* desktop only */
#endif

    for (uint32_t k = 0; k < n; k++) {
        const WorldMeshBatch *b = &batches[k];
        if (!b->vbo.id || !b->index_count) continue;   /* skip empty/disabled sections */

        glBindBuffer(GL_ARRAY_BUFFER, rhi_gl_buffer_name(b->vbo));
        if (aPos >= 0) { glEnableVertexAttribArray((GLuint)aPos);
            glVertexAttribPointer((GLuint)aPos, 3, GL_FLOAT, GL_FALSE, stride,
                                  (const void *)offsetof(BatchedVertex, pos)); }
        if (aNor >= 0) { glEnableVertexAttribArray((GLuint)aNor);
            glVertexAttribPointer((GLuint)aNor, 3, GL_FLOAT, GL_FALSE, stride,
                                  (const void *)offsetof(BatchedVertex, normal)); }
        if (aUV  >= 0) { glEnableVertexAttribArray((GLuint)aUV);
            glVertexAttribPointer((GLuint)aUV, 2, GL_FLOAT, GL_FALSE, stride,
                                  (const void *)offsetof(BatchedVertex, uv)); }
        if (aCol >= 0) { glEnableVertexAttribArray((GLuint)aCol);
            glVertexAttribPointer((GLuint)aCol, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride,
                                  (const void *)offsetof(BatchedVertex, col)); }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rhi_gl_buffer_name(b->ibo));
        glDrawElements(GL_TRIANGLES, (GLsizei)b->index_count, GL_UNSIGNED_SHORT, 0);
    }

#ifndef N2_GLES
    if (debug_mode == 2) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void destroy_world_mesh_batch(WorldMeshBatch *b) {
    if (!b) return;
    rhi_buffer_destroy(b->ibo);
    rhi_buffer_destroy(b->vbo);
    /* b->vao intentionally not deleted: never created on GL 2.1 core */
    memset(b, 0, sizeof *b);
}

/* ---- GLSL 1.20 program loader (file-based; GLSL ES 1.00 under N2_GLES) ---- */
static char *wm_read_text(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n < 0) { fclose(f); return NULL; }
    char *s = (char *)malloc((size_t)n + 1);
    if (!s) { fclose(f); return NULL; }
    if (fread(s, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(s); return NULL; }
    s[n] = 0; fclose(f);
    return s;
}
/* Final text for one stage. Desktop: the 1.20 source as is. GLES: the GLSL ES
   1.00 variant of the same source -- drop the "#version 120" line (ES has no
   1.20) and put the ES precision prologue in front. len < 0 = NUL-terminated. */
static char *wm_text(GLenum type, const char *src, GLint len) {
    if (len < 0) len = (GLint)strlen(src);
    const char *header = "";
#ifdef N2_GLES
    if (len >= 8 && !strncmp(src, "#version", 8)) {
        const char *nl = memchr(src, '\n', (size_t)len);
        GLint skip = nl ? (GLint)(nl - src) + 1 : len;
        src += skip; len -= skip;
    }
    header = type == GL_VERTEX_SHADER ? GLSL_VS_HEADER : GLSL_FS_HEADER;
#else
    (void)type;
#endif
    size_t n = strlen(header);
    char *t = (char *)malloc(n + (size_t)len + 1);
    if (!t) return NULL;
    memcpy(t, header, n); memcpy(t + n, src, (size_t)len); t[n + (size_t)len] = 0;
    return t;
}

RhiShader world_debug_shader_load(const char *vert_path, const char *frag_path) {
    /* Prefer on-disk files (live-editable during dev); fall back per-shader to
       the make-embedded source (generated_world_debug_shaders.h, xxd byte
       arrays -- not NUL-terminated, so pass their *_len) so the binary works
       from any working directory. */
    char *vfile = wm_read_text(vert_path), *ffile = wm_read_text(frag_path);
    const char *vsrc; GLint vlen;
    const char *fsrc; GLint flen;
    if (vfile) { vsrc = vfile; vlen = -1; }
    else { vsrc = (const char *)src_world_debug_120_vert;
           vlen = (GLint)src_world_debug_120_vert_len;
           fprintf(stderr, "world_debug: %s not found, using embedded vertex shader\n", vert_path); }
    if (ffile) { fsrc = ffile; flen = -1; }
    else { fsrc = (const char *)src_world_debug_120_frag;
           flen = (GLint)src_world_debug_120_frag_len;
           fprintf(stderr, "world_debug: %s not found, using embedded fragment shader\n", frag_path); }

    static const char *const attributes[] = { "a_Position", "a_Normal", "a_UV0", "a_Color" };
    char *vtext = wm_text(GL_VERTEX_SHADER, vsrc, vlen);
    char *ftext = wm_text(GL_FRAGMENT_SHADER, fsrc, flen);
    free(vfile); free(ffile);
    RhiShader shader = {0};
    if (vtext && ftext)
        shader = rhi_shader_create(&(RhiShaderDesc){ vtext, ftext, attributes, 4, "world_debug" });
    free(vtext); free(ftext);
    return shader;
}
