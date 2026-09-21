//
//  textureComposerSimple.h
//  ofxOceanodeTextures
//

#ifndef textureComposerSimple_h
#define textureComposerSimple_h

#include "ofxOceanodeNodeModel.h"

class textureComposerSimple : public ofxOceanodeNodeModel {
public:
    textureComposerSimple() : ofxOceanodeNodeModel("Texture Composer Simple"){}

    void setup() override{
        addInspectorParameter(numTextures.set("Num Textures", 2, 1, 32));
        addParameter(width.set("Width", 100, 1, 50000));
        addParameter(height.set("Height", 100, 1, 50000));
        addOutputParameter(output.set("Output", nullptr));

        inputs.resize(numTextures);
        xPositions.resize(numTextures);
        yPositions.resize(numTextures);
        blendModes.resize(numTextures);
        opacities.resize(numTextures);

        const std::vector<std::string> blendOptions = {
            "Normal",
            "Multiply",
            "Average",
            "Add",
            "Substract",
            "Difference",
            "Negation",
            "Exclusion",
            "Screen",
            "Overlay",
            "SoftLight",
            "HardLight",
            "ColorDodge",
            "ColorBurn",
            "LinearLight",
            "VividLight",
            "PinLight",
            "HardMix",
            "Reflect",
            "Glow",
            "Phoenix",
            "Hue",
            "Saturation",
            "Color",
            "Luminosity",
            "Maximum",
            "Minimum"
        };

        auto addLayerParameters = [this, blendOptions](int start, int count){
            for(int i = start; i < start + count; i++){
                const std::string suffix = ofToString(i + 1, 2, '0');

                addParameter(inputs[i].set("In " + suffix, nullptr));
                addParameter(xPositions[i].set("X " + suffix, 0, -50000, 50000));
                addParameter(yPositions[i].set("Y " + suffix, 0, -50000, 50000));
                addParameterDropdown(blendModes[i], "Blend " + suffix, 0, blendOptions);
                addParameter(opacities[i].set("Opacity " + suffix, 1.0f, 0.0f, 1.0f));
            }
        };

        addLayerParameters(0, numTextures);

        listener = numTextures.newListener([this, addLayerParameters](int &newSize){
            const int oldSize = inputs.size();
            if(oldSize == newSize){
                return;
            }

            if(oldSize > newSize){
                for(int i = oldSize - 1; i >= newSize; i--){
                    const std::string suffix = ofToString(i + 1, 2, '0');
                    removeParameter("In " + suffix);
                    removeParameter("X " + suffix);
                    removeParameter("Y " + suffix);
                    removeParameter("Blend " + suffix);
                    removeParameter("Opacity " + suffix);
                }
            }

            inputs.resize(newSize);
            xPositions.resize(newSize);
            yPositions.resize(newSize);
            blendModes.resize(newSize);
            opacities.resize(newSize);

            if(oldSize < newSize){
                addLayerParameters(oldSize, newSize - oldSize);
            }
        });

        const std::string defaultVertexSource =
#include "shaders/defaultVertexShader.h"
        ;
        const std::string blendFragmentSource =
#include "shaders/mixerAlphaShader.h"
        ;

        shader.setupShaderFromSource(GL_VERTEX_SHADER, defaultVertexSource);
        shader.setupShaderFromSource(GL_FRAGMENT_SHADER, blendFragmentSource);
        shader.bindDefaults();
        shader.linkProgram();
    }

