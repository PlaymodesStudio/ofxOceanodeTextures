
#ifndef textureInfo_h
#define textureInfo_h

#include <stdio.h>
#include "ofxOceanodeNodeModel.h"

class textureInfo : public ofxOceanodeNodeModel
{
public:
    textureInfo() : ofxOceanodeNodeModel("textureInfo"){
    }
    
    void setup()override
    {
        addParameter(input.set("Input",nullptr));
        addOutputParameter(ms.set("Ms",0,0,FLT_MAX));
        addOutputParameter(width.set("Width",0,0,FLT_MAX));
        addOutputParameter(height.set("Height",0,0,FLT_MAX));
        addOutputParameter(format.set("Format",""));
        
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
                    
                    // Get texture format information
                    int glInternalFormat = input.get()->getTextureData().glInternalFormat;
                    string formatStr = "";
                    
                    switch(glInternalFormat) {
                        case GL_RGBA32F: formatStr = "RGBA 32f"; break;
                        case GL_RGBA16F: formatStr = "RGBA 16f"; break;
                        case GL_RGBA8: formatStr = "RGBA 8"; break;
                        case GL_RGB32F: formatStr = "RGB 32f"; break;
                        case GL_RGB16F: formatStr = "RGB 16f"; break;
                        case GL_RGB8: formatStr = "RGB 8"; break;
                        case GL_RG32F: formatStr = "RG 32f"; break;
                        case GL_RG16F: formatStr = "RG 16f"; break;
                        case GL_RG8: formatStr = "RG 8"; break;
                        case GL_R32F: formatStr = "R 32f"; break;
                        case GL_R16F: formatStr = "R 16f"; break;
                        case GL_R8: formatStr = "R 8"; break;
                        default: formatStr = "Unknown (0x" + ofToHex(glInternalFormat) + ")"; break;
                    }
                    
                    format = formatStr;
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
    ofParameter<string> format;
    
    float lastTime;
    
    
 

};

#endif /* textureInfo_h */
