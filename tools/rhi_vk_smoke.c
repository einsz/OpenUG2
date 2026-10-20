/* rhi_vk_smoke.c - P11a gate for the Vulkan backend (src/rhi_vk.c).
 *
 * Drives the offscreen device through one frame: prepare_window, create a
 * hidden SDL window with rhi_window_flags, create the device, begin a frame,
 * clear the main target to a known colour, read pixels back and check they
 * equal the clear colour (+/-1), then end the frame. Fails (nonzero exit) on
 * any Vulkan validation message or a wrong colour.
 *
 * Build/run flags come from ~/nfsu2/vk-spike/build.sh + env.sh (sysroot Vulkan).
 */
#include "rhi.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Non-static global in rhi_vk.c (no rhi.h surface for it). */
extern unsigned rhi_vk_validation_messages;

static const int W = 256, H = 192;
static const float CLEAR[4] = { 0.25f, 0.5f, 0.75f, 1.0f };

static int fails;
#define EXPECT(cond, ...) do { if (!(cond)) { fprintf(stderr, "FAIL: " __VA_ARGS__); fputc('\n', stderr); fails++; } } while (0)

/* Expected 8-bit value for a 0..1 clear channel (round to nearest). */
static int chan(float f) { int v = (int)(f * 255.0f + 0.5f); return v < 0 ? 0 : v > 255 ? 255 : v; }
static int near8(int got, int want) { int d = got - want; return (d < 0 ? -d : d) <= 1; }

int main(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError()); return 2; }
    printf("SDL video driver: %s\n", SDL_GetCurrentVideoDriver());

    EXPECT(rhi_prepare_window(RHI_BACKEND_VULKAN) == RHI_OK, "rhi_prepare_window(VULKAN)");
    uint32_t flags = rhi_window_flags(RHI_BACKEND_VULKAN);
    EXPECT(flags != 0, "rhi_window_flags(VULKAN) should be nonzero");

    /* The offscreen SDL driver has no Vulkan surface support, so SDL_WINDOW_VULKAN
     * is rejected. Offscreen v0 creates no surface and uses the window only for
     * its size, so fall back to a plain hidden window there (like the spike). */
    SDL_Window *win = SDL_CreateWindow("rhi_vk_smoke", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                       W, H, flags | SDL_WINDOW_HIDDEN);
    if (!win) {
        fprintf(stderr, "note: SDL_WINDOW_VULKAN unavailable (%s); using a plain hidden window\n", SDL_GetError());
        win = SDL_CreateWindow("rhi_vk_smoke", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                               W, H, SDL_WINDOW_HIDDEN);
    }
    if (!win) { fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError()); SDL_Quit(); return 2; }

    RhiDeviceDesc desc = { RHI_BACKEND_VULKAN, 0, win };
    RhiResult dr = rhi_device_create(&desc);
    if (dr != RHI_OK) { fprintf(stderr, "rhi_device_create -> %d\n", (int)dr); SDL_DestroyWindow(win); SDL_Quit(); return 2; }

    int32_t dw = 0, dh = 0; rhi_drawable_size(&dw, &dh);
    printf("drawable size: %dx%d\n", dw, dh);
    EXPECT(dw == W && dh == H, "drawable size %dx%d != window %dx%d", dw, dh, W, H);

    const RhiCaps *c = rhi_caps();
    printf("caps: backend=%d max_viewport=%dx%d max_tex=%d bc1/2/3=%u/%u/%u wireframe=%u gpu_timer=%u aniso=%.1f\n",
           (int)c->backend, c->max_viewport[0], c->max_viewport[1], c->max_texture_size,
           c->tex_bc1, c->tex_bc2, c->tex_bc3, c->wireframe, c->gpu_timer, c->max_anisotropy);
    EXPECT(c->backend == RHI_BACKEND_VULKAN, "caps backend != VULKAN");
    EXPECT(c->max_texture_size > 0, "caps max_texture_size should be positive");

    RhiFrameInfo fi = {0};
    EXPECT(rhi_frame_begin(&fi) == RHI_OK, "rhi_frame_begin");
    EXPECT(fi.width == W && fi.height == H, "frame info %dx%d", fi.width, fi.height);
    EXPECT(fi.frame_index == 0, "first frame_index should be 0, got %llu", (unsigned long long)fi.frame_index);

    EXPECT(rhi_cmd_clear_main(CLEAR, 1.0f) == RHI_OK, "rhi_cmd_clear_main");

    /* Read a 4x4 block at the top-left corner and one pixel at the centre as RGBA8. */
    unsigned char rgba[4 * 4 * 4];
    EXPECT(rhi_read_pixels(0, 0, 4, 4, RHI_READ_RGBA8, rgba, 0) == RHI_OK, "read_pixels RGBA8 corner");
    int er = chan(CLEAR[0]), eg = chan(CLEAR[1]), eb = chan(CLEAR[2]), ea = chan(CLEAR[3]);
    printf("expected RGBA ~= %d,%d,%d,%d\n", er, eg, eb, ea);
    for (int i = 0; i < 16; i++) {
        const unsigned char *p = rgba + i * 4;
        EXPECT(near8(p[0], er) && near8(p[1], eg) && near8(p[2], eb) && near8(p[3], ea),
               "pixel %d = %u,%u,%u,%u", i, p[0], p[1], p[2], p[3]);
    }
    printf("corner pixel RGBA = %u,%u,%u,%u\n", rgba[0], rgba[1], rgba[2], rgba[3]);

    unsigned char ctr[4];
    EXPECT(rhi_read_pixels(W / 2, H / 2, 1, 1, RHI_READ_RGBA8, ctr, 0) == RHI_OK, "read_pixels RGBA8 centre");
    EXPECT(near8(ctr[0], er) && near8(ctr[1], eg) && near8(ctr[2], eb) && near8(ctr[3], ea),
           "centre pixel = %u,%u,%u,%u", ctr[0], ctr[1], ctr[2], ctr[3]);

    /* And an RGB8 read to exercise the 3-byte conversion path. */
    unsigned char rgb[3];
    EXPECT(rhi_read_pixels(0, 0, 1, 1, RHI_READ_RGB8, rgb, 0) == RHI_OK, "read_pixels RGB8");
    EXPECT(near8(rgb[0], er) && near8(rgb[1], eg) && near8(rgb[2], eb),
           "RGB8 pixel = %u,%u,%u", rgb[0], rgb[1], rgb[2]);

    EXPECT(rhi_frame_end() == RHI_OK, "rhi_frame_end");

    rhi_device_destroy();
    SDL_DestroyWindow(win);
    SDL_Quit();

    printf("validation messages: %u\n", rhi_vk_validation_messages);
    if (rhi_vk_validation_messages) { fprintf(stderr, "FAIL: %u validation message(s)\n", rhi_vk_validation_messages); fails++; }
    printf("%s\n", fails ? "SMOKE FAILED" : "SMOKE OK");
    return fails ? 1 : 0;
}
