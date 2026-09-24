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
        addParameter(x.set("X", std::vector<int>{0}, std::vector<int>{0}, std::vector<int>{INT_MAX}));
        addParameter(y.set("Y", std::vector<int>{0}, std::vector<int>{0}, std::vector<int>{INT_MAX}));
        addParameter(width.set("Width", 100, 1, INT_MAX));
        addParameter(height.set("Height", 100, 1, INT_MAX));
        addOutputParameter(output.set("Output", nullptr));
    }
    
    void draw(ofEventArgs &a){
        if(input.get() != nullptr){
            const auto &xValues = x.get();
            const auto &yValues = y.get();
            if(xValues.empty() || yValues.empty()){
                output = nullptr;
                return;
            }

            const int outputWidth = width * static_cast<int>(xValues.size());
            const int outputHeight = height * static_cast<int>(yValues.size());
            if(!fbo.isAllocated() || outputWidth != fbo.getWidth() || outputHeight != fbo.getHeight()){
                fbo.allocate(outputWidth, outputHeight, GL_RGBA32F);
                fbo.getTexture().setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
            }
            fbo.begin();
            ofClear(0, 0, 0, 255);
            for(std::size_t row = 0; row < yValues.size(); ++row){
                for(std::size_t column = 0; column < xValues.size(); ++column){
                    input.get()->drawSubsection(column * width, row * height, width, height,
                                                xValues[column], yValues[row], width, height);
                }
            }
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
    ofParameter<std::vector<int>> x, y;
    ofParameter<int> width, height;
    ofParameter<ofTexture*> output;
    ofFbo fbo;
    
    ofEventListener listener;
};

#endif /* subTexture_h */
