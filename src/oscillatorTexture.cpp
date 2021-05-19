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
    isSetup = false;
    isFirstPassAfterSetup = true;
    sizeChanged = false;
    color = ofColor(0, 127, 255);
}

oscillatorTexture::~oscillatorTexture(){
    if(isSetup){
        oscillatorShaderTexture.clear();
    }
}

void oscillatorTexture::setup(){
    addParameter(phasorIn.set("Phase", 0, 0, 1), ofxOceanodeParameterFlags_DisableSavePreset);
    addParameter(widthVec.set("Size.X", {100}, {1}, {5120}));
    addParameter(heightVec.set("Size.Y", {100}, {1}, {2880}));
    width = 100;
    height = 100;
    
    previousWidth = width;
    previousHeight = height;
    
    auto setAndBindXYParamsVecFloat = [this](ofParameter<vector<float>> *p, string name, float val, float min, float max) -> void{
        addParameter(p[0].set(name + ".X", vector<float>(1, val), vector<float>(1, min), vector<float>(1, max)));
        addParameter(p[1].set(name + ".Y", vector<float>(1, val), vector<float>(1, min), vector<float>(1, max)));
    };
    
    auto setAndBindXYParamsVecInt = [this](ofParameter<vector<int>> *p, string name, int val, int min, int max) -> void{
        addParameter(p[0].set(name + ".X", vector<int>(1, val), vector<int>(1, min), vector<int>(1, max)));
        addParameter(p[1].set(name + ".Y", vector<int>(1, val), vector<int>(1, min), vector<int>(1, max)));
    };
    
    addParameter(indexs.set("Indexs", nullptr, nullptr, nullptr));
    
    setAndBindXYParamsVecFloat(phaseOffset, "Ph Off", 0, 0, 1);
    setAndBindXYParamsVecFloat(roundness, "Round", .5, 0, 1);
    setAndBindXYParamsVecFloat(pulseWidth, "PulseW", 0.5, 0, 1);
    setAndBindXYParamsVecFloat(skew, "Skew", 0, -1, 1);
    setAndBindXYParamsVecFloat(randomAddition, "RndAdd", 0, -1, 1);
    setAndBindXYParamsVecFloat(scale, "Scale", 1, 0, 2);
    setAndBindXYParamsVecFloat(offset, "Offset", 0, -1, 1);
    setAndBindXYParamsVecFloat(pow, "Pow", 0, -1, 1);
    setAndBindXYParamsVecFloat(bipow, "BiPow", 0, -1, 1);
    setAndBindXYParamsVecInt(quantization, "Quant", 255, 2, 255);
    setAndBindXYParamsVecFloat(fader, "Fader", 1, 0, 1);
    setAndBindXYParamsVecFloat(invert, "Inv", 0, 0, 1);
    
    setParametersInfoMaps();
    
    addParameter(oscillatorOut.set("Output", nullptr, nullptr, nullptr));
    
    
    //Listeners
    listeners.push(widthVec.newListener([this](vector<int> &vi){
        width = *max_element(vi.begin(), vi.end());
		if(width < 1) width = 1;
        vector<float> vf(vi.begin(), vi.end());
        onOscillatorShaderParameterChanged(widthVec, vf);
    }));
    
    listeners.push(heightVec.newListener([this](vector<int> &vi){
		if(height < 1) height = 1;
        height = *max_element(vi.begin(), vi.end());
        vector<float> vf(vi.begin(), vi.end());
        onOscillatorShaderParameterChanged(heightVec, vf);
    }));
    
    listeners.push(indexs.newListener([this](ofTexture* &tex){
        if(tex != nullptr){
            if(width != tex->getWidth()) widthVec = {(int)tex->getWidth()};
            if(height != tex->getHeight()) heightVec = {(int)tex->getHeight()};
        }
    }));
    
    listeners.push(width.newListener(this, &oscillatorTexture::sizeChangedListener));
    listeners.push(height.newListener(this, &oscillatorTexture::sizeChangedListener));
    
    listeners.push(phasorIn.newListener(this, &oscillatorTexture::newPhasorIn));
    
    oscillatorShaderListeners.push(phaseOffset[0].newListener([&](vector<float> &vf){
        onOscillatorShaderParameterChanged(phaseOffset[0], vf);
    }));
    oscillatorShaderListeners.push(phaseOffset[1].newListener([&](vector<float> &vf){
        onOscillatorShaderParameterChanged(phaseOffset[1], vf);
    }));
    
    oscillatorShaderListeners.push(pulseWidth[0].newListener([&](vector<float> &vf){
        onOscillatorShaderParameterChanged(pulseWidth[0], vf);
    }));
    oscillatorShaderListeners.push(pulseWidth[1].newListener([&](vector<float> &vf){
        onOscillatorShaderParameterChanged(pulseWidth[1], vf);
    }));
    
    oscillatorShaderListeners.push(skew[0].newListener([&](vector<float> &vf){
        onOscillatorShaderParameterChanged(skew[0], vf);
    }));
    oscillatorShaderListeners.push(skew[1].newListener([&](vector<float> &vf){
        onOscillatorShaderParameterChanged(skew[1], vf);
    }));
    
    oscillatorShaderListeners.push(roundness[0].newListener([&](vector<float> &vf){
        onOscillatorShaderParameterChanged(roundness[0], vf);
    }));
    oscillatorShaderListeners.push(roundness[1].newListener([&](vector<float> &vf){
        onOscillatorShaderParameterChanged(roundness[1], vf);
    }));
    
    oscillatorShaderListeners.push(randomAddition[0].newListener([&](vector<float> &vf){
        onOscillatorShaderParameterChanged(randomAddition[0], vf);
    }));
    oscillatorShaderListeners.push(randomAddition[1].newListener([&](vector<float> &vf){
        onOscillatorShaderParameterChanged(randomAddition[1], vf);
    }));
    
    oscillatorShaderListeners.push(scale[0].newListener([&](vector<float> &vf){
        onOscillatorShaderParameterChanged(scale[0], vf);
    }));
    oscillatorShaderListeners.push(scale[1].newListener([&](vector<float> &vf){
        onOscillatorShaderParameterChanged(scale[1], vf);
    }));
    
    oscillatorShaderListeners.push(offset[0].newListener([&](vector<float> &vf){
        onOscillatorShaderParameterChanged(offset[0], vf);
    }));
    oscillatorShaderListeners.push(offset[1].newListener([&](vector<float> &vf){
        onOscillatorShaderParameterChanged(offset[1], vf);
    }));
    
    oscillatorShaderListeners.push(pow[0].newListener([&](vector<float> &vf){
        onOscillatorShaderParameterChanged(pow[0], vf);
    }));
    oscillatorShaderListeners.push(pow[1].newListener([&](vector<float> &vf){
        onOscillatorShaderParameterChanged(pow[1], vf);
    }));
    
    oscillatorShaderListeners.push(bipow[0].newListener([&](vector<float> &vf){
        onOscillatorShaderParameterChanged(bipow[0], vf);
    }));
    oscillatorShaderListeners.push(bipow[1].newListener([&](vector<float> &vf){
        onOscillatorShaderParameterChanged(bipow[1], vf);
    }));
    
    oscillatorShaderListeners.push(quantization[0].newListener([&](vector<int> &vi){
        vector<float> vf(vi.begin(), vi.end());
        onOscillatorShaderParameterChanged(quantization[0], vf);
    }));
    oscillatorShaderListeners.push(quantization[1].newListener([&](vector<int> &vi){
        vector<float> vf(vi.begin(), vi.end());
        onOscillatorShaderParameterChanged(quantization[1], vf);
    }));
    
    oscillatorShaderListeners.push(fader[0].newListener([&](vector<float> &vf){
        onOscillatorShaderParameterChanged(fader[0], vf);
    }));
    oscillatorShaderListeners.push(fader[1].newListener([&](vector<float> &vf){
        onOscillatorShaderParameterChanged(fader[1], vf);
    }));
    
    oscillatorShaderListeners.push(invert[0].newListener([&](vector<float> &vf){
        onOscillatorShaderParameterChanged(invert[0], vf);
    }));
    oscillatorShaderListeners.push(invert[1].newListener([&](vector<float> &vf){
        onOscillatorShaderParameterChanged(invert[1], vf);
    }));
    
    isFirstPassAfterSetup = true;
}

