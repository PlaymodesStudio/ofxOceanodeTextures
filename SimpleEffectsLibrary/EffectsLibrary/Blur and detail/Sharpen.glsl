//Amount:1:0:5, Radius:1:0.25:8, Mix:1:0:1
// @description Increases local edge contrast using four neighboring samples.
// @param Amount: Strength of the local contrast enhancement.
// @param Radius: Neighbor distance in source pixels.
// @param Mix: Blends between source and sharpened result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution;
uniform float Amount; uniform sampler2D AmountTex; uniform int AmountTexConnected;
uniform float Radius; uniform sampler2D RadiusTex; uniform int RadiusTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); vec2 px=vec2(seRange(RadiusTex,RadiusTexConnected,Radius,0.25,8.0,uv))/uResolution; vec3 neighbors=(texture(tSource,uv+vec2(px.x,0)).rgb+texture(tSource,uv-vec2(px.x,0)).rgb+texture(tSource,uv+vec2(0,px.y)).rgb+texture(tSource,uv-vec2(0,px.y)).rgb)*0.25; vec3 sharp=s.rgb+(s.rgb-neighbors)*seRange(AmountTex,AmountTexConnected,Amount,0.0,5.0,uv); out_color=vec4(mix(s.rgb,sharp,seUnit(MixTex,MixTexConnected,Mix,uv)),s.a); }
