//
//  oscillatorTexture.cpp
//  MIRABCN_Generator
//
//  Created by Eduard Frigola on 24/01/2018.
//
//

#include "oscillatorTexture.h"
#include <random>

#define STRINGIFY(x) #x

oscillatorTexture::oscillatorTexture() : ofxOceanodeNodeModel("Oscillator Texture"){
    resources = &sharedResources::getInstance();
    
    parameters->add(phasorIn.set("Phasor In", 0, 0, 1));
    addParameterToGroupAndInfo(width.set("Tex Width", 100, 1, 5120)).acceptInConnection = false;
    addParameterToGroupAndInfo(height.set("Tex Height", 100, 1, 2880)).acceptInConnection = false;
    
    previousWidth = width;
    previousHeight = height;
    
    width.addListener(this, &oscillatorTexture::sizeChanged);
    height.addListener(this, &oscillatorTexture::sizeChanged);
    
    
    fbo.allocate(width, height, GL_RGBA32F);
    fbo.begin();
    ofClear(255, 255, 255, 0);
    fbo.end();
    fbo.getTexture().setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
    
    fboBuffer.allocate(width, height, GL_RGBA32F);
    fboBuffer.begin();
    ofClear(255, 255, 255, 0);
    fboBuffer.end();
    fboBuffer.getTexture().setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);

    
    auto setAndBindXYParamsVecFloat = [this](ofParameter<vector<float>> *p, string name, float val, float min, float max) -> void{
        parameters->add(p[0].set(name + " X", vector<float>(1, val), vector<float>(1, min), vector<float>(1, max)));
        parameters->add(p[1].set(name + " Y", vector<float>(1, val), vector<float>(1, min), vector<float>(1, max)));
    };
    
    auto setAndBindXYParamsVecInt = [this](ofParameter<vector<int>> *p, string name, int val, int min, int max) -> void{
        parameters->add(p[0].set(name + " X", vector<int>(1, val), vector<int>(1, min), vector<int>(1, max)));
        parameters->add(p[1].set(name + " Y", vector<int>(1, val), vector<int>(1, min), vector<int>(1, max)));
    };
    
    
    parameters->add(indexNumWaves[0].set("Num Waves X", vector<float>(1, 1), vector<float>(1, 0), vector<float>(1, width)));
    parameters->add(indexNumWaves[1].set("Num Waves Y", vector<float>(1, 1), vector<float>(1, 0), vector<float>(1, height)));
    
    setAndBindXYParamsVecFloat(indexInvert, "Index Invert", 1, -1, 1);
    
    parameters->add(indexSymmetry[0].set("Symmetry X", vector<int>(1, 0), vector<int>(1, 0), vector<int>(1, width/2)));
    parameters->add(indexSymmetry[1].set("Symmetry Y", vector<int>(1, 0), vector<int>(1, 0), vector<int>(1, height/2)));
    
    setAndBindXYParamsVecFloat(indexRandom, "Index Random", 0, 0, 1);
    
    parameters->add(indexOffset[0].set("Index Offset X", vector<float>(1, 0), vector<float>(1, -width/2), vector<float>(1, width/2)));
    parameters->add(indexOffset[1].set("Index Offset Y", vector<float>(1, 0), vector<float>(1, -height/2), vector<float>(1, height/2)));
    
    parameters->add(indexQuantization[0].set("Index Quatization X", vector<int>(1, width), vector<int>(1, 1), vector<int>(1, width)));
    parameters->add(indexQuantization[1].set("Index Quatization Y", vector<int>(1, height), vector<int>(1, 1), vector<int>(1, height)));
    
    setAndBindXYParamsVecFloat(indexCombination, "Index Combination", 0, 0, 1);
    
    parameters->add(indexModulo[0].set("Index Modulo X", vector<int>(1, width), vector<int>(1, 1), vector<int>(1, width)));
    parameters->add(indexModulo[1].set("Index Modulo Y", vector<int>(1, height), vector<int>(1, 1), vector<int>(1, height)));
    
    setAndBindXYParamsVecFloat(phaseOffset, "Phase Offset", 0, 0, 1);
    
    //scaling shader vars
    setAndBindXYParamsVecFloat(randomAddition, "Rnd Add", 0, -1, 1);
    setAndBindXYParamsVecFloat(scale, "Scale", 1, 0, 2);
    setAndBindXYParamsVecFloat(offset, "Offset", 0, -1, 1);
    setAndBindXYParamsVecFloat(pow, "Pow", 0, -40, 40);
    setAndBindXYParamsVecFloat(bipow, "Bi Pow", 0, -40, 40);
    setAndBindXYParamsVecInt(quantization, "Quantize", 255, 2, 255);
    setAndBindXYParamsVecFloat(pulseWidth, "Pulse Width", 1, 0, 1);
    setAndBindXYParamsVecFloat(skew, "Skew", 0, -1, 1);
    setAndBindXYParamsVecFloat(fader, "Fader", 1, 0, 1);
    setAndBindXYParamsVecFloat(invert, "Invert", 0, 0, 1);
    setAndBindXYParamsVecFloat(waveform, "WaveForm", 0, 0, 8);
    
    setParametersInfoMaps();
    
    parameters->add(createDropdownAbstractParameter("Wave Select", {"sin", "cos", "tri", "square", "saw", "inverted saw", "rand1", "rand2", "rand3"}, waveSelect_Param));
    
    parameters->add(oscillatorOut.set("Oscillator Out", nullptr));
    
    
    //TBOs
    
    //OSCILLATOR SHADER INT
    oscillatorShaderIntBuffer.allocate();
    oscillatorShaderIntBuffer.bind(GL_TEXTURE_BUFFER);
    setOscillatorShaderIntParameterDataToTBO();
    oscillatorShaderIntTexture.allocateAsBufferTexture(oscillatorShaderIntBuffer, GL_R32UI);
    
    //OSCILLATOR SHADER FLOAT
    oscillatorShaderFloatBuffer.allocate();
    oscillatorShaderFloatBuffer.bind(GL_TEXTURE_BUFFER);
    setOscillatorShaderFloatParameterDataToTBO();
    oscillatorShaderFloatTexture.allocateAsBufferTexture(oscillatorShaderFloatBuffer, GL_R32F);
    
    
    //SCALING SHADER INT
    scalingShaderIntBuffer.allocate();
    scalingShaderIntBuffer.bind(GL_TEXTURE_BUFFER);
    setScalingShaderIntParameterDataToTBO();
    scalingShaderIntTexture.allocateAsBufferTexture(scalingShaderIntBuffer, GL_R32UI);
    
    
    //SCALING SHADER FLOAT
    scalingShaderFloatBuffer.allocate();
    scalingShaderFloatBuffer.bind(GL_TEXTURE_BUFFER);
    setScalingShaderFloatParameterDataToTBO();
    scalingShaderFloatTexture.allocateAsBufferTexture(scalingShaderFloatBuffer, GL_R32F);
    
    
    //IndexRandomValues
    indexRandomValuesBuffer.allocate();
    indexRandomValuesBuffer.bind(GL_TEXTURE_BUFFER);
    indexRandomValuesBuffer.setData(newRandomValuesVector(), GL_STREAM_DRAW);
    indexRandomValuesTexture.allocateAsBufferTexture(indexRandomValuesBuffer, GL_R32F);
    
    //LoadShader
    bool b = true;
    loadShader(b);
    
    
    //listeners
    
    phasorIn.addListener(this, &oscillatorTexture::newPhasorIn);
    
    oscillatorShaderFloatListeners.push(indexNumWaves[0].newListener([&](vector<float> &vf){
        onOscillatorShaderFloatParameterChanged(indexNumWaves[0], vf);
    }));
    oscillatorShaderFloatListeners.push(indexNumWaves[1].newListener([&](vector<float> &vf){
        onOscillatorShaderFloatParameterChanged(indexNumWaves[1], vf);
    }));
    
    oscillatorShaderFloatListeners.push(indexInvert[0].newListener([&](vector<float> &vf){
        onOscillatorShaderFloatParameterChanged(indexInvert[0], vf);
    }));
    oscillatorShaderFloatListeners.push(indexInvert[1].newListener([&](vector<float> &vf){
        onOscillatorShaderFloatParameterChanged(indexInvert[1], vf);
    }));
    
    oscillatorShaderIntListeners.push(indexSymmetry[0].newListener([&](vector<int> &vi){
        onOscillatorShaderIntParameterChanged(indexSymmetry[0], vi);
    }));
    oscillatorShaderIntListeners.push(indexSymmetry[1].newListener([&](vector<int> &vi){
        onOscillatorShaderIntParameterChanged(indexSymmetry[1], vi);
    }));
    
    oscillatorShaderFloatListeners.push(indexRandom[0].newListener([&](vector<float> &vf){
        onOscillatorShaderFloatParameterChanged(indexRandom[0], vf);
    }));
    oscillatorShaderFloatListeners.push(indexRandom[1].newListener([&](vector<float> &vf){
        onOscillatorShaderFloatParameterChanged(indexRandom[1], vf);
    }));
    
    oscillatorShaderFloatListeners.push(indexOffset[0].newListener([&](vector<float> &vf){
        onOscillatorShaderFloatParameterChanged(indexOffset[0], vf);
    }));
    oscillatorShaderFloatListeners.push(indexOffset[1].newListener([&](vector<float> &vf){
        onOscillatorShaderFloatParameterChanged(indexOffset[1], vf);
    }));
    
    oscillatorShaderIntListeners.push(indexQuantization[0].newListener([&](vector<int> &vi){
        onOscillatorShaderIntParameterChanged(indexQuantization[0], vi);
    }));
    oscillatorShaderIntListeners.push(indexQuantization[1].newListener([&](vector<int> &vi){
        onOscillatorShaderIntParameterChanged(indexQuantization[1], vi);
    }));
    
    oscillatorShaderFloatListeners.push(indexCombination[0].newListener([&](vector<float> &vf){
        onOscillatorShaderFloatParameterChanged(indexCombination[0], vf);
    }));
    oscillatorShaderFloatListeners.push(indexCombination[1].newListener([&](vector<float> &vf){
        onOscillatorShaderFloatParameterChanged(indexCombination[1], vf);
    }));
    
    oscillatorShaderIntListeners.push(indexModulo[0].newListener([&](vector<int> &vi){
        onOscillatorShaderIntParameterChanged(indexModulo[0], vi);
    }));
    oscillatorShaderIntListeners.push(indexModulo[1].newListener([&](vector<int> &vi){
        onOscillatorShaderIntParameterChanged(indexModulo[1], vi);
    }));
    
    oscillatorShaderFloatListeners.push(phaseOffset[0].newListener([&](vector<float> &vf){
        onOscillatorShaderFloatParameterChanged(phaseOffset[0], vf);
    }));
    oscillatorShaderFloatListeners.push(phaseOffset[1].newListener([&](vector<float> &vf){
        onOscillatorShaderFloatParameterChanged(phaseOffset[1], vf);
    }));
    
    scalingShaderFloatListeners.push(randomAddition[0].newListener([&](vector<float> &vf){
        onScalingShaderFloatParameterChanged(randomAddition[0], vf);
    }));
    scalingShaderFloatListeners.push(randomAddition[1].newListener([&](vector<float> &vf){
        onScalingShaderFloatParameterChanged(randomAddition[1], vf);
    }));
    
    scalingShaderFloatListeners.push(scale[0].newListener([&](vector<float> &vf){
        onScalingShaderFloatParameterChanged(scale[0], vf);
    }));
    scalingShaderFloatListeners.push(scale[1].newListener([&](vector<float> &vf){
        onScalingShaderFloatParameterChanged(scale[1], vf);
    }));
    
    scalingShaderFloatListeners.push(offset[0].newListener([&](vector<float> &vf){
        onScalingShaderFloatParameterChanged(offset[0], vf);
    }));
    scalingShaderFloatListeners.push(offset[1].newListener([&](vector<float> &vf){
        onScalingShaderFloatParameterChanged(offset[1], vf);
    }));
    
    scalingShaderFloatListeners.push(pow[0].newListener([&](vector<float> &vf){
        onScalingShaderFloatParameterChanged(pow[0], vf);
    }));
    scalingShaderFloatListeners.push(pow[1].newListener([&](vector<float> &vf){
        onScalingShaderFloatParameterChanged(pow[1], vf);
    }));
    
    scalingShaderFloatListeners.push(bipow[0].newListener([&](vector<float> &vf){
        onScalingShaderFloatParameterChanged(bipow[0], vf);
    }));
    scalingShaderFloatListeners.push(bipow[1].newListener([&](vector<float> &vf){
        onScalingShaderFloatParameterChanged(bipow[1], vf);
    }));
    
    scalingShaderIntListeners.push(quantization[0].newListener([&](vector<int> &vi){
        onScalingShaderIntParameterChanged(quantization[0], vi);
    }));
    scalingShaderIntListeners.push(quantization[1].newListener([&](vector<int> &vi){
        onScalingShaderIntParameterChanged(quantization[1], vi);
    }));
    
    oscillatorShaderFloatListeners.push(pulseWidth[0].newListener([&](vector<float> &vf){
        onOscillatorShaderFloatParameterChanged(pulseWidth[0], vf);
    }));
    oscillatorShaderFloatListeners.push(pulseWidth[1].newListener([&](vector<float> &vf){
        onOscillatorShaderFloatParameterChanged(pulseWidth[1], vf);
    }));
    
    oscillatorShaderFloatListeners.push(skew[0].newListener([&](vector<float> &vf){
        onOscillatorShaderFloatParameterChanged(skew[0], vf);
    }));
    oscillatorShaderFloatListeners.push(skew[1].newListener([&](vector<float> &vf){
        onOscillatorShaderFloatParameterChanged(skew[1], vf);
    }));
    
    scalingShaderFloatListeners.push(fader[0].newListener([&](vector<float> &vf){
        onScalingShaderFloatParameterChanged(fader[0], vf);
    }));
    scalingShaderFloatListeners.push(fader[1].newListener([&](vector<float> &vf){
        onScalingShaderFloatParameterChanged(fader[1], vf);
    }));
    
    scalingShaderFloatListeners.push(invert[0].newListener([&](vector<float> &vf){
        onScalingShaderFloatParameterChanged(invert[0], vf);
    }));
    scalingShaderFloatListeners.push(invert[1].newListener([&](vector<float> &vf){
        onScalingShaderFloatParameterChanged(invert[1], vf);
    }));
    
    oscillatorShaderFloatListeners.push(waveform[0].newListener([&](vector<float> &vf){
        onOscillatorShaderFloatParameterChanged(waveform[0], vf);
    }));
    oscillatorShaderFloatListeners.push(waveform[1].newListener([&](vector<float> &vf){
        onOscillatorShaderFloatParameterChanged(waveform[1], vf);
    }));
    
    waveSelect_Param.addListener(this, &oscillatorTexture::newWaveSelectParam);
}

