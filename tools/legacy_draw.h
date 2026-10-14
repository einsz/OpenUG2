/* legacy_draw.h - the draws render.c issued before pipelines (P7..P9):
 * attributes, buffers and glDrawElements straight from GL, in whatever raster
 * state the caller set. The engine draws through rhi.h now; these stay for the
 * tests that set GL state by hand. */
#ifndef OPENUG2_LEGACY_DRAW_H
#define OPENUG2_LEGACY_DRAW_H
#include "render.h"
#include "rhi_gl.h"

static inline void draw_batch(const N2Batch *b) {
    glBindBuffer(GL_ARRAY_BUFFER, rhi_gl_buffer_name(b->vbo));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BatchedVertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(BatchedVertex), (void*)12);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(BatchedVertex), (void*)20);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(BatchedVertex), (void*)32);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rhi_gl_buffer_name(b->ibo));
    rhi_gl_flush_params();
    glDrawElements(GL_TRIANGLES, b->index_count, GL_UNSIGNED_SHORT, 0);
}

static inline void draw_gpumesh(GpuMesh *g) {
    if (g->cbo.id) {
        glBindBuffer(GL_ARRAY_BUFFER,rhi_gl_buffer_name(g->cbo));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3,4,GL_UNSIGNED_BYTE,GL_TRUE,0,(void*)0);
    } else glDisableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, rhi_gl_buffer_name(g->vbo));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, rhi_gl_buffer_name(g->nbo));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rhi_gl_buffer_name(g->ibo));
    rhi_gl_flush_params();
    glDrawElements(GL_TRIANGLES, g->nidx, GL_UNSIGNED_SHORT, 0);
}

#endif
