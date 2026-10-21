#version 450
/* Vulkan port of the vertex shader in hud.c init_shader. Attribute locations
 * follow the RhiShaderDesc order (position, sampleUV, clipUV); the std140 UBO
 * matches HudParams (hud.c) and is declared identically in the fragment stage. */
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 sampleUV;
layout(location = 2) in vec2 clipUV;

layout(set = 0, binding = 0, std140) uniform HudParams {
    vec4  tint;        /* @0  */
    vec4  dialClip;    /* @16 */
    vec2  halfSize;    /* @32 */
    float roads;       /* @40 */
    int   art;         /* @44 (sampler unit, unused: binding is fixed) */
    int   silhouette;  /* @48 */
} p;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec2 muv;
layout(location = 2) out vec2 ndc;

void main() {
    uv = sampleUV; muv = clipUV; ndc = position;
    gl_Position = vec4(position, 0.0, 1.0);
}
