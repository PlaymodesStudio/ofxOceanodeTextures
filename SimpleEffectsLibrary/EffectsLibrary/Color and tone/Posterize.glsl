//Levels:8:2:256, Mix:1:0:1
// @description Reduces each RGB channel to a chosen number of discrete levels.
// @param Levels: Number of available tonal values per channel.
// @param Mix: Blends between the source and posterized result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform float Levels; uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); float levels=max(round(Levels),2.0); vec3 p=round(s.rgb*(levels-1.0))/(levels-1.0); float a=seUnit(MixTex,MixTexConnected,Mix,uv); out_color=vec4(mix(s.rgb,p,a),s.a); }
