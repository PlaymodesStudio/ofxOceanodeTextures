//TopX:0.5:0:1, BottomX:0.5:0:1, BendAngle:0:-180:180, Side:1:0:1, BorderMode:0:0:2, Mix:1:0:1
// @description Bends one side of a crease running between normalized positions on the top and bottom image edges.
// @param TopX: Horizontal position where the crease meets the top edge.
// @param BottomX: Horizontal position where the crease meets the bottom edge.
// @param BendAngle: Signed rotation of the sampled coordinates around the crease, in degrees.
// @param Side: Selects the left side zero or right side one. Rounded to an integer.
// @param BorderMode: Outside sampling: zero Transparent, one Clamp, or two Wrap. Rounded to an integer.
// @param Mix: Blends between the source and bent result.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform float TopX; uniform sampler2D TopXTex; uniform int TopXTexConnected;
uniform float BottomX; uniform sampler2D BottomXTex; uniform int BottomXTexConnected;
uniform float BendAngle; uniform sampler2D BendAngleTex; uniform int BendAngleTexConnected;
uniform float Side;
uniform float BorderMode;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected;
out vec4 out_color;

vec4 sampleBentSource(vec2 uv, int borderMode)
{
    if(borderMode == 1){
        return texture(tSource, clamp(uv, vec2(0.0), vec2(1.0)));
    }
    if(borderMode == 2){
        return texture(tSource, fract(uv));
    }
    return seInside(uv) ? texture(tSource, uv) : vec4(0.0);
}

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec2 pixel = gl_FragCoord.xy;
    float topX = seUnit(TopXTex, TopXTexConnected, TopX, uv);
    float bottomX = seUnit(BottomXTex, BottomXTexConnected, BottomX, uv);
    // Orient the crease from bottom to top so its normal points toward the
    // visually left side of a vertical crease.
    vec2 creaseStart = vec2(bottomX * uResolution.x, 0.0);
    vec2 creaseEnd = vec2(topX * uResolution.x, uResolution.y);
    vec2 tangent = normalize(creaseEnd - creaseStart);
    vec2 normal = vec2(-tangent.y, tangent.x);
    vec2 relative = pixel - creaseStart;
    float along = dot(relative, tangent);
    float across = dot(relative, normal);
    bool bendLeft = int(round(Side)) == 0;
    bool shouldBend = bendLeft ? across > 0.0 : across < 0.0;
    vec2 sourcePixel = pixel;

    if(shouldBend){
        float angle = radians(seRange(BendAngleTex, BendAngleTexConnected, BendAngle, -180.0, 180.0, uv));
        sourcePixel = creaseStart
                    + tangent * (along + across * sin(angle))
                    + normal * (across * cos(angle));
    }

    vec4 source = texture(tSource, uv);
    vec4 bent = sampleBentSource(sourcePixel / uResolution,
                                 clamp(int(round(BorderMode)), 0, 2));
    out_color = mix(source, bent, seUnit(MixTex, MixTexConnected, Mix, uv));
}
