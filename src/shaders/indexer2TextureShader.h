R"(
#version 410
#define M_PI 3.1415926535897932384626433832795

uniform ivec2 size;
uniform ivec2 resolution;
uniform float time;
uniform float createRandoms;

uniform samplerBuffer parameters;

uniform samplerBuffer indexRandomValues;

int indexNumWavesPosition = 0;
int indexInvertPosition = 1;
int indexSymmetryPosition = 5;
int indexRandomPosition = 2;
int indexOffsetPosition = 3;
int indexQuantizationPosition = 6;
int indexCombinationPosition = 4;
int indexModuloPosition = 7;
int sizePosition = 8;

out vec4 out_color;

float map(float value, float istart, float istop, float ostart, float ostop) {
    return ostart + (ostop - ostart) * ((value - istart) / (istop - istart));
}

void main(){
    //we grab the x and y and store them in an int
    int xVal = int(gl_FragCoord.x);
    int yVal = int(gl_FragCoord.y);
    int width = size.x;
    int height = size.y;
    int dimensionsSum = resolution.x+resolution.y;
    
    //Compute Index
    float x = xVal-(float(width)/2);
    float y = yVal-(float(height)/2);
    xVal = int((sqrt((x * x) + (y * y)) / sqrt((width*width)+(height*height))) * resolution.x * 2);
    float ang;
    if(x > 0 && y >= 0){
        ang = atan(y/x);
    }else if(x == 0 && y > 0){
        ang = M_PI / 2;
    }else if(x < 0){
        ang = atan(y/x) + M_PI;
    }else if(x == 0 && y < 0){
        ang = 3*M_PI / 2;
    }else{
        ang = atan(y/x) + (2*M_PI);
    }
    yVal = int((ang / (2*M_PI)) * resolution.y);
    
    int xIndex = xVal;
    int yIndex = yVal;
    
    int xQuantization = int(texelFetch(parameters, yVal + (dimensionsSum*indexQuantizationPosition)).r);
    int yQuantization = int(texelFetch(parameters, xVal + (dimensionsSum*indexQuantizationPosition) + resolution.y).r);
    int xSymmetry = int(texelFetch(parameters, yVal + (dimensionsSum*indexSymmetryPosition)).r);
    int ySymmetry = int(texelFetch(parameters, xVal + (dimensionsSum*indexSymmetryPosition)+ resolution.y).r);
    float xIndexOffset = texelFetch(parameters, yVal + (dimensionsSum*indexOffsetPosition)).r;
    float yIndexOffset = texelFetch(parameters, xVal + (dimensionsSum*indexOffsetPosition) + resolution.y).r;
    float xIndexRandom = texelFetch(parameters, yVal + (dimensionsSum*indexRandomPosition)).r;
    float yIndexRandom = texelFetch(parameters, xVal + (dimensionsSum*indexRandomPosition) + resolution.y).r;
    float xIndexCombination = texelFetch(parameters, yVal + (dimensionsSum*indexCombinationPosition)).r;
    float yIndexCombination = texelFetch(parameters, xVal + (dimensionsSum*indexCombinationPosition) + resolution.y).r;
    int xIndexModulo = int(texelFetch(parameters, yVal + (dimensionsSum*indexModuloPosition)).r);
    int yIndexModulo = int(texelFetch(parameters, xVal + (dimensionsSum*indexModuloPosition) + resolution.y).r);
    float xNumWaves = texelFetch(parameters, yVal + (dimensionsSum*indexNumWavesPosition)).r;
    float yNumWaves = texelFetch(parameters, xVal + (dimensionsSum*indexNumWavesPosition) + resolution.y).r;
    float xInvert = texelFetch(parameters, yVal + (dimensionsSum*indexInvertPosition)).r;
    float yInvert = texelFetch(parameters, xVal + (dimensionsSum*indexInvertPosition) + resolution.y).r;
    int widthItem = int(texelFetch(parameters, yVal + (dimensionsSum*sizePosition)).r);
    int heightItem = int(texelFetch(parameters, xVal + (dimensionsSum*sizePosition) + height).r);
    
    //Offset
    //xIndex = int(mod((xIndex - round(xIndexOffset)), widthItem));
    //yIndex = int(mod((yIndex - round(yIndexOffset)), heightItem));
//    float x = xVal-(float(width)/2);
//    float y = yVal-(float(height)/2);
//    xVal = int((sqrt((x * x) + (y * y)) / sqrt((width*width)+(height*height))) * resolution.x);
//    float ang;
//    if(x > 0 && y >= 0){
//        ang = atan(y/x);
//    }else if(x == 0 && y > 0){
//        ang = M_PI / 2;
//    }else if(x < 0){
//        ang = atan(y/x) + M_PI;
//    }else if(x == 0 && y < 0){
//        ang = 3*M_PI / 2;
//    }else{
//        ang = atan(y/x) + (2*M_PI);
//    }
//
//
//
//    yVal = int((ang / (2*M_PI)) * resolution.y);

    
    float debug = float(xIndex) / float(resolution.x);
    
    xQuantization = min(xQuantization, resolution.x);
    yQuantization = min(yQuantization, resolution.y);
    
    //Quantization
    xIndex = int(floor(float(xIndex)/(float(resolution.x)/float(xQuantization))));
    yIndex = int(floor(float(yIndex)/(float(resolution.y)/float(yQuantization))));

    
    
    //Symmetry
    while (xSymmetry > xQuantization-1) {
        xSymmetry--;
    }
    
    while (ySymmetry > yQuantization-1) {
        ySymmetry--;
    }
    
    
    //X
    bool odd = false;
    
    if(abs(xIndexOffset) - int(abs(xIndexOffset)) > 0.5){
        //odd = !odd;
    }
    
    if(int((xIndex/(xQuantization/(xSymmetry+1))))%2 == 1){
        odd = true;
    }
    
    int veusSym = int(xQuantization)/int(xSymmetry+1);
    xIndex = veusSym-int(abs(float((xIndex/veusSym)%2) * float(veusSym)-(float(xIndex%veusSym))));
    
    if(xQuantization % 2 == 0){
        if(odd) xIndex += 1;
    }
    else if(xSymmetry > 0){
        xIndex += xInvert < 0.5 ? 0 : 1;
        xIndex %= int(xQuantization);
    }
    
    //Y
    odd = false;
    
    if(abs(yIndexOffset) - int(abs(yIndexOffset)) > 0.5){
        //odd = !odd;
    }
    
    if(int((yIndex/(yQuantization/(ySymmetry+1))))%2 == 1){
        odd = true;
    }
    
    veusSym = int(yQuantization)/int(ySymmetry+1);
    yIndex = veusSym-int(abs(((int(yIndex/veusSym)%2) * veusSym)-(yIndex%veusSym)));
    
    
    if(yQuantization % 2 == 0){
        if(odd) yIndex += 1;
    }
    else if(ySymmetry > 0){
        yIndex += yInvert < 0.5 ? 0 : 1;
        yIndex %= int(yQuantization);
    }
    
    
    //Combination
    xIndex = int(abs(((xIndex%2)*resolution.x*xIndexCombination)-xIndex));
    yIndex = int(abs(((yIndex%2)*resolution.y*yIndexCombination)-yIndex));
    
    //Random
    float xIndexf = float(xIndex)*(1.0-xIndexRandom) + (float((texelFetch(indexRandomValues, xIndex-1).r) +1 ) * xIndexRandom);
    float yIndexf = float(yIndex)*(1.0-yIndexRandom) + (float((texelFetch(indexRandomValues, yIndex + resolution.x-1).r) +1 ) * yIndexRandom);
    
    //Invert
    float nonInvertIndex = (xIndexf-1.0);
    float invertedIndex = ((float(xQuantization)/(xSymmetry+1))-xIndexf);
    xIndexf = (xInvert*invertedIndex + (1-xInvert)*nonInvertIndex);
    
    nonInvertIndex = float(yIndexf-1);
    invertedIndex = ((float(yQuantization)/(ySymmetry+1))-float(yIndexf));
    yIndexf = (yInvert*invertedIndex + (1-yInvert)*nonInvertIndex);
    
    
    //Modulo
    if(xIndexModulo != resolution.x)
        xIndexf = mod(xIndexf, xIndexModulo);
    if(yIndexModulo != resolution.y)
        yIndexf = mod(yIndexf, yIndexModulo);
    
    xIndexf = ((float(xIndexf)/float(resolution.x)))*(xNumWaves)*(float(resolution.x)/float(xQuantization))*(xSymmetry+1);
    yIndexf = ((float(yIndexf)/float(resolution.y)))*(yNumWaves)*(float(resolution.y)/float(yQuantization))*(ySymmetry+1);
    
    
    float index = xIndexf + yIndexf;
    
    float val = mod(index, 1);
    
    out_color = vec4(val, val, val, 1);
//    out_color = vec4(debug, debug, debug, 1);
}
)"
