/* vk_ui_stubs.c -- no-op definitions of the GL-only UI entry points for the
 * Vulkan (`make nfsu2-vk`) build.
 *
 * The Vulkan offscreen `--shot` build links src/rhi_vk.c instead of the GL
 * backend, so src/ugui/ugui_gl.c (and the ImGui debug overlay) are dropped from
 * the sources. ugui_gl.c is the only translation unit that defines the ug_gl_*
 * entry points, yet pause_menu.c / main.c reference them unconditionally. For an
 * offscreen scene shot the authored pause/menu UI is never rendered (no
 * --pause-shot), so these can be no-ops: ug_gl_create returns NULL, which the
 * pause menu already treats as "no GL backend available" and skips drawing.
 *
 * This file is NOT compiled into any GL build; it exists only so the Vulkan
 * link resolves the GL-UI symbols without pulling in rhi_gl / ugui_gl.
 *
 * dbgui_* (Dear ImGui) entry points are NOT stubbed here: every call site in
 * main.c is behind `#ifdef DEBUG_UI`, and the Vulkan target is built without
 * DEBUG_UI, so no dbgui_* symbol is referenced.
 */
#include "ugui/ugui_gl.h"

UgGL *ug_gl_create(void) { return NULL; }

void ug_gl_destroy(UgGL *g) { (void)g; }

void ug_gl_render(UgGL *g, const UgCanvas *cv, int win_w, int win_h) {
    (void)g; (void)cv; (void)win_w; (void)win_h;
}

void ug_gl_render_overlay(UgGL *g, const UgCanvas *cv, int win_w, int win_h) {
    (void)g; (void)cv; (void)win_w; (void)win_h;
}

void ug_gl_set_texlib(UgGL *g, UgTexLib *lib) { (void)g; (void)lib; }

int ug_gl_errors(const char *tag) { (void)tag; return 0; }