void oscillatorTexture::update(ofEventArgs &a){
    if(!isSetup){
        //Texture Allocation
        ofFbo::Settings settings;
        settings.height = height;
        settings.width = width;
        settings.internalformat = GL_RGBA32F;
        settings.maxFilter = GL_NEAREST;
        settings.minFilter = GL_NEAREST;
        settings.numColorbuffers = 1;
        settings.useDepth = false;
        settings.useStencil = false;
        settings.textureTarget = GL_TEXTURE_2D;
        
        fbo.allocate(settings);
        fbo.begin();
        ofClear(0, 0, 0, 255);
        fbo.end();
        
        fboBuffer.allocate(settings);
        fboBuffer.begin();
        ofClear(0, 0, 0, 255);
        fboBuffer.end();
        
        blackIndexs.allocate(settings);
        blackIndexs.begin();
        ofClear(0, 0, 0, 255);
        blackIndexs.end();
    
        //TBOs
    
        //OSCILLATOR SHADER
        oscillatorShaderBuffer.allocate();
        oscillatorShaderBuffer.bind(GL_TEXTURE_BUFFER);
        setOscillatorShaderParameterDataToTBO();
        oscillatorShaderTexture.allocateAsBufferTexture(oscillatorShaderBuffer, GL_R32F);
        
        //LoadShader
        bool b = true;
        loadShader(b);
        
        isSetup = true;
    }
    
    if(isFirstPassAfterSetup){
        fbo.begin();
        ofClear(0, 0, 0, 255);
        fbo.end();
        
        fboBuffer.begin();
        ofClear(0, 0, 0, 255);
        fboBuffer.end();
        
        blackIndexs.begin();
        ofClear(0, 0, 0, 255);
        blackIndexs.end();
    }
    
    if(sizeChanged){
        ofFbo::Settings settings;
        settings.height = height;
        settings.width = width;
        settings.internalformat = GL_RGBA32F;
        settings.maxFilter = GL_NEAREST;
        settings.minFilter = GL_NEAREST;
        settings.numColorbuffers = 1;
        settings.useDepth = false;
        settings.useStencil = false;
        settings.textureTarget = GL_TEXTURE_2D;
        
        fbo.allocate(settings);
        fbo.begin();
        ofClear(0, 0, 0, 255);
        fbo.end();
        
        fboBuffer.allocate(settings);
        fboBuffer.begin();
        ofClear(0, 0, 0, 255);
        fboBuffer.end();
        
        blackIndexs.allocate(settings);
        blackIndexs.begin();
        ofClear(0, 0, 0, 255);
        blackIndexs.end();
        
        setParametersInfoMaps();
        setOscillatorShaderParameterDataToTBO();
        
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            ofLog() << "OpenGL error: " << err;
        }
        
        isFirstPassAfterSetup = true;
        sizeChanged = false;
    }
    
    for(auto &parameter : changedOscillatorParameters){
        vector<float> &vf = parameter.second;
        int position = oscillatorShaderParameterNameTBOPositionMap[parameter.first];
        int size = oscillatorShaderParameterNameTBOSizeMap[parameter.first];
        
        if(vf.size() == size){
            oscillatorShaderBuffer.updateData(position*4, vf);
        }else{
            oscillatorShaderBuffer.updateData(position*4, vector<float>(size, vf[0]));
        }
    }
    changedOscillatorParameters.clear();
}

