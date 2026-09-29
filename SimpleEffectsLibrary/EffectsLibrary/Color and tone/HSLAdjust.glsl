//Hue:0:-1:1, Saturation:1:0:2, Lightness:1:0:2, Mix:1:0:1
// @description Adjusts hue, HSL saturation, and HSL lightness while preserving source alpha.
// @param Hue: Rotates hue; a value of one represents a complete rotation.
// @param Saturation: Scales saturation in HSL colour space.
// @param Lightness: Scales lightness in HSL colour space.
// @param Mix: Blends between the source and adjusted colour.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform float Hue; uniform sampler2D HueTex; uniform int HueTexConnected;
uniform float Saturation; uniform sampler2D SaturationTex; uniform int SaturationTexConnected;
uniform float Lightness; uniform sampler2D LightnessTex; uniform int LightnessTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected;
out vec4 out_color;

vec3 rgbToHsl(vec3 color)
{
    float maximum = max(max(color.r, color.g), color.b);
    float minimum = min(min(color.r, color.g), color.b);
    float chroma = maximum - minimum;
    float lightness = (maximum + minimum) * 0.5;
    float saturation = chroma <= SE_EPSILON
                     ? 0.0
                     : chroma / max(1.0 - abs(2.0 * lightness - 1.0), SE_EPSILON);
    float hue = 0.0;

    if(chroma > SE_EPSILON){
        if(maximum == color.r){
            hue = mod((color.g - color.b) / chroma, 6.0);
        }else if(maximum == color.g){
            hue = (color.b - color.r) / chroma + 2.0;
        }else{
            hue = (color.r - color.g) / chroma + 4.0;
        }
        hue = fract(hue / 6.0);
    }

    return vec3(hue, saturation, lightness);
}

float hslChannel(float p, float q, float hue)
{
    hue = fract(hue);
    if(hue < 1.0 / 6.0) return p + (q - p) * 6.0 * hue;
    if(hue < 0.5) return q;
    if(hue < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - hue) * 6.0;
    return p;
}

vec3 hslToRgb(vec3 hsl)
{
    if(hsl.y <= SE_EPSILON){
        return vec3(hsl.z);
    }

    float q = hsl.z < 0.5
            ? hsl.z * (1.0 + hsl.y)
            : hsl.z + hsl.y - hsl.z * hsl.y;
    float p = 2.0 * hsl.z - q;
    return vec3(hslChannel(p, q, hsl.x + 1.0 / 3.0),
                hslChannel(p, q, hsl.x),
                hslChannel(p, q, hsl.x - 1.0 / 3.0));
}

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 source = texture(tSource, uv);
    float hue = seRange(HueTex, HueTexConnected, Hue, -1.0, 1.0, uv);
    float saturation = seRange(SaturationTex, SaturationTexConnected, Saturation, 0.0, 2.0, uv);
    float lightness = seRange(LightnessTex, LightnessTexConnected, Lightness, 0.0, 2.0, uv);
    float amount = seUnit(MixTex, MixTexConnected, Mix, uv);

    // Preserve exact HDR and signed values when the colour controls are neutral.
    if(abs(hue) <= SE_EPSILON &&
       abs(saturation - 1.0) <= SE_EPSILON &&
       abs(lightness - 1.0) <= SE_EPSILON){
        out_color = source;
        return;
    }

    vec3 hsl = rgbToHsl(clamp(source.rgb, 0.0, 1.0));
    hsl.x = fract(hsl.x + hue);
    hsl.y = clamp(hsl.y * saturation, 0.0, 1.0);
    hsl.z = clamp(hsl.z * lightness, 0.0, 1.0);
    vec3 adjusted = hslToRgb(hsl);
    out_color = vec4(mix(source.rgb, adjusted, amount), source.a);
}
