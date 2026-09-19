//Angle:20:-360:360, Samples:16:1:32, CenterX:0.5:0:1, CenterY:0.5:0:1, Mix:1:0:1
// @description Smears pixels around a center through an adjustable angular arc.
// @param Angle: Total rotational blur arc in degrees.
// @param Samples: Number of angular samples from one through thirty-two.
// @param CenterX: Horizontal rotation center.
// @param CenterY: Vertical rotation center.
// @param Mix: Blends between source and radial blur.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform float Samples;
uniform float Angle; uniform sampler2D AngleTex; uniform int AngleTexConnected;
uniform float CenterX; uniform sampler2D CenterXTex; uniform int CenterXTexConnected;
uniform float CenterY; uniform sampler2D CenterYTex; uniform int CenterYTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec2 center=vec2(seUnit(CenterXTex,CenterXTexConnected,CenterX,uv),seUnit(CenterYTex,CenterYTexConnected,CenterY,uv)); vec2 p=uv-center; int count=clamp(int(round(Samples)),1,32); float arc=radians(seRange(AngleTex,AngleTexConnected,Angle,-360.0,360.0,uv)); vec4 sum=vec4(0.0); for(int i=0;i<32;i++){ if(i>=count) break; float t=count<=1?0.0:float(i)/float(count-1)-0.5; float a=arc*t; mat2 rot=mat2(cos(a),-sin(a),sin(a),cos(a)); sum+=texture(tSource,center+rot*p); } vec4 s=texture(tSource,uv); out_color=mix(s,sum/float(count),seUnit(MixTex,MixTexConnected,Mix,uv)); }
