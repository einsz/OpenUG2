#version 450
/* Vulkan port of src/shaders/headlight_shadow.vert. Attribute locations follow
 * the RhiShaderDesc order (aPos=0, aUV=1); the uniforms live in the std140
 * HeadlightShadowParams block (render.c), declared identically in the fragment
 * stage. */
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;

layout(location = 0) out vec2  uv;
layout(location = 1) out float distance_;

layout(set = 0, binding = 0, std140) uniform HeadlightShadowParams {
    mat4  uMVP;
    float uCutout;
    float uRange;
} u;

void main() {
    uv = aUV;
    gl_Position = u.uMVP * vec4(aPos, 1.0);
    distance_ = gl_Position.w;
    /* GL clip z in [-w, w] -> Vulkan [0, w] (see ug2.vert). The packed distance
       above is w and is unaffected. */
    gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;
}
