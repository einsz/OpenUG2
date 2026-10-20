#version 450
/* Vulkan port of src/shaders/ug2.vert. Same transform; attribute locations come
 * from render_program()'s attribute order (aPos=0, aUV=1, aNor=2, aColor=3), the
 * varyings get fixed locations shared with ug2.frag, and the uniforms live in the
 * std140 UG2Params UBO (declared identically to the fragment stage) referenced as
 * u.<name>. clip.w == view-space depth under perspective (P*V or P*V*M); NDC HUD
 * quads have w==1 so fog leaves them alone. */
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNor;
layout(location = 3) in vec4 aColor;

layout(location = 0) out vec2  vUV;
layout(location = 1) out vec3  vN;
layout(location = 2) out float vDepth;
layout(location = 3) out vec3  vPos;
layout(location = 4) out vec4  vColor;

layout(set = 0, binding = 0, std140) uniform UG2Params {
    mat4 uMVP;
    mat4 uModel;
    mat4 uHeadShadowMVP[2];
    vec4 uWetLightPos[8];
    vec4 uWetLightColor[8];
    vec4 uHeadPos[2];
    vec4 uReflectionParams;
    vec4 uHeadShape;
    vec3 uColor;       float uVColor;
    vec3 uLight;       float uUseTex;
    vec3 uFogColor;    float uFogDensity;
    vec3 uCamPos;      float uUnlit;
    vec3 uEnvOrigin;   float uEnvReady;
    vec3 uEmissive;    float uAlpha;
    vec3 uHeadForward; float uSoft;
    vec3 uHeadRight;   float uSpec;
    vec3 uHeadUp;      float uDecal;
    float uAmbient, uDiffuse, uEnv, uUVCheck, uGloss, uFlipN;
    float uWetness, uWeatherTime, uRainIntensity, uPaintLights, uRoadReflection;
    float uRimTint, uVista, uAlphaTest, uTextureAlpha, uEmissiveTex, uFresnel, uClearcoat;
    float uHeadGain, uHeadLow, uHeadShadow;
    /* Sampler-unit ints: kept here to preserve std140 offsets, never read. */
    int uTex_unit, uEnvCube_unit, uReflectionColor_unit, uReflectionDepth_unit;
    int uHeadShadowMap0_unit, uHeadShadowMap1_unit;
} u;

void main() {
    vUV = aUV;
    vN = (u.uModel * vec4(aNor, 0.0)).xyz;
    vPos = (u.uModel * vec4(aPos, 1.0)).xyz;
    vColor = aColor;
    gl_Position = u.uMVP * vec4(aPos, 1.0);
    vDepth = gl_Position.w;
    /* The engine's projection (mat_persp) is GL convention: clip z in [-w, w].
       Vulkan clips to [0, w], so remap. This also makes the stored depth equal
       GL's window z = (ndc+1)/2, so roadReflection()'s depth reconstruction stays
       valid. vDepth (= w, for fog) is read above and is unchanged. Any other
       perspective shader ported to Vulkan (headlight_shadow) needs this line. */
    gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;
}
