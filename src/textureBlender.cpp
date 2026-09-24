//
//  textureBlender.cpp
//  ofxOceanodeTextures
//
//  Created by Eduard Frigola on 22/12/23.
//

#include "textureBlender.h"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>

namespace{
enum BlendFunctionIndex{
    BlendZero = 0,
    BlendOne,
    BlendSrcColor,
    BlendOneMinusSrcColor,
    BlendDstColor,
    BlendOneMinusDstColor,
    BlendSrcAlpha,
    BlendOneMinusSrcAlpha
};

enum BlendEquationIndex{
    BlendAdd = 0,
    BlendSubtract,
    BlendReverseSubtract,
    BlendMin,
    BlendMax
};

struct BlendModePreset{
    int srcColor;
    int dstColor;
    int srcAlpha;
    int dstAlpha;
    int colorEquation;
    int alphaEquation;
};

const std::array<BlendModePreset, 11> blendModePresets = {{
    {BlendOne, BlendOne, BlendOne, BlendOne, BlendMax, BlendMax},
    {BlendSrcAlpha, BlendOneMinusSrcAlpha, BlendOne, BlendOneMinusSrcAlpha, BlendAdd, BlendAdd},
    {BlendOne, BlendOneMinusSrcAlpha, BlendOne, BlendOneMinusSrcAlpha, BlendAdd, BlendAdd},
    {BlendOne, BlendZero, BlendOne, BlendZero, BlendAdd, BlendAdd},
    {BlendOne, BlendOne, BlendOne, BlendOne, BlendAdd, BlendAdd},
    {BlendSrcAlpha, BlendOne, BlendOne, BlendOneMinusSrcAlpha, BlendAdd, BlendAdd},
    {BlendOneMinusDstColor, BlendOne, BlendOne, BlendOneMinusSrcAlpha, BlendAdd, BlendAdd},
    {BlendDstColor, BlendZero, BlendOne, BlendOneMinusSrcAlpha, BlendAdd, BlendAdd},
    {BlendOne, BlendOne, BlendOne, BlendOne, BlendMin, BlendMax},
    {BlendOne, BlendOne, BlendOne, BlendOne, BlendSubtract, BlendMax},
    {BlendOne, BlendOne, BlendOne, BlendOne, BlendReverseSubtract, BlendMax}
}};

const std::vector<std::string> blendModeNames = {
    "Lighten / Max",
    "Normal / Alpha Over",
    "Normal / Premultiplied",
    "Replace",
    "Add",
    "Additive / Alpha",
    "Screen",
    "Multiply",
    "Darken / Min",
    "Subtract: Source - Result",
    "Subtract: Result - Source",
    "Custom"
};

constexpr int customBlendMode = 11;
}

textureBlender::textureBlender() : ofxOceanodeNodeModel("Texture Blender"){
    
}

