//Exposure:0:-10:10, Offset:0:-1:1
// @description Adjusts image brightness using photographic exposure stops and a linear offset.
// @param Exposure: Brightness change in stops; one stop doubles or halves the light.
// @param Offset: Linear value added after exposure scaling.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform float Exposure;
uniform sampler2D ExposureTex;
uniform int ExposureTexConnected;
uniform float Offset;
uniform sampler2D OffsetTex;
uniform int OffsetTexConnected;
out vec4 out_color;

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 source = texture(tSource, uv);
    float exposure = seRange(ExposureTex, ExposureTexConnected, Exposure, -10.0, 10.0, uv);
    float offset = seRange(OffsetTex, OffsetTexConnected, Offset, -1.0, 1.0, uv);
    out_color = vec4(source.rgb * exp2(exposure) + offset, source.a);
}
