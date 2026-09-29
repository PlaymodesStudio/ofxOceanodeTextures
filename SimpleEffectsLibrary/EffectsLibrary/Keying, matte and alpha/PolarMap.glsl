//CenterX:0.5:0:1, CenterY:0.5:0:1, Radius:0.5:0.001:2, AngleOffset:0:-1:1, AngleLimit:1:0:1, AngleFeather:0:0:1, DefaultAngle:0:0:1
// @description Generates an aspect-correct polar control map with inward radial falloff in red and wrapped angle in green.
// @param CenterX: Horizontal centre in normalized canvas coordinates.
// @param CenterY: Vertical centre in normalized canvas coordinates.
// @param Radius: Radius measured as a fraction of the shorter canvas dimension.
// @param AngleOffset: Rotates the green angle channel; one represents a complete turn.
// @param AngleLimit: Radial position where the angle starts returning to DefaultAngle.
// @param AngleFeather: Radial width of the circular angle transition.
// @param DefaultAngle: Green-channel value emitted outside the angle region.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform float CenterX; uniform sampler2D CenterXTex; uniform int CenterXTexConnected;
uniform float CenterY; uniform sampler2D CenterYTex; uniform int CenterYTexConnected;
uniform float Radius; uniform sampler2D RadiusTex; uniform int RadiusTexConnected;
uniform float AngleOffset; uniform sampler2D AngleOffsetTex; uniform int AngleOffsetTexConnected;
uniform float AngleLimit; uniform sampler2D AngleLimitTex; uniform int AngleLimitTexConnected;
uniform float AngleFeather; uniform sampler2D AngleFeatherTex; uniform int AngleFeatherTexConnected;
uniform float DefaultAngle; uniform sampler2D DefaultAngleTex; uniform int DefaultAngleTexConnected;
out vec4 out_color;

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec2 center = vec2(seUnit(CenterXTex, CenterXTexConnected, CenterX, uv),
                       seUnit(CenterYTex, CenterYTexConnected, CenterY, uv));
    float radius = seRange(RadiusTex, RadiusTexConnected, Radius, 0.001, 2.0, uv);
    vec2 delta = (uv - center) * uResolution;
    float radialDistance = length(delta) / max(min(uResolution.x, uResolution.y) * radius, SE_EPSILON);
    float radial = 1.0 - clamp(radialDistance, 0.0, 1.0);

    float offset = seRange(AngleOffsetTex, AngleOffsetTexConnected, AngleOffset, -1.0, 1.0, uv);
    float angle = fract(atan(delta.y, delta.x) / (2.0 * SE_PI) + offset);
    float limit = seUnit(AngleLimitTex, AngleLimitTexConnected, AngleLimit, uv);
    float feather = seUnit(AngleFeatherTex, AngleFeatherTexConnected, AngleFeather, uv);
    float angleBlend = feather <= SE_EPSILON
                     ? step(limit, radialDistance)
                     : smoothstep(limit, limit + feather, radialDistance);
    float defaultAngle = seUnit(DefaultAngleTex, DefaultAngleTexConnected, DefaultAngle, uv);
    float shortestDifference = fract(defaultAngle - angle + 0.5) - 0.5;
    angle = fract(angle + shortestDifference * angleBlend);

    out_color = vec4(radial, angle, 0.0, 1.0);
}
