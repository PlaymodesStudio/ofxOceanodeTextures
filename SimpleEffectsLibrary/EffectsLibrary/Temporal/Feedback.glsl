//Persistence:0:0:1, InputGain:1:0:2, Zoom:1:0.5:2, Rotation:0:-15:15, OffsetX:0:-0.25:0.25, OffsetY:0:-0.25:0.25, BlendMode:0:0:2, BorderMode:0:0:1, InvertHistory:0:0:1
// @description Recursive image feedback with fading, aspect-correct zoom, rotation, translation, and optional inversion of the previous output. Defaults pass the source through unchanged. History is automatic; connect only a source image. Feedback advances once per render, so speed and decay depend on rendering cadence.
// @param Persistence: Fraction of the previous output retained per render. Zero removes trails; one disables their decay.
// @param InputGain: Brightness of fresh input RGB. Zero stops injecting colour while existing feedback continues; source alpha is preserved.
// @param Zoom: Scale of the previous output per render, around the image centre. Above one expands trails; below one contracts them.
// @param Rotation: Rotation of the previous output in degrees per render, in texture coordinates. Display flips can reverse its apparent direction.
// @param OffsetX: Horizontal shift of the previous output per render, as a fraction of image width.
// @param OffsetY: Vertical shift of the previous output per render, as a fraction of image height, in texture coordinates.
// @param BlendMode: Zero Mix for soft ghosts, one Lighten for bright trails on dark backgrounds, two Add for luminous accumulation that can saturate. Rounded to an integer.
// @param BorderMode: Zero Transparent Black outside the transformed history; one Wrap for repeating edges. Rounded to an integer.
// @param InvertHistory: Inverts the previous output's RGB before blending and storing the next result. Zero is off, one is full inversion; alpha is preserved.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform sampler2D tPreviousOutput;
uniform int tPreviousOutputConnected;
uniform vec2 uResolution;

uniform float Persistence; uniform sampler2D PersistenceTex; uniform int PersistenceTexConnected;
uniform float InputGain; uniform sampler2D InputGainTex; uniform int InputGainTexConnected;
uniform float Zoom; uniform sampler2D ZoomTex; uniform int ZoomTexConnected;
uniform float Rotation; uniform sampler2D RotationTex; uniform int RotationTexConnected;
uniform float OffsetX; uniform sampler2D OffsetXTex; uniform int OffsetXTexConnected;
uniform float OffsetY; uniform sampler2D OffsetYTex; uniform int OffsetYTexConnected;
uniform float BlendMode;
uniform float BorderMode;
uniform float InvertHistory; uniform sampler2D InvertHistoryTex; uniform int InvertHistoryTexConnected;

out vec4 out_color;

vec4 historyTexel(ivec2 pixel, ivec2 size, bool wrapEdges)
{
    if(wrapEdges){
        pixel = ((pixel % size) + size) % size;
    }else if(any(lessThan(pixel, ivec2(0))) || any(greaterThanEqual(pixel, size))){
        return vec4(0.0);
    }
    return texelFetch(tPreviousOutput, pixel, 0);
}

vec4 sampleHistory(vec2 uv)
{
    // Explicit bilinear sampling keeps small transforms smooth even though
    // simpleEffect uses nearest-filtered buffers for general-purpose data.
    ivec2 size = textureSize(tPreviousOutput, 0);
    vec2 pixel = uv * vec2(size) - 0.5;
    ivec2 base = ivec2(floor(pixel));
    vec2 weight = fract(pixel);
    bool wrapEdges = int(round(BorderMode)) == 1;
    vec4 row0 = mix(historyTexel(base, size, wrapEdges),
                    historyTexel(base + ivec2(1, 0), size, wrapEdges), weight.x);
    vec4 row1 = mix(historyTexel(base + ivec2(0, 1), size, wrapEdges),
                    historyTexel(base + ivec2(1, 1), size, wrapEdges), weight.x);
    return mix(row0, row1, weight.y);
}

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    float gain = clamp(seRange(InputGainTex, InputGainTexConnected, InputGain, 0.0, 2.0, uv), 0.0, 2.0);
    vec4 source = texelFetch(tSource, ivec2(gl_FragCoord.xy), 0);
    vec4 current = vec4(source.rgb * gain, source.a);

    float persistence = clamp(seUnit(PersistenceTex, PersistenceTexConnected, Persistence, uv), 0.0, 1.0);
    // Seed from the input on the first render. With zero persistence, bypass
    // history entirely so even HDR and signed RGBA values pass through exactly.
    // Never fetch full-resolution coordinates from the 1x1 fallback texture.
    if(tPreviousOutputConnected == 0 || persistence <= 0.0){
        out_color = current;
        return;
    }

    float zoom = clamp(seRange(ZoomTex, ZoomTexConnected, Zoom, 0.5, 2.0, uv), 0.5, 2.0);
    float angle = radians(seRange(RotationTex, RotationTexConnected, Rotation, -15.0, 15.0, uv));
    vec2 offset = vec2(seRange(OffsetXTex, OffsetXTexConnected, OffsetX, -0.25, 0.25, uv),
                       seRange(OffsetYTex, OffsetYTexConnected, OffsetY, -0.25, 0.25, uv));

    // Transform in pixel space so a non-square camera image rotates without stretching.
    vec2 point = (uv - vec2(0.5) - offset) * uResolution;
    mat2 inverseRotation = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
    vec2 historyUv = (inverseRotation * point) / (zoom * uResolution) + vec2(0.5);
    vec4 previous = sampleHistory(historyUv);
    float inversion = seUnit(InvertHistoryTex, InvertHistoryTexConnected, InvertHistory, uv);
    // Scale inverted RGB by coverage so transparent transformed borders stay black.
    // For opaque camera input this is the usual 1 - RGB colour negative.
    previous.rgb = mix(previous.rgb, (vec3(1.0) - previous.rgb) * previous.a, inversion);

    int mode = clamp(int(round(BlendMode)), 0, 2);
    if(mode == 0){
        out_color = mix(current, previous, persistence);
    }else if(mode == 1){
        out_color = max(current, previous * persistence);
    }else{
        // Add RGB while retaining coverage, rather than repeatedly summing alpha.
        out_color = vec4(current.rgb + previous.rgb * persistence,
                         max(current.a, previous.a * persistence));
    }
    out_color = clamp(out_color, 0.0, 1.0);
}
