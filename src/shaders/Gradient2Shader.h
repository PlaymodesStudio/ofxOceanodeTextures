R"(
#version 410

uniform sampler2D tSource;

uniform int numCols;

uniform vec4 color1;
uniform vec4 color2;
uniform vec4 color3;
uniform vec4 color4;
uniform vec4 color5;
uniform vec4 color6;
uniform vec4 color7;
uniform vec4 color8;
uniform vec4 color9;
uniform vec4 color10;

uniform float pos1;
uniform float pos2;
uniform float pos3;
uniform float pos4;
uniform float pos5;
uniform float pos6;
uniform float pos7;
uniform float pos8;
uniform float pos9;
uniform float pos10;

out vec4 out_color;

vec4 getColor(int i){
    switch(i){
    case 1:
        return color1;
        break;
    case 2:
        return color2;
        break;
    case 3:
        return color3;
        break;
    case 4:
        return color4;
        break;
    case 5:
        return color5;
        break;
    case 6:
        return color6;
        break;
    case 7:
        return color7;
        break;
    case 8:
        return color8;
        break;
    case 9:
        return color9;
        break;
    case 10:
        return color10;
        break;
    }
}

float getPos(int i){
    switch(i){
    case 1:
        return pos1;
        break;
    case 2:
        return pos2;
        break;
    case 3:
        return pos3;
        break;
    case 4:
        return pos4;
        break;
    case 5:
        return pos5;
        break;
    case 6:
        return pos6;
        break;
    case 7:
        return pos7;
        break;
    case 8:
        return pos8;
        break;
    case 9:
        return pos9;
        break;
    case 10:
        return pos10;
        break;
    }
}

void main()
{
    ivec2 uv = ivec2(gl_FragCoord.st);
    
    float value = texelFetch(tSource, uv, 0).g;

    float a;
    vec4 col;

    if (value <= pos1)
    {
        col = color1;
    }

    for (int i = 1; i < numCols; i++){
        vec4 col1 = getColor(i);
        vec4 col2 = getColor(i+1);
        float pos1Val = getPos(i);
        float pos2Val = getPos(i+1);
        
        if (value > pos1Val && value <= pos2Val)
        {
            a = (value - pos1Val)/(pos2Val - pos1Val);
            col = mix(col1, col2, a);
        }
    }

    if (value > getPos(numCols))
    {
        col = getColor(numCols);
    }

    out_color = col;
}


)"