//
//  vectorToTexture.h
//  ofxOceanodeTextures
//
//  Created by Eduard Frigola Bagué on 08/02/2021.
//

#ifndef vectorToTexture_h
#define vectorToTexture_h

#include "ofxOceanodeNodeModel.h"

class vectorToTexture : public ofxOceanodeNodeModel{
public:
    vectorToTexture() : ofxOceanodeNodeModel("Vector to Texture"){};
    
    void setup(){
        addParameter(input.set("Input", {0}, {0}, {1}));
        addParameter(width.set("Width", 32, 1, INT_MAX));
        addParameter(height.set("Height", 4, 1, INT_MAX));
        addOutputParameter(output.set("Output Tex", nullptr));
    }
    
    void draw(ofEventArgs &a){
        if(!fbo.isAllocated() || fbo.getWidth() != width || fbo.getHeight() != height){
            fbo.allocate(width, height, GL_RGBA32F);
            fbo.getTexture().setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
        }
        vector<float> rgbaInput(input->size()*4, 0);
        for(int i = 0; i < input->size(); i++){
            rgbaInput[(i*4)] = input->at(i);
            rgbaInput[(i*4)+1] = input->at(i);
            rgbaInput[(i*4)+2] = input->at(i);
            rgbaInput[(i*4)+3] = 1.0f;
        }
        if(rgbaInput.size()>0)
        {
            tex.allocate(width, height, GL_RGBA32F);
            tex.loadData(rgbaInput.data(), width, height, GL_RGBA);
            tex.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
            fbo.begin();
            ofClear(0, 0, 0, 255);
            tex.draw(0, 0, width, height);
            fbo.end();
            output = &fbo.getTexture();
        }
    }
    
    void loadBeforeConnections(ofJson &json){
        deserializeParameter(json, width);
        deserializeParameter(json, height);
    }
    
    void deactivate(){
        fbo.clear();
        output = nullptr;
    }
    
private:
    ofParameter<vector<float>> input;
    ofParameter<ofTexture*> output;
    ofParameter<int> width, height;
    ofTexture tex;
    ofFbo fbo;
    
    ofEventListener listener;
};

#endif /* vectorToTexture_h */
