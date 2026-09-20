//VectorMap:texture, AmountX:50:-1000:1000, AmountY:50:-1000:1000, Center:0.5:0:1, InvertY:0:0:1, Mix:1:0:1
// @description Displaces source pixels using red and green vectors from a second texture.
// @param VectorMap: Texture supplying horizontal red and vertical green displacement.
// @param AmountX: Horizontal displacement scale in pixels.
// @param AmountY: Vertical displacement scale in pixels.
// @param Center: Vector-map value interpreted as zero displacement.
// @param InvertY: Reverses vertical vector direction.
// @param Mix: Blends between source and vector-warped result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform sampler2D VectorMap; uniform int VectorMapConnected; uniform vec2 uResolution;
uniform float AmountX; uniform sampler2D AmountXTex; uniform int AmountXTexConnected;
uniform float AmountY; uniform sampler2D AmountYTex; uniform int AmountYTexConnected;
uniform float Center; uniform sampler2D CenterTex; uniform int CenterTexConnected;
uniform float InvertY; uniform sampler2D InvertYTex; uniform int InvertYTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); if(VectorMapConnected==0){out_color=s;return;} vec2 v=texture(VectorMap,uv).rg-vec2(seUnit(CenterTex,CenterTexConnected,Center,uv)); v.y*=mix(1.0,-1.0,seUnit(InvertYTex,InvertYTexConnected,InvertY,uv)); vec2 q=uv+v*vec2(seRange(AmountXTex,AmountXTexConnected,AmountX,-1000.0,1000.0,uv),seRange(AmountYTex,AmountYTexConnected,AmountY,-1000.0,1000.0,uv))/uResolution; vec4 result=seInside(q)?texture(tSource,q):vec4(0.0); out_color=mix(s,result,seUnit(MixTex,MixTexConnected,Mix,uv)); }