void textureBlender::setup()
{
    addParameter(active.set("Active", true));
    addParameter(width.set("Width", 100, 1, INT_MAX));
    addParameter(height.set("Height", 100, 1, INT_MAX));
    addParameter(input.set("Input", {nullptr}));
    addParameter(transformInput.set("T. In", {glm::identity<glm::mat4>()}));
	addParameterDropdown(blendMode, "Blend Mode", 0, blendModeNames);
	addParameterDropdown(layerOrder, "Layer Order", 0, {"Ascending", "Descending"});

    std::vector<string> blendSourceFunctions = {"GL_ZERO", "GL_ONE", "GL_SRC_COLOR", "GL_ONE_MINUS_SRC_COLOR", "GL_DST_COLOR", "GL_ONE_MINUS_DST_COLOR", "GL_SRC_ALPHA", "GL_ONE_MINUS_SRC_ALPHA", "GL_DST_ALPHA", "GL_ONE_MINUS_DST_ALPHA", "GL_CONSTANT_COLOR", "GL_ONE_MINUS_CONSTANT_COLOR", "GL_CONSTANT_ALPHA", "GL_ONE_MINUS_CONSTANT_ALPHA", "GL_SRC_ALPHA_SATURATE"};
    std::vector<string> blendDestinationFunctions = {"GL_ZERO", "GL_ONE", "GL_SRC_COLOR", "GL_ONE_MINUS_SRC_COLOR", "GL_DST_COLOR", "GL_ONE_MINUS_DST_COLOR", "GL_SRC_ALPHA", "GL_ONE_MINUS_SRC_ALPHA", "GL_DST_ALPHA", "GL_ONE_MINUS_DST_ALPHA", "GL_CONSTANT_COLOR", "GL_ONE_MINUS_CONSTANT_COLOR", "GL_CONSTANT_ALPHA", "GL_ONE_MINUS_CONSTANT_ALPHA"};
    
    std::vector<string> blendEquations =  {"GL_FUNC_ADD", "GL_FUNC_SUBTRACT", "GL_FUNC_REVERSE_SUBTRACT", "GL_MIN", "GL_MAX"};
    
	addSeparator("Colors/Alpha",ofColor(0,255,255));
    addParameterDropdown(blendSrcColorFunction, "Src Color", 1, blendSourceFunctions);
    addParameterDropdown(blendSrcAlphaFunction, "Src Alpha", 1, blendSourceFunctions);
    addParameterDropdown(blendDstColorFunction, "Dst Color", 1, blendDestinationFunctions);
    addParameterDropdown(blendDstAlphaFunction, "Dst Alpha", 1, blendDestinationFunctions);
	addSeparator("Equations",ofColor(255,128,0));
    addParameterDropdown(blendColorEquation, "Color Eq", 4, blendEquations);
    addParameterDropdown(blendAlphaEquation, "Alpha Eq", 4, blendEquations);
    addParameter(blendColor.set("Blend Color", ofFloatColor(0.0f, 0.0f, 0.0f, 0.0f)));
	addSeparator("Opacity/Alpha",ofColor(255,255,0));
    addParameter(opacity.set("Opacity", {1}, {0}, {1}));
    addParameter(alpha.set("Alpha", {1}, {0}, {1}));
	addSeparator("Camera",ofColor(0,128,255));
    addParameterDropdown(cameraProjection, "Projection", 0, {"Perspective", "Orthographic"});
    addParameter(cameraFov.set("Field of View", 60.0f, 1.0f, 179.0f));
    addParameter(cameraAutoDistance.set("Auto Distance", true));
    addParameter(cameraDistance.set("Camera Distance", 1000.0f, 0.001f, FLT_MAX));
    addParameter(cameraNearClip.set("Near Clip (0 = Auto)", 0.0f, 0.0f, FLT_MAX));
    addParameter(cameraFarClip.set("Far Clip (0 = Auto)", 0.0f, 0.0f, FLT_MAX));
	addSeparator("Output",ofColor(128,128,128));
    addOutputParameter(output.set("Ouput", nullptr));

    cameraListeners.push(cameraProjection.newListener([this](int &){ render(); }));
    cameraListeners.push(cameraAutoDistance.newListener([this](bool &){ render(); }));
    auto cameraChanged = [this](float &){ render(); };
    cameraListeners.push(cameraFov.newListener(cameraChanged));
    cameraListeners.push(cameraDistance.newListener(cameraChanged));
    cameraListeners.push(cameraNearClip.newListener(cameraChanged));
    cameraListeners.push(cameraFarClip.newListener(cameraChanged));

    blendModeListeners.push(blendMode.newListener([this](int &mode){
        if(!updatingBlendMode && !loadingPreset){
            applyBlendMode(mode);
        }
    }));

    auto blendParameterChanged = [this](int &){
        updateBlendModeFromParameters();
    };
    blendModeListeners.push(blendSrcColorFunction.newListener(blendParameterChanged));
    blendModeListeners.push(blendSrcAlphaFunction.newListener(blendParameterChanged));
    blendModeListeners.push(blendDstColorFunction.newListener(blendParameterChanged));
    blendModeListeners.push(blendDstAlphaFunction.newListener(blendParameterChanged));
    blendModeListeners.push(blendColorEquation.newListener(blendParameterChanged));
    blendModeListeners.push(blendAlphaEquation.newListener(blendParameterChanged));
    
    listener = input.newListener([this](std::vector<ofTexture*> &){
        render();
    });
}

