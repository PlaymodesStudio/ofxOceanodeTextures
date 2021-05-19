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

void main()
{
    ivec2 uv = ivec2(gl_FragCoord.st);
    
    float value = texelFetch(tSource, uv, 0).g;

    float a;
    vec3 col;

    if (value <= color1.a)
    {
        col = color1.rgb;
    }

    for (int i = 1; i < numCols; i++){
        vec4 col1 = getColor(i);
        vec4 col2 = getColor(i+1);
        if (value > col1.a && value <= col2.a)
        {
            a = (value - col1.a)/(col2.a - col1.a);
            col = mix(col1.rgb, col2.rgb, a);
        }
    }

    if (value > getColor(numCols).a)
    {
        col = getColor(numCols).rgb;
    }

//    if (value > color1.a && value <= color2.a)
//    {
//        a = (value - color1.a)/(color2.a - color1.a);
//        col = mix(color1.rgb, color2.rgb, a);
//    }
//
//    if (value > color2.a && value <= color3.a)
//    {
//        a = (value - color2.a)/(color3.a - color2.a);
//        col = mix(color2.rgb, color3.rgb, a);
//    }
//
//    if (value > color3.a && value <= color4.a)
//    {
//        a = (value - color3.a)/(color4.a - color3.a);
//        col = mix(color3.rgb, color4.rgb, a);
//    }
//
//    if (value > color4.a && value <= color5.a)
//    {
//        a = (value - color4.a)/(color5.a - color4.a);
//        col = mix(color4.rgb, color5.rgb, a);
//    }

//    if (value > color5.a)
//    {
//        col = color5.rgb;
//    }

    out_color = vec4( col.rgb, 1.0 );
    //out_color = vec4(1.0, 1.0, 1.0, 1.0);
}


)"
