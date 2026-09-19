//Contrast:1.1:0.25:4, Fade:0.08:0:1, Saturation:0.9:0:2, TintColor:color:1:0.85:0.7:1, TintAmount:0.08:0:1, Grain:0.06:0:0.5, GrainSize:2:1:32, Flicker:0.02:0:0.25, Vignette:0.2:0:1, Mix:1:0:1
// @description Combines faded contrast, warm tint, animated grain, flicker, and vignette into a compact film treatment.
// @param Contrast: Tonal slope around middle gray.
// @param Fade: Raises black values and compresses highlights.
// @param Saturation: Global color saturation.
// @param TintColor: Color used for the film-stock tint.
// @param TintAmount: Strength of the selected tint color.
// @param Grain: Procedural grain amplitude.
// @param GrainSize: Grain cell size in source pixels.
// @param Flicker: Temporal exposure variation.
// @param Vignette: Edge darkening strength.
// @param Mix: Blends between source and the film treatment.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform float uTime; uniform vec4 TintColor;
uniform float Contrast; uniform sampler2D ContrastTex; uniform int ContrastTexConnected; uniform float Fade; uniform sampler2D FadeTex; uniform int FadeTexConnected;
uniform float Saturation; uniform sampler2D SaturationTex; uniform int SaturationTexConnected; uniform float TintAmount; uniform sampler2D TintAmountTex; uniform int TintAmountTexConnected;
uniform float Grain; uniform sampler2D GrainTex; uniform int GrainTexConnected; uniform float GrainSize; uniform sampler2D GrainSizeTex; uniform int GrainSizeTexConnected;
uniform float Flicker; uniform sampler2D FlickerTex; uniform int FlickerTexConnected; uniform float Vignette; uniform sampler2D VignetteTex; uniform int VignetteTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
float hash(vec3 p){p=fract(p*0.1031);p+=dot(p,p.yzx+33.33);return fract((p.x+p.y)*p.z);}
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); float contrast=seRange(ContrastTex,ContrastTexConnected,Contrast,0.25,4.0,uv), fade=seUnit(FadeTex,FadeTexConnected,Fade,uv); vec3 color=(s.rgb-0.5)*contrast+0.5; color=color*(1.0-fade)+fade*0.18; color=mix(vec3(seLuma(color)),color,seRange(SaturationTex,SaturationTexConnected,Saturation,0.0,2.0,uv)); color=mix(color,color*TintColor.rgb,seUnit(TintAmountTex,TintAmountTexConnected,TintAmount,uv)); float size=seRange(GrainSizeTex,GrainSizeTexConnected,GrainSize,1.0,32.0,uv); float noise=hash(vec3(floor(gl_FragCoord.xy/size),floor(uTime*24.0)))-0.5; color+=noise*seRange(GrainTex,GrainTexConnected,Grain,0.0,0.5,uv); color*=1.0+sin(uTime*37.0)*seRange(FlickerTex,FlickerTexConnected,Flicker,0.0,0.25,uv); vec2 p=(uv-0.5)*2.0; color*=1.0-smoothstep(0.4,1.4,length(p))*seUnit(VignetteTex,VignetteTexConnected,Vignette,uv); out_color=vec4(mix(s.rgb,color,seUnit(MixTex,MixTexConnected,Mix,uv)),s.a); }
