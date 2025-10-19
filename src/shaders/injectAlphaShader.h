R"(
#version 410

uniform sampler2D textureIn;
uniform sampler2D alphaTexture;
uniform vec2 size;
uniform int hasAlphaTexture;

out vec4 fragColor;

void main(){
    vec2 uv = gl_FragCoord.xy / size;
    vec4 texColor = texture(textureIn, uv);
    
    float alphaValue;
    
    if(hasAlphaTexture == 1) {
        // Use red channel from alpha texture
        vec4 alphaTex = texture(alphaTexture, uv);
        alphaValue = alphaTex.r;
    } else {
        // Use luminance of incoming texture as alpha
        // Standard luminance calculation: 0.299*R + 0.587*G + 0.114*B
        alphaValue = dot(texColor.rgb, vec3(0.299, 0.587, 0.114));
    }
    
    // Output the texture with modified alpha channel
    fragColor = vec4(texColor.rgb, alphaValue);
}
)"