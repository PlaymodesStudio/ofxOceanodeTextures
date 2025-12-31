R"(
#version 410

uniform sampler2D tSource;

// Multiple Render Targets - one for each channel
layout(location = 0) out vec4 redOut;
layout(location = 1) out vec4 greenOut;
layout(location = 2) out vec4 blueOut;
layout(location = 3) out vec4 alphaOut;

void main()
{
    ivec2 uv = ivec2(gl_FragCoord.st);
    
    vec4 sourceColor = texelFetch(tSource, uv, 0);
    
    // Red channel output - painted in red
    redOut = vec4(sourceColor.r, 0.0, 0.0, 1.0);
    
    // Green channel output - painted in green
    greenOut = vec4(0.0, sourceColor.g, 0.0, 1.0);
    
    // Blue channel output - painted in blue
    blueOut = vec4(0.0, 0.0, sourceColor.b, 1.0);
    
    // Alpha channel output - painted in white (grayscale)
    alphaOut = vec4(sourceColor.a, sourceColor.a, sourceColor.a, 1.0);
}

)"