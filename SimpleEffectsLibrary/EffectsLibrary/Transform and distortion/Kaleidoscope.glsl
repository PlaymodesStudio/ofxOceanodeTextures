//Segments:6:1:64, Rotation:0:-180:180, CenterX:0.5:0:1, CenterY:0.5:0:1, Zoom:1:0.1:10, Mix:1:0:1
// @description Folds polar image sectors into a centered kaleidoscopic repetition.
// @param Segments: Number of reflected angular sectors.
// @param Rotation: Angular offset in degrees.
// @param CenterX: Horizontal kaleidoscope center.
// @param CenterY: Vertical kaleidoscope center.
// @param Zoom: Radial magnification of the sampled image.
// @param Mix: Blends between source and kaleidoscope.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform float Segments;
uniform float Rotation; uniform sampler2D RotationTex; uniform int RotationTexConnected;
uniform float CenterX; uniform sampler2D CenterXTex; uniform int CenterXTexConnected;
uniform float CenterY; uniform sampler2D CenterYTex; uniform int CenterYTexConnected;
uniform float Zoom; uniform sampler2D ZoomTex; uniform int ZoomTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec2 center=vec2(seUnit(CenterXTex,CenterXTexConnected,CenterX,uv),seUnit(CenterYTex,CenterYTexConnected,CenterY,uv)); vec2 p=uv-center; p.x*=uResolution.x/uResolution.y; float radius=length(p)/max(seRange(ZoomTex,ZoomTexConnected,Zoom,0.1,10.0,uv),SE_EPSILON); float angle=atan(p.y,p.x)+radians(seRange(RotationTex,RotationTexConnected,Rotation,-180.0,180.0,uv)); float sector=2.0*SE_PI/max(round(Segments),1.0); angle=abs(mod(angle+sector*0.5,sector)-sector*0.5); vec2 q=vec2(cos(angle),sin(angle))*radius; q.x/=uResolution.x/uResolution.y; q+=center; vec4 result=seInside(q)?texture(tSource,q):vec4(0.0); out_color=mix(texture(tSource,uv),result,seUnit(MixTex,MixTexConnected,Mix,uv)); }
