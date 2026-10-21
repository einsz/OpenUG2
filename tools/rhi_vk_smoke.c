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

/* Non-static globals in rhi_vk.c (no rhi.h surface for them). */
extern unsigned rhi_vk_validation_messages;
/* Test-only backend path: reads buffer contents (STREAM mapping, or a STATIC
 * device->host staging copy). Returns 1 on success, 0 on failure. */
extern int rhi_vk_buffer_readback(RhiBuffer buf, size_t offset, size_t size, void *dst);
/* Test-only backend path: reads one uncompressed-colour texture mip/face back
 * as tightly packed RGBA8 (R at byte 0). Returns 1 on success, 0 on failure. */
extern int rhi_vk_texture_readback(RhiTexture tex, int32_t face, int32_t level, void *dst);

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

    /* ---- P11b: GPU buffers (STATIC + STREAM; create/update/replace/readback) ---- */
    /* Known byte patterns; i and 255-i are easy to eyeball on a mismatch. */
    unsigned char pat_a[64], pat_b[16], pat_c[128], got[128];
    for (int i = 0; i < 64; i++)  pat_a[i] = (unsigned char)i;
    for (int i = 0; i < 16; i++)  pat_b[i] = (unsigned char)(0xA0 + i);
    for (int i = 0; i < 128; i++) pat_c[i] = (unsigned char)(255 - i);

    /* STATIC VERTEX buffer created from pat_a, read back via a device->host copy. */
    RhiBufferDesc sdesc = { RHI_BUFFER_VERTEX, RHI_UPDATE_STATIC, sizeof pat_a, pat_a };
    RhiBuffer sb = rhi_buffer_create(&sdesc);
    EXPECT(sb.id != 0, "static buffer_create");
    memset(got, 0, sizeof got);
    EXPECT(rhi_vk_buffer_readback(sb, 0, sizeof pat_a, got), "static readback");
    EXPECT(memcmp(got, pat_a, sizeof pat_a) == 0, "static initial contents mismatch");

    /* Sub-range update at offset 16 (STATIC: stage + vkCmdCopyBuffer). */
    EXPECT(rhi_buffer_update(sb, 16, sizeof pat_b, pat_b) == RHI_OK, "static buffer_update");
    memset(got, 0, sizeof got);
    EXPECT(rhi_vk_buffer_readback(sb, 0, sizeof pat_a, got), "static readback after update");
    {
        unsigned char want[64]; memcpy(want, pat_a, 64); memcpy(want + 16, pat_b, 16);
        EXPECT(memcmp(got, want, 64) == 0, "static contents after sub-range update");
    }
    /* Out-of-range sub-range must be rejected, not silently clamped. */
    EXPECT(rhi_buffer_update(sb, 60, 8, pat_b) == RHI_ERR_INVALID, "static update past end rejected");

    /* Replace gives fresh, larger storage from pat_c. */
    EXPECT(rhi_buffer_replace(sb, sizeof pat_c, pat_c) == RHI_OK, "static buffer_replace");
    memset(got, 0, sizeof got);
    EXPECT(rhi_vk_buffer_readback(sb, 0, sizeof pat_c, got), "static readback after replace");
    EXPECT(memcmp(got, pat_c, sizeof pat_c) == 0, "static contents after replace");
    printf("static buffer: create/update/replace verified (%zu -> %zu bytes)\n",
           sizeof pat_a, sizeof pat_c);

    /* STREAM INDEX buffer, initially empty (size 0); the first replace allocates it. */
    RhiBufferDesc tdesc = { RHI_BUFFER_INDEX, RHI_UPDATE_STREAM, 0, NULL };
    RhiBuffer tb = rhi_buffer_create(&tdesc);
    EXPECT(tb.id != 0, "stream buffer_create (size 0)");
    EXPECT(rhi_buffer_replace(tb, sizeof pat_a, pat_a) == RHI_OK, "stream first replace allocates");
    memset(got, 0, sizeof got);
    EXPECT(rhi_vk_buffer_readback(tb, 0, sizeof pat_a, got), "stream readback via mapping");
    EXPECT(memcmp(got, pat_a, sizeof pat_a) == 0, "stream contents after replace");

    /* STREAM sub-range update (memcpy into the persistent mapping). */
    EXPECT(rhi_buffer_update(tb, 16, sizeof pat_b, pat_b) == RHI_OK, "stream buffer_update");
    memset(got, 0, sizeof got);
    EXPECT(rhi_vk_buffer_readback(tb, 0, sizeof pat_a, got), "stream readback after update");
    {
        unsigned char want[64]; memcpy(want, pat_a, 64); memcpy(want + 16, pat_b, 16);
        EXPECT(memcmp(got, want, 64) == 0, "stream contents after sub-range update");
    }
    printf("stream buffer: size-0 create + replace/update verified\n");

    /* An invalid description (two usage bits) is rejected like the GL backend. */
    RhiBufferDesc bad = { RHI_BUFFER_VERTEX | RHI_BUFFER_INDEX, RHI_UPDATE_STATIC, 16, pat_a };
    EXPECT(rhi_buffer_create(&bad).id == 0, "two usage bits rejected");

    rhi_buffer_destroy(sb);
    rhi_buffer_destroy(tb);
    /* A destroyed handle is stale; a read must fail (also logs a stale-handle line). */
    EXPECT(!rhi_vk_buffer_readback(sb, 0, 4, got), "readback of destroyed handle fails");

    /* ---- P11c: textures + samplers ---- */
    /* (a) RGBA8 2D SAMPLED 4x4: upload a known pattern to level 0, read it back. */
    RhiTextureDesc rgba_desc = { RHI_TEXTURE_2D, RHI_FORMAT_RGBA8, 4, 4, RHI_TEXTURE_SAMPLED };
    RhiTexture rgba_tex = rhi_texture_create(&rgba_desc);
    EXPECT(rgba_tex.id != 0, "rgba8 texture_create");
    EXPECT(rhi_texture_is_live(rgba_tex), "rgba8 texture is live");
    EXPECT(rhi_texture_native_id(rgba_tex) != 0, "rgba8 native id nonzero");
    unsigned char tx_rgba[4 * 4 * 4], tx_got[4 * 4 * 4];
    for (int i = 0; i < 4 * 4 * 4; i++) tx_rgba[i] = (unsigned char)((i * 7 + 3) & 0xff);
    EXPECT(rhi_texture_upload(rgba_tex, 0, 0, tx_rgba, sizeof tx_rgba) == RHI_OK, "rgba8 upload L0");
    memset(tx_got, 0, sizeof tx_got);
    EXPECT(rhi_vk_texture_readback(rgba_tex, 0, 0, tx_got), "rgba8 readback L0");
    EXPECT(memcmp(tx_got, tx_rgba, sizeof tx_rgba) == 0, "rgba8 contents mismatch");
    printf("rgba8 4x4 texture: upload/readback byte-exact\n");

    /* (b) RGB8 texture: upload 3-byte pixels, read back RGBA8 with A expanded to 255. */
    RhiTextureDesc rgb_desc = { RHI_TEXTURE_2D, RHI_FORMAT_RGB8, 4, 4, RHI_TEXTURE_SAMPLED };
    RhiTexture rgb_tex = rhi_texture_create(&rgb_desc);
    EXPECT(rgb_tex.id != 0, "rgb8 texture_create");
    unsigned char tx_rgb[4 * 4 * 3];
    for (int i = 0; i < 4 * 4 * 3; i++) tx_rgb[i] = (unsigned char)((i * 5 + 11) & 0xff);
    EXPECT(rhi_texture_upload(rgb_tex, 0, 0, tx_rgb, sizeof tx_rgb) == RHI_OK, "rgb8 upload L0");
    memset(tx_got, 0, sizeof tx_got);
    EXPECT(rhi_vk_texture_readback(rgb_tex, 0, 0, tx_got), "rgb8 readback L0");
    {
        int rgb_ok = 1;
        for (int p = 0; p < 16; p++) {
            if (tx_got[p*4] != tx_rgb[p*3] || tx_got[p*4+1] != tx_rgb[p*3+1] ||
                tx_got[p*4+2] != tx_rgb[p*3+2] || tx_got[p*4+3] != 255) rgb_ok = 0;
        }
        EXPECT(rgb_ok, "rgb8 -> rgba8 expansion (A=255) mismatch");
    }
    printf("rgb8 4x4 texture: RGB->RGBA8 expansion verified (A=255)\n");

    /* (c) BC1 texture: upload one 8-byte block; create+upload OK, no validation error. */
    RhiTextureDesc bc1_desc = { RHI_TEXTURE_2D, RHI_FORMAT_BC1, 4, 4, RHI_TEXTURE_SAMPLED };
    RhiTexture bc1_tex = rhi_texture_create(&bc1_desc);
    EXPECT(bc1_tex.id != 0, "bc1 texture_create");
    unsigned char bc1_block[8] = { 0x00, 0xF8, 0xE0, 0x07, 0x00, 0x55, 0xAA, 0xFF };
    EXPECT(rhi_texture_upload(bc1_tex, 0, 0, bc1_block, sizeof bc1_block) == RHI_OK, "bc1 upload one block");
    /* Undersized compressed upload (< the 4x4 BC1 block footprint) is rejected,
       not passed to a copy that would read past the staging buffer. */
    EXPECT(rhi_texture_upload(bc1_tex, 0, 0, bc1_block, 1) == RHI_ERR_INVALID, "bc1 undersized upload rejected");
    printf("bc1 4x4 texture: create + one-block upload OK\n");

    /* P11c review fixes: usage-only descriptions that rhi_texture_create accepts
       must not trip validation. Transfer-only (COPY_DST, no view-compatible
       usage) gets no image view; a non-sampled COLOR_TARGET upload rests in a
       layout the image's usage allows (not SHADER_READ_ONLY). */
    RhiTextureDesc copydst = { RHI_TEXTURE_2D, RHI_FORMAT_RGBA8, 8, 8, RHI_TEXTURE_COPY_DST };
    RhiTexture ct = rhi_texture_create(&copydst);
    EXPECT(ct.id != 0, "transfer-only (COPY_DST) texture_create");
    rhi_texture_destroy(ct);
    RhiTextureDesc rtonly = { RHI_TEXTURE_2D, RHI_FORMAT_RGBA8, 4, 4, RHI_TEXTURE_COLOR_TARGET };
    RhiTexture rt = rhi_texture_create(&rtonly);
    EXPECT(rt.id != 0, "colour-target-only texture_create");
    EXPECT(rhi_texture_upload(rt, 0, 0, tx_rgba, sizeof tx_rgba) == RHI_OK, "colour-target-only upload");
    rhi_texture_destroy(rt);
    printf("transfer-only + colour-target-only textures: no validation errors\n");

    /* (d) generate_mips on an RGBA8 texture (4x4 -> 3 levels). */
    EXPECT(rhi_texture_generate_mips(rgba_tex) == RHI_OK, "rgba8 generate_mips");
    printf("rgba8 generate_mips OK\n");

    /* (e) samplers: dedup by value, set/get round-trip, fresh texture default. */
    RhiSamplerDesc sd = { RHI_FILTER_LINEAR, RHI_FILTER_LINEAR, RHI_MIP_LINEAR,
                          RHI_WRAP_CLAMP, RHI_WRAP_CLAMP, 1.0f };
    RhiSampler smp1 = rhi_sampler_create(&sd);
    RhiSampler smp2 = rhi_sampler_create(&sd);
    EXPECT(smp1.id != 0, "sampler_create");
    EXPECT(smp1.id == smp2.id, "equal sampler descs dedup to the same id (%u vs %u)", smp1.id, smp2.id);
    RhiSamplerDesc sd_diff = sd; sd_diff.wrap_u = RHI_WRAP_REPEAT;
    RhiSampler smp3 = rhi_sampler_create(&sd_diff);
    EXPECT(smp3.id != 0 && smp3.id != smp1.id, "different sampler desc gets a different id");
    {
        const RhiSamplerDesc *rd = rhi_sampler_desc(smp1);
        EXPECT(rd && rd->wrap_u == RHI_WRAP_CLAMP && rd->mip_filter == RHI_MIP_LINEAR,
               "sampler_desc returns the stored description");
    }
    EXPECT(rhi_texture_set_sampler(rgba_tex, smp1) == RHI_OK, "texture_set_sampler");
    EXPECT(rhi_texture_sampler(rgba_tex).id == smp1.id, "texture_sampler round-trip");
    /* A fresh texture reports the documented default { NEAREST, LINEAR, LINEAR, REPEAT, 1 }. */
    RhiSamplerDesc def = { RHI_FILTER_NEAREST, RHI_FILTER_LINEAR, RHI_MIP_LINEAR,
                           RHI_WRAP_REPEAT, RHI_WRAP_REPEAT, 1.0f };
    RhiSampler def_id = rhi_sampler_create(&def);
    EXPECT(rhi_texture_sampler(rgb_tex).id == def_id.id, "fresh texture starts with the default sampler");
    printf("samplers: dedup + set/get + default verified\n");

    /* (f) negatives. */
    RhiTextureDesc d16_sampled = { RHI_TEXTURE_2D, RHI_FORMAT_D16, 4, 4,
                                   RHI_TEXTURE_SAMPLED | RHI_TEXTURE_DEPTH_TARGET };
    EXPECT(rhi_texture_create(&d16_sampled).id == 0, "D16 with SAMPLED rejected");
    EXPECT(rhi_texture_upload(rgba_tex, 0, 10, tx_rgba, sizeof tx_rgba) == RHI_ERR_INVALID,
           "upload past the mip count rejected");
    rhi_texture_destroy(bc1_tex);
    EXPECT(!rhi_texture_is_live(bc1_tex), "destroyed texture is not live");
    EXPECT(rhi_texture_native_id(bc1_tex) == 0, "destroyed texture native id is 0");
    EXPECT(rhi_texture_upload(bc1_tex, 0, 0, bc1_block, 8) == RHI_ERR_INVALID, "upload of destroyed handle safe");
    EXPECT(rhi_texture_generate_mips(bc1_tex) == RHI_ERR_INVALID, "generate_mips of destroyed handle safe");
    EXPECT(!rhi_vk_texture_readback(bc1_tex, 0, 0, tx_got), "readback of destroyed texture fails");
    rhi_texture_destroy(bc1_tex);   /* double destroy is a no-op */
    printf("texture/sampler negatives + destroyed-handle safety verified\n");

    rhi_texture_destroy(rgba_tex);
    rhi_texture_destroy(rgb_tex);

    rhi_device_destroy();
    SDL_DestroyWindow(win);
    SDL_Quit();

    printf("validation messages: %u\n", rhi_vk_validation_messages);
    if (rhi_vk_validation_messages) { fprintf(stderr, "FAIL: %u validation message(s)\n", rhi_vk_validation_messages); fails++; }
    printf("%s\n", fails ? "SMOKE FAILED" : "SMOKE OK");
    return fails ? 1 : 0;
}
