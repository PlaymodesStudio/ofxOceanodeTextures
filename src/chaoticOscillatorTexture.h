//
//  chaoticOscillatorTexture.h
//  PLedNodes
//
//  Created by Eduard Frigola Bagué on 03/03/2020.
//

#ifndef chaoticOscillatorTexture_h
#define chaoticOscillatorTexture_h

#include "sharedResources.h"
#include "ofxOceanode.h"

class chaoticOscillatorTexture : public ofxOceanodeNodeModel{
public:
    chaoticOscillatorTexture();
    ~chaoticOscillatorTexture();
    
    void setup() override;
    void update(ofEventArgs &a) override;
    void draw(ofEventArgs &a) override;
    
    void resetPhase() override{
        newSeed = true;
    }
    
    void presetRecallBeforeSettingParameters(ofJson &json) override;
private:
    bool isSetup;
    ofTexture&  computeBank(float phasor, bool isZeroComputeBank = false);
    
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
    
    void setOscillatorShaderParameterDataToTBO();
    
    vector<float> newRandomValuesVector();
    
    void loadShader(bool &b);
    
    void newPhasorIn(float &f);
    void sizeChangedListener(int &i);
    
    
    ofParameter<bool>       reloadShaderParam;
    ofParameter<float>    phasorIn;
    
    ofParameter<int> width;
    ofParameter<int> height;
    int previousWidth, previousHeight;
    
    ofEventListeners listeners;
    
    
    ofParameter<ofTexture*> indexs;
    ofParameter<vector<int>> widthVec;
    ofParameter<vector<int>> heightVec;
    ofParameter<vector<float>>   phaseOffset[2];
    ofParameter<vector<float>>   roundness[2];
    ofParameter<vector<float>>   pulseWidth[2];
    ofParameter<vector<float>>   skew[2];
    ofParameter<vector<float>>   randomAddition[2];
    ofParameter<vector<float>>   scale[2];
    ofParameter<vector<float>>   offset[2];
    ofParameter<vector<float>>   pow[2];
    ofParameter<vector<float>>   bipow[2];
    ofParameter<vector<int>>     quantization[2];
    ofParameter<vector<float>>   fader[2];
    ofParameter<vector<float>>   invert[2];
    ofParameter<vector<float>>   length[2];
    ofParameter<vector<int>>  seed[2];
    
    ofParameter<ofTexture*>      oscillatorOut;
    
    map<string, int> oscillatorShaderParameterNameTBOPositionMap;
    
    map<string, int> oscillatorShaderParameterNameTBOSizeMap;
    
    sharedResources* resources;
    
    unsigned int oscillatorShaderParametersTextureLocation;
    unsigned int indexsTextureLocation;
    unsigned int randomInfoOscillatorShaderTextureLocation;
    unsigned int oldPhasorOscillatorShaderTextureLocation;
    
    ofShader shaderOscillator;
    ofFbo   fbo;
    ofFbo   blackIndexs;
    ofFbo   randomInfoFbo;
    ofFbo   oldPhaseFbo;
    
    
    //Listeners
    ofEventListeners oscillatorShaderListeners;
    
    void onOscillatorShaderParameterChanged(ofAbstractParameter &p, vector<float> &vf);
    
    //TBOs
    ofTexture               oscillatorShaderTexture;
    ofBufferObject          oscillatorShaderBuffer;
    
    bool isFirstPassAfterSetup;
    bool newSeed = false;
    
    float oldPhasor = 0;
    
    vector<pair<string, vector<float>>> changedOscillatorParameters;
    
    bool sizeChanged;
};

#endif /* chaoticOscillatorTexture_h */
