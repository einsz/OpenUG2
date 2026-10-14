/* Compile the backend with a controlled allocator failure; run real GL sampling. */
#include "render.h"
#include "rhi_gl.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static size_t fail_bytes;
static void *test_realloc(void *ptr, size_t bytes) {
    if (bytes == fail_bytes) return NULL;
    return realloc(ptr, bytes);
}
#define realloc test_realloc
#include "../src/rhi_gl.c"
#undef realloc

int main(void) {
    assert(SDL_Init(SDL_INIT_VIDEO) == 0);
    assert(rhi_prepare_window(RHI_BACKEND_DEFAULT) == RHI_OK);
    SDL_Window *window = SDL_CreateWindow("rhi-sampler-test", 0, 0, 32, 32,
        rhi_window_flags(RHI_BACKEND_DEFAULT) | SDL_WINDOW_HIDDEN);
    assert(window);
    assert(rhi_device_create(&(RhiDeviceDesc){RHI_BACKEND_DEFAULT, 0, window}) == RHI_OK);
    unsigned char rgb[4 * 4 * 3]; memset(rgb, 127, sizeof rgb);
    N2Tex image = {.w = 4, .h = 4, .rgb = rgb};
    fail_bytes = 16 * sizeof(RhiSamplerDesc);
    assert(!upload_tex(&image).id); /* Failed default sampler must fail the upload. */
    fail_bytes = 0;
    RhiTexture texture = upload_tex(&image); assert(texture.id);
    RhiSamplerDesc d = *rhi_sampler_desc(rhi_texture_sampler(texture));
    assert(d.min_filter == RHI_FILTER_LINEAR && d.mip_filter == RHI_MIP_LINEAR);
    RhiSampler sampler = rhi_sampler_create(&d); assert(sampler.id);
    assert(sampler.id == rhi_sampler_create(&d).id);
    assert(!rhi_sampler_create(NULL).id);
    d.max_anisotropy = NAN; assert(!rhi_sampler_create(&d).id);
    d.max_anisotropy = 1; d.mip_filter = (RhiMipFilter)3;
    assert(!rhi_sampler_create(&d).id);
    assert(!rhi_sampler_desc((RhiSampler){UINT32_MAX}));
    assert(rhi_texture_set_sampler(texture, (RhiSampler){0}) == RHI_ERR_INVALID);
    static const GLint filters[2][3] = {
        {GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR},
        {GL_LINEAR, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR}
    };
    for (int min = 0; min < 2; min++) for (int mip = 0; mip < 3; mip++) {
        d.min_filter = (RhiFilter)min; d.mip_filter = (RhiMipFilter)mip;
        d.mag_filter = RHI_FILTER_LINEAR; d.wrap_u = d.wrap_v = RHI_WRAP_CLAMP;
        assert(rhi_texture_set_sampler(texture, rhi_sampler_create(&d)) == RHI_OK);
        GLint actual; glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &actual);
        assert(actual == filters[min][mip]);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &actual);
        assert(actual == GL_CLAMP_TO_EDGE);
    }
    render_texture_clamp_linear(texture);
    d = *rhi_sampler_desc(rhi_texture_sampler(texture));
    assert(d.min_filter == RHI_FILTER_LINEAR && d.mip_filter == RHI_MIP_NONE &&
           d.wrap_u == RHI_WRAP_CLAMP && d.wrap_v == RHI_WRAP_CLAMP);
    RhiTexture guard = rhi_texture_create(&(RhiTextureDesc){RHI_TEXTURE_2D, RHI_FORMAT_RGBA8, 1, 1, RHI_TEXTURE_SAMPLED});
    assert(guard.id);
    g_tex_aniso_max = (int)rhi_caps()->max_anisotropy;
    if (g_tex_aniso_max > 1) {
        render_texture_detail(2);
        GLint bound; glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound);
        assert((GLuint)bound == rhi_gl_texture_name(guard));
        d = *rhi_sampler_desc(rhi_texture_sampler(texture));
        assert(d.max_anisotropy == 2 && d.mip_filter == RHI_MIP_NONE && d.wrap_u == RHI_WRAP_CLAMP);
    }
    rhi_texture_destroy(texture); rhi_texture_destroy(guard);
    render_texture_detail(1); /* Tracking skips dead handles. */
    assert(glGetError() == GL_NO_ERROR);
    rhi_device_destroy(); SDL_DestroyWindow(window); SDL_Quit();
    puts("rhi_sampler_test: PASS (allocation rollback, descriptors, filters, clamp, live detail, binding, dead handles)");
}
