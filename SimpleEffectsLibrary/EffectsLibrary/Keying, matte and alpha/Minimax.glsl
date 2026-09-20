//Operation:0:0:3, Radius:1:0:128, Direction:0:0:2, Samples:12:1:24, BlendWithOriginal:0:0:1
// @description Applies After Effects-style minimum and maximum channel morphology to expand, contract, open, or close image regions and mattes.
// @param Operation: Selects Minimum zero, Maximum one, Minimum Then Maximum two, or Maximum Then Minimum three.
// @param Radius: Neighborhood radius measured in source pixels.
// @param Direction: Selects Horizontal and Vertical zero, Horizontal one, or Vertical two.
// @param Samples: Sampling quality from one through twenty-four; combined operations use Samples squared texture reads.
// @param BlendWithOriginal: Blends from the full effect at zero back to the original image at one.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;

uniform float Operation;

uniform float Radius;
uniform sampler2D RadiusTex;
uniform int RadiusTexConnected;

uniform float Direction;
uniform float Samples;

uniform float BlendWithOriginal;
uniform sampler2D BlendWithOriginalTex;
uniform int BlendWithOriginalTexConnected;

out vec4 out_color;

vec2 minimaxOffset(int index, int count, float radius, int direction)
{
    if(count <= 1 || index == 0 || radius <= SE_EPSILON){
        return vec2(0.0);
    }

    float position = float(index - 1) / float(max(count - 2, 1));
    if(direction == 1){
        return vec2(mix(-radius, radius, position), 0.0);
    }
    if(direction == 2){
        return vec2(0.0, mix(-radius, radius, position));
    }

    // A deterministic sunflower distribution provides an even, GPU-friendly
    // approximation of the full two-dimensional AE neighborhood scan.
    float normalizedIndex = float(index) / float(max(count - 1, 1));
    float angle = float(index) * 2.39996323;
    float distanceFromCenter = radius * sqrt(normalizedIndex);
    return vec2(cos(angle), sin(angle)) * distanceFromCenter;
}

vec4 morphAt(vec2 uv, float radius, int direction, int count, bool findMinimum)
{
    vec4 result = texture(tSource, clamp(uv, vec2(0.0), vec2(1.0)));

    for(int i = 1; i < 24; i++){
        if(i >= count){
            break;
        }

        vec2 sampleUv = uv + minimaxOffset(i, count, radius, direction) / uResolution;
        vec4 candidate = texture(tSource, clamp(sampleUv, vec2(0.0), vec2(1.0)));
        result = findMinimum ? min(result, candidate) : max(result, candidate);
    }

    return result;
}

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 source = texture(tSource, uv);

    int operation = clamp(int(round(Operation)), 0, 3);
    int direction = clamp(int(round(Direction)), 0, 2);
    int count = clamp(int(round(Samples)), 1, 24);
    float radius = seRange(RadiusTex, RadiusTexConnected, Radius, 0.0, 128.0, uv);

    vec4 result;
    if(operation == 0 || operation == 1){
        result = morphAt(uv, radius, direction, count, operation == 0);
    }else{
        bool innerMinimum = operation == 2;
        result = morphAt(uv, radius, direction, count, innerMinimum);

        for(int i = 1; i < 24; i++){
            if(i >= count){
                break;
            }

            vec2 outerUv = uv + minimaxOffset(i, count, radius, direction) / uResolution;
            vec4 innerResult = morphAt(outerUv, radius, direction, count, innerMinimum);
            result = innerMinimum ? max(result, innerResult) : min(result, innerResult);
        }
    }

    float originalAmount = seUnit(BlendWithOriginalTex,
                                  BlendWithOriginalTexConnected,
                                  BlendWithOriginal,
                                  uv);
    out_color = mix(result, source, originalAmount);
}
