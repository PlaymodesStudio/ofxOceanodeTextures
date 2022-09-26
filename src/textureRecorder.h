//
//  textureRecorder.h
//  MIRABCN_Generator
//
//  Created by Eduard Frigola on 24/05/2018.
//
//

#ifndef textureRecorder_h
#define textureRecorder_h

#include "ofxOceanodeNodeModel.h"

class textureRecorder : public ofxOceanodeNodeModel{
public:
    textureRecorder();
    ~textureRecorder(){};

    void draw(ofEventArgs &a);

private:
    void phasorInListener(float &f);
    void inputListener(ofTexture* &texture);
    void recordListener(bool &b);

    ofEventListeners listeners;
    
    ofParameter<bool> createVideo;

    ofParameter<float>  phasorIn;
    ofParameter<bool>   record;
    ofParameter<bool>   autoRecLoop;
    ofParameter<string> filename;
    ofParameter<ofTexture*>    input;

    int width, height;
    bool recorderIsSetup;
    bool lastFrame;

    float oldPhasor;
    int frameCounter;
    string initRecordingTimestamp;

    ofFbo fbo;
};

#endif /* textureRecorder_h */
