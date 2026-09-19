//Amount:5:-100:100, Angle:0:-180:180, Radial:0:0:1, CenterX:0.5:0:1, CenterY:0.5:0:1, Mix:1:0:1
// @description Separates red and blue samples linearly or radially for lens-like color fringing.
// @param Amount: Maximum channel separation in pixels.
// @param Angle: Direction of linear separation in degrees.
// @param Radial: Crossfades from linear to center-out radial separation.
// @param CenterX: Horizontal center used by radial separation.
// @param CenterY: Vertical center used by radial separation.
// @param Mix: Blends between source and separated result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution;
uniform float Amount; uniform sampler2D AmountTex; uniform int AmountTexConnected;
uniform float Angle; uniform sampler2D AngleTex; uniform int AngleTexConnected;
uniform float Radial; uniform sampler2D RadialTex; uniform int RadialTexConnected;
uniform float CenterX; uniform sampler2D CenterXTex; uniform int CenterXTexConnected;
uniform float CenterY; uniform sampler2D CenterYTex; uniform int CenterYTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; float angle=radians(seRange(AngleTex,AngleTexConnected,Angle,-180.0,180.0,uv)); vec2 linear=vec2(cos(angle),sin(angle)); vec2 center=vec2(seUnit(CenterXTex,CenterXTexConnected,CenterX,uv),seUnit(CenterYTex,CenterYTexConnected,CenterY,uv)); vec2 radial=length(uv-center)>SE_EPSILON?normalize((uv-center)*uResolution):linear; vec2 candidate=mix(linear,radial,seUnit(RadialTex,RadialTexConnected,Radial,uv)); vec2 direction=length(candidate)>SE_EPSILON?normalize(candidate):linear; vec2 offset=direction*seRange(AmountTex,AmountTexConnected,Amount,-100.0,100.0,uv)/uResolution; vec4 s=texture(tSource,uv); vec4 split=vec4(texture(tSource,uv+offset).r,s.g,texture(tSource,uv-offset).b,s.a); out_color=mix(s,split,seUnit(MixTex,MixTexConnected,Mix,uv)); }
