//RedX:5:-100:100, RedY:0:-100:100, BlueX:-5:-100:100, BlueY:0:-100:100, GreenFollow:0:0:1, Mix:1:0:1
// @description Offsets red and blue channels independently for graphic RGB separation.
// @param RedX: Horizontal red-channel offset in pixels.
// @param RedY: Vertical red-channel offset in pixels.
// @param BlueX: Horizontal blue-channel offset in pixels.
// @param BlueY: Vertical blue-channel offset in pixels.
// @param GreenFollow: Moves green halfway between the red and blue offsets.
// @param Mix: Blends between source and split result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution;
uniform float RedX; uniform sampler2D RedXTex; uniform int RedXTexConnected; uniform float RedY; uniform sampler2D RedYTex; uniform int RedYTexConnected;
uniform float BlueX; uniform sampler2D BlueXTex; uniform int BlueXTexConnected; uniform float BlueY; uniform sampler2D BlueYTex; uniform int BlueYTexConnected;
uniform float GreenFollow; uniform sampler2D GreenFollowTex; uniform int GreenFollowTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec2 red=vec2(seRange(RedXTex,RedXTexConnected,RedX,-100.0,100.0,uv),seRange(RedYTex,RedYTexConnected,RedY,-100.0,100.0,uv))/uResolution; vec2 blue=vec2(seRange(BlueXTex,BlueXTexConnected,BlueX,-100.0,100.0,uv),seRange(BlueYTex,BlueYTexConnected,BlueY,-100.0,100.0,uv))/uResolution; vec2 green=mix(vec2(0.0),(red+blue)*0.5,seUnit(GreenFollowTex,GreenFollowTexConnected,GreenFollow,uv)); vec4 s=texture(tSource,uv); vec4 result=vec4(texture(tSource,uv+red).r,texture(tSource,uv+green).g,texture(tSource,uv+blue).b,s.a); out_color=mix(s,result,seUnit(MixTex,MixTexConnected,Mix,uv)); }
