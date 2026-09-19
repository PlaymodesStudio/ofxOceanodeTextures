//Amount:0.15:-1:1, Samples:16:1:32, CenterX:0.5:0:1, CenterY:0.5:0:1, Mix:1:0:1
// @description Smears pixels toward or away from a configurable center to suggest rapid zoom.
// @param Amount: Length and direction of the zoom trail.
// @param Samples: Number of trail samples from one through thirty-two.
// @param CenterX: Horizontal zoom center.
// @param CenterY: Vertical zoom center.
// @param Mix: Blends between source and zoom blur.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform float Samples;
uniform float Amount; uniform sampler2D AmountTex; uniform int AmountTexConnected;
uniform float CenterX; uniform sampler2D CenterXTex; uniform int CenterXTexConnected;
uniform float CenterY; uniform sampler2D CenterYTex; uniform int CenterYTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec2 center=vec2(seUnit(CenterXTex,CenterXTexConnected,CenterX,uv),seUnit(CenterYTex,CenterYTexConnected,CenterY,uv)); int count=clamp(int(round(Samples)),1,32); float amount=seRange(AmountTex,AmountTexConnected,Amount,-1.0,1.0,uv); vec4 sum=vec4(0.0); for(int i=0;i<32;i++){ if(i>=count) break; float t=count<=1?0.0:float(i)/float(count-1)-0.5; sum+=texture(tSource,uv+(center-uv)*amount*t); } vec4 s=texture(tSource,uv); out_color=mix(s,sum/float(count),seUnit(MixTex,MixTexConnected,Mix,uv)); }
