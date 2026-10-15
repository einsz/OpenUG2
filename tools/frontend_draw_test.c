/* frontend_draw_test.c — draws every frontend screen into a hidden 16:9
 * window: the 4:3 menu area changes, the side bars and the caller's state do
 * not. Prints a pixel hash per screen. */
#include "rhi_gl.h"
#include "frontend/frontend_draw.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 160
#define H 90
#define MENU_X 20   /* 4:3 area: x 20..139 */

static unsigned char px[W * H * 3];

static unsigned hash_pixels(void) {
    unsigned h = 2166136261u;
    for (size_t i = 0; i < sizeof px; i++) h = (h ^ px[i]) * 16777619u;
    return h;
}

/* Raw GL state of a caller that draws with ambient state, like the HUD. */
static void set_caller_state(GLuint tex) {
    glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL); glDepthMask(GL_FALSE);
    glDisable(GL_BLEND); glBlendFunc(GL_ONE, GL_ZERO);
    glEnable(GL_CULL_FACE);
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, tex);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, tex);
    glUseProgram(0);
}

static void draw_screen(FeDraw *d, const Fe *fe, const char *name, int covers_menu) {
    static const float clear[4] = {0.2f, 0.4f, 0.6f, 1.0f};
    GLuint tex; glGenTextures(1, &tex);
    assert(rhi_cmd_clear_main(clear, 1.0f) == RHI_OK);
    glViewport(4, 4, 8, 8);
    set_caller_state(tex);
    RhiStateSnapshot before, after;
    rhi_cmd_save_state(&before);
    fed_draw(d, fe, W, H);
    rhi_cmd_save_state(&after);
    assert(!memcmp(&before, &after, sizeof before));   /* caller state comes back */
    assert(glGetError() == GL_NO_ERROR);

    assert(rhi_read_pixels(0, 0, W, H, RHI_READ_RGB8, px, 0) == RHI_OK);
    const unsigned char bg[3] = {51, 102, 153};
    int changed = 0;
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            const unsigned char *p = px + (y * W + x) * 3;
            int same = abs(p[0] - bg[0]) <= 1 && abs(p[1] - bg[1]) <= 1 && abs(p[2] - bg[2]) <= 1;
            if (x < MENU_X || x >= W - MENU_X) assert(same);   /* outside the 4:3 area */
            else changed += !same;
        }
    assert(changed > 0);
    if (covers_menu) assert(changed == (W - 2 * MENU_X) * H);
    printf("  %-14s %08x\n", name, hash_pixels());
    glDeleteTextures(1, &tex);
}

int main(void) {
    assert(SDL_Init(SDL_INIT_VIDEO) == 0);
    assert(rhi_prepare_window(RHI_BACKEND_DEFAULT) == RHI_OK);
    SDL_Window *window = SDL_CreateWindow("frontend draw", 0, 0, W, H,
        rhi_window_flags(RHI_BACKEND_DEFAULT) | SDL_WINDOW_HIDDEN);
    assert(window);
    RhiDeviceDesc device = {RHI_BACKEND_DEFAULT, 0, window};
    assert(rhi_device_create(&device) == RHI_OK);
    int32_t dw, dh; rhi_drawable_size(&dw, &dh);
    assert(dw == W && dh == H);
    assert(rhi_frame_begin(NULL) == RHI_OK);

    FeDraw *d = fed_init();
    assert(d);
    static const FeMenuEntry entries[] = {
        {"FREE ROAM", FE_ACTION_FREE_ROAM, 1},
        {"RACE SELECT", FE_ACTION_RACE_SELECT, 0},
        {"QUIT", FE_ACTION_QUIT, 1},
    };
    Fe fe;
    fe_init(&fe); fe_set_entries(&fe, entries, 3);
    fe.pulse = 0.5f;
    draw_screen(d, &fe, "title", 1);
    fe.screen = FE_SCREEN_MAIN; fe.selected = 2;
    draw_screen(d, &fe, "main", 0);
    fe_pause_init(&fe); fe.pulse = 0.25f;
    draw_screen(d, &fe, "pause", 0);
    fe.screen = FE_SCREEN_QUIT_CONFIRM; fe.quit_sel = 1;
    draw_screen(d, &fe, "quit", 1);
    fed_free(d);

    assert(rhi_frame_end() == RHI_OK);
    rhi_device_destroy(); SDL_DestroyWindow(window); SDL_Quit();
    printf("frontend_draw_test: PASS\n");
    return 0;
}
