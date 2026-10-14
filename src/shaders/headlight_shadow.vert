/* Headlight shadow map: writes view distance, packed into two 8-bit channels. */
attribute vec3 aPos;attribute vec2 aUV;uniform mat4 uMVP;
varying vec2 uv;varying float distance;
void main(){uv=aUV;gl_Position=uMVP*vec4(aPos,1.0);distance=gl_Position.w;}
