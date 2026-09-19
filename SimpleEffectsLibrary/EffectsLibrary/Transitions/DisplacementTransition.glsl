//LayerB:texture, Map:texture, Progress:0:0:1, AmountX:100:-1000:1000, AmountY:0:-1000:1000, Center:0.5:0:1, Softness:0:0:0.5
// @description Crossfades two textures while a map pushes them in opposite directions.
// @param LayerB: Image replacing the source.
// @param Map: Red and green displacement map; luminance also perturbs reveal timing.
// @param Progress: Transition completion from source zero to LayerB one.
// @param AmountX: Horizontal displacement strength in pixels.
// @param AmountY: Vertical displacement strength in pixels.
// @param Center: Map value interpreted as zero displacement.
// @param Softness: Width of the map-modulated transition boundary.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform sampler2D LayerB; uniform int LayerBConnected; uniform sampler2D Map; uniform int MapConnected; uniform vec2 uResolution;
uniform float Progress; uniform sampler2D ProgressTex; uniform int ProgressTexConnected; uniform float AmountX; uniform sampler2D AmountXTex; uniform int AmountXTexConnected;
uniform float AmountY; uniform sampler2D AmountYTex; uniform int AmountYTexConnected; uniform float Center; uniform sampler2D CenterTex; uniform int CenterTexConnected;
uniform float Softness; uniform sampler2D SoftnessTex; uniform int SoftnessTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 original=texture(tSource,uv); if(LayerBConnected==0){out_color=original;return;} vec4 target=texture(LayerB,uv); float p=seUnit(ProgressTex,ProgressTexConnected,Progress,uv); if(p<=0.0){out_color=original;return;} if(p>=1.0){out_color=target;return;} vec4 mapValue=MapConnected!=0?texture(Map,uv):vec4(0.5,0.5,0.5,1.0); float center=seUnit(CenterTex,CenterTexConnected,Center,uv); vec2 vectorValue=MapConnected!=0?mapValue.rg-vec2(center):vec2(0.0); vec2 displacement=vectorValue*vec2(seRange(AmountXTex,AmountXTexConnected,AmountX,-1000.0,1000.0,uv),seRange(AmountYTex,AmountYTexConnected,AmountY,-1000.0,1000.0,uv))/uResolution; vec4 a=texture(tSource,uv+displacement*p), b=texture(LayerB,uv-displacement*(1.0-p)); float soft=seRange(SoftnessTex,SoftnessTexConnected,Softness,0.0,0.5,uv); float threshold=p; if(MapConnected!=0) threshold=soft<=SE_EPSILON?step(mapValue.b,p):smoothstep(mapValue.b-soft,mapValue.b+soft,p); out_color=mix(a,b,threshold); }
