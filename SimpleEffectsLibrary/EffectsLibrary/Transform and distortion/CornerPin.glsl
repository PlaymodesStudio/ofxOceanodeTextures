//TLX:0:0:1, TLY:1:0:1, TRX:1:0:1, TRY:1:0:1, BLX:0:0:1, BLY:0:0:1, BRX:1:0:1, BRY:0:0:1, Mix:1:0:1
// @description Bilinearly maps the output rectangle through four configurable source corners.
// @param TLX: Top-left source corner horizontal coordinate.
// @param TLY: Top-left source corner vertical coordinate.
// @param TRX: Top-right source corner horizontal coordinate.
// @param TRY: Top-right source corner vertical coordinate.
// @param BLX: Bottom-left source corner horizontal coordinate.
// @param BLY: Bottom-left source corner vertical coordinate.
// @param BRX: Bottom-right source corner horizontal coordinate.
// @param BRY: Bottom-right source corner vertical coordinate.
// @param Mix: Blends between source and corner-mapped result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution;
uniform float TLX; uniform sampler2D TLXTex; uniform int TLXTexConnected; uniform float TLY; uniform sampler2D TLYTex; uniform int TLYTexConnected;
uniform float TRX; uniform sampler2D TRXTex; uniform int TRXTexConnected; uniform float TRY; uniform sampler2D TRYTex; uniform int TRYTexConnected;
uniform float BLX; uniform sampler2D BLXTex; uniform int BLXTexConnected; uniform float BLY; uniform sampler2D BLYTex; uniform int BLYTexConnected;
uniform float BRX; uniform sampler2D BRXTex; uniform int BRXTexConnected; uniform float BRY; uniform sampler2D BRYTex; uniform int BRYTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec2 tl=vec2(seUnit(TLXTex,TLXTexConnected,TLX,uv),seUnit(TLYTex,TLYTexConnected,TLY,uv)), tr=vec2(seUnit(TRXTex,TRXTexConnected,TRX,uv),seUnit(TRYTex,TRYTexConnected,TRY,uv)); vec2 bl=vec2(seUnit(BLXTex,BLXTexConnected,BLX,uv),seUnit(BLYTex,BLYTexConnected,BLY,uv)), br=vec2(seUnit(BRXTex,BRXTexConnected,BRX,uv),seUnit(BRYTex,BRYTexConnected,BRY,uv)); vec2 q=mix(mix(bl,br,uv.x),mix(tl,tr,uv.x),uv.y); vec4 result=seInside(q)?texture(tSource,q):vec4(0.0); out_color=mix(texture(tSource,uv),result,seUnit(MixTex,MixTexConnected,Mix,uv)); }
