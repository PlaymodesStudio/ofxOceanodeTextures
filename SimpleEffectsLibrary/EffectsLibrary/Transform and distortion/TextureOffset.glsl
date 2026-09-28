//OffsetX:0:-1:1, OffsetY:0:-1:1
// @description Moves the source texture horizontally and vertically, wrapping pixels across opposite edges.
// @param OffsetX: Horizontal offset as a fraction of the texture width; positive values move the image right.
// @param OffsetY: Vertical offset as a fraction of the texture height; positive values follow increasing texture V.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform float OffsetX;
uniform sampler2D OffsetXTex;
uniform int OffsetXTexConnected;
uniform float OffsetY;
uniform sampler2D OffsetYTex;
uniform int OffsetYTexConnected;
out vec4 out_color;

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec2 offset = vec2(
        seRange(OffsetXTex, OffsetXTexConnected, OffsetX, -1.0, 1.0, uv),
        seRange(OffsetYTex, OffsetYTexConnected, OffsetY, -1.0, 1.0, uv)
    );

    // Inverse sampling makes positive offsets move the visible image in the
    // positive axis direction. fract() wraps both positive and negative UVs.
    out_color = texture(tSource, fract(uv - offset));
}
