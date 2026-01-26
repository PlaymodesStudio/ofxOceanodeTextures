//
//  Gradient2.h
//  ofxOceanodeTextures
//
//  Created for gradient mapping with separate position parameters and alpha preservation
//

#ifndef Gradient2_h
#define Gradient2_h

#include "ofxOceanodeNodeModel.h"

#define STRINGIFY(A) #A

class Gradient2 : public ofxOceanodeNodeModel {
public:
    Gradient2() : ofxOceanodeNodeModel("Gradient2"){};
    
    void setup(){
        addParameter(numColors.set("Num Colors", 2, 2, 10));
        addParameterDropdown(mode, "Mode", 0, {"RGB", "HSV Short", "HSV Long", "Oklab", "Oklch Short", "Oklch Long"});
        addParameter(input.set("Input", nullptr));
        addOutputParameter(output.set("Output", nullptr));
        
        colors.resize(numColors);
        positions.resize(numColors);
        
        for(int i = 0; i < numColors; i++){
            addParameter(colors[i].set("Col " + ofToString(i), ofFloatColor(float(i)/(numColors-1.0f), float(i)/(numColors-1.0f), float(i)/(numColors-1.0f), 1.0f)));
            addParameter(positions[i].set("Pos " + ofToString(i), float(i)/(numColors-1.0f), 0.0f, 1.0f));
        }

        for(int i = 0; i < numColors; i++){
            auto listenerPtr = std::make_shared<ofEventListener>(positions[i].newListener([this, i](float &val){
                if(i > 0 && val < positions[i-1].get()){
                     positions[i].set(positions[i-1].get());
                }
                if(i < positions.size()-1 && val > positions[i+1].get()){
                     positions[i].set(positions[i+1].get());
                }
            }));
            posListeners.push_back(listenerPtr);
        }
        
        listener = numColors.newListener([this](int &i){
            parametersChanged = true;
        });
        
        string defaultVertSource =
        #include "defaultVertexShader.h"
        ;

        string drawFragSource =
        #include "Gradient2Shader.h"
        ;
                                    
        shader.setupShaderFromSource(GL_VERTEX_SHADER, defaultVertSource);
        shader.setupShaderFromSource(GL_FRAGMENT_SHADER, drawFragSource);
        shader.bindDefaults();
        shader.linkProgram();
    }
    
    void update(ofEventArgs &e){
        if(parametersChanged){
            updateParameters();
            parametersChanged = false;
        }
    }

    void draw(ofEventArgs &a){
        if((input.get() != nullptr) && (input.get()->getWidth()>0 && input.get()->getHeight()>0) )
		{
            if( !fbo.isAllocated() || fbo.getWidth()!=input.get()->getWidth() || fbo.getHeight()!=input.get()->getHeight() )
			{
                
                ofFbo::Settings settings;
                settings.width = input.get()->getWidth();
                settings.height = input.get()->getHeight();
                settings.internalformat = GL_RGBA32F;
                settings.maxFilter = GL_NEAREST;
                settings.minFilter = GL_NEAREST;
                settings.numColorbuffers = 1;
                settings.useDepth = false;
                settings.useStencil = false;
                settings.textureTarget = GL_TEXTURE_2D;
                
                fbo.allocate(settings);
                fbo.begin();
                ofClear(0, 0, 0, 0);
                fbo.end();
            }
            
            // Enable alpha blending
            ofEnableAlphaBlending();
            
            shader.begin();
            fbo.begin();
			ofClear(0, 0, 0, 0);
            ofPushStyle();
            ofSetColor(255, 255, 255, 255);
            shader.setUniformTexture("tSource", *input.get(), 0);
            shader.setUniform1i("numCols", numColors);
            shader.setUniform1i("interpMode", mode);
            for(int i = 0; i < numColors; i++){
                shader.setUniform4f("color" + ofToString(i+1), colors[i].get());
                shader.setUniform1f("pos" + ofToString(i+1), positions[i].get());
            }
            ofDrawRectangle(0, 0, fbo.getWidth(), fbo.getHeight());
            ofPopStyle();
            fbo.end();
            shader.end();
            
//            // Cleanup: unbind texture
//            glActiveTexture(GL_TEXTURE0);
//            glBindTexture(GL_TEXTURE_2D, 0);
            
            ofDisableAlphaBlending();
            
            output = &fbo.getTexture();
        }
    }
	
	void loadBeforeConnections(ofJson &json){
	       deserializeParameter(json, numColors);
	    deserializeParameter(json, mode);
	       updateParameters();
	   }
	
	void deactivate(){
        fbo.clear();
//        output = nullptr;
    }
    
private:
    ofShader shader;
    
    ofParameter<ofTexture*> input;
    ofParameter<ofTexture*> output;
    
    ofParameter<int> numColors;
    ofParameter<int> mode;
    vector<ofParameter<ofFloatColor>> colors;
    vector<ofParameter<float>> positions;
    
    ofFbo fbo;
    
    ofEventListener listener;
    vector<std::shared_ptr<ofEventListener>> posListeners;

    bool parametersChanged = false;

    void updateParameters(){
        int i = numColors;
        if(colors.size() != i){
            int oldSize = colors.size();
            bool remove = oldSize > i;
            
            posListeners.clear();
            colors.resize(i);
            positions.resize(i);
            
            if(remove){
                for(int j = oldSize-1; j >= i; j--){
                    removeParameter("Col " + ofToString(j));
                    removeParameter("Pos " + ofToString(j));
                }
                for(int j = 0; j < i; j++){
                    getParameter<ofFloatColor>("Col " + ofToString(j)) = ofFloatColor(float(j)/(i-1.0f), float(j)/(i-1.0f), float(j)/(i-1.0f), 1.0f);
                    getParameter<float>("Pos " + ofToString(j)) = float(j)/(i-1.0f);
                }
            }else{
                for(int j = 0; j < i; j++){
                    if(j < oldSize){
                        getParameter<ofFloatColor>("Col " + ofToString(j)) = ofFloatColor(float(j)/(i-1.0f), float(j)/(i-1.0f), float(j)/(i-1.0f), 1.0f);
                        getParameter<float>("Pos " + ofToString(j)) = float(j)/(i-1.0f);
                    }
                    else{
                        addParameter(colors[j].set("Col " + ofToString(j), ofFloatColor(float(j)/(i-1.0f), float(j)/(i-1.0f), float(j)/(i-1.0f), 1.0f)));
                        addParameter(positions[j].set("Pos " + ofToString(j), float(j)/(i-1.0f), 0.0f, 1.0f));
                    }
                }
            }

            for(int j = 0; j < i; j++){
                auto listenerPtr = std::make_shared<ofEventListener>(positions[j].newListener([this, j](float &val){
                    if(j > 0 && val < positions[j-1].get()){
                         positions[j].set(positions[j-1].get());
                    }
                    if(j < positions.size()-1 && val > positions[j+1].get()){
                         positions[j].set(positions[j+1].get());
                    }
                }));
                posListeners.push_back(listenerPtr);
            }
        }
    }
};
    

#endif /* Gradient2_h */
