//Radius:6:0.25:64, Amount:1:0:5, Threshold:0:0:1, Samples:12:4:16, Mix:1:0:1
// @description Sharpens detail by subtracting a soft radial blur with an edge threshold.
// @param Radius: Blur radius used to detect local detail.
// @param Amount: Strength of detected-detail amplification.
// @param Threshold: Minimum luminance difference sharpened.
// @param Samples: Number of radial samples from four through sixteen.
// @param Mix: Blends between source and unsharp result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform float Samples;
uniform float Radius; uniform sampler2D RadiusTex; uniform int RadiusTexConnected;
uniform float Amount; uniform sampler2D AmountTex; uniform int AmountTexConnected;
uniform float Threshold; uniform sampler2D ThresholdTex; uniform int ThresholdTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); int count=clamp(int(round(Samples)),4,16); float radius=seRange(RadiusTex,RadiusTexConnected,Radius,0.25,64.0,uv); vec3 blur=s.rgb; for(int i=0;i<16;i++){if(i>=count)break;float a=2.0*SE_PI*float(i)/float(count);blur+=texture(tSource,uv+vec2(cos(a),sin(a))*radius/uResolution).rgb;} blur/=float(count+1); vec3 detail=s.rgb-blur; float edge=step(seUnit(ThresholdTex,ThresholdTexConnected,Threshold,uv),abs(seLuma(detail))); vec3 result=s.rgb+detail*seRange(AmountTex,AmountTexConnected,Amount,0.0,5.0,uv)*edge; out_color=vec4(mix(s.rgb,result,seUnit(MixTex,MixTexConnected,Mix,uv)),s.a); }