oscillatorTexture::~oscillatorTexture(){
    resources->makeTextureLocationAvailable(oscillatorShaderIntParametersTextureLocation);
    resources->makeTextureLocationAvailable(oscillatorShaderFloatParametersTextureLocation);
    resources->makeTextureLocationAvailable(scalingShaderIntParametersTextureLocation);
    resources->makeTextureLocationAvailable(scalingShaderFloatParametersTextureLocation);
    resources->makeTextureLocationAvailable(randomIndexsTextureLocation);
    resources->makeTextureLocationAvailable(randomInfoOscillatorShaderTextureLocation);
    resources->makeTextureLocationAvailable(randomInfoScalingShaderTextureLocation);
}

void oscillatorTexture::setParametersInfoMaps(){
    int dimensionsSum = width+height;
    oscillatorShaderFloatParameterNameTBOPositionMap[indexNumWaves[0].getName()] = 0;
    oscillatorShaderFloatParameterNameTBOPositionMap[indexNumWaves[1].getName()] = height;
    oscillatorShaderParameterNameTBOSizeMap[indexNumWaves[0].getName()] = height;
    oscillatorShaderParameterNameTBOSizeMap[indexNumWaves[1].getName()] = width;
    
    oscillatorShaderFloatParameterNameTBOPositionMap[indexInvert[0].getName()] = dimensionsSum;
    oscillatorShaderFloatParameterNameTBOPositionMap[indexInvert[1].getName()] = dimensionsSum + height;
    oscillatorShaderParameterNameTBOSizeMap[indexInvert[0].getName()] = height;
    oscillatorShaderParameterNameTBOSizeMap[indexInvert[1].getName()] = width;
    
    oscillatorShaderIntParameterNameTBOPositionMap[indexSymmetry[0].getName()] = 0;
    oscillatorShaderIntParameterNameTBOPositionMap[indexSymmetry[1].getName()] = height;
    oscillatorShaderParameterNameTBOSizeMap[indexSymmetry[0].getName()] = height;
    oscillatorShaderParameterNameTBOSizeMap[indexSymmetry[1].getName()] = width;
    
    oscillatorShaderFloatParameterNameTBOPositionMap[indexRandom[0].getName()] = dimensionsSum * 2;
    oscillatorShaderFloatParameterNameTBOPositionMap[indexRandom[1].getName()] = (dimensionsSum * 2) + height;
    oscillatorShaderParameterNameTBOSizeMap[indexRandom[0].getName()] = height;
    oscillatorShaderParameterNameTBOSizeMap[indexRandom[1].getName()] = width;
    
    oscillatorShaderFloatParameterNameTBOPositionMap[indexOffset[0].getName()] = dimensionsSum * 3;
    oscillatorShaderFloatParameterNameTBOPositionMap[indexOffset[1].getName()] = (dimensionsSum * 3) + height;
    oscillatorShaderParameterNameTBOSizeMap[indexOffset[0].getName()] = height;
    oscillatorShaderParameterNameTBOSizeMap[indexOffset[1].getName()] = width;
    
    oscillatorShaderIntParameterNameTBOPositionMap[indexQuantization[0].getName()] = dimensionsSum;
    oscillatorShaderIntParameterNameTBOPositionMap[indexQuantization[1].getName()] = (dimensionsSum) + height;
    oscillatorShaderParameterNameTBOSizeMap[indexQuantization[0].getName()] = height;
    oscillatorShaderParameterNameTBOSizeMap[indexQuantization[1].getName()] = width;
    
    oscillatorShaderFloatParameterNameTBOPositionMap[indexCombination[0].getName()] = dimensionsSum * 4;
    oscillatorShaderFloatParameterNameTBOPositionMap[indexCombination[1].getName()] = (dimensionsSum * 4) + height;
    oscillatorShaderParameterNameTBOSizeMap[indexCombination[0].getName()] = height;
    oscillatorShaderParameterNameTBOSizeMap[indexCombination[1].getName()] = width;
    
    oscillatorShaderIntParameterNameTBOPositionMap[indexModulo[0].getName()] = dimensionsSum * 2;
    oscillatorShaderIntParameterNameTBOPositionMap[indexModulo[1].getName()] = (dimensionsSum * 2) + height;
    oscillatorShaderParameterNameTBOSizeMap[indexModulo[0].getName()] = height;
    oscillatorShaderParameterNameTBOSizeMap[indexModulo[1].getName()] = width;

    oscillatorShaderFloatParameterNameTBOPositionMap[phaseOffset[0].getName()] = dimensionsSum * 5;
    oscillatorShaderFloatParameterNameTBOPositionMap[phaseOffset[1].getName()] = (dimensionsSum * 5) + width;
    oscillatorShaderParameterNameTBOSizeMap[phaseOffset[0].getName()] = width;
    oscillatorShaderParameterNameTBOSizeMap[phaseOffset[1].getName()] = height;
    
    scalingShaderFloatParameterNameTBOPositionMap[randomAddition[0].getName()] = 0;
    scalingShaderFloatParameterNameTBOPositionMap[randomAddition[1].getName()] = width;
    scalingShaderParameterNameTBOSizeMap[randomAddition[0].getName()] = width;
    scalingShaderParameterNameTBOSizeMap[randomAddition[1].getName()] = height;
    
    scalingShaderFloatParameterNameTBOPositionMap[scale[0].getName()] = dimensionsSum;
    scalingShaderFloatParameterNameTBOPositionMap[scale[1].getName()] = (dimensionsSum) + width;
    scalingShaderParameterNameTBOSizeMap[scale[0].getName()] = width;
    scalingShaderParameterNameTBOSizeMap[scale[1].getName()] = height;
    
    scalingShaderFloatParameterNameTBOPositionMap[offset[0].getName()] = dimensionsSum * 2;
    scalingShaderFloatParameterNameTBOPositionMap[offset[1].getName()] = (dimensionsSum * 2) + width;
    scalingShaderParameterNameTBOSizeMap[offset[0].getName()] = width;
    scalingShaderParameterNameTBOSizeMap[offset[1].getName()] = height;
    
    scalingShaderFloatParameterNameTBOPositionMap[pow[0].getName()] = dimensionsSum * 3;
    scalingShaderFloatParameterNameTBOPositionMap[pow[1].getName()] = (dimensionsSum * 3) + width;
    scalingShaderParameterNameTBOSizeMap[pow[0].getName()] = width;
    scalingShaderParameterNameTBOSizeMap[pow[1].getName()] = height;
    
    scalingShaderFloatParameterNameTBOPositionMap[bipow[0].getName()] = dimensionsSum * 4;
    scalingShaderFloatParameterNameTBOPositionMap[bipow[1].getName()] = (dimensionsSum * 4) + width;
    scalingShaderParameterNameTBOSizeMap[bipow[0].getName()] = width;
    scalingShaderParameterNameTBOSizeMap[bipow[1].getName()] = height;
    
    scalingShaderIntParameterNameTBOPositionMap[quantization[0].getName()] = 0;
    scalingShaderIntParameterNameTBOPositionMap[quantization[1].getName()] = width;
    scalingShaderParameterNameTBOSizeMap[quantization[0].getName()] = width;
    scalingShaderParameterNameTBOSizeMap[quantization[1].getName()] = height;
    
    oscillatorShaderFloatParameterNameTBOPositionMap[pulseWidth[0].getName()] = dimensionsSum * 6;
    oscillatorShaderFloatParameterNameTBOPositionMap[pulseWidth[1].getName()] = (dimensionsSum * 6) + width;
    oscillatorShaderParameterNameTBOSizeMap[pulseWidth[0].getName()] = width;
    oscillatorShaderParameterNameTBOSizeMap[pulseWidth[1].getName()] = height;
    
    oscillatorShaderFloatParameterNameTBOPositionMap[skew[0].getName()] = dimensionsSum * 7;
    oscillatorShaderFloatParameterNameTBOPositionMap[skew[1].getName()] = (dimensionsSum * 7) + width;
    oscillatorShaderParameterNameTBOSizeMap[skew[0].getName()] = width;
    oscillatorShaderParameterNameTBOSizeMap[skew[1].getName()] = height;
    
    scalingShaderFloatParameterNameTBOPositionMap[fader[0].getName()] = dimensionsSum * 5;
    scalingShaderFloatParameterNameTBOPositionMap[fader[1].getName()] = (dimensionsSum * 5) + width;
    scalingShaderParameterNameTBOSizeMap[fader[0].getName()] = width;
    scalingShaderParameterNameTBOSizeMap[fader[1].getName()] = height;
    
    scalingShaderFloatParameterNameTBOPositionMap[invert[0].getName()] = dimensionsSum * 6;
    scalingShaderFloatParameterNameTBOPositionMap[invert[1].getName()] = (dimensionsSum * 6) + width;
    scalingShaderParameterNameTBOSizeMap[invert[0].getName()] = width;
    scalingShaderParameterNameTBOSizeMap[invert[1].getName()] = height;
    
    oscillatorShaderFloatParameterNameTBOPositionMap[waveform[0].getName()] = dimensionsSum * 8;
    oscillatorShaderFloatParameterNameTBOPositionMap[waveform[1].getName()] = (dimensionsSum * 8) + width;
    oscillatorShaderParameterNameTBOSizeMap[waveform[0].getName()] = width;
    oscillatorShaderParameterNameTBOSizeMap[waveform[1].getName()] = height;
}

