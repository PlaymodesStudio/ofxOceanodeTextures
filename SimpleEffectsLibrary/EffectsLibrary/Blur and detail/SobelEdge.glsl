//Strength:1:0:10, Threshold:0.1:0:1, Softness:0.1:0:1, Invert:0:0:1, Mix:1:0:1
// @description Extracts luminance edges using a three-by-three Sobel convolution.
// @param Strength: Multiplies the detected edge magnitude.
// @param Threshold: Minimum edge magnitude retained in the matte.
// @param Softness: Smoothness around the edge threshold.
// @param Invert: Crossfades between normal and inverted edge mattes.
// @param Mix: Blends between the source and monochrome edge result.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform vec2 uTexelSize;
uniform float Strength; uniform sampler2D StrengthTex; uniform int StrengthTexConnected;
uniform float Threshold; uniform sampler2D ThresholdTex; uniform int ThresholdTexConnected;
uniform float Softness; uniform sampler2D SoftnessTex; uniform int SoftnessTexConnected;
uniform float Invert; uniform sampler2D InvertTex; uniform int InvertTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected;
out vec4 out_color;

float sampleLuma(vec2 uv)
{
    return seLuma(texture(tSource, clamp(uv, vec2(0.0), vec2(1.0))).rgb);
}

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec2 p = uTexelSize;
    float tl = sampleLuma(uv + vec2(-p.x,  p.y));
    float tc = sampleLuma(uv + vec2( 0.0,  p.y));
    float tr = sampleLuma(uv + vec2( p.x,  p.y));
    float ml = sampleLuma(uv + vec2(-p.x,  0.0));
    float mr = sampleLuma(uv + vec2( p.x,  0.0));
    float bl = sampleLuma(uv + vec2(-p.x, -p.y));
    float bc = sampleLuma(uv + vec2( 0.0, -p.y));
    float br = sampleLuma(uv + vec2( p.x, -p.y));
    float gx = -tl - 2.0 * ml - bl + tr + 2.0 * mr + br;
    float gy =  tl + 2.0 * tc + tr - bl - 2.0 * bc - br;
    float strength = seRange(StrengthTex, StrengthTexConnected, Strength, 0.0, 10.0, uv);
    float threshold = seUnit(ThresholdTex, ThresholdTexConnected, Threshold, uv);
    float softness = seUnit(SoftnessTex, SoftnessTexConnected, Softness, uv);
    float edge = length(vec2(gx, gy)) * strength;
    float matte = softness <= SE_EPSILON
                ? step(threshold, edge)
                : smoothstep(threshold, threshold + softness, edge);
    float invert = seUnit(InvertTex, InvertTexConnected, Invert, uv);
    matte = mix(matte, 1.0 - matte, invert);
    vec4 source = texture(tSource, uv);
    float amount = seUnit(MixTex, MixTexConnected, Mix, uv);
    out_color = vec4(mix(source.rgb, vec3(matte), amount), source.a);
}
