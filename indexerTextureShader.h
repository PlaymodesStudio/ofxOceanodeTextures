R"(
#version 410
#define M_PI 3.1415926535897932384626433832795

uniform ivec2 size;
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
    int dimensionsSum = width+height;
    
    //Compute Index
    int xIndex = xVal;
    int yIndex = yVal;
    int xQuantization = int(texelFetch(parameters, yVal + (dimensionsSum*indexQuantizationPosition)).r);
    int yQuantization = int(texelFetch(parameters, xVal + (dimensionsSum*indexQuantizationPosition) + height).r);
    int xSymmetry = int(texelFetch(parameters, yVal + (dimensionsSum*indexSymmetryPosition)).r);
    int ySymmetry = int(texelFetch(parameters, xVal + (dimensionsSum*indexSymmetryPosition)+ height).r);
    float xIndexOffset = texelFetch(parameters, yVal + (dimensionsSum*indexOffsetPosition)).r;
    float yIndexOffset = texelFetch(parameters, xVal + (dimensionsSum*indexOffsetPosition) + height).r;
    float xIndexRandom = texelFetch(parameters, yVal + (dimensionsSum*indexRandomPosition)).r;
    float yIndexRandom = texelFetch(parameters, xVal + (dimensionsSum*indexRandomPosition) + height).r;
    float xIndexCombination = texelFetch(parameters, yVal + (dimensionsSum*indexCombinationPosition)).r;
    float yIndexCombination = texelFetch(parameters, xVal + (dimensionsSum*indexCombinationPosition) + height).r;
    int xIndexModulo = int(texelFetch(parameters, yVal + (dimensionsSum*indexModuloPosition)).r);
    int yIndexModulo = int(texelFetch(parameters, xVal + (dimensionsSum*indexModuloPosition) + height).r);
    float xNumWaves = texelFetch(parameters, yVal + (dimensionsSum*indexNumWavesPosition)).r;
    float yNumWaves = texelFetch(parameters, xVal + (dimensionsSum*indexNumWavesPosition) + height).r;
    float xInvert = texelFetch(parameters, yVal + (dimensionsSum*indexInvertPosition)).r;
    float yInvert = texelFetch(parameters, xVal + (dimensionsSum*indexInvertPosition) + height).r;
    int widthItem = int(texelFetch(parameters, yVal + (dimensionsSum*sizePosition)).r);
    int heightItem = int(texelFetch(parameters, xVal + (dimensionsSum*sizePosition) + height).r);
    
    //Offset
    xIndex = int(mod((xIndex - round(xIndexOffset)), widthItem));
    yIndex = int(mod((yIndex - round(yIndexOffset)), heightItem));
    
    xQuantization = min(xQuantization, widthItem);
    yQuantization = min(yQuantization, heightItem);
    
    //Quantization
    xIndex = int(floor(float(xIndex)/(float(widthItem)/float(xQuantization))));
    yIndex = int(floor(float(yIndex)/(float(heightItem)/float(yQuantization))));
    
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
        xIndex += xInvert < 1 ? 0 : 1;
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
        yIndex += yInvert < 1 ? 0 : 1;
        yIndex %= int(yQuantization);
    }
    
    
    //Combination
    xIndex = int(abs(((xIndex%2)*widthItem*xIndexCombination)-xIndex));
    yIndex = int(abs(((yIndex%2)*heightItem*yIndexCombination)-yIndex));
    
    //Random
    //    double(index)*(1-indexRand_Param) + (double(indexRand[index-1] + 1)*indexRand_Param)
    float xIndexf = float(xIndex)*(1.0-xIndexRandom) + (float((texelFetch(indexRandomValues, xIndex).r) +1 ) * xIndexRandom);
    float yIndexf = float(yIndex)*(1.0-yIndexRandom) + (float((texelFetch(indexRandomValues, yIndex + width).r) +1 ) * yIndexRandom);
    
    //Invert
    float nonInvertIndex = (xIndexf-1.0);
    float invertedIndex = ((float(xQuantization)/(xSymmetry+1))-xIndexf);
    xIndexf = (map(xInvert, -1, 1, 1, 0)*invertedIndex + (1-map(xInvert, -1, 1, 1, 0))*nonInvertIndex);
    
    nonInvertIndex = float(yIndexf-1);
    invertedIndex = ((float(yQuantization)/(ySymmetry+1))-float(yIndexf));
    yIndexf = (map(yInvert, -1, 1, 1, 0)*invertedIndex + (1-map(yInvert, -1, 1, 1, 0))*nonInvertIndex);
    
    
    //Modulo
    if(xIndexModulo != widthItem)
        xIndexf = mod(xIndexf, xIndexModulo);
    if(yIndexModulo != heightItem)
        yIndexf = mod(yIndexf, yIndexModulo);
    
    xIndexf = ((float(xIndexf)/float(widthItem)))*(xNumWaves)*(float(widthItem)/float(xQuantization))*(xSymmetry+1);
    yIndexf = ((float(yIndexf)/float(heightItem)))*(yNumWaves)*(float(heightItem)/float(yQuantization))*(ySymmetry+1);
    
    
    float index = xIndexf + yIndexf;
    
    float val = mod(index, 1);
    
    out_color = vec4(val, val, val, 1);
}
)"
