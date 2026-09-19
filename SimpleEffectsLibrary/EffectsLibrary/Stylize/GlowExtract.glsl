//Threshold:0.75:0:1, Softness:0.1:0:1, Radius:12:0:96, Gain:1:0:8, GlowColor:color:1:1:1:1, Composite:1:0:1, Mix:1:0:1
// @description Extracts bright pixels, softens them radially, and optionally adds the glow over the source.
// @param Threshold: Luminance where pixels begin contributing to glow.
// @param Softness: Width of bright-pixel extraction.
// @param Radius: Glow spread in source pixels.
// @param Gain: Glow brightness multiplier.
// @param GlowColor: Color tint applied to extracted light.
// @param Composite: Crossfades isolated glow into glow added over source.
// @param Mix: Blends between source and the selected glow result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform vec4 GlowColor;
uniform float Threshold; uniform sampler2D ThresholdTex; uniform int ThresholdTexConnected; uniform float Softness; uniform sampler2D SoftnessTex; uniform int SoftnessTexConnected;
uniform float Radius; uniform sampler2D RadiusTex; uniform int RadiusTexConnected; uniform float Gain; uniform sampler2D GainTex; uniform int GainTexConnected;
uniform float Composite; uniform sampler2D CompositeTex; uniform int CompositeTexConnected; uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); float threshold=seUnit(ThresholdTex,ThresholdTexConnected,Threshold,uv), soft=seUnit(SoftnessTex,SoftnessTexConnected,Softness,uv), radius=seRange(RadiusTex,RadiusTexConnected,Radius,0.0,96.0,uv); vec3 glow=vec3(0.0); float alpha=0.0; for(int i=0;i<16;i++){float a=2.0*SE_PI*float(i)/16.0;vec4 q=texture(tSource,uv+vec2(cos(a),sin(a))*radius/uResolution);float m=soft<=SE_EPSILON?step(threshold,seLuma(q.rgb)):smoothstep(threshold-soft,threshold+soft,seLuma(q.rgb));glow+=q.rgb*m;alpha+=q.a*m;} glow=glow/16.0*GlowColor.rgb*seRange(GainTex,GainTexConnected,Gain,0.0,8.0,uv); alpha=alpha/16.0*GlowColor.a; vec4 isolated=vec4(glow,alpha); vec4 added=vec4(s.rgb+glow,s.a); vec4 selected=mix(isolated,added,seUnit(CompositeTex,CompositeTexConnected,Composite,uv)); out_color=mix(s,selected,seUnit(MixTex,MixTexConnected,Mix,uv)); }
