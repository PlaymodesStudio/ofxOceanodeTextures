//Amount:0:-1:1, Radius:0.5:0.001:2, CenterX:0.5:0:1, CenterY:0.5:0:1, Mix:1:0:1
// @description Produces a radial bulge for positive values and pinch for negative values.
// @param Amount: Strength and direction of radial distortion.
// @param Radius: Normalized influence radius.
// @param CenterX: Horizontal distortion center.
// @param CenterY: Vertical distortion center.
// @param Mix: Blends between source and distorted result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution;
uniform float Amount; uniform sampler2D AmountTex; uniform int AmountTexConnected;
uniform float Radius; uniform sampler2D RadiusTex; uniform int RadiusTexConnected;
uniform float CenterX; uniform sampler2D CenterXTex; uniform int CenterXTexConnected;
uniform float CenterY; uniform sampler2D CenterYTex; uniform int CenterYTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec2 center=vec2(seUnit(CenterXTex,CenterXTexConnected,CenterX,uv),seUnit(CenterYTex,CenterYTexConnected,CenterY,uv)); vec2 p=uv-center; p.x*=uResolution.x/uResolution.y; float radius=seRange(RadiusTex,RadiusTexConnected,Radius,0.001,2.0,uv), d=length(p); if(d<radius){ float x=1.0-d/radius; float factor=max(0.05,1.0-seRange(AmountTex,AmountTexConnected,Amount,-1.0,1.0,uv)*x*x); p*=factor; } p.x/=uResolution.x/uResolution.y; vec2 q=p+center; vec4 result=seInside(q)?texture(tSource,q):vec4(0.0); out_color=mix(texture(tSource,uv),result,seUnit(MixTex,MixTexConnected,Mix,uv)); }
