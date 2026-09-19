//Length:8:0:128, Angle:0:-180:180, Samples:9:1:17, Mix:1:0:1
// @description Blurs the image along a chosen direction using up to seventeen samples.
// @param Length: Total blur length measured in pixels.
// @param Angle: Blur direction in degrees.
// @param Samples: Number of samples used along the blur line.
// @param Mix: Blends between the source and blurred result.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform vec2 uTexelSize;
uniform float Length; uniform sampler2D LengthTex; uniform int LengthTexConnected;
uniform float Angle; uniform sampler2D AngleTex; uniform int AngleTexConnected;
uniform float Samples;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected;
out vec4 out_color;

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    float lengthPixels = seRange(LengthTex, LengthTexConnected, Length, 0.0, 128.0, uv);
    float angle = radians(seRange(AngleTex, AngleTexConnected, Angle, -180.0, 180.0, uv));
    int sampleCount = clamp(int(round(Samples)), 1, 17);
    vec2 direction = vec2(cos(angle), sin(angle)) * uTexelSize * lengthPixels;
    vec4 accumulated = vec4(0.0);
    float count = 0.0;

    for(int i = 0; i < 17; i++){
        if(i >= sampleCount){
            break;
        }
        float position = sampleCount == 1 ? 0.0 : float(i) / float(sampleCount - 1) - 0.5;
        accumulated += texture(tSource, clamp(uv + direction * position, vec2(0.0), vec2(1.0)));
        count += 1.0;
    }

    vec4 source = texture(tSource, uv);
    vec4 blurred = accumulated / max(count, 1.0);
    float amount = seUnit(MixTex, MixTexConnected, Mix, uv);
    out_color = mix(source, blurred, amount);
}
