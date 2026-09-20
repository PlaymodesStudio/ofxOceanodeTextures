//LUT:texture, Size:32:2:64, Mix:1:0:1
// @description Applies a horizontal-strip two-dimensional lookup table to source RGB.
// @param LUT: LUT image laid out as Size tiles horizontally by one tile vertically.
// @param Size: Cube dimension used when the LUT was exported.
// @param Mix: Blends between source and LUT-processed color.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform sampler2D LUT; uniform int LUTConnected; uniform vec2 uResolution; uniform float Size;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
vec3 sampleLut(vec3 color,float size){ color=clamp(color,0.0,1.0); float blue=color.b*(size-1.0); float z0=floor(blue), z1=min(z0+1.0,size-1.0); vec2 texel=vec2(1.0/(size*size),1.0/size); vec2 uv0=vec2((z0*size+color.r*(size-1.0)+0.5)*texel.x,(color.g*(size-1.0)+0.5)*texel.y); vec2 uv1=vec2((z1*size+color.r*(size-1.0)+0.5)*texel.x,(color.g*(size-1.0)+0.5)*texel.y); return mix(texture(LUT,uv0).rgb,texture(LUT,uv1).rgb,fract(blue)); }
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); if(LUTConnected==0){out_color=s;return;} float size=clamp(round(Size),2.0,64.0); vec3 graded=sampleLut(s.rgb,size); out_color=vec4(mix(s.rgb,graded,seUnit(MixTex,MixTexConnected,Mix,uv)),s.a); }
