//Amount:1:0:1, AlphaFloor:0.001:0.000001:1
// @description Divides RGB by alpha to recover straight color while safely handling transparent pixels.
// @param Amount: Blends between unchanged RGB and fully unpremultiplied RGB.
// @param AlphaFloor: Smallest divisor used to prevent extreme values near zero alpha.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution;
uniform float Amount; uniform sampler2D AmountTex; uniform int AmountTexConnected;
uniform float AlphaFloor; uniform sampler2D AlphaFloorTex; uniform int AlphaFloorTexConnected;
out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); float floorValue=seRange(AlphaFloorTex,AlphaFloorTexConnected,AlphaFloor,0.000001,1.0,uv); vec3 straight=s.a>floorValue?s.rgb/s.a:vec3(0.0); float amount=seUnit(AmountTex,AmountTexConnected,Amount,uv); out_color=vec4(mix(s.rgb,straight,amount),s.a); }
