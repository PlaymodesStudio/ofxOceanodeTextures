//
//  textureHelpers.h
//  lille
//
//  Created by Eduard Frigola Bagué on 08/02/2021.
//

#ifndef textureHelpers_h
#define textureHelpers_h

#include "ofxOceanodeNodeModel.h"

class interactiveCanvas : public ofxOceanodeNodeModel{
public:
    interactiveCanvas() : ofxOceanodeNodeModel("Interactive Canvas"){}
    
    void setup(){
        addParameter(input.set("Input", nullptr));
        addParameter(x.set("X", 0.5, 0, 1));
        addParameter(y.set("Y", 0.5, 0, 1));
        addParameter(force.set("Force", 0, -1, 1));
        
        black.allocate(4, 4, GL_RGB);
//        black.begin();
//        ofClear(0, 0, 0, 255);
//        black.end();
    }
    
    void draw(ofEventArgs &a){
        if(ImGui::Begin(("Interactive Canvas " + ofToString(getNumIdentifier())).c_str())){
            auto screenSize = ImGui::GetContentRegionAvail();
            auto screenPos = ImGui::GetCursorScreenPos();
            ImTextureID textureID = (ImTextureID)(uintptr_t)black.texData.textureID;
            if(input.get() != nullptr){
                textureID = (ImTextureID)(uintptr_t)input.get()->texData.textureID;
            }
            ImGui::Image(textureID, screenSize);
            if(ImGui::IsWindowHovered()){
                if(ImGui::IsMouseDown(0)){
                    auto normPos = (ImGui::GetMousePos() - screenPos) / screenSize;
                    x = normPos.x;
                    y = normPos.y;
                    force = 1;
                }
                else if(ImGui::IsMouseDown(1)){
                    auto normPos = (ImGui::GetMousePos() - screenPos) / screenSize;
                    x = normPos.x;
                    y = normPos.y;
                    force = -1;
                }
                else{
                    force = 0;
                }
            }
        }
        ImGui::End();
    }
private:
    ofParameter<ofTexture*> input;
    ofTexture black;
    ofParameter<float> x, y, force;
};

#endif /* textureHelpers_h */
