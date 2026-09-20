//Hue:0:-1:1, Saturation:1:0:2, Value:1:0:2
// @description Adjusts hue, HSV saturation, and HSV value while preserving source alpha.
// @param Hue: Rotates hue; a value of one represents a complete rotation.
// @param Saturation: Scales HSV saturation.
// @param Value: Scales HSV brightness.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform float Hue; uniform sampler2D HueTex; uniform int HueTexConnected;
uniform float Saturation; uniform sampler2D SaturationTex; uniform int SaturationTexConnected;
uniform float Value; uniform sampler2D ValueTex; uniform int ValueTexConnected;
out vec4 out_color;

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 source = texture(tSource, uv);
    float hue = seRange(HueTex, HueTexConnected, Hue, -1.0, 1.0, uv);
    float saturation = seRange(SaturationTex, SaturationTexConnected, Saturation, 0.0, 2.0, uv);
    float value = seRange(ValueTex, ValueTexConnected, Value, 0.0, 2.0, uv);
    vec3 hsv = seRgbToHsv(max(source.rgb, vec3(0.0)));
    hsv.x = fract(hsv.x + hue);
    hsv.y = max(hsv.y * saturation, 0.0);
    hsv.z = max(hsv.z * value, 0.0);
    out_color = vec4(seHsvToRgb(hsv), source.a);
}
