//Levels:4:2:32, Amount:1:0:1, Monochrome:0:0:1, Mix:1:0:1
// @description Quantizes color with a stable four-by-four ordered dither pattern.
// @param Levels: Number of tonal values retained per channel.
// @param Amount: Strength of the ordered threshold offset.
// @param Monochrome: Crossfades RGB processing into luminance-only dithering.
// @param Mix: Blends between source and dithered result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform float Levels;
uniform float Amount; uniform sampler2D AmountTex; uniform int AmountTexConnected;
uniform float Monochrome; uniform sampler2D MonochromeTex; uniform int MonochromeTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
const float BAYER4[16]=float[16](0.0,8.0,2.0,10.0,12.0,4.0,14.0,6.0,3.0,11.0,1.0,9.0,15.0,7.0,13.0,5.0);
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); float levels=max(round(Levels),2.0); ivec2 cell=ivec2(mod(floor(gl_FragCoord.xy),4.0)); float threshold=(BAYER4[cell.x+cell.y*4]/16.0-0.5)*seUnit(AmountTex,AmountTexConnected,Amount,uv); vec3 sourceColor=mix(s.rgb,vec3(seLuma(s.rgb)),seUnit(MonochromeTex,MonochromeTexConnected,Monochrome,uv)); vec3 result=clamp(floor(clamp(sourceColor,0.0,1.0)*(levels-1.0)+threshold+0.5)/(levels-1.0),0.0,1.0); out_color=vec4(mix(s.rgb,result,seUnit(MixTex,MixTexConnected,Mix,uv)),s.a); }
