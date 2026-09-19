//Amount:0.5:-1:1, Radius:0.65:0:2, Softness:0.35:0.001:1, Roundness:1:0.1:4, CenterX:0.5:0:1, CenterY:0.5:0:1
// @description Darkens or brightens image edges using a soft adjustable elliptical mask.
// @param Amount: Positive darkens edges and negative brightens them.
// @param Radius: Distance from center where the vignette begins.
// @param Softness: Width of the edge transition.
// @param Roundness: Changes the mask from wide ellipse to circular form.
// @param CenterX: Horizontal vignette center.
// @param CenterY: Vertical vignette center.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution;
uniform float Amount; uniform sampler2D AmountTex; uniform int AmountTexConnected;
uniform float Radius; uniform sampler2D RadiusTex; uniform int RadiusTexConnected;
uniform float Softness; uniform sampler2D SoftnessTex; uniform int SoftnessTexConnected;
uniform float Roundness; uniform sampler2D RoundnessTex; uniform int RoundnessTexConnected;
uniform float CenterX; uniform sampler2D CenterXTex; uniform int CenterXTexConnected;
uniform float CenterY; uniform sampler2D CenterYTex; uniform int CenterYTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); vec2 center=vec2(seUnit(CenterXTex,CenterXTexConnected,CenterX,uv),seUnit(CenterYTex,CenterYTexConnected,CenterY,uv)); vec2 p=(uv-center)*2.0; p.x*=mix(1.0,uResolution.x/uResolution.y,clamp(seRange(RoundnessTex,RoundnessTexConnected,Roundness,0.1,4.0,uv)/4.0,0.0,1.0)); float radius=seRange(RadiusTex,RadiusTexConnected,Radius,0.0,2.0,uv), soft=seRange(SoftnessTex,SoftnessTexConnected,Softness,0.001,1.0,uv); float mask=smoothstep(radius,radius+soft,length(p)); float amount=seRange(AmountTex,AmountTexConnected,Amount,-1.0,1.0,uv); vec3 target=amount>=0.0?s.rgb*(1.0-mask*amount):s.rgb+mask*(-amount)*(1.0-s.rgb); out_color=vec4(target,s.a); }
