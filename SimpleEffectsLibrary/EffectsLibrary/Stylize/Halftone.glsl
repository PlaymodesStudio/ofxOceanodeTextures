//CellSize:12:2:128, Angle:45:-180:180, Contrast:1:0.1:8, Shape:0:0:2, Mix:1:0:1
// @description Converts luminance into a rotatable grid of dots, lines, or square halftone cells.
// @param CellSize: Halftone cell size measured in source pixels.
// @param Angle: Pattern rotation in degrees.
// @param Contrast: Tonal contrast before halftone conversion.
// @param Shape: Selects circles zero, lines one, or squares two.
// @param Mix: Blends between source and halftone result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform float Shape;
uniform float CellSize; uniform sampler2D CellSizeTex; uniform int CellSizeTexConnected;
uniform float Angle; uniform sampler2D AngleTex; uniform int AngleTexConnected;
uniform float Contrast; uniform sampler2D ContrastTex; uniform int ContrastTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); float size=seRange(CellSizeTex,CellSizeTexConnected,CellSize,2.0,128.0,uv), angle=radians(seRange(AngleTex,AngleTexConnected,Angle,-180.0,180.0,uv)); mat2 rot=mat2(cos(angle),-sin(angle),sin(angle),cos(angle)); vec2 p=rot*(gl_FragCoord.xy-uResolution*0.5)/size; vec2 local=fract(p)-0.5; float l=clamp((seLuma(s.rgb)-0.5)*seRange(ContrastTex,ContrastTexConnected,Contrast,0.1,8.0,uv)+0.5,0.0,1.0); int shape=clamp(int(round(Shape)),0,2); float d=shape==0?length(local):shape==1?abs(local.y):max(abs(local.x),abs(local.y)); float radius=mix(0.48,0.02,l); float ink=1.0-smoothstep(radius-0.02,radius+0.02,d); vec3 result=vec3(1.0-ink); out_color=vec4(mix(s.rgb,result,seUnit(MixTex,MixTexConnected,Mix,uv)),s.a); }
