R"(
#version 410
#define M_PI 3.1415926535897932384626433832795

//uniform vec2 size;
uniform float phase;
uniform float time;
uniform float createRandoms;
uniform sampler2D indexs;
uniform sampler2D randomInfo;
uniform sampler2D oldPhaseInfo;

uniform samplerBuffer parameters;

uniform samplerBuffer indexRandomValues;

int sizePosition = 0;
int phaseOffsetPosition = 1;
int roundnessPosition = 2;
int pulseWidthPosition = 3;
int skewPosition = 4;
int randomAdditionPosition = 5;
int scalePosition = 6;
int offsetPosition = 7;
int powPosition = 8;
int bipowPosition = 9;
int quantizationPosition = 10;
int faderPosition = 11;
int invertPosition = 12;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_randomInfo;
layout(location = 2) out vec4 out_oldPhase;

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

float map(float value, float istart, float istop, float ostart, float ostop, bool _clamp) {
    float val = ostart + (ostop - ostart) * ((value - istart) / (istop - istart));
    if(_clamp){
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
    return (value * (k2+1) / k3);
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
	//we grab the x and y and store them in an int
	int xVal = int(gl_FragCoord.x);
	int yVal = int(gl_FragCoord.y);
    int width = textureSize(randomInfo, 0).x;
    int height = textureSize(randomInfo, 0).y;
    int dimensionsSum = width+height;
    
    //Compute Index
    int xIndex = xVal;
    int yIndex = yVal;
    int widthItem = int(texelFetch(parameters, yVal + (dimensionsSum*sizePosition)).r);
    int heightItem = int(texelFetch(parameters, xVal + (dimensionsSum*sizePosition) + height).r);
    
    float index = texelFetch(indexs, ivec2(xVal, yVal), 0).r;
    
    //Compute parameters of current coord;
    float phaseOffsetParam = texelFetch(parameters, xVal + (dimensionsSum*phaseOffsetPosition)).r + texelFetch(parameters, yVal + (dimensionsSum*phaseOffsetPosition) + width).r;
    float roundnessParamX = texelFetch(parameters, xVal + (dimensionsSum*roundnessPosition)).r;
    float roundnessParamY = texelFetch(parameters, yVal + (dimensionsSum*roundnessPosition) + width).r;
    float roundnessParam = map(map(roundnessParamX, 0, 1, -1, 1) + map(roundnessParamY, 0, 1, -1, 1), -1, 1, 0, 1, true);
    float pulseWidthParamX = texelFetch(parameters, xVal + (dimensionsSum*pulseWidthPosition)).r;
    float pulseWidthParamY = texelFetch(parameters, yVal + (dimensionsSum*pulseWidthPosition) + width).r;
    float pulseWidthParam = map(map(pulseWidthParamX, 0, 1, -1, 1) + map(pulseWidthParamY, 0, 1, -1, 1), -1, 1, 0, 1, true);
    float skewParam = texelFetch(parameters, xVal + (dimensionsSum*skewPosition)).r + texelFetch(parameters, yVal + (dimensionsSum*skewPosition) + width).r;
    float randomAdditionParam = texelFetch(parameters, xVal + (dimensionsSum*randomAdditionPosition)).r + texelFetch(parameters, yVal + (dimensionsSum*randomAdditionPosition) + width).r;
    float scaleParam = texelFetch(parameters, xVal + (dimensionsSum*scalePosition)).r * texelFetch(parameters, yVal + (dimensionsSum*scalePosition) + width).r;
    float offsetParam = texelFetch(parameters, xVal + (dimensionsSum*offsetPosition)).r + texelFetch(parameters, yVal + (dimensionsSum*offsetPosition) + width).r;
    float powParam = texelFetch(parameters, xVal + (dimensionsSum*powPosition)).r + texelFetch(parameters, yVal + (dimensionsSum*powPosition) + width).r;
    float bipowParam = texelFetch(parameters, xVal + (dimensionsSum*bipowPosition)).r + texelFetch(parameters, yVal + (dimensionsSum*bipowPosition) + width).r;
    int quantizationParam = int(min(texelFetch(parameters, xVal + (dimensionsSum*quantizationPosition)).r, texelFetch(parameters, yVal + (dimensionsSum*quantizationPosition) + width).r));
    float faderParam = texelFetch(parameters, xVal + (dimensionsSum*faderPosition)).r * texelFetch(parameters, yVal + (dimensionsSum*faderPosition) + width).r;
    float invertParam = max(texelFetch(parameters, xVal + (dimensionsSum*invertPosition)).r, texelFetch(parameters, yVal + (dimensionsSum*invertPosition) + width).r);
    
    //randon Info
    vec4 r_info = texelFetch(randomInfo, ivec2(xVal, yVal), 0);
    float pastRandom = r_info.r;
    float newRandom = r_info.g;
    float oldRandom = r_info.b;
    float futureRandom = r_info.a;
    
    float oldPhasor = texelFetch(oldPhaseInfo, ivec2(xVal, yVal), 0).r;
    
    
    //If we have changed size or initialized the texture we have to insert new items for pastRandom, newRandom, futureRandom and oldRandom, as we have no random data.
    if(createRandoms == 1){
        pastRandom = hash13(vec3(xIndex, yIndex, time*2));
        newRandom = hash13(vec3(xIndex, yIndex, time));
        oldRandom = hash13(vec3(xIndex, yIndex, time*3));
        futureRandom = hash13(vec3(xIndex, yIndex, time*4));
    }
    
    float linPhase = phase + index + phaseOffsetParam;
    
    linPhase = mod(linPhase, 1);
    
    if(pulseWidthParam < 0.5){
        linPhase = map(linPhase, 0.5-pulseWidthParam, 0.5+pulseWidthParam, 0, 1, true);
        if(skewParam < 0 && linPhase == 1) linPhase = 0;
    }else if (pulseWidthParam == 1){
        linPhase = 0.5;
    }else{
        if(linPhase < 0.5){
            linPhase = map(linPhase, 0, 1-pulseWidthParam, 0, 0.5, true);
        }else{
            linPhase = map(linPhase, pulseWidthParam, 1, 0.5, 1, true);
        }
    }
    
    float skewedLinPhase = linPhase;
    
    if(skewParam < 0){
        if(linPhase < 0.5+((abs(skewParam))*0.5))
            skewedLinPhase = map(linPhase, 0.0, 0.5+((abs(skewParam))*0.5), 0.0, 0.5);
        else
            skewedLinPhase = map(linPhase, 0.5+((abs(skewParam))*0.5), 1, 0.5, 1);
    }
    else if(skewParam > 0){
        if(linPhase > ((1-abs(skewParam))*0.5))
            skewedLinPhase = map(linPhase, (1-abs(skewParam))*0.5, 1, 0.5, 1);
        else
            skewedLinPhase = map(linPhase, 0, ((1-abs(skewParam))*0.5), 0.0, 0.5);
    }
    
    linPhase = skewedLinPhase;
    
    float val = 0;
    if(linPhase < oldPhasor){
        pastRandom = oldRandom;
        oldRandom = newRandom;
        newRandom = futureRandom;
//        if(customDiscreteDistribution.size() > 1){
//            std::discrete_distribution<float> disdist(customDiscreteDistribution.begin(), customDiscreteDistribution.end());
//            futureRandom = disdist(mt)/(customDiscreteDistribution.size()-1);
//        }else{
            futureRandom = hash13(vec3(xIndex, yIndex, time));
//        }
        //pow
        if(powParam != 0)
            futureRandom = customPow(futureRandom, powParam);
        
        //bipow
        if(bipowParam != 0){
            futureRandom = (futureRandom*2) -1;
            futureRandom = customPow(futureRandom, bipowParam);
            futureRandom = (futureRandom+1) * 0.5;
        }
        
        futureRandom = clamp(futureRandom, 0.0, 1.0);
        
        //Quantization
        if(quantizationParam < 255){
            futureRandom = (1.0/(float(quantizationParam-1)))*float(floor(futureRandom*quantizationParam));
        }
        
        
        val = oldRandom;
    }
    else{
        //rand2
        float lin_interp = oldRandom*(1-linPhase) + newRandom*linPhase;
        
        //rand3
        float x = linPhase;
        if(roundnessParam > 0.5){
            x = (x*2) - 1;
            x = customPow(x, (roundnessParam-0.5) * 2);
            x = (x + 1) / 2.0;
        }
        float L0 = (newRandom - pastRandom) * 0.5;
        float L1 = L0 + (oldRandom-newRandom);
        float L2 = L1 + ((futureRandom - oldRandom)*0.5) + (oldRandom - newRandom);
        float curve_interp = oldRandom + (x * (L0 + (x * ((x * L2) - (L1 + L2)))));
        
        if(roundnessParam < 0.5){
            val = (1-(roundnessParam*2)) * lin_interp + (roundnessParam*2)*curve_interp;
        }else{
            val = curve_interp;
        }
    }
    
    //random Add
    if(randomAdditionParam != 0){
        val = val + randomAdditionParam*hash13(vec3(xVal, yVal, time));
    }
    
    val = clamp(val, 0.0, 1.0);
    
    //SCALE
    val *= scaleParam;
    
    //OFFSET
    val += offsetParam;
    
    val = clamp(val, 0.0, 1.0);
    
    val *= faderParam;
    
    float invertedValue = 1-val;
    float nonInvertedValue = val;
    val = (invertParam * invertedValue) + ((1-invertParam) * nonInvertedValue);
    
//    val = oldPhasor;
    
    out_color = vec4(val, val, val, 1.0);
//    out_randomInfo = vec4(val, val, val, 1.0);
//    out_oldPhase = vec4(val, val, val, 1.0);
    
    out_randomInfo = vec4(pastRandom, newRandom, oldRandom, futureRandom);
    out_oldPhase = vec4(linPhase, 0.0, 0.0, 1.0);
}
)"
