//
//  mixer.h
//  lille
//
//  Created by Eduard Frigola Bagué on 05/02/2021.
//

#ifndef mixer_h
#define mixer_h

#include "ofxOceanodeNodeModel.h"
#include "imgui.h"

#define STRINGIFY(A) #A

class mixer : public ofxOceanodeNodeModel {
public:
    mixer() : ofxOceanodeNodeModel("Mixer"){};
    
    void setup(){
        addInspectorParameter(numTextures.set("Num Textures", 5, 2, 20));
        addParameter(width.set("Width", 100, 1, 50000));
        addParameter(height.set("Height", 100, 1, 50000));
        //addParameter(input.set("Input", nullptr));
        addOutputParameter(output.set("Output", nullptr));
        
        inputs.resize(numTextures);
        blendmodes.resize(numTextures, 0);
        opacities.resize(numTextures, 1);
        textures.resize(numTextures, nullptr);
        
        auto vector_getter = [](void* vec, int idx, const char** out_text)
        {
            auto& vector = *static_cast<std::vector<std::string>*>(vec);
            if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
            *out_text = vector.at(idx).c_str();
            return true;
        };
        
        
        for(int i = 0; i < numTextures; i++){
            auto parameterRef = addParameter(inputs[i].set("In " + ofToString(i), [i, vector_getter, this](){
                ImGui::SetNextItemWidth(250);
                //ImGui::Separator();
                ImGui::Text("%s", ("Layer " + ofToString(i)).c_str());
                
                vector<string> options = {"Normal",
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
                    "Luminosity"};
                ImGui::Combo("##Dropdown", &blendmodes[i], vector_getter, static_cast<void*>(&options), options.size());
                ImGui::SameLine();
                ImGui::SliderFloat("##Slider", &opacities[i], 0, 1);
            }));
            
            parameterRef->addReceiveFunc<ofTexture*>([this, i](ofTexture *const &tex){
                textures[i] = (ofTexture*)tex;
            });
            
            parameterRef->addDisconnectFunc([this, i](){
                textures[i] = nullptr;
            });
        }
        
        string defaultVertSource =
        #include "defaultVertexShader.h"
        ;

        string drawFragSource =
        #include "mixerShader.h"
        ;
                                    
        shader.setupShaderFromSource(GL_VERTEX_SHADER, defaultVertSource);
        shader.setupShaderFromSource(GL_FRAGMENT_SHADER, drawFragSource);
        shader.bindDefaults();
        shader.linkProgram();
    }
    
    void draw(ofEventArgs &a){
        int i = numTextures-1;
        ofTexture* bottom;
        bottom = textures[numTextures-1];
        while(bottom == nullptr){
            i--;
            if(i < 0) break;
            bottom = textures[i];
        }
        if(bottom != nullptr){
            ofPushStyle();
            ofSetColor(255, 255, 255, 255);
            int fboIndex = 0;
            if(!baseFbo.isAllocated() || baseFbo.getWidth() != width || baseFbo.getHeight() != height){
                ofFbo::Settings fboSettings;
                fboSettings.width = width;
                fboSettings.height = height;
                fboSettings.internalformat = GL_RGBA;
                //fboSettings.numSamples = 4;
                fboSettings.useDepth = false;
                fboSettings.useStencil = false;
                fboSettings.textureTarget = GL_TEXTURE_2D;
                fboSettings.maxFilter = GL_NEAREST;
                fboSettings.minFilter = GL_NEAREST;
                //fboSettings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
                //fboSettings.wrapModeVertical = GL_CLAMP_TO_EDGE;
                baseFbo.allocate(fboSettings);
                canvasFbo.allocate(fboSettings);
            }
            
            baseFbo.begin();
            ofSetColor(0, 0, 0, 255);
            ofDrawRectangle(0, 0, width, height);
            ofSetColor(255, 255, 255, 255*opacities[i]);
            bottom->draw(0, 0, width, height);
            baseFbo.end();
            ofSetColor(255, 255, 255, 255);
            
            for(i--; i >= 0; i--){
                ofTexture* up;
                up = textures[i];
                while(up == nullptr){
                    i--;
                    if(i < 0) break;
                    up = textures[i];
                }
                if(up != nullptr){
//                    targetFbo.begin();
//                    up->draw(0, 0, width, height);
//                    targetFbo.end();
                    canvasFbo.begin();
                    shader.begin();
                    ofSetColor(255, 255, 255, 255);
                    shader.setUniformTexture("base", baseFbo.getTexture(), 1);
                    shader.setUniformTexture("blendTgt", *up, 2);
                    shader.setUniform1i("mode", blendmodes[i]);
                    //shader.setUniform1f("opacity", opacities[i]);
                    ofDrawRectangle(0, 0, width, height);
                    shader.end();
                    canvasFbo.end();
                    
                    baseFbo.begin();
                    ofEnableAlphaBlending();
                    ofSetColor(255, 255, 255, 255*opacities[i]);
                    canvasFbo.draw(0, 0);
                    ofDisableAlphaBlending();
                    baseFbo.end();
                }
            }
            output = &baseFbo.getTexture();
            GLenum err;
            while ((err = glGetError()) != GL_NO_ERROR) {
                ofLog() << "OpenGL error: " << err;
            }
            ofPopStyle();
        }
    }
    
private:
    ofShader shader;
    
    ofParameter<ofTexture*> output;
    ofParameter<int> width, height;
    ofParameter<int> numTextures;
    vector<customGuiRegion> inputs;
    vector<ofTexture*> textures;
    vector<int> blendmodes;
    vector<float> opacities;
    
    ofFbo baseFbo, canvasFbo;//, targetFbo;
};

#endif /* mixer_h */
