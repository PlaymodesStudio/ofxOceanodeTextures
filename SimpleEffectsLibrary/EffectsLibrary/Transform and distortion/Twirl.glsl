//Amount:180:-720:720, Radius:0.5:0.001:2, Falloff:2:0.1:8, CenterX:0.5:0:1, CenterY:0.5:0:1, Mix:1:0:1
// @description Rotates pixels progressively around a center to create a twirl distortion.
// @param Amount: Rotation at the center in degrees.
// @param Radius: Normalized radius influenced by the twirl.
// @param Falloff: Shapes how quickly rotation fades toward the radius.
// @param CenterX: Horizontal center of the twirl.
// @param CenterY: Vertical center of the twirl.
// @param Mix: Blends between source and distorted result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution;
uniform float Amount; uniform sampler2D AmountTex; uniform int AmountTexConnected;
uniform float Radius; uniform sampler2D RadiusTex; uniform int RadiusTexConnected;
uniform float Falloff; uniform sampler2D FalloffTex; uniform int FalloffTexConnected;
uniform float CenterX; uniform sampler2D CenterXTex; uniform int CenterXTexConnected;
uniform float CenterY; uniform sampler2D CenterYTex; uniform int CenterYTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec2 center=vec2(seUnit(CenterXTex,CenterXTexConnected,CenterX,uv),seUnit(CenterYTex,CenterYTexConnected,CenterY,uv)); vec2 p=uv-center; p.x*=uResolution.x/uResolution.y; float radius=seRange(RadiusTex,RadiusTexConnected,Radius,0.001,2.0,uv), d=length(p); float influence=pow(clamp(1.0-d/radius,0.0,1.0),seRange(FalloffTex,FalloffTexConnected,Falloff,0.1,8.0,uv)); float angle=radians(seRange(AmountTex,AmountTexConnected,Amount,-720.0,720.0,uv))*influence; mat2 rot=mat2(cos(angle),-sin(angle),sin(angle),cos(angle)); p=rot*p; p.x/=uResolution.x/uResolution.y; vec2 q=p+center; vec4 result=seInside(q)?texture(tSource,q):vec4(0.0); out_color=mix(texture(tSource,uv),result,seUnit(MixTex,MixTexConnected,Mix,uv)); }
