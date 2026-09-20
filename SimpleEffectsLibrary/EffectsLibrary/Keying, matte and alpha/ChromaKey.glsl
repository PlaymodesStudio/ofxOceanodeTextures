//KeyColor:color:0:1:0:1, Tolerance:0.12:0:1, Softness:0.12:0:1, Despill:0.5:0:1, Preview:0:0:1
// @description Removes pixels close to a selected chroma color and optionally suppresses color spill.
// @param KeyColor: Color to remove from the source.
// @param Tolerance: Chroma distance removed completely.
// @param Softness: Width of the transition from keyed to retained pixels.
// @param Despill: Reduces the key color near transparent areas.
// @param Preview: Crossfades from the keyed image to its grayscale matte.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform vec4 KeyColor;
uniform float Tolerance; uniform sampler2D ToleranceTex; uniform int ToleranceTexConnected;
uniform float Softness; uniform sampler2D SoftnessTex; uniform int SoftnessTexConnected;
uniform float Despill; uniform sampler2D DespillTex; uniform int DespillTexConnected;
uniform float Preview; uniform sampler2D PreviewTex; uniform int PreviewTexConnected;
out vec4 out_color;

vec2 chroma(vec3 color)
{
    float sum = max(color.r + color.g + color.b, SE_EPSILON);
    return color.rg / sum;
}

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 source = texture(tSource, uv);
    float tolerance = seUnit(ToleranceTex, ToleranceTexConnected, Tolerance, uv);
    float softness = seUnit(SoftnessTex, SoftnessTexConnected, Softness, uv);
    float distanceFromKey = length(chroma(max(source.rgb, vec3(0.0))) - chroma(max(KeyColor.rgb, vec3(0.0))));
    float matte = softness <= SE_EPSILON
                ? step(tolerance, distanceFromKey)
                : smoothstep(tolerance, tolerance + softness, distanceFromKey);

    float despill = seUnit(DespillTex, DespillTexConnected, Despill, uv);
    vec3 keyDirection = normalize(max(KeyColor.rgb, vec3(SE_EPSILON)));
    float spill = max(dot(source.rgb, keyDirection) - seLuma(source.rgb), 0.0);
    vec3 despilled = max(source.rgb - keyDirection * spill * despill * (1.0 - matte), vec3(0.0));
    vec3 keyed = despilled * matte;
    float preview = seUnit(PreviewTex, PreviewTexConnected, Preview, uv);
    out_color = vec4(mix(keyed, vec3(matte), preview), source.a * matte);
}
