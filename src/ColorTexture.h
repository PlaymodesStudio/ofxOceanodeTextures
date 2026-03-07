//
//  ColorTexture.h
//  ofxOceanodeTextures
//
//  Created by Eloi Maduell
//

#ifndef ColorTexture_h
#define ColorTexture_h

#include "ofxOceanodeNodeModel.h"

class ColorTexture : public ofxOceanodeNodeModel {
public:
    ColorTexture() : ofxOceanodeNodeModel("Color Texture"){};

    void setup() override {
        addParameter(resX.set("Res X", 100, 1, 50000));
        addParameter(resY.set("Res Y", 100, 1, 50000));
        addParameter(color.set("Color", ofFloatColor(1.0f, 1.0f, 1.0f, 1.0f)));
        addOutputParameter(output.set("Output", nullptr));
    }

    void draw(ofEventArgs &a) {
        if (!fbo.isAllocated() || fbo.getWidth() != resX || fbo.getHeight() != resY) {
            ofFbo::Settings fboSettings;
            fboSettings.width = resX;
            fboSettings.height = resY;
            fboSettings.internalformat = GL_RGBA32F;
            fboSettings.useDepth = false;
            fboSettings.useStencil = false;
            fboSettings.textureTarget = GL_TEXTURE_2D;
            fboSettings.maxFilter = GL_NEAREST;
            fboSettings.minFilter = GL_NEAREST;
            fboSettings.numColorbuffers = 1;
            fbo.allocate(fboSettings);
        }

        fbo.begin();
        ofClear(color.get().r * 255.0f, color.get().g * 255.0f, color.get().b * 255.0f, color.get().a * 255.0f);
        fbo.end();

        output = &fbo.getTexture();
    }

    void deactivate() {
        fbo.clear();
        output = nullptr;
    }

private:
    ofParameter<int> resX;
    ofParameter<int> resY;
    ofParameter<ofFloatColor> color;
    ofParameter<ofTexture*> output;

    ofFbo fbo;
};

#endif /* ColorTexture_h */
