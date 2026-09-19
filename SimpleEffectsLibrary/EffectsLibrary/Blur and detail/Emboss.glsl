//Angle:45:-180:180, Distance:2:0:32, Strength:1:0:8, Bias:0.5:0:1, Color:0:0:1, Mix:1:0:1
// @description Converts directional luminance differences into a raised or engraved relief.
// @param Angle: Lighting and relief direction in degrees.
// @param Distance: Sample separation in source pixels.
// @param Strength: Contrast of the embossed relief.
// @param Bias: Base gray level behind the relief.
// @param Color: Crossfades grayscale relief toward source-tinted relief.
// @param Mix: Blends between source and embossed result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution;
uniform float Angle; uniform sampler2D AngleTex; uniform int AngleTexConnected;
uniform float Distance; uniform sampler2D DistanceTex; uniform int DistanceTexConnected;
uniform float Strength; uniform sampler2D StrengthTex; uniform int StrengthTexConnected;
uniform float Bias; uniform sampler2D BiasTex; uniform int BiasTexConnected;
uniform float Color; uniform sampler2D ColorTex; uniform int ColorTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); float angle=radians(seRange(AngleTex,AngleTexConnected,Angle,-180.0,180.0,uv)); vec2 offset=vec2(cos(angle),sin(angle))*seRange(DistanceTex,DistanceTexConnected,Distance,0.0,32.0,uv)/uResolution; float relief=(seLuma(texture(tSource,uv+offset).rgb)-seLuma(texture(tSource,uv-offset).rgb))*seRange(StrengthTex,StrengthTexConnected,Strength,0.0,8.0,uv)+seUnit(BiasTex,BiasTexConnected,Bias,uv); vec3 result=mix(vec3(relief),s.rgb*relief*2.0,seUnit(ColorTex,ColorTexConnected,Color,uv)); out_color=vec4(mix(s.rgb,result,seUnit(MixTex,MixTexConnected,Mix,uv)),s.a); }
