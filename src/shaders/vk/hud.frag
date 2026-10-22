#version 450
/* Vulkan port of the fragment shader in hud.c init_shader. Samplers: art on
 * unit 0 (binding 1), silhouette on unit 1 (binding 2). dialClip: centre (NDC),
 * limit (radians), mode 0 off / 1 keep <= / 2 keep >=, with dial angle 0 straight
 * down increasing screen-clockwise (C1-4). */
layout(location = 0) in vec2 uv;
layout(location = 1) in vec2 muv;
layout(location = 2) in vec2 ndc;

layout(set = 0, binding = 0, std140) uniform HudParams {
    vec4  tint;
    vec4  dialClip;
    vec2  halfSize;
    float roads;
    int   art;
    int   silhouette;
} p;
layout(set = 0, binding = 1) uniform sampler2D art;
layout(set = 0, binding = 2) uniform sampler2D silhouette;

layout(location = 0) out vec4 oColor;

void main() {
    if (p.dialClip.w > 0.5) {
        vec2 d = (ndc - p.dialClip.xy) * p.halfSize;
        float a = atan(-d.x, -d.y);
        if (a < 0.0) a += 6.28318531;
        if (p.dialClip.w < 1.5) { if (a > p.dialClip.z) discard; }
        else if (a < p.dialClip.z) discard;
    }
    if (p.roads > 1.5 && (muv.x < 0.0 || muv.x > 1.0 || muv.y < 0.0 || muv.y > 1.0)) discard;
    vec4 c = p.roads > 1.5 ? vec4(1.0) : texture(art, uv);
    if (p.roads > 0.5) c.a = texture(silhouette, muv).a;
    oColor = c * p.tint;
}
