//Lift:0:-1:1, Gamma:1:0.01:8, Gain:1:0:8, Saturation:1:0:2
// @description Provides compact color grading controls for shadows, midtones, highlights, and saturation.
// @param Lift: Adds or removes density primarily from darker values.
// @param Gamma: Shapes the midtones.
// @param Gain: Multiplies the graded signal.
// @param Saturation: Scales color intensity around luminance.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform float Lift; uniform sampler2D LiftTex; uniform int LiftTexConnected;
uniform float Gamma; uniform sampler2D GammaTex; uniform int GammaTexConnected;
uniform float Gain; uniform sampler2D GainTex; uniform int GainTexConnected;
uniform float Saturation; uniform sampler2D SaturationTex; uniform int SaturationTexConnected;
out vec4 out_color;

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 source = texture(tSource, uv);
    float lift = seRange(LiftTex, LiftTexConnected, Lift, -1.0, 1.0, uv);
    float gamma = seRange(GammaTex, GammaTexConnected, Gamma, 0.01, 8.0, uv);
    float gain = seRange(GainTex, GainTexConnected, Gain, 0.0, 8.0, uv);
    float saturation = seRange(SaturationTex, SaturationTexConnected, Saturation, 0.0, 2.0, uv);
    vec3 graded = pow(max((source.rgb + lift) * gain, vec3(0.0)), vec3(1.0 / max(gamma, 0.01)));
    graded = mix(vec3(seLuma(graded)), graded, saturation);
    out_color = vec4(graded, source.a);
}
