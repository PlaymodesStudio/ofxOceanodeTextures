//
//  waveScope.cpp
//  MIRABCN_Generator
//
//  Created by Eduard Frigola on 10/01/2017.
//
//

#include "waveScope.h"
#include "sharedInfo.h"

waveScope::waveScope() : ofxOceanodeNodeModelExternalWindow("Texture Scope"){
    
    texturesInput.setup(parameters, ofParameter<ofTexture*>("Group In", nullptr));
    listener = texturesInput.parameterGroupChanged.newListener([&](){
        ofNotifyEvent(parameterGroupChanged);
    });
    color = ofColor::lightGray;
    storedShape = sharedInfo::getInstance().getRect("ScopeWindowRect");
}

waveScope::~waveScope(){
    if(getNumIdentifier() == 1){
        ofRectangle windowShape(0,0,0,0);
        if(externalWindow != nullptr){
            windowShape.setPosition(glm::vec3(externalWindow->getWindowPosition(), 0));
            windowShape.setSize(externalWindow->getWidth(), externalWindow->getHeight());
        }else{
            windowShape = externalWindowRect;
        }
        sharedInfo::getInstance().setRect("ScopeWindowRect", windowShape);
    }
}


void waveScope::drawInExternalWindow(ofEventArgs &e){
    if(getNumIdentifier() == 1 && storedShape != ofRectangle(0,0,0,0)){
        setExternalWindowPosition(storedShape.x, storedShape.y);
        setExternalWindowShape(storedShape.width, storedShape.height);
        saveStateOnPreset = false;
        storedShape = ofRectangle(0,0,0,0);
    }
    ofBackground(0);
    ofSetColor(255);
    //
    int contentWidth = 2*ofGetWidth()/3 + contentWidthOffset;
    
    
    int numActiveGroups = 0;
    for(auto texture : texturesInput.getParameters()){
        if(texture.second != nullptr){
            numActiveGroups++;
        }
    }
    
    if(numActiveGroups > 0){
        int elementHeight = ofGetHeight() / numActiveGroups;
        int oscDrawIndex = 0;
        
        //Draw the groups
        for(auto texture : texturesInput.getParameters()){
            if(texture.second != nullptr){
                int topPosition = (elementHeight * oscDrawIndex);
                texture.second.get()->draw(0,topPosition, ofGetWidth(), elementHeight);
                ofPushStyle();
                ofSetColor(ofColor::indianRed);
                ofNoFill();
                ofSetLineWidth(2);
                ofDrawRectangle(0, topPosition, ofGetWidth(), elementHeight);
                ofPopStyle();
                oscDrawIndex++;
            }
        }
    }
    
    //Draw the framerate
    ofSetColor(255, 0,0);
    ofDrawBitmapString(ofToString(ofGetFrameRate()), 20, ofGetHeight()-10);
}
