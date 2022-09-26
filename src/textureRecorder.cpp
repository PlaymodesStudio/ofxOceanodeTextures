//
//  textureRecorder.cpp
//  MIRABCN_Generator
//
//  Created by Eduard Frigola on 24/05/2018.
//
//

#include "textureRecorder.h"

textureRecorder::textureRecorder() : ofxOceanodeNodeModel("Texture Recorder"){
    addParameter(phasorIn.set("Phase", 0, 0, 1));
    addParameter(record.set("Record", false));
    addParameter(autoRecLoop.set("Auto.Rec", false));
    addParameter(filename.set("File", "recTest"));
    addParameter(input.set("Input", nullptr));
    
    addInspectorParameter(createVideo.set("Create Video", false));

    listeners.push(phasorIn.newListener(this, &textureRecorder::phasorInListener));
    listeners.push(record.newListener(this, &textureRecorder::recordListener));
    listeners.push(input.newListener(this, &textureRecorder::inputListener));
    oldPhasor = 0;
    frameCounter = 0;
    recorderIsSetup = false;
    lastFrame = false;
}

void textureRecorder::phasorInListener(float &f){
    if(autoRecLoop){
        if(f < oldPhasor){
            if(!record) record = true;
            else record = false;
        }
    }
    oldPhasor = f;
}

void textureRecorder::draw(ofEventArgs &a){

}

void textureRecorder::inputListener(ofTexture* &texture){
    if(input != nullptr){
        if(!recorderIsSetup || input.get()->getWidth() != width || input.get()->getHeight() != height){
            width = input.get()->getWidth();
            height = input.get()->getHeight();
            fbo.allocate(width, height, GL_RGB);
            fbo.begin();
            ofClear(0);
            input.get()->draw(0,0);
            fbo.end();
            recorderIsSetup = true;
        }
        if(record){
            if(input != nullptr){
                fbo.begin();
                input.get()->draw(0,0);
                fbo.end();
                ofFloatPixels pixels;
                fbo.getTexture().readToPixels(pixels);
                ofImage image;
                image.setFromPixels(pixels);
                image.save("recordings/" + filename.get() +  "_" + initRecordingTimestamp + "/" + filename.get() + "_" + ofToString(frameCounter, 9, '0') + ".png");
            }
            frameCounter++;
            if(lastFrame){
                record = false;
                lastFrame = false;
            }
        }
    }
}

void textureRecorder::recordListener(bool &b){
    if(b){
        initRecordingTimestamp = ofGetTimestampString();
        frameCounter = 0;
        setFlags(ofxOceanodeNodeModelFlags_ForceFrameMode);
    }else{
        autoRecLoop = false;
        recorderIsSetup = false;
        setFlags(ofxOceanodeNodeModelFlags_None);
        if(createVideo){
            string command = "cd " + ofToDataPath("recordings/" + filename.get() +  "_" + initRecordingTimestamp, true);
            command += " && /usr/local/Cellar/ffmpeg/4.4_2/bin/ffmpeg -f image2 -framerate " + ofToString(ofGetTargetFrameRate()) + " -pattern_type glob -i '*.png' -c:v prores_ks -profile:v 4 " + filename.get() + ".mov";
            system(command.c_str());
        }
    }
}
