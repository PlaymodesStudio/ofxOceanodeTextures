//Radius:8:0.25:64, Contrast:1:0:5, Samples:12:4:16, Monochrome:0:0:1, Mix:1:0:1
// @description Isolates image detail around middle gray using a local radial average.
// @param Radius: Neighborhood radius used to separate high frequencies.
// @param Contrast: Strength of isolated detail around middle gray.
// @param Samples: Number of radial samples from four through sixteen.
// @param Monochrome: Crossfades color detail into luminance-only detail.
// @param Mix: Blends between source and high-pass result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform float Samples;
uniform float Radius; uniform sampler2D RadiusTex; uniform int RadiusTexConnected;
uniform float Contrast; uniform sampler2D ContrastTex; uniform int ContrastTexConnected;
uniform float Monochrome; uniform sampler2D MonochromeTex; uniform int MonochromeTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); int count=clamp(int(round(Samples)),4,16); float radius=seRange(RadiusTex,RadiusTexConnected,Radius,0.25,64.0,uv); vec3 blur=s.rgb; for(int i=0;i<16;i++){if(i>=count)break;float a=2.0*SE_PI*float(i)/float(count);blur+=texture(tSource,uv+vec2(cos(a),sin(a))*radius/uResolution).rgb;} blur/=float(count+1); vec3 detail=(s.rgb-blur)*seRange(ContrastTex,ContrastTexConnected,Contrast,0.0,5.0,uv)+0.5; detail=mix(detail,vec3(seLuma(detail)),seUnit(MonochromeTex,MonochromeTexConnected,Monochrome,uv)); out_color=vec4(mix(s.rgb,detail,seUnit(MixTex,MixTexConnected,Mix,uv)),s.a); }