void oscillatorTexture::draw(ofEventArgs &a){
    oscillatorOut = &computeBank(phasorIn);
}

void oscillatorTexture::setParametersInfoMaps(){
    int dimensionsSum = width+height;
    
    oscillatorShaderParameterNameTBOPositionMap[widthVec.getName()] = 0;
    oscillatorShaderParameterNameTBOPositionMap[heightVec.getName()] = height;
    oscillatorShaderParameterNameTBOSizeMap[widthVec.getName()] = height;
    oscillatorShaderParameterNameTBOSizeMap[heightVec.getName()] = width;
    
    oscillatorShaderParameterNameTBOPositionMap[phaseOffset[0].getName()] = dimensionsSum;
    oscillatorShaderParameterNameTBOPositionMap[phaseOffset[1].getName()] = (dimensionsSum) + width;
    oscillatorShaderParameterNameTBOSizeMap[phaseOffset[0].getName()] = width;
    oscillatorShaderParameterNameTBOSizeMap[phaseOffset[1].getName()] = height;
    
    oscillatorShaderParameterNameTBOPositionMap[roundness[0].getName()] = dimensionsSum * 2;
    oscillatorShaderParameterNameTBOPositionMap[roundness[1].getName()] = (dimensionsSum * 2) + width;
    oscillatorShaderParameterNameTBOSizeMap[roundness[0].getName()] = width;
    oscillatorShaderParameterNameTBOSizeMap[roundness[1].getName()] = height;
    
    oscillatorShaderParameterNameTBOPositionMap[pulseWidth[0].getName()] = dimensionsSum * 3;
    oscillatorShaderParameterNameTBOPositionMap[pulseWidth[1].getName()] = (dimensionsSum * 3) + width;
    oscillatorShaderParameterNameTBOSizeMap[pulseWidth[0].getName()] = width;
    oscillatorShaderParameterNameTBOSizeMap[pulseWidth[1].getName()] = height;
    
    oscillatorShaderParameterNameTBOPositionMap[skew[0].getName()] = dimensionsSum * 4;
    oscillatorShaderParameterNameTBOPositionMap[skew[1].getName()] = (dimensionsSum * 4) + width;
    oscillatorShaderParameterNameTBOSizeMap[skew[0].getName()] = width;
    oscillatorShaderParameterNameTBOSizeMap[skew[1].getName()] = height;
    
    oscillatorShaderParameterNameTBOPositionMap[randomAddition[0].getName()] = dimensionsSum * 5;
    oscillatorShaderParameterNameTBOPositionMap[randomAddition[1].getName()] = (dimensionsSum * 5) + width;
    oscillatorShaderParameterNameTBOSizeMap[randomAddition[0].getName()] = width;
    oscillatorShaderParameterNameTBOSizeMap[randomAddition[1].getName()] = height;
    
    oscillatorShaderParameterNameTBOPositionMap[scale[0].getName()] = dimensionsSum * 6;
    oscillatorShaderParameterNameTBOPositionMap[scale[1].getName()] = (dimensionsSum * 6) + width;
    oscillatorShaderParameterNameTBOSizeMap[scale[0].getName()] = width;
    oscillatorShaderParameterNameTBOSizeMap[scale[1].getName()] = height;
    
    oscillatorShaderParameterNameTBOPositionMap[offset[0].getName()] = dimensionsSum * 7;
    oscillatorShaderParameterNameTBOPositionMap[offset[1].getName()] = (dimensionsSum * 7) + width;
    oscillatorShaderParameterNameTBOSizeMap[offset[0].getName()] = width;
    oscillatorShaderParameterNameTBOSizeMap[offset[1].getName()] = height;
    
    oscillatorShaderParameterNameTBOPositionMap[pow[0].getName()] = dimensionsSum * 8;
    oscillatorShaderParameterNameTBOPositionMap[pow[1].getName()] = (dimensionsSum * 8) + width;
    oscillatorShaderParameterNameTBOSizeMap[pow[0].getName()] = width;
    oscillatorShaderParameterNameTBOSizeMap[pow[1].getName()] = height;
    
    oscillatorShaderParameterNameTBOPositionMap[bipow[0].getName()] = dimensionsSum * 9;
    oscillatorShaderParameterNameTBOPositionMap[bipow[1].getName()] = (dimensionsSum * 9) + width;
    oscillatorShaderParameterNameTBOSizeMap[bipow[0].getName()] = width;
    oscillatorShaderParameterNameTBOSizeMap[bipow[1].getName()] = height;
    
    oscillatorShaderParameterNameTBOPositionMap[quantization[0].getName()] = dimensionsSum * 10;
    oscillatorShaderParameterNameTBOPositionMap[quantization[1].getName()] = (dimensionsSum * 10) + width;
    oscillatorShaderParameterNameTBOSizeMap[quantization[0].getName()] = width;
    oscillatorShaderParameterNameTBOSizeMap[quantization[1].getName()] = height;
    
    oscillatorShaderParameterNameTBOPositionMap[fader[0].getName()] = dimensionsSum * 11;
    oscillatorShaderParameterNameTBOPositionMap[fader[1].getName()] = (dimensionsSum * 11) + width;
    oscillatorShaderParameterNameTBOSizeMap[fader[0].getName()] = width;
    oscillatorShaderParameterNameTBOSizeMap[fader[1].getName()] = height;
    
    oscillatorShaderParameterNameTBOPositionMap[invert[0].getName()] = dimensionsSum * 12;
    oscillatorShaderParameterNameTBOPositionMap[invert[1].getName()] = (dimensionsSum * 12) + width;
    oscillatorShaderParameterNameTBOSizeMap[invert[0].getName()] = width;
    oscillatorShaderParameterNameTBOSizeMap[invert[1].getName()] = height;
}

