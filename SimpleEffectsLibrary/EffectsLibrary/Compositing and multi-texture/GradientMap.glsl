//Gradient:texture, InputBlack:0:0:1, InputWhite:1:0:1, Reverse:0:0:1, Mix:1:0:1
// @description Replaces source colors by looking up luminance along a horizontal gradient texture.
// @param Gradient: Horizontal color ramp sampled from left to right.
// @param InputBlack: Source luminance mapped to the left edge of the gradient.
// @param InputWhite: Source luminance mapped to the right edge of the gradient.
// @param Reverse: Reverses the gradient lookup direction.
// @param Mix: Blends between the source and mapped color.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform sampler2D Gradient;
uniform int GradientConnected;
uniform float InputBlack; uniform sampler2D InputBlackTex; uniform int InputBlackTexConnected;
uniform float InputWhite; uniform sampler2D InputWhiteTex; uniform int InputWhiteTexConnected;
uniform float Reverse; uniform sampler2D ReverseTex; uniform int ReverseTexConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected;
out vec4 out_color;

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 source = texture(tSource, uv);
    if(GradientConnected == 0){
        out_color = source;
        return;
    }
    float inputBlack = seUnit(InputBlackTex, InputBlackTexConnected, InputBlack, uv);
    float inputWhite = seUnit(InputWhiteTex, InputWhiteTexConnected, InputWhite, uv);
    float position = clamp((seLuma(source.rgb) - inputBlack) / max(inputWhite - inputBlack, SE_EPSILON), 0.0, 1.0);
    float reverse = seUnit(ReverseTex, ReverseTexConnected, Reverse, uv);
    position = mix(position, 1.0 - position, reverse);
    vec3 mapped = texture(Gradient, vec2(position, 0.5)).rgb;
    float amount = seUnit(MixTex, MixTexConnected, Mix, uv);
    out_color = vec4(mix(source.rgb, mapped, amount), source.a);
}
