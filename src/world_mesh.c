/* world_mesh.c — see world_mesh.h. Pure C99, GL 2.1, no VAO. */
#include "world_mesh.h"
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

    return b;   /* GL 2.1: no VAO, so attribute pointers are set at draw time */
}

static const RhiParamField world_debug_params_fields[] = {
    {"u_MVP", RHI_PARAM_MAT4, offsetof(WorldDebugParams, u_MVP), 1},
    {"u_DebugMode", RHI_PARAM_INT, offsetof(WorldDebugParams, u_DebugMode), 1},
};
const RhiParamLayout world_debug_params_layout = {
    world_debug_params_fields,
    (uint32_t)(sizeof world_debug_params_fields / sizeof world_debug_params_fields[0]),
    (uint32_t)sizeof(WorldDebugParams), "WorldDebugParams"
};

/* BatchedVertex in shader attribute order (a_Position, a_Normal, a_UV0,
   a_Color); depth test LEQUAL with write, no blend, two-sided. */
static RhiPipeline world_map_pipeline(RhiShader shader) {
    static RhiShader cached_shader;
    static RhiPipeline cached;
    if (cached.id && cached_shader.id == shader.id) return cached;
    RhiPipelineDesc d;
    rhi_pipeline_desc_init(&d);
    d.shader = shader;
    d.vertex.attribs[0] = (RhiVertexAttrib){ 0, 0, (uint32_t)offsetof(BatchedVertex, pos),    RHI_VERTEX_FLOAT3 };
    d.vertex.attribs[1] = (RhiVertexAttrib){ 1, 0, (uint32_t)offsetof(BatchedVertex, normal), RHI_VERTEX_FLOAT3 };
    d.vertex.attribs[2] = (RhiVertexAttrib){ 2, 0, (uint32_t)offsetof(BatchedVertex, uv),     RHI_VERTEX_FLOAT2 };
    d.vertex.attribs[3] = (RhiVertexAttrib){ 3, 0, (uint32_t)offsetof(BatchedVertex, col),    RHI_VERTEX_UNORM8X4 };
    d.vertex.attrib_count = 4;
    d.vertex.stride[0] = (uint32_t)sizeof(BatchedVertex);   /* 36 B */
    d.depth_test = 1;
    d.depth_write = 1;
    d.depth_compare = RHI_COMPARE_LESS_EQUAL;
    cached = rhi_pipeline_create(&d);
    cached_shader = shader;
    return cached;
}

void render_world_map(const WorldMeshBatch *batches, uint32_t n,
                      RhiShader shader, const float *mvp16, int debug_mode) {
    WorldDebugParams *p = (WorldDebugParams *)rhi_shader_params(shader, 0);
    if (!batches || !p) return;
    RhiPipeline pipeline = world_map_pipeline(shader);
    if (!pipeline.id) return;
    if (mvp16) memcpy(p->u_MVP, mvp16, sizeof p->u_MVP);
    p->u_DebugMode = debug_mode;
    rhi_cmd_bind_pipeline(pipeline);
    int wire = debug_mode == 2 && rhi_cmd_set_wireframe(1) == RHI_OK;

    for (uint32_t k = 0; k < n; k++) {
        const WorldMeshBatch *b = &batches[k];
        if (!b->vbo.id || !b->index_count) continue;   /* skip empty/disabled sections */
        rhi_cmd_bind_vertex_buffer(0, b->vbo, 0);
        rhi_cmd_bind_index_buffer(b->ibo);
        rhi_cmd_draw_indexed(b->index_count, 0);
    }

    if (wire) rhi_cmd_set_wireframe(0);
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
static char *wm_text(int is_vertex, const char *src, int len) {
    if (len < 0) len = (int)strlen(src);
    const char *header = "";
#ifdef N2_GLES
    if (len >= 8 && !strncmp(src, "#version", 8)) {
        const char *nl = memchr(src, '\n', (size_t)len);
        int skip = nl ? (int)(nl - src) + 1 : len;
        src += skip; len -= skip;
    }
    header = is_vertex ? GLSL_VS_HEADER : GLSL_FS_HEADER;
#else
    (void)is_vertex;
#endif
    size_t n = strlen(header);
    char *t = (char *)malloc(n + (size_t)len + 1);
    if (!t) return NULL;
    memcpy(t, header, n); memcpy(t + n, src, (size_t)len); t[n + (size_t)len] = 0;
    return t;
}

RhiShader world_debug_shader_load(const char *vert_path, const char *frag_path) {
    /* The Vulkan backend ignores this GLSL (it ships build-time SPIR-V selected by
       the "world_debug" label), so on-disk live editing only applies to GL.
       Prefer on-disk files (live-editable during dev); fall back per-shader to
       the make-embedded source (generated_world_debug_shaders.h, xxd byte
       arrays -- not NUL-terminated, so pass their *_len) so the binary works
       from any working directory. */
    char *vfile = wm_read_text(vert_path), *ffile = wm_read_text(frag_path);
    const char *vsrc; int vlen;
    const char *fsrc; int flen;
    if (vfile) { vsrc = vfile; vlen = -1; }
    else { vsrc = (const char *)src_world_debug_120_vert;
           vlen = (int)src_world_debug_120_vert_len;
           fprintf(stderr, "world_debug: %s not found, using embedded vertex shader\n", vert_path); }
    if (ffile) { fsrc = ffile; flen = -1; }
    else { fsrc = (const char *)src_world_debug_120_frag;
           flen = (int)src_world_debug_120_frag_len;
           fprintf(stderr, "world_debug: %s not found, using embedded fragment shader\n", frag_path); }

    static const char *const attributes[] = { "a_Position", "a_Normal", "a_UV0", "a_Color" };
    static const RhiParamLayout *const blocks[] = { &world_debug_params_layout };
    char *vtext = wm_text(1, vsrc, vlen);
    char *ftext = wm_text(0, fsrc, flen);
    free(vfile); free(ffile);
    RhiShader shader = {0};
    if (vtext && ftext)
        shader = rhi_shader_create(&(RhiShaderDesc){ vtext, ftext, attributes, 4, "world_debug", blocks, 1 });
    free(vtext); free(ftext);
    return shader;
}
