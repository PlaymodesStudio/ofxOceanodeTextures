//
//  textureBlender.h
//  ofxOceanodeTextures
//
//  Created by Eduard Frigola on 22/12/23.
//

#ifndef textureBlender_h
#define textureBlender_h

#include "ofxOceanodeNodeModel.h"
#include "ofCamera.h"

class textureBlender : public ofxOceanodeNodeModel{
public:
    textureBlender();
    
    void setup() override;

    void presetRecallBeforeSettingParameters(ofJson &) override{
        loadingPreset = true;
    }

    void presetRecallAfterSettingParameters(ofJson &) override{
        loadingPreset = false;
        updateBlendModeFromParameters();
        render();
    }
    
    void deactivate(){
        fbo.clear();
        output = nullptr;
    }
    
private:
    void render();
    void configureCamera();
    void applyBlendMode(int mode);
    void updateBlendModeFromParameters();

    ofEventListener listener;
    ofEventListeners blendModeListeners;
    ofEventListeners cameraListeners;

    ofParameter<int> blendMode;
    ofParameter<int> layerOrder;
    
    ofParameter<int> width;
    ofParameter<int> height;
    
    ofParameter<std::vector<ofTexture*>> input;
    ofParameter<std::vector<glm::mat4>> transformInput;
    
    ofParameter<int> blendSrcColorFunction;
    ofParameter<int> blendSrcAlphaFunction;
    ofParameter<int> blendDstColorFunction;
    ofParameter<int> blendDstAlphaFunction;
    ofParameter<int> blendColorEquation;
    ofParameter<int> blendAlphaEquation;
    ofParameter<ofFloatColor> blendColor;
    
    ofParameter<std::vector<float>> opacity;
    ofParameter<std::vector<float>> alpha;
    
    ofParameter<ofTexture*> output;
    
    ofFbo fbo;
    ofCamera camera;
    ofParameter<int> cameraProjection;
    ofParameter<float> cameraFov;
    ofParameter<bool> cameraAutoDistance;
    ofParameter<float> cameraDistance;
    ofParameter<float> cameraNearClip;
    ofParameter<float> cameraFarClip;
    
    ofParameter<bool> active;

    bool updatingBlendMode = false;
    bool loadingPreset = false;
};
#endif /* textureBlender_h */
