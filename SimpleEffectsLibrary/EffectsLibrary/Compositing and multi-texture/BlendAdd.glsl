//LayerB:texture, Opacity:1:0:1, Mix:1:0:1, Clamp:0:0:1
// @description Adds a second texture to the source with optional HDR-preserving output.
// @param LayerB: Image added to the source.
// @param Opacity: Scales LayerB including its alpha contribution.
// @param Mix: Blends between source and the Add result.
// @param Clamp: Crossfades between unclamped HDR and zero-to-one RGB.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform sampler2D LayerB; uniform int LayerBConnected; uniform vec2 uResolution;
uniform float Opacity; uniform sampler2D OpacityTex; uniform int OpacityTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected;
uniform float Clamp; uniform sampler2D ClampTex; uniform int ClampTexConnected;
out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 a=texture(tSource,uv); if(LayerBConnected==0){ out_color=a; return; } vec4 b=texture(LayerB,uv); float opacity=seUnit(OpacityTex,OpacityTexConnected,Opacity,uv); vec3 added=a.rgb+b.rgb*b.a*opacity; added=mix(added,clamp(added,0.0,1.0),seUnit(ClampTex,ClampTexConnected,Clamp,uv)); float alpha=a.a+b.a*opacity*(1.0-a.a); vec4 result=vec4(added,alpha); out_color=mix(a,result,seUnit(MixTex,MixTexConnected,Mix,uv)); }
