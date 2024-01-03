//
//  textureComposer.cpp
//  ofxOceanodeTextures
//
//  Created by Eduard Frigola on 22/12/23.
//

#include "textureComposer.h"

//------------------------------------------------------------------
textureComposer::textureComposer(): ofxOceanodeNodeModel("Texture Composer"){
    
}
//------------------------------------------------------------------
textureComposer::~textureComposer(){
    
}

//------------------------------------------------------------------
void textureComposer::setup()
{
    color = ofColor::white;

    addParameter(width.set("Width", 100, 1, INT_MAX));
    addParameter(height.set("Height", 100, 1, INT_MAX));
    
    addParameter(input.set("Input", {nullptr}));
    addParameter(transformInput.set("T. In", {glm::identity<glm::mat4>()}));
    anchor.resize(3);
    addParameter(anchor[0].set("Anch.X", {0}, {-FLT_MAX}, {FLT_MAX}));
    addParameter(anchor[1].set("Anch.Y", {0}, {-FLT_MAX}, {FLT_MAX}));
    addParameter(anchor[2].set("Anch.Z", {0}, {-FLT_MAX}, {FLT_MAX}));
    
    position.resize(3);
    addParameter(position[0].set("Pos.X", {0}, {-FLT_MAX}, {FLT_MAX}));
    addParameter(position[1].set("Pos.Y", {0}, {-FLT_MAX}, {FLT_MAX}));
    addParameter(position[2].set("Pos.Z", {0}, {-FLT_MAX}, {FLT_MAX}));
    
    rotation.resize(3);
    addParameter(rotation[0].set("Rot.X", {0}, {-1}, {1}));
    addParameter(rotation[1].set("Rot.Y", {0}, {-1}, {1}));
    addParameter(rotation[2].set("Rot.Z", {0}, {-1}, {1}));
    
    scale.resize(3);
    addParameter(scale[0].set("Sca.X", {1}, {0}, {FLT_MAX}));
    addParameter(scale[1].set("Sca.Y", {1}, {0}, {FLT_MAX}));
    addParameter(scale[2].set("Sca.Z", {1}, {0}, {FLT_MAX}));

    addParameter(output.set("Output", {nullptr}));
    addParameter(transformOutput.set("T. Out", {glm::identity<glm::mat4>()}));
    
    addInspectorParameter(normalizedAnchor.set("Normalized Anchor", false));
    addInspectorParameter(normalizedPosition.set("Normalized Positioning", false));
    
//    listeners.push(normalizedAnchor.newListener([this](bool &b){
//        if(!b){
//            if(!getOceanodeParameter(anchor[0]).hasInConnection() && input.get()[0] != nullptr){
//                anchor[0] = vector<float>(1, anchor[0]->at(0) * input.get()[0]->getWidth());
//            }
//            if(!getOceanodeParameter(anchor[1]).hasInConnection() && input.get()[0] != nullptr){
//                anchor[1] = vector<float>(1, anchor[1]->at(0) * input.get()[0]->getHeight());
//            }
//        }else{
//            if(!getOceanodeParameter(anchor[0]).hasInConnection() && input.get()[0] != nullptr){
//                anchor[0] = vector<float>(1, anchor[0]->at(0) / input.get()[0]->getWidth());
//            }
//            if(!getOceanodeParameter(anchor[1]).hasInConnection() && input.get()[0] != nullptr){
//                anchor[1] = vector<float>(1, anchor[1]->at(0) / input.get()[0]->getHeight());
//            }
//        }
//    }));
//
//    listeners.push(normalizedPosition.newListener([this](bool &b){
//        if(!b){
//            if(!getOceanodeParameter(position[0]).hasInConnection()){
//                position[0] = vector<float>(1, position[0]->at(0) * width);
//            }
//            if(!getOceanodeParameter(position[1]).hasInConnection()){
//                position[1] = vector<float>(1, position[1]->at(0) * height);
//            }
//        }else{
//            if(!getOceanodeParameter(position[0]).hasInConnection()){
//                position[0] = vector<float>(1, position[0]->at(0) / width);
//            }
//            if(!getOceanodeParameter(position[1]).hasInConnection()){
//                position[1] = vector<float>(1, position[1]->at(0) / height);
//            }
//        }
//    }));
    
    listeners.push(input.newListener([this](vector<ofTexture*> &textures){
        calculate();
    }));
}

void textureComposer::calculate(){
    if(input->size() > 0){
        vector<glm::mat4> matrices(input->size());
        for(int i = 0; i < input->size(); i++){
            ofTexture* tex = input->at(i);
            if(input->at(i) != nullptr){
                
                //TRANSFORM VEC
                float anch_x = getValueForPosition(anchor[0], i);
                float anch_y = getValueForPosition(anchor[1], i);
                float anch_z = getValueForPosition(anchor[2], i);
                float pos_x = getValueForPosition(position[0], i);
                float pos_y = getValueForPosition(position[1], i);
                float pos_z = getValueForPosition(position[2], i);
                float rot_x = getValueForPosition(rotation[0], i);
                float rot_y = getValueForPosition(rotation[1], i);
                float rot_z = getValueForPosition(rotation[2], i);
                float sca_x = getValueForPosition(scale[0], i);
                float sca_y = getValueForPosition(scale[1], i);
                float sca_z = getValueForPosition(scale[2], i);
                
                //Translate
                if(!normalizedPosition){
                    matrices[i] = glm::translate(matrices[i], glm::vec3(pos_x, pos_y, pos_z));
                }else{
                    matrices[i] = glm::translate(matrices[i], glm::vec3(pos_x * width, pos_y * height, pos_z));
                }
                
                //Scale and rotate
                matrices[i] = glm::rotate(matrices[i], rot_x * (float)PI * 2, glm::vec3(1, 0, 0));
                matrices[i] = glm::rotate(matrices[i], rot_y * (float)PI * 2, glm::vec3(0, 1, 0));
                matrices[i] = glm::rotate(matrices[i], rot_z * (float)PI * 2, glm::vec3(0, 0, 1));
                matrices[i] = glm::scale(matrices[i], glm::vec3(sca_x, sca_y, 1));
                
                //Translate to anchor
                if(!normalizedAnchor){
                    matrices[i] = glm::translate(matrices[i], glm::vec3(-anch_x, -anch_y, -anch_z));
                }else{
                    matrices[i] = glm::translate(matrices[i], glm::vec3(-anch_x * tex->getWidth(), -anch_y * tex->getHeight(), -anch_z));
                }
                
                //Apply input transformation
                if(transformInput->size() == input->size())
                    matrices[i] = matrices[i] * transformInput->at(i);
                
            }
        }
        transformOutput = matrices;
        output = input;
    }
}
