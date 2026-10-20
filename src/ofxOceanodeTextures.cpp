//
//  ofxOceanodeTextures.cpp
//  example
//
//  Created by Eduard Frigola Bagué on 14/10/2020.
//

#include "ofxOceanodeTextures.h"
#include "simpleEffect.h"
#include "effectsLibraryUpdateUI.h"

#include "ofxOceanode.h"

#include <functional>

namespace ofxOceanodeTextures{

// ofxOceanodeTexturesNodes<Group>.cpp
void registerGeneratorNodes(ofxOceanode &o);
void registerMediaNodes(ofxOceanode &o);
void registerProcessingNodes(ofxOceanode &o);
void registerAnalysisNodes(ofxOceanode &o);

void registerModels(ofxOceanode &o){
    registerGeneratorNodes(o);
    registerMediaNodes(o);
    registerProcessingNodes(o);
    registerAnalysisNodes(o);

    std::function<void(const std::string &, const std::string &)> registerEffectsDirectory;
    registerEffectsDirectory = [&o, &registerEffectsDirectory](const std::string &directoryPath,
                                                               const std::string &relativeDirectory){
        ofDirectory directory(directoryPath);
        directory.listDir();
        directory.sort();

        for(size_t i = 0; i < directory.size(); i++){
            const ofFile file = directory.getFile(i);
            const std::string relativePath = relativeDirectory.empty()
                                           ? file.getFileName()
                                           : relativeDirectory + "/" + file.getFileName();

            if(file.isDirectory()){
                registerEffectsDirectory(file.getAbsolutePath(), relativePath);
                continue;
            }

            if(ofToLower(file.getExtension()) != "glsl"){
                continue;
            }

            ofBuffer buffer = ofBufferFromFile(file.getAbsolutePath());
            std::string config;

            if(buffer.size() > 0){
                const std::string firstLine = ofTrim(buffer.getFirstLine());
                if(firstLine.rfind("//", 0) == 0){
                    config = ofTrim(firstLine.substr(2));
                }else{
                    ofLogWarning("ofxOceanodeTextures")
                        << relativePath
                        << " has no first-line parameter metadata; registering it without custom parameters";
                }
            }else{
                ofLogWarning("ofxOceanodeTextures")
                    << relativePath
                    << " is empty; registering it so simpleEffect can expose the shader error";
            }

            const std::string category = relativeDirectory.empty()
                                       ? "Effects"
                                       : "Effects/" + relativeDirectory;
            o.registerModel<simpleEffect>(category,
                                          file.getBaseName(),
                                          config,
                                          "Effects/" + relativePath);
        }

        directory.close();
    };

    registerEffectsDirectory(ofToDataPath("Effects", true), "");

    auto updater = std::make_shared<effectsLibraryUpdateUI>(o);
    o.registerConfigMenuItem("Update Effects", [updater](){ updater->request(); },
        [updater](){ updater->update(); }, [updater](){ updater->draw(); },
        [updater](){ return !updater->busy(); });
}
void registerType(ofxOceanode &o){
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
void registerScope(ofxOceanode &o){
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
void registerCollection(ofxOceanode &o){
    registerModels(o);
    registerType(o);
    registerScope(o);
}
}
