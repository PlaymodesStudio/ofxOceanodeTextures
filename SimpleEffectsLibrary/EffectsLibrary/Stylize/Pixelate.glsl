//CellWidth:16:1:512, CellHeight:16:1:512, Mix:1:0:1
// @description Divides the image into rectangular cells and samples one color per cell.
// @param CellWidth: Cell width measured in source pixels.
// @param CellHeight: Cell height measured in source pixels.
// @param Mix: Blends between the source and pixelated result.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform float CellWidth; uniform sampler2D CellWidthTex; uniform int CellWidthTexConnected;
uniform float CellHeight; uniform sampler2D CellHeightTex; uniform int CellHeightTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected;
out vec4 out_color;

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec2 cellSize = vec2(seRange(CellWidthTex, CellWidthTexConnected, CellWidth, 1.0, 512.0, uv),
                         seRange(CellHeightTex, CellHeightTexConnected, CellHeight, 1.0, 512.0, uv));
    vec2 pixelCenter = (floor(gl_FragCoord.xy / cellSize) * cellSize + cellSize * 0.5) / uResolution;
    vec4 source = texture(tSource, uv);
    vec4 pixelated = texture(tSource, clamp(pixelCenter, vec2(0.0), vec2(1.0)));
    float amount = seUnit(MixTex, MixTexConnected, Mix, uv);
    out_color = mix(source, pixelated, amount);
}