void oscillatorTexture::setOscillatorShaderIntParameterDataToTBO(){
    vector<int> accumulateParametersOscillatorShaderIntParameters;
    
    vector<int> indexSymmetryX_tempVec(height, indexSymmetry[0].get()[0]);
    accumulateParametersOscillatorShaderIntParameters.insert(accumulateParametersOscillatorShaderIntParameters.end(), indexSymmetryX_tempVec.begin(), indexSymmetryX_tempVec.end());
    vector<int> indexSymmetryY_tempVec(width, indexSymmetry[1].get()[0]);
    accumulateParametersOscillatorShaderIntParameters.insert(accumulateParametersOscillatorShaderIntParameters.end(), indexSymmetryY_tempVec.begin(), indexSymmetryY_tempVec.end());
    
    vector<int> indexQuantizationX_tempVec(height, indexQuantization[0].get()[0]);
    accumulateParametersOscillatorShaderIntParameters.insert(accumulateParametersOscillatorShaderIntParameters.end(), indexQuantizationX_tempVec.begin(), indexQuantizationX_tempVec.end());
    vector<int> indexQuantizationY_tempVec(width, indexQuantization[1].get()[0]);
    accumulateParametersOscillatorShaderIntParameters.insert(accumulateParametersOscillatorShaderIntParameters.end(), indexQuantizationY_tempVec.begin(), indexQuantizationY_tempVec.end());
    
    vector<int> indexModuloX_tempVec(height, indexModulo[0].get()[0]);
    accumulateParametersOscillatorShaderIntParameters.insert(accumulateParametersOscillatorShaderIntParameters.end(), indexModuloX_tempVec.begin(), indexModuloX_tempVec.end());
    vector<int> indexModuloY_tempVec(width, indexModulo[1].get()[0]);
    accumulateParametersOscillatorShaderIntParameters.insert(accumulateParametersOscillatorShaderIntParameters.end(), indexModuloY_tempVec.begin(), indexModuloY_tempVec.end());

    oscillatorShaderIntBuffer.setData(accumulateParametersOscillatorShaderIntParameters, GL_STREAM_DRAW);
}

