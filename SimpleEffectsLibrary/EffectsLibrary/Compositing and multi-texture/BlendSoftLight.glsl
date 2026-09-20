//LayerB:texture, Mix:1:0:1
// @description Applies a smooth Soft Light contrast blend using a second texture.
// @param LayerB: Texture controlling the soft contrast adjustment.
// @param Mix: Overall blend amount including LayerB alpha.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform sampler2D LayerB; uniform int LayerBConnected; uniform vec2 uResolution;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
vec3 softLight(vec3 a,vec3 b){ vec3 d=mix(((16.0*a-12.0)*a+4.0)*a,sqrt(max(a,vec3(0.0))),step(vec3(0.25),a)); return mix(a-(1.0-2.0*b)*a*(1.0-a),a+(2.0*b-1.0)*(d-a),step(vec3(0.5),b)); }
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 a=texture(tSource,uv); if(LayerBConnected==0){out_color=a;return;} vec4 b=texture(LayerB,uv); float mixAmount=seUnit(MixTex,MixTexConnected,Mix,uv); float rgbAmount=mixAmount*b.a; out_color=vec4(mix(a.rgb,softLight(a.rgb,b.rgb),rgbAmount),a.a+b.a*mixAmount*(1.0-a.a)); }
