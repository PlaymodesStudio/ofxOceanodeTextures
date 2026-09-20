//Threshold:0.5:0:1, Softness:0:0:1, Mix:1:0:1
// @description Inverts values above a luminance threshold with optional tonal softness.
// @param Threshold: Luminance where inversion begins.
// @param Softness: Width of the transition into the inverted result.
// @param Mix: Blends between the source and solarized result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution;
uniform float Threshold; uniform sampler2D ThresholdTex; uniform int ThresholdTexConnected;
uniform float Softness; uniform sampler2D SoftnessTex; uniform int SoftnessTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); float t=seUnit(ThresholdTex,ThresholdTexConnected,Threshold,uv); float soft=seUnit(SoftnessTex,SoftnessTexConnected,Softness,uv); float mask=soft<=SE_EPSILON?step(t,seLuma(s.rgb)):smoothstep(t,t+soft,seLuma(s.rgb)); vec3 solar=mix(s.rgb,1.0-s.rgb,mask); float a=seUnit(MixTex,MixTexConnected,Mix,uv); out_color=vec4(mix(s.rgb,solar,a),s.a); }
