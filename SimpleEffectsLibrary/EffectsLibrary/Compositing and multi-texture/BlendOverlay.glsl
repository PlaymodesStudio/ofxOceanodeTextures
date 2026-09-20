//LayerB:texture, Mix:1:0:1
// @description Applies the Overlay blend mode using a second texture.
// @param LayerB: Texture blended over the source.
// @param Mix: Overall blend amount including LayerB alpha.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform sampler2D LayerB; uniform int LayerBConnected; uniform vec2 uResolution;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
vec3 overlay(vec3 a,vec3 b){ return mix(2.0*a*b,1.0-2.0*(1.0-a)*(1.0-b),step(vec3(0.5),a)); }
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 a=texture(tSource,uv); if(LayerBConnected==0){out_color=a;return;} vec4 b=texture(LayerB,uv); float mixAmount=seUnit(MixTex,MixTexConnected,Mix,uv); float rgbAmount=mixAmount*b.a; out_color=vec4(mix(a.rgb,overlay(a.rgb,b.rgb),rgbAmount),a.a+b.a*mixAmount*(1.0-a.a)); }
