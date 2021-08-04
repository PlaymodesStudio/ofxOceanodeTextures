//
//  textureUnifier.h
//  MIRABCN_Generator
//
//  Created by Eduard Frigola on 22/09/2017.
//
//

#ifndef textureUnifier_h
#define textureUnifier_h

#include "ofxOceanodeNodeModel.h"

class textureUnifier : public ofxOceanodeNodeModel{
public:
    textureUnifier();
    ~textureUnifier(){};
    
    void setup() override;
    
private:
    void computeOutput(ofTexture* &in);
    
    ofEventListeners listeners;
    
    ofParameter<int>    triggerTextureIndex;
    ofParameter<int>    spacing;
    vector<ofParameter<ofTexture*>>    inputs;
    vector<ofParameter<string>> customPositions;
    vector<ofParameter<float>> opacities;
    ofParameter<ofTexture*>    output;
    
    ofFbo outputFbo;
};

#endif /* textureUnifier_h */
