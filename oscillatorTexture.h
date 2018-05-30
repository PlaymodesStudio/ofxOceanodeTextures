//
//  oscillatorTexture.h
//  MIRABCN_Generator
//
//  Created by Eduard Frigola on 24/01/2018.
//
//

#ifndef oscillatorTexture_h
#define oscillatorTexture_h

#include "sharedResources.h"
#include "ofxOceanode.h"

class oscillatorTexture : public ofxOceanodeNodeModel{
public:
    oscillatorTexture();
    ~oscillatorTexture();
    
    void presetRecallBeforeSettingParameters(ofJson &json) override;
private:
    ofTexture&  computeBank(float phasor);
    
    template<typename T>
    void changeMinMaxOfVecParameter(ofParameter<vector<T>> &param, T min = -1, T max = -1, bool scaleValue = false){
        float valueNormalized;
        if(param.get().size() == 1 && scaleValue)
            valueNormalized = ofMap(param.get()[0], param.getMin()[0], param.getMax()[0], 0, 1, true);
        if(min != -1)
            param.setMin(vector<T>(1, min));
        if(max != -1)
            param.setMax(vector<T>(1, max));
        string name = param.getName();
        ofNotifyEvent(parameterChangedMinMax, name);
        if(param.get().size() == 1){
            if(scaleValue){
                param = vector<T>(1, ofMap(valueNormalized, 0, 1, param.getMin()[0], param.getMax()[0]));
            }else{
                param = vector<T>(1, ofClamp(param.get()[0], param.getMin()[0], param.getMax()[0]));
            }
        }
    }
    
    void setParametersInfoMaps();
    
    void setOscillatorShaderIntParameterDataToTBO();
    void setOscillatorShaderFloatParameterDataToTBO();
    void setScalingShaderIntParameterDataToTBO();
    void setScalingShaderFloatParameterDataToTBO();
    
    vector<float> newRandomValuesVector();
    
    void loadShader(bool &b);
    
    void newPhasorIn(float &f);
    void sizeChanged(int &i);
    void newWaveSelectParam(int &i);
    
    
    ofParameter<bool>       reloadShaderParam;
    ofParameter<float>    phasorIn;
    
    ofParameter<int> width;
    ofParameter<int> height;
    int previousWidth, previousHeight;
    
    ofParameter<vector<float>>   indexNumWaves[2];
    ofParameter<vector<float>>   indexInvert[2];
    ofParameter<vector<int>>   indexSymmetry[2];
    ofParameter<vector<float>>   indexRandom[2];
    ofParameter<vector<float>>   indexOffset[2];
    ofParameter<vector<int>>   indexQuantization[2];
    ofParameter<vector<float>>   indexCombination[2];
    ofParameter<vector<int>>   indexModulo[2];
    
    ofParameter<vector<float>>   phaseOffset[2];
    ofParameter<vector<float>>   randomAddition[2];
    ofParameter<vector<float>>   scale[2];
    ofParameter<vector<float>>   offset[2];
    ofParameter<vector<float>>   pow[2];
    ofParameter<vector<float>>   bipow[2];
    ofParameter<vector<int>>     quantization[2];
    ofParameter<vector<float>>   pulseWidth[2];
    ofParameter<vector<float>>   skew[2];
    ofParameter<vector<float>>   fader[2];
    ofParameter<vector<float>>   invert[2];
    ofParameter<vector<float>>  waveform[2];
    ofParameter<int>            waveformDropdown[2];
    
    ofParameter<int>      waveSelect_Param;

    
    ofParameter<ofTexture*>      oscillatorOut;
    
    map<string, int> oscillatorShaderIntParameterNameTBOPositionMap;
    map<string, int> oscillatorShaderFloatParameterNameTBOPositionMap;
    map<string, int> scalingShaderIntParameterNameTBOPositionMap;
    map<string, int> scalingShaderFloatParameterNameTBOPositionMap;
    
    map<string, int> oscillatorShaderParameterNameTBOSizeMap;
    map<string, int> scalingShaderParameterNameTBOSizeMap;

    sharedResources* resources;
    
    uint oscillatorShaderIntParametersTextureLocation;
    uint oscillatorShaderFloatParametersTextureLocation;
    uint scalingShaderIntParametersTextureLocation;
    uint scalingShaderFloatParametersTextureLocation;
    
    uint randomIndexsTextureLocation;
    
    uint randomInfoOscillatorShaderTextureLocation;
    uint randomInfoScalingShaderTextureLocation;
    
    ofShader shaderOscillator;
    ofShader shaderScaling;
    ofFbo   fbo;
    ofFbo   fboBuffer;
    
    
    //Listeners
    ofEventListeners oscillatorShaderIntListeners;
    ofEventListeners oscillatorShaderFloatListeners;
    ofEventListeners scalingShaderIntListeners;
    ofEventListeners scalingShaderFloatListeners;
    
    void onOscillatorShaderIntParameterChanged(ofAbstractParameter &p, vector<int> &vi);
    void onOscillatorShaderFloatParameterChanged(ofAbstractParameter &p, vector<float> &vf);
    void onScalingShaderIntParameterChanged(ofAbstractParameter &p, vector<int> &vi);
    void onScalingShaderFloatParameterChanged(ofAbstractParameter &p, vector<float> &vf);
    
    //TBOs
    
    ofTexture               oscillatorShaderIntTexture;
    ofBufferObject          oscillatorShaderIntBuffer;
    
    ofTexture               oscillatorShaderFloatTexture;
    ofBufferObject          oscillatorShaderFloatBuffer;
    
    ofTexture               scalingShaderIntTexture;
    ofBufferObject          scalingShaderIntBuffer;
    
    ofTexture               scalingShaderFloatTexture;
    ofBufferObject          scalingShaderFloatBuffer;
    
    //Index Random Values
    ofTexture               indexRandomValuesTexture;
    ofBufferObject          indexRandomValuesBuffer;
};

#endif /* oscillatorTexture_h */
