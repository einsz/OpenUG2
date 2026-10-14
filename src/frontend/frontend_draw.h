/* frontend_draw.h — drawing adapter for the frontend state machine.
 * Requires a live rhi device.
 * Does NOT create a window or own the application loop.
 *
 * Lifecycle:
 *   fed_init()  → compile shader, upload quad VBO (call once, rhi device live)
 *   fed_draw()  → render the current Fe state (call each frame)
 *   fed_free()  → release the shader and buffer
 *
 * fed_draw saves the caller's state (rhi_cmd_save_state) and restores it on
 * exit, so a caller that draws with ambient state gets it back. */
#ifndef OPENUG2_FRONTEND_DRAW_H
#define OPENUG2_FRONTEND_DRAW_H

#include "frontend.h"

/* Opaque drawing context. */
typedef struct FeDraw FeDraw;

/* Create the shader, pipeline and quad buffer. Returns NULL on failure. */
FeDraw *fed_init(void);

/* Draw the current Fe state into the current framebuffer.
 * vp_w/vp_h: viewport pixel size (for aspect-correct layout). */
void fed_draw(FeDraw *d, const Fe *fe, int vp_w, int vp_h);

/* Release the shader and buffer and free memory. */
void fed_free(FeDraw *d);

#endif
