//
//  vectorToTexture.h
//  ofxOceanodeTextures
//
//  Created by Eduard Frigola Bagué on 08/02/2021.
//

#ifndef vectorToTexture_h
#define vectorToTexture_h

#include "ofxOceanodeNodeModel.h"
#include "ofxOceanodeShared.h"

class vectorToTexture : public ofxOceanodeNodeModel{
public:
    vectorToTexture() : ofxOceanodeNodeModel("Vector to Texture"){};
    
    void setup(){
        addParameter(input.set("Input", {0}, {0}, {1}));
        addParameter(width.set("Width", 32, 1, INT_MAX));
        addParameter(height.set("Height", 4, 1, INT_MAX));
        addOutputParameter(output.set("Output Tex", nullptr));
        listener = input.newListener([this](vector<float>&){ uploadDirty = true; });
    }
    
    void draw(ofEventArgs &a){
        if(textureLimit == 0) glGetIntegerv(GL_MAX_TEXTURE_SIZE, &textureLimit);
        if(width.get()<1 || height.get()<1 || width.get()>textureLimit || height.get()>textureLimit){
            output = nullptr;
            return;
        }
        const int format = ofxOceanodeShared::getColorTextureInternalFormat();
        if(!tex.isAllocated() || tex.getWidth() != width || tex.getHeight() != height || tex.getTextureData().glInternalFormat != format){
            tex.allocate(width, height, format);
            tex.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
            uploadDirty = true;
        }
        if(!fbo.isAllocated() || fbo.getWidth() != width || fbo.getHeight() != height || fbo.getTexture().getTextureData().glInternalFormat != format){
            // Fresh settings avoid retaining the previous attachment colorFormats.
            ofFbo::Settings settings;
            settings.width = width; settings.height = height;
            settings.internalformat = format;
            settings.useDepth = false; settings.useStencil = false;
            fbo.allocate(settings);
            fbo.getTexture().setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
        }
        if(uploadDirty){
            const size_t count = size_t(width.get()) * height.get();
            rgbaInput.assign(count * 4, 0.0f);
            const auto& values = input.get();
            for(size_t i = 0; i < std::min(values.size(), count); ++i){
                rgbaInput[4*i] = rgbaInput[4*i+1] = rgbaInput[4*i+2] = values[i];
                rgbaInput[4*i+3] = 1.0f;
            }
            tex.loadData(rgbaInput.data(), width, height, GL_RGBA);
            uploadDirty = false;
        }
        // Retain the original compositing pass: it defines padded pixels' alpha
        // and respects the host's blend/tint state. Allocation/upload storage is reused.
        fbo.begin();
        ofClear(0, 0, 0, 255);
        tex.draw(0, 0, width, height);
        fbo.end();
        output = &fbo.getTexture();
    }

    void loadBeforeConnections(ofJson &json){
        deserializeParameter(json, width);
        deserializeParameter(json, height);
    }
    
    void deactivate(){
        tex.clear();
        fbo.clear();
        output = nullptr;
    }
    
private:
    ofParameter<vector<float>> input;
    ofParameter<ofTexture*> output;
    ofParameter<int> width, height;
    ofTexture tex;
    ofFbo fbo;
    vector<float> rgbaInput;
    GLint textureLimit = 0;
    bool uploadDirty = true;
    
    ofEventListener listener;
};

#endif /* vectorToTexture_h */
