#version 450
/* Vulkan port of src/shaders/frontend.vert. Same transform; declarations use
 * explicit attribute locations (RhiShaderDesc order: aPos = 0) and a std140 UBO
 * whose layout matches FeParams (frontend_draw.c) exactly. */
layout(location = 0) in vec2 aPos;
layout(set = 0, binding = 0, std140) uniform FeParams {
    mat4 uMVP;    /* @0  */
    vec4 uColor;  /* @64 */
} p;
void main() { gl_Position = p.uMVP * vec4(aPos, 0.0, 1.0); }
