//
//  textureRecorder.cpp
//  MIRABCN_Generator
//
//  Created by Eduard Frigola on 24/05/2018.
//
//

#include "textureRecorder.h"

#include <algorithm>
#include <utility>

textureRecorder::textureRecorder() : ofxOceanodeNodeModel("Texture Recorder"){
    addParameter(phasorIn.set("Phase", 0, 0, 1));
    addParameter(record.set("Record", false));
    addParameter(autoRecLoop.set("Auto.Rec", false));
    addParameter(filename.set("File", "recTest"));
    addParameter(input.set("Input", nullptr));
    
    addParameter(recordAlpha.set("Alpha?", false));
    
    addInspectorParameter(createVideo.set("Create Video", false));
    addInspectorParameter(highPrecision.set("32 bit", false));

    listeners.push(phasorIn.newListener(this, &textureRecorder::phasorInListener));
    listeners.push(record.newListener(this, &textureRecorder::recordListener));
    listeners.push(input.newListener(this, &textureRecorder::inputListener));
    oldPhasor = 0;
    frameCounter = 0;
    recorderIsSetup = false;
    lastFrame = false;
    fboInternalFormat = 0;

    // Enough queued frames to ride out the variance in PNG encoding times
    // without letting a 15-megapixel backlog eat memory.
    maxQueuedFrames = 8;
}

textureRecorder::~textureRecorder(){
    stopWriters();
}

void textureRecorder::startWriters(){
    if(writersRunning.load()) return;
    writersRunning = true;
    unsigned int cores = std::thread::hardware_concurrency();
    const unsigned int count = std::max(2u, std::min(4u, cores == 0 ? 2u : cores / 2));
    for(unsigned int i = 0; i < count; i++){
        writers.emplace_back(&textureRecorder::writerLoop, this);
    }
}

void textureRecorder::stopWriters(){
    if(!writersRunning.load()) return;
    {
        std::lock_guard<std::mutex> lock(writeMutex);
        writersRunning = false;
    }
    // Both conditions: a writer may be waiting for work, and the producer may
    // be waiting for space.
    writeWorkAvailable.notify_all();
    writeSpaceAvailable.notify_all();
    for(auto &writer : writers){
        if(writer.joinable()) writer.join();
    }
    writers.clear();
}

void textureRecorder::enqueueFrame(ofPixels &&pixels, const std::string &path){
    std::unique_lock<std::mutex> lock(writeMutex);
    writeSpaceAvailable.wait(lock, [this]{
        return writeQueue.size() < maxQueuedFrames || !writersRunning.load();
    });
    if(!writersRunning.load()) return;
    writeQueue.push_back({std::move(pixels), path});
    lock.unlock();
    writeWorkAvailable.notify_one();
}

void textureRecorder::writerLoop(){
    while(true){
        pendingFrame frame;
        {
            std::unique_lock<std::mutex> lock(writeMutex);
            writeWorkAvailable.wait(lock, [this]{
                return !writeQueue.empty() || !writersRunning.load();
            });
            // Drain what is queued even after a stop, so every captured frame
            // reaches disk before the video is assembled.
            if(writeQueue.empty()) return;
            frame = std::move(writeQueue.front());
            writeQueue.pop_front();
        }
        writeSpaceAvailable.notify_one();
        ofSaveImage(frame.pixels, frame.path);
    }
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
        const int wantedFormat = highPrecision ? (recordAlpha ? GL_RGBA32F : GL_RGB32F)
                                               : (recordAlpha ? GL_RGBA8  : GL_RGB8);
        if(!recorderIsSetup || fboInternalFormat != wantedFormat ||
           input.get()->getWidth() != width || input.get()->getHeight() != height){
            width = input.get()->getWidth();
            height = input.get()->getHeight();
            fbo.allocate(width, height, wantedFormat);
            fboInternalFormat = wantedFormat;
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
                
                enqueueFrame(std::move(pixels),
                             "recordings/" + filename.get() + "_" + initRecordingTimestamp +
                             "/" + filename.get() + "_" + ofToString(frameCounter, 9, '0') + ".png");
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
        // The writer threads cannot create this, and a missing folder would
        // otherwise fail once per frame with nothing to show for it.
        ofDirectory::createDirectory(
            ofToDataPath("recordings/" + filename.get() + "_" + initRecordingTimestamp, true),
            true, true);
        startWriters();
        setFlags(ofxOceanodeNodeModelFlags_ForceFrameMode);
    }else{
        autoRecLoop = false;
        recorderIsSetup = false;
        setFlags(ofxOceanodeNodeModelFlags_None);
        // Every queued frame must be on disk before ffmpeg globs the folder.
        stopWriters();
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
