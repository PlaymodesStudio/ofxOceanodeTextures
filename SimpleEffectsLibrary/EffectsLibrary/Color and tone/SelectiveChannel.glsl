//Channel:3:0:4, Colorize:0:0:1
// @description Extracts red, green, blue, luminance, or alpha as a grayscale image.
// @param Channel: Selects red zero, green one, blue two, luminance three, or alpha four.
// @param Colorize: Crossfades between grayscale and the selected RGB channel color.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform float Channel; uniform float Colorize; uniform sampler2D ColorizeTex; uniform int ColorizeTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); int c=clamp(int(round(Channel)),0,4); float value=c==0?s.r:c==1?s.g:c==2?s.b:c==3?seLuma(s.rgb):s.a; vec3 channelColor=c==0?vec3(value,0,0):c==1?vec3(0,value,0):c==2?vec3(0,0,value):vec3(value); float amount=seUnit(ColorizeTex,ColorizeTexConnected,Colorize,uv); out_color=vec4(mix(vec3(value),channelColor,amount),s.a); }
