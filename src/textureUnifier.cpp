//
//  textureUnifier.cpp
//  MIRABCN_Generator
//
//  Created by Eduard Frigola on 22/09/2017.
//
//

#include "textureUnifier.h"

#define NUM_INPUTS 8

textureUnifier::textureUnifier() : ofxOceanodeNodeModel("Texture Unifier"){
    color = ofColor::lightGray;
}

void textureUnifier::setup(){
    addParameter(triggerTextureIndex.set("Trig.Index", 0, 0, NUM_INPUTS));
    addParameter(spacing.set("Spacing", 1, 0, 10));
    inputs.resize(NUM_INPUTS);
    customPositions.resize(NUM_INPUTS);
    opacities.resize(NUM_INPUTS);
    for(int i = 0; i < inputs.size() ; i++){
        addParameter(inputs[i].set("Input." + ofToString(i), nullptr));
        addParameter(customPositions[i].set("Pos." + ofToString(i), "0,0"));
        addParameter(opacities[i    ].set("Opac." + ofToString(i),1.0,0.0,1.0));
        listeners.push(customPositions[i].newListener([this, i](string &s){
            if(ofSplitString(s, ",").size() != 2){
                customPositions[i] = "0,0";
            }
        }));
        listeners.push(inputs[i].newListener(this, &textureUnifier::computeOutput));
    }
    addParameter(output.set("Output", nullptr));
}

void textureUnifier::computeOutput(ofTexture* &in){
    if(in != nullptr){
        if(&in == &inputs[triggerTextureIndex].get()){
            ofRectangle fboRect(0, 0, 0, 0);
            int totalHeight = -spacing;
            int maxWidth = 0;
            for(int i = 0; i < inputs.size(); i++){
                auto &t = inputs[i];
                if(t != nullptr){
                    if(customPositions[i].get() == "0,0"){
                        fboRect.growToInclude(t.get()->getWidth(), fboRect.getHeight() + t.get()->getHeight() + spacing);
                    }
                    else{
                        vector<string> positionInfo = ofSplitString(customPositions[i], ",");
                        fboRect.growToInclude(ofRectangle(ofToInt(positionInfo[0]), ofToInt(positionInfo[1]), t.get()->getWidth(), t.get()->getHeight()));
                    }
                }
            }
            if(outputFbo.getHeight() != fboRect.getHeight() || outputFbo.getWidth() != fboRect.getWidth() || !outputFbo.isAllocated()){
                if(fboRect.getHeight() != 0 && fboRect.getWidth() != 0){
                    outputFbo.clear();
    
                    ofFbo::Settings settings;
                    settings.height = fboRect.getHeight();
                    settings.width = fboRect.getWidth();
                    settings.internalformat = GL_RGB32F;
                    settings.maxFilter = GL_NEAREST;
                    settings.minFilter = GL_NEAREST;
                    settings.numColorbuffers = 1;
                    settings.useDepth = false;
                    settings.useStencil = false;
                    
                    outputFbo.allocate(settings);
                    outputFbo.begin();
                    ofClear(0, 0, 0, 255);
                    outputFbo.end();
                }else{
                    return;
                }
            }
            
            outputFbo.begin();
            ofClear(0, 0, 0);
            int currentLine = 0;//spacing;
            for(int i = 0; i < inputs.size(); i++){
                auto &t = inputs[i];
                if(t != nullptr){
                    ofPushStyle();
                    ofSetColor(255.0 * opacities[i]);
                    if(customPositions[i].get() == "0,0"){
                        t.get()->draw(0, currentLine);
                        currentLine += t.get()->getHeight() + spacing;
                    }
                    else{
                        vector<string> positionInfo = ofSplitString(customPositions[i], ",");
                        t.get()->draw(ofToInt(positionInfo[0]), ofToInt(positionInfo[1]));
                        currentLine = max(int(currentLine), int(ofToInt(positionInfo[1]) + t.get()->getHeight())) + spacing;
                    }
                    ofPopStyle();
                }
            }
            outputFbo.end();
            
            output = &outputFbo.getTexture();
        }
    }
}
