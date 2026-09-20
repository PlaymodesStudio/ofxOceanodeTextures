//AlphaSource:texture, Channel:3:0:4, Amount:1:0:1, Invert:0:0:1, Premultiply:1:0:1
// @description Replaces source alpha using a channel from another texture.
// @param AlphaSource: Texture supplying the new alpha information.
// @param Channel: Selects red zero, green one, blue two, luminance three, or alpha four.
// @param Amount: Blends between original and injected alpha.
// @param Invert: Inverts the selected alpha source.
// @param Premultiply: Multiplies RGB by the final alpha.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform sampler2D AlphaSource; uniform int AlphaSourceConnected; uniform vec2 uResolution; uniform float Channel;
uniform float Amount; uniform sampler2D AmountTex; uniform int AmountTexConnected;
uniform float Invert; uniform sampler2D InvertTex; uniform int InvertTexConnected;
uniform float Premultiply; uniform sampler2D PremultiplyTex; uniform int PremultiplyTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); if(AlphaSourceConnected==0){out_color=s;return;} vec4 m=texture(AlphaSource,uv); int c=clamp(int(round(Channel)),0,4); float alpha=c==0?m.r:c==1?m.g:c==2?m.b:c==3?seLuma(m.rgb):m.a; alpha=mix(alpha,1.0-alpha,seUnit(InvertTex,InvertTexConnected,Invert,uv)); alpha=mix(s.a,alpha,seUnit(AmountTex,AmountTexConnected,Amount,uv)); float pre=seUnit(PremultiplyTex,PremultiplyTexConnected,Premultiply,uv); out_color=vec4(s.rgb*mix(1.0,alpha,pre),alpha); }