void oscillatorTexture::setOscillatorShaderFloatParameterDataToTBO(){
    vector<float> accumulateParametersOscillatorShaderFloatParameters;
    
    vector<float> indexNumWavesX_tempVec(height, indexNumWaves[0].get()[0]);
    accumulateParametersOscillatorShaderFloatParameters.insert(accumulateParametersOscillatorShaderFloatParameters.end(), indexNumWavesX_tempVec.begin(), indexNumWavesX_tempVec.end());
    vector<float> indexNumWavesY_tempVec(width, indexNumWaves[1].get()[0]);
    accumulateParametersOscillatorShaderFloatParameters.insert(accumulateParametersOscillatorShaderFloatParameters.end(), indexNumWavesY_tempVec.begin(), indexNumWavesY_tempVec.end());
    
    vector<float> indexInvertX_tempVec(height, indexInvert[0].get()[0]);
    accumulateParametersOscillatorShaderFloatParameters.insert(accumulateParametersOscillatorShaderFloatParameters.end(), indexInvertX_tempVec.begin(), indexInvertX_tempVec.end());
    vector<float> indexInvertY_tempVec(width, indexInvert[1].get()[0]);
    accumulateParametersOscillatorShaderFloatParameters.insert(accumulateParametersOscillatorShaderFloatParameters.end(), indexInvertY_tempVec.begin(), indexInvertY_tempVec.end());
    
    vector<float> indexRandomX_tempVec(height, indexRandom[0].get()[0]);
    accumulateParametersOscillatorShaderFloatParameters.insert(accumulateParametersOscillatorShaderFloatParameters.end(), indexRandomX_tempVec.begin(), indexRandomX_tempVec.end());
    vector<float> indexRandomY_tempVec(width, indexRandom[1].get()[0]);
    accumulateParametersOscillatorShaderFloatParameters.insert(accumulateParametersOscillatorShaderFloatParameters.end(), indexRandomY_tempVec.begin(), indexRandomY_tempVec.end());
    
    vector<float> indexOffsetX_tempVec(height, indexOffset[0].get()[0]);
    accumulateParametersOscillatorShaderFloatParameters.insert(accumulateParametersOscillatorShaderFloatParameters.end(), indexOffsetX_tempVec.begin(), indexOffsetX_tempVec.end());
    vector<float> indexOffsetY_tempVec(width, indexOffset[1].get()[0]);
    accumulateParametersOscillatorShaderFloatParameters.insert(accumulateParametersOscillatorShaderFloatParameters.end(), indexOffsetY_tempVec.begin(), indexOffsetY_tempVec.end());
    
    vector<float> indexCombinationX_tempVec(height, indexCombination[0].get()[0]);
    accumulateParametersOscillatorShaderFloatParameters.insert(accumulateParametersOscillatorShaderFloatParameters.end(), indexCombinationX_tempVec.begin(), indexCombinationX_tempVec.end());
    vector<float> indexCombinationY_tempVec(width, indexCombination[1].get()[0]);
    accumulateParametersOscillatorShaderFloatParameters.insert(accumulateParametersOscillatorShaderFloatParameters.end(), indexCombinationY_tempVec.begin(), indexCombinationY_tempVec.end());
    
    vector<float> phaseOffsetX_tempVec(width, phaseOffset[0].get()[0]);
    accumulateParametersOscillatorShaderFloatParameters.insert(accumulateParametersOscillatorShaderFloatParameters.end(), phaseOffsetX_tempVec.begin(), phaseOffsetX_tempVec.end());
    vector<float> phaseOffsetY_tempVec(height, phaseOffset[1].get()[0]);
    accumulateParametersOscillatorShaderFloatParameters.insert(accumulateParametersOscillatorShaderFloatParameters.end(), phaseOffsetY_tempVec.begin(), phaseOffsetY_tempVec.end());
    
    vector<float> pulseWidthX_tempVec(width, pulseWidth[0].get()[0]);
    accumulateParametersOscillatorShaderFloatParameters.insert(accumulateParametersOscillatorShaderFloatParameters.end(), pulseWidthX_tempVec.begin(), pulseWidthX_tempVec.end());
    vector<float> pulseWidthY_tempVec(height, pulseWidth[1].get()[0]);
    accumulateParametersOscillatorShaderFloatParameters.insert(accumulateParametersOscillatorShaderFloatParameters.end(), pulseWidthY_tempVec.begin(), pulseWidthY_tempVec.end());
    
    vector<float> skewX_tempVec(width, skew[0].get()[0]);
    accumulateParametersOscillatorShaderFloatParameters.insert(accumulateParametersOscillatorShaderFloatParameters.end(), skewX_tempVec.begin(), skewX_tempVec.end());
    vector<float> skewY_tempVec(height, skew[1].get()[0]);
    accumulateParametersOscillatorShaderFloatParameters.insert(accumulateParametersOscillatorShaderFloatParameters.end(), skewY_tempVec.begin(), skewY_tempVec.end());
    
    vector<float> waveformX_tempVec(width, waveform[0].get()[0]);
    accumulateParametersOscillatorShaderFloatParameters.insert(accumulateParametersOscillatorShaderFloatParameters.end(), waveformX_tempVec.begin(), waveformX_tempVec.end());
    vector<float> waveformY_tempVec(height, waveform[1].get()[0]);
    accumulateParametersOscillatorShaderFloatParameters.insert(accumulateParametersOscillatorShaderFloatParameters.end(), waveformY_tempVec.begin(), waveformY_tempVec.end());
    
    oscillatorShaderFloatBuffer.setData(accumulateParametersOscillatorShaderFloatParameters, GL_STREAM_DRAW);
}

