//ColorSource:texture, Amount:1:0:1
// @description Replaces source RGB from another texture while preserving source alpha.
// @param ColorSource: Texture supplying replacement RGB.
// @param Amount: Blends between original and replacement RGB.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform sampler2D ColorSource; uniform int ColorSourceConnected; uniform vec2 uResolution;
uniform float Amount; uniform sampler2D AmountTex; uniform int AmountTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); if(ColorSourceConnected==0){out_color=s;return;} vec3 color=texture(ColorSource,uv).rgb; out_color=vec4(mix(s.rgb,color,seUnit(AmountTex,AmountTexConnected,Amount,uv)),s.a); }
