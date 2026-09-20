//LayerB:texture, Mask:texture, Amount:1:0:1, UseMaskAlpha:0:0:1, Invert:0:0:1
// @description Composites a second texture through the luminance or alpha of a mask texture.
// @param LayerB: Image revealed by the mask.
// @param Mask: Texture defining where LayerB appears.
// @param Amount: Scales the mask strength.
// @param UseMaskAlpha: Crossfades mask interpretation from luminance to alpha.
// @param Invert: Inverts the mask.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform sampler2D LayerB; uniform int LayerBConnected; uniform sampler2D Mask; uniform int MaskConnected; uniform vec2 uResolution;
uniform float Amount; uniform sampler2D AmountTex; uniform int AmountTexConnected;
uniform float UseMaskAlpha; uniform sampler2D UseMaskAlphaTex; uniform int UseMaskAlphaTexConnected;
uniform float Invert; uniform sampler2D InvertTex; uniform int InvertTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 a=texture(tSource,uv); if(LayerBConnected==0||MaskConnected==0){out_color=a;return;} vec4 b=texture(LayerB,uv), m=texture(Mask,uv); float matte=mix(seLuma(m.rgb),m.a,seUnit(UseMaskAlphaTex,UseMaskAlphaTexConnected,UseMaskAlpha,uv)); matte=mix(matte,1.0-matte,seUnit(InvertTex,InvertTexConnected,Invert,uv)); matte=clamp(matte*seUnit(AmountTex,AmountTexConnected,Amount,uv),0.0,1.0)*b.a; out_color=mix(a,b,matte); }
