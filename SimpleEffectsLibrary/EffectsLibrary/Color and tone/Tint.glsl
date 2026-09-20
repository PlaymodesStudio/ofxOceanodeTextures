//BlackColor:color:0:0:0:1, WhiteColor:color:1:1:1:1, Amount:1:0:1
// @description Maps luminance between selected black and white colors.
// @param BlackColor: Color assigned to black source values.
// @param WhiteColor: Color assigned to white source values.
// @param Amount: Blends between the source and tint mapping.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform vec4 BlackColor; uniform vec4 WhiteColor;
uniform float Amount; uniform sampler2D AmountTex; uniform int AmountTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); vec3 mapped=mix(BlackColor.rgb,WhiteColor.rgb,clamp(seLuma(s.rgb),0.0,1.0)); float a=seUnit(AmountTex,AmountTexConnected,Amount,uv); out_color=vec4(mix(s.rgb,mapped,a),s.a); }
