//Vibrance:0:-1:1, Saturation:1:0:2
// @description Adjusts muted colors more strongly than already saturated colors.
// @param Vibrance: Adaptive saturation change with reduced influence on vivid pixels.
// @param Saturation: Final global saturation multiplier.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution;
uniform float Vibrance; uniform sampler2D VibranceTex; uniform int VibranceTexConnected;
uniform float Saturation; uniform sampler2D SaturationTex; uniform int SaturationTexConnected;
out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); float v=seRange(VibranceTex,VibranceTexConnected,Vibrance,-1.0,1.0,uv); float sat=seRange(SaturationTex,SaturationTexConnected,Saturation,0.0,2.0,uv); float mx=max(s.r,max(s.g,s.b)); float mn=min(s.r,min(s.g,s.b)); float chroma=mx-mn; float adaptive=1.0+v*(1.0-clamp(chroma,0.0,1.0)); vec3 gray=vec3(seLuma(s.rgb)); out_color=vec4(mix(gray,s.rgb,max(adaptive*sat,0.0)),s.a); }
