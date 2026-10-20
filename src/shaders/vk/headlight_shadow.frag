#version 450
/* Vulkan port of src/shaders/headlight_shadow.frag: writes the view distance
 * packed into two 8-bit channels. */
layout(set = 0, binding = 0, std140) uniform HeadlightShadowParams {
    mat4  uMVP;
    float uCutout;
    float uRange;
} u;
layout(set = 0, binding = 1) uniform sampler2D uTex;

layout(location = 0) in vec2  uv;
layout(location = 1) in float distance_;
layout(location = 0) out vec4 oColor;

void main() {
    if (u.uCutout > 0.5 && texture(uTex, uv).a < 0.5) discard;
    vec2 enc = fract(clamp(distance_ / u.uRange, 0.0, 0.99999) * vec2(1.0, 255.0));
    enc.x -= enc.y / 255.0;
    oColor = vec4(enc, 0.0, 1.0);
}
