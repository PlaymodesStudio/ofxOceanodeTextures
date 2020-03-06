//
//  indexerTexture.cpp
//  PLedNodes
//
//  Created by Eduard Frigola Bagué on 03/03/2020.
//

#include "indexerTexture.h"
#include <random>

#define STRINGIFY(x) #x

indexerTexture::indexerTexture() : ofxOceanodeNodeModel("Indexer Texture"){
    isSetup = false;
    isFirstPassAfterSetup = true;
    sizeChanged = false;
}

indexerTexture::~indexerTexture(){
    if(isSetup){
        shaderParametersTexture.clear();
        indexRandomValuesTexture.clear();
        
        resources->makeTextureLocationAvailable(shaderParametersTextureLocation);
        resources->makeTextureLocationAvailable(randomIndexsTextureLocation);;
    }
}

void indexerTexture::setup(){
    resources = &sharedResources::getInstance();
    
    addParameterToGroupAndInfo(widthVec.set("Tex Width", {100}, {1}, {5120}));
    addParameterToGroupAndInfo(heightVec.set("Tex Height", {100}, {1}, {2880}));
    width = 100;
    height = 100;
    
    previousWidth = width;
    previousHeight = height;
    
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
    
    setParametersInfoMaps();
    
    parameters->add(indexsOut.set("Indexs Out", nullptr, nullptr, nullptr));
    
    
    //Listeners
    listeners.push(widthVec.newListener([this](vector<int> &vi){
        width = *max_element(vi.begin(), vi.end());
        vector<float> vf(vi.begin(), vi.end());
        onShaderParameterChanged(widthVec, vf);
    }));
    
    listeners.push(heightVec.newListener([this](vector<int> &vi){
        height = *max_element(vi.begin(), vi.end());
        vector<float> vf(vi.begin(), vi.end());
        onShaderParameterChanged(heightVec, vf);
    }));
    
    listeners.push(width.newListener(this, &indexerTexture::sizeChangedListener));
    listeners.push(height.newListener(this, &indexerTexture::sizeChangedListener));
    
    
    shaderParameterListeners.push(indexNumWaves[0].newListener([&](vector<float> &vf){
        onShaderParameterChanged(indexNumWaves[0], vf);
    }));
    shaderParameterListeners.push(indexNumWaves[1].newListener([&](vector<float> &vf){
        onShaderParameterChanged(indexNumWaves[1], vf);
    }));
    
    shaderParameterListeners.push(indexInvert[0].newListener([&](vector<float> &vf){
        onShaderParameterChanged(indexInvert[0], vf);
    }));
    shaderParameterListeners.push(indexInvert[1].newListener([&](vector<float> &vf){
        onShaderParameterChanged(indexInvert[1], vf);
    }));
    
    shaderParameterListeners.push(indexSymmetry[0].newListener([&](vector<int> &vi){
        vector<float> vf(vi.begin(), vi.end());
        onShaderParameterChanged(indexSymmetry[0], vf);
    }));
    shaderParameterListeners.push(indexSymmetry[1].newListener([&](vector<int> &vi){
        vector<float> vf(vi.begin(), vi.end());
        onShaderParameterChanged(indexSymmetry[1], vf);
    }));
    
    shaderParameterListeners.push(indexRandom[0].newListener([&](vector<float> &vf){
        onShaderParameterChanged(indexRandom[0], vf);
    }));
    shaderParameterListeners.push(indexRandom[1].newListener([&](vector<float> &vf){
        onShaderParameterChanged(indexRandom[1], vf);
    }));
    
    shaderParameterListeners.push(indexOffset[0].newListener([&](vector<float> &vf){
        onShaderParameterChanged(indexOffset[0], vf);
    }));
    shaderParameterListeners.push(indexOffset[1].newListener([&](vector<float> &vf){
        onShaderParameterChanged(indexOffset[1], vf);
    }));
    
    shaderParameterListeners.push(indexQuantization[0].newListener([&](vector<int> &vi){
        vector<float> vf(vi.begin(), vi.end());
        onShaderParameterChanged(indexQuantization[0], vf);
    }));
    shaderParameterListeners.push(indexQuantization[1].newListener([&](vector<int> &vi){
        vector<float> vf(vi.begin(), vi.end());
        onShaderParameterChanged(indexQuantization[1], vf);
    }));
    
    shaderParameterListeners.push(indexCombination[0].newListener([&](vector<float> &vf){
        onShaderParameterChanged(indexCombination[0], vf);
    }));
    shaderParameterListeners.push(indexCombination[1].newListener([&](vector<float> &vf){
        onShaderParameterChanged(indexCombination[1], vf);
    }));
    
    shaderParameterListeners.push(indexModulo[0].newListener([&](vector<int> &vi){
        vector<float> vf(vi.begin(), vi.end());
        onShaderParameterChanged(indexModulo[0], vf);
    }));
    shaderParameterListeners.push(indexModulo[1].newListener([&](vector<int> &vi){
        vector<float> vf(vi.begin(), vi.end());
        onShaderParameterChanged(indexModulo[1], vf);
    }));
    
    isFirstPassAfterSetup = true;
}

