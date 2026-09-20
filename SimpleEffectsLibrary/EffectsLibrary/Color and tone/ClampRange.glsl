//Minimum:0:-10:10, Maximum:1:-10:10, Normalize:0:0:1
// @description Clamps arbitrary float RGB values and optionally normalizes the selected range.
// @param Minimum: Lowest retained RGB value.
// @param Maximum: Highest retained RGB value.
// @param Normalize: Crossfades from simple clamping to remapping the range into zero through one.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution;
uniform float Minimum; uniform sampler2D MinimumTex; uniform int MinimumTexConnected;
uniform float Maximum; uniform sampler2D MaximumTex; uniform int MaximumTexConnected;
uniform float Normalize; uniform sampler2D NormalizeTex; uniform int NormalizeTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); float lo=seRange(MinimumTex,MinimumTexConnected,Minimum,-10.0,10.0,uv); float hi=seRange(MaximumTex,MaximumTexConnected,Maximum,-10.0,10.0,uv); float low=min(lo,hi), high=max(lo,hi); vec3 clamped=clamp(s.rgb,vec3(low),vec3(high)); vec3 normalized=(clamped-low)/max(high-low,SE_EPSILON); float n=seUnit(NormalizeTex,NormalizeTexConnected,Normalize,uv); out_color=vec4(mix(clamped,normalized,n),s.a); }
