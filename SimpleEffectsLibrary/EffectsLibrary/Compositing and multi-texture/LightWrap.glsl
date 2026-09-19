//Background:texture, Radius:8:0:64, Gain:1:0:4, Threshold:0:0:1, Mix:1:0:1
// @description Wraps blurred background color into translucent foreground edges for softer integration.
// @param Background: Image whose color wraps into the foreground.
// @param Radius: Edge search radius in source pixels.
// @param Gain: Strength of the wrapped light.
// @param Threshold: Suppresses dim background light before wrapping.
// @param Mix: Blends between source and light-wrapped result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform sampler2D Background; uniform int BackgroundConnected; uniform vec2 uResolution;
uniform float Radius; uniform sampler2D RadiusTex; uniform int RadiusTexConnected;
uniform float Gain; uniform sampler2D GainTex; uniform int GainTexConnected;
uniform float Threshold; uniform sampler2D ThresholdTex; uniform int ThresholdTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); if(BackgroundConnected==0){out_color=s;return;} float radius=seRange(RadiusTex,RadiusTexConnected,Radius,0.0,64.0,uv); vec3 bg=vec3(0.0); float blurredAlpha=0.0; for(int i=0;i<12;i++){ float angle=2.0*SE_PI*float(i)/12.0; vec2 q=uv+vec2(cos(angle),sin(angle))*radius/uResolution; bg+=texture(Background,q).rgb; blurredAlpha+=texture(tSource,q).a; } bg/=12.0; blurredAlpha/=12.0; float edge=max(blurredAlpha-s.a,0.0); float threshold=seUnit(ThresholdTex,ThresholdTexConnected,Threshold,uv); bg=max(bg-vec3(threshold),vec3(0.0)); vec3 wrapped=s.rgb+bg*edge*seRange(GainTex,GainTexConnected,Gain,0.0,4.0,uv); out_color=vec4(mix(s.rgb,wrapped,seUnit(MixTex,MixTexConnected,Mix,uv)),s.a); }