void oscillatorTexture::setScalingShaderIntParameterDataToTBO(){
    vector<int> accumulateParametersScalingShaderIntParameters;
    
    vector<int> quantizationX_tempVec(width, quantization[0].get()[0]);
    accumulateParametersScalingShaderIntParameters.insert(accumulateParametersScalingShaderIntParameters.end(), quantizationX_tempVec.begin(), quantizationX_tempVec.end());
    vector<int> quantizationY_tempVec(height, quantization[1].get()[0]);
    accumulateParametersScalingShaderIntParameters.insert(accumulateParametersScalingShaderIntParameters.end(), quantizationY_tempVec.begin(), quantizationY_tempVec.end());
    
    scalingShaderIntBuffer.setData(accumulateParametersScalingShaderIntParameters, GL_STREAM_DRAW);
}

void oscillatorTexture::setScalingShaderFloatParameterDataToTBO(){
    vector<float> accumulateParametersScalingShaderFloatParameters;
    
    vector<float> randomAdditionX_tempVec(width, randomAddition[0].get()[0]);
    accumulateParametersScalingShaderFloatParameters.insert(accumulateParametersScalingShaderFloatParameters.end(), randomAdditionX_tempVec.begin(), randomAdditionX_tempVec.end());
    vector<float> randomAdditionY_tempVec(height, randomAddition[1].get()[0]);
    accumulateParametersScalingShaderFloatParameters.insert(accumulateParametersScalingShaderFloatParameters.end(), randomAdditionY_tempVec.begin(), randomAdditionY_tempVec.end());
    
    vector<float> scaleX_tempVec(width, scale[0].get()[0]);
    accumulateParametersScalingShaderFloatParameters.insert(accumulateParametersScalingShaderFloatParameters.end(), scaleX_tempVec.begin(), scaleX_tempVec.end());
    vector<float> scaleY_tempVec(height, scale[1].get()[0]);
    accumulateParametersScalingShaderFloatParameters.insert(accumulateParametersScalingShaderFloatParameters.end(), scaleY_tempVec.begin(), scaleY_tempVec.end());
    
    vector<float> offsetX_tempVec(width, offset[0].get()[0]);
    accumulateParametersScalingShaderFloatParameters.insert(accumulateParametersScalingShaderFloatParameters.end(), offsetX_tempVec.begin(), offsetX_tempVec.end());
    vector<float> offsetY_tempVec(height, offset[1].get()[0]);
    accumulateParametersScalingShaderFloatParameters.insert(accumulateParametersScalingShaderFloatParameters.end(), offsetY_tempVec.begin(), offsetY_tempVec.end());
    
    vector<float> powX_tempVec(width, pow[0].get()[0]);
    accumulateParametersScalingShaderFloatParameters.insert(accumulateParametersScalingShaderFloatParameters.end(), powX_tempVec.begin(), powX_tempVec.end());
    vector<float> powY_tempVec(height, pow[1].get()[0]);
    accumulateParametersScalingShaderFloatParameters.insert(accumulateParametersScalingShaderFloatParameters.end(), powY_tempVec.begin(), powY_tempVec.end());
    
    vector<float> bipowX_tempVec(width, bipow[0].get()[0]);
    accumulateParametersScalingShaderFloatParameters.insert(accumulateParametersScalingShaderFloatParameters.end(), bipowX_tempVec.begin(), bipowX_tempVec.end());
    vector<float> bipowY_tempVec(height, bipow[1].get()[0]);
    accumulateParametersScalingShaderFloatParameters.insert(accumulateParametersScalingShaderFloatParameters.end(), bipowY_tempVec.begin(), bipowY_tempVec.end());
    
    vector<float> faderX_tempVec(width, fader[0].get()[0]);
    accumulateParametersScalingShaderFloatParameters.insert(accumulateParametersScalingShaderFloatParameters.end(), faderX_tempVec.begin(), faderX_tempVec.end());
    vector<float> faderY_tempVec(height, fader[1].get()[0]);
    accumulateParametersScalingShaderFloatParameters.insert(accumulateParametersScalingShaderFloatParameters.end(), faderY_tempVec.begin(), faderY_tempVec.end());
    
    vector<float> invertX_tempVec(width, invert[0].get()[0]);
    accumulateParametersScalingShaderFloatParameters.insert(accumulateParametersScalingShaderFloatParameters.end(), invertX_tempVec.begin(), invertX_tempVec.end());
    vector<float> invertY_tempVec(height, invert[1].get()[0]);
    accumulateParametersScalingShaderFloatParameters.insert(accumulateParametersScalingShaderFloatParameters.end(), invertY_tempVec.begin(), invertY_tempVec.end());
    
    scalingShaderFloatBuffer.setData(accumulateParametersScalingShaderFloatParameters, GL_STREAM_DRAW);
}


