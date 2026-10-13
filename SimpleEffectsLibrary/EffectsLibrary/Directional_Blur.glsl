//Amount:8:0, Angle:0, Samples:16:2:64, Mask:texture
// @description A motion-blur-style effect that samples the source repeatedly along a configurable line and combines the taps with a center-weighted triangular kernel to produce a directional smear. Amount sets the blur radius in pixels, Angle sets the direction in degrees (0 horizontal, 90 vertical), and Samples sets the tap count (2-64, higher is smoother); both Amount and Angle can additionally be driven per-pixel by connecting textures to them. An optional Mask texture blends between the untouched source and the blurred result, with white areas receiving full blur, while alpha is preserved.
// @param Amount: Blur radius in pixels along the blur direction.
// @param Angle: Blur direction in degrees; 0 is horizontal, 90 is vertical.
// @param Samples: Number of texture taps used to build the blur (higher is smoother).
// @param Mask: Optional texture controlling where the blur is applied (white = full blur).
#version 410

// Required source texture. simpleEffect renders at this texture's resolution.
uniform sampler2D tSource;

// Standard uniforms supplied automatically by simpleEffect.
uniform vec2 uResolution;
uniform vec2 uTexelSize;
uniform float uTime;
uniform int uFrame;

// Float parameters. Every float can optionally receive a texture connection.
uniform float Amount;
uniform sampler2D AmountTex;
uniform int AmountTexConnected;

uniform float Angle;
uniform sampler2D AngleTex;
uniform int AngleTexConnected;

uniform float Samples;

// Explicit texture parameter and its connection flag.
uniform sampler2D Mask;
uniform int MaskConnected;

out vec4 out_color;

float readAmount(vec2 uv)
{
    return AmountTexConnected != 0 ? texture(AmountTex, uv).r : Amount;
}

float readAngle(vec2 uv)
{
    return AngleTexConnected != 0 ? texture(AngleTex, uv).r : Angle;
}

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 source = texture(tSource, uv);

    // Resolve parameters (texture-driven where connected).
    float amount = max(readAmount(uv), 0.0);
    float angle = radians(readAngle(uv));

    // Clamp the tap count to a safe, loopable range.
    int sampleCount = int(clamp(floor(Samples + 0.5), 2.0, 64.0));

    // Unit vector along the blur direction.
    vec2 dir = vec2(cos(angle), sin(angle));

    // Weighted line sampling with a triangular (center-weighted) kernel.
    vec3 sum = vec3(0.0);
    float weightSum = 0.0;

    for(int i = 0; i < sampleCount; ++i){
        // Map tap index to -1..1 across the blur span.
        float t = (float(i) / float(sampleCount - 1)) * 2.0 - 1.0;
        float w = 1.0 - abs(t);
        vec2 offset = dir * (t * amount) * uTexelSize;
        sum += texture(tSource, uv + offset).rgb * w;
        weightSum += w;
    }

    // Guard against a zero-weight denominator.
    vec3 blurred = weightSum > 0.0 ? sum / weightSum : source.rgb;

    vec3 processed = blurred;

    // Mask blends between the untouched source and the blurred result.
    if(MaskConnected != 0){
        float m = clamp(texture(Mask, uv).r, 0.0, 1.0);
        processed = mix(source.rgb, blurred, m);
    }

    // Preserve source alpha unless the effect intentionally changes it.
    out_color = vec4(processed, source.a);
}
