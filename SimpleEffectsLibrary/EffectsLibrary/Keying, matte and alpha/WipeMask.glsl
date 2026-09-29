//Progress:0:0:1, Softness:0.05:0:1, Shape:0:0:1, Angle:0:-180:180, Symmetric:0:0:1, CenterX:0.5:0:1, CenterY:0.5:0:1, Curve:1:0.1:8, Invert:0:0:1
// @description Generates a reusable circular or directional wipe matte without requiring a second image.
// @param Progress: Reveals the matte from fully black at zero to fully white at one.
// @param Softness: Feather width as a fraction of the complete wipe span.
// @param Shape: Selects circular zero or linear one. Rounded to an integer.
// @param Angle: Direction of the linear wipe in degrees.
// @param Symmetric: Makes a linear wipe expand from the centre line toward both sides.
// @param CenterX: Horizontal origin for circular and symmetric linear wipes.
// @param CenterY: Vertical origin for circular and symmetric linear wipes.
// @param Curve: Shapes the generated grayscale transition.
// @param Invert: Crossfades to the inverse matte.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform float Progress; uniform sampler2D ProgressTex; uniform int ProgressTexConnected;
uniform float Softness; uniform sampler2D SoftnessTex; uniform int SoftnessTexConnected;
uniform float Shape;
uniform float Angle; uniform sampler2D AngleTex; uniform int AngleTexConnected;
uniform float Symmetric;
uniform float CenterX; uniform sampler2D CenterXTex; uniform int CenterXTexConnected;
uniform float CenterY; uniform sampler2D CenterYTex; uniform int CenterYTexConnected;
uniform float Curve; uniform sampler2D CurveTex; uniform int CurveTexConnected;
uniform float Invert; uniform sampler2D InvertTex; uniform int InvertTexConnected;
out vec4 out_color;

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec2 center = vec2(seUnit(CenterXTex, CenterXTexConnected, CenterX, uv),
                       seUnit(CenterYTex, CenterYTexConnected, CenterY, uv));
    vec2 point = (uv - center) * uResolution;
    vec2 corners[4] = vec2[4](
        (vec2(0.0, 0.0) - center) * uResolution,
        (vec2(1.0, 0.0) - center) * uResolution,
        (vec2(0.0, 1.0) - center) * uResolution,
        (vec2(1.0, 1.0) - center) * uResolution
    );

    float distanceValue;
    if(int(round(Shape)) == 0){
        float maximumDistance = max(max(length(corners[0]), length(corners[1])),
                                    max(length(corners[2]), length(corners[3])));
        distanceValue = length(point) / max(maximumDistance, SE_EPSILON);
    }else{
        float angle = radians(seRange(AngleTex, AngleTexConnected, Angle, -180.0, 180.0, uv));
        vec2 direction = vec2(cos(angle), sin(angle));
        float projection = dot(point, direction);
        float p0 = dot(corners[0], direction);
        float p1 = dot(corners[1], direction);
        float p2 = dot(corners[2], direction);
        float p3 = dot(corners[3], direction);
        if(Symmetric >= 0.5){
            float maximumProjection = max(max(abs(p0), abs(p1)), max(abs(p2), abs(p3)));
            distanceValue = abs(projection) / max(maximumProjection, SE_EPSILON);
        }else{
            float minimumProjection = min(min(p0, p1), min(p2, p3));
            float maximumProjection = max(max(p0, p1), max(p2, p3));
            distanceValue = (projection - minimumProjection) /
                            max(maximumProjection - minimumProjection, SE_EPSILON);
        }
    }

    float progress = seUnit(ProgressTex, ProgressTexConnected, Progress, uv);
    float softness = seUnit(SoftnessTex, SoftnessTexConnected, Softness, uv);
    float matte;
    if(progress <= 0.0){
        matte = 0.0;
    }else if(progress >= 1.0){
        matte = 1.0;
    }else{
        matte = softness <= SE_EPSILON
              ? step(distanceValue, progress)
              : 1.0 - smoothstep(progress - softness * 0.5,
                                 progress + softness * 0.5,
                                 distanceValue);
    }

    float curve = seRange(CurveTex, CurveTexConnected, Curve, 0.1, 8.0, uv);
    matte = pow(clamp(matte, 0.0, 1.0), curve);
    matte = mix(matte, 1.0 - matte, seUnit(InvertTex, InvertTexConnected, Invert, uv));
    out_color = vec4(vec3(matte), 1.0);
}