void oscillatorTexture::setOscillatorShaderParameterDataToTBO(){
    vector<float> accumulateParametersOscillatorShaderParameters;
    
    vector<float> widthVec_tempVec(height, widthVec.get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), widthVec_tempVec.begin(), widthVec_tempVec.end());
    vector<float> heightVec_tempVec(width, heightVec.get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), heightVec_tempVec.begin(), heightVec_tempVec.end());
    
    vector<float> phaseOffsetX_tempVec(width, phaseOffset[0].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), phaseOffsetX_tempVec.begin(), phaseOffsetX_tempVec.end());
    vector<float> phaseOffsetY_tempVec(height, phaseOffset[1].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), phaseOffsetY_tempVec.begin(), phaseOffsetY_tempVec.end());
    
    vector<float> roundnessX_tempVec(width, roundness[0].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), roundnessX_tempVec.begin(), roundnessX_tempVec.end());
    vector<float> roundnessY_tempVec(height, roundness[1].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), roundnessY_tempVec.begin(), roundnessY_tempVec.end());
    
    vector<float> pulseWidthX_tempVec(width, pulseWidth[0].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), pulseWidthX_tempVec.begin(), pulseWidthX_tempVec.end());
    vector<float> pulseWidthY_tempVec(height, pulseWidth[1].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), pulseWidthY_tempVec.begin(), pulseWidthY_tempVec.end());
    
    vector<float> skewX_tempVec(width, skew[0].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), skewX_tempVec.begin(), skewX_tempVec.end());
    vector<float> skewY_tempVec(height, skew[1].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), skewY_tempVec.begin(), skewY_tempVec.end());
    
    vector<float> randomAdditionX_tempVec(width, randomAddition[0].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), randomAdditionX_tempVec.begin(), randomAdditionX_tempVec.end());
    vector<float> randomAdditionY_tempVec(height, randomAddition[1].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), randomAdditionY_tempVec.begin(), randomAdditionY_tempVec.end());
    
    vector<float> scaleX_tempVec(width, scale[0].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), scaleX_tempVec.begin(), scaleX_tempVec.end());
    vector<float> scaleY_tempVec(height, scale[1].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), scaleY_tempVec.begin(), scaleY_tempVec.end());
    
    vector<float> offsetX_tempVec(width, offset[0].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), offsetX_tempVec.begin(), offsetX_tempVec.end());
    vector<float> offsetY_tempVec(height, offset[1].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), offsetY_tempVec.begin(), offsetY_tempVec.end());
    
    vector<float> powX_tempVec(width, pow[0].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), powX_tempVec.begin(), powX_tempVec.end());
    vector<float> powY_tempVec(height, pow[1].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), powY_tempVec.begin(), powY_tempVec.end());
    
    vector<float> bipowX_tempVec(width, bipow[0].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), bipowX_tempVec.begin(), bipowX_tempVec.end());
    vector<float> bipowY_tempVec(height, bipow[1].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), bipowY_tempVec.begin(), bipowY_tempVec.end());
    
    vector<float> quantizationX_tempVec(width, quantization[0].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), quantizationX_tempVec.begin(), quantizationX_tempVec.end());
    vector<float> quantizationY_tempVec(height, quantization[1].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), quantizationY_tempVec.begin(), quantizationY_tempVec.end());
    
    vector<float> faderX_tempVec(width, fader[0].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), faderX_tempVec.begin(), faderX_tempVec.end());
    vector<float> faderY_tempVec(height, fader[1].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), faderY_tempVec.begin(), faderY_tempVec.end());
    
    vector<float> invertX_tempVec(width, invert[0].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), invertX_tempVec.begin(), invertX_tempVec.end());
    vector<float> invertY_tempVec(height, invert[1].get()[0]);
    accumulateParametersOscillatorShaderParameters.insert(accumulateParametersOscillatorShaderParameters.end(), invertY_tempVec.begin(), invertY_tempVec.end());
    
    oscillatorShaderBuffer.setData(accumulateParametersOscillatorShaderParameters, GL_STREAM_DRAW);
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
}

