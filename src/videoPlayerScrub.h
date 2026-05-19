//
//  videoPlayerScrub.h
//
//  Video Player Scrub — position is an input that drives frame seeking.
//

#ifndef videoPlayerScrub_h
#define videoPlayerScrub_h

#include "ofxOceanodeNodeModel.h"

class videoPlayerScrub : public ofxOceanodeNodeModel {
public:
    videoPlayerScrub() : ofxOceanodeNodeModel("Video Player Scrub"){}

    void setup(){
        blackTexture.allocate(1, 1, GL_RGBA32F);

        ofDirectory dir;
        dir.open("Movies");
        dir.sort();
        vector<string> files = {"None"};
        for(int i = 0; i < dir.listDir(); i++){
            files.push_back(dir.getName(i));
        }
        dir.close();

        addParameterDropdown(fileIndex, "File s", 0, files);
        addParameter(position.set("Position", 0, 0, 1));
        addOutputParameter(texture.set("Output", nullptr));

        listeners.push(fileIndex.newListener([this, files](int &i){
            string filename = files[i];
            if(filename == "None"){
                vPlayer.stop();
                vPlayer.close();
            } else {
                vPlayer.load("Movies/" + filename);
                vPlayer.setLoopState(OF_LOOP_NORMAL);
                vPlayer.play();
                vPlayer.setPaused(true);
            }
        }));
    }

    void update(ofEventArgs &a){
        if(vPlayer.isLoaded()){
            vPlayer.setPosition(position);
            vPlayer.update();
            if(vPlayer.isFrameNew()){
                texture = &vPlayer.getTexture();
            }
        } else {
            texture = &blackTexture;
        }
    }

    void deactivate(){
        texture = nullptr;
    }

private:
    ofParameter<int> fileIndex;
    ofParameter<float> position;
    ofParameter<ofTexture*> texture;

    ofEventListeners listeners;

    ofVideoPlayer vPlayer;
    ofTexture blackTexture;
};

#endif /* videoPlayerScrub_h */
