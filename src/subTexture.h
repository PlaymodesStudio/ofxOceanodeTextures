//
//  subTexture.h
//  ofxOceanodeTextures
//
//  Created by Eduard Frigola Bagué on 08/02/2021.
//

#ifndef subTexture_h
#define subTexture_h

#include "ofxOceanodeNodeModel.h"

class subTexture : public ofxOceanodeNodeModel{
public:
    subTexture() : ofxOceanodeNodeModel("SubTexture"){};
    
    void setup(){
        addParameter(input.set("Input", nullptr));
        addParameter(x.set("X", 0, 0, INT_MAX));
        addParameter(y.set("Y", 0, 0, INT_MAX));
        addParameter(width.set("Width", 100, 1, INT_MAX));
        addParameter(height.set("Height", 100, 1, INT_MAX));
        addOutputParameter(output.set("Output", nullptr));
    }
    
    void draw(ofEventArgs &a){
        if(input.get() != nullptr){
            if(!fbo.isAllocated() || width != fbo.getWidth() || height != fbo.getHeight()){
                fbo.allocate(width, height, GL_RGBA32F);
                fbo.getTexture().setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
            }
            fbo.begin();
            ofClear(0, 0, 0, 255);
            input.get()->drawSubsection(0, 0, width, height, x, y, width, height);
            fbo.end();
            output = &fbo.getTexture();
        }
    }
    
    void deactivate(){
        fbo.clear();
        output = nullptr;
    }
    
private:
    ofParameter<ofTexture*> input;
    ofParameter<int> x, y, width, height;
    ofParameter<ofTexture*> output;
    ofFbo fbo;
    
    ofEventListener listener;
};

#endif /* subTexture_h */
