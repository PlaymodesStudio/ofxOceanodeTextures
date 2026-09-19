//ShadowColor:color:0.05:0.1:0.25:1, HighlightColor:color:1:0.65:0.2:1, Steps:6:2:64, Contrast:1:0.1:8, Mix:1:0:1
// @description Posterizes luminance and maps it between two selected colors.
// @param ShadowColor: Color assigned to darkest steps.
// @param HighlightColor: Color assigned to brightest steps.
// @param Steps: Number of luminance bands.
// @param Contrast: Tonal contrast applied before posterization.
// @param Mix: Blends between source and duotone result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform vec4 ShadowColor; uniform vec4 HighlightColor; uniform float Steps;
uniform float Contrast; uniform sampler2D ContrastTex; uniform int ContrastTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); float steps=max(round(Steps),2.0); float l=clamp((seLuma(s.rgb)-0.5)*seRange(ContrastTex,ContrastTexConnected,Contrast,0.1,8.0,uv)+0.5,0.0,1.0); l=round(l*(steps-1.0))/(steps-1.0); vec3 result=mix(ShadowColor.rgb,HighlightColor.rgb,l); out_color=vec4(mix(s.rgb,result,seUnit(MixTex,MixTexConnected,Mix,uv)),s.a); }