void oscillatorTexture::loadShader(bool &b){
    string defaultVertSource =
    #include "defaultVertexShader.h"
    ;

    string oscillatorFragSource =
    #include "oscillatorFragShader.h"
    ;
    
    shaderOscillator.setupShaderFromSource(GL_VERTEX_SHADER, defaultVertSource);
    shaderOscillator.setupShaderFromSource(GL_FRAGMENT_SHADER, oscillatorFragSource);
    shaderOscillator.bindDefaults();
    shaderOscillator.linkProgram();
    
    string scalingFragSource =
    #include "scalingFragShader.h"
    ;
    
    shaderScaling.setupShaderFromSource(GL_VERTEX_SHADER, defaultVertSource);
    shaderScaling.setupShaderFromSource(GL_FRAGMENT_SHADER, scalingFragSource);
    shaderScaling.bindDefaults();
    shaderScaling.linkProgram();
    
    oscillatorShaderIntParametersTextureLocation = resources->getNextAvailableShaderTextureLocation();
    oscillatorShaderFloatParametersTextureLocation = resources->getNextAvailableShaderTextureLocation();
    scalingShaderIntParametersTextureLocation = resources->getNextAvailableShaderTextureLocation();
    scalingShaderFloatParametersTextureLocation = resources->getNextAvailableShaderTextureLocation();
    randomIndexsTextureLocation = resources->getNextAvailableShaderTextureLocation();
    
    shaderOscillator.begin();
    shaderOscillator.setUniformTexture("intParameters", oscillatorShaderIntTexture, oscillatorShaderIntParametersTextureLocation);
    shaderOscillator.setUniformTexture("floatParameters", oscillatorShaderFloatTexture, oscillatorShaderFloatParametersTextureLocation);
    shaderOscillator.setUniformTexture("indexRandomValues", indexRandomValuesTexture, randomIndexsTextureLocation);
    shaderOscillator.end();
    
    shaderScaling.begin();
    shaderScaling.setUniformTexture("intParameters", scalingShaderIntTexture, scalingShaderIntParametersTextureLocation);
    shaderScaling.setUniformTexture("floatParameters", scalingShaderFloatTexture, scalingShaderFloatParametersTextureLocation);
    shaderScaling.end();
    
    randomInfoOscillatorShaderTextureLocation = resources->getNextAvailableShaderTextureLocation();
    randomInfoScalingShaderTextureLocation = resources->getNextAvailableShaderTextureLocation();
}

