//LayerB:texture, Progress:0:0:1, Softness:0.05:0:0.5, Scale:32:1:512, Seed:0:0:1000, Speed:0:0:10, EdgeColor:color:1:1:1:1, EdgeWidth:0:0:0.25
// @description Reveals a second texture through scalable procedural noise with an optional colored edge.
// @param LayerB: Image revealed by the noise field.
// @param Progress: Transition completion from source zero to LayerB one.
// @param Softness: Feather width around the dissolve threshold.
// @param Scale: Number of noise cells across the image width.
// @param Seed: Static noise variation seed.
// @param Speed: Temporal noise update speed.
// @param EdgeColor: Color placed around the active dissolve boundary.
// @param EdgeWidth: Width of the colored threshold band.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform sampler2D LayerB; uniform int LayerBConnected; uniform vec2 uResolution; uniform float uTime; uniform vec4 EdgeColor;
uniform float Progress; uniform sampler2D ProgressTex; uniform int ProgressTexConnected; uniform float Softness; uniform sampler2D SoftnessTex; uniform int SoftnessTexConnected;
uniform float Scale; uniform sampler2D ScaleTex; uniform int ScaleTexConnected; uniform float Seed; uniform sampler2D SeedTex; uniform int SeedTexConnected;
uniform float Speed; uniform sampler2D SpeedTex; uniform int SpeedTexConnected; uniform float EdgeWidth; uniform sampler2D EdgeWidthTex; uniform int EdgeWidthTexConnected; out vec4 out_color;
float hash(vec3 p){p=fract(p*0.1031);p+=dot(p,p.yzx+33.33);return fract((p.x+p.y)*p.z);}
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 a=texture(tSource,uv); if(LayerBConnected==0){out_color=a;return;} vec4 b=texture(LayerB,uv); float p=seUnit(ProgressTex,ProgressTexConnected,Progress,uv); if(p<=0.0){out_color=a;return;} if(p>=1.0){out_color=b;return;} float scale=seRange(ScaleTex,ScaleTexConnected,Scale,1.0,512.0,uv), seed=seRange(SeedTex,SeedTexConnected,Seed,0.0,1000.0,uv), speed=seRange(SpeedTex,SpeedTexConnected,Speed,0.0,10.0,uv); float noise=hash(vec3(floor(uv*scale),seed+floor(uTime*speed))); float soft=seRange(SoftnessTex,SoftnessTexConnected,Softness,0.0,0.5,uv); float matte=soft<=SE_EPSILON?step(noise,p):smoothstep(noise-soft,noise+soft,p); vec4 result=mix(a,b,matte); float width=seRange(EdgeWidthTex,EdgeWidthTexConnected,EdgeWidth,0.0,0.25,uv); float edge=0.0; if(width>SE_EPSILON) edge=soft<=SE_EPSILON?1.0-step(width,abs(noise-p)):1.0-smoothstep(width,width+soft,abs(noise-p)); out_color=mix(result,EdgeColor,edge*EdgeColor.a); }
