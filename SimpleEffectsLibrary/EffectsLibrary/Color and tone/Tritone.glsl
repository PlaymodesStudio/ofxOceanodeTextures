//Shadow:color:0.03:0.05:0.12:1, Midtone:color:0.75:0.28:0.12:1, Highlight:color:1:0.88:0.62:1, Balance:0.5:0:1, Mix:1:0:1
// @description Maps source luminance through independently chosen shadow, midtone, and highlight colors.
// @param Shadow: Color assigned to the darkest values.
// @param Midtone: Color assigned around the tonal balance point.
// @param Highlight: Color assigned to the brightest values.
// @param Balance: Position of the midtone color within the luminance range.
// @param Mix: Blends between the source and the tritone result.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform vec4 Shadow;
uniform vec4 Midtone;
uniform vec4 Highlight;
uniform float Balance; uniform sampler2D BalanceTex; uniform int BalanceTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected;
out vec4 out_color;

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 source = texture(tSource, uv);
    float balance = seUnit(BalanceTex, BalanceTexConnected, Balance, uv);
    float amount = seUnit(MixTex, MixTexConnected, Mix, uv);
    float luma = clamp(seLuma(source.rgb), 0.0, 1.0);
    float lowPosition = clamp(luma / max(balance, SE_EPSILON), 0.0, 1.0);
    float highPosition = clamp((luma - balance) / max(1.0 - balance, SE_EPSILON), 0.0, 1.0);
    lowPosition = lowPosition * lowPosition * (3.0 - 2.0 * lowPosition);
    highPosition = highPosition * highPosition * (3.0 - 2.0 * highPosition);
    vec3 lowToMid = mix(Shadow.rgb, Midtone.rgb, lowPosition);
    vec3 midToHigh = mix(Midtone.rgb, Highlight.rgb, highPosition);
    vec3 mapped = luma <= balance ? lowToMid : midToHigh;
    out_color = vec4(mix(source.rgb, mapped, amount), source.a);
}
