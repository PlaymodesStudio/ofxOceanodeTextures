//Thickness:1:0.25:16, Threshold:0.1:0:1, Strength:2:0:10, EdgeColor:color:1:1:1:1, SourceMix:0:0:1, AlphaEdges:0:0:1
// @description Draws a colored outline from local luminance and optional alpha discontinuities.
// @param Thickness: Neighbor distance used to find boundaries in source pixels.
// @param Threshold: Minimum edge magnitude retained.
// @param Strength: Multiplies edge intensity above the threshold.
// @param EdgeColor: Color applied to detected outlines.
// @param SourceMix: Blends the original image beneath the outlines.
// @param AlphaEdges: Adds alpha discontinuities to luminance edges.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform vec4 EdgeColor;
uniform float Thickness; uniform sampler2D ThicknessTex; uniform int ThicknessTexConnected;
uniform float Threshold; uniform sampler2D ThresholdTex; uniform int ThresholdTexConnected;
uniform float Strength; uniform sampler2D StrengthTex; uniform int StrengthTexConnected;
uniform float SourceMix; uniform sampler2D SourceMixTex; uniform int SourceMixTexConnected;
uniform float AlphaEdges; uniform sampler2D AlphaEdgesTex; uniform int AlphaEdgesTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); vec2 px=vec2(seRange(ThicknessTex,ThicknessTexConnected,Thickness,0.25,16.0,uv))/uResolution; vec4 l=texture(tSource,uv-vec2(px.x,0)),r=texture(tSource,uv+vec2(px.x,0)),b=texture(tSource,uv-vec2(0,px.y)),t=texture(tSource,uv+vec2(0,px.y)); float lum=sqrt(pow(seLuma(r.rgb)-seLuma(l.rgb),2.0)+pow(seLuma(t.rgb)-seLuma(b.rgb),2.0)); float alpha=sqrt(pow(r.a-l.a,2.0)+pow(t.a-b.a,2.0)); float edge=mix(lum,max(lum,alpha),seUnit(AlphaEdgesTex,AlphaEdgesTexConnected,AlphaEdges,uv)); edge=max(edge-seUnit(ThresholdTex,ThresholdTexConnected,Threshold,uv),0.0)*seRange(StrengthTex,StrengthTexConnected,Strength,0.0,10.0,uv); vec4 outlined=vec4(EdgeColor.rgb*edge,EdgeColor.a*edge); out_color=mix(outlined,s,seUnit(SourceMixTex,SourceMixTexConnected,SourceMix,uv)); }
