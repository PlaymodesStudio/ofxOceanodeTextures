//
//  textureBlender.cpp
//  ofxOceanodeTextures
//
//  Created by Eduard Frigola on 22/12/23.
//

#include "textureBlender.h"

textureBlender::textureBlender() : ofxOceanodeNodeModel("Texture Blender"){
    
}

void textureBlender::setup(){
    addParameter(width.set("Width", 100, 1, INT_MAX));
    addParameter(height.set("Height", 100, 1, INT_MAX));
    
    addParameter(input.set("Input", {nullptr}));
    addParameter(transformInput.set("T. In", {glm::identity<glm::mat4>()}));
    
    std::vector<string> blendFunctions = {"GL_ZERO", "GL_ONE", "GL_SRC_COLOR", "GL_ONE_MINUS_SRC_COLOR", "GL_DST_COLOR", "GL_ONE_MINUS_DST_COLOR", "GL_SRC_ALPHA", "GL_ONE_MINUS_SRC_ALPHA", "GL_DST_ALPHA", "GL_ONE_MINUS_DST_ALPHA", "GL_CONSTANT_COLOR", "GL_ONE_MINUS_CONSTANT_COLOR", "GL_CONSTANT_ALPHA", "GL_ONE_MINUS_CONSTANT_ALPHA", "GL_SRC_ALPHA_SATURATE", "GL_SRC1_COLOR", "GL_ONE_MINUS_SRC1_COLOR", "GL_SRC1_ALPHA", "GL_ONE_MINUS_SRC1_ALPHA"};
    
    std::vector<string> blendEquations =  {"GL_FUNC_ADD", "GL_FUNC_SUBTRACT", "GL_FUNC_REVERSE_SUBTRACT", "GL_MIN", "GL_MAX"};
    
    addParameterDropdown(blendSrcColorFunction, "Src Color", 0, blendFunctions);
    addParameterDropdown(blendSrcAlphaFunction, "Src Alpha", 0, blendFunctions);
    addParameterDropdown(blendDstColorFunction, "Dst Color", 0, blendFunctions);
    addParameterDropdown(blendDstAlphaFunction, "Dst Alpha", 0, blendFunctions);
    addParameterDropdown(blendColorEquation, "Color Eq", 0, blendEquations);
    addParameterDropdown(blendAlphaEquation, "Alpha Eq", 0, blendEquations);
    
    addParameter(opacity.set("Opacity", {1}, {0}, {1}));
    addParameter(alpha.set("Alpha", {1}, {0}, {1}));
    
    addParameter(output.set("Ouput", nullptr));
    
    listener = input.newListener([this](std::vector<ofTexture*> &vec){
        if(vec.size() > 0 && vec[0] != nullptr){
            if(!fbo.isAllocated() || fbo.getWidth() != width || fbo.getHeight() != height){
                fbo.allocate(width, height);
            }
            
            fbo.begin();
            ofClear(0, 0, 0, 255);
            glEnable(GL_BLEND);
           
            auto getGLenumFromFunctionInt = [](int val)->GLenum{
                switch(val){
                    case 0: return GL_ZERO;
                    case 1: return GL_ONE;
                    case 2: return GL_SRC_COLOR;
                    case 3: return GL_ONE_MINUS_SRC_COLOR;
                    case 4: return GL_DST_COLOR;
                    case 5: return GL_ONE_MINUS_DST_COLOR;
                    case 6: return GL_SRC_ALPHA;
                    case 7: return GL_ONE_MINUS_SRC_ALPHA;
                    case 8: return GL_DST_ALPHA;
                    case 9: return GL_ONE_MINUS_DST_ALPHA;
                    case 10: return GL_CONSTANT_COLOR;
                    case 11: return GL_ONE_MINUS_CONSTANT_COLOR;
                    case 12: return GL_CONSTANT_ALPHA;
                    case 13: return GL_ONE_MINUS_CONSTANT_ALPHA;
                    case 14: return GL_SRC_ALPHA_SATURATE;
                    case 15: return GL_SRC1_COLOR;
                    case 16: return GL_ONE_MINUS_SRC1_COLOR;
                    case 17: return GL_SRC1_ALPHA;
                    case 18: return GL_ONE_MINUS_SRC1_ALPHA;
                    default: return GL_INVALID_ENUM;
                }
            };
            
            auto getGLenumFromEquationInt = [](int val)->GLenum{
                switch(val){
                    case 0: return GL_FUNC_ADD;
                    case 1: return GL_FUNC_SUBTRACT;
                    case 2: return GL_FUNC_REVERSE_SUBTRACT;
                    case 3: return GL_MIN;
                    case 4: return GL_MAX;
                    default: return GL_INVALID_ENUM;
                }
            };
            
            for(int i = 0; i < vec.size(); i++){
                glBlendFuncSeparate(getGLenumFromFunctionInt(blendSrcColorFunction), getGLenumFromFunctionInt(blendDstColorFunction),  getGLenumFromFunctionInt(blendSrcAlphaFunction), getGLenumFromFunctionInt(blendDstAlphaFunction));
                glBlendEquationSeparate(getGLenumFromEquationInt(blendColorEquation), getGLenumFromEquationInt(blendAlphaEquation));
                float _opacity = opacity->at(0);
                if(opacity->size() == vec.size()) _opacity = opacity->at(i);
                float _alpha = alpha->at(0);
                if(alpha->size() == vec.size()) _alpha = alpha->at(i);
                ofSetColor(_opacity * 255.0, _opacity * 255.0, _opacity * 255.0, _alpha * 255.0);
                
                ofPushMatrix();
                if(transformInput->size() == vec.size())
                    ofMultMatrix(transformInput->at(i));
                
                vec[i]->draw(0, 0);
                ofPopMatrix();
            }
            glDisable(GL_BLEND);
            fbo.end();
            
            output = &fbo.getTexture();
        }
    });
}
