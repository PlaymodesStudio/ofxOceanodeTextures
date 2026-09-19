//InputBlack:0:0:1, InputWhite:1:0:1, Gamma:1:0.01:8, OutputBlack:0:0:1, OutputWhite:1:0:1
// @description Remaps the input tonal range with gamma correction and a configurable output range.
// @param InputBlack: Input value mapped to the output black point.
// @param InputWhite: Input value mapped to the output white point.
// @param Gamma: Shapes the midtones without moving the input endpoints.
// @param OutputBlack: Darkest value produced by the effect.
// @param OutputWhite: Brightest value produced by the effect.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform float InputBlack; uniform sampler2D InputBlackTex; uniform int InputBlackTexConnected;
uniform float InputWhite; uniform sampler2D InputWhiteTex; uniform int InputWhiteTexConnected;
uniform float Gamma; uniform sampler2D GammaTex; uniform int GammaTexConnected;
uniform float OutputBlack; uniform sampler2D OutputBlackTex; uniform int OutputBlackTexConnected;
uniform float OutputWhite; uniform sampler2D OutputWhiteTex; uniform int OutputWhiteTexConnected;
out vec4 out_color;

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 source = texture(tSource, uv);
    float inputBlack = seUnit(InputBlackTex, InputBlackTexConnected, InputBlack, uv);
    float inputWhite = seUnit(InputWhiteTex, InputWhiteTexConnected, InputWhite, uv);
    float gamma = seRange(GammaTex, GammaTexConnected, Gamma, 0.01, 8.0, uv);
    float outputBlack = seUnit(OutputBlackTex, OutputBlackTexConnected, OutputBlack, uv);
    float outputWhite = seUnit(OutputWhiteTex, OutputWhiteTexConnected, OutputWhite, uv);
    float inputRange = max(inputWhite - inputBlack, SE_EPSILON);
    vec3 normalized = clamp((source.rgb - inputBlack) / inputRange, 0.0, 1.0);
    vec3 corrected = pow(normalized, vec3(1.0 / max(gamma, 0.01)));
    out_color = vec4(mix(vec3(outputBlack), vec3(outputWhite), corrected), source.a);
}
