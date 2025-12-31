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
    
    addParameter(recordAlpha.set("Alpha?", false));
    
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
            fbo.allocate(width, height, recordAlpha ? GL_RGBA : GL_RGB);
            fbo.begin();
            if(recordAlpha){
                ofClear(0, 0, 0, 0);  // Clear with full transparency for alpha
            }else{
                ofClear(0, 0, 0, 255);  // Clear with opaque black for RGB
            }
            input.get()->draw(0,0);
            fbo.end();
            recorderIsSetup = true;
        }
        if(record){
            if(input != nullptr){
                fbo.begin();
                if(recordAlpha){
                    ofClear(0, 0, 0, 0);  // Clear with full transparency before each frame
                }else{
                    ofClear(0, 0, 0, 255);  // Clear with opaque black before each frame
                }
                input.get()->draw(0,0);
                fbo.end();
                ofPixels pixels;
                fbo.getTexture().readToPixels(pixels);
                
                // Convert from premultiplied to straight alpha if recording with alpha
                if(recordAlpha && pixels.getNumChannels() == 4){
                    for(int i = 0; i < pixels.size(); i += 4){
                        float alpha = pixels[i + 3] / 255.0f;
                        if(alpha > 0.0f){
                            // Unpremultiply: divide RGB by alpha
                            pixels[i] = ofClamp(pixels[i] / alpha, 0, 255);
                            pixels[i + 1] = ofClamp(pixels[i + 1] / alpha, 0, 255);
                            pixels[i + 2] = ofClamp(pixels[i + 2] / alpha, 0, 255);
                        }
                    }
                }
                
                ofSaveImage(pixels, "recordings/" + filename.get() +  "_" + initRecordingTimestamp + "/" + filename.get() + "_" + ofToString(frameCounter, 9, '0') + ".png");
//                image.clear();
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
			string command = "cd " +ofToString("\"") + ofToDataPath("recordings/" + filename.get() +  "_" + initRecordingTimestamp, true) +ofToString("\"");
            if(recordAlpha){
                command += " && /opt/homebrew/bin/ffmpeg -f image2 -framerate " + ofToString(ofGetTargetFrameRate()) + " -pattern_type glob -i '*.png' -c:v prores_ks -profile:v 4444 -pix_fmt yuva444p10le " + filename.get() + ".mov";
            }else{
                command += " && /opt/homebrew/bin/ffmpeg -f image2 -framerate " + ofToString(ofGetTargetFrameRate()) + " -pattern_type glob -i '*.png' -c:v prores_ks -profile:v 4 " + filename.get() + ".mov";
            }
            system(command.c_str());
        }
    }
}
