//Amount:1:0:1
// @description Multiplies RGB by source alpha for premultiplied-alpha compositing.
// @param Amount: Blends between unchanged RGB and fully premultiplied RGB.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution;
uniform float Amount; uniform sampler2D AmountTex; uniform int AmountTexConnected;
out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); float amount=seUnit(AmountTex,AmountTexConnected,Amount,uv); out_color=vec4(s.rgb*mix(1.0,s.a,amount),s.a); }
