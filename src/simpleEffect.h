//
//  simpleEffect.h
//  17820_Cam
//
//  Created by Eduard Frigola Bagué on 26/9/22.
//

#ifndef simpleEffect_h
#define simpleEffect_h

#include "ofMain.h"
#include "ofxOceanodeNodeModel.h"

class simpleEffect : public ofxOceanodeNodeModel {
public:
    simpleEffect(std::string name, std::string config) : effectName(name), conf(config), ofxOceanodeNodeModel(name){};
    ~simpleEffect(){
        
    }
    
    void setup(){
        blackTexture.allocate(1, 1, GL_RGBA32F);
        
        addParameter(input.set("Input", nullptr));
        addParameter(bypass.set("Bypass",false));
        addEffectParameters();
        addOutputParameter(output.set("Output", nullptr));
        
        addInspectorParameter(drawOnEvent.set("Draw On Event", false));
        
        string defaultVertSource =
        #include "defaultVertexShader.h"
        ;
        
        listener = input.newListener([this](ofTexture* &tex){
            if(drawOnEvent){
                compute();
            }
        });
                                    
        shader.setupShaderFromSource(GL_VERTEX_SHADER, defaultVertSource);
        shader.setupShaderFromFile(GL_FRAGMENT_SHADER, "Effects/" + effectName + ".glsl");
        shader.bindDefaults();
        shader.linkProgram();
    }
    
    void draw(ofEventArgs &a){
        if(!drawOnEvent){
            compute();
        }
    }
    
    void compute(){
        if(input.get() != nullptr){
            if(!fbo.isAllocated() || fbo.getWidth() != input.get()->getWidth() || fbo.getHeight() != input.get()->getHeight()){
                ofFbo::Settings settings;
                settings.height = input.get()->getHeight();
                settings.width = input.get()->getWidth();
                settings.internalformat = GL_RGBA32F;
                settings.maxFilter = GL_NEAREST;
                settings.minFilter = GL_NEAREST;
                settings.numColorbuffers = 1;
                settings.useDepth = false;
                settings.useStencil = false;
                settings.textureTarget = GL_TEXTURE_2D;
                
                fbo.allocate(settings);
            }
            
            if(!bypass)
            {
                fbo.begin();
                shader.begin();
                ofClear(0, 0, 0, 255);
                ofPushStyle();
                ofSetColor(255, 255, 255, 255);
                shader.setUniformTexture("tSource", *input.get(), 1);
                bindUniforms();
                ofDrawRectangle(0, 0, fbo.getWidth(), fbo.getHeight());
                ofPopStyle();
                shader.end();
                fbo.end();
                
                output = &fbo.getTexture();
            }
            else
            {
                output = input;
            }
        }
    }
    
    void deactivate(){
        fbo.clear();
//        output = nullptr;
    }
    
    void addEffectParameters(){
        vector<std::string> splittedConfig = ofSplitString(conf, ", ");
        if(conf.empty()){
            splittedConfig.clear();
        }
        int numParams = splittedConfig.size();
        paramTypes.resize(numParams);
        floatParams.resize(numParams);
        colorParams.resize(numParams);
        textures.resize(numParams, nullptr);
        
        for(int i = 0; i < numParams; i++){
            vector<std::string> paramInfo = ofSplitString(splittedConfig[i], ":");
            
            if(paramInfo.size() == 2 && paramInfo[1] == "color"){
                paramTypes[i] = "color";
                addParameter(colorParams[i].set(paramInfo[0], ofFloatColor(1.0, 1.0, 1.0, 1.0), ofFloatColor(0.0, 0.0, 0.0, 0.0), ofFloatColor(1.0, 1.0, 1.0, 1.0)));
            }else{
                paramTypes[i] = "float";
                auto pRef = addParameter(
                                     floatParams[i].set(
                                                        paramInfo[0],
                                                        paramInfo.size() > 1 ? ofToFloat(paramInfo[1]) : 0.0f,
                                                        paramInfo.size() > 2 ? (paramInfo[2] == "min" ? -FLT_MAX : ofToFloat(paramInfo[2])) : -FLT_MAX,
                                                        paramInfo.size() > 3 ? (paramInfo[3] == "max" ? FLT_MAX : ofToFloat(paramInfo[3])) : FLT_MAX));

                pRef->addReceiveFunc<ofTexture*>([this, i](ofTexture *const &tex){
                    textures[i] = (ofTexture*)tex;
                });
                pRef->addDisconnectFunc([this, i](){
                    textures[i] = nullptr;
                });
            }
        }
    }
    
    void bindUniforms(){
        vector<std::string> splittedConfig = ofSplitString(conf, ", ");
        if(conf.empty()){
            splittedConfig.clear();
        }
        int numParams = splittedConfig.size();
        for(int i = 0; i < numParams; i++){
            vector<std::string> paramInfo = ofSplitString(splittedConfig[i], ":");

            if(paramTypes[i] == "color"){
                shader.setUniform4f(paramInfo[0], colorParams[i]);
            }else{ // It's a float
                shader.setUniformTexture(paramInfo[0] + "Tex", textures[i] != nullptr ? *textures[i] : blackTexture, i+2);
                shader.setUniform1f(paramInfo[0], floatParams[i]);
            }
        }
    }
    
private:
    
    ofEventListener listener;
    
    std::vector<std::string> paramTypes;
    std::vector<ofParameter<float>> floatParams;
    std::vector<ofParameter<ofFloatColor>> colorParams;
    std::vector<ofTexture*> textures;

    ofParameter<ofTexture*> input;
    ofParameter<ofTexture*> output;
    
    ofParameter<bool> drawOnEvent;
    ofParameter<bool> bypass;
    
    ofFbo fbo;
    
    ofTexture blackTexture;
    
    ofShader shader;
    
    std::string effectName;
    std::string conf;
};

#endif /* simpleEffect_h */