void oscillatorTexture::presetRecallBeforeSettingParameters(ofJson &json){
    if(json.count(width.getEscapedName()) == 1){
        ofDeserialize(json, width);
    }
    if(json.count(height.getEscapedName()) == 1){
        ofDeserialize(json, height);
    }
}

ofTexture& oscillatorTexture::computeBank(float phasor){
    swap(fbo, fboBuffer);
    
    ofPushStyle();
    ofDisableAlphaBlending();
    ofSetColor(255, 255);
    fboBuffer.begin();
    ofClear(0, 0, 0, 255);
    shaderOscillator.begin();
    shaderOscillator.setUniform1f("phase", phasor);
    shaderOscillator.setUniform1f("time", ofGetElapsedTimef());
    shaderOscillator.setUniformTexture("randomInfo", fbo.getTexture(), randomInfoOscillatorShaderTextureLocation);
    ofDrawRectangle(0, 0, width, height);
    shaderOscillator.end();
    fboBuffer.end();
    
    fbo.begin();
    ofClear(0, 0, 0, 255);
    shaderScaling.begin();
    shaderScaling.setUniform1f("phase", phasor);
    shaderScaling.setUniform1f("time", ofGetElapsedTimef());
    shaderScaling.setUniformTexture("randomInfo", fboBuffer.getTexture(), randomInfoScalingShaderTextureLocation);
    ofDrawRectangle(0, 0, width, height);
    shaderScaling.end();
    fbo.end();
    ofPopStyle();
    
    return fbo.getTexture();
}

