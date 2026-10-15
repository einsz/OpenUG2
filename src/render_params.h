/* render_params.h - the UG2 shader's parameter block (game side).
 *
 * One C struct holds every uniform of the UG2 shader (src/shaders/ug2.*);
 * ug2_params_layout names each field for rhi.h. Field names are the uniform
 * names. The order is chosen so the C layout is also std140 (mat4s, vec4
 * arrays, vec4s, each vec3 followed by a float, then the other scalars), which
 * a uniform-buffer backend needs; rhi_param_layout_check verifies it.
 *
 * The block is written through RProg.p (render.h) and read by the next draw.
 * The sampler units are ints here until texture binding is part of rhi.h. */
#ifndef OPENUG2_RENDER_PARAMS_H
#define OPENUG2_RENDER_PARAMS_H

#include <stdint.h>
#include <string.h>
#include "rhi.h"

/* Field notes:
 *   uLight       sun direction in world space
 *   uVColor      0..1 strength of per-vertex prelight (world geometry only)
 *   uDecal       1 = texture is an alpha-masked decal over uColor paint
 *   uVista       >0.5: alpha-blended backdrop pass; >1.5: tire-mark pass
 *                (rgb = clamp(2*texel*vertex), a = texel*vertex alpha, TM-15)
 *   uAlphaTest   >0.5: discard texels below 0.5 alpha (N2_DRAW_CUTOUT world
 *                batches, M135)
 *   uTextureAlpha >0.5: output alpha = texture alpha * uAlpha (N2_DRAW_BLEND/ADD
 *                world batches only, M135-R)
 *   uEmissiveTex >0.5: texture-backed unlit RGB/alpha (authored sky and
 *                point-light sprites)
 *   uFogColor, uFogDensity  exp^2 distance fog (matches the sky)
 *   uCamPos      camera in world space
 *   uEnv         environment-reflection amount (cars only)
 *   uUVCheck     1 = show the diagnostic UV-coordinate visualization instead of
 *                lighting/texture (toggle lives in the ImGui Session panel,
 *                make debug only)
 *   uFlipN       1 = negate the vertex normal (inspector diagnostic)
 *   uGloss       specular pow() exponent: high = tight metallic-paint highlight,
 *                low = broad plastic/trim sheen (cars only)
 *   uFresnel     >0.5: alpha rides the fresnel term (car glass pass only)
 *   uClearcoat   >0: second tight specular lobe -- the lacquer over the coloured
 *                base coat (car body/trim only)
 *   uRimTint     0 = raw rim texture, 1 = recolor toward uColor (rim paint)
 *   uEmissive    vec3 added AFTER lighting: a lamp lens that is lit AND glowing.
 *                Going fully uUnlit instead is what made every taillight a flat
 *                red sticker -- it discards the specular, the fresnel rim and
 *                the environment reflection, i.e. everything that reads as
 *                moulded plastic. Dome-weighted by dot(N,V) in the shader, so a
 *                curved lens is hottest where it faces the camera. Zero
 *                everywhere else; the car pass clears it. */
typedef struct UG2Params {
    float uMVP[16], uModel[16];
    float uHeadShadowMVP[2][16];
    float uWetLightPos[8][4], uWetLightColor[8][4];
    float uHeadPos[2][4];
    float uReflectionParams[4], uHeadShape[4];
    float uColor[3],       uVColor;
    float uLight[3],       uUseTex;
    float uFogColor[3],    uFogDensity;
    float uCamPos[3],      uUnlit;
    float uEnvOrigin[3],   uEnvReady;
    float uEmissive[3],    uAlpha;
    float uHeadForward[3], uSoft;
    float uHeadRight[3],   uSpec;
    float uHeadUp[3],      uDecal;
    float uAmbient, uDiffuse, uEnv, uUVCheck, uGloss, uFlipN;
    float uWetness, uWeatherTime, uRainIntensity, uPaintLights, uRoadReflection;
    float uRimTint, uVista, uAlphaTest, uTextureAlpha, uEmissiveTex, uFresnel, uClearcoat;
    float uHeadGain, uHeadLow, uHeadShadow;
    int32_t uTex, uEnvCube, uReflectionColor, uReflectionDepth, uHeadShadowMap[2];
} UG2Params;

extern const RhiParamLayout ug2_params_layout;

static inline void ug2_vec3(float *d, float x, float y, float z) { d[0] = x; d[1] = y; d[2] = z; }
static inline void ug2_vec4(float *d, float x, float y, float z, float w) {
    d[0] = x; d[1] = y; d[2] = z; d[3] = w;
}
static inline void ug2_copy(float *d, const float *s, int n) { memcpy(d, s, (size_t)n * sizeof *d); }

#endif /* OPENUG2_RENDER_PARAMS_H */
