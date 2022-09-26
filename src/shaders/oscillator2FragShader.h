R"(
#version 410
#define M_PI 3.1415926535897932384626433832795

//uniform vec2 size;
uniform float phase;
uniform float time;
uniform float createRandoms;
uniform sampler2D indexs;

//vec2 size;

uniform float phaseOffset;
uniform float roundness;
uniform float pulseWidth;
uniform float skew;
uniform float randomAddition;
uniform float scale;
uniform float offset;
uniform float pow;
uniform float bipow;
uniform int quantization;
uniform float fader;
uniform float invert;

uniform sampler2D phaseOffset_tex;
uniform sampler2D roundness_tex;
uniform sampler2D pulseWidth_tex;
uniform sampler2D skew_tex;
uniform sampler2D randomAddition_tex;
uniform sampler2D scale_tex;
uniform sampler2D offset_tex;
uniform sampler2D pow_tex;
uniform sampler2D bipow_tex;
uniform sampler2D quantization_tex;
uniform sampler2D fader_tex;
uniform sampler2D invert_tex;

out vec4 out_color;

// ---------- A simplex noise helper method to please the eye:

// ---------
//
// Description : Array and textureless GLSL 2D simplex noise function.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : ijm
//     Lastmod : 20110822 (ijm)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise
//

vec3 mod289(vec3 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec2 mod289(vec2 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec3 permute(vec3 x) {
    return mod289(((x*34.0)+1.0)*x);
}

float snoise(vec2 v)
{
    const vec4 C = vec4(0.211324865405187,  // (3.0-sqrt(3.0))/6.0
                        0.366025403784439,  // 0.5*(sqrt(3.0)-1.0)
                        -0.577350269189626,  // -1.0 + 2.0 * C.x
                        0.024390243902439); // 1.0 / 41.0
    // First corner
    vec2 i  = floor(v + dot(v, C.yy) );
    vec2 x0 = v -   i + dot(i, C.xx);
    
    // Other corners
    vec2 i1;
    //i1.x = step( x0.y, x0.x ); // x0.x > x0.y ? 1.0 : 0.0
    //i1.y = 1.0 - i1.x;
    i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
    // x0 = x0 - 0.0 + 0.0 * C.xx ;
    // x1 = x0 - i1 + 1.0 * C.xx ;
    // x2 = x0 - 1.0 + 2.0 * C.xx ;
    vec4 x12 = x0.xyxy + C.xxzz;
    x12.xy -= i1;
    
    // Permutations
    i = mod289(i); // Avoid truncation effects in permutation
    vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
                     + i.x + vec3(0.0, i1.x, 1.0 ));
    
    vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
    m = m*m ;
    m = m*m ;
    
    // Gradients: 41 points uniformly over a line, mapped onto a diamond.
    // The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)
    
    vec3 x = 2.0 * fract(p * C.www) - 1.0;
    vec3 h = abs(x) - 0.5;
    vec3 ox = floor(x + 0.5);
    vec3 a0 = x - ox;
    
    // Normalise gradients implicitly by scaling m
    // Approximation of: m *= inversesqrt( a0*a0 + h*h );
    m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );
    
    // Compute final noise value at P
    vec3 g;
    g.x  = a0.x  * x0.x  + h.x  * x0.y;
    g.yz = a0.yz * x12.xz + h.yz * x12.yw;
    return 130.0 * dot(m, g);
}

// --------- / end simplex noise

float random (in vec2 _st) {
    return fract(sin(dot(_st.xy,
                         vec2(12.9898,78.233)))*
                 43758.5453123);
}

float map(float value, float istart, float istop, float ostart, float ostop, bool clampVal) {
    float val = ostart + (ostop - ostart) * ((value - istart) / (istop - istart));
    if(clampVal){
        return clamp(val, ostart, ostop);
    }
    return val;
}

float map(float value, float istart, float istop, float ostart, float ostop) {
    return map(value, istart, istop, ostart, ostop, false);
}

