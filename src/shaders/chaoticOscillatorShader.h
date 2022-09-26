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
int lengthPosition = 13;
int seedPosition = 14;

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

float modulateRandom(float value, float powValue, float bipowValue, int quantizationValue){
    //pow
    if(powValue != 0)
        value = customPow(value, powValue);
    
    //bipow
    if(bipowValue != 0){
        value = (value*2) -1;
        value = customPow(value, bipowValue);
        value = (value+1) * 0.5;
    }
    
    value = clamp(value, 0.0, 1.0);
    
    //Quantization
    if(quantizationValue < 255){
        value = (1.0/(float(quantizationValue-1)))*float(floor(value*quantizationValue));
    }
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
    
    if(xVal >= widthItem || yVal >= heightItem){
        out_color = vec4(0, 0, 0, 1);
        return;
    }

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
    float length = texelFetch(parameters, xVal + (dimensionsSum*lengthPosition)).r * texelFetch(parameters, yVal + (dimensionsSum*lengthPosition) + width).r;
    int seed = int(texelFetch(parameters, xVal + (dimensionsSum*seedPosition)).r + texelFetch(parameters, yVal + (dimensionsSum*seedPosition) + width).r);
    
    //randon Info
    vec4 r_info = texelFetch(randomInfo, ivec2(xVal, yVal), 0);
    float pastRandomPreMod = r_info.r;
    float newRandomPreMod = r_info.g;
    float oldRandomPreMod = r_info.b;
    float futureRandomPreMod = r_info.a;
    
    
    float randomCountStep = 0.001;
    float maxFloat = 3.402823466e+38;


    vec4 phasor_info = texelFetch(oldPhaseInfo, ivec2(xVal, yVal), 0);
    float oldPhasor = phasor_info.r;
    float randomCount = phasor_info.g;
    float accumulateCycles = phasor_info.b;
    
    
    //If we have changed size or initialized the texture we have to insert new items for pastRandom, newRandom, futureRandom and oldRandom, as we have no random data.
    if(createRandoms == 1){
        randomCount = 0;
        accumulateCycles = 0;
        if(seed > 0){
            oldPhasor = -1;
            pastRandomPreMod = 0;
            newRandomPreMod = hash13(vec3(seed, seed, randomCount));
            oldRandomPreMod = 0;
            futureRandomPreMod = hash13(vec3(seed, seed, randomCount));
        }else if(seed == 0){
            pastRandomPreMod = hash13(vec3(index, index, time*2));
            newRandomPreMod = hash13(vec3(index, index, time));
            oldRandomPreMod = hash13(vec3(index, index, time*3));
            futureRandomPreMod = hash13(vec3(index, index, time*4));
        }else{
            pastRandomPreMod = hash13(vec3(xIndex*seed, yIndex*seed, time*2));
            newRandomPreMod = hash13(vec3(xIndex*seed, yIndex*seed, time));
            oldRandomPreMod = hash13(vec3(xIndex*seed, yIndex*seed, time*3));
            futureRandomPreMod = hash13(vec3(xIndex*seed, yIndex*seed, time*4));
        }
    }
    
    float pastRandom = modulateRandom(pastRandomPreMod, powParam, bipowParam, quantizationParam);
    float newRandom = modulateRandom(newRandomPreMod, powParam, bipowParam, quantizationParam);
    float oldRandom = modulateRandom(oldRandomPreMod, powParam, bipowParam, quantizationParam);
    float futureRandom = modulateRandom(futureRandomPreMod, powParam, bipowParam, quantizationParam);
    
    float linPhase = phase + (index*length) + phaseOffsetParam;
    
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
        pastRandomPreMod = oldRandomPreMod;
        oldRandomPreMod = newRandomPreMod;
        newRandomPreMod = futureRandomPreMod;
        
        pastRandom = oldRandom;
        oldRandom = newRandom;
        newRandom = futureRandom;
        
        float indexPosShifted = (1-mod(index+phaseOffsetParam, 1))*length;
        if(indexPosShifted == floor(indexPosShifted)) indexPosShifted-=1;
        if(accumulateCycles >= floor(indexPosShifted)){
            if(seed == 0){
                futureRandomPreMod = hash13(vec3(index, index, time));
            }else if(seed < 0){
                futureRandomPreMod = hash13(vec3(xIndex*seed, yIndex*seed, randomCount));
            }else{
                futureRandomPreMod = hash13(vec3(seed, seed, randomCount));
            }
            
            if(maxFloat - randomCount < 1){ //randomCountStep){
                randomCount = 0;
            }else{
                randomCount = randomCount + 1;//randomCountStep;
            }
            futureRandom = modulateRandom(futureRandomPreMod, powParam, bipowParam, quantizationParam);
        }
        else{
            accumulateCycles+=1;
        }
        
    }
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
    
    //out_color = vec4(randomCount /255.0f, mod(randomCount, 255.0f)/255.0f, mod(randomCount, 255.0f)/255.0f, 1.0);
    out_color = vec4(val, val , val, 1.0f);
//    out_randomInfo = vec4(val, val, val, 1.0);
//    out_oldPhase = vec4(val, val, val, 1.0);
    
    out_randomInfo = vec4(pastRandomPreMod, newRandomPreMod, oldRandomPreMod, futureRandomPreMod);
    //out_randomInfo = vec4(mod(randomCount, 255.0f)/255.0f, newRandomPreMod, oldRandomPreMod, futureRandomPreMod);
    out_oldPhase = vec4(linPhase, randomCount, accumulateCycles, 1.0);
}
)"
