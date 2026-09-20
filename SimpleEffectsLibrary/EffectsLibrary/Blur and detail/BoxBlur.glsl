//Radius:4:0:64, Quality:2:1:4, Mix:1:0:1
// @description Averages a square pixel neighborhood for a fast, graphic blur.
// @param Radius: Half-width of the blur footprint in source pixels.
// @param Quality: Sampling grid radius from one through four.
// @param Mix: Blends between source and blurred result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform float Quality;
uniform float Radius; uniform sampler2D RadiusTex; uniform int RadiusTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; float radius=seRange(RadiusTex,RadiusTexConnected,Radius,0.0,64.0,uv); int quality=clamp(int(round(Quality)),1,4); vec4 sum=vec4(0.0); float count=0.0; for(int y=-4;y<=4;y++){ for(int x=-4;x<=4;x++){ if(abs(x)>quality||abs(y)>quality) continue; vec2 offset=vec2(x,y)*(radius/float(quality))/uResolution; sum+=texture(tSource,uv+offset); count+=1.0; }} vec4 s=texture(tSource,uv); out_color=mix(s,sum/max(count,1.0),seUnit(MixTex,MixTexConnected,Mix,uv)); }