void indexerTexture::update(ofEventArgs &a){
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
        
        //TBO
        
        //OSCILLATOR SHADER FLOAT
        shaderParametersBuffer.allocate();
        shaderParametersBuffer.bind(GL_TEXTURE_BUFFER);
        setShaderParameterDataToTBO();
        shaderParametersTexture.allocateAsBufferTexture(shaderParametersBuffer, GL_R32F);
        
        //IndexRandomValues
        indexRandomValuesBuffer.allocate();
        indexRandomValuesBuffer.bind(GL_TEXTURE_BUFFER);
        indexRandomValuesBuffer.setData(newRandomValuesVector(), GL_STREAM_DRAW);
        indexRandomValuesTexture.allocateAsBufferTexture(indexRandomValuesBuffer, GL_R32F);
        
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
        
        setParametersInfoMaps();
        setShaderParameterDataToTBO();
        
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            ofLog() << "OpenGL error: " << err;
        }
        
        indexRandomValuesBuffer.setData(newRandomValuesVector(), GL_STREAM_DRAW);
        
        isFirstPassAfterSetup = true;
        sizeChanged = false;
    }
    
    for(auto &parameter : changedParameters){
        vector<float> &vf = parameter.second;
        int position = shaderParameterNameTBOPositionMap[parameter.first];
        int size = shaderParameterNameTBOSizeMap[parameter.first];
        
        if(parameter.first == indexRandom[0].getName() || parameter.first == indexRandom[1].getName()){
            if(vf.size() == size){
                if(std::accumulate(vf.begin(), vf.end(), 0) == 0){
                    indexRandomValuesBuffer.setData(newRandomValuesVector(), GL_STREAM_DRAW);
                }
            }else{
                if(vf[0] == 0){
                    indexRandomValuesBuffer.setData(newRandomValuesVector(), GL_STREAM_DRAW);
                }
            }
        }
        
        if(vf.size() == size){
            shaderParametersBuffer.updateData(position*4, vf);
        }else{
            shaderParametersBuffer.updateData(position*4, vector<float>(size, vf[0]));
        }
    }
    changedParameters.clear();
}

void indexerTexture::draw(ofEventArgs &a){
    indexsOut = &computeBank();
}

void indexerTexture::setParametersInfoMaps(){
    int dimensionsSum = width+height;
    shaderParameterNameTBOPositionMap[indexNumWaves[0].getName()] = 0;
    shaderParameterNameTBOPositionMap[indexNumWaves[1].getName()] = height;
    shaderParameterNameTBOSizeMap[indexNumWaves[0].getName()] = height;
    shaderParameterNameTBOSizeMap[indexNumWaves[1].getName()] = width;
    
    shaderParameterNameTBOPositionMap[indexInvert[0].getName()] = dimensionsSum;
    shaderParameterNameTBOPositionMap[indexInvert[1].getName()] = dimensionsSum + height;
    shaderParameterNameTBOSizeMap[indexInvert[0].getName()] = height;
    shaderParameterNameTBOSizeMap[indexInvert[1].getName()] = width;
    
    shaderParameterNameTBOPositionMap[indexRandom[0].getName()] = dimensionsSum * 2;
    shaderParameterNameTBOPositionMap[indexRandom[1].getName()] = (dimensionsSum * 2) + height;
    shaderParameterNameTBOSizeMap[indexRandom[0].getName()] = height;
    shaderParameterNameTBOSizeMap[indexRandom[1].getName()] = width;
    
    shaderParameterNameTBOPositionMap[indexOffset[0].getName()] = dimensionsSum * 3;
    shaderParameterNameTBOPositionMap[indexOffset[1].getName()] = (dimensionsSum * 3) + height;
    shaderParameterNameTBOSizeMap[indexOffset[0].getName()] = height;
    shaderParameterNameTBOSizeMap[indexOffset[1].getName()] = width;
    
    shaderParameterNameTBOPositionMap[indexCombination[0].getName()] = dimensionsSum * 4;
    shaderParameterNameTBOPositionMap[indexCombination[1].getName()] = (dimensionsSum * 4) + height;
    shaderParameterNameTBOSizeMap[indexCombination[0].getName()] = height;
    shaderParameterNameTBOSizeMap[indexCombination[1].getName()] = width;
    
    shaderParameterNameTBOPositionMap[indexSymmetry[0].getName()] = dimensionsSum * 5;
    shaderParameterNameTBOPositionMap[indexSymmetry[1].getName()] = (dimensionsSum * 5) + height;
    shaderParameterNameTBOSizeMap[indexSymmetry[0].getName()] = height;
    shaderParameterNameTBOSizeMap[indexSymmetry[1].getName()] = width;
    
    shaderParameterNameTBOPositionMap[indexQuantization[0].getName()] = dimensionsSum * 6;
    shaderParameterNameTBOPositionMap[indexQuantization[1].getName()] = (dimensionsSum * 6) + height;
    shaderParameterNameTBOSizeMap[indexQuantization[0].getName()] = height;
    shaderParameterNameTBOSizeMap[indexQuantization[1].getName()] = width;
    
    shaderParameterNameTBOPositionMap[indexModulo[0].getName()] = dimensionsSum * 7;
    shaderParameterNameTBOPositionMap[indexModulo[1].getName()] = (dimensionsSum * 7) + height;
    shaderParameterNameTBOSizeMap[indexModulo[0].getName()] = height;
    shaderParameterNameTBOSizeMap[indexModulo[1].getName()] = width;
    
    shaderParameterNameTBOPositionMap[widthVec.getName()] = dimensionsSum * 8;
    shaderParameterNameTBOPositionMap[heightVec.getName()] = (dimensionsSum * 8) + height;
    shaderParameterNameTBOSizeMap[widthVec.getName()] = height;
    shaderParameterNameTBOSizeMap[heightVec.getName()] = width;
}

