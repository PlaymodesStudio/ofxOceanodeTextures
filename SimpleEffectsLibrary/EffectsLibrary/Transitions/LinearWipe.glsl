//LayerB:texture, Progress:0:0:1, Softness:0.05:0:1, Angle:0:-180:180
// @description Reveals a second texture using an angle-controlled linear transition.
// @param LayerB: Texture revealed by the wipe.
// @param Progress: Transition completion from source to LayerB.
// @param Softness: Width of the feathered transition edge.
// @param Angle: Direction of the wipe in degrees.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform sampler2D LayerB;
uniform int LayerBConnected;
uniform float Progress; uniform sampler2D ProgressTex; uniform int ProgressTexConnected;
uniform float Softness; uniform sampler2D SoftnessTex; uniform int SoftnessTexConnected;
uniform float Angle; uniform sampler2D AngleTex; uniform int AngleTexConnected;
out vec4 out_color;

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 source = texture(tSource, uv);
    if(LayerBConnected == 0){
        out_color = source;
        return;
    }
    vec4 layer = texture(LayerB, uv);
    float progress = seUnit(ProgressTex, ProgressTexConnected, Progress, uv);
    if(progress <= 0.0){
        out_color = source;
        return;
    }
    if(progress >= 1.0){
        out_color = layer;
        return;
    }
    float angle = radians(seRange(AngleTex, AngleTexConnected, Angle, -180.0, 180.0, uv));
    vec2 direction = vec2(cos(angle), sin(angle));
    float projected = dot(uv - 0.5, direction) / max(abs(direction.x) + abs(direction.y), SE_EPSILON) + 0.5;
    float softness = seUnit(SoftnessTex, SoftnessTexConnected, Softness, uv);
    float matte = softness <= SE_EPSILON
                ? step(projected, progress)
                : 1.0 - smoothstep(progress - softness * 0.5, progress + softness * 0.5, projected);
    out_color = mix(source, layer, matte);
}
