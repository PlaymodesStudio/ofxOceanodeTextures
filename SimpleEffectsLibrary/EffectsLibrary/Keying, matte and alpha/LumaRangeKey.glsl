//Low:0.2:0:1, High:0.8:0:1, Softness:0.05:0:0.5, Curve:1:0.1:8, Invert:0:0:1, Preview:0:0:1, Premultiply:1:0:1
// @description Creates a matte from a selectable luminance band with soft lower and upper boundaries.
// @param Low: Lower boundary of the retained luminance band.
// @param High: Upper boundary of the retained luminance band.
// @param Softness: Feather width around both range boundaries.
// @param Curve: Shapes the resulting matte density.
// @param Invert: Inverts the generated matte.
// @param Preview: Crossfades from the keyed image to a grayscale matte preview.
// @param Premultiply: Multiplies RGB by the generated matte.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution;
uniform float Low; uniform sampler2D LowTex; uniform int LowTexConnected;
uniform float High; uniform sampler2D HighTex; uniform int HighTexConnected;
uniform float Softness; uniform sampler2D SoftnessTex; uniform int SoftnessTexConnected;
uniform float Curve; uniform sampler2D CurveTex; uniform int CurveTexConnected;
uniform float Invert; uniform sampler2D InvertTex; uniform int InvertTexConnected;
uniform float Preview; uniform sampler2D PreviewTex; uniform int PreviewTexConnected;
uniform float Premultiply; uniform sampler2D PremultiplyTex; uniform int PremultiplyTexConnected;
out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); float a=seUnit(LowTex,LowTexConnected,Low,uv); float b=seUnit(HighTex,HighTexConnected,High,uv); float lo=min(a,b), hi=max(a,b), l=seLuma(s.rgb); float soft=seRange(SoftnessTex,SoftnessTexConnected,Softness,0.0,0.5,uv); float lower=soft<=SE_EPSILON?step(lo,l):smoothstep(lo-soft,lo+soft,l); float upper=soft<=SE_EPSILON?step(l,hi):1.0-smoothstep(hi-soft,hi+soft,l); float matte=pow(clamp(lower*upper,0.0,1.0),seRange(CurveTex,CurveTexConnected,Curve,0.1,8.0,uv)); matte=mix(matte,1.0-matte,seUnit(InvertTex,InvertTexConnected,Invert,uv)); float pre=seUnit(PremultiplyTex,PremultiplyTexConnected,Premultiply,uv); vec4 keyed=vec4(s.rgb*mix(1.0,matte,pre),s.a*matte); out_color=mix(keyed,vec4(vec3(matte),1.0),seUnit(PreviewTex,PreviewTexConnected,Preview,uv)); }