    void draw(ofEventArgs &args) override{
        (void)args;

        if(!canvasIsAllocated()){
            allocateCanvas();
        }

        if(!canvasIsAllocated()){
            output = nullptr;
            return;
        }

        pingPongIndex = 0;
        pingPongFbo[pingPongIndex].begin();
        ofClear(0, 0, 0, 0);
        pingPongFbo[pingPongIndex].end();

        bool hasBaseLayer = false;
        for(int i = numTextures - 1; i >= 0; i--){
            // Standard texture connections update the parameter itself.
            ofTexture *texture = inputs[i].get();
            if(texture == nullptr || !texture->isAllocated() || opacities[i] <= 0.0f){
                continue;
            }

            // Render each source at its native size. X/Y use the canvas top-left as origin.
            layerFbo.begin();
            ofClear(0, 0, 0, 0);
            ofPushStyle();
            // Copy the source RGBA values verbatim. Most texture-producing
            // nodes render into a transparent FBO, so their RGB is already
            // weighted by alpha. Alpha blending here would premultiply the
            // texture a second time before it even reaches the blend shader.
            ofDisableAlphaBlending();
            ofSetColor(255, 255, 255, 255);
            texture->draw(xPositions[i], yPositions[i]);
            ofPopStyle();
            layerFbo.end();

            pingPongFbo[!pingPongIndex].begin();
            ofClear(0, 0, 0, 0);
            ofPushStyle();
            // The shader computes the complete RGBA result. Fixed-function
            // alpha blending here would composite that result onto the newly
            // cleared FBO once more. It is especially destructive for empty
            // upper layers: even when the shader returns baseCol unchanged,
            // GL_SRC_ALPHA would multiply it by its own alpha again.
            ofDisableAlphaBlending();
            shader.begin();
            shader.setUniformTexture("base", pingPongFbo[pingPongIndex].getTexture(), 0);
            shader.setUniformTexture("blendTgt", layerFbo.getTexture(), 1);
            // The first visible layer has no base to blend against, so it is composited normally.
            shader.setUniform1i("mode", hasBaseLayer ? blendModes[i].get() : 0);
            shader.setUniform1f("opacity", opacities[i].get());
            shader.setUniform1i("premultipliedInput", 1);
            ofDrawRectangle(0, 0, width, height);
            shader.end();
            ofPopStyle();
            pingPongFbo[!pingPongIndex].end();

            unbindTextureUnits();
            pingPongIndex = !pingPongIndex;
            hasBaseLayer = true;
        }

        output = &pingPongFbo[pingPongIndex].getTexture();
    }

    void deactivate(){
        layerFbo.clear();
        pingPongFbo[0].clear();
        pingPongFbo[1].clear();
        output = nullptr;
    }

    void loadBeforeConnections(ofJson &json){
        deserializeParameter(json, numTextures);
    }

private:
    bool canvasIsAllocated() const{
        return pingPongFbo[0].isAllocated() &&
               pingPongFbo[1].isAllocated() &&
               layerFbo.isAllocated() &&
               pingPongFbo[0].getWidth() == width &&
               pingPongFbo[0].getHeight() == height &&
               layerFbo.getWidth() == width &&
               layerFbo.getHeight() == height;
    }

    void allocateCanvas(){
        ofFbo::Settings settings;
        settings.width = width;
        settings.height = height;
        // Composition does not need float render targets. RGBA8 keeps very large
        // canvases practical (three canvas-sized FBOs are used while composing).
        settings.internalformat = GL_RGBA8;
        settings.numColorbuffers = 1;
        settings.useDepth = false;
        settings.useStencil = false;
        settings.textureTarget = GL_TEXTURE_2D;
        settings.maxFilter = GL_NEAREST;
        settings.minFilter = GL_NEAREST;

        pingPongFbo[0].allocate(settings);
        pingPongFbo[1].allocate(settings);
        layerFbo.allocate(settings);
    }

    void unbindTextureUnits(){
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
    }

    ofShader shader;

    ofParameter<int> numTextures;
    ofParameter<int> width;
    ofParameter<int> height;
    ofParameter<ofTexture*> output;

    std::vector<ofParameter<ofTexture*>> inputs;
    std::vector<ofParameter<int>> xPositions;
    std::vector<ofParameter<int>> yPositions;
    std::vector<ofParameter<int>> blendModes;
    std::vector<ofParameter<float>> opacities;

    ofEventListener listener;

    ofFbo layerFbo;
    ofFbo pingPongFbo[2];
    int pingPongIndex = 0;
};

#endif /* textureComposerSimple_h */
