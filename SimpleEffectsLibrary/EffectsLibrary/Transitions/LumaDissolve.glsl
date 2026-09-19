//LayerB:texture, Map:texture, Progress:0:0:1, Softness:0.05:0:1, InvertMap:0:0:1, BurnWidth:0.04:0:0.25, BurnColor:color:1:0.25:0.02:1, BurnIntensity:1:0:8
// @description Reveals a second texture according to a luminance map with an optional colored burn edge.
// @param LayerB: Texture revealed by the dissolve.
// @param Map: Optional luminance map; LayerB luminance is used when disconnected.
// @param Progress: Transition completion from source to LayerB.
// @param Softness: Tonal softness of the dissolve boundary.
// @param InvertMap: Crossfades between normal and inverted map order.
// @param BurnWidth: Width of the colored boundary around the dissolve front.
// @param BurnColor: Color added around the dissolve front.
// @param BurnIntensity: Brightness of the burn color.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform sampler2D LayerB;
uniform int LayerBConnected;
uniform sampler2D Map;
uniform int MapConnected;
uniform float Progress; uniform sampler2D ProgressTex; uniform int ProgressTexConnected;
uniform float Softness; uniform sampler2D SoftnessTex; uniform int SoftnessTexConnected;
uniform float InvertMap; uniform sampler2D InvertMapTex; uniform int InvertMapTexConnected;
uniform float BurnWidth; uniform sampler2D BurnWidthTex; uniform int BurnWidthTexConnected;
uniform vec4 BurnColor;
uniform float BurnIntensity; uniform sampler2D BurnIntensityTex; uniform int BurnIntensityTexConnected;
out vec4 out_color;

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 source = texture(tSource, uv);
    if(LayerBConnected == 0){
        out_color = source;
        return;
    }
    vec4 layer = texture(LayerB, uv);
    float progress = seUnit(ProgressTex, ProgressTexConnected, Progress, uv);
    if(progress <= 0.0){
        out_color = source;
        return;
    }
    if(progress >= 1.0){
        out_color = layer;
        return;
    }

    float mapValue = MapConnected != 0
                   ? clamp(seLuma(texture(Map, uv).rgb), 0.0, 1.0)
                   : clamp(seLuma(layer.rgb), 0.0, 1.0);
    float invertMap = seUnit(InvertMapTex, InvertMapTexConnected, InvertMap, uv);
    mapValue = mix(mapValue, 1.0 - mapValue, invertMap);
    float softness = seUnit(SoftnessTex, SoftnessTexConnected, Softness, uv);
    float matte = softness <= SE_EPSILON
                ? step(mapValue, progress)
                : 1.0 - smoothstep(progress - softness, progress + softness, mapValue);

    vec4 result = mix(source, layer, matte);
    float burnWidth = seRange(BurnWidthTex, BurnWidthTexConnected, BurnWidth, 0.0, 0.25, uv);
    float burnIntensity = seRange(BurnIntensityTex, BurnIntensityTexConnected, BurnIntensity, 0.0, 8.0, uv);
    float burn = burnWidth <= SE_EPSILON
               ? 0.0
               : 1.0 - smoothstep(0.0, burnWidth, abs(mapValue - progress));
    result.rgb += BurnColor.rgb * burn * burnIntensity;
    out_color = result;
}
