//
//  displayOutput.h
//  ofxOceanodeGraphics
//
//  Created by Eduard Frigola Bagué on 27/09/2022.
//

#ifndef displayOutput_h
#define displayOutput_h

#include "ofxOceanodeNodeModelExternalWindow.h"
#include <algorithm>

class displayOutput : public ofxOceanodeNodeModelExternalWindow{
public:
    displayOutput() : ofxOceanodeNodeModelExternalWindow("Display Output"){};
    ~displayOutput(){};
    
    void setup() override{
        addParameter(texture.set("Texture", nullptr));
		addParameter(masterFader.set("Master Fader", 1, 0, 1), ofxOceanodeParameterFlags_DisableSavePreset);
        addParameter(topLeftCorner.set("TopLeft", false));
        addParameter(fit.set("Fit", false));
        addParameter(backgroundTint.set("Bg Tint", false));
    };
    
    void presetHasLoaded() override{
        setShowWindow(true);
        auto currentWin = ofGetCurrentWindow();
        setExternalWindowFullScreen(true);
        currentWin->makeCurrent();
        ofGetMainLoop()->setCurrentWindow(currentWin);
    }
    
private:
    void drawInExternalWindow(ofEventArgs &e) override{
        ofBackground(0);
        if(texture.get() != nullptr){
            if(backgroundTint){
                ofClear(255, 0, 0, 255);
            }
            ofPushStyle();
            ofSetRectMode(OF_RECTMODE_CENTER);
            ofSetColor(masterFader*255);
            texture.get()->setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
            if(fit){
                float drawWidth = texture.get()->getWidth();
                float drawHeight = texture.get()->getHeight();

                if(drawWidth > 0 && drawHeight > 0){
                    const float scale = std::min((float)ofGetWidth() / drawWidth,
                                                 (float)ofGetHeight() / drawHeight);
                    drawWidth *= scale;
                    drawHeight *= scale;
                }

                if(topLeftCorner){
                    ofSetRectMode(OF_RECTMODE_CORNER);
                    texture.get()->draw(0, 0, drawWidth, drawHeight);
                }else{
                    ofSetRectMode(OF_RECTMODE_CENTER);
                    texture.get()->draw(ofGetWidth()/2, ofGetHeight()/2, drawWidth, drawHeight);
                }
            }else if(topLeftCorner){
                ofSetRectMode(OF_RECTMODE_CORNER);
                texture.get()->draw(0, 0);
            }else{
                ofSetRectMode(OF_RECTMODE_CENTER);
                texture.get()->draw(ofGetWidth()/2, ofGetHeight()/2);
            }
            ofPopStyle();
        }
    }
    
    void keyPressed(ofKeyEventArgs &a){
	if(a.key == 'f'){
	    toggleFullscreen();
	}
    }
    
    ofParameter<ofTexture*> texture;
    ofParameter<float> masterFader;
    ofParameter<bool> topLeftCorner;
    ofParameter<bool> fit;
    ofParameter<bool> backgroundTint;
};

#endif /* displayOutput_h */
