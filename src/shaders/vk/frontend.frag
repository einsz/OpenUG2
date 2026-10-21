#version 450
/* Vulkan port of src/shaders/frontend.frag. gl_FragColor -> an explicit output;
 * the FeParams UBO is declared identically to the vertex stage. */
layout(set = 0, binding = 0, std140) uniform FeParams {
    mat4 uMVP;
    vec4 uColor;
} p;
layout(location = 0) out vec4 oColor;
void main() { oColor = p.uColor; }
