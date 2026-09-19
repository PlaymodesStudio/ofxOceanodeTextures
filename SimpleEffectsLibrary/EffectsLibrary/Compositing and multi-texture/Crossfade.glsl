//LayerB:texture, Mix:0.5:0:1
// @description Crossfades RGBA between the source and a second texture.
// @param LayerB: Second image used at the end of the crossfade.
// @param Mix: Blend amount from source zero to LayerB one.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform sampler2D LayerB; uniform int LayerBConnected; uniform vec2 uResolution;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected;
out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 a=texture(tSource,uv); if(LayerBConnected==0){ out_color=a; return; } out_color=mix(a,texture(LayerB,uv),seUnit(MixTex,MixTexConnected,Mix,uv)); }
