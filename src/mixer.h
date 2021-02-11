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
        addInspectorParameter(numColors.set("Num Textures", 5, 2, 20));
        //addParameter(input.set("Input", nullptr));
        addOutputParameter(output.set("Output", nullptr));
        
        inputs.resize(numColors);
        blendmodes.resize(numColors, 0);
        opacities.resize(numColors, 1);
        textures.resize(numColors, nullptr);
        
        auto vector_getter = [](void* vec, int idx, const char** out_text)
        {
            auto& vector = *static_cast<std::vector<std::string>*>(vec);
            if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
            *out_text = vector.at(idx).c_str();
            return true;
        };
        
        
        for(int i = 0; i < numColors; i++){
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
        int i = numColors-1;
        ofTexture* bottom;
        bottom = textures[numColors-1];
        while(bottom == nullptr){
            i--;
            if(i < 0) break;
            bottom = textures[i];
        }
        if(bottom != nullptr){
            ofPushStyle();
            ofSetColor(255, 255, 255, 255);
            int fboIndex = 0;
            if(!fbos[0].isAllocated() || fbos[0].getWidth() != bottom->getWidth() || fbos[0].getHeight() != bottom->getHeight()){
                ofFbo::Settings fboSettings;
                fboSettings.width = bottom->getWidth();
                fboSettings.height = bottom->getHeight();
                fboSettings.internalformat = GL_RGBA;
                //fboSettings.numSamples = 4;
                fboSettings.useDepth = false;
                fboSettings.useStencil = false;
                fboSettings.textureTarget = GL_TEXTURE_2D;
                //fboSettings.minFilter = GL_LINEAR;
                //fboSettings.maxFilter = GL_LINEAR;
                //fboSettings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
                //fboSettings.wrapModeVertical = GL_CLAMP_TO_EDGE;
                fbos[0].allocate(fboSettings);
                fbos[1].allocate(fboSettings);
            }
            
            fbos[!fboIndex].begin();
            ofSetColor(255, 255, 255, 255*opacities[i]);
            bottom->draw(0, 0);
            fbos[!fboIndex].end();
            ofSetColor(255, 255, 255, 255);
            
            for(i-1; i >= 0; i--){
                ofTexture* up;
                up = textures[i];
                while(up == nullptr){
                    i--;
                    if(i < 0) break;
                    up = textures[i];
                }
                if(up != nullptr){
                    fbos[fboIndex].begin();
                    shader.begin();
                    ofSetColor(255, 255, 255, 255);
                    shader.setUniformTexture("base", fbos[!fboIndex].getTexture(), 1);
                    shader.setUniformTexture("blendTgt", *up, 2);
                    shader.setUniform1i("mode", blendmodes[i]);
                    //shader.setUniform1f("opacity", opacities[i]);
                    ofDrawRectangle(0, 0, fbos[fboIndex].getWidth(), fbos[fboIndex].getHeight());
                    shader.end();
                    fbos[fboIndex].end();
                    
                    fbos[!fboIndex].begin();
                    ofEnableAlphaBlending();
                    ofSetColor(255, 255, 255, 255*opacities[i]);
                    fbos[fboIndex].draw(0, 0);
                    ofDisableAlphaBlending();
                    fbos[!fboIndex].end();
                    
                    //fboIndex = !fboIndex;
                }
            }
            output = &fbos[!fboIndex].getTexture();
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
    
    ofParameter<int> numColors;
    vector<customGuiRegion> inputs;
    vector<ofTexture*> textures;
    vector<int> blendmodes;
    vector<float> opacities;
    
    ofFbo fbos[2];
};

#endif /* mixer_h */
