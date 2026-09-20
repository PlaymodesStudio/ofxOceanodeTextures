//Matte:texture, Amount:1:0:1, UseAlpha:1:0:1, Invert:0:0:1, Premultiply:1:0:1
// @description Applies luminance or alpha from a second texture to the source alpha.
// @param Matte: Texture supplying the new matte.
// @param Amount: Blends between the original alpha and the supplied matte.
// @param UseAlpha: Selects luminance at zero and Matte alpha at one.
// @param Invert: Crossfades between the matte and its inverse.
// @param Premultiply: Multiplies RGB by the resulting matte for directly visible black areas.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform sampler2D Matte;
uniform int MatteConnected;
uniform float Amount; uniform sampler2D AmountTex; uniform int AmountTexConnected;
uniform float UseAlpha; uniform sampler2D UseAlphaTex; uniform int UseAlphaTexConnected;
uniform float Invert; uniform sampler2D InvertTex; uniform int InvertTexConnected;
uniform float Premultiply; uniform sampler2D PremultiplyTex; uniform int PremultiplyTexConnected;
out vec4 out_color;

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 source = texture(tSource, uv);
    if(MatteConnected == 0){
        out_color = source;
        return;
    }
    vec4 matteSource = texture(Matte, uv);
    float useAlpha = seUnit(UseAlphaTex, UseAlphaTexConnected, UseAlpha, uv);
    float matte = mix(clamp(seLuma(matteSource.rgb), 0.0, 1.0), matteSource.a, useAlpha);
    float invert = seUnit(InvertTex, InvertTexConnected, Invert, uv);
    matte = mix(matte, 1.0 - matte, invert);
    float amount = seUnit(AmountTex, AmountTexConnected, Amount, uv);
    float appliedMatte = mix(1.0, matte, amount);
    float premultiply = seUnit(PremultiplyTex, PremultiplyTexConnected, Premultiply, uv);
    vec3 rgb = mix(source.rgb, source.rgb * appliedMatte, premultiply);
    out_color = vec4(rgb, source.a * appliedMatte);
}
