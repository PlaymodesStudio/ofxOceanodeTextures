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

class textureReceiver : public ofxOceanodeNodeModel{
public:
	textureReceiver() : ofxOceanodeNodeModel("Texture Receiver"){}
	
	void setup(){
		dir.setup();
		client.setup();


		directoriesStrings = {"None"};
		for( auto& server : dir.getServerList()){
			directoriesStrings.push_back(server.serverName + "-" + server.appName);
		}

		auto selectorRef = addParameterDropdown(syphonSelector, "Server", 0, directoriesStrings);
		addOutputParameter(output.set("Texture", nullptr));

		listeners.push(dir.events.serverAnnounced.newListener([this, selectorRef](ofxSyphonServerDirectoryEventArgs &arg){
			directoriesStrings = {"None"};
			for( auto& server : dir.getServerList()){
				directoriesStrings.push_back(server.serverName + "-" + server.appName);
			}
			selectorRef->setDropdownOptions(directoriesStrings);
			syphonSelector.setMax(directoriesStrings.size());
		}));

		listeners.push(dir.events.serverUpdated.newListener([this, selectorRef](ofxSyphonServerDirectoryEventArgs &arg){
			directoriesStrings = {"None"};
			for( auto& server : dir.getServerList()){
				directoriesStrings.push_back(server.serverName + "-" + server.appName);
			}
			selectorRef->setDropdownOptions(directoriesStrings);
			syphonSelector.setMax(directoriesStrings.size());
		}));

		listeners.push(dir.events.serverRetired.newListener([this, selectorRef](ofxSyphonServerDirectoryEventArgs &arg){
			directoriesStrings = {"None"};
			for( auto& server : dir.getServerList()){
				directoriesStrings.push_back(server.serverName + "-" + server.appName);
			}
			selectorRef->setDropdownOptions(directoriesStrings);
			syphonSelector.setMax(directoriesStrings.size());
		}));

		listeners.push(syphonSelector.newListener([this](int &i){
			if(i > 0){
				client.set(dir.getDescription(i-1));
			}
		}));
	}
	
	void update(ofEventArgs &a){
		if(dir.isValidIndex(syphonSelector-1)){
			client.bind();
			int width = client.getWidth();
			int height = client.getHeight();
			client.unbind();
			if((!fbo.isAllocated()
			   || fbo.getWidth() != width
			   || fbo.getHeight() != height)
			   && width != 0
			   && height != 0){
				ofFbo::Settings fboSettings;
                fboSettings.width = width;
                fboSettings.height = height;
                fboSettings.internalformat = GL_RGBA32F;
				fboSettings.numColorbuffers = 1;
                fboSettings.useDepth = false;
                fboSettings.useStencil = false;
                fboSettings.textureTarget = GL_TEXTURE_2D;
                fboSettings.maxFilter = GL_NEAREST;
                fboSettings.minFilter = GL_NEAREST;
				fbo.allocate(fboSettings);
			}
			fbo.begin();
			ofClear(0, 0, 0, 0);
			client.draw(0, 0);
			fbo.end();

			output = &fbo.getTexture();
		}else{
			output = nullptr;
		}
	}
	
	void serverAnnounced(ofxSyphonServerDirectoryEventArgs &arg);
	void serverUpdated(ofxSyphonServerDirectoryEventArgs &args);
	void serverRetired(ofxSyphonServerDirectoryEventArgs &arg);
	
private:
	vector<string> directoriesStrings;
	ofxSyphonServerDirectory dir;
	ofxSyphonClient client;
	ofParameter<int> syphonSelector;
	ofParameter<ofTexture*> output;

	ofFbo fbo;

	ofEventListeners listeners;
};

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
        addParameter(enable.set("Enable", 0), ofxOceanodeParameterFlags_DisableSavePreset);
        addParameter(syphonName.set("Server", "Texture"));
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
	
	void presetHasLoaded(){
		enable = true;
	}
    
private:
    void sendTexture(ofTexture *&info){
        #ifdef TARGET_OSX
            if(syphonServer != NULL && enable && info != nullptr){
                if(colorFbo.getHeight() != info->getHeight() || colorFbo.getWidth() != info->getWidth()){
                    colorFbo.allocate(info->getWidth(), info->getHeight(), GL_RGBA32F);
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
