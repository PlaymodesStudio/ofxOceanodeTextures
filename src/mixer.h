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
        addOutputParameter(output.set("Output", nullptr));
        
        inputs.resize(numTextures);
        blendmodes.resize(numTextures, 0);
        opacities.resize(numTextures);
        textures.resize(numTextures, nullptr);
        
        auto createNewParams = [this](int start, int size){
            auto vector_getter = [](void* vec, int idx, const char** out_text)
            {
                auto& vector = *static_cast<std::vector<std::string>*>(vec);
                if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
                *out_text = vector.at(idx).c_str();
                return true;
            };
            
            
            for(int i = start; i < (size+start); i++){
                auto parameterRef = addParameter(inputs[i].set("In " + ofToString(i), [i, vector_getter, this](){
                    ImGui::SetNextItemWidth(250);
					ImGui::Dummy(ImVec2(10, 1));
                    ImGui::Text("%s", ("Layer " + ofToString(i+1, 2, '0')).c_str());
                    ImGui::SameLine(90);
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
                    //ImGui::SameLine();
                    //ImGui::SliderFloat("##Slider", &opacities[i], 0, 1);
                }));
				
				addParameter(opacities[i].set("Opac " + ofToString(i+1), 1, 0, 1));
                
                parameterRef->addReceiveFunc<ofTexture*>([this, i](ofTexture *const &tex){
                    textures[i] = (ofTexture*)tex;
                });
                
                parameterRef->addDisconnectFunc([this, i](){
					if(i < textures.size()){
						textures[i] = nullptr;
					}
                });
				
				
            }
        };
        
        createNewParams(0, numTextures);
        
        
        listener = numTextures.newListener([this, createNewParams](int &i){
            if(inputs.size() != i){
                int oldSize = inputs.size();
                bool remove = oldSize > i;
                inputs.resize(numTextures);
                blendmodes.resize(numTextures, 0);
                opacities.resize(numTextures);
                textures.resize(numTextures, nullptr);
                
                if(remove){
                    for(int j = oldSize-1; j >= i; j--){
                        removeParameter("In " + ofToString(j));
						removeParameter("Opac " + ofToString(j+1));
                    }
                }else{
                    createNewParams(oldSize, i-oldSize);
                }
            }
        });
        
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
        int numActive = 0;
        int activeIndex = 0;
        for(int i = 0; i < opacities.size(); i++){
            if(opacities[i] != 0){
                numActive++;
                activeIndex = i;
            }
        }
        if(numActive == 1 && opacities[activeIndex] == 1){
            output = textures[activeIndex];
        }else{
        int i = numTextures-1;
        ofTexture* bottom = nullptr;
        bottom = textures[numTextures-1];
        while(bottom == nullptr || !bottom->isAllocated() || opacities[i] == 0){
            i--;
            if(i < 0){
//                pingPongFbo[!pingPongIndex].begin();
//                ofClear(0, 0, 0, 255);
//                ofSetColor(0, 0, 0, 255);
//                ofDrawRectangle(0, 0, width, height);
//                pingPongFbo[!pingPongIndex].end();
//                output = &pingPongFbo[pingPongIndex].getTexture();
//                return;
                bottom = nullptr;
                break;
            }
            bottom = textures[i];
        }
            int fboIndex = 0;
            pingPongIndex = 0;
            if(!pingPongFbo[0].isAllocated() || pingPongFbo[0].getWidth() != width || pingPongFbo[0].getHeight() != height){
                ofFbo::Settings fboSettings;
                fboSettings.width = width;
                fboSettings.height = height;
                fboSettings.internalformat = GL_RGBA32F;
                fboSettings.numColorbuffers = 1;
                //fboSettings.numSamples = 4;
                fboSettings.useDepth = false;
                fboSettings.useStencil = false;
                fboSettings.textureTarget = GL_TEXTURE_2D;
                fboSettings.maxFilter = GL_NEAREST;
                fboSettings.minFilter = GL_NEAREST;
                //fboSettings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
                //fboSettings.wrapModeVertical = GL_CLAMP_TO_EDGE;
                pingPongFbo[0].allocate(fboSettings);
                pingPongFbo[1].allocate(fboSettings);
            }
        if(bottom != nullptr){
            ofPushStyle();
            ofSetColor(255, 255, 255, 255);
            
            pingPongFbo[pingPongIndex].begin();
			ofClear(0, 0, 0, 255);
			ofEnableAlphaBlending();
            ofSetColor(0, 0, 0, 255);
            ofDrawRectangle(0, 0, width, height);
            ofSetColor(255, 255, 255, 255*opacities[i]);
            bottom->draw(0, 0, width, height);
			ofDisableBlendMode();
            pingPongFbo[pingPongIndex].end();
            ofSetColor(255, 255, 255, 255);
            
            for(i--; i >= 0; i--){
                ofTexture* up = nullptr;
                up = textures[i];
                while(up == nullptr || !up->isAllocated() || opacities[i] == 0){
                    i--;
                    if(i < 0) break;
                    up = textures[i];
                }
                if(i >= 0 && up != nullptr && up->isAllocated() && opacities[i] != 0){
                    pingPongFbo[!pingPongIndex].begin();
					ofClear(0, 0, 0,  255);
                    shader.begin();
                    ofSetColor(255, 255, 255, 255);
                    shader.setUniformTexture("base", pingPongFbo[pingPongIndex].getTexture(), 1);
                    shader.setUniformTexture("blendTgt", *up, 2);
                    shader.setUniform1i("mode", blendmodes[i]);
                    shader.setUniform1f("opacity", opacities[i]);
                    ofDrawRectangle(0, 0, width, height);
                    shader.end();
                    pingPongFbo[!pingPongIndex].end();
					
					pingPongIndex = !pingPongIndex;
                }
            }
            output = &pingPongFbo[pingPongIndex].getTexture();
            GLenum err;
            while ((err = glGetError()) != GL_NO_ERROR) {
                ofLog() << "OpenGL error: " << err;
            }
            ofPopStyle();
        }else{
            pingPongFbo[pingPongIndex].begin();
            ofClear(0, 0, 0, 255);
            ofSetColor(0, 0, 0, 255);
            ofDrawRectangle(0, 0, width, height);
            pingPongFbo[pingPongIndex].end();
            output = &pingPongFbo[pingPongIndex].getTexture();
        }
        }
    }
    
    void presetSave(ofJson &json){
        for(int i = 0; i < numTextures; i++){
            json["LayerInfo"][i]["Blend"] = blendmodes[i];
//            json["LayerInfo"][i]["Opacity"] = opacities[i];
        }
    }
	
	void loadBeforeConnections(ofJson &json){
		deserializeParameter(json, numTextures);
	}
    
    void presetRecallAfterSettingParameters(ofJson &json){
        for(int i = 0; i < numTextures; i++){
            try{
                blendmodes[i] = json["LayerInfo"][i]["Blend"];
            }catch (ofJson::exception& e)
            {
                ofLog() << e.what();
            }
        }
    }
    
    void deactivate(){
        baseFbo.clear();
        canvasFbo.clear();
        pingPongFbo[0].clear();
        pingPongFbo[1].clear();
        output = nullptr;
    }
    
private:
    ofShader shader;
    
    ofParameter<ofTexture*> output;
    ofParameter<int> width, height;
    ofParameter<int> numTextures;
    vector<customGuiRegion> inputs;
	vector<ofParameter<float>> opacities;
    vector<ofTexture*> textures;
    vector<int> blendmodes;
//    vector<float> opacities;
    
    ofEventListener listener;
    
    ofFbo baseFbo, canvasFbo;
	ofFbo pingPongFbo[2];
	int pingPongIndex;
};

#endif /* mixer_h */
