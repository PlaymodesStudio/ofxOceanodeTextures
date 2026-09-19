//AmplitudeX:20:-500:500, AmplitudeY:0:-500:500, FrequencyX:2:0:50, FrequencyY:2:0:50, Phase:0:-10:10, Speed:0:-10:10, Mix:1:0:1
// @description Displaces image coordinates with independent horizontal and vertical sine waves.
// @param AmplitudeX: Horizontal displacement amplitude in pixels.
// @param AmplitudeY: Vertical displacement amplitude in pixels.
// @param FrequencyX: Vertical cycles driving horizontal displacement.
// @param FrequencyY: Horizontal cycles driving vertical displacement.
// @param Phase: Manual wave phase measured in cycles.
// @param Speed: Automatic phase speed in cycles per second.
// @param Mix: Blends between source and wave-warped result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform float uTime;
uniform float AmplitudeX; uniform sampler2D AmplitudeXTex; uniform int AmplitudeXTexConnected;
uniform float AmplitudeY; uniform sampler2D AmplitudeYTex; uniform int AmplitudeYTexConnected;
uniform float FrequencyX; uniform sampler2D FrequencyXTex; uniform int FrequencyXTexConnected;
uniform float FrequencyY; uniform sampler2D FrequencyYTex; uniform int FrequencyYTexConnected;
uniform float Phase; uniform sampler2D PhaseTex; uniform int PhaseTexConnected;
uniform float Speed; uniform sampler2D SpeedTex; uniform int SpeedTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; float phase=seRange(PhaseTex,PhaseTexConnected,Phase,-10.0,10.0,uv)+uTime*seRange(SpeedTex,SpeedTexConnected,Speed,-10.0,10.0,uv); float fx=seRange(FrequencyXTex,FrequencyXTexConnected,FrequencyX,0.0,50.0,uv), fy=seRange(FrequencyYTex,FrequencyYTexConnected,FrequencyY,0.0,50.0,uv); vec2 offset=vec2(sin((uv.y*fx+phase)*2.0*SE_PI)*seRange(AmplitudeXTex,AmplitudeXTexConnected,AmplitudeX,-500.0,500.0,uv),sin((uv.x*fy+phase)*2.0*SE_PI)*seRange(AmplitudeYTex,AmplitudeYTexConnected,AmplitudeY,-500.0,500.0,uv))/uResolution; vec2 q=uv+offset; vec4 result=seInside(q)?texture(tSource,q):vec4(0.0); out_color=mix(texture(tSource,uv),result,seUnit(MixTex,MixTexConnected,Mix,uv)); }
