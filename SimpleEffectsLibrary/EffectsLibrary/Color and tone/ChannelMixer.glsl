//RR:1:-2:2, RG:0:-2:2, RB:0:-2:2, GR:0:-2:2, GG:1:-2:2, GB:0:-2:2, BR:0:-2:2, BG:0:-2:2, BB:1:-2:2
// @description Remixes RGB channels through a configurable three-by-three matrix.
// @param RR: Red contribution to output red.
// @param RG: Green contribution to output red.
// @param RB: Blue contribution to output red.
// @param GR: Red contribution to output green.
// @param GG: Green contribution to output green.
// @param GB: Blue contribution to output green.
// @param BR: Red contribution to output blue.
// @param BG: Green contribution to output blue.
// @param BB: Blue contribution to output blue.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource;
uniform vec2 uResolution;
uniform float RR; uniform sampler2D RRTex; uniform int RRTexConnected;
uniform float RG; uniform sampler2D RGTex; uniform int RGTexConnected;
uniform float RB; uniform sampler2D RBTex; uniform int RBTexConnected;
uniform float GR; uniform sampler2D GRTex; uniform int GRTexConnected;
uniform float GG; uniform sampler2D GGTex; uniform int GGTexConnected;
uniform float GB; uniform sampler2D GBTex; uniform int GBTexConnected;
uniform float BR; uniform sampler2D BRTex; uniform int BRTexConnected;
uniform float BG; uniform sampler2D BGTex; uniform int BGTexConnected;
uniform float BB; uniform sampler2D BBTex; uniform int BBTexConnected;
out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); mat3 m=mat3(seRange(RRTex,RRTexConnected,RR,-2.0,2.0,uv),seRange(GRTex,GRTexConnected,GR,-2.0,2.0,uv),seRange(BRTex,BRTexConnected,BR,-2.0,2.0,uv),seRange(RGTex,RGTexConnected,RG,-2.0,2.0,uv),seRange(GGTex,GGTexConnected,GG,-2.0,2.0,uv),seRange(BGTex,BGTexConnected,BG,-2.0,2.0,uv),seRange(RBTex,RBTexConnected,RB,-2.0,2.0,uv),seRange(GBTex,GBTexConnected,GB,-2.0,2.0,uv),seRange(BBTex,BBTexConnected,BB,-2.0,2.0,uv)); out_color=vec4(m*s.rgb,s.a); }