void textureBlender::render()
{
    if(loadingPreset) return;
    const auto &vec = input.get();
    if(active)
    {
        bool hasValidTexture = false;
        for(auto *texture : vec){
            if(texture != nullptr && texture->isAllocated()){
                hasValidTexture = true;
                break;
            }
        }

        if(hasValidTexture){
            if(!fbo.isAllocated() || fbo.getWidth() != width || fbo.getHeight() != height){
                fbo.allocate(width, height, GL_RGBA32F);
            }

            fbo.begin();
            configureCamera();
            camera.begin(ofRectangle(0, 0, fbo.getWidth(), fbo.getHeight()));
            ofClear(0, 0, 0, 255);
            glBlendColor(blendColor->r, blendColor->g, blendColor->b, blendColor->a);

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

            glBlendFuncSeparate(getGLenumFromFunctionInt(blendSrcColorFunction), getGLenumFromFunctionInt(blendDstColorFunction),  getGLenumFromFunctionInt(blendSrcAlphaFunction), getGLenumFromFunctionInt(blendDstAlphaFunction));
            glBlendEquationSeparate(getGLenumFromEquationInt(blendColorEquation), getGLenumFromEquationInt(blendAlphaEquation));

            bool hasBaseLayer = false;
            for(std::size_t offset = 0; offset < vec.size(); offset++){
                std::size_t i = layerOrder == 0 ? offset : vec.size() - 1 - offset;
                if(vec[i] == nullptr || !vec[i]->isAllocated()) continue;

                if(hasBaseLayer){
                    glEnable(GL_BLEND);
                }else{
                    glDisable(GL_BLEND);
                }
                
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
                
                hasBaseLayer = true;
            }
            glDisable(GL_BLEND);
            camera.end();
            fbo.end();

            output = &fbo.getTexture();
        }

    }
}

void textureBlender::configureCamera()
{
    // Match the default FBO view: centred on Z = 0 with pixel-sized coordinates.
    const float fov = ofClamp(cameraFov.get(), 1.0f, 179.0f);
    const float fittedDistance = (fbo.getHeight() * 0.5f) / std::tan(glm::radians(fov * 0.5f));
    // Keep extreme user-entered distances finite during projection calculations.
    const float maxDistance = 1.0e12f;
    const float distance = cameraAutoDistance ? fittedDistance : ofClamp(cameraDistance.get(), 0.001f, maxDistance);
    const float nearClip = cameraNearClip > 0.0f
        ? ofClamp(cameraNearClip.get(), 0.000001f, maxDistance) : distance / 10.0f;
    const float requestedFarClip = cameraFarClip > 0.0f
        ? ofClamp(cameraFarClip.get(), 0.000001f, maxDistance) : distance * 10.0f;
    // Equal or reversed clip planes would make the projection invalid.
    const float farClip = std::max(requestedFarClip, nearClip + std::max(0.001f, nearClip * 0.001f));

    camera.setFov(fov);
    camera.setNearClip(nearClip);
    camera.setFarClip(farClip);
    camera.setForceAspectRatio(false);
    // Preserve the orientation already established by fbo.begin().
    camera.setVFlip(ofGetCurrentRenderer()->isVFlipped());
    camera.setPosition(fbo.getWidth() * 0.5f, fbo.getHeight() * 0.5f, distance);
    camera.lookAt(glm::vec3(fbo.getWidth() * 0.5f, fbo.getHeight() * 0.5f, 0.0f), glm::vec3(0, 1, 0));
    if(cameraProjection == 1){
        camera.enableOrtho();
    }else{
        camera.disableOrtho();
    }
}

void textureBlender::applyBlendMode(int mode)
{
    if(mode < 0 || mode >= customBlendMode) return;

    const auto &preset = blendModePresets[mode];
    updatingBlendMode = true;
    blendSrcColorFunction = preset.srcColor;
    blendDstColorFunction = preset.dstColor;
    blendSrcAlphaFunction = preset.srcAlpha;
    blendDstAlphaFunction = preset.dstAlpha;
    blendColorEquation = preset.colorEquation;
    blendAlphaEquation = preset.alphaEquation;
    updatingBlendMode = false;
}

void textureBlender::updateBlendModeFromParameters()
{
    if(updatingBlendMode || loadingPreset) return;

    int matchingMode = customBlendMode;
    for(std::size_t i = 0; i < blendModePresets.size(); i++){
        const auto &preset = blendModePresets[i];
        if(blendSrcColorFunction == preset.srcColor &&
           blendDstColorFunction == preset.dstColor &&
           blendSrcAlphaFunction == preset.srcAlpha &&
           blendDstAlphaFunction == preset.dstAlpha &&
           blendColorEquation == preset.colorEquation &&
           blendAlphaEquation == preset.alphaEquation){
            matchingMode = static_cast<int>(i);
            break;
        }
    }

    updatingBlendMode = true;
    blendMode = matchingMode;
    updatingBlendMode = false;
}
