//LayerB:texture, Progress:0:0:1, Softness:0.05:0:0.5, NoiseScale:48:1:512, BurnWidth:0.08:0:0.5, BurnColor:color:1:0.2:0.01:1, BurnIntensity:2:0:10, Seed:0:0:1000
// @description Dissolves into a second texture through noise with a bright colored burn front.
// @param LayerB: Image revealed behind the burn front.
// @param Progress: Transition completion from source zero to LayerB one.
// @param Softness: Feather width of the dissolve boundary.
// @param NoiseScale: Number of procedural cells across the image width.
// @param BurnWidth: Width of the colored burn band.
// @param BurnColor: Color of the active burn front.
// @param BurnIntensity: HDR brightness multiplier for the burn color.
// @param Seed: Static procedural variation seed.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform sampler2D LayerB; uniform int LayerBConnected; uniform vec2 uResolution; uniform vec4 BurnColor;
uniform float Progress; uniform sampler2D ProgressTex; uniform int ProgressTexConnected; uniform float Softness; uniform sampler2D SoftnessTex; uniform int SoftnessTexConnected;
uniform float NoiseScale; uniform sampler2D NoiseScaleTex; uniform int NoiseScaleTexConnected; uniform float BurnWidth; uniform sampler2D BurnWidthTex; uniform int BurnWidthTexConnected;
uniform float BurnIntensity; uniform sampler2D BurnIntensityTex; uniform int BurnIntensityTexConnected; uniform float Seed; uniform sampler2D SeedTex; uniform int SeedTexConnected; out vec4 out_color;
float hash(vec3 p){p=fract(p*0.1031);p+=dot(p,p.yzx+33.33);return fract((p.x+p.y)*p.z);}
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 a=texture(tSource,uv); if(LayerBConnected==0){out_color=a;return;} vec4 b=texture(LayerB,uv); float p=seUnit(ProgressTex,ProgressTexConnected,Progress,uv); if(p<=0.0){out_color=a;return;} if(p>=1.0){out_color=b;return;} float scale=seRange(NoiseScaleTex,NoiseScaleTexConnected,NoiseScale,1.0,512.0,uv), seed=seRange(SeedTex,SeedTexConnected,Seed,0.0,1000.0,uv); float noise=hash(vec3(floor(uv*scale),seed)); float soft=seRange(SoftnessTex,SoftnessTexConnected,Softness,0.0,0.5,uv), width=seRange(BurnWidthTex,BurnWidthTexConnected,BurnWidth,0.0,0.5,uv); float matte=soft<=SE_EPSILON?step(noise,p):smoothstep(noise-soft,noise+soft,p); vec4 result=mix(a,b,matte); float edge=0.0; if(width>SE_EPSILON) edge=soft<=SE_EPSILON?1.0-step(width,abs(noise-p)):1.0-smoothstep(width,width+soft,abs(noise-p)); vec3 burn=BurnColor.rgb*seRange(BurnIntensityTex,BurnIntensityTexConnected,BurnIntensity,0.0,10.0,uv); out_color=vec4(mix(result.rgb,burn,edge*BurnColor.a),result.a); }
