//
//  ColorByValue.h
//  ofxOceanodeTextures
//
//  Created by Roo on 26/01/2026.
//

#ifndef ColorByValue_h
#define ColorByValue_h

#include "ofxOceanodeNodeModel.h"

class ColorByValue : public ofxOceanodeNodeModel {
public:
    ColorByValue() : ofxOceanodeNodeModel("Color By Value") {}

    void setup() override {
        addParameter(valuesIn.set("Values.In", {0.0f}, {0.0f}, {1.0f}));
        addParameter(texIn.set("Tex.In", nullptr));
        addParameter(width.set("Width", 100, 1, 4096));
        addOutputParameter(output.set("Output", nullptr));

        string defaultVertSource =
        #include "defaultVertexShader.h"
        ;

        string fragSource =
        #include "ColorByValueShader.h"
        ;

        shader.setupShaderFromSource(GL_VERTEX_SHADER, defaultVertSource);
        shader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragSource);
        shader.bindDefaults();
        shader.linkProgram();
    }

    void update(ofEventArgs &a) override {
        if(valuesIn.get().empty()) return;
        
        int numValues = valuesIn.get().size();
        // Check if buffer needs reallocation (size changed)
        if(!valuesTexture.isAllocated() || valuesBuffer.size() != numValues * sizeof(float)){
            valuesBuffer.allocate();
            valuesBuffer.bind(GL_TEXTURE_BUFFER);
            valuesBuffer.setData(valuesIn.get(), GL_STREAM_DRAW);
            valuesTexture.allocateAsBufferTexture(valuesBuffer, GL_R32F);
        } else {
            valuesBuffer.updateData(0, valuesIn.get());
        }
    }

    void draw(ofEventArgs &a) override {
        if (texIn.get() == nullptr || valuesIn.get().empty()) return;
        if (!valuesTexture.isAllocated()) return;

        int outW = width;
        int outH = valuesIn.get().size();

        if (!fbo.isAllocated() || fbo.getWidth() != outW || fbo.getHeight() != outH) {
            ofFbo::Settings settings;
            settings.width = outW;
            settings.height = outH;
            settings.internalformat = GL_RGBA32F;
            settings.maxFilter = GL_NEAREST;
            settings.minFilter = GL_NEAREST;
            settings.numColorbuffers = 1;
            settings.useDepth = false;
            settings.useStencil = false;
            settings.textureTarget = GL_TEXTURE_2D;

            fbo.allocate(settings);
            fbo.begin();
            ofClear(0,0,0,0);
            fbo.end();
        }

        fbo.begin();
        ofClear(0, 0, 0, 0);
        shader.begin();
        shader.setUniformTexture("tex", *texIn.get(), 0);
        shader.setUniformTexture("values", valuesTexture, 1);
        
        ofDrawRectangle(0, 0, outW, outH);
        
        shader.end();
        fbo.end();

        output = &fbo.getTexture();
    }
    
    void deactivate() override {
        fbo.clear();
//        valuesTexture.clear();
        output = nullptr;
    }

private:
    ofParameter<vector<float>> valuesIn;
    ofParameter<ofTexture*> texIn;
    ofParameter<int> width;
    ofParameter<ofTexture*> output;

    ofShader shader;
    ofFbo fbo;
    
    ofTexture valuesTexture;
    ofBufferObject valuesBuffer;
};

#endif /* ColorByValue_h */
