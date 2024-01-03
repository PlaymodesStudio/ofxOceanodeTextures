//
//  textureComposer.h
//  ofxOceanodeTextures
//
//  Created by Eduard Frigola on 22/12/23.
//

#ifndef textureComposer_h
#define textureComposer_h

#include "ofxOceanodeNodeModel.h"

class textureComposer : public ofxOceanodeNodeModel
{
public:
    textureComposer();
    virtual ~textureComposer();

    void setup() override;
    
    void calculate();

private:
    ofParameter<int> width;
    ofParameter<int> height;
    
    ofParameter<std::vector<ofTexture*>> input;
    ofParameter<std::vector<glm::mat4>> transformInput;
    vector<ofParameter<vector<float>>> anchor;
    vector<ofParameter<vector<float>>> position;
    vector<ofParameter<vector<float>>> rotation;
    vector<ofParameter<vector<float>>> scale;
    
    ofParameter<bool> normalizedAnchor;
    ofParameter<bool> normalizedPosition;

    ofParameter<std::vector<ofTexture*>> output;
    ofParameter<std::vector<glm::mat4>> transformOutput;

    template <typename T>
    T getValueForPosition(const ofParameter<vector<T>> &param, int index){
        if(param->size() == 1 || param->size() <= index){
            return param.get()[0];
        }
        else{
            return param.get()[index];
        }
    }

    ofEventListeners listeners;
};

#endif /* textureComposer_h */
