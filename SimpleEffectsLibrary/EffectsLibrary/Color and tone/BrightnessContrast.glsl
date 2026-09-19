//Brightness:0:-1:1, Contrast:0:-1:1, Pivot:0.5:0:1
// @description Adjusts linear brightness and contrast around a configurable tonal pivot.
// @param Brightness: Linear value added to RGB.
// @param Contrast: Contrast reduction below zero and expansion above zero.
// @param Pivot: Tonal value that remains fixed during contrast adjustment.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution;
uniform float Brightness; uniform sampler2D BrightnessTex; uniform int BrightnessTexConnected;
uniform float Contrast; uniform sampler2D ContrastTex; uniform int ContrastTexConnected;
uniform float Pivot; uniform sampler2D PivotTex; uniform int PivotTexConnected;
out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); float b=seRange(BrightnessTex,BrightnessTexConnected,Brightness,-1.0,1.0,uv); float c=seRange(ContrastTex,ContrastTexConnected,Contrast,-1.0,1.0,uv); float p=seUnit(PivotTex,PivotTexConnected,Pivot,uv); float factor=c>=0.0?1.0/(1.0-min(c,0.999)):1.0+c; out_color=vec4((s.rgb-p)*factor+p+b,s.a); }
