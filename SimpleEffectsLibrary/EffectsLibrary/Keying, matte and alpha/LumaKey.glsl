//Threshold:0.15:0:1, Softness:0.25:0:1, Curve:1:0.1:8, Feather:0:0:16, Preview:0:0:1
// @description Produces a controlled soft luminance key that turns low-luminance values black.
// @param Threshold: Luminance at or below which the result is fully black.
// @param Softness: Tonal width of the transition back to the original image.
// @param Curve: Shapes the transition; higher values hold dark areas longer.
// @param Feather: Spatial smoothing radius for reducing noisy matte edges.
// @param Preview: Crossfades from the keyed image to its grayscale matte.
#version 410

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform vec2 uTexelSize;

uniform float Threshold;
uniform sampler2D ThresholdTex;
uniform int ThresholdTexConnected;

uniform float Softness;
uniform sampler2D SoftnessTex;
uniform int SoftnessTexConnected;

uniform float Curve;
uniform sampler2D CurveTex;
uniform int CurveTexConnected;

uniform float Feather;
uniform sampler2D FeatherTex;
uniform int FeatherTexConnected;

uniform float Preview;
uniform sampler2D PreviewTex;
uniform int PreviewTexConnected;

out vec4 out_color;

const vec3 LUMA_WEIGHTS = vec3(0.2126, 0.7152, 0.0722);
const float EPSILON = 0.00001;

float readUnitControl(sampler2D controlTexture,
                      int textureConnected,
                      float fallback,
                      vec2 uv)
{
    return textureConnected != 0
         ? clamp(texture(controlTexture, uv).r, 0.0, 1.0)
         : fallback;
}

float readRangedControl(sampler2D controlTexture,
                        int textureConnected,
                        float fallback,
                        float minimum,
                        float maximum,
                        vec2 uv)
{
    return textureConnected != 0
         ? mix(minimum, maximum, clamp(texture(controlTexture, uv).r, 0.0, 1.0))
         : fallback;
}

float luminance(vec3 color)
{
    return dot(max(color, vec3(0.0)), LUMA_WEIGHTS);
}

float sampleLuminance(vec2 uv)
{
    return luminance(texture(tSource, clamp(uv, vec2(0.0), vec2(1.0))).rgb);
}

float featheredLuminance(vec2 uv, float centerLuminance, float radius)
{
    if(radius <= EPSILON){
        return centerLuminance;
    }

    vec2 offset = uTexelSize * radius;

    // Compact 3x3 Gaussian kernel. The already sampled center has weight 4.
    float total = centerLuminance * 4.0;
    total += sampleLuminance(uv + vec2( offset.x, 0.0)) * 2.0;
    total += sampleLuminance(uv + vec2(-offset.x, 0.0)) * 2.0;
    total += sampleLuminance(uv + vec2(0.0,  offset.y)) * 2.0;
    total += sampleLuminance(uv + vec2(0.0, -offset.y)) * 2.0;
    total += sampleLuminance(uv + vec2( offset.x,  offset.y));
    total += sampleLuminance(uv + vec2(-offset.x,  offset.y));
    total += sampleLuminance(uv + vec2( offset.x, -offset.y));
    total += sampleLuminance(uv + vec2(-offset.x, -offset.y));

    return total / 16.0;
}

float lumaMatte(float value, float threshold, float softness, float curve)
{
    // Avoid smoothstep with equal edges when Softness is zero.
    if(softness <= EPSILON){
        return step(threshold, value);
    }

    float transitionEnd = min(threshold + softness, 1.0);
    float transitionWidth = max(transitionEnd - threshold, EPSILON);
    float normalized = clamp((value - threshold) / transitionWidth, 0.0, 1.0);

    // Hermite smoothing followed by an artist-controlled curve.
    float smoothMatte = normalized * normalized * (3.0 - 2.0 * normalized);
    return pow(smoothMatte, max(curve, 0.1));
}

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 source = texture(tSource, uv);

    float threshold = readUnitControl(ThresholdTex, ThresholdTexConnected, Threshold, uv);
    float softness = readUnitControl(SoftnessTex, SoftnessTexConnected, Softness, uv);
    float curve = readRangedControl(CurveTex, CurveTexConnected, Curve, 0.1, 8.0, uv);
    float feather = readRangedControl(FeatherTex, FeatherTexConnected, Feather, 0.0, 16.0, uv);
    float preview = readUnitControl(PreviewTex, PreviewTexConnected, Preview, uv);

    float sourceLuminance = luminance(source.rgb);
    float filteredLuminance = featheredLuminance(uv, sourceLuminance, feather);
    float matte = lumaMatte(filteredLuminance, threshold, softness, curve);

    vec3 keyedColor = source.rgb * matte;
    vec3 previewColor = vec3(matte);

    out_color = vec4(mix(keyedColor, previewColor, preview), source.a);
}