void oscillatorTexture::newPhasorIn(float &f){
    oscillatorOut = &computeBank(f);
}

vector<float> oscillatorTexture::newRandomValuesVector(){
    vector<float> randomValuesVecX(width, 0);
    vector<float> randomValuesVecY(height, 0);
    for(int i = 0; i < randomValuesVecX.size(); i++){
        randomValuesVecX[i] = (int)-randomValuesVecX.size()/2 + i;
    }
    for(int i = 0; i < randomValuesVecY.size(); i++){
        randomValuesVecY[i] = (int)-randomValuesVecY.size()/2 + i;
    }
    
    mt19937 g(static_cast<uint32_t>(time(0)));
    shuffle(randomValuesVecX.begin(), randomValuesVecX.end(), g);
    shuffle(randomValuesVecY.begin(), randomValuesVecY.end(), g);
    
    randomValuesVecX.insert(randomValuesVecX.end(), randomValuesVecY.begin(), randomValuesVecY.end());
    
    return randomValuesVecX;
}


#pragma mark Parameter Listeners
void oscillatorTexture::onOscillatorShaderIntParameterChanged(ofAbstractParameter &p, vector<int> &vi){
    int position = oscillatorShaderIntParameterNameTBOPositionMap[p.getName()];
    int size = oscillatorShaderParameterNameTBOSizeMap[p.getName()];
    if(vi.size() == size){
        oscillatorShaderIntBuffer.updateData(position*4, vi);
    }else{
        oscillatorShaderIntBuffer.updateData(position*4, vector<int>(size, vi[0]));
    }
}

void oscillatorTexture::onOscillatorShaderFloatParameterChanged(ofAbstractParameter &p, vector<float> &vf){
    int position = oscillatorShaderFloatParameterNameTBOPositionMap[p.getName()];
    int size = oscillatorShaderParameterNameTBOSizeMap[p.getName()];
    if(vf.size() == size){
        oscillatorShaderFloatBuffer.updateData(position*4, vf);
    }else{
        oscillatorShaderFloatBuffer.updateData(position*4, vector<float>(size, vf[0]));
    }
}

void oscillatorTexture::onScalingShaderIntParameterChanged(ofAbstractParameter &p, vector<int> &vi){
    int position = scalingShaderIntParameterNameTBOPositionMap[p.getName()];
    int size = scalingShaderParameterNameTBOSizeMap[p.getName()];
    if(vi.size() == size){
        scalingShaderIntBuffer.updateData(position*4, vi);
    }else{
        scalingShaderIntBuffer.updateData(position*4, vector<int>(size, vi[0]));
    }
}

void oscillatorTexture::onScalingShaderFloatParameterChanged(ofAbstractParameter &p, vector<float> &vf){
    int position = scalingShaderFloatParameterNameTBOPositionMap[p.getName()];
    int size = scalingShaderParameterNameTBOSizeMap[p.getName()];
    if(vf.size() == size){
        scalingShaderFloatBuffer.updateData(position*4, vf);
    }else{
        scalingShaderFloatBuffer.updateData(position*4, vector<float>(size, vf[0]));
    }
}

void oscillatorTexture::newWaveSelectParam(int &i){
    waveform[0] = vector<float>(1, i);
}

void oscillatorTexture::sizeChanged(int &i){
    bool sizeChanged = false;
    if(&i == &width.get()){
        if(width != previousWidth){
            changeMinMaxOfVecParameter(indexNumWaves[0], -1.0f, float(width), false);
            changeMinMaxOfVecParameter(indexSymmetry[0], -1, width/2, false);
            changeMinMaxOfVecParameter(indexOffset[0], float(-width/2), float(width/2), true);
            changeMinMaxOfVecParameter(indexQuantization[0], -1, width.get(), true);
            changeMinMaxOfVecParameter(indexModulo[0], -1, width.get(), true);
            sizeChanged = true;
        }
        previousWidth = width;
    }else{
        if(height != previousHeight){
            changeMinMaxOfVecParameter(indexNumWaves[1], -1.0f, float(height), false);
            changeMinMaxOfVecParameter(indexSymmetry[1], -1, height/2, false);
            changeMinMaxOfVecParameter(indexOffset[1], float(-height/2), float(height/2), true);
            changeMinMaxOfVecParameter(indexQuantization[1], -1, height.get(), true);
            changeMinMaxOfVecParameter(indexModulo[1], -1, height.get(), true);
            sizeChanged = true;
        }
        previousHeight = height;
    }
    
    if(sizeChanged){
        fbo.allocate(width, height, GL_RGBA32F);
        fbo.begin();
        ofClear(255, 255, 255, 0);
        fbo.end();
        fbo.getTexture().setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
        
        fboBuffer.allocate(width, height, GL_RGBA32F);
        fboBuffer.begin();
        ofClear(255, 255, 255, 0);
        fboBuffer.end();
        fboBuffer.getTexture().setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
    
        setParametersInfoMaps();
        setOscillatorShaderIntParameterDataToTBO();
        setOscillatorShaderFloatParameterDataToTBO();
        setScalingShaderIntParameterDataToTBO();
        setScalingShaderFloatParameterDataToTBO();
        
        indexRandomValuesBuffer.setData(newRandomValuesVector(), GL_STREAM_DRAW);
    }
}


