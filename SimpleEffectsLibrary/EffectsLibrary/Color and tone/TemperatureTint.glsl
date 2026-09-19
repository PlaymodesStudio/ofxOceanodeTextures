//Temperature:0:-1:1, Tint:0:-1:1, Mix:1:0:1
// @description Applies an approximate warm-cool and green-magenta white-balance adjustment.
// @param Temperature: Moves the image between cool blue and warm orange.
// @param Tint: Moves the image between green and magenta.
// @param Mix: Blends between the source and corrected result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution;
uniform float Temperature; uniform sampler2D TemperatureTex; uniform int TemperatureTexConnected;
uniform float Tint; uniform sampler2D TintTex; uniform int TintTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected;
out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); float t=seRange(TemperatureTex,TemperatureTexConnected,Temperature,-1.0,1.0,uv); float g=seRange(TintTex,TintTexConnected,Tint,-1.0,1.0,uv); vec3 balanced=s.rgb+vec3(0.12*t+0.05*g,-0.03*t-0.10*g,-0.12*t+0.05*g); float amount=seUnit(MixTex,MixTexConnected,Mix,uv); out_color=vec4(mix(s.rgb,balanced,amount),s.a); }
