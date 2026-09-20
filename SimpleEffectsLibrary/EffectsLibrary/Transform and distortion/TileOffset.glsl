//TilesX:1:1:32, TilesY:1:1:32, OffsetX:0:-1:1, OffsetY:0:-1:1, MirrorTiles:0:0:1, Mix:1:0:1
// @description Repeats, offsets, and optionally alternates mirrored image tiles.
// @param TilesX: Number of horizontal repetitions.
// @param TilesY: Number of vertical repetitions.
// @param OffsetX: Horizontal tile-space offset.
// @param OffsetY: Vertical tile-space offset.
// @param MirrorTiles: Mirrors alternating rows and columns to hide hard repetition seams.
// @param Mix: Blends between source and tiled result.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution;
uniform float TilesX; uniform sampler2D TilesXTex; uniform int TilesXTexConnected;
uniform float TilesY; uniform sampler2D TilesYTex; uniform int TilesYTexConnected;
uniform float OffsetX; uniform sampler2D OffsetXTex; uniform int OffsetXTexConnected;
uniform float OffsetY; uniform sampler2D OffsetYTex; uniform int OffsetYTexConnected;
uniform float MirrorTiles; uniform sampler2D MirrorTilesTex; uniform int MirrorTilesTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec2 tiles=vec2(seRange(TilesXTex,TilesXTexConnected,TilesX,1.0,32.0,uv),seRange(TilesYTex,TilesYTexConnected,TilesY,1.0,32.0,uv)); vec2 p=uv*tiles+vec2(seRange(OffsetXTex,OffsetXTexConnected,OffsetX,-1.0,1.0,uv),seRange(OffsetYTex,OffsetYTexConnected,OffsetY,-1.0,1.0,uv)); vec2 cell=floor(p); vec2 q=fract(p); if(seUnit(MirrorTilesTex,MirrorTilesTexConnected,MirrorTiles,uv)>=0.5){ if(mod(cell.x,2.0)!=0.0) q.x=1.0-q.x; if(mod(cell.y,2.0)!=0.0) q.y=1.0-q.y; } out_color=mix(texture(tSource,uv),texture(tSource,q),seUnit(MixTex,MixTexConnected,Mix,uv)); }
