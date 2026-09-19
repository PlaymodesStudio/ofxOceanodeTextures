//Threshold:0.5:0:1, Softness:0.1:0:1, Curve:1:0.1:8, Preview:0:0:1, Premultiply:1:0:1
// @description Keeps dark luminance values and removes bright values with a controllable soft rolloff.
// @param Threshold: Luminance at the center of the key transition.
// @param Softness: Width of the transition between retained and removed values.
// @param Curve: Shapes the retained matte after thresholding.
// @param Preview: Crossfades from the keyed image to a grayscale matte preview.
// @param Premultiply: Multiplies RGB by the generated matte.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution;
uniform float Threshold; uniform sampler2D ThresholdTex; uniform int ThresholdTexConnected;
uniform float Softness; uniform sampler2D SoftnessTex; uniform int SoftnessTexConnected;
uniform float Curve; uniform sampler2D CurveTex; uniform int CurveTexConnected;
uniform float Preview; uniform sampler2D PreviewTex; uniform int PreviewTexConnected;
uniform float Premultiply; uniform sampler2D PremultiplyTex; uniform int PremultiplyTexConnected;
out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); float t=seUnit(ThresholdTex,ThresholdTexConnected,Threshold,uv); float soft=seUnit(SoftnessTex,SoftnessTexConnected,Softness,uv); float edge=soft<=SE_EPSILON?step(seLuma(s.rgb),t):1.0-smoothstep(t-soft*0.5,t+soft*0.5,seLuma(s.rgb)); float curve=seRange(CurveTex,CurveTexConnected,Curve,0.1,8.0,uv); float matte=pow(clamp(edge,0.0,1.0),curve); float pre=seUnit(PremultiplyTex,PremultiplyTexConnected,Premultiply,uv); vec4 keyed=vec4(s.rgb*mix(1.0,matte,pre),s.a*matte); float preview=seUnit(PreviewTex,PreviewTexConnected,Preview,uv); out_color=mix(keyed,vec4(vec3(matte),1.0),preview); }
