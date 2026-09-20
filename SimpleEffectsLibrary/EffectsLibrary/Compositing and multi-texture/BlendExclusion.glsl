//LayerB:texture, Mix:1:0:1
// @description Applies the lower-contrast Exclusion difference blend mode.
// @param LayerB: Texture combined with the source.
// @param Mix: Overall blend amount including LayerB alpha.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform sampler2D LayerB; uniform int LayerBConnected; uniform vec2 uResolution;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 a=texture(tSource,uv); if(LayerBConnected==0){out_color=a;return;} vec4 b=texture(LayerB,uv); float mixAmount=seUnit(MixTex,MixTexConnected,Mix,uv); float rgbAmount=mixAmount*b.a; vec3 blended=a.rgb+b.rgb-2.0*a.rgb*b.rgb; out_color=vec4(mix(a.rgb,blended,rgbAmount),a.a+b.a*mixAmount*(1.0-a.a)); }
