//
//  videoPlayer.h
//  lille
//
//  Created by Eduard Frigola Bagué on 16/02/2021.
//

#ifndef videoPlayer_h
#define videoPlayer_h

#include "ofxOceanodeNodeModel.h"

class videoPlayer : public ofxOceanodeNodeModel {
public:
    videoPlayer() : ofxOceanodeNodeModel("Video Player"){}
    
    void setup(){
        ofDirectory dir;
        dir.open("Movies");
        dir.sort();
        vector<string> files = {"None"};
        for(int i = 0; i < dir.listDir(); i++){
            files.push_back(dir.getName(i));
        }
        
        addParameterDropdown(fileIndex, "File s", 0, files);
        addParameter(loop.set("Loop", true));
        addParameter(play.set("Play", false));
        addParameter(speed.set("Speed", 1, 0, 10));
        addParameter(position.set("Position", 0, 0, 1));
        addOutputParameter(texture.set("Output", nullptr));
        
        listeners.push(fileIndex.newListener([this, files](int &i){
            string filename = files[i];
            if(filename == "None"){
                //vPlayer.unload();
            }else{
                vPlayer.load("Movies/" + filename);
                vPlayer.setLoopState(loop ? OF_LOOP_NORMAL : OF_LOOP_NONE);
            }
        }));
        
        listeners.push(loop.newListener([this](bool &b){
            vPlayer.setLoopState(b ? OF_LOOP_NORMAL : OF_LOOP_NONE);
        }));
        
        listeners.push(play.newListener([this](bool &b){
            if(b) vPlayer.play();
            else vPlayer.stop();
        }));
        
        listeners.push(speed.newListener([this](float &f){
            vPlayer.setSpeed(f);
        }));
        
        listeners.push(position.newListener([this](float &f){
            if(!positionSetByItself){
                //vPlayer.setPaused(true);
                //vPlayer.setFrame(f * vPlayer.getTotalNumFrames());
                //vPlayer.setPaused(false);
                requestedPosition = f;
            }
            positionSetByItself = false;
        }));
    }
    
    void update(ofEventArgs &a){
        if(requestedPosition != -1){
            vPlayer.setPosition(requestedPosition);
            requestedPosition = -1;
        }
        vPlayer.update();
        if(vPlayer.isFrameNew()){
            texture = &vPlayer.getTexture();
        }
        positionSetByItself = true;
        position = vPlayer.getPosition();// / vPlayer.getDuration();
    }
    
    void draw(ofEventArgs &a){
        //texture = &image.getTexture();
    }
    
private:
    //ofParameter<string> filename;
    ofParameter<int> fileIndex;
    ofParameter<bool> loop;
    ofParameter<bool> play;
    ofParameter<float> speed;
    ofParameter<float> position;
    ofParameter<ofTexture*> texture;
    
    bool positionSetByItself;
    float requestedPosition = 0;
    
    ofEventListeners listeners;
    
    ofVideoPlayer vPlayer;
};


#endif /* videoPlayer_h */
