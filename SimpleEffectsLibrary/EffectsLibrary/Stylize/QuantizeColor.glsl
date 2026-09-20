//RedLevels:8:2:256, GreenLevels:8:2:256, BlueLevels:8:2:256, Dither:0:0:1, Mix:1:0:1
// @description Reduces RGB channels to independent level counts with optional noise dithering.
// @param RedLevels: Number of discrete red values.
// @param GreenLevels: Number of discrete green values.
// @param BlueLevels: Number of discrete blue values.
// @param Dither: Adds sub-level noise before quantization.
// @param Mix: Blends between source and quantized color.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform float RedLevels; uniform float GreenLevels; uniform float BlueLevels;
uniform float Dither; uniform sampler2D DitherTex; uniform int DitherTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
float hash(vec2 p){return fract(sin(dot(p,vec2(12.9898,78.233)))*43758.5453);}
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); vec3 levels=max(round(vec3(RedLevels,GreenLevels,BlueLevels)),vec3(2.0)); vec3 noise=vec3(hash(gl_FragCoord.xy),hash(gl_FragCoord.xy+17.0),hash(gl_FragCoord.xy+41.0))-0.5; vec3 sourceColor=s.rgb+noise*seUnit(DitherTex,DitherTexConnected,Dither,uv)/levels; vec3 result=round(sourceColor*(levels-1.0))/(levels-1.0); out_color=vec4(mix(s.rgb,result,seUnit(MixTex,MixTexConnected,Mix,uv)),s.a); }