void oscillatorTexture::presetRecallBeforeSettingParameters(ofJson &json){
    if(json.count(widthVec.getEscapedName()) == 1){
        if(json[widthVec.getEscapedName()].is_string()){
            widthVec = vector<int>(1, ofToInt(json[widthVec.getEscapedName()]));
        }else{
            widthVec = vector<int>(1, int(json[widthVec.getEscapedName()]));
        }
    }
    if(json.count(heightVec.getEscapedName()) == 1){
        if(json[heightVec.getEscapedName()].is_string()){
            heightVec = vector<int>(1, ofToInt(json[heightVec.getEscapedName()]));
        }else{
            heightVec = vector<int>(1, int(json[heightVec.getEscapedName()]));
        }
    }
    isFirstPassAfterSetup = true;
}

ofTexture& oscillatorTexture::computeBank(float phasor){
//    swap(fbo, fboBuffer);
    
    ofPushStyle();
    ofDisableAlphaBlending();
    ofSetColor(255, 255);
    fboBuffer.begin();
    ofClear(0, 0, 0, 255);
    shaderOscillator.begin();
    shaderOscillator.setUniform1f("phase", phasor);
    shaderOscillator.setUniform1f("time", ofGetElapsedTimef());
    if(indexs.get() != nullptr && indexs.get()->getWidth() == width && indexs.get()->getHeight() == height){
        shaderOscillator.setUniformTexture("indexs", *indexs.get(), 0);
    }else{
        shaderOscillator.setUniformTexture("indexs", blackIndexs.getTexture(), 0);
    }
    shaderOscillator.setUniformTexture("parameters", oscillatorShaderTexture, 1);
    ofDrawRectangle(0, 0, width, height);
    shaderOscillator.end();
    fboBuffer.end();
    
//    fbo.begin();
//    ofClear(0, 0, 0, 255);
//    shaderScaling.begin();
//    shaderScaling.setUniform1f("phase", phasor);
//    shaderScaling.setUniform1f("time", ofGetElapsedTimef());
//    shaderScaling.setUniformTexture("randomInfo", fboBuffer.getTexture(), randomInfoScalingShaderTextureLocation);
//    ofDrawRectangle(0, 0, width, height);
//    shaderScaling.end();
//    fbo.end();
    ofPopStyle();
    
    isFirstPassAfterSetup = false;
    
    return fboBuffer.getTexture();
}

