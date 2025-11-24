//  displayOutput.h
//  ofxOceanodeGraphics
//
//  Reworked to:
//  - not force fullscreen on preset load
//  - handle key events on the external window (f toggles, ESC exits)
//  - remove event listeners on destruction
//
//  Created by Eduard Frigola Bagué on 27/09/2022.
//  Updated by ChatGPT on 04/10/2025

#ifndef displayOutput_h
#define displayOutput_h

#include "ofxOceanodeNodeModelExternalWindow.h"

class displayOutput : public ofxOceanodeNodeModelExternalWindow{
public:
	displayOutput()
	: ofxOceanodeNodeModelExternalWindow("Display Output")
	, isFullscreen(false)
	, listenerAttached(false)
	{}

	~displayOutput(){
		detachKeyListener();
	}
	
	void setup() override{
		addParameter(texture.set("Texture", nullptr));
		addParameter(masterFader.set("Master Fader", 1, 0, 1), ofxOceanodeParameterFlags_DisableSavePreset);
		addParameter(topLeftCorner.set("TopLeft", false));
		addParameter(backgroundTint.set("Bg Tint", false));
	}
	
	void presetHasLoaded() override{
		// Show the external window, but DO NOT force fullscreen here.
		setShowWindow(true);

		// (Re)attach key listener to the external window so ESC / 'f' work there.
		attachKeyListener();

		// Restore focus to the current window as you had before (optional).
		auto currentWin = ofGetCurrentWindow();
		if(currentWin){
			currentWin->makeCurrent();
			ofGetMainLoop()->setCurrentWindow(currentWin);
		}
	}
	
private:
	// ---------- Rendering in the external window ----------
	void drawInExternalWindow(ofEventArgs &e) override{
		ofBackground(0);
		if(texture.get() != nullptr){
			if(backgroundTint){
				ofClear(255, 0, 0, 255);
			}
			ofPushStyle();
			ofSetColor(masterFader * 255);
			texture.get()->setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);

			if(topLeftCorner){
				ofSetRectMode(OF_RECTMODE_CORNER);
				texture.get()->draw(0, 0);
			}else{
				ofSetRectMode(OF_RECTMODE_CENTER);
				texture.get()->draw(ofGetWidth() / 2, ofGetHeight() / 2);
			}
			ofPopStyle();
		}
	}

	// ---------- Key handling on the EXTERNAL window ----------
	void onKeyPressed(ofKeyEventArgs &a){
		if(a.key == OF_KEY_ESC){
			// Always exit fullscreen on ESC
			if(isFullscreen){
				setExternalWindowFullScreen(false);
				isFullscreen = false;
			}
		}else if(a.key == 'f' || a.key == 'F'){
			// Toggle fullscreen on 'f'
			isFullscreen = !isFullscreen;
			setExternalWindowFullScreen(isFullscreen);
		}
	}

	// Keep the original signature if something else calls it; forward to external handler.
	void keyPressed(ofKeyEventArgs &a){
		onKeyPressed(a);
	}

	// ---------- Listener wiring helpers ----------
	void attachKeyListener(){
		// use the externalWindow provided by the base class
		if(externalWindow && !listenerAttached){
			ofAddListener(externalWindow->events().keyPressed, this, &displayOutput::onKeyPressed);
			listenerAttached = true;
		}
	}

	void detachKeyListener(){
		if(externalWindow && listenerAttached){
			ofRemoveListener(externalWindow->events().keyPressed, this, &displayOutput::onKeyPressed);
			listenerAttached = false;
		}
	}

	// ---------- Members ----------
	bool isFullscreen;
	bool listenerAttached;

	ofParameter<ofTexture*> texture;
	ofParameter<float>      masterFader;
	ofParameter<bool>       topLeftCorner;
	ofParameter<bool>       backgroundTint;
};

#endif /* displayOutput_h */
