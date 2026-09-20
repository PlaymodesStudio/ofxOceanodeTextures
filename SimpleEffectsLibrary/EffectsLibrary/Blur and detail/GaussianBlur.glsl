//Radius:6:0:64, Quality:2:1:4, Mix:1:0:1
// @description Applies a soft two-dimensional Gaussian-style blur with adjustable sampling quality.
// @param Radius: Blur footprint radius in source pixels.
// @param Quality: Sampling grid radius from one through four.
// @param Mix: Blends between source and blurred result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform float Quality;
uniform float Radius; uniform sampler2D RadiusTex; uniform int RadiusTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; float radius=seRange(RadiusTex,RadiusTexConnected,Radius,0.0,64.0,uv); int quality=clamp(int(round(Quality)),1,4); vec4 sum=vec4(0.0); float total=0.0; for(int y=-4;y<=4;y++){ for(int x=-4;x<=4;x++){ if(abs(x)>quality||abs(y)>quality) continue; vec2 grid=vec2(x,y)/float(quality); float weight=exp(-dot(grid,grid)*2.0); sum+=texture(tSource,uv+grid*radius/uResolution)*weight; total+=weight; }} vec4 s=texture(tSource,uv); out_color=mix(s,sum/max(total,SE_EPSILON),seUnit(MixTex,MixTexConnected,Mix,uv)); }