void oscillatorTexture::newPhasorIn(float &f){
//    oscillatorOut = &computeBank(f);
}

vector<float> oscillatorTexture::newRandomValuesVector(){
    vector<float> randomValuesVecX(width, 0);
    vector<float> randomValuesVecY(height, 0);
    iota(randomValuesVecX.begin(), randomValuesVecX.end(), 0);
    iota(randomValuesVecY.begin(), randomValuesVecY.end(), 0);
//    for(int i = 0; i < randomValuesVecX.size(); i++){
//        randomValuesVecX[i] = (int)-randomValuesVecX.size()/2 + i;
//    }
//    for(int i = 0; i < randomValuesVecY.size(); i++){
//        randomValuesVecY[i] = (int)-randomValuesVecY.size()/2 + i;
//    }
    
    mt19937 g(static_cast<uint32_t>(time(0)));
    shuffle(randomValuesVecX.begin(), randomValuesVecX.end(), g);
    shuffle(randomValuesVecY.begin(), randomValuesVecY.end(), g);
    
    randomValuesVecX.insert(randomValuesVecX.end(), randomValuesVecY.begin(), randomValuesVecY.end());
    
    return randomValuesVecX;
}


#pragma mark Parameter Listeners

void oscillatorTexture::onOscillatorShaderParameterChanged(ofAbstractParameter &p, vector<float> &vf){
    changedOscillatorParameters.emplace_back(p.getName(), vf);
}

void oscillatorTexture::sizeChangedListener(int &i){
    if(&i == &width.get()){
        if(width != previousWidth){
            sizeChanged = true;
        }
        previousWidth = width;
    }else{
        if(height != previousHeight){
            sizeChanged = true;
        }
        previousHeight = height;
    }
}


