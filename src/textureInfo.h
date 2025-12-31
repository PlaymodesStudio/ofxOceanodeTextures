
#ifndef textureInfo_h
#define textureInfo_h

#include <stdio.h>
#include "ofxOceanodeNodeModel.h"

class textureInfo : public ofxOceanodeNodeModel
{
public:
    textureInfo() : ofxOceanodeNodeModel("Texture Info"){
    }
    
    void setup()override
    {
        addParameter(input.set("Input",nullptr));
        addOutputParameter(ms.set("Ms",0,0,FLT_MAX));
        addOutputParameter(width.set("Width",0,0,FLT_MAX));
        addOutputParameter(height.set("Height",0,0,FLT_MAX));
        
        textureInListener = input.newListener([this](ofTexture* &t){
            if(input.get()!=nullptr)
            {
                if(input.get()->isAllocated())
                {
                    float now = ofGetElapsedTimeMillis();
                    ms = now-lastTime;
                    lastTime = now;
                    
                    width = input.get()->getWidth();
                    height = input.get()->getHeight();
                }
            }
        });
        
        
    }
    
    void update(ofEventArgs &args)override
    {

    }
    
private:
    ofEventListener textureInListener;
    
    ofParameter<ofTexture*> input;
    ofParameter<float> ms;
    ofParameter<int> width;
    ofParameter<int> height;
    
    float lastTime;
    
    
 

};

#endif /* textureInfo_h */
