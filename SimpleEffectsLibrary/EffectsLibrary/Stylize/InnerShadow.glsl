//ShadowColor:color:0:0:0:1, OffsetX:6:-128:128, OffsetY:-6:-128:128, Blur:8:0:64, Choke:0:0:1, Opacity:0.75:0:1
// @description Shades the inside of source alpha edges with an offset, softened color.
// @param ShadowColor: Color applied inside the alpha boundary.
// @param OffsetX: Horizontal virtual-light offset in pixels.
// @param OffsetY: Vertical virtual-light offset in pixels.
// @param Blur: Radial softness in pixels.
// @param Choke: Hardens the inner shadow boundary.
// @param Opacity: Overall inner-shadow density.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform vec4 ShadowColor;
uniform float OffsetX; uniform sampler2D OffsetXTex; uniform int OffsetXTexConnected; uniform float OffsetY; uniform sampler2D OffsetYTex; uniform int OffsetYTexConnected;
uniform float Blur; uniform sampler2D BlurTex; uniform int BlurTexConnected; uniform float Choke; uniform sampler2D ChokeTex; uniform int ChokeTexConnected;
uniform float Opacity; uniform sampler2D OpacityTex; uniform int OpacityTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); vec2 offset=vec2(seRange(OffsetXTex,OffsetXTexConnected,OffsetX,-128.0,128.0,uv),seRange(OffsetYTex,OffsetYTexConnected,OffsetY,-128.0,128.0,uv))/uResolution; float blur=seRange(BlurTex,BlurTexConnected,Blur,0.0,64.0,uv), outside=0.0; for(int i=0;i<12;i++){float a=2.0*SE_PI*float(i)/12.0;outside+=1.0-texture(tSource,uv+offset+vec2(cos(a),sin(a))*blur/uResolution).a;} outside/=12.0; outside=clamp(outside+seUnit(ChokeTex,ChokeTexConnected,Choke,uv)*(1.0-outside),0.0,1.0)*s.a*seUnit(OpacityTex,OpacityTexConnected,Opacity,uv)*ShadowColor.a; out_color=vec4(mix(s.rgb,ShadowColor.rgb,outside),s.a); }
