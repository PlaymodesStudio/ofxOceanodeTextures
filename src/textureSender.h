//
//  textureSender.h
//  example
//
//  Created by Eduard Frigola on 01/03/2021.
//
//

#ifndef senderManager_h
#define senderManager_h

#include "ofxOceanodeNodeModel.h"
#ifdef TARGET_OSX
#include "ofxSyphon.h"
#elif TARGET_WIN64
//#include "ofxSpout.h"
#endif

class textureSender : public ofxOceanodeNodeModel{
public:
    textureSender() : ofxOceanodeNodeModel("Texture Sender"){
        syphonServer = nullptr;
    };
    ~textureSender(){
#ifdef TARGET_OSX
        if(syphonServer != nullptr) delete syphonServer;
#endif
    };
    
    void setup(){
        addParameter(enable.set("Enable", 1), ofxOceanodeParameterFlags_DisableSavePreset);
        addParameter(syphonName.set("Server", "Texture"), ofxOceanodeParameterFlags_DisableSavePreset);
        addParameter(masterFader.set("Opacity", 1, 0, 1), ofxOceanodeParameterFlags_DisableSavePreset);
        addParameter(textureIn.set("Texture In", nullptr));
        
        listeners.push(textureIn.newListener(this, &textureSender::sendTexture));
        
        bool tempEnable = true;
        enableSyphonListener(tempEnable);
        color = ofColor::lightGray;
    }
    
    void setname(string name){syphonName = name;};
    
    void enableSyphonListener(bool &b){
    #ifdef TARGET_OSX
        if(b){
            syphonServer = new ofxSyphonServer;
            
            syphonServer->setName(syphonName);
            
            listeners.push(syphonName.newListener(this, &textureSender::syphonNameListener));
        }else{
            listeners.unsubscribe(1);
            delete syphonServer;
        }
    #endif
    }

    void syphonNameListener(string &s){
    #ifdef TARGET_OSX
        syphonServer->setName(syphonName);
    #endif
    }
    
private:
    void sendTexture(ofTexture *&info){
        #ifdef TARGET_OSX
            if(syphonServer != NULL && enable && info != nullptr){
                if(colorFbo.getHeight() != info->getHeight() || colorFbo.getWidth() != info->getWidth()){
                    colorFbo.allocate(info->getWidth(), info->getHeight(), GL_RGB);
                }
                colorFbo.begin();
                ofClear(0, 0, 0, 255);
                ofPushStyle();
                ofSetColor(masterFader * 255);
                info->draw(0, 0);
                ofPopStyle();
                colorFbo.end();
                syphonServer->publishTexture(&colorFbo.getTexture());
            }
        #endif
    }
    	
#ifdef TARGET_OSX
    ofxSyphonServer*   syphonServer;
#else
    void* syphonServer;
#endif

    ofParameter<bool>   enable;
    ofParameter<string> syphonName;
    ofParameter<float>  masterFader;
    ofParameter<ofTexture*>    textureIn;
    
    ofFbo colorFbo;
    
    bool invert;
    
    ofEventListeners listeners;
};

#endif /* senderManager_h */
