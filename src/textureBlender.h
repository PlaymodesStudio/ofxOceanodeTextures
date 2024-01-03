//
//  textureBlender.h
//  ofxOceanodeTextures
//
//  Created by Eduard Frigola on 22/12/23.
//

#ifndef textureBlender_h
#define textureBlender_h

#include "ofxOceanodeNodeModel.h"

class textureBlender : public ofxOceanodeNodeModel{
public:
    textureBlender();
    
    void setup() override;
    
private:
    ofEventListener listener;
    
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
    
    ofParameter<std::vector<float>> opacity;
    ofParameter<std::vector<float>> alpha;
    
    ofParameter<ofTexture*> output;
    
    ofFbo fbo;
};
#endif /* textureBlender_h */
