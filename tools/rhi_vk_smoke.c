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
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Non-static globals in rhi_vk.c (no rhi.h surface for them). */
extern unsigned rhi_vk_validation_messages;
extern unsigned rhi_vk_arena_flushes;            /* mid-frame UBO-arena flushes so far */
/* Test-only backend hook: recreate the per-frame UBO arena at `bytes` so the
 * multi-draw path can be forced to flush mid-frame with just a couple of draws. */
extern void rhi_vk_test_force_arena(size_t bytes);
extern void rhi_vk_test_force_stream_arena(size_t bytes);
extern void rhi_vk_test_fail_pending_alloc(unsigned n);       /* the next n deferred-free list growths fail */
extern int rhi_vk_test_sync_hazard(void);                    /* two unsynchronised clears (WAW) */   /* the STREAM-version arena */
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
static int is_red(const unsigned char *p)   { return p[0] == 255 && p[1] == 0 && p[2] == 0 && p[3] == 255; }
static int is_green(const unsigned char *p) { return p[0] == 0 && p[1] == 255 && p[2] == 0 && p[3] == 255; }
static int is_blue(const unsigned char *p)  { return p[0] == 0 && p[1] == 0 && p[2] == 255 && p[3] == 255; }
static int is_black(const unsigned char *p) { return p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 255; }

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

    /* P11d: shader create for a label whose Vulkan SPIR-V is embedded ("frontend").
       Param block matches FeParams (mat4 uMVP @0, vec4 uColor @64). */
    {
        static const RhiParamField fe_fields[] = {
            { "uMVP", RHI_PARAM_MAT4, 0, 1 }, { "uColor", RHI_PARAM_VEC4, 64, 1 },
        };
        static const RhiParamLayout fe_layout = { fe_fields, 2, 80, "FeParams" };
        static const char *const attrs[] = { "aPos" };
        static const RhiParamLayout *const blocks[] = { &fe_layout };
        RhiShaderDesc sd = { "", "", attrs, 1, "frontend", blocks, 1 };
        RhiShader sh = rhi_shader_create(&sd);
        EXPECT(sh.id != 0, "frontend shader_create");
        EXPECT(rhi_shader_native_program(sh) != 0, "frontend native program nonzero");
        void *pp = rhi_shader_params(sh, 0);
        EXPECT(pp != NULL, "frontend shader_params block 0");
        float probe[20];
        for (int i = 0; i < 20; i++) probe[i] = (float)(i * 3 + 1);
        memcpy(pp, probe, sizeof probe);
        EXPECT(memcmp(pp, probe, sizeof probe) == 0, "shader_params write/read round-trip");
        EXPECT(rhi_shader_params(sh, 1) == NULL, "out-of-range param block rejected");
        RhiShaderDesc bad = { "", "", attrs, 1, "no_such_label", blocks, 1 };
        EXPECT(rhi_shader_create(&bad).id == 0, "unknown-label shader rejected");
        RhiShaderDesc nullblocks = { "", "", attrs, 1, "frontend", NULL, 1 };
        EXPECT(rhi_shader_create(&nullblocks).id == 0, "NULL param_blocks rejected");

        /* Stale-handle safety: destroy sh, create sh2 (reuses the slot), then
           destroy the STALE sh again — sh2 must survive (generation check). */
        rhi_shader_destroy(sh);
        RhiShader sh2 = rhi_shader_create(&sd);
        EXPECT(sh2.id != 0, "frontend shader re-create (slot reuse)");
        rhi_shader_destroy(sh);                 /* stale handle to the reused slot */
        EXPECT(rhi_shader_params(sh2, 0) != NULL, "sh2 survives stale destroy of sh");
        rhi_shader_destroy(sh2);
        printf("frontend shader: create + params round-trip + unknown/NULL reject + stale-destroy safe\n");
    }

    rhi_texture_destroy(rgba_tex);
    rhi_texture_destroy(rgb_tex);

    /* ---- P11e: render the first real triangle (shader -> pipeline -> draw) ----
     * A red triangle covering the TOP-LEFT quadrant in GL-convention NDC (y up).
     * After read_pixels (which returns top-down rows, row 0 = top), the red must
     * land in the top-left of the image: that proves the negative-height viewport
     * reproduces GL's y-up orientation end to end. */
    {
        static const RhiParamField fe_fields[] = {
            { "uMVP", RHI_PARAM_MAT4, 0, 1 }, { "uColor", RHI_PARAM_VEC4, 64, 1 },
        };
        static const RhiParamLayout fe_layout = { fe_fields, 2, 80, "FeParams" };
        static const char *const attrs[] = { "aPos" };
        static const RhiParamLayout *const blocks[] = { &fe_layout };
        RhiShaderDesc shd = { "", "", attrs, 1, "frontend", blocks, 1 };
        RhiShader sh = rhi_shader_create(&shd);
        EXPECT(sh.id != 0, "P11e frontend shader_create");

        /* uMVP = identity (column-major), uColor = red. */
        float *fp = (float *)rhi_shader_params(sh, 0);
        EXPECT(fp != NULL, "P11e shader_params");
        if (fp) {
            memset(fp, 0, 80);
            fp[0] = fp[5] = fp[10] = fp[15] = 1.0f;     /* identity mat4 */
            fp[16] = 1.0f; fp[17] = 0.0f; fp[18] = 0.0f; fp[19] = 1.0f;   /* uColor = (1,0,0,1) */
        }

        RhiPipelineDesc pd;
        rhi_pipeline_desc_init(&pd);
        pd.shader = sh;
        pd.vertex.attrib_count = 1;
        pd.vertex.attribs[0] = (RhiVertexAttrib){ 0, 0, 0, RHI_VERTEX_FLOAT2 };
        pd.vertex.stride[0] = 8;
        pd.primitive = RHI_PRIMITIVE_TRIANGLES;
        pd.depth_test = 0; pd.depth_write = 0;
        pd.blend = 0; pd.cull = RHI_CULL_NONE;
        RhiPipeline pipe = rhi_pipeline_create(&pd);
        EXPECT(pipe.id != 0, "P11e pipeline_create");
        /* Dedup: an identical desc must return the same handle. */
        EXPECT(rhi_pipeline_create(&pd).id == pipe.id, "P11e pipeline dedup by value");

        /* Top-left triangle in GL NDC (y up): corner (-1,1), (0,1), (-1,0). */
        static const float tri[6] = { -1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f };
        RhiBufferDesc vbd = { RHI_BUFFER_VERTEX, RHI_UPDATE_STATIC, sizeof tri, tri };
        RhiBuffer vb = rhi_buffer_create(&vbd);
        EXPECT(vb.id != 0, "P11e vertex buffer_create");

        const float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        EXPECT(rhi_frame_begin(NULL) == RHI_OK, "P11e frame_begin");
        EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "P11e clear black");
        rhi_cmd_bind_pipeline(pipe);
        rhi_cmd_bind_vertex_buffer(0, vb, 0);
        rhi_cmd_draw(3, 0);

        unsigned char *frame = (unsigned char *)malloc((size_t)W * H * 4);
        EXPECT(frame != NULL, "P11e frame alloc");
        EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "P11e read_pixels");
        EXPECT(rhi_frame_end() == RHI_OK, "P11e frame_end");

        if (frame) {
            /* Sample the four corners (x, row), row 0 = top of the readback. */
            #define PX(x, row) (frame + ((size_t)(row) * W + (x)) * 4)
            const unsigned char *tl = PX(10, 10);          /* top-left */
            const unsigned char *tr = PX(W - 10, 10);      /* top-right */
            const unsigned char *bl = PX(10, H - 10);      /* bottom-left */
            const unsigned char *br = PX(W - 10, H - 10);  /* bottom-right */
            printf("P11e corners RGBA  top-left=%u,%u,%u,%u  top-right=%u,%u,%u,%u\n",
                   tl[0], tl[1], tl[2], tl[3], tr[0], tr[1], tr[2], tr[3]);
            printf("                   bottom-left=%u,%u,%u,%u  bottom-right=%u,%u,%u,%u\n",
                   bl[0], bl[1], bl[2], bl[3], br[0], br[1], br[2], br[3]);
            EXPECT(is_red(tl), "top-left must be red (triangle), got %u,%u,%u,%u", tl[0], tl[1], tl[2], tl[3]);
            EXPECT(is_black(br), "bottom-right (opposite corner) must be black, got %u,%u,%u,%u",
                   br[0], br[1], br[2], br[3]);
            /* Orientation: red at top, black at bottom (a Y-flip would swap these). */
            EXPECT(is_black(bl), "bottom-left must be black (Y-orientation), got %u,%u,%u,%u",
                   bl[0], bl[1], bl[2], bl[3]);
            EXPECT(is_black(tr), "top-right must be black (triangle is left-only), got %u,%u,%u,%u",
                   tr[0], tr[1], tr[2], tr[3]);
            printf("P11e triangle: red in TOP-LEFT, opposite corner black -> Y-orientation matches GL\n");
            #undef PX
        }
        free(frame);
        rhi_buffer_destroy(vb);
        rhi_shader_destroy(sh);
    }

    /* ---- P11f-3: cull winding matches GL ----
     * GL treats CCW (in y-up NDC) as front. The backend once declared Vulkan's
     * front face CCW too, but Vulkan measures winding in y-down framebuffer space,
     * so GL-CCW arrived as back-facing: CULL_BACK removed every outer car panel
     * (player body rendered black) while CULL_NONE draws looked right. */
    {
        static const RhiParamField fe_fields[] = {
            { "uMVP", RHI_PARAM_MAT4, 0, 1 }, { "uColor", RHI_PARAM_VEC4, 64, 1 },
        };
        static const RhiParamLayout fe_layout = { fe_fields, 2, 80, "FeParams" };
        static const char *const attrs[] = { "aPos" };
        static const RhiParamLayout *const blocks[] = { &fe_layout };
        RhiShaderDesc shd = { "", "", attrs, 1, "frontend", blocks, 1 };
        RhiShader sh = rhi_shader_create(&shd);
        EXPECT(sh.id != 0, "P11f-3 frontend shader_create");
        float *fp = (float *)rhi_shader_params(sh, 0);
        if (fp) {
            memset(fp, 0, 80);
            fp[0] = fp[5] = fp[10] = fp[15] = 1.0f;
            fp[16] = 1.0f; fp[19] = 1.0f;                       /* red */
        }
        /* Full-screen-ish triangles; centre pixel is inside each. */
        static const float ccw[6] = { -1.0f, -1.0f,   3.0f, -1.0f,   -1.0f, 3.0f };  /* CCW, y up */
        static const float cw[6]  = { -1.0f, -1.0f,   -1.0f, 3.0f,   3.0f, -1.0f };  /* CW, y up */
        RhiBuffer vb_ccw = rhi_buffer_create(&(RhiBufferDesc){ RHI_BUFFER_VERTEX, RHI_UPDATE_STATIC, sizeof ccw, ccw });
        RhiBuffer vb_cw  = rhi_buffer_create(&(RhiBufferDesc){ RHI_BUFFER_VERTEX, RHI_UPDATE_STATIC, sizeof cw, cw });
        struct { RhiCullMode cull; int use_cw; int want_red; const char *name; } cases[] = {
            { RHI_CULL_BACK,  0, 1, "CULL_BACK keeps a GL-CCW triangle" },
            { RHI_CULL_BACK,  1, 0, "CULL_BACK culls a GL-CW triangle" },
            { RHI_CULL_FRONT, 0, 0, "CULL_FRONT culls a GL-CCW triangle" },
            { RHI_CULL_FRONT, 1, 1, "CULL_FRONT keeps a GL-CW triangle" },
        };
        const float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
            RhiPipelineDesc pd;
            rhi_pipeline_desc_init(&pd);
            pd.shader = sh;
            pd.vertex.attrib_count = 1;
            pd.vertex.attribs[0] = (RhiVertexAttrib){ 0, 0, 0, RHI_VERTEX_FLOAT2 };
            pd.vertex.stride[0] = 8;
            pd.primitive = RHI_PRIMITIVE_TRIANGLES;
            pd.cull = cases[i].cull;
            RhiPipeline pipe = rhi_pipeline_create(&pd);
            unsigned char px[4] = {0};
            unsigned char *frame = (unsigned char *)malloc((size_t)W * H * 4);
            EXPECT(rhi_frame_begin(NULL) == RHI_OK, "P11f-3 frame_begin");
            rhi_cmd_clear_main(black, 1.0f);
            rhi_cmd_bind_pipeline(pipe);
            rhi_cmd_bind_vertex_buffer(0, cases[i].use_cw ? vb_cw : vb_ccw, 0);
            rhi_cmd_draw(3, 0);
            if (frame) {
                EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "P11f-3 read_pixels");
                memcpy(px, frame + ((size_t)(H / 2) * W + W / 2) * 4, 4);
            }
            EXPECT(rhi_frame_end() == RHI_OK, "P11f-3 frame_end");
            free(frame);
            EXPECT(cases[i].want_red ? is_red(px) : is_black(px), "P11f-3 %s, got %u,%u,%u,%u",
                   cases[i].name, px[0], px[1], px[2], px[3]);
        }
        printf("P11f-3 cull winding: BACK/FRONT agree with GL CCW-front convention\n");
        rhi_buffer_destroy(vb_ccw);
        rhi_buffer_destroy(vb_cw);
        rhi_shader_destroy(sh);
    }

    /* ---- P11 review: per-draw param snapshot, mid-frame flush, partial viewport ----
     * One shared frontend shader + triangle pipeline, driven immediate-mode: write
     * params -> draw -> write DIFFERENT params -> draw. Before Fix 1 both draws saw
     * the LAST params (single mapped UBO); after it each draw snapshots its own. */
    {
        static const RhiParamField fe_fields[] = {
            { "uMVP", RHI_PARAM_MAT4, 0, 1 }, { "uColor", RHI_PARAM_VEC4, 64, 1 },
        };
        static const RhiParamLayout fe_layout = { fe_fields, 2, 80, "FeParams" };
        static const char *const attrs[] = { "aPos" };
        static const RhiParamLayout *const blocks[] = { &fe_layout };
        RhiShaderDesc shd = { "", "", attrs, 1, "frontend", blocks, 1 };
        RhiShader sh = rhi_shader_create(&shd);
        EXPECT(sh.id != 0, "review frontend shader_create");

        RhiPipelineDesc pd;
        rhi_pipeline_desc_init(&pd);
        pd.shader = sh;
        pd.vertex.attrib_count = 1;
        pd.vertex.attribs[0] = (RhiVertexAttrib){ 0, 0, 0, RHI_VERTEX_FLOAT2 };
        pd.vertex.stride[0] = 8;
        pd.primitive = RHI_PRIMITIVE_TRIANGLES;
        pd.depth_test = 0; pd.depth_write = 0; pd.blend = 0; pd.cull = RHI_CULL_NONE;
        RhiPipeline pipe = rhi_pipeline_create(&pd);
        EXPECT(pipe.id != 0, "review pipeline_create");

        /* A quadrant-sized triangle in the top-left of GL NDC; a translate in uMVP
         * moves the second draw's copy to the bottom-right quadrant. */
        static const float tri[6] = { -1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f };
        RhiBufferDesc vbd = { RHI_BUFFER_VERTEX, RHI_UPDATE_STATIC, sizeof tri, tri };
        RhiBuffer vb = rhi_buffer_create(&vbd);
        EXPECT(vb.id != 0, "review vertex buffer_create");
        /* A full-NDC triangle (covers the whole viewport) for the partial-viewport test. */
        static const float big[6] = { -1.0f, -1.0f,   3.0f, -1.0f,   -1.0f, 3.0f };
        RhiBufferDesc bbd = { RHI_BUFFER_VERTEX, RHI_UPDATE_STATIC, sizeof big, big };
        RhiBuffer bvb = rhi_buffer_create(&bbd);
        EXPECT(bvb.id != 0, "review big-triangle buffer_create");

        const float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        unsigned char *frame = (unsigned char *)malloc((size_t)W * H * 4);
        EXPECT(frame != NULL, "review frame alloc");
        #define PX(x, row) (frame + ((size_t)(row) * W + (x)) * 4)

        /* Fill FeParams: identity+translate (column-major col 3) and a uColor. */
        #define SET_PARAMS(tx, ty, r, g, b) do { \
            float *fp = (float *)rhi_shader_params(sh, 0); \
            memset(fp, 0, 80); fp[0]=fp[5]=fp[10]=fp[15]=1.0f; fp[12]=(tx); fp[13]=(ty); \
            fp[16]=(r); fp[17]=(g); fp[18]=(b); fp[19]=1.0f; } while (0)

        /* (1) MULTI-DRAW different params: red TL (identity) + green BR (translate). */
        if (frame) {
            EXPECT(rhi_frame_begin(NULL) == RHI_OK, "multi-draw frame_begin");
            EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "multi-draw clear");
            rhi_cmd_bind_pipeline(pipe);
            rhi_cmd_bind_vertex_buffer(0, vb, 0);
            SET_PARAMS(0.0f, 0.0f, 1.0f, 0.0f, 0.0f);   /* red, top-left */
            rhi_cmd_draw(3, 0);
            SET_PARAMS(1.0f, -1.0f, 0.0f, 1.0f, 0.0f);  /* green, bottom-right */
            rhi_cmd_draw(3, 0);
            EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "multi-draw read");
            EXPECT(rhi_frame_end() == RHI_OK, "multi-draw frame_end");
            const unsigned char *red = PX(43, 32), *grn = PX(171, 128);
            printf("MULTI-DRAW  red-quadrant(43,32)=%u,%u,%u,%u  green-quadrant(171,128)=%u,%u,%u,%u\n",
                   red[0], red[1], red[2], red[3], grn[0], grn[1], grn[2], grn[3]);
            EXPECT(is_red(red), "top-left quadrant must be RED (per-draw snapshot), got %u,%u,%u,%u",
                   red[0], red[1], red[2], red[3]);
            EXPECT(is_green(grn), "bottom-right quadrant must be GREEN, got %u,%u,%u,%u",
                   grn[0], grn[1], grn[2], grn[3]);
            printf("MULTI-DRAW: two DIFFERENT params -> RED + GREEN quadrants (not both last colour)\n");
        }

        /* (2) PARTIAL VIEWPORT: bottom half in GL convention; full-NDC blue triangle
         * must land in the BOTTOM half of the returned image, top half untouched. */
        if (frame) {
            EXPECT(rhi_frame_begin(NULL) == RHI_OK, "partial-vp frame_begin");
            EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "partial-vp clear");
            EXPECT(rhi_cmd_set_viewport(0, 0, W, H / 2) == RHI_OK, "set bottom-half viewport");
            rhi_cmd_bind_pipeline(pipe);
            rhi_cmd_bind_vertex_buffer(0, bvb, 0);
            SET_PARAMS(0.0f, 0.0f, 0.0f, 0.0f, 1.0f);   /* blue */
            rhi_cmd_draw(3, 0);
            EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "partial-vp read");
            /* PARTIAL READBACK: x, y is the rectangle's TOP-left corner (rhi.h), like the
               GL backend. The bottom-half GL viewport is blue, so a read near the top of
               the target is black and one near the bottom is blue; and any sub-rectangle
               must equal the same crop of the full-frame read. */
            unsigned char pr_top[4], pr_bot[4];
            EXPECT(rhi_read_pixels(W / 2, H / 4, 1, 1, RHI_READ_RGBA8, pr_top, 0) == RHI_OK, "partial read top");
            EXPECT(rhi_read_pixels(W / 2, 3 * H / 4, 1, 1, RHI_READ_RGBA8, pr_bot, 0) == RHI_OK, "partial read bottom");
            printf("PARTIAL-READ  y=%d (top-down)=%u,%u,%u,%u  y=%d=%u,%u,%u,%u\n",
                   H / 4, pr_top[0], pr_top[1], pr_top[2], pr_top[3],
                   3 * H / 4, pr_bot[0], pr_bot[1], pr_bot[2], pr_bot[3]);
            EXPECT(is_black(pr_top), "sub-read in the top half must be black, got %u,%u,%u,%u",
                   pr_top[0], pr_top[1], pr_top[2], pr_top[3]);
            EXPECT(is_blue(pr_bot), "sub-read in the bottom half must be blue, got %u,%u,%u,%u",
                   pr_bot[0], pr_bot[1], pr_bot[2], pr_bot[3]);
            {   /* rectangles at the top edge, the bottom edge and straddling the viewport boundary */
                static const int32_t rects[4][4] = { { 0, 0, W, 2 }, { 5, H - 3, 40, 3 },
                                                     { 10, H / 2 - 8, 60, 16 }, { 17, 23, 31, 29 } };
                for (int i = 0; i < 4; i++) {
                    int32_t rx = rects[i][0], ry = rects[i][1], rw = rects[i][2], rh = rects[i][3];
                    unsigned char *sub = (unsigned char *)malloc((size_t)rw * rh * 4);
                    int same = sub && rhi_read_pixels(rx, ry, rw, rh, RHI_READ_RGBA8, sub, 0) == RHI_OK;
                    for (int32_t r = 0; same && r < rh; r++)
                        same = !memcmp(sub + (size_t)r * rw * 4, frame + ((size_t)(ry + r) * W + rx) * 4, (size_t)rw * 4);
                    EXPECT(same, "sub-rect read %d (%d,%d %dx%d) differs from the full-frame crop", i, rx, ry, rw, rh);
                    free(sub);
                }
            }
            EXPECT(rhi_frame_end() == RHI_OK, "partial-vp frame_end");
            EXPECT(rhi_cmd_set_viewport(0, 0, W, H) == RHI_OK, "restore full viewport");
            const unsigned char *bot = PX(W / 2, 3 * H / 4), *top = PX(W / 2, H / 4);
            printf("PARTIAL-VP  bottom(128,%d)=%u,%u,%u,%u  top(128,%d)=%u,%u,%u,%u\n",
                   3 * H / 4, bot[0], bot[1], bot[2], bot[3], H / 4, top[0], top[1], top[2], top[3]);
            EXPECT(is_blue(bot), "GL bottom-half viewport must colour the BOTTOM of the image, got %u,%u,%u,%u",
                   bot[0], bot[1], bot[2], bot[3]);
            EXPECT(is_black(top), "top half must be untouched (black), got %u,%u,%u,%u",
                   top[0], top[1], top[2], top[3]);
            printf("PARTIAL-VP: bottom-half GL viewport lands in image bottom half (matches GL)\n");
        }

        /* (3) MID-FRAME FLUSH: shrink the arena so the 2nd draw overflows it and
         * forces a submit+flush; the first draw's RED must still survive. */
        if (frame) {
            unsigned before = rhi_vk_arena_flushes;
            EXPECT(rhi_frame_begin(NULL) == RHI_OK, "flush-test frame_begin");
            EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "flush-test clear");
            rhi_vk_test_force_arena(128);   /* holds one 80-byte block, not two */
            rhi_cmd_bind_pipeline(pipe);
            rhi_cmd_bind_vertex_buffer(0, vb, 0);
            SET_PARAMS(0.0f, 0.0f, 1.0f, 0.0f, 0.0f);   /* red, top-left */
            rhi_cmd_draw(3, 0);
            SET_PARAMS(1.0f, -1.0f, 0.0f, 1.0f, 0.0f);  /* green, bottom-right (forces flush) */
            rhi_cmd_draw(3, 0);
            EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "flush-test read");
            EXPECT(rhi_frame_end() == RHI_OK, "flush-test frame_end");
            unsigned did = rhi_vk_arena_flushes - before;
            const unsigned char *red = PX(43, 32), *grn = PX(171, 128);
            printf("MID-FLUSH   flushes=%u  red(43,32)=%u,%u,%u,%u  green(171,128)=%u,%u,%u,%u\n",
                   did, red[0], red[1], red[2], red[3], grn[0], grn[1], grn[2], grn[3]);
            EXPECT(did > 0, "a mid-frame arena flush must have occurred");
            EXPECT(is_red(red), "earlier draw's RED must survive the mid-frame flush, got %u,%u,%u,%u",
                   red[0], red[1], red[2], red[3]);
            EXPECT(is_green(grn), "post-flush GREEN present, got %u,%u,%u,%u",
                   grn[0], grn[1], grn[2], grn[3]);
            printf("MID-FLUSH: arena overflow flushed mid-frame; earlier draw preserved (loadOp=LOAD)\n");
        }
        #undef SET_PARAMS
        #undef PX
        free(frame);
        rhi_buffer_destroy(vb);
        rhi_buffer_destroy(bvb);
        rhi_shader_destroy(sh);
    }

    /* ---- P11f: the real UG2 material shader + its 6 descriptor bindings ----
     * Translate src/shaders/ug2.* -> src/shaders/vk/ug2.* and drive one draw
     * through the full material pipeline. The UBO mirrors render_params.h's
     * UG2Params (size 828 = sizeof(UG2Params)); we declare only a few fields
     * (enough for rhi_param_layout_check) but write the block by byte offset.
     * This proves the ug2 pipeline + its UBO at binding 0 and the six samplers
     * at bindings 1..6 are wired validation-clean. Exact colours are checked
     * later by a full Vulkan-vs-GL scene screenshot; here we only require SOME
     * non-black output and 0 validation messages. */
    {
        /* The MID-FLUSH test above shrank the per-frame UBO arena (to 128 B, then
         * 256 B); the real device starts it at a few MB, big enough for any one
         * block. Restore a large arena so the 828-byte UG2 block fits in one
         * frame (outside a frame, as rhi_vk_test_force_arena requires). */
        rhi_vk_test_force_arena(1u << 20);

        /* Minimal layout: real block size + uMVP/uModel + one int sampler field. */
        static const RhiParamField ug2_fields[] = {
            { "uMVP",   RHI_PARAM_MAT4, 0,   1 },
            { "uModel", RHI_PARAM_MAT4, 64,  1 },
            { "uTex",   RHI_PARAM_INT,  804, 1 },   /* sampler-unit int, inside the block */
        };
        static const RhiParamLayout ug2_layout = { ug2_fields, 3, 828, "UG2Params" };
        EXPECT(rhi_param_layout_check(&ug2_layout) == RHI_OK, "ug2 layout check");
        static const char *const ug2_attrs[] = { "aPos", "aUV", "aNor", "aColor" };
        static const RhiParamLayout *const ug2_blocks[] = { &ug2_layout };
        RhiShaderDesc ug2d = { "", "", ug2_attrs, 4, "ug2", ug2_blocks, 1 };
        RhiShader sh = rhi_shader_create(&ug2d);
        EXPECT(sh.id != 0, "ug2 shader_create");
        EXPECT(rhi_shader_native_program(sh) != 0, "ug2 native program nonzero");

        float *fp = (float *)rhi_shader_params(sh, 0);
        EXPECT(fp != NULL, "ug2 shader_params block 0");
        if (fp) {
            memset(fp, 0, 828);
            fp[0] = fp[5] = fp[10] = fp[15] = 1.0f;            /* uMVP   = identity @0   */
            fp[16] = fp[21] = fp[26] = fp[31] = 1.0f;          /* uModel = identity @64  */
            fp[144] = 0.80f; fp[145] = 0.20f; fp[146] = 0.10f; /* uColor        @576 */
            /* fp[147] uVColor = 0 */
            fp[150] = 1.0f;                                    /* uLight = (0,0,1) @592 */
            /* fp[151] uUseTex = 0 -> base = uColor (deterministic) */
            fp[152] = 0.06f; fp[153] = 0.07f; fp[154] = 0.11f; /* uFogColor     @608 */
            /* fp[155] uFogDensity = 0 -> fog = 1 (no fog wash) */
            fp[158] = 10.0f;                                   /* uCamPos = (0,0,10) @624 */
            /* fp[163] uEnvReady = 0 (per spec) */
            fp[167] = 1.0f;                                    /* uAlpha        @668 */
            fp[180] = 0.30f;                                   /* uAmbient      @720 */
            fp[181] = 0.70f;                                   /* uDiffuse      @724 */
            fp[182] = 0.20f;                                   /* uEnv          @728 (samples uEnvCube) */
            fp[184] = 20.0f;                                   /* uGloss        @736 */
            /* fp[190] uRoadReflection = 0 (per spec); fp[198] uHeadGain = 0;
               fp[200] uHeadShadow = 0; uVista/uAlphaTest/uEmissiveTex/uFresnel/
               uClearcoat all 0 -> the main lit path, no discard. */
        }

        RhiPipelineDesc pd;
        rhi_pipeline_desc_init(&pd);
        pd.shader = sh;
        pd.vertex.attrib_count = 4;                            /* the engine's BatchedVertex */
        pd.vertex.attribs[0] = (RhiVertexAttrib){ 0, 0, 0,  RHI_VERTEX_FLOAT3 };
        pd.vertex.attribs[1] = (RhiVertexAttrib){ 1, 0, 12, RHI_VERTEX_FLOAT2 };
        pd.vertex.attribs[2] = (RhiVertexAttrib){ 2, 0, 20, RHI_VERTEX_FLOAT3 };
        pd.vertex.attribs[3] = (RhiVertexAttrib){ 3, 0, 32, RHI_VERTEX_UNORM8X4 };
        pd.vertex.stride[0] = 36;
        pd.primitive = RHI_PRIMITIVE_TRIANGLES;
        pd.depth_test = 1; pd.depth_write = 1;                 /* depth test on */
        pd.blend = 0; pd.cull = RHI_CULL_NONE;                 /* blend off */
        RhiPipeline pipe = rhi_pipeline_create(&pd);
        EXPECT(pipe.id != 0, "ug2 pipeline_create");

        /* One screen-filling BatchedVertex triangle (36 B/vertex): pos (z=0),
         * uv, normal (0,0,1 toward the camera), colour white. */
        struct BV { float pos[3]; float uv[2]; float nor[3]; unsigned char col[4]; };
        struct BV tri[3];
        memset(tri, 0, sizeof tri);
        const float P[3][2] = { { -1.0f, -1.0f }, { 3.0f, -1.0f }, { -1.0f, 3.0f } };
        for (int i = 0; i < 3; i++) {
            tri[i].pos[0] = P[i][0]; tri[i].pos[1] = P[i][1]; tri[i].pos[2] = 0.0f;
            tri[i].uv[0] = (P[i][0] + 1.0f) * 0.5f; tri[i].uv[1] = (P[i][1] + 1.0f) * 0.5f;
            tri[i].nor[0] = 0.0f; tri[i].nor[1] = 0.0f; tri[i].nor[2] = 1.0f;
            tri[i].col[0] = tri[i].col[1] = tri[i].col[2] = tri[i].col[3] = 255;
        }
        EXPECT(sizeof(struct BV) == 36, "BatchedVertex is 36 bytes (got %zu)", sizeof(struct BV));
        RhiBufferDesc vbd = { RHI_BUFFER_VERTEX, RHI_UPDATE_STATIC, sizeof tri, tri };
        RhiBuffer vb = rhi_buffer_create(&vbd);
        EXPECT(vb.id != 0, "ug2 vertex buffer_create");

        /* uTex -> an RGBA8 2D texture at slot 0; uEnvCube -> a CUBE at slot 5. */
        RhiTextureDesc t2d = { RHI_TEXTURE_2D, RHI_FORMAT_RGBA8, 4, 4, RHI_TEXTURE_SAMPLED };
        RhiTexture tex2d = rhi_texture_create(&t2d);
        EXPECT(tex2d.id != 0, "ug2 uTex (2D) create");
        unsigned char solid[4 * 4 * 4];
        for (int i = 0; i < 16; i++) { solid[i*4]=180; solid[i*4+1]=180; solid[i*4+2]=180; solid[i*4+3]=255; }
        EXPECT(rhi_texture_upload(tex2d, 0, 0, solid, sizeof solid) == RHI_OK, "ug2 uTex upload");

        RhiTextureDesc tcube = { RHI_TEXTURE_CUBE, RHI_FORMAT_RGBA8, 4, 4, RHI_TEXTURE_SAMPLED };
        RhiTexture cube = rhi_texture_create(&tcube);
        EXPECT(cube.id != 0, "ug2 uEnvCube (CUBE) create");
        for (int f = 0; f < 6; f++)
            EXPECT(rhi_texture_upload(cube, f, 0, solid, sizeof solid) == RHI_OK, "ug2 uEnvCube face upload");

        unsigned char *frame = (unsigned char *)malloc((size_t)W * H * 4);
        EXPECT(frame != NULL, "ug2 frame alloc");
        const float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        /* ---- P11f-1: a ug2 draw with NO cube bound (samplerCube slot 5 and every
         * other slot but uTex fall back to the dummies) must be validation-clean ---- */
        {
            unsigned before = rhi_vk_validation_messages;
            EXPECT(rhi_frame_begin(NULL) == RHI_OK, "no-cube frame_begin");
            EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "no-cube clear");
            rhi_cmd_bind_pipeline(pipe);
            rhi_cmd_bind_vertex_buffer(0, vb, 0);
            rhi_cmd_bind_texture(0, tex2d);
            rhi_cmd_draw(3, 0);
            EXPECT(frame && rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "no-cube read_pixels");
            EXPECT(rhi_frame_end() == RHI_OK, "no-cube frame_end");
            EXPECT(rhi_vk_validation_messages == before, "ug2 draw without a cube bound raised %u validation message(s)",
                   rhi_vk_validation_messages - before);
            const unsigned char *c0 = frame + ((size_t)(H / 2) * W + (W / 2)) * 4;
            EXPECT(c0[0] || c0[1] || c0[2], "no-cube draw must still be non-black");
            printf("NO-CUBE: ug2 draw with samplerCube unbound -> cube dummy, 0 new validation messages\n");
        }

        EXPECT(rhi_frame_begin(NULL) == RHI_OK, "ug2 frame_begin");
        EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "ug2 clear black");
        rhi_cmd_bind_pipeline(pipe);
        rhi_cmd_bind_vertex_buffer(0, vb, 0);
        rhi_cmd_bind_texture(0, tex2d);    /* uTex          (binding 1) */
        rhi_cmd_bind_texture(5, cube);     /* uEnvCube      (binding 6) */
        rhi_cmd_draw(3, 0);
        EXPECT(frame && rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "ug2 read_pixels");
        EXPECT(rhi_frame_end() == RHI_OK, "ug2 frame_end");

        if (frame) {
            const unsigned char *ctrp = frame + ((size_t)(H / 2) * W + (W / 2)) * 4;
            printf("UG2 centre pixel RGBA = %u,%u,%u,%u\n", ctrp[0], ctrp[1], ctrp[2], ctrp[3]);
            int nonblack = 0;
            for (int i = 0; i < W * H; i++) {
                const unsigned char *p = frame + (size_t)i * 4;
                if (p[0] || p[1] || p[2]) { nonblack++; }
            }
            printf("UG2 non-black pixels = %d / %d\n", nonblack, W * H);
            EXPECT(nonblack > 0, "ug2 draw must produce SOME non-black output");
            EXPECT(ctrp[0] || ctrp[1] || ctrp[2], "ug2 centre pixel must be non-black, got %u,%u,%u,%u",
                   ctrp[0], ctrp[1], ctrp[2], ctrp[3]);
            printf("UG2 material: 6-binding pipeline (UBO + uTex + uEnvCube) drew non-black, validation below\n");
        }
        /* ---- P11f-1: cube vs 2D on the same slot ----
         * Slot 5 is uEnvCube. A red cube + a BLUE 2D texture bound to slot 5 must
         * shade with the cube (red-shifted env), not the 2D one: the 2D binding of
         * a samplerCube slot is ignored and does not displace the cube binding. */
        if (frame && fp) {
            unsigned char redpx[4 * 4 * 4], bluepx[4 * 4 * 4];
            for (int i = 0; i < 16; i++) {
                redpx[i*4] = 255; redpx[i*4+1] = 0; redpx[i*4+2] = 0; redpx[i*4+3] = 255;
                bluepx[i*4] = 0; bluepx[i*4+1] = 0; bluepx[i*4+2] = 255; bluepx[i*4+3] = 255;
            }
            for (int f = 0; f < 6; f++)
                EXPECT(rhi_texture_upload(cube, f, 0, redpx, sizeof redpx) == RHI_OK, "red cube face upload");
            RhiTexture blue2d = rhi_texture_create(&t2d);
            EXPECT(blue2d.id != 0 && rhi_texture_upload(blue2d, 0, 0, bluepx, sizeof bluepx) == RHI_OK, "blue 2D");
            fp[144] = fp[145] = fp[146] = 0.5f;     /* grey base, so env tint is the only colour */
            fp[163] = 1.0f;                         /* uEnvReady */
            fp[182] = 1.0f;                         /* uEnv      */
            EXPECT(rhi_frame_begin(NULL) == RHI_OK, "cube/2D frame_begin");
            EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "cube/2D clear");
            rhi_cmd_bind_pipeline(pipe);
            rhi_cmd_bind_vertex_buffer(0, vb, 0);
            rhi_cmd_bind_texture(5, cube);          /* cube binding */
            rhi_cmd_bind_texture(5, blue2d);        /* 2D binding of the same unit: must not clobber the cube */
            rhi_cmd_draw(3, 0);
            EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "cube/2D read");
            EXPECT(rhi_frame_end() == RHI_OK, "cube/2D frame_end");
            const unsigned char *cp = frame + ((size_t)(H / 2) * W + (W / 2)) * 4;
            printf("CUBE-vs-2D centre RGBA = %u,%u,%u,%u\n", cp[0], cp[1], cp[2], cp[3]);
            EXPECT(cp[0] > cp[2] + 8, "cube (red) must win slot 5 over the 2D (blue): got %u,%u,%u", cp[0], cp[1], cp[2]);
            fp[163] = 0.0f; fp[182] = 0.20f;
            printf("CUBE-vs-2D: red cube + blue 2D on slot 5 -> red-shifted (cube binding kept, 2D ignored)\n");

            /* ---- upload-after-use + destroy-after-use + save/restore (uTex = slot 0) ---- */
            fp[151] = 1.0f;                         /* uUseTex: output = the texture */
            RhiTexture texA = rhi_texture_create(&t2d);
            EXPECT(texA.id != 0 && rhi_texture_upload(texA, 0, 0, redpx, sizeof redpx) == RHI_OK, "texA red");

            /* Upload after use: the first (left-half) draw was recorded before the
             * upload, so it must show the OLD (red) contents; the second sees blue. */
            EXPECT(rhi_frame_begin(NULL) == RHI_OK, "upload-after-use frame_begin");
            EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "upload-after-use clear");
            rhi_cmd_bind_pipeline(pipe);
            rhi_cmd_bind_vertex_buffer(0, vb, 0);
            rhi_cmd_bind_texture(0, texA);
            EXPECT(rhi_cmd_set_viewport(0, 0, W / 2, H) == RHI_OK, "left viewport");
            rhi_cmd_draw(3, 0);
            EXPECT(rhi_texture_upload(texA, 0, 0, bluepx, sizeof bluepx) == RHI_OK, "mid-frame upload");
            EXPECT(rhi_cmd_set_viewport(W / 2, 0, W / 2, H) == RHI_OK, "right viewport");
            rhi_cmd_draw(3, 0);
            EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "upload-after-use read");
            EXPECT(rhi_frame_end() == RHI_OK, "upload-after-use frame_end");
            const unsigned char *l = frame + ((size_t)(H / 2) * W + (W / 4)) * 4;
            const unsigned char *r = frame + ((size_t)(H / 2) * W + (3 * W / 4)) * 4;
            printf("UPLOAD-AFTER-USE left=%u,%u,%u right=%u,%u,%u\n", l[0], l[1], l[2], r[0], r[1], r[2]);
            EXPECT(l[0] > l[2] + 100, "draw before the upload must see the OLD (red) texture");
            EXPECT(r[2] > r[0] + 100, "draw after the upload must see the NEW (blue) texture");
            printf("UPLOAD-AFTER-USE: earlier draw saw the old contents, later draw the new\n");

            /* Destroy after use: the recorded draw still samples texA; the free is
             * deferred past the submit. */
            EXPECT(rhi_frame_begin(NULL) == RHI_OK, "destroy-after-use frame_begin");
            EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "destroy-after-use clear");
            rhi_cmd_bind_pipeline(pipe);
            rhi_cmd_bind_vertex_buffer(0, vb, 0);
            rhi_cmd_bind_texture(0, texA);
            EXPECT(rhi_cmd_set_viewport(0, 0, W, H) == RHI_OK, "full viewport");
            rhi_cmd_draw(3, 0);
            rhi_texture_destroy(texA);
            EXPECT(!rhi_texture_is_live(texA), "texA destroyed");
            EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "destroy-after-use read");
            EXPECT(rhi_frame_end() == RHI_OK, "destroy-after-use frame_end");
            const unsigned char *d0 = frame + ((size_t)(H / 2) * W + (W / 2)) * 4;
            EXPECT(d0[2] > d0[0] + 100, "draw before destroy must still show the texture (blue), got %u,%u,%u",
                   d0[0], d0[1], d0[2]);
            printf("DESTROY-AFTER-USE: same-frame destroy deferred, draw intact\n");

            /* State snapshot round trip. */
            RhiTexture texR = rhi_texture_create(&t2d), texB = rhi_texture_create(&t2d);
            EXPECT(rhi_texture_upload(texR, 0, 0, redpx, sizeof redpx) == RHI_OK, "texR");
            EXPECT(rhi_texture_upload(texB, 0, 0, bluepx, sizeof bluepx) == RHI_OK, "texB");
            EXPECT(rhi_frame_begin(NULL) == RHI_OK, "snapshot frame_begin");
            EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "snapshot clear");
            rhi_cmd_bind_pipeline(pipe);
            rhi_cmd_bind_vertex_buffer(0, vb, 0);
            rhi_cmd_bind_texture(0, texR);
            EXPECT(rhi_cmd_set_viewport(0, 0, W / 2, H) == RHI_OK, "snapshot left viewport");
            RhiStateSnapshot snap, snap2;
            rhi_cmd_save_state(&snap);
            /* a helper changes everything the snapshot covers */
            rhi_cmd_bind_texture(0, texB);
            rhi_cmd_bind_pipeline((RhiPipeline){0});       /* invalid -> no pipeline */
            EXPECT(rhi_cmd_set_viewport(W / 2, 0, W / 2, H) == RHI_OK, "snapshot right viewport");
            rhi_cmd_restore_state(&snap);
            int32_t vp[4]; rhi_viewport(vp);
            EXPECT(vp[0] == 0 && vp[1] == 0 && vp[2] == W / 2 && vp[3] == H, "restore must bring the viewport back, got %d,%d,%d,%d",
                   vp[0], vp[1], vp[2], vp[3]);
            rhi_cmd_draw(3, 0);                            /* pipeline + texR + left viewport restored */
            EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "snapshot read 1");
            const unsigned char *sl = frame + ((size_t)(H / 2) * W + (W / 4)) * 4;
            const unsigned char *sr = frame + ((size_t)(H / 2) * W + (3 * W / 4)) * 4;
            printf("SNAPSHOT left=%u,%u,%u right=%u,%u,%u\n", sl[0], sl[1], sl[2], sr[0], sr[1], sr[2]);
            EXPECT(sl[0] > sl[2] + 100, "restored texR draw must be red on the left");
            EXPECT(is_black(sr), "right half must stay untouched (restore put the viewport back)");
            /* cube textures are not accepted by rhi_state_set_texture; 2D ones are */
            snap2 = snap;
            rhi_state_set_texture(&snap2, 0, cube);        /* ignored: still texR */
            rhi_state_set_texture(&snap, 0, texB);         /* restore texB on slot 0 instead */
            EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "snapshot clear 2");
            rhi_cmd_bind_texture(0, texR);
            EXPECT(rhi_cmd_set_viewport(W / 2, 0, W / 2, H) == RHI_OK, "snapshot right viewport 2");
            rhi_cmd_restore_state(&snap2);
            rhi_cmd_draw(3, 0);
            EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "snapshot read 2");
            EXPECT(sl[0] > sl[2] + 100, "set_texture(cube) must be ignored (still red)");
            rhi_cmd_restore_state(&snap);
            EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "snapshot clear 3");
            rhi_cmd_restore_state(&snap);                   /* clear_main reset the viewport; restore brings it back */
            rhi_cmd_draw(3, 0);
            EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "snapshot read 3");
            EXPECT(rhi_frame_end() == RHI_OK, "snapshot frame_end");
            EXPECT(sl[2] > sl[0] + 100, "restored set_texture(blue) draw must be blue on the left");
            EXPECT(is_black(sr), "right half must stay untouched (3)");
            /* restore of a zeroed (never saved) snapshot is a no-op */
            RhiStateSnapshot zero; memset(&zero, 0, sizeof zero);
            rhi_cmd_restore_state(&zero);
            rhi_viewport(vp);
            EXPECT(vp[2] == W / 2, "restoring a never-saved snapshot must not change state");
            printf("SNAPSHOT: save/restore round trip (pipeline, viewport, 2D textures) + set_texture OK\n");
            rhi_cmd_set_viewport(0, 0, W, H);
            rhi_texture_destroy(texR); rhi_texture_destroy(texB);
            fp[151] = 0.0f;
            rhi_texture_destroy(blue2d);
        }

        /* ---- P11f-1b: a layout that omits an attribute the shader declares ----
         * Drop aColor (location 3) from the pipeline layout. The shader then reads
         * the default (0,0,0,1) for it, so with uVColor=2 (lit *= vColor.rgb) the
         * output is black, while the full layout (white aColor) stays bright. */
        if (frame && fp) {
            RhiPipelineDesc pn = pd;
            pn.vertex.attrib_count = 3;                  /* aPos, aUV, aNor only */
            unsigned before = rhi_vk_validation_messages;
            RhiPipeline pipe_nocol = rhi_pipeline_create(&pn);
            EXPECT(pipe_nocol.id != 0 && pipe_nocol.id != pipe.id, "pipeline without aColor");
            EXPECT(rhi_pipeline_create(&pn).id == pipe_nocol.id, "dedup still works for the reduced layout");
            fp[167] = 1.0f; fp[148] = 0.0f;
            fp[151] = 0.0f;
            fp[147] = 2.0f;                              /* uVColor: lit *= vColor.rgb */
            unsigned char centre[2][4];
            for (int k = 0; k < 2; k++) {
                EXPECT(rhi_frame_begin(NULL) == RHI_OK, "attr-default frame_begin");
                EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "attr-default clear");
                rhi_cmd_bind_pipeline(k ? pipe_nocol : pipe);
                rhi_cmd_bind_vertex_buffer(0, vb, 0);
                rhi_cmd_draw(3, 0);
                EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "attr-default read");
                EXPECT(rhi_frame_end() == RHI_OK, "attr-default frame_end");
                memcpy(centre[k], frame + ((size_t)(H / 2) * W + (W / 2)) * 4, 4);
            }
            printf("ATTR-DEFAULT full-layout=%u,%u,%u,%u  no-aColor=%u,%u,%u,%u\n",
                   centre[0][0], centre[0][1], centre[0][2], centre[0][3],
                   centre[1][0], centre[1][1], centre[1][2], centre[1][3]);
            EXPECT(centre[0][0] > 40, "full layout (white aColor) must be bright");
            EXPECT(centre[1][0] < 8 && centre[1][1] < 8 && centre[1][2] < 8 && centre[1][3] == 255,
                   "absent aColor must read (0,0,0,1) -> black, alpha 1");
            EXPECT(rhi_vk_validation_messages == before, "absent-attribute pipeline raised validation messages");
            fp[147] = 0.0f;
            printf("ATTR-DEFAULT: attribute absent from layout reads (0,0,0,1), 0 validation messages\n");
        }

        /* ---- P11f-1: render-target textures start defined (zero) ---- */
        {
            RhiTextureDesc rt = { RHI_TEXTURE_2D, RHI_FORMAT_RGBA8, 8, 8, RHI_TEXTURE_SAMPLED | RHI_TEXTURE_COLOR_TARGET };
            RhiTexture rtex = rhi_texture_create(&rt);
            unsigned char px[8 * 8 * 4];
            memset(px, 0x7f, sizeof px);
            EXPECT(rtex.id != 0 && rhi_vk_texture_readback(rtex, 0, 0, px), "render target readback");
            int allzero = 1;
            for (size_t i = 0; i < sizeof px; i++) if (px[i]) allzero = 0;
            EXPECT(allzero, "a fresh SAMPLED|COLOR_TARGET texture must read back zero");
            RhiTextureDesc dd = { RHI_TEXTURE_2D, RHI_FORMAT_DEPTH, 16, 16, RHI_TEXTURE_SAMPLED | RHI_TEXTURE_COPY_DST };
            RhiTexture dtex = rhi_texture_create(&dd);
            EXPECT(dtex.id != 0, "RHI_FORMAT_DEPTH texture create");
            /* sample both before they were ever rendered: bound to ug2 slots 0 and 3 */
            EXPECT(rhi_frame_begin(NULL) == RHI_OK, "rt frame_begin");
            EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "rt clear");
            rhi_cmd_bind_pipeline(pipe);
            rhi_cmd_bind_vertex_buffer(0, vb, 0);
            rhi_cmd_bind_texture(0, rtex);
            rhi_cmd_bind_texture(4, rtex);
            rhi_cmd_draw(3, 0);
            EXPECT(rhi_frame_end() == RHI_OK, "rt frame_end");
            rhi_cmd_bind_texture(0, (RhiTexture){0});
            rhi_cmd_bind_texture(4, (RhiTexture){0});
            rhi_texture_destroy(rtex);
            rhi_texture_destroy(dtex);
            printf("RENDER-TARGET-INIT: colour target reads zero; depth texture created; sampling before first render is clean\n");
        }

        /* ---- P11f-2: render passes, main-target copies ---- */
        if (frame && fp) {
            #define REDDISH(p)  ((p)[0] > 100 && (p)[1] < 60 && (p)[2] < 60)
            #define BLUEISH(p)  ((p)[2] > 100 && (p)[0] < 60 && (p)[1] < 60)
            #define DARK(p)     ((p)[0] < 12 && (p)[1] < 12 && (p)[2] < 12)
            unsigned vbefore = rhi_vk_validation_messages;
            unsigned char redpx[4 * 4 * 4], bluepx[4 * 4 * 4];
            for (int i = 0; i < 16; i++) {
                redpx[i*4] = 255; redpx[i*4+1] = 0; redpx[i*4+2] = 0; redpx[i*4+3] = 255;
                bluepx[i*4] = 0; bluepx[i*4+1] = 0; bluepx[i*4+2] = 255; bluepx[i*4+3] = 255;
            }
            RhiTexture texR = rhi_texture_create(&t2d), texB = rhi_texture_create(&t2d);
            EXPECT(rhi_texture_upload(texR, 0, 0, redpx, sizeof redpx) == RHI_OK, "P11f2 texR");
            EXPECT(rhi_texture_upload(texB, 0, 0, bluepx, sizeof bluepx) == RHI_OK, "P11f2 texB");
            fp[151] = 1.0f;                                    /* uUseTex: output = the texture */
            fp[182] = 0.0f;                                    /* uEnv off */
            RhiPipelineDesc pdl = pd; pdl.depth_test = 1; pdl.depth_write = 1; pdl.depth_compare = RHI_COMPARE_LESS;
            RhiPipeline pipeL = rhi_pipeline_create(&pdl);
            const RhiTextureDesc rtd = { RHI_TEXTURE_2D, RHI_FORMAT_RGBA8, 64, 32,
                                         RHI_TEXTURE_SAMPLED | RHI_TEXTURE_COLOR_TARGET };
            const RhiTextureDesc dd16 = { RHI_TEXTURE_2D, RHI_FORMAT_D16, 64, 32, RHI_TEXTURE_DEPTH_TARGET };
            RhiTexture rt = rhi_texture_create(&rtd), dep = rhi_texture_create(&dd16);
            EXPECT(rt.id && dep.id, "P11f2 pass targets");
            unsigned char px[64 * 32 * 4];
            #define RTPX(x, row) (px + ((size_t)(row) * 64 + (x)) * 4)

            /* (1) a pass with no draws still clears (colour AND depth). */
            EXPECT(rhi_frame_begin(NULL) == RHI_OK, "P11f2 frame_begin");
            {
                RhiPassDesc pass = { {rt, 0}, {dep, 0}, RHI_LOAD_CLEAR, RHI_LOAD_CLEAR, {0.2f, 0.4f, 0.6f, 1.0f}, 1.0f };
                EXPECT(rhi_cmd_begin_pass(&pass) == RHI_OK, "clear-only begin_pass");
                int32_t vp[4]; rhi_viewport(vp);
                EXPECT(vp[0] == 0 && vp[1] == 0 && vp[2] == 64 && vp[3] == 32, "begin_pass viewport = attachment size");
                rhi_cmd_end_pass();
                EXPECT(rhi_vk_texture_readback(rt, 0, 0, px), "clear-only readback");
                int ok = 1;
                for (int i = 0; i < 64 * 32; i++)
                    if (!near8(px[i*4], chan(0.2f)) || !near8(px[i*4+1], chan(0.4f)) ||
                        !near8(px[i*4+2], chan(0.6f)) || px[i*4+3] != 255) ok = 0;
                EXPECT(ok, "clear-only pass: texture must equal the clear colour");
                printf("PASS-CLEAR: draw-less pass cleared the texture to %u,%u,%u\n", px[0], px[1], px[2]);
            }

            /* (2) orientation end to end: draw red into the GL bottom-left quarter of
             * the pass target, then sample it in the main target. */
            rhi_cmd_bind_pipeline(pipeL);
            rhi_cmd_bind_vertex_buffer(0, vb, 0);
            rhi_cmd_bind_texture(0, texR);
            RhiStateSnapshot saved; rhi_cmd_save_state(&saved);
            EXPECT(rhi_cmd_set_viewport(0, 0, W, H) == RHI_OK, "main viewport");
            {
                RhiPassDesc pass = { {rt, 0}, {dep, 0}, RHI_LOAD_CLEAR, RHI_LOAD_CLEAR, {0, 0, 0, 1}, 1.0f };
                EXPECT(rhi_cmd_begin_pass(&pass) == RHI_OK, "orientation begin_pass");
                EXPECT(rhi_cmd_begin_pass(&pass) == RHI_ERR_INVALID, "nested begin_pass rejected");
                float cc[4] = {0, 0, 0, 1};
                EXPECT(rhi_cmd_clear_main(cc, 1.0f) == RHI_ERR_INVALID, "clear_main inside a pass rejected");
                EXPECT(rhi_read_pixels(0, 0, 4, 4, RHI_READ_RGBA8, px, 0) == RHI_ERR_INVALID, "read_pixels inside a pass rejected");
                EXPECT(rhi_cmd_copy_main_to_texture(texR, 0, 0, 4, 4) == RHI_ERR_INVALID, "copy inside a pass rejected");
                rhi_cmd_set_viewport(0, 0, 32, 16);
                rhi_cmd_draw(3, 0);
                rhi_cmd_end_pass();
                rhi_cmd_end_pass();   /* extra end_pass is a no-op */
            }
            EXPECT(rhi_vk_texture_readback(rt, 0, 0, px), "orientation readback");
            /* memory row == GL row: red at the low rows / low x */
            EXPECT(REDDISH(RTPX(8, 4)) && DARK(RTPX(40, 4)) && DARK(RTPX(8, 24)) && DARK(RTPX(40, 24)),
                   "pass draw landed in the wrong quarter: (8,4)=%u,%u,%u (40,4)=%u,%u,%u (8,24)=%u,%u,%u",
                   RTPX(8,4)[0], RTPX(8,4)[1], RTPX(8,4)[2], RTPX(40,4)[0], RTPX(40,4)[1], RTPX(40,4)[2],
                   RTPX(8,24)[0], RTPX(8,24)[1], RTPX(8,24)[2]);
            rhi_cmd_restore_state(&saved);
            rhi_cmd_bind_texture(0, rt);
            EXPECT(rhi_cmd_set_viewport(0, 0, W, H) == RHI_OK, "main viewport 2");
            EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "orientation main clear");
            rhi_cmd_bind_pipeline(pipeL);
            rhi_cmd_bind_vertex_buffer(0, vb, 0);
            rhi_cmd_draw(3, 0);
            EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "orientation main read");
            {
                #define FPX(x, row) (frame + ((size_t)(row) * W + (x)) * 4)
                const unsigned char *bl = FPX(W / 4, 3 * H / 4), *tl = FPX(W / 4, H / 4);
                const unsigned char *br = FPX(3 * W / 4, 3 * H / 4), *tr = FPX(3 * W / 4, H / 4);
                printf("PASS-ORIENT sampled in main: top-left=%u,%u,%u bottom-left=%u,%u,%u top-right=%u,%u,%u\n",
                       tl[0], tl[1], tl[2], bl[0], bl[1], bl[2], tr[0], tr[1], tr[2]);
                EXPECT(REDDISH(bl) && DARK(tl) && DARK(br) && DARK(tr),
                       "red must sample at the bottom-left of the main frame (GL orientation)");
            }
            EXPECT(rhi_frame_end() == RHI_OK, "P11f2 frame_end 1");

            /* (3) cube face N keeps the other faces; (4) back-to-back passes sharing a depth */
            {
                RhiTextureDesc cd = { RHI_TEXTURE_CUBE, RHI_FORMAT_RGBA8, 16, 16,
                                      RHI_TEXTURE_SAMPLED | RHI_TEXTURE_COLOR_TARGET };
                RhiTextureDesc cdd = { RHI_TEXTURE_2D, RHI_FORMAT_D16, 16, 16, RHI_TEXTURE_DEPTH_TARGET };
                RhiTexture ct = rhi_texture_create(&cd), cdep = rhi_texture_create(&cdd);
                EXPECT(ct.id && cdep.id, "cube RT create");
                EXPECT(rhi_frame_begin(NULL) == RHI_OK, "cube frame_begin");
                for (int f = 0; f < 6; f++) {
                    RhiPassDesc pass = { {ct, f}, {cdep, 0}, RHI_LOAD_CLEAR, RHI_LOAD_CLEAR,
                                         {(f & 1) ? 1.0f : 0.0f, (f & 2) ? 1.0f : 0.0f, (f & 4) ? 1.0f : 0.0f, 1}, 1.0f };
                    EXPECT(rhi_cmd_begin_pass(&pass) == RHI_OK, "cube face %d begin_pass", f);
                    rhi_cmd_end_pass();
                }
                RhiPassDesc p3 = { {ct, 3}, {cdep, 0}, RHI_LOAD_CLEAR, RHI_LOAD_CLEAR, {0.5f, 0.5f, 0.5f, 1}, 1.0f };
                EXPECT(rhi_cmd_begin_pass(&p3) == RHI_OK, "cube face 3 re-render");
                rhi_cmd_end_pass();
                int cube_ok = 1;
                for (int f = 0; f < 6; f++) {
                    unsigned char cpx[16 * 16 * 4];
                    EXPECT(rhi_vk_texture_readback(ct, f, 0, cpx), "cube readback face %d", f);
                    int want[3] = { f == 3 ? 128 : ((f & 1) ? 255 : 0), f == 3 ? 128 : ((f & 2) ? 255 : 0),
                                    f == 3 ? 128 : ((f & 4) ? 255 : 0) };
                    if (!near8(cpx[0], want[0]) || !near8(cpx[1], want[1]) || !near8(cpx[2], want[2]) ||
                        !near8(cpx[16*16*4-4], want[0])) cube_ok = 0;
                }
                EXPECT(cube_ok, "cube: re-rendering face 3 must leave the other faces intact");
                printf("PASS-CUBE: six faces rendered one by one, re-render of face 3 kept the other five\n");

                /* shared depth: pass A (clear depth) draws; pass B KEEPs the depth and
                 * redraws the same geometry -> LESS rejects it (colour stays clear); pass C
                 * clears the depth again and draws. */
                RhiTextureDesc cd2 = { RHI_TEXTURE_2D, RHI_FORMAT_RGBA8, 16, 16, RHI_TEXTURE_SAMPLED | RHI_TEXTURE_COLOR_TARGET };
                RhiTexture c2 = rhi_texture_create(&cd2);
                rhi_cmd_bind_pipeline(pipeL);
                rhi_cmd_bind_vertex_buffer(0, vb, 0);
                rhi_cmd_bind_texture(0, texR);
                unsigned char r3[3][16 * 16 * 4];
                for (int k = 0; k < 3; k++) {
                    RhiPassDesc pass = { {c2, 0}, {cdep, 0}, RHI_LOAD_CLEAR, k == 1 ? RHI_LOAD_KEEP : RHI_LOAD_CLEAR,
                                         {0, 0, 0, 1}, 1.0f };
                    EXPECT(rhi_cmd_begin_pass(&pass) == RHI_OK, "shared-depth pass %d", k);
                    rhi_cmd_draw(3, 0);
                    rhi_cmd_end_pass();
                    EXPECT(rhi_vk_texture_readback(c2, 0, 0, r3[k]), "shared-depth readback %d", k);
                }
                printf("PASS-DEPTH: A=%u B(keep)=%u C(clear)=%u (red channel at centre)\n",
                       r3[0][(8*16+8)*4], r3[1][(8*16+8)*4], r3[2][(8*16+8)*4]);
                EXPECT(REDDISH(&r3[0][(8*16+8)*4]) && DARK(&r3[1][(8*16+8)*4]) && REDDISH(&r3[2][(8*16+8)*4]),
                       "depth KEEP across back-to-back passes must reject the equal-depth redraw; CLEAR must not");
                rhi_texture_destroy(c2);
                rhi_texture_destroy(ct);
                rhi_texture_destroy(cdep);
                EXPECT(rhi_frame_end() == RHI_OK, "cube frame_end");
            }

            /* (5) forced mid-pass arena flush: earlier pass draws survive, no re-clear. */
            {
                rhi_vk_test_force_arena(1000);
                unsigned f0 = rhi_vk_arena_flushes;
                EXPECT(rhi_frame_begin(NULL) == RHI_OK, "flush frame_begin");
                RhiPassDesc pass = { {rt, 0}, {dep, 0}, RHI_LOAD_CLEAR, RHI_LOAD_CLEAR, {0, 0, 0, 1}, 1.0f };
                EXPECT(rhi_cmd_begin_pass(&pass) == RHI_OK, "flush begin_pass");
                rhi_cmd_bind_pipeline(pipeL);
                rhi_cmd_bind_vertex_buffer(0, vb, 0);
                rhi_cmd_bind_texture(0, texR);
                rhi_cmd_set_viewport(0, 0, 32, 32);
                rhi_cmd_draw(3, 0);
                rhi_cmd_bind_texture(0, texB);
                rhi_cmd_set_viewport(32, 0, 32, 32);
                rhi_cmd_draw(3, 0);          /* arena overflow -> mid-pass flush before this draw */
                rhi_cmd_end_pass();
                EXPECT(rhi_vk_texture_readback(rt, 0, 0, px), "flush readback");
                EXPECT(rhi_vk_arena_flushes > f0, "the arena did not flush (%u -> %u)", f0, rhi_vk_arena_flushes);
                printf("PASS-FLUSH: %u mid-pass flush(es); left=%u,%u,%u right=%u,%u,%u\n", rhi_vk_arena_flushes - f0,
                       RTPX(16,16)[0], RTPX(16,16)[1], RTPX(16,16)[2], RTPX(48,16)[0], RTPX(48,16)[1], RTPX(48,16)[2]);
                EXPECT(REDDISH(RTPX(16, 16)) && BLUEISH(RTPX(48, 16)),
                       "a mid-pass flush must keep the earlier draw (LOAD, not a re-clear)");
                EXPECT(rhi_frame_end() == RHI_OK, "flush frame_end");
                rhi_vk_test_force_arena(1u << 20);
            }

            /* (6) copy colour (partial rect) == read_pixels of the same rect; resize in place;
             * (7) copy depth; (8) restore_state ends a pass; (9) validation failures; (10) destroys */
            {
                EXPECT(rhi_frame_begin(NULL) == RHI_OK, "copy frame_begin");
                EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "copy clear");
                rhi_cmd_bind_pipeline(pipeL);
                rhi_cmd_bind_vertex_buffer(0, vb, 0);
                rhi_cmd_bind_texture(0, texR);
                rhi_cmd_set_viewport(0, 0, W / 2, H / 2);
                rhi_cmd_draw(3, 0);
                rhi_cmd_bind_texture(0, texB);
                rhi_cmd_set_viewport(W / 2, H / 2, W / 2, H / 2);
                rhi_cmd_draw(3, 0);
                rhi_cmd_set_viewport(W - 2, H - 2, 1, 1);   /* later "sampling" draws touch one pixel only */
                RhiTextureDesc cdst = { RHI_TEXTURE_2D, RHI_FORMAT_RGBA8, 4, 4, RHI_TEXTURE_SAMPLED | RHI_TEXTURE_COPY_DST };
                RhiTextureDesc ddst = { RHI_TEXTURE_2D, RHI_FORMAT_DEPTH, 4, 4, RHI_TEXTURE_SAMPLED | RHI_TEXTURE_COPY_DST };
                RhiTexture cc = rhi_texture_create(&cdst), dc = rhi_texture_create(&ddst);
                EXPECT(cc.id && dc.id, "copy destinations");
                /* sample the (4x4, never copied) destination before the resize so the old image is referenced */
                rhi_cmd_bind_texture(0, cc);
                rhi_cmd_draw(3, 0);
                const int cx = W / 4, cy = H / 4, cw = W / 2, ch = H / 2;
                EXPECT(rhi_cmd_copy_main_to_texture(cc, cx, cy, cw, ch) == RHI_OK, "copy colour (resizing)");
                unsigned char *tx = (unsigned char *)malloc((size_t)cw * ch * 4), *rp = (unsigned char *)malloc((size_t)cw * ch * 4);
                EXPECT(rhi_vk_texture_readback(cc, 0, 0, tx), "copy readback");
                /* the copy takes x, y from the bottom-left, read_pixels from the top-left (rhi.h) */
                EXPECT(rhi_read_pixels(cx, H - cy - ch, cw, ch, RHI_READ_RGBA8, rp, 0) == RHI_OK, "copy reference read_pixels");
                int same = 1;
                for (int r = 0; r < ch; r++)   /* texture rows are bottom-up (GL), read_pixels top-down */
                    if (memcmp(tx + (size_t)(ch - 1 - r) * cw * 4, rp + (size_t)r * cw * 4, (size_t)cw * 4)) same = 0;
                EXPECT(same, "copy_main_to_texture must equal read_pixels of the same rect");
                printf("COPY-COLOR: %dx%d rect at (%d,%d) equals read_pixels (texture took the rect size), centre=%u,%u,%u\n",
                       cw, ch, cx, cy, tx[((size_t)(ch/2) * cw + cw/2) * 4], tx[((size_t)(ch/2) * cw + cw/2) * 4 + 1],
                       tx[((size_t)(ch/2) * cw + cw/2) * 4 + 2]);
                /* the copy must contain both the red (low-left) and blue (high-right) parts */
                EXPECT(REDDISH(tx + ((size_t)10 * cw + 10) * 4) && BLUEISH(tx + ((size_t)(ch - 10) * cw + cw - 10) * 4),
                       "copied texture lost the asymmetric pattern");
                /* shrink the copy again: new size, matches again; the old image is freed deferred */
                rhi_cmd_bind_texture(0, cc);
                rhi_cmd_draw(3, 0);
                EXPECT(rhi_cmd_copy_main_to_texture(cc, 0, 0, 40, 20) == RHI_OK, "copy colour (shrinking)");
                unsigned char t2[40 * 20 * 4], r2[40 * 20 * 4];
                EXPECT(rhi_vk_texture_readback(cc, 0, 0, t2) && rhi_read_pixels(0, H - 20, 40, 20, RHI_READ_RGBA8, r2, 0) == RHI_OK, "copy2 reads");
                same = 1;
                for (int r = 0; r < 20; r++)
                    if (memcmp(t2 + (size_t)(19 - r) * 40 * 4, r2 + (size_t)r * 40 * 4, 40 * 4)) same = 0;
                EXPECT(same, "second (smaller) copy must equal read_pixels");
                /* codex P11f-2: a resized colour destination keeps its full mip chain, so mip
                 * generation after the copy really fills level 1 (40x20 -> 20x10; the lower-left
                 * of the main target is the uniform red quadrant). */
                {
                    unsigned char l1[20 * 10 * 4];
                    EXPECT(rhi_texture_generate_mips(cc) == RHI_OK, "generate_mips after a resizing copy");
                    EXPECT(rhi_vk_texture_readback(cc, 0, 1, l1), "level-1 readback after a resizing copy");
                    EXPECT(REDDISH(l1 + ((size_t)5 * 20 + 10) * 4), "level 1 must hold the downsampled copy, not nothing");
                    printf("COPY-MIPS: resized copy destination generates mip level 1 (%u,%u,%u)\n",
                           l1[((size_t)5 * 20 + 10) * 4], l1[((size_t)5 * 20 + 10) * 4 + 1], l1[((size_t)5 * 20 + 10) * 4 + 2]);
                }
                free(tx); free(rp);

                /* depth copy: the lower-left quarter was drawn at z=0 -> depth 0.5 (Vulkan range), the rest is 1 */
                EXPECT(rhi_cmd_copy_main_to_texture(dc, 0, 0, W, H) == RHI_OK, "copy depth");
                float *dv = (float *)malloc((size_t)W * H * sizeof(float));
                EXPECT(rhi_vk_texture_readback(dc, 0, 0, dv), "depth readback");
                #define DPX(x, row) dv[(size_t)(row) * W + (x)]
                printf("COPY-DEPTH ll=%.4f lr=%.4f ul=%.4f ur(blue quarter)=%.4f\n", DPX(W/4, H/4), DPX(3*W/4, H/4),
                       DPX(W/4, 3*H/4), DPX(3*W/4, 3*H/4));
                EXPECT(DPX(W/4, H/4) > 0.45f && DPX(W/4, H/4) < 0.55f && DPX(3*W/4, H/4) > 0.99f &&
                       DPX(W/4, 3*H/4) > 0.99f && DPX(3*W/4, 3*H/4) > 0.45f && DPX(3*W/4, 3*H/4) < 0.55f,
                       "copied depth wrong (memory row == GL row expected)");
                free(dv);
                rhi_cmd_bind_texture(0, texR);
                rhi_cmd_bind_texture(4, dc);    /* uReflectionDepth: sampling a depth copy must be valid */
                rhi_cmd_bind_texture(3, cc);
                rhi_cmd_draw(3, 0);
                rhi_cmd_bind_texture(4, (RhiTexture){0}); rhi_cmd_bind_texture(3, (RhiTexture){0});

                /* copy validation */
                EXPECT(rhi_cmd_copy_main_to_texture(texR, 0, 0, 4, 4) == RHI_ERR_INVALID, "copy into a non-COPY_DST texture rejected");
                EXPECT(rhi_cmd_copy_main_to_texture(cc, W - 4, 0, 8, 4) == RHI_ERR_INVALID, "copy rect outside the frame rejected");
                EXPECT(rhi_cmd_copy_main_to_texture(cc, 0, 0, 0, 4) == RHI_ERR_INVALID, "empty copy rect rejected");
                EXPECT(rhi_cmd_copy_main_to_texture((RhiTexture){0}, 0, 0, 4, 4) == RHI_ERR_INVALID, "copy into no texture rejected");

                /* pass validation */
                {
                    RhiTexture nontarget = texR;
                    RhiPassDesc bad = { {nontarget, 0}, {{0}, 0}, RHI_LOAD_CLEAR, RHI_LOAD_KEEP, {0}, 1 };
                    EXPECT(rhi_cmd_begin_pass(&bad) == RHI_ERR_INVALID, "begin_pass with a non-target colour rejected");
                    bad = (RhiPassDesc){ {rt, 1}, {{0}, 0}, RHI_LOAD_CLEAR, RHI_LOAD_KEEP, {0}, 1 };
                    EXPECT(rhi_cmd_begin_pass(&bad) == RHI_ERR_INVALID, "begin_pass with face 1 of a 2D target rejected");
                    bad = (RhiPassDesc){ {rt, 0}, {dep, 1}, RHI_LOAD_CLEAR, RHI_LOAD_KEEP, {0}, 1 };
                    EXPECT(rhi_cmd_begin_pass(&bad) == RHI_ERR_INVALID, "begin_pass with depth face 1 rejected");
                    bad = (RhiPassDesc){ {rt, 0}, {{0}, 0}, (RhiLoadOp)7, RHI_LOAD_KEEP, {0}, 1 };
                    EXPECT(rhi_cmd_begin_pass(&bad) == RHI_ERR_INVALID, "begin_pass with a bad load op rejected");
                    bad = (RhiPassDesc){ {{0}, 0}, {{0}, 0}, RHI_LOAD_CLEAR, RHI_LOAD_KEEP, {0}, 1 };
                    EXPECT(rhi_cmd_begin_pass(&bad) == RHI_ERR_INVALID, "begin_pass with no colour rejected");
                    EXPECT(rhi_cmd_begin_pass(NULL) == RHI_ERR_INVALID, "begin_pass(NULL) rejected");
                    RhiTextureDesc smalld = { RHI_TEXTURE_2D, RHI_FORMAT_D16, 8, 8, RHI_TEXTURE_DEPTH_TARGET };
                    RhiTexture sd8 = rhi_texture_create(&smalld);
                    bad = (RhiPassDesc){ {rt, 0}, {sd8, 0}, RHI_LOAD_CLEAR, RHI_LOAD_CLEAR, {0}, 1 };
                    EXPECT(rhi_cmd_begin_pass(&bad) == RHI_ERR_UNSUPPORTED, "depth smaller than colour -> UNSUPPORTED");
                    rhi_texture_destroy(sd8);
                }

                /* restore_state ends the pass and brings back pipeline/viewport/textures */
                rhi_cmd_bind_pipeline(pipeL);
                rhi_cmd_bind_texture(0, texR);
                rhi_cmd_set_viewport(0, 0, W / 2, H);
                RhiStateSnapshot sv; rhi_cmd_save_state(&sv);
                {
                    RhiPassDesc pass = { {rt, 0}, {{0}, 0}, RHI_LOAD_CLEAR, RHI_LOAD_KEEP, {0, 0, 0, 1}, 1.0f };
                    EXPECT(rhi_cmd_begin_pass(&pass) == RHI_OK, "restore-test begin_pass (no depth)");
                    rhi_cmd_bind_texture(0, texB);
                    rhi_cmd_bind_pipeline((RhiPipeline){0});
                    rhi_cmd_restore_state(&sv);
                    int32_t vp[4]; rhi_viewport(vp);
                    EXPECT(vp[2] == W / 2 && vp[3] == H, "restore after a pass restores the viewport, got %d,%d,%d,%d",
                           vp[0], vp[1], vp[2], vp[3]);
                    EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "restore_state must have ended the pass");
                    rhi_cmd_set_viewport(0, 0, W / 2, H);
                    rhi_cmd_draw(3, 0);     /* pipeline + texR from the snapshot, main target */
                    EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "restore read");
                    EXPECT(REDDISH(FPX(W / 4, H / 2)) && DARK(FPX(3 * W / 4, H / 2)),
                           "post-restore draw: red in the left half only");
                    printf("PASS-RESTORE: restore_state ended the pass; viewport/pipeline/texture back, main draw OK\n");
                }

                /* destroy inside a pass ends it; destroy right after a pass is deferred */
                {
                    RhiTexture tmp = rhi_texture_create(&rtd);
                    RhiPassDesc pass = { {tmp, 0}, {{0}, 0}, RHI_LOAD_CLEAR, RHI_LOAD_KEEP, {1, 1, 0, 1}, 1.0f };
                    EXPECT(rhi_cmd_begin_pass(&pass) == RHI_OK, "destroy-in-pass begin_pass");
                    rhi_cmd_draw(3, 0);
                    rhi_texture_destroy(tmp);
                    EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "destroying the attachment ended the pass");
                    RhiTexture tmp2 = rhi_texture_create(&rtd);
                    pass.color.texture = tmp2;
                    EXPECT(rhi_cmd_begin_pass(&pass) == RHI_OK, "destroy-after-pass begin_pass");
                    rhi_cmd_draw(3, 0);
                    rhi_cmd_end_pass();
                    rhi_texture_destroy(tmp2);
                    printf("PASS-DESTROY: destroy inside a pass ends it; destroy after a pass is deferred\n");
                }
                rhi_texture_destroy(cc);
                rhi_texture_destroy(dc);

                /* an open pass is closed by frame_end */
                {
                    RhiPassDesc pass = { {rt, 0}, {dep, 0}, RHI_LOAD_CLEAR, RHI_LOAD_CLEAR, {0, 0, 0, 1}, 1.0f };
                    EXPECT(rhi_cmd_begin_pass(&pass) == RHI_OK, "frame_end-in-pass begin_pass");
                    EXPECT(rhi_frame_end() == RHI_OK, "frame_end closes an open pass");
                    EXPECT(rhi_frame_begin(NULL) == RHI_OK, "frame after an open-pass frame_end");
                    EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "main target usable again");
                    EXPECT(rhi_frame_end() == RHI_OK, "last frame_end");
                }
            }
            rhi_texture_destroy(rt); rhi_texture_destroy(dep);
            rhi_texture_destroy(texR); rhi_texture_destroy(texB);
            fp[151] = 0.0f; fp[182] = 0.20f;
            EXPECT(rhi_vk_validation_messages == vbefore, "P11f-2 pass/copy tests raised %u validation message(s)",
                   rhi_vk_validation_messages - vbefore);
            #undef RTPX
            #undef FPX
            #undef DPX
            printf("P11f-2: passes, cube faces, shared depth, mid-pass flush, copies, restore, validation, destroys OK\n");
        }

        free(frame);
        rhi_texture_destroy(tex2d);
        rhi_texture_destroy(cube);
        rhi_buffer_destroy(vb);
        rhi_shader_destroy(sh);
    }

    /* ---- P11f-1: stencil on the main target ----
     * Draw A (colour writes off) INCRs the stencil in the top-left triangle; draw B
     * is full-screen green with stencil EQUAL 0, so A's footprint must stay black. */
    {
        static const RhiParamField fe_fields[] = {
            { "uMVP", RHI_PARAM_MAT4, 0, 1 }, { "uColor", RHI_PARAM_VEC4, 64, 1 },
        };
        static const RhiParamLayout fe_layout = { fe_fields, 2, 80, "FeParams" };
        static const char *const attrs[] = { "aPos" };
        static const RhiParamLayout *const blocks[] = { &fe_layout };
        RhiShaderDesc shd = { "", "", attrs, 1, "frontend", blocks, 1 };
        RhiShader sh = rhi_shader_create(&shd);
        EXPECT(sh.id != 0, "stencil frontend shader_create");
        RhiPipelineDesc pa;
        rhi_pipeline_desc_init(&pa);
        pa.shader = sh;
        pa.vertex.attrib_count = 1;
        pa.vertex.attribs[0] = (RhiVertexAttrib){ 0, 0, 0, RHI_VERTEX_FLOAT2 };
        pa.vertex.stride[0] = 8;
        pa.depth_write = 0;
        pa.color_write = 0;
        pa.stencil_test = 1;
        pa.stencil_compare = RHI_COMPARE_ALWAYS;
        pa.stencil_pass = RHI_STENCIL_INCR;
        RhiPipeline pipeA = rhi_pipeline_create(&pa);
        RhiPipelineDesc pb = pa;
        pb.color_write = 0xF;
        pb.stencil_compare = RHI_COMPARE_EQUAL;
        pb.stencil_ref = 0;
        pb.stencil_pass = RHI_STENCIL_KEEP;
        RhiPipeline pipeB = rhi_pipeline_create(&pb);
        EXPECT(pipeA.id && pipeB.id && pipeA.id != pipeB.id, "stencil pipelines");
        static const float tri[6] = { -1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f };
        static const float big[6] = { -1.0f, -1.0f,   3.0f, -1.0f,   -1.0f, 3.0f };
        RhiBufferDesc vbd = { RHI_BUFFER_VERTEX, RHI_UPDATE_STATIC, sizeof tri, tri };
        RhiBuffer vb = rhi_buffer_create(&vbd);
        RhiBufferDesc bbd = { RHI_BUFFER_VERTEX, RHI_UPDATE_STATIC, sizeof big, big };
        RhiBuffer bvb = rhi_buffer_create(&bbd);
        const float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        unsigned char *frame = (unsigned char *)malloc((size_t)W * H * 4);
        EXPECT(frame != NULL, "stencil frame alloc");
        if (frame) {
            float *fp = (float *)rhi_shader_params(sh, 0);
            memset(fp, 0, 80); fp[0] = fp[5] = fp[10] = fp[15] = 1.0f;
            fp[16] = 0.0f; fp[17] = 1.0f; fp[18] = 0.0f; fp[19] = 1.0f;   /* green */
            EXPECT(rhi_cmd_set_viewport(0, 0, W, H) == RHI_OK, "stencil viewport");
            EXPECT(rhi_frame_begin(NULL) == RHI_OK, "stencil frame_begin");
            EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "stencil clear (clears stencil to 0)");
            rhi_cmd_bind_pipeline(pipeA);
            rhi_cmd_bind_vertex_buffer(0, vb, 0);
            rhi_cmd_draw(3, 0);
            rhi_cmd_bind_pipeline(pipeB);
            rhi_cmd_bind_vertex_buffer(0, bvb, 0);
            rhi_cmd_draw(3, 0);
            EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "stencil read");
            EXPECT(rhi_frame_end() == RHI_OK, "stencil frame_end");
            const unsigned char *masked = frame + ((size_t)32 * W + 43) * 4;
            const unsigned char *open = frame + ((size_t)128 * W + 171) * 4;
            printf("STENCIL masked(43,32)=%u,%u,%u,%u open(171,128)=%u,%u,%u,%u\n",
                   masked[0], masked[1], masked[2], masked[3], open[0], open[1], open[2], open[3]);
            EXPECT(is_black(masked), "stencil-INCR footprint must be masked by EQUAL 0, got %u,%u,%u",
                   masked[0], masked[1], masked[2]);
            EXPECT(is_green(open), "outside the INCR footprint stencil is still 0 -> green, got %u,%u,%u",
                   open[0], open[1], open[2]);
            /* a second clear resets stencil to 0 so the same footprint is green again */
            EXPECT(rhi_frame_begin(NULL) == RHI_OK, "stencil2 frame_begin");
            EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "stencil2 clear");
            rhi_cmd_bind_pipeline(pipeB);
            rhi_cmd_bind_vertex_buffer(0, bvb, 0);
            rhi_cmd_draw(3, 0);
            EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "stencil2 read");
            EXPECT(rhi_frame_end() == RHI_OK, "stencil2 frame_end");
            EXPECT(is_green(frame + ((size_t)32 * W + 43) * 4), "after clear_main the stencil is 0 again");
            printf("STENCIL: INCR then EQUAL-0 draw is masked; clear_main resets stencil to 0\n");
        }
        free(frame);
        rhi_buffer_destroy(vb);
        rhi_buffer_destroy(bvb);
        rhi_shader_destroy(sh);
    }

    /* ---- P11g-1: wireframe + GPU timers ---- */
    {
        unsigned vbefore = rhi_vk_validation_messages;
        static const RhiParamField fe_fields[] = {
            { "uMVP", RHI_PARAM_MAT4, 0, 1 }, { "uColor", RHI_PARAM_VEC4, 64, 1 },
        };
        static const RhiParamLayout fe_layout = { fe_fields, 2, 80, "FeParams" };
        static const char *const attrs[] = { "aPos" };
        static const RhiParamLayout *const blocks[] = { &fe_layout };
        RhiShaderDesc shd = { "", "", attrs, 1, "frontend", blocks, 1 };
        RhiShader sh = rhi_shader_create(&shd);
        EXPECT(sh.id != 0, "wire shader_create");
        RhiPipelineDesc pd;
        rhi_pipeline_desc_init(&pd);
        pd.shader = sh;
        pd.vertex.attrib_count = 1;
        pd.vertex.attribs[0] = (RhiVertexAttrib){ 0, 0, 0, RHI_VERTEX_FLOAT2 };
        pd.vertex.stride[0] = 8;
        pd.depth_write = 0;
        RhiPipeline tris = rhi_pipeline_create(&pd);
        RhiPipelineDesc ld = pd;
        ld.primitive = RHI_PRIMITIVE_LINES;
        RhiPipeline lines = rhi_pipeline_create(&ld);
        EXPECT(tris.id && lines.id && tris.id != lines.id, "wire pipelines");
        static const float tri[6] = { -0.8f, -0.8f,   0.8f, -0.8f,   -0.8f, 0.8f };
        static const float seg[4] = { -0.8f, 0.5f,   0.8f, 0.5f };
        RhiBuffer vb = rhi_buffer_create(&(RhiBufferDesc){ RHI_BUFFER_VERTEX, RHI_UPDATE_STATIC, sizeof tri, tri });
        RhiBuffer sb = rhi_buffer_create(&(RhiBufferDesc){ RHI_BUFFER_VERTEX, RHI_UPDATE_STATIC, sizeof seg, seg });
        const float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        unsigned char *frame = (unsigned char *)malloc((size_t)W * H * 4);
        float *fp = (float *)rhi_shader_params(sh, 0);
        EXPECT(frame && fp, "wire alloc");
        if (frame && fp) {
            memset(fp, 0, 80); fp[0] = fp[5] = fp[10] = fp[15] = 1.0f;
            fp[16] = 0.0f; fp[17] = 1.0f; fp[18] = 0.0f; fp[19] = 1.0f;   /* green */
            EXPECT(rhi_cmd_set_viewport(0, 0, W, H) == RHI_OK, "wire viewport");
            EXPECT(rhi_caps()->wireframe, "lavapipe has fillModeNonSolid");
            RhiTimer tm = rhi_timer_create();
            EXPECT(rhi_caps()->gpu_timer && tm.id, "timer_create with caps.gpu_timer");
            uint64_t ns = 123;
            EXPECT(rhi_cmd_timer_begin(tm) == RHI_ERR_INVALID, "timer_begin outside a frame");
            EXPECT(rhi_frame_begin(NULL) == RHI_OK, "wire frame_begin");
            EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "wire clear");
            EXPECT(rhi_timer_read(tm, &ns) == 0, "never-measured timer is not ready");
            EXPECT(rhi_cmd_timer_end() == RHI_ERR_INVALID, "timer_end with none running");
            EXPECT(rhi_cmd_timer_begin(tm) == RHI_OK, "timer_begin");
            EXPECT(rhi_cmd_timer_begin(tm) == RHI_ERR_INVALID, "timer_begin while running");
            EXPECT(rhi_cmd_timer_begin((RhiTimer){ 0x7fffff }) == RHI_ERR_INVALID, "timer_begin bad handle");
            /* fill: the interior is lit */
            rhi_cmd_bind_pipeline(tris);
            rhi_cmd_bind_vertex_buffer(0, vb, 0);
            rhi_cmd_draw(3, 0);
            EXPECT(rhi_cmd_timer_end() == RHI_OK, "timer_end");
            EXPECT(rhi_timer_read(tm, &ns) == 0, "timer not ready while its stamps are unsubmitted");
            EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "wire fill read");
            const unsigned char *inside = frame + ((size_t)64 * W + 64) * 4;
            EXPECT(is_green(inside), "filled triangle interior should be green, got %u,%u,%u", inside[0], inside[1], inside[2]);
            /* the read forced a submit: the timer has a result now */
            EXPECT(rhi_timer_read(tm, &ns) == 1, "timer ready after the frame was submitted");
            printf("TIMER: draw measured %llu ns\n", (unsigned long long)ns);
            /* wireframe: interior dark, the bottom edge (y = -0.8 -> row ~172 of the top-down buffer) lit */
            EXPECT(rhi_cmd_set_wireframe(1) == RHI_OK, "set_wireframe(1)");
            EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "wire clear 2");
            rhi_cmd_bind_pipeline(tris);
            rhi_cmd_bind_vertex_buffer(0, vb, 0);
            rhi_cmd_draw(3, 0);
            /* a LINES pipeline is not changed by wireframe mode */
            rhi_cmd_bind_pipeline(lines);
            rhi_cmd_bind_vertex_buffer(0, sb, 0);
            rhi_cmd_draw(2, 0);
            EXPECT(rhi_cmd_set_wireframe(0) == RHI_OK, "set_wireframe(0)");
            EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "wire read");
            inside = frame + ((size_t)64 * W + 64) * 4;
            EXPECT(is_black(inside), "wireframe triangle interior must stay black, got %u,%u,%u", inside[0], inside[1], inside[2]);
            int edge_lit = 0, seg_lit = 0;
            for (int y = 167; y <= 177; y++) edge_lit += is_green(frame + ((size_t)y * W + 128) * 4);
            for (int y = 43; y <= 53; y++) seg_lit += is_green(frame + ((size_t)y * W + 128) * 4);
            printf("WIREFRAME: interior=%u,%u,%u edge pixels lit=%d line-pipeline pixels lit=%d\n",
                   inside[0], inside[1], inside[2], edge_lit, seg_lit);
            EXPECT(edge_lit >= 1, "wireframe must draw the triangle edge");
            EXPECT(seg_lit >= 1, "a LINES pipeline still draws in wireframe mode");
            /* wireframe off again: filled */
            EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "wire clear 3");
            rhi_cmd_bind_pipeline(tris);
            rhi_cmd_bind_vertex_buffer(0, vb, 0);
            rhi_cmd_draw(3, 0);
            EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "wire read 3");
            EXPECT(is_green(frame + ((size_t)64 * W + 64) * 4), "wireframe off draws filled again");
            /* a timer reused inside the same unsubmitted recording splits the frame */
            EXPECT(rhi_cmd_timer_begin(tm) == RHI_OK, "timer reuse begin");
            EXPECT(rhi_cmd_timer_end() == RHI_OK, "timer reuse end");
            EXPECT(rhi_cmd_timer_begin(tm) == RHI_OK, "timer reuse in the same recording (frame split)");
            EXPECT(rhi_cmd_timer_end() == RHI_OK, "timer reuse end 2");
            /* destroy + create recycles the query pair: still pending in this recording */
            {
                rhi_timer_destroy(tm);
                RhiTimer t2 = rhi_timer_create();
                EXPECT(t2.id && t2.id != tm.id, "recycled timer slot gets a new generation");
                EXPECT(rhi_cmd_timer_begin(t2) == RHI_OK, "begin on a recycled slot");
                EXPECT(rhi_cmd_timer_end() == RHI_OK, "end on a recycled slot");
                rhi_timer_destroy(t2);
                tm = rhi_timer_create();   /* the same slot again, queries pending in the recording */
            }
            EXPECT(rhi_cmd_timer_begin(tm) == RHI_OK, "begin on a re-created timer (frame split)");
            EXPECT(rhi_cmd_timer_end() == RHI_OK, "end on a re-created timer");
            EXPECT(rhi_frame_end() == RHI_OK, "wire frame_end");
            EXPECT(rhi_timer_read(tm, &ns) == 1, "timer ready after frame_end");
            rhi_timer_destroy(tm);
            EXPECT(rhi_timer_read(tm, &ns) == 0, "destroyed timer handle is stale");
            EXPECT(rhi_cmd_timer_begin(tm) == RHI_ERR_INVALID, "begin on a stale timer");
            printf("P11g-1: wireframe fill/edge/lines-unaffected, timers begin/end/read/reuse/stale OK\n");
        }
        free(frame);
        rhi_buffer_destroy(vb);
        rhi_buffer_destroy(sb);
        rhi_shader_destroy(sh);
        EXPECT(rhi_vk_validation_messages == vbefore, "P11g-1 tests raised %u validation message(s)",
               rhi_vk_validation_messages - vbefore);
    }

    /* ---- P11g-2: world_debug shader (BatchedVertex layout, 3 debug modes) ---- */
    {
        unsigned vbefore = rhi_vk_validation_messages;
        static const RhiParamField wd_fields[] = {
            { "u_MVP", RHI_PARAM_MAT4, 0, 1 }, { "u_DebugMode", RHI_PARAM_INT, 64, 1 },
        };
        static const RhiParamLayout wd_layout = { wd_fields, 2, 80, "WorldDebugParams" };
        static const char *const attrs[] = { "a_Position", "a_Normal", "a_UV0", "a_Color" };
        static const RhiParamLayout *const blocks[] = { &wd_layout };
        RhiShaderDesc shd = { "", "", attrs, 4, "world_debug", blocks, 1 };
        RhiShader sh = rhi_shader_create(&shd);
        EXPECT(sh.id != 0, "world_debug shader_create");
        RhiPipelineDesc pd;
        rhi_pipeline_desc_init(&pd);
        pd.shader = sh;
        pd.vertex.attribs[0] = (RhiVertexAttrib){ 0, 0, 0,  RHI_VERTEX_FLOAT3 };
        pd.vertex.attribs[1] = (RhiVertexAttrib){ 1, 0, 20, RHI_VERTEX_FLOAT3 };
        pd.vertex.attribs[2] = (RhiVertexAttrib){ 2, 0, 12, RHI_VERTEX_FLOAT2 };
        pd.vertex.attribs[3] = (RhiVertexAttrib){ 3, 0, 32, RHI_VERTEX_UNORM8X4 };
        pd.vertex.attrib_count = 4;
        pd.vertex.stride[0] = 36;
        RhiPipeline pipe = rhi_pipeline_create(&pd);
        EXPECT(pipe.id != 0, "world_debug pipeline");
        /* one big triangle covering the frame: pos, uv, normal (0,0,1), prelight (200,100,50) */
        float v[3][9];
        unsigned char col[4] = { 200, 100, 50, 255 };
        static const float pos[3][3] = { { -1, -1, 0 }, { 3, -1, 0 }, { -1, 3, 0 } };
        for (int i = 0; i < 3; i++) {
            memset(v[i], 0, sizeof v[i]);
            memcpy(&v[i][0], pos[i], 12);
            v[i][5] = 0.0f; v[i][6] = 0.0f; v[i][7] = 1.0f;
            memcpy(&v[i][8], col, 4);
        }
        RhiBuffer vb = rhi_buffer_create(&(RhiBufferDesc){ RHI_BUFFER_VERTEX, RHI_UPDATE_STATIC, sizeof v, v });
        const float black[4] = { 0, 0, 0, 1 };
        unsigned char *frame = (unsigned char *)malloc((size_t)W * H * 4);
        int32_t *wp = (int32_t *)rhi_shader_params(sh, 0);
        EXPECT(frame && wp && vb.id, "world_debug setup");
        if (frame && wp) {
            static const int want[3][3] = { { 200, 100, 50 }, { 128, 128, 255 }, { 217, 217, 230 } };
            for (int mode = 0; mode < 3; mode++) {
                memset(wp, 0, 80);
                float *m = (float *)wp; m[0] = m[5] = m[10] = m[15] = 1.0f;
                wp[16] = mode;
                EXPECT(rhi_cmd_set_viewport(0, 0, W, H) == RHI_OK, "world_debug viewport");
                EXPECT(rhi_frame_begin(NULL) == RHI_OK, "world_debug frame_begin");
                EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "world_debug clear");
                rhi_cmd_bind_pipeline(pipe);
                rhi_cmd_bind_vertex_buffer(0, vb, 0);
                rhi_cmd_draw(3, 0);
                EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "world_debug read");
                EXPECT(rhi_frame_end() == RHI_OK, "world_debug frame_end");
                const unsigned char *px = frame + ((size_t)96 * W + 128) * 4;
                printf("WORLD-DEBUG mode %d centre=%u,%u,%u\n", mode, px[0], px[1], px[2]);
                EXPECT(near8(px[0], want[mode][0]) && near8(px[1], want[mode][1]) && near8(px[2], want[mode][2]),
                       "world_debug mode %d wants %d,%d,%d got %u,%u,%u", mode,
                       want[mode][0], want[mode][1], want[mode][2], px[0], px[1], px[2]);
            }
        }
        free(frame);
        rhi_buffer_destroy(vb);
        rhi_shader_destroy(sh);
        EXPECT(rhi_vk_validation_messages == vbefore, "P11g-2 tests raised %u validation message(s)",
               rhi_vk_validation_messages - vbefore);
        printf("P11g-2: world_debug prelight / normals / solid modes OK\n");
    }

    /* ---- P11g-3: STREAM buffers updated between draws (hud pattern) ---- */
    {
        unsigned vbefore = rhi_vk_validation_messages;
        static const RhiParamField fe_fields[] = {
            { "uMVP", RHI_PARAM_MAT4, 0, 1 }, { "uColor", RHI_PARAM_VEC4, 64, 1 },
        };
        static const RhiParamLayout fe_layout = { fe_fields, 2, 80, "FeParams" };
        static const char *const attrs[] = { "aPos" };
        static const RhiParamLayout *const blocks[] = { &fe_layout };
        RhiShaderDesc shd = { "", "", attrs, 1, "frontend", blocks, 1 };
        RhiShader sh = rhi_shader_create(&shd);
        RhiPipelineDesc pd;
        rhi_pipeline_desc_init(&pd);
        pd.shader = sh;
        pd.vertex.attrib_count = 1;
        pd.vertex.attribs[0] = (RhiVertexAttrib){ 0, 0, 0, RHI_VERTEX_FLOAT2 };
        pd.vertex.stride[0] = 8;
        RhiPipeline pipe = rhi_pipeline_create(&pd);
        EXPECT(sh.id && pipe.id, "stream test shader/pipeline");
        /* left (x -1..0) and right (x 0..1) full-height quads, two triangles each */
        static const float left[12]  = { -1,-1, 0,-1, -1,1,  -1,1, 0,-1, 0,1 };
        static const float right[12] = {  0,-1, 1,-1,  0,1,   0,1, 1,-1, 1,1 };
        static const float quads[16] = { -1,-1, 0,-1, -1,1, 0,1,   0,-1, 1,-1, 0,1, 1,1 };
        static const uint16_t idx_left[6]  = { 0, 1, 2, 2, 1, 3 };
        static const uint16_t idx_right[6] = { 4, 5, 6, 6, 5, 7 };
        const float black[4] = { 0, 0, 0, 1 };
        unsigned char *frame = (unsigned char *)malloc((size_t)W * H * 4);
        float *fp = (float *)rhi_shader_params(sh, 0);
        RhiBuffer vb = rhi_buffer_create(&(RhiBufferDesc){ RHI_BUFFER_VERTEX, RHI_UPDATE_STREAM, sizeof left, left });
        RhiBuffer qb = rhi_buffer_create(&(RhiBufferDesc){ RHI_BUFFER_VERTEX, RHI_UPDATE_STATIC, sizeof quads, quads });
        RhiBuffer ib = rhi_buffer_create(&(RhiBufferDesc){ RHI_BUFFER_INDEX, RHI_UPDATE_STREAM, sizeof idx_left, idx_left });
        EXPECT(frame && fp && vb.id && qb.id && ib.id, "stream test setup");
        if (frame && fp && vb.id && qb.id && ib.id) {
            memset(fp, 0, 80); fp[0] = fp[5] = fp[10] = fp[15] = 1.0f;
            EXPECT(rhi_cmd_set_viewport(0, 0, W, H) == RHI_OK, "stream viewport");
            #define COLOUR(r, g, b) do { fp[16] = (r); fp[17] = (g); fp[18] = (b); fp[19] = 1.0f; } while (0)
            #define PXAT(x, y) (frame + ((size_t)(y) * W + (x)) * 4)

            /* 1. update between two draws: each draw shows the contents it was issued with */
            EXPECT(rhi_frame_begin(NULL) == RHI_OK, "stream frame_begin");
            EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "stream clear");
            rhi_cmd_bind_pipeline(pipe); rhi_cmd_bind_vertex_buffer(0, vb, 0);
            COLOUR(1, 0, 0); rhi_cmd_draw(6, 0);
            EXPECT(rhi_buffer_update(vb, 0, sizeof right, right) == RHI_OK, "stream update between draws");
            COLOUR(0, 1, 0); rhi_cmd_draw(6, 0);
            EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "stream read 1");
            printf("STREAM-1 left=%u,%u,%u right=%u,%u,%u\n", PXAT(64, 96)[0], PXAT(64, 96)[1], PXAT(64, 96)[2],
                   PXAT(192, 96)[0], PXAT(192, 96)[1], PXAT(192, 96)[2]);
            EXPECT(is_red(PXAT(64, 96)), "first draw keeps the contents it was issued with (left red)");
            EXPECT(is_green(PXAT(192, 96)), "second draw sees the update (right green)");
            float back[12];
            EXPECT(rhi_vk_buffer_readback(vb, 0, sizeof back, back) == 1 && !memcmp(back, right, sizeof back),
                   "readback sees the latest update");
            EXPECT(rhi_frame_end() == RHI_OK, "stream frame_end");
            EXPECT(rhi_vk_buffer_readback(vb, 0, sizeof back, back) == 1 && !memcmp(back, right, sizeof back),
                   "after frame_end the buffer's own storage holds the last update");

            /* 2. a full arena: the update falls back to submit-then-write-in-place */
            rhi_vk_test_force_stream_arena(64);   /* one 48-byte version, not two */
            EXPECT(rhi_buffer_update(vb, 0, sizeof left, left) == RHI_OK, "stream reset to left");
            EXPECT(rhi_frame_begin(NULL) == RHI_OK, "stream2 frame_begin");
            EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "stream2 clear");
            rhi_cmd_bind_pipeline(pipe); rhi_cmd_bind_vertex_buffer(0, vb, 0);
            COLOUR(1, 0, 0); rhi_cmd_draw(6, 0);                                  /* left red */
            EXPECT(rhi_buffer_update(vb, 0, sizeof right, right) == RHI_OK, "stream2 update 1 (arena copy)");
            COLOUR(0, 1, 0); rhi_cmd_draw(6, 0);                                  /* right green */
            EXPECT(rhi_buffer_update(vb, 0, sizeof left, left) == RHI_OK, "stream2 update 2 (arena full)");
            COLOUR(0, 0, 1); rhi_cmd_draw(6, 0);                                  /* left blue over red */
            EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "stream read 2");
            EXPECT(is_blue(PXAT(64, 96)), "third draw (after the arena-full split) drew left in blue");
            EXPECT(is_green(PXAT(192, 96)), "the second draw survived the split");
            EXPECT(rhi_frame_end() == RHI_OK, "stream2 frame_end");

            /* 3. an index STREAM buffer updated between indexed draws */
            EXPECT(rhi_frame_begin(NULL) == RHI_OK, "stream3 frame_begin");   /* demand grew the arena */
            EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "stream3 clear");
            rhi_cmd_bind_pipeline(pipe); rhi_cmd_bind_vertex_buffer(0, qb, 0); rhi_cmd_bind_index_buffer(ib);
            COLOUR(1, 0, 0); rhi_cmd_draw_indexed(6, 0);                          /* left red */
            EXPECT(rhi_buffer_update(ib, 0, sizeof idx_right, idx_right) == RHI_OK, "stream3 index update");
            COLOUR(0, 1, 0); rhi_cmd_draw_indexed(6, 0);                          /* right green */
            EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "stream read 3");
            EXPECT(is_red(PXAT(64, 96)) && is_green(PXAT(192, 96)), "index stream update between draws");
            EXPECT(rhi_frame_end() == RHI_OK, "stream3 frame_end");

            /* 4. replace + destroy while a draw is recorded */
            EXPECT(rhi_frame_begin(NULL) == RHI_OK, "stream4 frame_begin");
            EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "stream4 clear");
            rhi_cmd_bind_pipeline(pipe); rhi_cmd_bind_vertex_buffer(0, vb, 0);
            COLOUR(1, 0, 0); rhi_cmd_draw(6, 0);
            EXPECT(rhi_buffer_update(vb, 0, sizeof right, right) == RHI_OK, "stream4 update");
            EXPECT(rhi_buffer_replace(vb, sizeof left, left) == RHI_OK, "stream4 replace after an arena copy");
            COLOUR(0, 0, 1); rhi_cmd_draw(6, 0);
            EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "stream read 4");
            EXPECT(is_blue(PXAT(64, 96)), "draw after replace uses the new storage (left)");
            EXPECT(is_black(PXAT(192, 96)), "the replaced contents are not the arena copy (right stays black)");
            EXPECT(rhi_frame_end() == RHI_OK, "stream4 frame_end");
            #undef COLOUR
            #undef PXAT
            printf("P11g-3: STREAM vertex/index updates between draws, arena-full split, replace after copy OK\n");
        }
        free(frame);
        rhi_buffer_destroy(vb); rhi_buffer_destroy(qb); rhi_buffer_destroy(ib);
        rhi_shader_destroy(sh);
        EXPECT(rhi_vk_validation_messages == vbefore, "P11g-3 tests raised %u validation message(s)",
               rhi_vk_validation_messages - vbefore);
    }

    /* ---- review follow-ups F2/F3/F4 (accepted after the #105 review): STATIC updates, deferred-free failure, oversized blocks ---- */
    {
        unsigned vbefore = rhi_vk_validation_messages;
        static const RhiParamField fe_fields[] = {
            { "uMVP", RHI_PARAM_MAT4, 0, 1 }, { "uColor", RHI_PARAM_VEC4, 64, 1 },
        };
        static const RhiParamLayout fe_layout = { fe_fields, 2, 80, "FeParams" };
        static const char *const attrs[] = { "aPos" };
        static const RhiParamLayout *const blocks[] = { &fe_layout };
        RhiShaderDesc shd = { "", "", attrs, 1, "frontend", blocks, 1 };
        RhiShader sh = rhi_shader_create(&shd);
        RhiPipelineDesc pd;
        rhi_pipeline_desc_init(&pd);
        pd.shader = sh;
        pd.vertex.attrib_count = 1;
        pd.vertex.attribs[0] = (RhiVertexAttrib){ 0, 0, 0, RHI_VERTEX_FLOAT2 };
        pd.vertex.stride[0] = 8;
        RhiPipeline pipe = rhi_pipeline_create(&pd);
        EXPECT(sh.id && pipe.id, "follow-up shader/pipeline");
        static const float left[12]  = { -1,-1, 0,-1, -1,1,  -1,1, 0,-1, 0,1 };
        static const float right[12] = {  0,-1, 1,-1,  0,1,   0,1, 1,-1, 1,1 };
        static const float quads[16] = { -1,-1, 0,-1, -1,1, 0,1,   0,-1, 1,-1, 0,1, 1,1 };
        static const uint16_t idx_left[6]  = { 0, 1, 2, 2, 1, 3 };
        static const uint16_t idx_right[6] = { 4, 5, 6, 6, 5, 7 };
        const float black[4] = { 0, 0, 0, 1 };
        unsigned char *frame = (unsigned char *)malloc((size_t)W * H * 4);
        float *fp = (float *)rhi_shader_params(sh, 0);
        EXPECT(frame && fp, "follow-up setup");
        if (frame && fp) {
            memset(fp, 0, 80); fp[0] = fp[5] = fp[10] = fp[15] = 1.0f;
            EXPECT(rhi_cmd_set_viewport(0, 0, W, H) == RHI_OK, "follow-up viewport");
            #define COLOUR(r, g, b) do { fp[16] = (r); fp[17] = (g); fp[18] = (b); fp[19] = 1.0f; } while (0)
            #define PXAT(x, y) (frame + ((size_t)(y) * W + (x)) * 4)

            /* F3a: a STATIC vertex buffer updated between two draws; each draw must show the contents it was issued with. */
            {
                RhiBuffer vb = rhi_buffer_create(&(RhiBufferDesc){ RHI_BUFFER_VERTEX, RHI_UPDATE_STATIC, sizeof left, left });
                EXPECT(vb.id, "F3a buffer");
                EXPECT(rhi_frame_begin(NULL) == RHI_OK, "F3a frame_begin");
                EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "F3a clear");
                rhi_cmd_bind_pipeline(pipe); rhi_cmd_bind_vertex_buffer(0, vb, 0);
                COLOUR(1, 0, 0); rhi_cmd_draw(6, 0);                                       /* left quad, red */
                EXPECT(rhi_buffer_update(vb, 0, sizeof right, right) == RHI_OK, "F3a STATIC update between draws");
                COLOUR(0, 1, 0); rhi_cmd_draw(6, 0);                                       /* right quad, green */
                EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "F3a read");
                printf("F3a STATIC vertex update: left=%u,%u,%u right=%u,%u,%u\n", PXAT(64, 96)[0], PXAT(64, 96)[1], PXAT(64, 96)[2],
                       PXAT(192, 96)[0], PXAT(192, 96)[1], PXAT(192, 96)[2]);
                EXPECT(is_red(PXAT(64, 96)), "F3a: the first draw must keep the geometry it was issued with (left red)");
                EXPECT(is_green(PXAT(192, 96)), "F3a: the second draw sees the update (right green)");
                EXPECT(rhi_frame_end() == RHI_OK, "F3a frame_end");
                rhi_buffer_destroy(vb);
            }
            /* F3b: the same for a STATIC index buffer. */
            {
                RhiBuffer qb = rhi_buffer_create(&(RhiBufferDesc){ RHI_BUFFER_VERTEX, RHI_UPDATE_STATIC, sizeof quads, quads });
                RhiBuffer ib = rhi_buffer_create(&(RhiBufferDesc){ RHI_BUFFER_INDEX, RHI_UPDATE_STATIC, sizeof idx_left, idx_left });
                EXPECT(qb.id && ib.id, "F3b buffers");
                EXPECT(rhi_frame_begin(NULL) == RHI_OK, "F3b frame_begin");
                EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "F3b clear");
                rhi_cmd_bind_pipeline(pipe); rhi_cmd_bind_vertex_buffer(0, qb, 0); rhi_cmd_bind_index_buffer(ib);
                COLOUR(1, 0, 0); rhi_cmd_draw_indexed(6, 0);
                EXPECT(rhi_buffer_update(ib, 0, sizeof idx_right, idx_right) == RHI_OK, "F3b STATIC index update between draws");
                COLOUR(0, 1, 0); rhi_cmd_draw_indexed(6, 0);
                EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "F3b read");
                printf("F3b STATIC index update:  left=%u,%u,%u right=%u,%u,%u\n", PXAT(64, 96)[0], PXAT(64, 96)[1], PXAT(64, 96)[2],
                       PXAT(192, 96)[0], PXAT(192, 96)[1], PXAT(192, 96)[2]);
                EXPECT(is_red(PXAT(64, 96)) && is_green(PXAT(192, 96)), "F3b: index update between draws");
                EXPECT(rhi_frame_end() == RHI_OK, "F3b frame_end");
                rhi_buffer_destroy(qb); rhi_buffer_destroy(ib);
            }
            /* F4: the deferred-free list cannot grow while a recorded draw still uses the buffer being replaced. */
            {
                RhiBuffer vb = rhi_buffer_create(&(RhiBufferDesc){ RHI_BUFFER_VERTEX, RHI_UPDATE_STATIC, sizeof left, left });
                EXPECT(vb.id, "F4 buffer");
                unsigned v4 = rhi_vk_validation_messages;
                EXPECT(rhi_frame_begin(NULL) == RHI_OK, "F4 frame_begin");
                EXPECT(rhi_cmd_clear_main(black, 1.0f) == RHI_OK, "F4 clear");
                rhi_cmd_bind_pipeline(pipe); rhi_cmd_bind_vertex_buffer(0, vb, 0);
                COLOUR(1, 0, 0); rhi_cmd_draw(6, 0);                                       /* records a draw that reads the old storage */
                rhi_vk_test_fail_pending_alloc(1);                                         /* the pending-free list cannot grow */
                EXPECT(rhi_buffer_replace(vb, sizeof right, right) == RHI_OK, "F4 replace");
                COLOUR(0, 1, 0); rhi_cmd_draw(6, 0);                                       /* new storage */
                EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "F4 read");
                EXPECT(rhi_frame_end() == RHI_OK, "F4 frame_end");
                printf("F4 deferred-free failure: left=%u,%u,%u right=%u,%u,%u validation messages %u\n", PXAT(64, 96)[0], PXAT(64, 96)[1],
                       PXAT(64, 96)[2], PXAT(192, 96)[0], PXAT(192, 96)[1], PXAT(192, 96)[2], rhi_vk_validation_messages - v4);
                EXPECT(is_red(PXAT(64, 96)) && is_green(PXAT(192, 96)), "F4: both draws render with the storage they were issued with");
                EXPECT(rhi_vk_validation_messages == v4, "F4: replacing a buffer with a failing free list raised %u validation message(s)",
                       rhi_vk_validation_messages - v4);
                rhi_buffer_destroy(vb);
            }
            #undef COLOUR
            #undef PXAT
        }
        free(frame);
        rhi_shader_destroy(sh);
        EXPECT(rhi_vk_validation_messages == vbefore, "follow-up tests raised %u validation message(s)", rhi_vk_validation_messages - vbefore);
    }

    /* ---- gate: two clears in one recording (WAW on the colour/depth images) ----
     * Valid use; with a barrier that does not wait for the first clear, synchronization
     * validation (RHI_VK_SYNC_VALIDATION=1) reports a hazard. */
    {
        unsigned vbefore = rhi_vk_validation_messages;
        const float red[4] = { 1, 0, 0, 1 }, blue[4] = { 0, 0, 1, 1 };
        unsigned char *frame = (unsigned char *)malloc((size_t)W * H * 4);
        EXPECT(frame != NULL, "double clear alloc");
        if (frame) {
            EXPECT(rhi_frame_begin(NULL) == RHI_OK, "double clear frame_begin");
            EXPECT(rhi_cmd_clear_main(red, 1.0f) == RHI_OK, "first clear");
            EXPECT(rhi_cmd_clear_main(blue, 0.5f) == RHI_OK, "second clear, same recording");
            EXPECT(rhi_read_pixels(0, 0, W, H, RHI_READ_RGBA8, frame, 0) == RHI_OK, "double clear read");
            EXPECT(is_blue(frame + ((size_t)96 * W + 128) * 4), "the second clear wins");
            EXPECT(rhi_frame_end() == RHI_OK, "double clear frame_end");
            printf("GATE: back-to-back clears in one recording OK\n");
        }
        free(frame);
        EXPECT(rhi_vk_validation_messages == vbefore, "double clear raised %u validation message(s)",
               rhi_vk_validation_messages - vbefore);
    }

    /* ---- gate: is synchronization validation really active? ----
     * Only with RHI_VK_SYNC_VALIDATION=1: record a deliberate WAW hazard and require the
     * layer to report it, so a clean run under the flag means something. The reported
     * messages are expected and removed from the count. */
    {
        const char *sv = getenv("RHI_VK_SYNC_VALIDATION");
        if (sv && *sv && *sv != '0') {
            unsigned vbefore = rhi_vk_validation_messages;
            EXPECT(rhi_frame_begin(NULL) == RHI_OK, "hazard frame_begin");
            EXPECT(rhi_vk_test_sync_hazard() == 1, "record the deliberate hazard");
            EXPECT(rhi_frame_end() == RHI_OK, "hazard frame_end");
            unsigned got = rhi_vk_validation_messages - vbefore;
            printf("SYNC-VALIDATION: deliberate WAW hazard produced %u message(s)\n", got);
            EXPECT(got > 0, "synchronization validation did not report a deliberate hazard (layer inactive)");
            rhi_vk_validation_messages = vbefore;
        } else {
            printf("SYNC-VALIDATION: off (set RHI_VK_SYNC_VALIDATION=1 to enable)\n");
        }
    }

    rhi_device_destroy();
    SDL_DestroyWindow(win);
    SDL_Quit();

    printf("validation messages: %u\n", rhi_vk_validation_messages);
    if (rhi_vk_validation_messages) { fprintf(stderr, "FAIL: %u validation message(s)\n", rhi_vk_validation_messages); fails++; }
    printf("%s\n", fails ? "SMOKE FAILED" : "SMOKE OK");
    return fails ? 1 : 0;
}
