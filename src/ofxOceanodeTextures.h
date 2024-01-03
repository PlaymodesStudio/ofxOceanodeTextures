//
//  ofxOceanodeTextures.h
//  example
//
//  Created by Eduard Frigola Bagué on 14/10/2020.
//

#ifndef ofxOceanodeTextures_h
#define ofxOceanodeTextures_h

#include "indexerTexture.h"
#include "oscillatorTexture.h"
#include "chaoticOscillatorTexture.h"
#include "imageLoader.h"
#include "mixer.h"
#include "textureHelpers.h"
#include "videoPlayer.h"
#include "noiseTexture.h"
#include "textureSender.h"
#include "Gradient.h"
#include "textureResize.h"
//#include "senderManager.h"
//#include "colorApplier.h"
#include "textureRecorder.h"
#include "textureUnifier.h"
//#include "textureReader.h"
//#include "textureMixer.h"
//#include "oscTextureSender.h"
//#include "vectorToTexture.h"
//#include "textureResize.h"
#include "displayOutput.h"
#include "simpleEffect.h"

#include "ofxOceanode.h"

namespace ofxOceanodeTextures{
static void registerModels(ofxOceanode &o){
    o.registerModel<indexerTexture>("Textures");
    o.registerModel<oscillatorTexture>("Textures");
    o.registerModel<chaoticOscillatorTexture>("Textures");
    o.registerModel<imageLoader>("Textures");
    o.registerModel<mixer>("Textures");
    o.registerModel<interactiveCanvas>("Textures");
    o.registerModel<videoPlayer>("Textures");
    o.registerModel<noiseTexture>("Textures");
    o.registerModel<textureSender>("Textures");
	o.registerModel<vectorToTexture>("Textures");
	o.registerModel<textureReader>("Textures");
	o.registerModel<Gradient>("Textures");
	o.registerModel<textureResize>("Texture");
	o.registerModel<textureUnifier>("Texture");
	o.registerModel<textureReceiver>("Textures");
	o.registerModel<subTexture>("Textures");
	o.registerModel<textureRecorder>("Textures");
    o.registerModel<oscillatorTexture2>("Textures");
    o.registerModel<indexerTexture2>("Textures");
    o.registerModel<displayOutput>("Textures");
    
    ofDirectory dir("Effects");
    for(auto f : dir.getFiles()){
        ofFile file(f.getAbsolutePath());
        ofBuffer buffer(file);
        std::string config = buffer.getFirstLine();
        config.erase(0,2); //Removes First Character
        
        std::string fileName = file.getFileName();
        fileName.erase(fileName.size()-5, 5); //Removes .glsl
        
        o.registerModel<simpleEffect>("Effects", fileName, config);
    }
}
static void registerType(ofxOceanode &o){
    auto textureBufferAssignFunction = [](ofTexture* &tex, ofFbo &fbo){
        if(fbo.getWidth() != tex->getWidth() ||
           fbo.getHeight() != tex->getHeight() ||
           fbo.getTexture().texData.glInternalFormat != tex->texData.glInternalFormat){
            fbo.allocate(tex->getWidth(), tex->getHeight(), tex->texData.glInternalFormat);
        }
        fbo.begin();
        tex->draw(0, 0);
        fbo.end();
    };
    
    auto textureBufferReturnFunction =  [](ofFbo &fbo)->ofTexture*{
        return &fbo.getTexture();
    };
    
    auto textureBufferCheckFunction = [](ofTexture* &data)->bool{return data != nullptr;};
    
    o.registerTypeWithBufferAndHeader<ofTexture*, ofFbo>("Texture", nullptr, textureBufferAssignFunction,
                                                            textureBufferReturnFunction,
                                                            textureBufferCheckFunction);
}
static void registerScope(ofxOceanode &o){
    o.registerScope<ofTexture*>([](ofxOceanodeAbstractParameter *p, ImVec2 size){
        auto tex = p->cast<ofTexture*>().getParameter().get();
        auto size2 = ImGui::GetContentRegionAvail();
        bool keepAspectRatio = (p->getFlags() & ofxOceanodeParameterFlags_ScopeKeepAspectRatio);
        float sizeAspectRatio=size.x/size.y;
        float texAspectRatio;
        if(tex != nullptr){
            texAspectRatio = tex->getWidth() / tex->getHeight();
        }
        if(keepAspectRatio)
        {
            if(sizeAspectRatio<texAspectRatio)
            {
                size2.y = size2.x / texAspectRatio;
                size2.x = size.x;
            }
            else
            {
                size2.x = size2.y * texAspectRatio;
                size2.y = size.y;
            }
        }
        
        if(tex != nullptr){
            ImTextureID textureID = (ImTextureID)(uintptr_t)tex->texData.textureID;
            ImGui::Image(textureID, size2);
        }
    });
    o.registerScope<vector<ofTexture*>>([](ofxOceanodeAbstractParameter *p, ImVec2 size){
        auto vtex = p->cast<vector<ofTexture*>>().getParameter().get();
        auto size2 = ImGui::GetContentRegionAvail();
        bool keepAspectRatio = (p->getFlags() & ofxOceanodeParameterFlags_ScopeKeepAspectRatio);
        float sizeAspectRatio=size.x/size.y;
        
        auto cursorpos = ImGui::GetCursorPos();
        for(int i = 0; i < vtex.size(); i++){
            auto tex = vtex[i];
            float texAspectRatio;
            if(tex != nullptr){
                texAspectRatio = tex->getWidth() / tex->getHeight();
                if(keepAspectRatio)
                {
                    if(sizeAspectRatio<texAspectRatio)
                    {
                        size2.y = size2.x / texAspectRatio;
                        size2.x = size.x;
                    }
                    else
                    {
                        size2.x = size2.y * texAspectRatio;
                        size2.y = size.y;
                    }
                }
                
                ImGui::SetCursorPos(cursorpos);
                ImTextureID textureID = (ImTextureID)(uintptr_t)tex->texData.textureID;
                ImGui::Image(textureID, size2, ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, sqrt(1.0/vtex.size())));
            }
        }
    });
}
static void registerCollection(ofxOceanode &o){
    registerModels(o);
    registerType(o);
    registerScope(o);
}
}

#endif /* ofxOceanodeTextures_h */