void indexerTexture::setShaderParameterDataToTBO(){
    vector<float> accumulateParametersShaderParameters;
    
    vector<float> indexNumWavesX_tempVec(height, indexNumWaves[0].get()[0]);
    accumulateParametersShaderParameters.insert(accumulateParametersShaderParameters.end(), indexNumWavesX_tempVec.begin(), indexNumWavesX_tempVec.end());
    vector<float> indexNumWavesY_tempVec(width, indexNumWaves[1].get()[0]);
    accumulateParametersShaderParameters.insert(accumulateParametersShaderParameters.end(), indexNumWavesY_tempVec.begin(), indexNumWavesY_tempVec.end());
    
    vector<float> indexInvertX_tempVec(height, indexInvert[0].get()[0]);
    accumulateParametersShaderParameters.insert(accumulateParametersShaderParameters.end(), indexInvertX_tempVec.begin(), indexInvertX_tempVec.end());
    vector<float> indexInvertY_tempVec(width, indexInvert[1].get()[0]);
    accumulateParametersShaderParameters.insert(accumulateParametersShaderParameters.end(), indexInvertY_tempVec.begin(), indexInvertY_tempVec.end());
    
    vector<float> indexRandomX_tempVec(height, indexRandom[0].get()[0]);
    accumulateParametersShaderParameters.insert(accumulateParametersShaderParameters.end(), indexRandomX_tempVec.begin(), indexRandomX_tempVec.end());
    vector<float> indexRandomY_tempVec(width, indexRandom[1].get()[0]);
    accumulateParametersShaderParameters.insert(accumulateParametersShaderParameters.end(), indexRandomY_tempVec.begin(), indexRandomY_tempVec.end());
    
    vector<float> indexOffsetX_tempVec(height, indexOffset[0].get()[0]);
    accumulateParametersShaderParameters.insert(accumulateParametersShaderParameters.end(), indexOffsetX_tempVec.begin(), indexOffsetX_tempVec.end());
    vector<float> indexOffsetY_tempVec(width, indexOffset[1].get()[0]);
    accumulateParametersShaderParameters.insert(accumulateParametersShaderParameters.end(), indexOffsetY_tempVec.begin(), indexOffsetY_tempVec.end());
    
    vector<float> indexCombinationX_tempVec(height, indexCombination[0].get()[0]);
    accumulateParametersShaderParameters.insert(accumulateParametersShaderParameters.end(), indexCombinationX_tempVec.begin(), indexCombinationX_tempVec.end());
    vector<float> indexCombinationY_tempVec(width, indexCombination[1].get()[0]);
    accumulateParametersShaderParameters.insert(accumulateParametersShaderParameters.end(), indexCombinationY_tempVec.begin(), indexCombinationY_tempVec.end());
    
    vector<float> indexSymmetryX_tempVec(height, indexSymmetry[0].get()[0]);
    accumulateParametersShaderParameters.insert(accumulateParametersShaderParameters.end(), indexSymmetryX_tempVec.begin(), indexSymmetryX_tempVec.end());
    vector<float> indexSymmetryY_tempVec(width, indexSymmetry[1].get()[0]);
    accumulateParametersShaderParameters.insert(accumulateParametersShaderParameters.end(), indexSymmetryY_tempVec.begin(), indexSymmetryY_tempVec.end());
    
    vector<float> indexQuantizationX_tempVec(height, indexQuantization[0].get()[0]);
    accumulateParametersShaderParameters.insert(accumulateParametersShaderParameters.end(), indexQuantizationX_tempVec.begin(), indexQuantizationX_tempVec.end());
    vector<float> indexQuantizationY_tempVec(width, indexQuantization[1].get()[0]);
    accumulateParametersShaderParameters.insert(accumulateParametersShaderParameters.end(), indexQuantizationY_tempVec.begin(), indexQuantizationY_tempVec.end());
    
    vector<float> indexModuloX_tempVec(height, indexModulo[0].get()[0]);
    accumulateParametersShaderParameters.insert(accumulateParametersShaderParameters.end(), indexModuloX_tempVec.begin(), indexModuloX_tempVec.end());
    vector<float> indexModuloY_tempVec(width, indexModulo[1].get()[0]);
    accumulateParametersShaderParameters.insert(accumulateParametersShaderParameters.end(), indexModuloY_tempVec.begin(), indexModuloY_tempVec.end());
    
    vector<float> widthVec_tempVec(height, widthVec.get()[0]);
    accumulateParametersShaderParameters.insert(accumulateParametersShaderParameters.end(), widthVec_tempVec.begin(), widthVec_tempVec.end());
    vector<float> heightVec_tempVec(width, heightVec.get()[0]);
    accumulateParametersShaderParameters.insert(accumulateParametersShaderParameters.end(), heightVec_tempVec.begin(), heightVec_tempVec.end());
    
    
    shaderParametersBuffer.setData(accumulateParametersShaderParameters, GL_STREAM_DRAW);
}


