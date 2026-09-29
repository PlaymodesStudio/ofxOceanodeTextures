//Direction:0:0:3, Speed:0.002:0:0.25, SourcePosition:1:0:1
// @description Builds a slit-scan history by shifting the previous output and inserting a row or column from the current source on every render.
// @param Direction: History movement: zero Negative X, one Positive X, two Negative Y, or three Positive Y. Rounded to an integer.
// @param Speed: Distance shifted per render as a fraction of the corresponding canvas dimension. Zero freezes existing history.
// @param SourcePosition: Normalized source row or column copied into the entering edge.
#version 410

uniform sampler2D tSource;
uniform sampler2D tPreviousOutput;
uniform int tPreviousOutputConnected;
uniform vec2 uResolution;
uniform float Direction;
uniform float Speed;
uniform float SourcePosition; uniform sampler2D SourcePositionTex; uniform int SourcePositionTexConnected;
out vec4 out_color;

void main()
{
    ivec2 size = textureSize(tSource, 0);
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    if(tPreviousOutputConnected == 0){
        out_color = texelFetch(tSource, pixel, 0);
        return;
    }

    int direction = clamp(int(round(Direction)), 0, 3);
    int dimension = direction < 2 ? size.x : size.y;
    float normalizedSpeed = clamp(Speed, 0.0, 0.25);
    int shift = normalizedSpeed <= 0.0
              ? 0
              : clamp(int(round(normalizedSpeed * float(dimension))), 1, dimension);
    if(shift == 0){
        out_color = texelFetch(tPreviousOutput, pixel, 0);
        return;
    }

    vec2 uv = gl_FragCoord.xy / uResolution;
    float sourcePosition = SourcePositionTexConnected != 0
                         ? clamp(texture(SourcePositionTex, uv).r, 0.0, 1.0)
                         : SourcePosition;
    ivec2 sourcePixel = pixel;

    if(direction == 0){
        if(pixel.x < size.x - shift){
            out_color = texelFetch(tPreviousOutput, pixel + ivec2(shift, 0), 0);
            return;
        }
        sourcePixel.x = int(round(sourcePosition * float(size.x - 1)));
    }else if(direction == 1){
        if(pixel.x >= shift){
            out_color = texelFetch(tPreviousOutput, pixel - ivec2(shift, 0), 0);
            return;
        }
        sourcePixel.x = int(round(sourcePosition * float(size.x - 1)));
    }else if(direction == 2){
        if(pixel.y < size.y - shift){
            out_color = texelFetch(tPreviousOutput, pixel + ivec2(0, shift), 0);
            return;
        }
        sourcePixel.y = int(round(sourcePosition * float(size.y - 1)));
    }else{
        if(pixel.y >= shift){
            out_color = texelFetch(tPreviousOutput, pixel - ivec2(0, shift), 0);
            return;
        }
        sourcePixel.y = int(round(sourcePosition * float(size.y - 1)));
    }

    out_color = texelFetch(tSource, sourcePixel, 0);
}
