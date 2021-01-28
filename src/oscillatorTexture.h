//
//  oscillatorTexture.h
//  MIRABCN_Generator
//
//  Created by Eduard Frigola on 24/01/2018.
//
//

#ifndef oscillatorTexture_h
#define oscillatorTexture_h

#include "ofxOceanode.h"

class oscillatorTexture : public ofxOceanodeNodeModel{
public:
    oscillatorTexture();
    ~oscillatorTexture();
    
    void setup() override;
    void update(ofEventArgs &a) override;
    void draw(ofEventArgs &a) override;
    
    void presetRecallBeforeSettingParameters(ofJson &json) override;
private:
    bool isSetup;
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
    
    ofParameter<ofTexture*>      oscillatorOut;
    
    map<string, int> oscillatorShaderParameterNameTBOPositionMap;
    
    map<string, int> oscillatorShaderParameterNameTBOSizeMap;
    
    ofShader shaderOscillator;
    ofFbo   fbo;
    ofFbo   fboBuffer;
    ofFbo   blackIndexs;
    
    
    //Listeners
    ofEventListeners oscillatorShaderListeners;
    
    void onOscillatorShaderParameterChanged(ofAbstractParameter &p, vector<float> &vf);
    
    //TBOs
    ofTexture               oscillatorShaderTexture;
    ofBufferObject          oscillatorShaderBuffer;
    
    bool isFirstPassAfterSetup;
    
    vector<pair<string, vector<float>>> changedOscillatorParameters;
    
    bool sizeChanged;
};

#endif /* oscillatorTexture_h */
