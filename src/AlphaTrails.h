//
//  AlphaTrails.h
//  ofxOceanodeTextures
//
//  Created for alpha trail effect by controlling framebuffer clear amount
//

#ifndef AlphaTrails_h
#define AlphaTrails_h

#include "ofxOceanodeNodeModel.h"

class AlphaTrails : public ofxOceanodeNodeModel {
public:
    AlphaTrails() : ofxOceanodeNodeModel("Alpha Trails"){};
    
    void setup(){
		doClear=false;
        addParameter(input.set("Input", nullptr));
        addParameter(persistence.set("Persistence", 0.95, 0.0, 1.0));
		addParameter(clear.set("Clear"));
        addOutputParameter(output.set("Output", nullptr));
		
		listener = clear.newListener([this](){
			doClear=true;
		});

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
                ofClear(0, 0, 0, 0);
                fbo.end();
            }

			//ofEnableBlendMode(OF_BLENDMODE_MULTIPLY);
			ofEnableAlphaBlending();
			fbo.begin();
			
			//ofEnableBlendMode(OF_BLENDMODE_ALPHA);

			float fadeAmount = (1.0f - persistence.get()) * 1.0f;
			ofSetFloatColor(0, 0, 0, fadeAmount);
			ofDrawRectangle(0, 0, fbo.getWidth(), fbo.getHeight());

			ofSetFloatColor(1.0f);
			input.get()->draw(0, 0, fbo.getWidth(), fbo.getHeight());
			//ofDisableBlendMode();
			
			fbo.end();
			ofDisableAlphaBlending();
			
			if(doClear)
			{
				fbo.begin();
				ofClear(0,0,0,255);
				fbo.end();
				doClear=false;
			}
			
            output = &fbo.getTexture();
        }
    }
    
    void deactivate(){
        fbo.clear();
    }
    
private:
    ofParameter<ofTexture*> input;
    ofParameter<ofTexture*> output;
    ofParameter<float> persistence;
	ofParameter<void> clear;
	bool doClear;
	ofEventListener listener;
    ofFbo fbo;
};

#endif /* AlphaTrails_h */
