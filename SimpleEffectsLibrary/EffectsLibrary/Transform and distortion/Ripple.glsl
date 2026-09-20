//Amplitude:12:-200:200, Frequency:12:0:100, Phase:0:-10:10, Speed:0:-10:10, Radius:1:0.001:2, Decay:2:0:10, CenterX:0.5:0:1, CenterY:0.5:0:1, Mix:1:0:1
// @description Sends concentric animated waves outward from a configurable center.
// @param Amplitude: Radial displacement amplitude in pixels.
// @param Frequency: Number of oscillations across normalized radius.
// @param Phase: Manual ripple phase measured in cycles.
// @param Speed: Automatic phase speed in cycles per second.
// @param Radius: Normalized region reached by the ripple.
// @param Decay: Exponential attenuation from center to radius.
// @param CenterX: Horizontal ripple center.
// @param CenterY: Vertical ripple center.
// @param Mix: Blends between source and rippled result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform float uTime;
uniform float Amplitude; uniform sampler2D AmplitudeTex; uniform int AmplitudeTexConnected;
uniform float Frequency; uniform sampler2D FrequencyTex; uniform int FrequencyTexConnected;
uniform float Phase; uniform sampler2D PhaseTex; uniform int PhaseTexConnected;
uniform float Speed; uniform sampler2D SpeedTex; uniform int SpeedTexConnected;
uniform float Radius; uniform sampler2D RadiusTex; uniform int RadiusTexConnected;
uniform float Decay; uniform sampler2D DecayTex; uniform int DecayTexConnected;
uniform float CenterX; uniform sampler2D CenterXTex; uniform int CenterXTexConnected;
uniform float CenterY; uniform sampler2D CenterYTex; uniform int CenterYTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec2 center=vec2(seUnit(CenterXTex,CenterXTexConnected,CenterX,uv),seUnit(CenterYTex,CenterYTexConnected,CenterY,uv)); vec2 p=uv-center; p.x*=uResolution.x/uResolution.y; float d=length(p), radius=seRange(RadiusTex,RadiusTexConnected,Radius,0.001,2.0,uv); float phase=seRange(PhaseTex,PhaseTexConnected,Phase,-10.0,10.0,uv)+uTime*seRange(SpeedTex,SpeedTexConnected,Speed,-10.0,10.0,uv); float envelope=1.0-smoothstep(radius*0.8,radius,d); float wave=sin((d*seRange(FrequencyTex,FrequencyTexConnected,Frequency,0.0,100.0,uv)-phase)*2.0*SE_PI)*exp(-d*seRange(DecayTex,DecayTexConnected,Decay,0.0,10.0,uv))*envelope; vec2 dir=d>SE_EPSILON?p/d:vec2(0.0); p+=dir*wave*seRange(AmplitudeTex,AmplitudeTexConnected,Amplitude,-200.0,200.0,uv)/uResolution.y; p.x/=uResolution.x/uResolution.y; vec2 q=p+center; vec4 result=seInside(q)?texture(tSource,q):vec4(0.0); out_color=mix(texture(tSource,uv),result,seUnit(MixTex,MixTexConnected,Mix,uv)); }
