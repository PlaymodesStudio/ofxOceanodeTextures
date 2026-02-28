R"(
#version 410

uniform sampler2D tSource;

uniform int numCols;
uniform int interpMode;

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

vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// Oklab/Oklch conversions
const float PI = 3.14159265359;

vec3 rgb2oklab(vec3 c) {
    float l = 0.4122214708 * c.r + 0.5363325363 * c.g + 0.0514459929 * c.b;
    float m = 0.2119034982 * c.r + 0.6806995451 * c.g + 0.1073969566 * c.b;
    float s = 0.0883024619 * c.r + 0.2817188376 * c.g + 0.6299787005 * c.b;

    float l_ = pow(l, 1.0/3.0);
    float m_ = pow(m, 1.0/3.0);
    float s_ = pow(s, 1.0/3.0);

    return vec3(
        0.2104542553 * l_ + 0.7936177850 * m_ - 0.0040720468 * s_,
        1.9779984951 * l_ - 2.4285922050 * m_ + 0.4505937099 * s_,
        0.0259040371 * l_ + 0.7827717662 * m_ - 0.8086757660 * s_
    );
}

vec3 oklab2rgb(vec3 c) {
    float l_ = c.x + 0.3963377774 * c.y + 0.2158037573 * c.z;
    float m_ = c.x - 0.1055613458 * c.y - 0.0638541728 * c.z;
    float s_ = c.x - 0.0894841775 * c.y - 1.2914855480 * c.z;

    float l = l_ * l_ * l_;
    float m = m_ * m_ * m_;
    float s = s_ * s_ * s_;

    return vec3(
        4.07660 * l - 3.307711 * m + 0.230969 * s,
        -1.26843 * l + 2.609757 * m - 0.341319 * s,
        -0.00419 * l - 0.703418 * m + 1.707614 * s
    );
}

vec3 oklab2oklch(vec3 c) {
    float l = c.x;
    float a = c.y;
    float b = c.z;

    float C = sqrt(a * a + b * b);
    float h = atan(b, a) / (2.0 * PI); // -0.5 to 0.5
    if (h < 0.0) h += 1.0; // 0.0 to 1.0

    return vec3(l, C, h);
}

vec3 oklch2oklab(vec3 c) {
    float l = c.x;
    float C = c.y;
    float h = c.z * 2.0 * PI;

    return vec3(l, C * cos(h), C * sin(h));
}

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
    
    vec3 src = texelFetch(tSource, uv, 0).rgb;
    float value = dot(src, vec3(0.299, 0.587, 0.114));

    vec4 col = getColor(numCols);

    if (value <= pos1)
    {
        col = color1;
    }
    else
    {
        for (int i = 1; i < numCols; i++){
            float pos2Val = getPos(i+1);
            if (value <= pos2Val)
            {
                float pos1Val = getPos(i);
                vec4 col1 = getColor(i);
                vec4 col2 = getColor(i+1);
                
                float denom = pos2Val - pos1Val;
                if (denom == 0.0) denom = 0.000001;
                float a = smoothstep(0.0, 1.0, clamp((value - pos1Val) / denom, 0.0, 1.0));
                
                if (interpMode == 0) { // RGB
                    col = mix(col1, col2, a);
                } else if (interpMode == 1 || interpMode == 2) { // HSV
                    vec3 hsv1 = rgb2hsv(col1.rgb);
                    vec3 hsv2 = rgb2hsv(col2.rgb);
                    
                    float h;
                    bool shortPath = (interpMode == 1);
                    
                    if(shortPath){
                        // Shortest path interpolation for hue
                        h = mix(hsv1.x, hsv2.x, a);
                        if (abs(hsv1.x - hsv2.x) > 0.5) {
                            if (hsv1.x < hsv2.x) {
                                h = mix(hsv1.x + 1.0, hsv2.x, a);
                            } else {
                                h = mix(hsv1.x, hsv2.x + 1.0, a);
                            }
                            h = fract(h);
                        }
                    } else {
                        // Longest path interpolation for hue
                        h = mix(hsv1.x, hsv2.x, a);
                        if (abs(hsv1.x - hsv2.x) <= 0.5) {
                            if (hsv1.x < hsv2.x) {
                                h = mix(hsv1.x + 1.0, hsv2.x, a);
                            } else {
                                h = mix(hsv1.x, hsv2.x + 1.0, a);
                            }
                            h = fract(h);
                        }
                    }
                    
                    vec3 s = mix(vec3(hsv1.y), vec3(hsv2.y), a);
                    vec3 v = mix(vec3(hsv1.z), vec3(hsv2.z), a);
                    
                    col = vec4(hsv2rgb(vec3(h, s.x, v.x)), mix(col1.a, col2.a, a));
                } else if (interpMode == 3) { // Oklab
                    vec3 lab1 = rgb2oklab(col1.rgb);
                    vec3 lab2 = rgb2oklab(col2.rgb);
                    vec3 lab = mix(lab1, lab2, a);
                    col = vec4(oklab2rgb(lab), mix(col1.a, col2.a, a));
                } else if (interpMode == 4 || interpMode == 5) { // Oklch
                    vec3 lab1 = rgb2oklab(col1.rgb);
                    vec3 lab2 = rgb2oklab(col2.rgb);
                    vec3 lch1 = oklab2oklch(lab1);
                    vec3 lch2 = oklab2oklch(lab2);
                    
                    float h;
                    bool shortPath = (interpMode == 4);
                    
                    if(shortPath){
                        // Shortest path interpolation for hue
                        h = mix(lch1.z, lch2.z, a);
                        if (abs(lch1.z - lch2.z) > 0.5) {
                            if (lch1.z < lch2.z) {
                                h = mix(lch1.z + 1.0, lch2.z, a);
                            } else {
                                h = mix(lch1.z, lch2.z + 1.0, a);
                            }
                            h = fract(h);
                        }
                    } else {
                        // Longest path interpolation for hue
                        h = mix(lch1.z, lch2.z, a);
                        if (abs(lch1.z - lch2.z) <= 0.5) {
                            if (lch1.z < lch2.z) {
                                h = mix(lch1.z + 1.0, lch2.z, a);
                            } else {
                                h = mix(lch1.z, lch2.z + 1.0, a);
                            }
                            h = fract(h);
                        }
                    }
                    
                    float l = mix(lch1.x, lch2.x, a);
                    float c = mix(lch1.y, lch2.y, a);
                    
                    vec3 resLch = vec3(l, c, h);
                    col = vec4(oklab2rgb(oklch2oklab(resLch)), mix(col1.a, col2.a, a));
                }
                break;
            }
        }
    }

	out_color = vec4(clamp(col.rgb, 0.0, 1.0), col.a);
}


)"