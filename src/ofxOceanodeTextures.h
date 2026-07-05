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
#include "mixerAlpha.h"
#include "interactiveCanvas.h"
#include "textureReader.h"
#include "vectorToTexture.h"
#include "subTexture.h"
#include "videoPlayer.h"
#include "noiseTexture.h"
#include "textureSender.h"
#include "Gradient.h"
#include "Gradient2.h"
#include "ColorByValue.h"
//#include "ColorCycler.h"
//#include "ReColorCycle.h"
#include "textureResize.h"
//#include "senderManager.h"
//#include "colorApplier.h"
#include "textureRecorder.h"
#include "textureUnifier.h"
//#include "textureMixer.h"
//#include "oscTextureSender.h"
#include "textureResizeFast.h"
#include "displayOutput.h"
#include "simpleEffect.h"
#include "textureComposer.h"
#include "textureBlender.h"
#include "averageBrightness.h"
#include "textureInfo.h"
#include "injectAlpha.h"
#include "AlphaTrails.h"
#include "TextureChannels.h"
#include "textureUnitMonitor.h"
#include "ColorTexture.h"
#include "textureDisplay.h"
#include "videoPlayerScrub.h"

#include "ofxOceanode.h"

namespace ofxOceanodeTextures{
static void registerModels(ofxOceanode &o){
    o.registerModel<indexerTexture>("Textures");
    o.registerModel<oscillatorTexture>("Textures");
    o.registerModel<chaoticOscillatorTexture>("Textures");
    o.registerModel<imageLoader>("Textures");
    o.registerModel<mixer>("Textures");
    o.registerModel<mixerAlpha>("Textures");
    o.registerModel<interactiveCanvas>("Textures");
    o.registerModel<videoPlayer>("Textures");
    o.registerModel<noiseTexture>("Textures");
    o.registerModel<textureSender>("Textures");
	o.registerModel<vectorToTexture>("Textures");
	o.registerModel<textureReader>("Textures");
	o.registerModel<Gradient>("Textures");
	o.registerModel<Gradient2>("Textures");
	o.registerModel<ColorByValue>("Textures");
	//o.registerModel<ColorCycler>("Textures");
	//o.registerModel<ReColorCycle>("Textures");
	o.registerModel<textureResize>("Textures");
	o.registerModel<textureUnifier>("Textures");
	o.registerModel<textureReceiver>("Textures");
	o.registerModel<subTexture>("Textures");
	o.registerModel<textureRecorder>("Textures");
    o.registerModel<oscillatorTexture2>("Textures");
    o.registerModel<indexerTexture2>("Textures");
    o.registerModel<displayOutput>("Textures");
    o.registerModel<textureComposer>("Textures");
    o.registerModel<textureBlender>("Textures");
    o.registerModel<AverageBrightness>("Textures");
    o.registerModel<textureInfo>("Textures");
	o.registerModel<textureResizeFast>("Textures");
	
    
    o.registerModel<injectAlpha>("Textures");
    o.registerModel<AlphaTrails>("Textures");
    o.registerModel<TextureChannels>("Textures");
    o.registerModel<ColorTexture>("Textures");
	o.registerModel<textureDisplay>("Textures");
	o.registerModel<videoPlayerScrub>("Textures");
	
    o.registerModel<textureUnitMonitor>("Debug");
	
	
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
    dir.close();
}
static void registerType(ofxOceanode &o){
    auto textureBufferAssignFunction = [](ofTexture* &tex, ofFbo &fbo){
        if(fbo.getWidth() != tex->getWidth() ||
           fbo.getHeight() != tex->getHeight() ||
           fbo.getTexture().texData.glInternalFormat != tex->texData.glInternalFormat){
            fbo.allocate(tex->getWidth(), tex->getHeight(), tex->texData.glInternalFormat);
        }
        fbo.begin();
        ofClear(0,0,0,255);
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
    o.registerType<std::vector<glm::mat4>>("mat4", {glm::identity<glm::mat4>()});
}
static void registerScope(ofxOceanode &o){
    o.registerScope<ofTexture*>([](ofxOceanodeAbstractParameter *p, ImVec2 size){
        auto tex = p->cast<ofTexture*>().getParameter().get();
        auto &absParam = *p;
        bool keepAspectRatio = (p->getFlags() & ofxOceanodeParameterFlags_ScopeKeepAspectRatio);
        float sizeAspectRatio=size.x/size.y;
        float texAspectRatio;
        if(tex != nullptr){
            texAspectRatio = tex->getWidth() / tex->getHeight();
        }
        if(keepAspectRatio && tex != nullptr)
        {
            if(sizeAspectRatio<texAspectRatio)
            {
                // Width is limiting factor
                size.x = size.x;
                size.y = size.x / texAspectRatio;
            }
            else
            {
                // Height is limiting factor
                size.y = size.y;
                size.x = size.y * texAspectRatio;
            }
        }
        
        if(tex != nullptr){
            // Save cursor position before drawing image
            ImVec2 imagePos = ImGui::GetCursorPos();
            
            ImTextureID textureID = (ImTextureID)(uintptr_t)tex->texData.textureID;
            ImGui::Image(textureID, size);
            
            // Restore cursor position to overlay button
            ImGui::SetCursorPos(ImVec2(imagePos.x + size.x - 20, imagePos.y + 5));
            
            // Draw the [AR] toggle button
            if(keepAspectRatio) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0,0.5,0.0,0.5));
            else ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55,0.55,0.55,0.5));
            
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0, 0.0, 0.0, 0.0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(0.0, 0.0, 0.0, 0.0));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(0.0, 0.0, 0.0, 0.0));
			
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            
            if(ImGui::Button("[]##KeepAspectRatioNode"))
            {
                if(keepAspectRatio){
                    absParam.setFlags(absParam.getFlags()&~ofxOceanodeParameterFlags_ScopeKeepAspectRatio);
                }
                else absParam.setFlags(absParam.getFlags()|ofxOceanodeParameterFlags_ScopeKeepAspectRatio);
            }
            
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(4);
        }
    });
	
    o.registerScope<vector<ofTexture*>>([](ofxOceanodeAbstractParameter *p, ImVec2 size){
        auto vtex = p->cast<vector<ofTexture*>>().getParameter().get();
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
                        size.y = size.x / texAspectRatio;
                        size.x = size.x;
                    }
                    else
                    {
                        size.x = size.y * texAspectRatio;
                        size.y = size.y;
                    }
                }
                
                ImGui::SetCursorPos(cursorpos);
                ImTextureID textureID = (ImTextureID)(uintptr_t)tex->texData.textureID;
                ImGui::ImageWithBg(textureID, size, ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, sqrt(1.0/vtex.size())));
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
