//
//  Gradient.h
//  lille
//
//  Created by Eduard Frigola Bagué on 03/02/2021.
//

#ifndef Gradient_h
#define Gradient_h

#include "ofxOceanodeNodeModel.h"

#define STRINGIFY(A) #A

class Gradient : public ofxOceanodeNodeModel {
public:
    Gradient() : ofxOceanodeNodeModel("Gradient"){};
    
    void setup(){
        addInspectorParameter(numColors.set("Num Colors", 5, 2, 10));
        addParameter(input.set("Input", nullptr));
        addOutputParameter(output.set("Output", nullptr));
        
        colors.resize(numColors);
        
        for(int i = 0; i < numColors; i++){
            addParameter(colors[i].set("Col " + ofToString(i), ofFloatColor(float(i)/(numColors-1.0f), float(i)/(numColors-1.0f), float(i)/(numColors-1.0f), float(i)/(numColors-1.0f))));
        }
        
        listener = numColors.newListener([this](int &i){
            if(colors.size() != i){
                int oldSize = colors.size();
                bool remove = oldSize > i;
                
                colors.resize(i);
                
                if(remove){
                    for(int j = oldSize-1; j >= i; j--){
                        removeParameter("Col " + ofToString(j));
                    }
                    for(int j = 0; j < i; j++){
                        getParameter<ofFloatColor>("Col " + ofToString(j)) = ofFloatColor(float(j)/(numColors-1.0f), float(j)/(numColors-1.0f), float(j)/(numColors-1.0f), float(j)/(numColors-1.0f));
                    }
                }else{
                    for(int j = 0; j < numColors; j++){
                        if(j < oldSize){
                            getParameter<ofFloatColor>("Col " + ofToString(j)) = ofFloatColor(float(j)/(numColors-1.0f), float(j)/(numColors-1.0f), float(j)/(numColors-1.0f), float(j)/(numColors-1.0f));
                        }
                        else{
                            addParameter(colors[j].set("Col " + ofToString(j), ofFloatColor(float(j)/(numColors-1.0f), float(j)/(numColors-1.0f), float(j)/(numColors-1.0f), float(j)/(numColors-1.0f))));
                        }
                    }
                }
            }
        });
        
        string defaultVertSource =
        #include "defaultVertexShader.h"
        ;

        string drawFragSource =
        #include "GradientShader.h"
        ;
                                    
        shader.setupShaderFromSource(GL_VERTEX_SHADER, defaultVertSource);
        shader.setupShaderFromSource(GL_FRAGMENT_SHADER, drawFragSource);
        shader.bindDefaults();
        shader.linkProgram();
    }
    
    void draw(ofEventArgs &a){
        if(input.get() != nullptr){
            if(!fbo.isAllocated() || fbo.getWidth() != input.get()->getWidth() || fbo.getHeight() != input.get()->getHeight()){
                
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
                ofClear(0, 0, 0, 255);
                fbo.end();
            }
            
            shader.begin();
            fbo.begin();
            ofPushStyle();
            ofSetColor(255, 255, 255, 255);
            shader.setUniformTexture("tSource", *input.get(), 15);
            shader.setUniform1i("numCols", numColors);
            for(int i = 0; i < numColors; i++){
                shader.setUniform4f("color" + ofToString(i+1), colors[i].get());
            }
            ofDrawRectangle(0, 0, fbo.getWidth(), fbo.getHeight());
            ofPopStyle();
            fbo.end();
            shader.end();
            
            output = &fbo.getTexture();
        }
    }
	
	void loadBeforeConnections(ofJson &json){
		deserializeParameter(json, numColors);
	}
    
    void deactivate(){
        fbo.clear();
    }
    
private:
    ofShader shader;
    
    ofParameter<ofTexture*> input;
    ofParameter<ofTexture*> output;
    
    ofParameter<int> numColors;
    vector<ofParameter<ofFloatColor>> colors;
    
    ofFbo fbo;
    
    ofEventListener listener;
};
    

#endif /* Gradient_h */
