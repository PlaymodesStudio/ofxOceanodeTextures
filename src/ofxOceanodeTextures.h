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
//#include "senderManager.h"
//#include "colorApplier.h"
//#include "textureRecorder.h"
//#include "textureUnifier.h"
//#include "textureReader.h"
//#include "textureMixer.h"
//#include "oscTextureSender.h"
//#include "vectorToTexture.h"
//#include "textureResize.h"

#include "ofxOceanode.h"

namespace ofxOceanodeTextures{
static void registerModels(ofxOceanode &o){
    o.registerModel<indexerTexture>("Textures");
    o.registerModel<oscillatorTexture>("Textures");
    o.registerModel<chaoticOscillatorTexture>("Textures");
}
static void registerType(ofxOceanode &o){
    o.registerType<ofTexture*>();//"Texture");
}
static void registerScope(ofxOceanode &o){
    o.registerScope<ofTexture*>([](ofxOceanodeAbstractParameter *p, ImVec2 size){
        auto tex = p->cast<ofTexture*>().getParameter().get();
        auto size2 = ImGui::GetContentRegionAvail();

        if(tex != nullptr){
            ImTextureID textureID = (ImTextureID)(uintptr_t)tex->texData.textureID;
            ImGui::Image(textureID, size2);
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
