//Map:texture, Amount:1:0:1, Centered:0:0:1, ClampEdges:0:0:1
// @description Remaps source coordinates from the red and green channels of a second texture.
// @param Map: Texture whose red and green channels define source coordinates.
// @param Amount: Blends between normal coordinates and remapped coordinates.
// @param Centered: Interprets Map around one-half as signed displacement instead of absolute UV.
// @param ClampEdges: Clamps remapped coordinates instead of returning transparent outside the image.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform sampler2D Map; uniform int MapConnected; uniform vec2 uResolution;
uniform float Amount; uniform sampler2D AmountTex; uniform int AmountTexConnected;
uniform float Centered; uniform sampler2D CenteredTex; uniform int CenteredTexConnected;
uniform float ClampEdges; uniform sampler2D ClampEdgesTex; uniform int ClampEdgesTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); if(MapConnected==0){out_color=s;return;} vec2 mapUv=texture(Map,uv).rg; float centered=seUnit(CenteredTex,CenteredTexConnected,Centered,uv); vec2 target=mix(mapUv,uv+(mapUv-0.5),centered); target=mix(uv,target,seUnit(AmountTex,AmountTexConnected,Amount,uv)); float clampEdges=seUnit(ClampEdgesTex,ClampEdgesTexConnected,ClampEdges,uv); if(clampEdges<0.5&&!seInside(target)){out_color=vec4(0.0);return;} out_color=texture(tSource,clamp(target,0.0,1.0)); }