void indexerTexture::loadShader(bool &b){
    string defaultVertSource =
#include "defaultVertexShader.h"
    ;
    
    string fragSource =
#include "indexerTextureShader.h"
    ;
    
    shader.setupShaderFromSource(GL_VERTEX_SHADER, defaultVertSource);
    shader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragSource);
    shader.bindDefaults();
    shader.linkProgram();
    
    shaderParametersTextureLocation = resources->getNextAvailableShaderTextureLocation();
    randomIndexsTextureLocation = resources->getNextAvailableShaderTextureLocation();
    
    shader.begin();
    shader.setUniform2i("size", width, height);
    shader.setUniformTexture("parameters", shaderParametersTexture, shaderParametersTextureLocation);
    shader.setUniformTexture("indexRandomValues", indexRandomValuesTexture, randomIndexsTextureLocation);
    shader.end();
}

void indexerTexture::presetRecallBeforeSettingParameters(ofJson &json){
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

ofTexture& indexerTexture::computeBank(){
//    swap(fbo, fboBuffer);
    
    ofPushStyle();
    ofDisableAlphaBlending();
    ofSetColor(255, 255);
//    fboBuffer.begin();
//    ofClear(0, 0, 0, 255);
//    shaderOscillator.begin();
//    shaderOscillator.setUniform1f("phase", 0);
//    shaderOscillator.setUniform1f("time", ofGetElapsedTimef());
//    shaderOscillator.setUniform1f("createRandoms", isFirstPassAfterSetup ? 1 : 0);
//    ofDrawRectangle(0, 0, width, height);
//    shaderOscillator.end();
//    fboBuffer.end();
    
    fbo.begin();
    ofClear(0, 0, 0, 255);
    shader.begin();
    shader.setUniform1f("time", ofGetElapsedTimef());
    ofDrawRectangle(0, 0, width, height);
    shader.end();
    fbo.end();
    ofPopStyle();
    
    isFirstPassAfterSetup = false;
    
    return fbo.getTexture();
}

vector<float> indexerTexture::newRandomValuesVector(){
    vector<float> randomValuesVecX(width, 0);
    vector<float> randomValuesVecY(height, 0);
    iota(randomValuesVecX.begin(), randomValuesVecX.end(), 0);
    iota(randomValuesVecY.begin(), randomValuesVecY.end(), 0);
    
    mt19937 g(static_cast<uint32_t>(time(0)));
    shuffle(randomValuesVecX.begin(), randomValuesVecX.end(), g);
    shuffle(randomValuesVecY.begin(), randomValuesVecY.end(), g);
    
    randomValuesVecX.insert(randomValuesVecX.end(), randomValuesVecY.begin(), randomValuesVecY.end());
    
    return randomValuesVecX;
}


#pragma mark Parameter Listeners
void indexerTexture::onShaderParameterChanged(ofAbstractParameter &p, vector<float> &vf){
    changedParameters.emplace_back(p.getName(), vf);
}

void indexerTexture::sizeChangedListener(int &i){
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
}
