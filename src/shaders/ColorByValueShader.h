R"(
#version 410

uniform sampler2D tex;
uniform samplerBuffer values;

out vec4 out_color;

void main(){
    int index = int(gl_FragCoord.y);
    float val = texelFetch(values, index).r;
    
    out_color = texture(tex, vec2(val, 0.5));
}
)"
