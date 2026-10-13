#ifndef textureFlip_h
#define textureFlip_h

#include "ofxOceanodeNodeModel.h"

class textureFlip : public ofxOceanodeNodeModel {
public:
    textureFlip() : ofxOceanodeNodeModel("Texture Flip"){};
    ~textureFlip(){};

    void setup() override {
        description = "Flips the input texture in the horizontal or vertical axis based on the toggles.";

        addParameter(input.set("Input", nullptr));
        addParameter(flipH.set("H", false));  // Horizontal flip toggle
        addParameter(flipV.set("V", false));  // Vertical flip toggle
        addOutputParameter(output.set("Output", nullptr));

        inputSize = glm::vec2(0,0);
    }

    void draw(ofEventArgs &a) {
        if(input != nullptr) {
            if(inputSize.x != input.get()->getWidth() || inputSize.y != input.get()->getHeight()) {
                inputSize.x = input.get()->getWidth();
                inputSize.y = input.get()->getHeight();

                // Allocate the FBO to match the input texture size
                ofFbo::Settings settings;
                settings.width = inputSize.x;
                settings.height = inputSize.y;
                settings.internalformat = GL_RGBA32F;
                settings.maxFilter = GL_NEAREST;
                settings.minFilter = GL_NEAREST;
                settings.numColorbuffers = 1;
                settings.useDepth = false;
                settings.useStencil = false;
                settings.textureTarget = GL_TEXTURE_2D;

                fbo.allocate(settings);
            }

            fbo.begin();
            ofPushMatrix();
            ofTranslate(flipH ? fbo.getWidth() : 0, flipV ? fbo.getHeight() : 0);
            ofScale(flipH ? -1 : 1, flipV ? -1 : 1);
            input.get()->draw(0, 0);
            ofPopMatrix();
            fbo.end();

            output = &fbo.getTexture();
        }
    }


private:
    ofParameter<ofTexture*> input;
    ofParameter<bool> flipH;
    ofParameter<bool> flipV;
    ofParameter<ofTexture*> output;

    ofFbo fbo;
    glm::vec2 inputSize;
};

#endif /* textureFlip_h */
