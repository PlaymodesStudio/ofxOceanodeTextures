//Position:0.5:0:1, Direction:0:0:3, Mix:1:0:1
// @description Extends one source row or column from a chosen position to an image edge.
// @param Position: Normalized position of the row or column that supplies the stretched pixels.
// @param Direction: Stretch direction: zero Negative X, one Positive X, two Negative Y, or three Positive Y. Rounded to an integer.
// @param Mix: Blends between the original image and the pixel-stretched result.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform float Position;
uniform sampler2D PositionTex;
uniform int PositionTexConnected;
uniform float Direction;
uniform float Mix;
uniform sampler2D MixTex;
uniform int MixTexConnected;
out vec4 out_color;

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    float position = seUnit(PositionTex, PositionTexConnected, Position, uv);
    int direction = clamp(int(round(Direction)), 0, 3);
    vec2 sourceUv = uv;

    if(direction == 0 && uv.x <= position){
        sourceUv.x = position;
    }else if(direction == 1 && uv.x >= position){
        sourceUv.x = position;
    }else if(direction == 2 && uv.y <= position){
        sourceUv.y = position;
    }else if(direction == 3 && uv.y >= position){
        sourceUv.y = position;
    }

    vec4 source = texture(tSource, uv);
    vec4 stretched = texture(tSource, sourceUv);
    out_color = mix(source, stretched, seUnit(MixTex, MixTexConnected, Mix, uv));
}
