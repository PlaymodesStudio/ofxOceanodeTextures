//Radius:4:0:64, Samples:12:4:16, Premultiply:1:0:1
// @description Softens source alpha with a radial blur while leaving color detail untouched.
// @param Radius: Blur radius measured in source pixels.
// @param Samples: Number of radial samples used around each pixel.
// @param Premultiply: Multiplies RGB by the blurred alpha.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform float Samples;
uniform float Radius; uniform sampler2D RadiusTex; uniform int RadiusTexConnected;
uniform float Premultiply; uniform sampler2D PremultiplyTex; uniform int PremultiplyTexConnected;
out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); float radius=seRange(RadiusTex,RadiusTexConnected,Radius,0.0,64.0,uv); int count=clamp(int(round(Samples)),4,16); float matte=s.a; float total=1.0; for(int i=0;i<16;i++){ if(i>=count) break; float angle=2.0*SE_PI*(float(i)/float(count)); matte+=texture(tSource,uv+vec2(cos(angle),sin(angle))*radius/uResolution).a; total+=1.0; } matte/=total; float pre=seUnit(PremultiplyTex,PremultiplyTexConnected,Premultiply,uv); out_color=vec4(s.rgb*mix(1.0,matte,pre),matte); }
