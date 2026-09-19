//Amount:0.35:0:1, Blocks:24:2:128, Jitter:20:0:200, Noise:0.15:0:1, RGBSplit:6:0:100, Scanlines:0.25:0:1, Speed:1:0:10, Mix:1:0:1
// @description Combines block jitter, noise, RGB separation, and scanlines into an animated damaged-video look.
// @param Amount: Probability and strength of horizontal block disruptions.
// @param Blocks: Number of horizontal timing bands.
// @param Jitter: Maximum horizontal displacement in pixels.
// @param Noise: Added signal-noise amplitude.
// @param RGBSplit: Red-blue channel separation in pixels.
// @param Scanlines: Darkness of alternating scanlines.
// @param Speed: Temporal update speed.
// @param Mix: Blends between source and glitch result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform float uTime; uniform float Blocks;
uniform float Amount; uniform sampler2D AmountTex; uniform int AmountTexConnected;
uniform float Jitter; uniform sampler2D JitterTex; uniform int JitterTexConnected;
uniform float Noise; uniform sampler2D NoiseTex; uniform int NoiseTexConnected;
uniform float RGBSplit; uniform sampler2D RGBSplitTex; uniform int RGBSplitTexConnected;
uniform float Scanlines; uniform sampler2D ScanlinesTex; uniform int ScanlinesTexConnected;
uniform float Speed; uniform sampler2D SpeedTex; uniform int SpeedTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); float frame=floor(uTime*12.0*seRange(SpeedTex,SpeedTexConnected,Speed,0.0,10.0,uv)); float band=floor(uv.y*max(round(Blocks),2.0)); float rnd=hash(vec2(band,frame)); float enabled=step(1.0-seUnit(AmountTex,AmountTexConnected,Amount,uv),rnd); float shift=(rnd-0.5)*2.0*enabled*seRange(JitterTex,JitterTexConnected,Jitter,0.0,200.0,uv)/uResolution.x; float split=seRange(RGBSplitTex,RGBSplitTexConnected,RGBSplit,0.0,100.0,uv)/uResolution.x; vec2 q=vec2(fract(uv.x+shift),uv.y); vec3 result=vec3(texture(tSource,q+vec2(split,0)).r,texture(tSource,q).g,texture(tSource,q-vec2(split,0)).b); float noise=(hash(gl_FragCoord.xy+frame)-0.5)*seUnit(NoiseTex,NoiseTexConnected,Noise,uv); result+=noise; result*=1.0-mod(floor(gl_FragCoord.y),2.0)*seUnit(ScanlinesTex,ScanlinesTexConnected,Scanlines,uv); out_color=vec4(mix(s.rgb,result,seUnit(MixTex,MixTexConnected,Mix,uv)),s.a); }
