//Multiplier:1:0:10, Offset:0:-1:1, Mix:1:0:1
// @description Wraps RGB values cyclically instead of clipping them, producing repeating colour bands and overflow effects.
// @param Multiplier: Scales RGB before wrapping each channel into the zero-to-one interval.
// @param Offset: Shifts all channels around the colour cycle after multiplication.
// @param Mix: Blends between the source and wrapped colour.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform float Multiplier; uniform sampler2D MultiplierTex; uniform int MultiplierTexConnected;
uniform float Offset; uniform sampler2D OffsetTex; uniform int OffsetTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected;
out vec4 out_color;

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 source = texture(tSource, uv);
    float multiplier = seRange(MultiplierTex, MultiplierTexConnected, Multiplier, 0.0, 10.0, uv);
    float offset = seRange(OffsetTex, OffsetTexConnected, Offset, -1.0, 1.0, uv);
    vec3 wrapped = fract(source.rgb * multiplier + offset);
    out_color = vec4(mix(source.rgb, wrapped, seUnit(MixTex, MixTexConnected, Mix, uv)), source.a);
}
