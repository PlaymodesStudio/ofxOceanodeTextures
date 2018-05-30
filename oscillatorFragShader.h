R"(
#version 410
#define M_PI 3.1415926535897932384626433832795

//uniform vec2 size;
uniform float phase;
uniform float time;
uniform sampler2D randomInfo;

uniform usamplerBuffer intParameters;
uniform samplerBuffer floatParameters;

uniform samplerBuffer indexRandomValues;

int indexNumWavesPosition = 0;
int indexInvertPosition = 1;
int indexSymmetryPosition = 0;
int indexRandomPosition = 2;
int indexOffsetPosition = 3;
int indexQuantizationPosition = 1;
int indexCombinationPosition = 4;
int indexModuloPosition = 2;
int phaseOffsetPosition = 5;
int pulseWidthPosition = 6;
int skewPosition = 7;
int waveformPosition = 8;



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

float map(float value, float istart, float istop, float ostart, float ostop) {
    return ostart + (ostop - ostart) * ((value - istart) / (istop - istart));
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
    uint xQuantization = texelFetch(intParameters, yVal + (dimensionsSum*indexQuantizationPosition)).r;
    uint yQuantization = texelFetch(intParameters, xVal + (dimensionsSum*indexQuantizationPosition) + height).r;
    uint xSymmetry = texelFetch(intParameters, yVal + (dimensionsSum*indexSymmetryPosition)).r;
    uint ySymmetry = texelFetch(intParameters, xVal + (dimensionsSum*indexSymmetryPosition)+ height).r;
    float xIndexOffset = texelFetch(floatParameters, yVal + (dimensionsSum*indexOffsetPosition)).r;
    float yIndexOffset = texelFetch(floatParameters, xVal + (dimensionsSum*indexOffsetPosition) + height).r;
    float xIndexRandom = texelFetch(floatParameters, yVal + (dimensionsSum*indexRandomPosition)).r;
    float yIndexRandom = texelFetch(floatParameters, xVal + (dimensionsSum*indexRandomPosition) + height).r;
    float xIndexCombination = texelFetch(floatParameters, yVal + (dimensionsSum*indexCombinationPosition)).r;
    float yIndexCombination = texelFetch(floatParameters, xVal + (dimensionsSum*indexCombinationPosition) + height).r;
    uint xIndexModulo = texelFetch(intParameters, yVal + (dimensionsSum*indexModuloPosition)).r;
    uint yIndexModulo = texelFetch(intParameters, xVal + (dimensionsSum*indexModuloPosition) + height).r;
    float xNumWaves = texelFetch(floatParameters, yVal + (dimensionsSum*indexNumWavesPosition)).r;
    float yNumWaves = texelFetch(floatParameters, xVal + (dimensionsSum*indexNumWavesPosition) + height).r;
    float xInvert = texelFetch(floatParameters, yVal + (dimensionsSum*indexInvertPosition)).r;
    float yInvert = texelFetch(floatParameters, xVal + (dimensionsSum*indexInvertPosition) + height).r;

    //Offset
    xIndex = int(mod((xIndex - round(xIndexOffset)), width));
    yIndex = int(mod((yIndex - round(yIndexOffset)), height));
    
    //Quantization
    xIndex = int(floor(float(xIndex)/(float(width)/float(xQuantization))));
    yIndex = int(floor(float(yIndex)/(float(height)/float(yQuantization))));

    //Symmetry
    while (xSymmetry > xQuantization-1) {
        xSymmetry--;
    }
    
    while (ySymmetry > yQuantization-1) {
        ySymmetry--;
    }


    //X
    bool odd = false;
    
    if(int((float(xIndex)/(float(xQuantization)/float(xSymmetry+1))))%2 == 1){
        odd = true;
    }

    if(abs(xIndexOffset) - int(abs(xIndexOffset)) > 0.5){
        //odd = !odd;
    }
    
    int veusSym = int(xQuantization)/int(xSymmetry+1);
    xIndex = veusSym-int(abs(((int(xIndex/veusSym)%2) * veusSym)-(xIndex%veusSym)));

    if(xQuantization % 2 == 0){
        xIndex += odd ? 1 : 0;
    }
    else if(xSymmetry > 0){
        xIndex += 1;
        xIndex %= int(xQuantization);
    }

    //Y
    odd = false;
   
    if(int((float(yIndex)/(float(yQuantization)/float(ySymmetry+1))))%2 == 1){
        odd = true;
    }
    
    if(abs(yIndexOffset) - int(abs(yIndexOffset)) > 0.5){
        //odd = !odd;
    }

    veusSym = int(yQuantization)/int(ySymmetry+1);
    yIndex = veusSym-int(abs(((int(yIndex/veusSym)%2) * veusSym)-(yIndex%veusSym)));


    if(yQuantization % 2 == 0){
        yIndex += odd ? 1 : 0;
    }
    
    else if(ySymmetry > 0){
        yIndex += 1;
        yIndex %= int(yQuantization);
    }
    
    
    xIndex = width - xIndex;
    yIndex = height - yIndex;
    
//    if(xQuantization % 2 == 0){
//        xIndex -= 1;
//    }
    
//    if(yQuantization % 2 == 0){
//        yIndex -= 1;
//    }


    //Random
    xIndex += int(texelFetch(indexRandomValues, xIndex).r * xIndexRandom);
    xIndex %= width;
    if(xIndex < 0){
        xIndex += width;
    }

    yIndex += int(texelFetch(indexRandomValues, yIndex + width).r * yIndexRandom);
    yIndex %= height;
    if(yIndex < 0){
        yIndex += height;
    }

    //COMBINATION
    xIndex = int(abs(((xIndex%2)*width*xIndexCombination)-xIndex));
    yIndex = int(abs(((yIndex%2)*height*yIndexCombination)-yIndex));

    //Modulo
    if(xIndexModulo != width)
        xIndex %= int(xIndexModulo);
    if(yIndexModulo != height)
        yIndex %= int(yIndexModulo);
    
    float xNumWavesInverted = -xNumWaves * xInvert;
    float yNumWavesInverted = -yNumWaves * yInvert;


    float xIndexf = ((float(xIndex)/float(width)))*(xNumWavesInverted)*(float(width)/float(xQuantization))*(xSymmetry+1);
    float yIndexf = ((float(yIndex)/float(height)))*(yNumWavesInverted)*(float(height)/float(yQuantization))*(ySymmetry+1);


    float index = xIndexf + yIndexf;
    
    
    //Compute parameters of current coord;
    float phaseOffsetParam = texelFetch(floatParameters, xVal + (dimensionsSum*phaseOffsetPosition)).r + texelFetch(floatParameters, yVal + (dimensionsSum*phaseOffsetPosition) + width).r;
    float pulseWidthParam = texelFetch(floatParameters, xVal + (dimensionsSum*pulseWidthPosition)).r * texelFetch(floatParameters, yVal + (dimensionsSum*pulseWidthPosition) + width).r;
    float skewParam = texelFetch(floatParameters, xVal + (dimensionsSum*skewPosition)).r + texelFetch(floatParameters, yVal + (dimensionsSum*skewPosition) + width).r;
    
    //How to blend waveform Parameter???
    float waveformParam = max(texelFetch(floatParameters, xVal + (dimensionsSum*waveformPosition)).r, texelFetch(floatParameters, yVal + (dimensionsSum*waveformPosition) + width).r);
    
    
    //randon Info
    vec4 r_info = texelFetch(randomInfo, ivec2(xVal, yVal), 0);
    float oldValue = r_info.r;
    float oldPhasor = r_info.g;
    float pastRandom = r_info.b;
    float newRandom = r_info.a;
    
    
    
    //get phasor to be w (radial freq)
    float w = (phase * 2 * M_PI);
    
    float k = (index + phaseOffsetParam) * 2 * M_PI;
    
    w = w + k;
    w = mod(w, 2*M_PI);
//    
    w = map(w, (1-pulseWidthParam)*2*M_PI, 2*M_PI, 0, 2*M_PI);
    w = clamp(w, 0, 2*M_PI);

    float w_skewed = w;


    if(skewParam < 0){
        if(w < M_PI+((abs(skewParam))*M_PI)){
            w_skewed = map(w, 0, M_PI+((abs(skewParam))*M_PI), 0, M_PI);
        }
        else{
            w_skewed = map(w, M_PI+((abs(skewParam))*M_PI), 2*M_PI, M_PI, 2*M_PI);
        }
    }
    else if(skewParam > 0){
        if(w > ((1-abs(skewParam))*M_PI)){
            w_skewed = map(w, (1-abs(skewParam))*M_PI, 2*M_PI, M_PI, 2*M_PI);
        }
        else{
            w_skewed = map(w, 0, ((1-abs(skewParam))*M_PI), 0, M_PI);
        }
    }

    w = clamp(w_skewed, 0, 2*M_PI);    
    
    float linPhase =  w / (2*M_PI);
    float val1 = 0;
    float val2 = 0;
    if(waveformParam >= 0 && waveformParam < 1){ //SIN
        val1 = (sin(w) + 1 ) / 2;
    }
    if(waveformParam > 0 && waveformParam < 2){ //COS
        val2 = (cos(w) + 1 ) / 2;
    }
    if(waveformParam > 1 && waveformParam < 3){ //TRI
        val1 = 1-(abs((linPhase * (-2)) + 1));
    }
    if(waveformParam > 2 && waveformParam < 4){ //SQUARE
        val2 = (linPhase > 0) ? 1 : 0;
    }
    if(waveformParam > 3 && waveformParam < 5){ //SAW
        val1 = 1-linPhase;
    }
    if(waveformParam > 4 && waveformParam < 6){ //INVERTED SAW
        val2 = linPhase;
    }
    if(waveformParam > 5 && waveformParam < 7){ //Random
        if(linPhase < oldPhasor){
            val1 = hash13(vec3(xIndex, yIndex, time));
        }
        else{
            val1 = oldValue;
        }
    }
    if(waveformParam > 6 && waveformParam < 8){
        if(linPhase < oldPhasor){
            pastRandom = newRandom;
            newRandom = hash13(vec3(xIndex, yIndex, time));
            val2 = pastRandom;
        }
        else{
            val2 = (pastRandom*(1-linPhase)) + (newRandom*linPhase);
        }
    }
    if(waveformParam > 7 && waveformParam <= 8){
        if(linPhase < oldPhasor){
            pastRandom = newRandom;
            newRandom = hash13(vec3(xIndex, yIndex, time));
            val1 = pastRandom;
        }
        else{
            float smoothPhase = 1 - (cos(w/2) + 1)/ 2;
            val1 = (pastRandom*(1-smoothPhase)) + (newRandom*smoothPhase);
        }
    }
    
    float val = 0;
    float waveInterp = waveformParam - int(waveformParam);
    if(int(waveformParam)%2 == 0){
        val = val1 * (1-waveInterp) + val2 * waveInterp;
    }
    else{
        val = val1 * waveInterp + val2 * (1-waveInterp);
    }
    
    out_color = vec4(val, linPhase, pastRandom, newRandom);
}
)"
