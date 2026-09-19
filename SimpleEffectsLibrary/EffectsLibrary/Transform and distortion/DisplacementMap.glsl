//Map:texture, AmountX:0:-500:500, AmountY:0:-500:500, Center:0.5:0:1, ClampEdges:1:0:1, Mix:1:0:1
// @description Offsets source sampling coordinates using red and green channels from a second texture.
// @param Map: Displacement texture; red drives X and green drives Y.
// @param AmountX: Maximum horizontal displacement in pixels.
// @param AmountY: Maximum vertical displacement in pixels.
// @param Center: Map value representing zero displacement.
// @param ClampEdges: Crossfades from transparent boundaries to clamped boundaries.
// @param Mix: Blends between the source and displaced result.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform vec2 uTexelSize;
uniform sampler2D Map;
uniform int MapConnected;
uniform float AmountX; uniform sampler2D AmountXTex; uniform int AmountXTexConnected;
uniform float AmountY; uniform sampler2D AmountYTex; uniform int AmountYTexConnected;
uniform float Center; uniform sampler2D CenterTex; uniform int CenterTexConnected;
uniform float ClampEdges; uniform sampler2D ClampEdgesTex; uniform int ClampEdgesTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected;
out vec4 out_color;

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 source = texture(tSource, uv);
    if(MapConnected == 0){
        out_color = source;
        return;
    }
    vec2 displacement = texture(Map, uv).rg;
    float center = seUnit(CenterTex, CenterTexConnected, Center, uv);
    vec2 amount = vec2(seRange(AmountXTex, AmountXTexConnected, AmountX, -500.0, 500.0, uv),
                       seRange(AmountYTex, AmountYTexConnected, AmountY, -500.0, 500.0, uv));
    vec2 displacedUv = uv + (displacement - center) * amount * uTexelSize;
    float clampEdges = seUnit(ClampEdgesTex, ClampEdgesTexConnected, ClampEdges, uv);
    vec4 displaced = seInside(displacedUv)
                   ? texture(tSource, displacedUv)
                   : vec4(0.0);
    displaced = mix(displaced, texture(tSource, clamp(displacedUv, vec2(0.0), vec2(1.0))), clampEdges);
    float mixAmount = seUnit(MixTex, MixTexConnected, Mix, uv);
    out_color = mix(source, displaced, mixAmount);
}
