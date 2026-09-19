//Amount:0:-1:1, Scale:1:0.1:4, Chromatic:0:0:30, CenterX:0.5:0:1, CenterY:0.5:0:1, Mix:1:0:1
// @description Applies barrel or pincushion lens distortion with optional radial color separation.
// @param Amount: Negative barrel or positive pincushion distortion strength.
// @param Scale: Compensating zoom applied before lens distortion.
// @param Chromatic: Red-blue radial separation measured in pixels.
// @param CenterX: Horizontal optical center.
// @param CenterY: Vertical optical center.
// @param Mix: Blends between source and lens-distorted result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution;
uniform float Amount; uniform sampler2D AmountTex; uniform int AmountTexConnected;
uniform float Scale; uniform sampler2D ScaleTex; uniform int ScaleTexConnected;
uniform float Chromatic; uniform sampler2D ChromaticTex; uniform int ChromaticTexConnected;
uniform float CenterX; uniform sampler2D CenterXTex; uniform int CenterXTexConnected;
uniform float CenterY; uniform sampler2D CenterYTex; uniform int CenterYTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec2 center=vec2(seUnit(CenterXTex,CenterXTexConnected,CenterX,uv),seUnit(CenterYTex,CenterYTexConnected,CenterY,uv)); vec2 p=(uv-center)/seRange(ScaleTex,ScaleTexConnected,Scale,0.1,4.0,uv); p.x*=uResolution.x/uResolution.y; float amount=seRange(AmountTex,AmountTexConnected,Amount,-1.0,1.0,uv); vec2 warped=p*(1.0+amount*dot(p,p)); warped.x/=uResolution.x/uResolution.y; vec2 q=center+warped; float chroma=seRange(ChromaticTex,ChromaticTexConnected,Chromatic,0.0,30.0,uv); vec2 separation=length(warped)>SE_EPSILON?normalize(warped)*chroma/uResolution:vec2(0.0); vec4 result=vec4(texture(tSource,q+separation).r,texture(tSource,q).g,texture(tSource,q-separation).b,texture(tSource,q).a); if(!seInside(q)) result=vec4(0.0); out_color=mix(texture(tSource,uv),result,seUnit(MixTex,MixTexConnected,Mix,uv)); }
