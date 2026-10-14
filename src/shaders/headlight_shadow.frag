uniform sampler2D uTex;uniform float uCutout,uRange;
varying vec2 uv;varying float distance;
void main(){
  if(uCutout>0.5 && texture2D(uTex,uv).a<0.5)discard;
  vec2 enc=fract(clamp(distance/uRange,0.0,0.99999)*vec2(1.0,255.0));
  enc.x-=enc.y/255.0;gl_FragColor=vec4(enc,0.0,1.0);}
