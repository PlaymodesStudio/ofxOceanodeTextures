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
#include "ofxOceanodeShared.h"
#include <atomic>
#ifdef TARGET_OSX
#include "ofxSyphon.h"
#elif TARGET_WIN64
//#include "ofxSpout.h"
#endif

class textureReceiver : public ofxOceanodeNodeModel {
public:
    textureReceiver() : ofxOceanodeNodeModel("Texture Receiver") {}

    void setup() override {
        selector = addParameterDropdown(syphonSelector, "Server", 0, {"None"},
            ofxOceanodeParameterFlags_DisableSavePreset);
        addOutputParameter(output.set("Texture", nullptr));
        dir.setup();
        // Directory notifications only request a refresh. Resolve selection and
        // create/release GL clients on the node's normal update thread.
        auto changed = [this](ofxSyphonServerDirectoryEventArgs&) { directoryDirty = true; };
        listeners.push(dir.events.serverAnnounced.newListener(changed));
        listeners.push(dir.events.serverUpdated.newListener(changed));
        listeners.push(dir.events.serverRetired.newListener(changed));
        listeners.push(syphonSelector.newListener([this](int& index) {
            if(refreshing) return;
            selectedServer = index > 0 && size_t(index) < serverNames.size()
                ? serverNames[size_t(index)] : "None";
            refreshServers();
        }));
        refreshServers();
    }

    void update(ofEventArgs&) override {
        if(directoryDirty.exchange(false)) refreshServers();
        if(!client || !client->lockTexture()) { output = nullptr; return; }
        struct Unlock {
            ofxSyphonClient& client;
            ~Unlock() { client.unlockTexture(); }
        } unlock{*client};
        auto& texture = client->getTexture();
        const int width = int(texture.getWidth()), height = int(texture.getHeight());
        if(width <= 0 || height <= 0) { output = nullptr; return; }
        if(!fbo.isAllocated() || fbo.getWidth() != width || fbo.getHeight() != height || fbo.getTexture().getTextureData().glInternalFormat != ofxOceanodeShared::getColorTextureInternalFormat()) {
            ofFbo::Settings settings;
            settings.width = width; settings.height = height;
            settings.internalformat = ofxOceanodeShared::getColorTextureInternalFormat();
            settings.numColorbuffers = 1;
            settings.useDepth = false; settings.useStencil = false;
            settings.textureTarget = GL_TEXTURE_2D;
            settings.maxFilter = GL_NEAREST; settings.minFilter = GL_NEAREST;
            fbo.allocate(settings);
        }
        fbo.begin();
        ofClear(0, 0, 0, 0);
        ofPushStyle();
        ofSetColor(255);
        ofDisableAlphaBlending(); // Copy the received RGBA values without compositing twice.
        texture.draw(0, 0);
        ofPopStyle();
        fbo.end();
        output = &fbo.getTexture();
    }

    void presetSave(ofJson& json) override {
        // Preserve the requested name even when its server is temporarily absent.
        json["ServerName"] = selectedServer;
    }

    void presetRecallAfterSettingParameters(ofJson& json) override {
        selectedServer = json.contains("ServerName") && json["ServerName"].is_string()
            ? json["ServerName"].get<string>() : "None";
        refreshServers();
    }

    void activate() override { directoryDirty = true; }
    void deactivate() override {
        output = nullptr;
        client.reset();
        connectedServer.clear();
        fbo.clear();
    }

private:
    void refreshServers() {
        serverNames = {"None"};
        const auto& servers = dir.getServerList();
        int selected = 0;
        for(const auto& server : servers) {
            serverNames.push_back(server.serverName + "-" + server.appName);
            if(serverNames.back() == selectedServer) selected = int(serverNames.size()) - 1;
        }
        refreshing = true;
        selector->setDropdownOptions(serverNames);
        syphonSelector.setMax(int(serverNames.size()) - 1);
        syphonSelector = selected;
        refreshing = false;
        if(selected == 0) {
            output = nullptr;
            client.reset();
            connectedServer.clear();
        } else if(!client || connectedServer != selectedServer) {
            output = nullptr;
            client = std::make_unique<ofxSyphonClient>();
            client->setup();
            client->set(servers[size_t(selected - 1)]);
            connectedServer = selectedServer;
        }
    }

    vector<string> serverNames;
    string selectedServer = "None", connectedServer;
    bool refreshing = false;
    std::atomic<bool> directoryDirty{false};
    ofxSyphonServerDirectory dir;
    std::unique_ptr<ofxSyphonClient> client;
    ofParameter<int> syphonSelector;
    shared_ptr<ofxOceanodeParameter<int>> selector;
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
        addInspectorParameter(directPublish.set("Direct Publish (Keep Alpha)", false));
        
        listeners.push(textureIn.newListener(this, &textureSender::sendTexture));
        
        listeners.push(enable.newListener(this, &textureSender::enableSyphonListener));
        color = ofColor::lightGray;
    }
    
    void setname(string name){syphonName = name;};
    
    void enableSyphonListener(bool &b){
    #ifdef TARGET_OSX
        if(b){
			if(syphonServer!=nullptr) return;
			
            syphonServer = new ofxSyphonServer;
            
            syphonServer->setName(syphonName);
            
            syphonNameEventListener = syphonName.newListener(this, &textureSender::syphonNameListener);
        }else{
            syphonNameEventListener.unsubscribe();
            delete syphonServer;
			syphonServer=nullptr;
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
            if(syphonServer != NULL && enable && info != nullptr && info->isAllocated()){
                if(directPublish && masterFader == 1.0f){
                    syphonServer->publishTexture(info);
                    return;
                }
                if(!colorFbo.isAllocated() || colorFbo.getHeight() != info->getHeight() || colorFbo.getWidth() != info->getWidth() || colorFbo.getTexture().getTextureData().glInternalFormat != ofxOceanodeShared::getColorTextureInternalFormat()){
                    // Fresh settings avoid retaining the previous attachment colorFormats.
                    ofFbo::Settings settings;
                    settings.width = info->getWidth(); settings.height = info->getHeight();
                    settings.internalformat = ofxOceanodeShared::getColorTextureInternalFormat();
                    settings.useDepth = false; settings.useStencil = false;
                    colorFbo.allocate(settings);
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
    ofParameter<bool> directPublish;
    ofParameter<string> syphonName;
    ofParameter<float>  masterFader;
    ofParameter<ofTexture*>    textureIn;
    
    ofFbo colorFbo;
    
    bool invert;
    
    ofEventListeners listeners;
    ofEventListener syphonNameEventListener;
};

#endif /* senderManager_h */
