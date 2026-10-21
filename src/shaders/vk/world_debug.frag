#version 450
/* Vulkan port of src/world_debug_120.frag. gl_FragColor -> an explicit output;
 * the WorldDebugParams UBO is declared identically to the vertex stage. */
layout(location = 0) in vec3 v_Normal;
layout(location = 1) in vec2 v_UV0;
layout(location = 2) in vec4 v_Color;

layout(set = 0, binding = 0, std140) uniform WorldDebugParams {
    mat4 u_MVP;
    int  u_DebugMode;   /* 0 prelight, 1 normals, 2 solid/wireframe overlay */
} p;

layout(location = 0) out vec4 oColor;

void main() {
    if (p.u_DebugMode == 1) {
        oColor = vec4(normalize(v_Normal) * 0.5 + 0.5, 1.0);   /* RGB surface normals */
    } else if (p.u_DebugMode == 2) {
        oColor = vec4(0.85, 0.85, 0.90, 1.0);   /* solid overlay; wireframe is the host's polygon mode */
    } else {
        oColor = vec4(v_Color.rgb, 1.0);        /* baked vertex prelight */
    }
}
