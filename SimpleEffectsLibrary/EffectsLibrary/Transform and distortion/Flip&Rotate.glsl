//FlipX:bool, FlipY:bool, Rotate90:bool
// @description Flips the complete source on either axis, then optionally rotates it 90 degrees clockwise. Rotation swaps output width and height without stretching or cropping. Preserves every source texel, including alpha. All controls off is identity.
// @param FlipX: Reverses source columns before rotation.
// @param FlipY: Reverses source rows before rotation.
// @param Rotate90: Rotates clockwise by 90 degrees and swaps width and height.
// @swap-dimensions Rotate90
#version 410

uniform sampler2D tSource;
uniform bool FlipX;
uniform bool FlipY;
uniform bool Rotate90;
out vec4 out_color;

void main()
{
    ivec2 sourceSize = textureSize(tSource, 0);
    ivec2 pixel = ivec2(gl_FragCoord.xy);

    // Undo the clockwise turn to find the flipped source pixel.
    if(Rotate90){
        pixel = ivec2(pixel.y, sourceSize.y - 1 - pixel.x);
    }
    if(FlipX){
        pixel.x = sourceSize.x - 1 - pixel.x;
    }
    if(FlipY){
        pixel.y = sourceSize.y - 1 - pixel.y;
    }

    out_color = texelFetch(tSource, pixel, 0);
}
