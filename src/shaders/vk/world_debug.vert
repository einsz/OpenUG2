#version 450
/* Vulkan port of src/world_debug_120.vert. Attribute locations follow the
 * RhiShaderDesc order (a_Position, a_Normal, a_UV0, a_Color); the std140 UBO
 * matches WorldDebugParams (world_mesh.h). */
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_UV0;
layout(location = 3) in vec4 a_Color;   /* RGBA8 prelight, normalized to [0,1] */

layout(set = 0, binding = 0, std140) uniform WorldDebugParams {
    mat4 u_MVP;        /* @0  */
    int  u_DebugMode;  /* @64 */
} p;

layout(location = 0) out vec3 v_Normal;
layout(location = 1) out vec2 v_UV0;
layout(location = 2) out vec4 v_Color;

void main() {
    v_Normal = a_Normal;
    v_UV0    = a_UV0;
    v_Color  = a_Color;
    gl_Position = p.u_MVP * vec4(a_Position, 1.0);
    /* GL-convention clip z in [-w, w] -> Vulkan [0, w] (see ug2.vert). */
    gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;
}
