//Amount:1:0:2, Tint:color:1:1:1:1, Mask:texture
// @description Demonstrates the standard Simple Effect shader structure.
// @param Amount: Scalar strength; it can also be driven by a texture.
// @param Tint: Color multiplied with the source.
// @param Mask: Optional texture limiting where the effect is applied.
#version 410

// Required source texture. simpleEffect renders at this texture's resolution.
uniform sampler2D tSource;

// Standard uniforms supplied automatically by simpleEffect.
uniform vec2 uResolution;
uniform vec2 uTexelSize;
uniform float uTime;
uniform int uFrame;

// Float parameter. Every float can optionally receive a texture connection.
uniform float Amount;
uniform sampler2D AmountTex;
uniform int AmountTexConnected;

// Color parameters are exposed as ofFloatColor and bound as vec4.
uniform vec4 Tint;

// Explicit texture parameter and its connection flag.
uniform sampler2D Mask;
uniform int MaskConnected;

out vec4 out_color;

float readAmount(vec2 uv)
{
    return AmountTexConnected != 0 ? texture(AmountTex, uv).r : Amount;
}

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 source = texture(tSource, uv);

    float amount = readAmount(uv);
    vec3 processed = source.rgb * Tint.rgb * amount;

    if(MaskConnected != 0){
        processed *= texture(Mask, uv).r;
    }

    // Preserve source alpha unless the effect intentionally changes it.
    out_color = vec4(processed, source.a);
}
