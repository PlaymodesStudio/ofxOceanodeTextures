//PositionX:0.5:0:1, PositionY:0.5:0:1, ScaleX:1:0.01:10, ScaleY:1:0.01:10, Rotation:0:-180:180, AnchorX:0.5:0:1, AnchorY:0.5:0:1, Opacity:1:0:1
// @description Applies an inverse-sampled two-dimensional transform inside the source canvas.
// @param PositionX: Horizontal position of the anchor in normalized canvas coordinates.
// @param PositionY: Vertical position of the anchor in normalized canvas coordinates.
// @param ScaleX: Horizontal scale multiplier.
// @param ScaleY: Vertical scale multiplier.
// @param Rotation: Clockwise rotation in degrees.
// @param AnchorX: Horizontal transform anchor within the source.
// @param AnchorY: Vertical transform anchor within the source.
// @param Opacity: Scales the transformed source alpha.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform float PositionX; uniform sampler2D PositionXTex; uniform int PositionXTexConnected;
uniform float PositionY; uniform sampler2D PositionYTex; uniform int PositionYTexConnected;
uniform float ScaleX; uniform sampler2D ScaleXTex; uniform int ScaleXTexConnected;
uniform float ScaleY; uniform sampler2D ScaleYTex; uniform int ScaleYTexConnected;
uniform float Rotation; uniform sampler2D RotationTex; uniform int RotationTexConnected;
uniform float AnchorX; uniform sampler2D AnchorXTex; uniform int AnchorXTexConnected;
uniform float AnchorY; uniform sampler2D AnchorYTex; uniform int AnchorYTexConnected;
uniform float Opacity; uniform sampler2D OpacityTex; uniform int OpacityTexConnected;
out vec4 out_color;

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec2 position = vec2(seUnit(PositionXTex, PositionXTexConnected, PositionX, uv),
                         seUnit(PositionYTex, PositionYTexConnected, PositionY, uv));
    vec2 anchor = vec2(seUnit(AnchorXTex, AnchorXTexConnected, AnchorX, uv),
                       seUnit(AnchorYTex, AnchorYTexConnected, AnchorY, uv));
    vec2 scale = vec2(seRange(ScaleXTex, ScaleXTexConnected, ScaleX, 0.01, 10.0, uv),
                      seRange(ScaleYTex, ScaleYTexConnected, ScaleY, 0.01, 10.0, uv));
    float angle = radians(seRange(RotationTex, RotationTexConnected, Rotation, -180.0, 180.0, uv));
    float opacity = seUnit(OpacityTex, OpacityTexConnected, Opacity, uv);

    vec2 point = uv - position;
    mat2 inverseRotation = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
    vec2 sourceUv = (inverseRotation * point) / max(scale, vec2(0.01)) + anchor;

    if(!seInside(sourceUv)){
        out_color = vec4(0.0);
        return;
    }

    vec4 source = texture(tSource, sourceUv);
    out_color = vec4(source.rgb, source.a * opacity);
}
