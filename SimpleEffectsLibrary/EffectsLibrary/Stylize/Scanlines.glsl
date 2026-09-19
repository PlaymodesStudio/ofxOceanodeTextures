//Spacing:4:1:128, Width:0.5:0:1, Darkness:0.35:0:1, Offset:0:-10:10, Speed:0:-10:10, Flicker:0:0:1
// @description Overlays animated horizontal scanlines with optional display flicker.
// @param Spacing: Distance between scanlines in source pixels.
// @param Width: Fraction of each spacing occupied by the dark line.
// @param Darkness: Maximum scanline attenuation.
// @param Offset: Manual vertical pattern offset measured in cycles.
// @param Speed: Automatic vertical movement in cycles per second.
// @param Flicker: Strength of whole-frame brightness fluctuation.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform float uTime;
uniform float Spacing; uniform sampler2D SpacingTex; uniform int SpacingTexConnected;
uniform float Width; uniform sampler2D WidthTex; uniform int WidthTexConnected;
uniform float Darkness; uniform sampler2D DarknessTex; uniform int DarknessTexConnected;
uniform float Offset; uniform sampler2D OffsetTex; uniform int OffsetTexConnected;
uniform float Speed; uniform sampler2D SpeedTex; uniform int SpeedTexConnected;
uniform float Flicker; uniform sampler2D FlickerTex; uniform int FlickerTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); float spacing=seRange(SpacingTex,SpacingTexConnected,Spacing,1.0,128.0,uv); float phase=gl_FragCoord.y/spacing+seRange(OffsetTex,OffsetTexConnected,Offset,-10.0,10.0,uv)+uTime*seRange(SpeedTex,SpeedTexConnected,Speed,-10.0,10.0,uv); float line=step(fract(phase),seUnit(WidthTex,WidthTexConnected,Width,uv)); float attenuation=1.0-line*seUnit(DarknessTex,DarknessTexConnected,Darkness,uv); attenuation*=1.0+(sin(uTime*53.0)*0.5+sin(uTime*97.0)*0.5)*0.05*seUnit(FlickerTex,FlickerTexConnected,Flicker,uv); out_color=vec4(s.rgb*attenuation,s.a); }