float customPow(float value, float pow){
    float k1 = 2*pow*0.99999;
    float k2 = (k1/((-pow*0.999999)+1));
    float k3 = k2 * abs(value) + 1;
    value = value * (k2+1) / k3;
    return value;
}

highp float rrrand(vec2 co, float time)
{
    highp float a = 12.9898;
    highp float b = 78.233;
    highp float c = 43758.5453;
    highp float dt= dot(co.xy ,vec2(a,b));
    highp float sn= mod(dt,3.14);
    return fract(sin(sn) * c);
}


// *** Change these to suit your range of random numbers..

// *** Use this for integer stepped ranges, ie Value-Noise/Perlin noise functions.
#define HASHSCALE1 .1031

// For smaller input rangers like audio tick or 0-1 UVs use these...
//#define HASHSCALE1 443.8975



//----------------------------------------------------------------------------------------
//  1 out, 3 in...
float hash13(vec3 p3)
{
    p3  = fract(p3 * HASHSCALE1);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

void main(){
    float debug = 0;
	//we grab the x and y and store them in an int
    ivec2 st   = ivec2(gl_FragCoord.st);
    vec2 size = textureSize(indexs, 0).xy;
	int xVal = st.x;
	int yVal = st.y;
    int width = int(size.x);
    int height = int(size.y);
    
    //Compute Index
    int xIndex = xVal;
    int yIndex = yVal;
    
    float index = texelFetch(indexs, ivec2(xVal, yVal), 0).r;
    
    //Compute parameters of current coord;
    float phaseOffsetParam = phaseOffset;
    if(textureSize(phaseOffset_tex, 0).xy != vec2(1.0f, 1.0f)){
        phaseOffsetParam = mix(0, 1, texelFetch(phaseOffset_tex, st, 0).r);
    }
    
    float roundnessParam = roundness;
    if(textureSize(roundness_tex, 0).xy != vec2(1.0f, 1.0f)){
        phaseOffsetParam = mix(0, 1, texelFetch(roundness_tex, st, 0 ).r);
    }
    
    float pulseWidthParam = pulseWidth;
    if(textureSize(pulseWidth_tex, 0).xy != vec2(1.0f, 1.0f)){
        pulseWidthParam = mix(0, 1, texelFetch(pulseWidth_tex, st, 0 ).r);
    }

    float skewParam = skew;
    if(textureSize(skew_tex, 0).xy != vec2(1.0f, 1.0f)){
        skewParam = mix(-1, 1, texelFetch(skew_tex, st, 0 ).r);
    }
    
    
    float linPhase = phase + index + phaseOffsetParam;
        
    debug = phaseOffsetParam;

    linPhase = mod(linPhase, 1);
    
    if(pulseWidthParam < 0.5){
        linPhase = map(linPhase, 0.5-pulseWidthParam, 0.5+pulseWidthParam, 0, 1, true);
        if(skewParam < 0 && linPhase == 1) linPhase = 0;
    }else if (pulseWidthParam == 1){
        linPhase = map(skewParam, -1, 1, 1, 0);
    }else{
        float midpoint = map(skewParam, -1, 1, 1, 0);
        if(linPhase < midpoint){
            linPhase = map(linPhase, 0, map(1-pulseWidthParam, 0, 0.5, 0, midpoint), 0, midpoint, true);
        }else{
            linPhase = map(linPhase, map(pulseWidthParam, 0.5, 1, midpoint, 1), 1, midpoint, 1, true);
        }
    }
    
    float skewedLinPhase = linPhase;
    
    if(skewParam < 0){
        if(linPhase < 0.5+((abs(skewParam))*0.5))
            skewedLinPhase = map(linPhase, 0.0, 0.5+((abs(skewParam))*0.5), 0.0, 0.5, true);
        else
            skewedLinPhase = map(linPhase, 0.5+((abs(skewParam))*0.5), 1, 0.5, 1, true);
    }
    else if(skewParam > 0){
        if(linPhase > ((1-abs(skewParam))*0.5))
            skewedLinPhase = map(linPhase, (1-abs(skewParam))*0.5, 1, 0.5, 1, true);
        else
            skewedLinPhase = map(linPhase, 0, ((1-abs(skewParam))*0.5), 0.0, 0.5, true);
    }
    
    linPhase = skewedLinPhase;
    
    //get phasor to be w (radial freq)
    float w = linPhase * 2*M_PI;
    float val = 0;
    if(roundnessParam == 0){
        val = 1-(abs((linPhase * (-2)) + 1));
    }else if(roundnessParam == 0.5){
        val = (cos(w+M_PI) + 1 ) / 2;
    }else if(roundnessParam == 1){
        val = linPhase < 0.25 || linPhase >= 0.75 ? 0 : 1;
    }else if(roundnessParam < 0.5){
        float tri_val = 1-(abs((linPhase * (-2)) + 1));
        float cos_val = (cos(w+M_PI) + 1 ) / 2;
        val = mix(tri_val, cos_val, roundnessParam*2);
    }else{
        float cos_val = cos(w+M_PI);
        cos_val = customPow(cos_val, (roundnessParam-0.5)*2);
        cos_val = map(cos_val, -1.0, 1.0, 0.0, 1.0);
        val = cos_val;
    }
    
    float randomAdditionParam = randomAddition;
    if(textureSize(randomAddition_tex, 0).xy != vec2(1.0f, 1.0f)){
            randomAdditionParam = mix(-1, 1, texelFetch(randomAddition_tex, st, 0 ).r);
    }

    float scaleParam = scale;
    if(textureSize(scale_tex, 0).xy != vec2(1.0f, 1.0f)){
            scaleParam = mix(0, 2, texelFetch(scale_tex, st, 0 ).r);
    }

    float offsetParam = offset;
    if(textureSize(offset_tex, 0).xy != vec2(1.0f, 1.0f)){
            offsetParam = mix(-1, 1, texelFetch(offset_tex, st, 0 ).r);
    }

    float powParam = pow;
    if(textureSize(pow_tex, 0).xy != vec2(1.0f, 1.0f)){
            powParam = mix(-1, 1, texelFetch(pow_tex, st, 0 ).r);
    }

    float bipowParam = bipow;
    if(textureSize(bipow_tex, 0).xy != vec2(1.0f, 1.0f)){
            bipowParam = mix(-1, 1, texelFetch(bipow_tex, st, 0 ).r);
    }

    int quantizationParam = quantization;
//    if(textureSize(quantization_tex, 0).xy != vec2(1.0f, 1.0f)){
//            quantizationParam = mix(0, 255, texelFetch(quantization_tex, st, 0 ).r);
//    }

    float faderParam = fader;
    if(textureSize(fader_tex, 0).xy != vec2(1.0f, 1.0f)){
            faderParam = mix(0, 1, texelFetch(fader_tex, st, 0 ).r);
    }

    float invertParam = invert;
    if(textureSize(invert_tex, 0).xy != vec2(1.0f, 1.0f)){
            invertParam = mix(0, 1, texelFetch(invert_tex, st, 0 ).r);
    }

    
    //random Add
    if(randomAdditionParam != 0){
        val = val + randomAdditionParam*hash13(vec3(xVal, yVal, time));
    }
    
    val = clamp(val, 0.0, 1.0);
    
    //pow
    if(powParam != 0)
        val = customPow(val, powParam);
    
    //bipow
    if(bipowParam != 0){
        val = (val*2) -1;
        val = customPow(val, bipowParam);
        val = (val+1) * 0.5;
    }
    
    val = clamp(val, 0.0, 1.0);
    
    //Quantization
    if(quantizationParam < 255){
        val = (1.0/(float(quantizationParam-1)))*float(floor(val*quantizationParam));
    }
    
    //SCALE
    val *= scaleParam;
    
    //OFFSET
    val += offsetParam;
    
    val = clamp(val, 0.0, 1.0);
    
    val *= faderParam;
    
    float invertedValue = 1-val;
    float nonInvertedValue = val;
    val = (invertParam * invertedValue) + ((1-invertParam) * nonInvertedValue);
    
    out_color = vec4(val, val, val, 1);
//    out_color = vec4(debug, debug, debug, 1);
}
)"
