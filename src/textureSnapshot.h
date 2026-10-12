#pragma once

#include "ofMain.h"
#include "ofxOceanodeNodeModel.h"

class textureSnapshot : public ofxOceanodeNodeModel {
public:
	textureSnapshot()
	: ofxOceanodeNodeModel("Texture Snapshot") {

		// Parameters (follow your style: addParameter(param.set(...)))
		addParameter(texIn.set("Texture In", nullptr));
		addParameter(texOut.set("Texture Out", nullptr));
		addParameter(snapTrigger.set("Snap!"));
		snapTrigger.addListener(this, &textureSnapshot::onSnap);
	}

	~textureSnapshot() override {
		snapTrigger.removeListener(this, &textureSnapshot::onSnap);
	}

	void setup() override {}
	void update(ofEventArgs &e) override {
		// Nothing to do; output remains the last captured frame.
		// Ensure output points to FBO texture if we have one.
		if(fboSnapshot.isAllocated()){
			texOut = &fboSnapshot.getTexture();
		}
	}
	void draw(ofEventArgs &e) override {}

private:
	// --- Parameters ---
	ofParameter<ofTexture*> texIn;
	ofParameter<ofTexture*> texOut;
	ofParameter<void>       snapTrigger;

	// --- Internals ---
	ofFbo                   fboSnapshot;

	void onSnap(){
		ofTexture* src = texIn.get();
		if(src == nullptr || !src->isAllocated()){
			ofLogWarning("textureSnapshot") << "Snap ignored: input texture is null or not allocated";
			return;
		}

		const int w = static_cast<int>(src->getWidth());
		const int h = static_cast<int>(src->getHeight());

		// Allocate (or reallocate) FBO to match input at snap time
		bool needsAlloc = !fboSnapshot.isAllocated()
					   || fboSnapshot.getWidth()  != w
					   || fboSnapshot.getHeight() != h;

		if(needsAlloc){
			ofFbo::Settings s;
			s.width  = w;
			s.height = h;

			// Try to preserve internal format from the source texture if available
			// (falls back to RGBA if not accessible)
			GLint internalFormat = GL_RGBA;
			const ofTextureData& td = src->getTextureData();
			if(td.glInternalFormat != 0){
				internalFormat = td.glInternalFormat;
			}
			s.internalformat = internalFormat;

			s.useDepth   = false;
			s.useStencil = false;
			s.numSamples = 0;

			fboSnapshot.allocate(s);
		}

		// Copy the texture into the FBO (snapshot)
		fboSnapshot.begin();
		ofClear(0,0,0,0);
		src->draw(0, 0, w, h);
		fboSnapshot.end();

		// Point output to the frozen snapshot texture
		texOut = &fboSnapshot.getTexture();
	}
};
