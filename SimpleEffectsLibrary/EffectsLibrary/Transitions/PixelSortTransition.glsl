//LayerB:texture, Progress:0:0:1, Threshold:0.5:0:1, Length:120:0:1000, Direction:0:0:1, Softness:0.08:0:0.5
// @description Creates a pixel-sort-inspired transition using luminance-gated horizontal streaks.
// @param LayerB: Image replacing the source.
// @param Progress: Transition completion from source zero to LayerB one.
// @param Threshold: Luminance bias controlling which rows streak first.
// @param Length: Maximum streak length in source pixels.
// @param Direction: Reverses horizontal streak direction.
// @param Softness: Feather width of the luminance reveal.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform sampler2D LayerB; uniform int LayerBConnected; uniform vec2 uResolution;
uniform float Progress; uniform sampler2D ProgressTex; uniform int ProgressTexConnected; uniform float Threshold; uniform sampler2D ThresholdTex; uniform int ThresholdTexConnected;
uniform float Length; uniform sampler2D LengthTex; uniform int LengthTexConnected; uniform float Direction; uniform sampler2D DirectionTex; uniform int DirectionTexConnected;
uniform float Softness; uniform sampler2D SoftnessTex; uniform int SoftnessTexConnected; out vec4 out_color;
float hash(float p){return fract(sin(p*127.1)*43758.5453);}
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 original=texture(tSource,uv); if(LayerBConnected==0){out_color=original;return;} vec4 target=texture(LayerB,uv); float p=seUnit(ProgressTex,ProgressTexConnected,Progress,uv); if(p<=0.0){out_color=original;return;} if(p>=1.0){out_color=target;return;} float row=hash(floor(gl_FragCoord.y/3.0)); float l=seLuma(original.rgb), threshold=seUnit(ThresholdTex,ThresholdTexConnected,Threshold,uv); float soft=seRange(SoftnessTex,SoftnessTexConnected,Softness,0.0,0.5,uv), boundary=l*(1.0-threshold)+row*threshold; float reveal=soft<=SE_EPSILON?step(boundary,p):smoothstep(boundary-soft,boundary+soft,p); float direction=mix(-1.0,1.0,seUnit(DirectionTex,DirectionTexConnected,Direction,uv)); float streak=seRange(LengthTex,LengthTexConnected,Length,0.0,1000.0,uv)*sin(p*SE_PI)*direction/uResolution.x; vec4 a=texture(tSource,vec2(fract(uv.x+streak*(1.0-reveal)),uv.y)), b=texture(LayerB,vec2(fract(uv.x-streak*reveal),uv.y)); out_color=mix(a,b,reveal); }
