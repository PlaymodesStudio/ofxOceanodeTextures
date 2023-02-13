//
//  indexerTexture.h
//  PLedNodes
//
//  Created by Eduard Frigola Bagué on 03/03/2020.
//

#ifndef indexerTexture_h
#define indexerTexture_h

#include "ofxOceanodeNodeModel.h"

class indexerTexture : public ofxOceanodeNodeModel{
public:
    indexerTexture();
    ~indexerTexture();
    
    void setup() override;
    void update(ofEventArgs &a) override;
    void draw(ofEventArgs &a) override;
    
    void presetRecallBeforeSettingParameters(ofJson &json) override;
    
    void deactivate(){
        fbo.clear();
        fboBuffer.clear();
        indexsOut = nullptr;
        
        isSetup = false;
    }
private:
    bool isSetup;
    ofTexture&  computeBank();
    
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
    
    void setShaderParameterDataToTBO();
    
    vector<float> newRandomValuesVector(bool x = true, bool y = true);
    
    void loadShader(bool &b);
    
    void sizeChangedListener(int &i);
    
    ofParameter<bool>       reloadShaderParam;
    
    ofParameter<int> width;
    ofParameter<int> height;
    int previousWidth, previousHeight;
    
    ofEventListeners listeners;
    
    ofParameter<vector<int>> widthVec;
    ofParameter<vector<int>> heightVec;
    ofParameter<vector<float>>   indexNumWaves[2];
    ofParameter<vector<float>>   indexInvert[2];
    ofParameter<vector<int>>   indexSymmetry[2];
    ofParameter<vector<float>>   indexRandom[2];
    ofParameter<vector<float>>   indexOffset[2];
    ofParameter<vector<int>>   indexQuantization[2];
    ofParameter<vector<float>>   indexCombination[2];
    ofParameter<vector<int>>   indexModulo[2];
    
    ofParameter<ofTexture*>      indexsOut;
    
    map<string, int> shaderParameterNameTBOPositionMap;
    
    map<string, int> shaderParameterNameTBOSizeMap;
    
    ofShader shader;
    ofFbo   fbo;
    ofFbo   fboBuffer;
    
    
    //Listeners
    ofEventListeners shaderParameterListeners;
    
    void onShaderParameterChanged(ofAbstractParameter &p, vector<float> &vf);
    
    //TBOs
    
    ofTexture               shaderParametersTexture;
    ofBufferObject          shaderParametersBuffer;
    
    //Index Random Values
    ofTexture               indexRandomValuesTexture;
    ofBufferObject          indexRandomValuesBuffer;
    
    bool isFirstPassAfterSetup;
    
    vector<pair<string, vector<float>>> changedParameters;
    
    bool sizeChanged;
    
    vector<float> randomValuesXStore;
    vector<float> randomValuesYStore;
};


class indexerTexture2 : public ofxOceanodeNodeModel{
public:
    indexerTexture2();
    ~indexerTexture2();
    
    void setup() override;
    void update(ofEventArgs &a) override;
    void draw(ofEventArgs &a) override;
    
    void presetRecallBeforeSettingParameters(ofJson &json) override;
    
    void deactivate(){
        fbo.clear();
        fboBuffer.clear();
        indexsOut = nullptr;
        
        isSetup = false;
    }
private:
    bool isSetup;
    ofTexture&  computeBank();
    
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
    
    void setShaderParameterDataToTBO();
    
    vector<float> newRandomValuesVector(bool x = true, bool y = true);
    
    void loadShader();
    
    void sizeChangedListener(int &i);
    void resolutionChangedListener(int &i);
    
    ofParameter<bool>       reloadShaderParam;
    
    ofParameter<int> width;
    ofParameter<int> height;
    ofParameter<int> radiusResolution;
    ofParameter<int> angleResolution;
    ofParameter<float> xCenter;
    ofParameter<float> yCenter;
    int previousWidth, previousHeight;
    int previousRadiusResolution, previousAngleResolution;
    
    ofEventListeners listeners;
    
    ofParameter<vector<int>> widthVec;
    ofParameter<vector<int>> heightVec;
    ofParameter<vector<float>>   indexNumWaves[2];
    ofParameter<vector<float>>   indexInvert[2];
    ofParameter<vector<int>>   indexSymmetry[2];
    ofParameter<vector<float>>   indexRandom[2];
    ofParameter<vector<float>>   indexOffset[2];
    ofParameter<vector<int>>   indexQuantization[2];
    ofParameter<vector<float>>   indexCombination[2];
    ofParameter<vector<int>>   indexModulo[2];
    
    ofParameter<ofTexture*>      indexsOut;
    
    map<string, int> shaderParameterNameTBOPositionMap;
    
    map<string, int> shaderParameterNameTBOSizeMap;
    
    ofShader shader;
    ofFbo   fbo;
    ofFbo   fboBuffer;
    
    
    //Listeners
    ofEventListeners shaderParameterListeners;
    
    void onShaderParameterChanged(ofAbstractParameter &p, vector<float> &vf);
    
    //TBOs
    
    ofTexture               shaderParametersTexture;
    ofBufferObject          shaderParametersBuffer;
    
    //Index Random Values
    ofTexture               indexRandomValuesTexture;
    ofBufferObject          indexRandomValuesBuffer;
    
    bool isFirstPassAfterSetup;
    
    vector<pair<string, vector<float>>> changedParameters;
    
    bool sizeChanged;
    bool resolutionChanged;
    
    vector<float> randomValuesRStore;
    vector<float> randomValuesAStore;
    
    ofParameter<void> reload;
    
};

#endif /* indexerTexture_h */
