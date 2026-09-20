//Axis:0:0:3, CenterX:0.5:0:1, CenterY:0.5:0:1, Direction:0:0:1, Mix:1:0:1
// @description Reflects one side of an image across a vertical, horizontal, or diagonal axis.
// @param Axis: Selects vertical zero, horizontal one, descending diagonal two, or ascending diagonal three.
// @param CenterX: Horizontal position of the reflection axis.
// @param CenterY: Vertical position of the reflection axis.
// @param Direction: Selects which side of the axis supplies the reflected image.
// @param Mix: Blends between source and mirrored result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform float Axis;
uniform float CenterX; uniform sampler2D CenterXTex; uniform int CenterXTexConnected;
uniform float CenterY; uniform sampler2D CenterYTex; uniform int CenterYTexConnected;
uniform float Direction; uniform sampler2D DirectionTex; uniform int DirectionTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec2 q=uv; float cx=seUnit(CenterXTex,CenterXTexConnected,CenterX,uv), cy=seUnit(CenterYTex,CenterYTexConnected,CenterY,uv); float dir=seUnit(DirectionTex,DirectionTexConnected,Direction,uv); int axis=clamp(int(round(Axis)),0,3); if(axis==0) q.x=dir<0.5?cx-abs(uv.x-cx):cx+abs(uv.x-cx); else if(axis==1) q.y=dir<0.5?cy-abs(uv.y-cy):cy+abs(uv.y-cy); else { vec2 p=uv-vec2(cx,cy); if(axis==2){ float side=p.x+p.y; if((dir<0.5&&side>0.0)||(dir>=0.5&&side<0.0)) p=-p.yx; } else { float side=p.x-p.y; if((dir<0.5&&side>0.0)||(dir>=0.5&&side<0.0)) p=p.yx; } q=p+vec2(cx,cy); } vec4 s=texture(tSource,uv); vec4 mirrored=seInside(q)?texture(tSource,q):vec4(0.0); out_color=mix(s,mirrored,seUnit(MixTex,MixTexConnected,Mix,uv)); }
