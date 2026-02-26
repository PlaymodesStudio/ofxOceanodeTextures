//
//  AlphaTrails.h
//  ofxOceanodeTextures
//
//  Alpha trail effect by multiplying the previous frame by `persistence`
//  and then drawing the new input on top with normal alpha blending.
//

#ifndef AlphaTrails_h
#define AlphaTrails_h

#include "ofxOceanodeNodeModel.h"

class AlphaTrails : public ofxOceanodeNodeModel {
public:
	AlphaTrails() : ofxOceanodeNodeModel("Alpha Trails") {}

	void setup(){
		doClear = false;

		addParameter(input.set("Input", nullptr));
		addParameter(persistence.set("Persistence", 0.95f, 0.0f, 1.0f));
		addParameter(clear.set("Clear"));
		addOutputParameter(output.set("Output", nullptr));

		listener = clear.newListener([this](){
			doClear = true;
		});
	}

	void draw(ofEventArgs &){
		ofTexture* src = input.get();
		if(src == nullptr || !src->isAllocated()){
			return;
		}

		// (Re)allocate FBO to match input
		if(!fbo.isAllocated()
		   || fbo.getWidth()  != src->getWidth()
		   || fbo.getHeight() != src->getHeight())
		{
			ofFbo::Settings settings;
			settings.width          = src->getWidth();
			settings.height         = src->getHeight();
			settings.internalformat = GL_RGBA32F;
			settings.numColorbuffers= 1;
			settings.useDepth       = false;
			settings.useStencil     = false;
			settings.textureTarget  = GL_TEXTURE_2D;
			settings.minFilter      = GL_LINEAR;   // smoother gradients
			settings.maxFilter      = GL_LINEAR;

			fbo.allocate(settings);

			fbo.begin();
			ofClear(0, 0, 0, 0); // start fully transparent
			fbo.end();
		}

		// Do a transparent clear when requested
		if(doClear){
			fbo.begin();
			ofClear(0, 0, 0, 0);
			fbo.end();
			doClear = false;
		}

		// ---- TRAIL COMPOSITE PASS ----
		fbo.begin();
		ofPushStyle();

		// 1) Fade previous content by multiplying both color and alpha by `p`
		{
			float p = ofClamp(persistence.get(), 0.0f, 1.0f);

			// Pure multiply using fixed-function blending:
			// result = dst * p  (for both RGB and A)
			glEnable(GL_BLEND);
			glBlendFuncSeparate(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA,
								GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);

			// Choose src alpha so that (1 - src.a) = p  =>  src.a = 1 - p
			ofSetColor(ofFloatColor(0, 0, 0, 1.0f - p));
			ofDrawRectangle(0, 0, fbo.getWidth(), fbo.getHeight());
		}

		// 2) Draw the new input over it with normal alpha blending
		{
			glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA,
								GL_ONE,       GL_ONE_MINUS_SRC_ALPHA);
			// or: ofEnableBlendMode(OF_BLENDMODE_ALPHA);

			ofSetColor(255); // no modulation
			src->draw(0, 0, fbo.getWidth(), fbo.getHeight());

			glDisable(GL_BLEND);
		}

		ofPopStyle();
		fbo.end();

		output = &fbo.getTexture();
	}

	void deactivate(){
		fbo.clear();
        output = nullptr;
	}

private:
	ofParameter<ofTexture*> input;
	ofParameter<ofTexture*> output;
	ofParameter<float>      persistence;
	ofParameter<void>       clear;

	bool            doClear = false;
	ofEventListener listener;
	ofFbo           fbo;
};

#endif /* AlphaTrails_h */
