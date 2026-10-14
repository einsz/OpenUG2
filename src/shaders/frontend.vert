/* GLES2-compatible, no version directive = desktop GL2 default */
attribute vec2 aPos;
uniform mat4 uMVP;
void main() { gl_Position = uMVP * vec4(aPos, 0.0, 1.0); }
