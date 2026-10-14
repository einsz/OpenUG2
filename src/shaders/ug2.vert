attribute vec3 aPos; attribute vec2 aUV; attribute vec3 aNor; attribute vec4 aColor;
uniform mat4 uMVP; uniform mat4 uModel; varying vec2 vUV; varying vec3 vN; varying float vDepth;
varying vec3 vPos; varying vec4 vColor;
/* clip.w == view-space depth under a perspective projection, for world
   (P*V) and car (P*V*M) alike -- no view matrix or extra uniforms needed.
   NDC-drawn HUD quads have w==1, so fog leaves them alone. */
void main(){ vUV=aUV; vN=(uModel*vec4(aNor,0.0)).xyz;
  vPos=(uModel*vec4(aPos,1.0)).xyz; vColor=aColor;
  gl_Position=uMVP*vec4(aPos,1.0); vDepth=gl_Position.w; }
