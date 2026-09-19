//Reference:texture, Threshold:0.08:0:1, Softness:0.08:0:1, Gain:1:0:10, Preview:0:0:1
// @description Creates a matte from the RGB difference between the source and a reference texture.
// @param Reference: Clean plate or comparison image.
// @param Threshold: Difference below which pixels are removed.
// @param Softness: Width of the transition around the difference threshold.
// @param Gain: Multiplies small differences before thresholding.
// @param Preview: Crossfades from the keyed image to its grayscale matte.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform sampler2D Reference;
uniform int ReferenceConnected;
uniform float Threshold; uniform sampler2D ThresholdTex; uniform int ThresholdTexConnected;
uniform float Softness; uniform sampler2D SoftnessTex; uniform int SoftnessTexConnected;
uniform float Gain; uniform sampler2D GainTex; uniform int GainTexConnected;
uniform float Preview; uniform sampler2D PreviewTex; uniform int PreviewTexConnected;
out vec4 out_color;

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 source = texture(tSource, uv);
    if(ReferenceConnected == 0){
        out_color = source;
        return;
    }
    vec3 reference = texture(Reference, uv).rgb;
    float threshold = seUnit(ThresholdTex, ThresholdTexConnected, Threshold, uv);
    float softness = seUnit(SoftnessTex, SoftnessTexConnected, Softness, uv);
    float gain = seRange(GainTex, GainTexConnected, Gain, 0.0, 10.0, uv);
    float difference = length(source.rgb - reference) / sqrt(3.0) * gain;
    float matte = softness <= SE_EPSILON
                ? step(threshold, difference)
                : smoothstep(threshold, threshold + softness, difference);
    float preview = seUnit(PreviewTex, PreviewTexConnected, Preview, uv);
    vec3 keyed = source.rgb * matte;
    out_color = vec4(mix(keyed, vec3(matte), preview), source.a * matte);
}
