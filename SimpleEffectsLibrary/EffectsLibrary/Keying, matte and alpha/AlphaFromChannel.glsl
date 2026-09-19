//Channel:3:0:4, Gain:1:0:4, Offset:0:-1:1, Invert:0:0:1, Premultiply:1:0:1, Preview:0:0:1
// @description Builds output alpha from a selected source channel or luminance.
// @param Channel: Selects red zero, green one, blue two, luminance three, or alpha four.
// @param Gain: Multiplies the selected channel before clipping.
// @param Offset: Adds a value to the selected channel before clipping.
// @param Invert: Inverts the generated alpha.
// @param Premultiply: Multiplies source RGB by the generated alpha.
// @param Preview: Crossfades from the result to a grayscale alpha preview.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform float Channel;
uniform float Gain; uniform sampler2D GainTex; uniform int GainTexConnected;
uniform float Offset; uniform sampler2D OffsetTex; uniform int OffsetTexConnected;
uniform float Invert; uniform sampler2D InvertTex; uniform int InvertTexConnected;
uniform float Premultiply; uniform sampler2D PremultiplyTex; uniform int PremultiplyTexConnected;
uniform float Preview; uniform sampler2D PreviewTex; uniform int PreviewTexConnected;
out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); int channel=clamp(int(round(Channel)),0,4); float v=channel==0?s.r:channel==1?s.g:channel==2?s.b:channel==3?seLuma(s.rgb):s.a; v=clamp(v*seRange(GainTex,GainTexConnected,Gain,0.0,4.0,uv)+seRange(OffsetTex,OffsetTexConnected,Offset,-1.0,1.0,uv),0.0,1.0); v=mix(v,1.0-v,seUnit(InvertTex,InvertTexConnected,Invert,uv)); float pre=seUnit(PremultiplyTex,PremultiplyTexConnected,Premultiply,uv); vec4 result=vec4(s.rgb*mix(1.0,v,pre),v); out_color=mix(result,vec4(vec3(v),1.0),seUnit(PreviewTex,PreviewTexConnected,Preview,uv)); }
