//PositionX:0.5:0:1, PositionY:0.5:0:1, Scale:0.5:0:2, Color:color:1:1:1:1, Pow:1:0:8, Amount:1:0:1, UseSource:bool:true, Mask:texture
// @description CircularGradientPro generates an aspect-corrected circular radial gradient centered anywhere on the frame and composites it over the incoming texture, or over a transparent black background when UseSource is off. Its principal controls are the gradient center position (X/Y), radius Scale, Color (whose alpha sets peak center opacity), a Pow exponent shaping the falloff from linear to a tight bright center, an Amount blend strength, and an optional Mask texture that limits where the gradient appears. As with other simpleEffect parameters, the scalar controls can each be driven by an external texture connection for animation or modulation.
// @param PositionX: Normalized horizontal center of the gradient.
// @param PositionY: Normalized vertical center of the gradient.
// @param Scale: Normalized radius of the gradient (aspect-corrected fraction of the screen).
// @param Color: Gradient color; its alpha sets the peak opacity at the center.
// @param Pow: Power applied to the falloff curve (1 = linear, higher = tighter/brighter center).
// @param Amount: Overall blend strength of the gradient; can also be driven by a texture.
// @param UseSource: When true, the incoming texture is kept as the background; when false, the background is transparent black (alpha = 0).
// @param Mask: Optional texture limiting where the gradient appears.
#version 410

// Required source texture. simpleEffect renders at this texture's resolution.
uniform sampler2D tSource;

// Standard uniforms supplied automatically by simpleEffect.
uniform vec2 uResolution;

// Normalized position of the gradient center. Each float can optionally
// receive a texture connection for animation/modulation.
uniform float PositionX;
uniform sampler2D PositionXTex;
uniform int PositionXTexConnected;

uniform float PositionY;
uniform sampler2D PositionYTex;
uniform int PositionYTexConnected;

// Normalized radius of the gradient.
uniform float Scale;
uniform sampler2D ScaleTex;
uniform int ScaleTexConnected;

// Curve exponent of the falloff.
uniform float Pow;
uniform sampler2D PowTex;
uniform int PowTexConnected;

// Overall blend strength.
uniform float Amount;
uniform sampler2D AmountTex;
uniform int AmountTexConnected;

// Gradient color (alpha = peak opacity at the center).
uniform vec4 Color;

// Background mode: true keeps the incoming texture, false uses transparent black.
uniform bool UseSource;

// Explicit texture parameter and its connection flag.
uniform sampler2D Mask;
uniform int MaskConnected;

out vec4 out_color;

float readFloat(sampler2D tex, int connected, float fallback, vec2 uv)
{
    return connected != 0 ? texture(tex, uv).r : fallback;
}

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 source = texture(tSource, uv);

    float posX = readFloat(PositionXTex, PositionXTexConnected, PositionX, uv);
    float posY = readFloat(PositionYTex, PositionYTexConnected, PositionY, uv);
    float scale = readFloat(ScaleTex, ScaleTexConnected, Scale, uv);
    float curvePow = readFloat(PowTex, PowTexConnected, Pow, uv);
    float amount = readFloat(AmountTex, AmountTexConnected, Amount, uv);

    // Aspect-corrected distance keeps the gradient circular on non-square outputs.
    vec2 aspect = vec2(uResolution.x / max(uResolution.y, 1e-5), 1.0);
    vec2 center = vec2(posX, posY);

    float dist = length((uv - center) * aspect);

    // Normalize by scale: 1.0 at the center, 0.0 at the gradient edge.
    float radius = max(scale, 1e-5);
    float g = clamp(1.0 - dist / radius, 0.0, 1.0);

    // Pow curve: 1 = linear falloff, higher values concentrate the gradient.
    g = pow(g, max(curvePow, 1e-4));

    float alpha = clamp(g * Color.a * amount, 0.0, 1.0);

    if(MaskConnected != 0){
        alpha *= texture(Mask, uv).r;
    }

    vec3 processed;
    float outAlpha;

    if(UseSource){
        // Keep the incoming texture as the background and blend the gradient over it.
        processed = mix(source.rgb, Color.rgb, alpha);
        outAlpha = source.a;
    } else {
        // Transparent black background: only the gradient remains, with its own alpha.
        processed = Color.rgb;
        outAlpha = alpha;
    }

    out_color = vec4(processed, outAlpha);
}
