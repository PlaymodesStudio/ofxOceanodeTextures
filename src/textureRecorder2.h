//
//  textureRecorder2.h
//  MIRABCN_Generator
//
//  Created by Eduard Frigola on 24/05/2018.
//
//

#ifndef textureRecorder2_h
#define textureRecorder2_h

#include "ofxOceanodeNodeModel.h"

class textureRecorder2 : public ofxOceanodeNodeModel{
public:
    textureRecorder2();
    ~textureRecorder2(){};

    void draw(ofEventArgs &a);

private:
    void phasorInListener(float &f);
    void inputListener(ofTexture* &texture);
    void recordListener(bool &b);
    void formatListener(int &f);
    void imageFormatListener(int &f);
    
    void allocateFboWithFormat(int formatIndex);
    bool isRGBAFormat(int formatIndex);

    ofEventListeners listeners;
    
    ofParameter<bool> createVideo;

    ofParameter<float>  phasorIn;
    ofParameter<bool>   record;
    ofParameter<bool>   autoRecLoop;
    ofParameter<string> filename;
    ofParameter<ofTexture*>    input;
    ofParameter<int> format;
    ofParameter<int> imageFormat;

    int width, height;
    bool recorderIsSetup;
    bool lastFrame;

    float oldPhasor;
    int frameCounter;
    string initRecordingTimestamp;

    ofFbo fbo;
};

#endif /* textureRecorder2_h */