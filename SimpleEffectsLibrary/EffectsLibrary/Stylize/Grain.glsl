//Amount:0.1:0:1, Size:1:1:64, Seed:0:0:1000, Colored:0:0:1, Temporal:1:0:1, Mix:1:0:1
// @description Adds scalable monochrome or colored procedural grain with optional temporal motion.
// @param Amount: Grain amplitude added around the source value.
// @param Size: Grain cell size in source pixels.
// @param Seed: Static variation seed.
// @param Colored: Crossfades monochrome noise into independent RGB noise.
// @param Temporal: Controls how strongly the grain changes over time.
// @param Mix: Blends between source and grain result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform float uTime;
uniform float Amount; uniform sampler2D AmountTex; uniform int AmountTexConnected;
uniform float Size; uniform sampler2D SizeTex; uniform int SizeTexConnected;
uniform float Seed; uniform sampler2D SeedTex; uniform int SeedTexConnected;
uniform float Colored; uniform sampler2D ColoredTex; uniform int ColoredTexConnected;
uniform float Temporal; uniform sampler2D TemporalTex; uniform int TemporalTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
float hash(vec3 p){p=fract(p*0.1031);p+=dot(p,p.yzx+33.33);return fract((p.x+p.y)*p.z);}
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); float size=seRange(SizeTex,SizeTexConnected,Size,1.0,64.0,uv), seed=seRange(SeedTex,SeedTexConnected,Seed,0.0,1000.0,uv); float frame=floor(uTime*24.0)*seUnit(TemporalTex,TemporalTexConnected,Temporal,uv); vec2 cell=floor(gl_FragCoord.xy/size); vec3 n=vec3(hash(vec3(cell,seed+frame)),hash(vec3(cell+17.0,seed+frame)),hash(vec3(cell+43.0,seed+frame)))-0.5; n=mix(vec3(n.r),n,seUnit(ColoredTex,ColoredTexConnected,Colored,uv)); vec3 result=s.rgb+n*seUnit(AmountTex,AmountTexConnected,Amount,uv); out_color=vec4(mix(s.rgb,result,seUnit(MixTex,MixTexConnected,Mix,uv)),s.a); }
