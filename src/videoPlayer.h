//
//  videoPlayer.h
//  lille
//
//  Created by Eduard Frigola Bagué on 16/02/2021.
//

#ifndef videoPlayer_h
#define videoPlayer_h

#include "ofxOceanodeNodeModel.h"
#include <algorithm>

class videoPlayer : public ofxOceanodeNodeModel {
public:
    videoPlayer() : ofxOceanodeNodeModel("Video Player"){}
    
    void setup(){
        blackTexture.allocate(1, 1, GL_RGBA32F);
        
        ofDirectory dir;
        dir.open("Movies");
        dir.sort();
        files = {"None"};
        int fileCount = dir.listDir();
        for(int i = 0; i < fileCount; i++){
            files.push_back(dir.getName(i));
        }
        dir.close();
        
        addParameterDropdown(fileIndex, "File s", 0, files,
                             ofxOceanodeParameterFlags_DisableSavePreset |
                             ofxOceanodeParameterFlags_DisableSaveProject);
        addParameter(loop.set("Loop", true));
        addParameter(play.set("Play", false));
        addParameter(speed.set("Speed", 1, 0, 10));
        addParameter(position.set("Position", 0, 0, 1));
        addOutputParameter(texture.set("Output", nullptr));
        
        listeners.push(fileIndex.newListener([this](int &i){
            if(i <= 0 || i >= static_cast<int>(files.size())){
                vPlayer.stop();
                vPlayer.close();
                texture = &blackTexture;
            }else{
                const string &filename = files[i];
                vPlayer.load("Movies/" + filename);
                vPlayer.setLoopState(loop ? OF_LOOP_NORMAL : OF_LOOP_NONE);
                vPlayer.setSpeed(speed);
                if(play) vPlayer.play();
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
        if(vPlayer.isPlaying())
        {
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
        else texture=&blackTexture;
    }
    
    void draw(ofEventArgs &a){
        //texture = &image.getTexture();
    }
    
    void deactivate(){
        texture = nullptr;
    }

    void presetSave(ofJson &json) override {
        const int index = fileIndex.get();
        json["selectedVideoFile"] = index > 0 && index < static_cast<int>(files.size())
                                  ? files[index] : "";
    }

    void presetRecallAfterSettingParameters(ofJson &json) override {
        if(getOceanodeParameter(fileIndex).hasInConnection()) return;

        auto selected = json.find("selectedVideoFile");
        if(selected != json.end() && selected->is_string()){
            const string filename = selected->get<string>();
            auto it = std::find(files.begin() + 1, files.end(), filename);
            fileIndex = it == files.end() ? 0 : static_cast<int>(it - files.begin());
        }else{
            // Presets saved before filename persistence only have the dropdown index.
            auto legacy = json.find(fileIndex.getEscapedName());
            if(legacy != json.end() && legacy->is_number_integer()){
                int index = legacy->get<int>();
                fileIndex = index >= 0 && index < static_cast<int>(files.size()) ? index : 0;
            }
        }
    }
    
private:
    vector<string> files;
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
    ofTexture blackTexture;
};


#endif /* videoPlayer_h */
