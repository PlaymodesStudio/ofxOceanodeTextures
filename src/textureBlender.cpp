//
//  textureBlender.cpp
//  ofxOceanodeTextures
//
//  Created by Eduard Frigola on 22/12/23.
//

#include "textureBlender.h"
#include "textureComposer.h"
#include "ofxOceanodeConnection.h"
#include "imgui.h"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <unordered_set>

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

GLenum blendFunction(int value){
    const std::array<GLenum, 15> functions = {{
        GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR,
        GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
        GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_COLOR,
        GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA, GL_SRC_ALPHA_SATURATE
    }};
    return value >= 0 && value < functions.size() ? functions[value] : GL_INVALID_ENUM;
}

GLenum blendEquation(int value){
    const std::array<GLenum, 5> equations = {{GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MIN, GL_MAX}};
    return value >= 0 && value < equations.size() ? equations[value] : GL_INVALID_ENUM;
}

// The preview temporarily changes raw GL state inside the host's ImGui frame.
// Keep stencil, blend constants and write masks intact as well as test flags.
struct PreviewGLState{
    GLboolean depth = glIsEnabled(GL_DEPTH_TEST), cull = glIsEnabled(GL_CULL_FACE);
    GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST), blend = glIsEnabled(GL_BLEND);
    GLboolean stencil = glIsEnabled(GL_STENCIL_TEST), colorMask[4];
    GLint srcRgb, dstRgb, srcAlpha, dstAlpha, equationRgb, equationAlpha, clearStencil;
    GLfloat blendColor[4];
    struct StencilFace{
        GLint function, reference, valueMask, writeMask, fail, depthFail, pass;
    } front, back;

    PreviewGLState(){
        glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
        glGetFloatv(GL_BLEND_COLOR, blendColor);
        glGetIntegerv(GL_BLEND_SRC_RGB, &srcRgb);
        glGetIntegerv(GL_BLEND_DST_RGB, &dstRgb);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);
        glGetIntegerv(GL_BLEND_EQUATION_RGB, &equationRgb);
        glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &equationAlpha);
        glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &clearStencil);
        glGetIntegerv(GL_STENCIL_FUNC, &front.function);
        glGetIntegerv(GL_STENCIL_REF, &front.reference);
        glGetIntegerv(GL_STENCIL_VALUE_MASK, &front.valueMask);
        glGetIntegerv(GL_STENCIL_WRITEMASK, &front.writeMask);
        glGetIntegerv(GL_STENCIL_FAIL, &front.fail);
        glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &front.depthFail);
        glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &front.pass);
        glGetIntegerv(GL_STENCIL_BACK_FUNC, &back.function);
        glGetIntegerv(GL_STENCIL_BACK_REF, &back.reference);
        glGetIntegerv(GL_STENCIL_BACK_VALUE_MASK, &back.valueMask);
        glGetIntegerv(GL_STENCIL_BACK_WRITEMASK, &back.writeMask);
        glGetIntegerv(GL_STENCIL_BACK_FAIL, &back.fail);
        glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_FAIL, &back.depthFail);
        glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_PASS, &back.pass);
    }

    ~PreviewGLState(){
        auto restoreTest = [](GLenum test, GLboolean enabled){
            if(enabled) glEnable(test);
            else glDisable(test);
        };
        restoreTest(GL_DEPTH_TEST, depth);
        restoreTest(GL_CULL_FACE, cull);
        restoreTest(GL_SCISSOR_TEST, scissor);
        restoreTest(GL_BLEND, blend);
        restoreTest(GL_STENCIL_TEST, stencil);
        glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
        glBlendColor(blendColor[0], blendColor[1], blendColor[2], blendColor[3]);
        glBlendFuncSeparate(srcRgb, dstRgb, srcAlpha, dstAlpha);
        glBlendEquationSeparate(equationRgb, equationAlpha);
        glClearStencil(clearStencil);
        auto restoreFace = [](GLenum face, const StencilFace &state){
            glStencilFuncSeparate(face, state.function, state.reference, state.valueMask);
            glStencilMaskSeparate(face, state.writeMask);
            glStencilOpSeparate(face, state.fail, state.depthFail, state.pass);
        };
        restoreFace(GL_FRONT, front);
        restoreFace(GL_BACK, back);
    }
};

void drawPreviewFullscreenQuad(bool textured){
    ofPushView();
    ofGetCurrentRenderer()->setOrientation(OF_ORIENTATION_DEFAULT, false);
    ofSetMatrixMode(OF_MATRIX_PROJECTION);
    ofLoadIdentityMatrix();
    ofSetMatrixMode(OF_MATRIX_MODELVIEW);
    ofLoadIdentityMatrix();
    ofMesh quad;
    quad.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
    quad.addVertex(glm::vec3(-1, -1, 0));
    quad.addVertex(glm::vec3(1, -1, 0));
    quad.addVertex(glm::vec3(1, 1, 0));
    quad.addVertex(glm::vec3(-1, 1, 0));
    if(textured){
        quad.addTexCoord(glm::vec2(0, 0));
        quad.addTexCoord(glm::vec2(1, 0));
        quad.addTexCoord(glm::vec2(1, 1));
        quad.addTexCoord(glm::vec2(0, 1));
    }
    quad.draw();
    ofPopView();
}

bool finitePoint(const glm::vec4 &point){
    return std::isfinite(point.x) && std::isfinite(point.y)
        && std::isfinite(point.z) && std::isfinite(point.w);
}

// Clip in homogeneous space before dividing by W, including lines crossing
// the eye/near plane. ImGui's rectangle clip alone cannot handle those lines.
bool clipPreviewLine(glm::vec4 &a, glm::vec4 &b){
    if(!finitePoint(a) || !finitePoint(b)) return false;
    const std::array<glm::vec4, 6> planes = {{
        {1, 0, 0, 1}, {-1, 0, 0, 1}, {0, 1, 0, 1},
        {0, -1, 0, 1}, {0, 0, 1, 1}, {0, 0, -1, 1}
    }};
    for(const auto &plane : planes){
        const float da = glm::dot(a, plane);
        const float db = glm::dot(b, plane);
        if(da < 0.0f && db < 0.0f) return false;
        if(da < 0.0f || db < 0.0f){
            const glm::vec4 intersection = a + (b - a) * (da / (da - db));
            if(da < 0.0f) a = intersection;
            else b = intersection;
        }
    }
    return a.w > 0.0f && b.w > 0.0f;
}

ImVec2 previewScreenPoint(const glm::vec4 &point, const ofRectangle &viewport){
    return ImVec2(viewport.x + (point.x / point.w + 1.0f) * viewport.width * 0.5f,
                  viewport.y + (1.0f - point.y / point.w) * viewport.height * 0.5f);
}
}

textureBlender::textureBlender() : ofxOceanodeNodeModel("Texture Blender"){
    
}

void textureBlender::setup()
{
    description = "Composites textures using the matrices in T. In.\n"
        "Show opens a 3D preview with an independent navigation camera and optional texture display.";
    addParameter(active.set("Active", true));
    addParameter(show.set("Show", false));
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
            configureBlending();

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
    // Use the requested size so the preview also works before an FBO exists.
    const float viewWidth = width.get();
    const float viewHeight = height.get();
    const float fov = ofClamp(cameraFov.get(), 1.0f, 179.0f);
    const float fittedDistance = (viewHeight * 0.5f) / std::tan(glm::radians(fov * 0.5f));
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
    camera.setPosition(viewWidth * 0.5f, viewHeight * 0.5f, distance);
    camera.lookAt(glm::vec3(viewWidth * 0.5f, viewHeight * 0.5f, 0.0f), glm::vec3(0, 1, 0));
    if(cameraProjection == 1){
        camera.enableOrtho();
    }else{
        camera.disableOrtho();
    }
}

textureComposer *textureBlender::getPreviewComposer()
{
    ofxOceanodeAbstractParameter *parameter = &getOceanodeParameter(transformInput);
    std::unordered_set<ofxOceanodeAbstractParameter *> visited;
    while(parameter != nullptr && visited.insert(parameter).second){
        if(auto *composer = dynamic_cast<textureComposer *>(parameter->getNodeModel())){
            const auto &source = parameter->cast<std::vector<glm::mat4>>().getParameter();
            const auto &output = composer->getTransformOutput();
            // Do not use an unrelated composer parameter or a stale value from
            // a disabled connection. Routers are followed through their input.
            if(source.isReferenceTo(output)){
                return transformInput.get() == output.get() ? composer : nullptr;
            }
        }
        auto *connection = parameter->getInConnection();
        if(connection == nullptr) break;
        auto &source = connection->getSourceParameter();
        if(source.valueType() != transformInput.valueType()) break;
        parameter = &source;
    }
    return nullptr;
}

std::vector<textureBlender::PreviewLayer> textureBlender::getPreviewLayers()
{
    std::vector<PreviewLayer> layers;
    const auto &textures = input.get();
    const auto &transforms = transformInput.get();
    auto *composer = transforms.size() == textures.size() ? getPreviewComposer() : nullptr;
    layers.reserve(textures.size());
    for(std::size_t i = 0; i < textures.size(); ++i){
        const auto *texture = textures[i];
        if(texture == nullptr || !texture->isAllocated()) continue;

        // Retain native texture UVs and anchors, using the preview camera's
        // Y-up orientation rather than the output camera's screen-space flip.
        const auto mesh = texture->getMeshForSubsection(0, 0, 0,
            texture->getWidth(), texture->getHeight(), 0, 0,
            texture->getWidth(), texture->getHeight(), previewCamera.isVFlipped(), ofGetRectMode());
        if(mesh.getNumVertices() != 4) continue;
        const glm::mat4 transform = transforms.size() == textures.size()
            ? transforms[i] : glm::mat4(1.0f);
        PreviewLayer layer;
        layer.index = i;
        layer.origin = transform * glm::vec4(0, 0, 0, 1);
        layer.hasComposerPivot = composer != nullptr && composer->getPreviewPivot(i, layer.composerPivot)
            && finitePoint(glm::vec4(layer.composerPivot, 1.0f));
        bool valid = true;
        for(std::size_t corner = 0; corner < 4; ++corner){
            const glm::vec4 point = transform * glm::vec4(mesh.getVertex(corner), 1.0f);
            if(!finitePoint(point) || std::abs(point.w) < 1.0e-6f){
                valid = false;
                break;
            }
            layer.corners[corner] = glm::vec3(point) / point.w;
            layer.texCoords[corner] = mesh.getTexCoord(corner);
            if(!finitePoint(glm::vec4(layer.corners[corner], 1.0f))){
                valid = false;
                break;
            }
        }
        if(valid) layers.push_back(layer);
    }
    return layers;
}

std::array<glm::vec3, 4> textureBlender::getCameraPreviewCorners(float distance) const
{
    const float halfHeight = camera.getOrtho() ? height.get() * 0.5f
        : std::tan(glm::radians(camera.getFov() * 0.5f)) * distance;
    const float halfWidth = halfHeight * (static_cast<float>(width.get()) / height.get());
    std::array<glm::vec3, 4> corners = {{
        {-halfWidth, -halfHeight, -distance}, {halfWidth, -halfHeight, -distance},
        {halfWidth, halfHeight, -distance}, {-halfWidth, halfHeight, -distance}
    }};
    for(auto &corner : corners) corner = glm::vec3(camera.getGlobalTransformMatrix() * glm::vec4(corner, 1.0f));
    return corners;
}

float textureBlender::getPreviewLayerAlpha(const PreviewLayer &layer) const
{
    const glm::mat4 view = camera.getModelViewMatrix();
    for(const auto &corner : layer.corners){
        const float depth = -(view * glm::vec4(corner, 1.0f)).z;
        if(depth < camera.getNearClip() || depth > camera.getFarClip()) return 0.25f;
    }
    return 0.75f;
}

void textureBlender::updatePreviewGridStep(float viewportHeight)
{
    if(previewGridStep <= 0.0f){
        previewGridStep = std::pow(10.0f, std::floor(std::log10(previewDistance * 0.08f)));
    }
    const float unitsPerPixel = 2.0f * previewDistance * std::tan(glm::radians(22.5f)) / viewportHeight;
    // Hold the current spacing over a wide zoom range, then change by only 2x.
    // Different thresholds in each direction prevent flicker around a boundary.
    while(previewGridStep / unitsPerPixel > 180.0f) previewGridStep *= 0.5f;
    while(previewGridStep / unitsPerPixel < 30.0f) previewGridStep *= 2.0f;
}

void textureBlender::framePreview(const std::vector<PreviewLayer> &layers, float aspectRatio)
{
    glm::vec3 minimum(0.0f);
    glm::vec3 maximum(width.get(), height.get(), 0.0f);
    auto include = [&](const glm::vec3 &point){
        minimum = glm::min(minimum, point);
        maximum = glm::max(maximum, point);
    };
    include(camera.getGlobalPosition());
    const float imageDistance = ofClamp(camera.getGlobalPosition().z, camera.getNearClip(), camera.getFarClip());
    for(const auto &corner : getCameraPreviewCorners(imageDistance)) include(corner);
    for(const auto &corner : getCameraPreviewCorners(camera.getNearClip())) include(corner);
    if(previewClipPlanes){
        for(const auto &corner : getCameraPreviewCorners(camera.getFarClip())) include(corner);
    }
    for(const auto &layer : layers){
        for(const auto &corner : layer.corners) include(corner);
        if(finitePoint(layer.origin) && std::abs(layer.origin.w) >= 1.0e-6f){
            include(glm::vec3(layer.origin) / layer.origin.w);
        }
        if(layer.hasComposerPivot) include(layer.composerPivot);
    }
    previewTarget = (minimum + maximum) * 0.5f;
    previewSceneRadius = std::max(1.0f, glm::length(maximum - minimum) * 0.5f);
    const float verticalHalfFov = glm::radians(45.0f * 0.5f);
    const float horizontalHalfFov = std::atan(std::tan(verticalHalfFov) * aspectRatio);
    previewDistance = previewSceneRadius * 1.15f / std::sin(std::min(verticalHalfFov, horizontalHalfFov));
    previewInitialized = true;
}

void textureBlender::draw(ofEventArgs &)
{
    if(!show.get()){
        previewFbo.clear();
        previewLayersFbo.clear();
        return;
    }

    bool open = true;
    // Canvas IDs distinguish nodes with the same identifier in nested macros.
    const std::string title = "Texture Blender " + ofToString(getNumIdentifier())
        + " - 3D Preview###TextureBlenderPreview_" + canvasID + "_" + ofToString(getNumIdentifier());
    ImGui::SetNextWindowSize(ImVec2(760, 540), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(360, 260), ImVec2(FLT_MAX, FLT_MAX));
    if(ImGui::Begin(title.c_str(), &open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)){
        configureCamera();
        const auto layers = getPreviewLayers();
        const bool frameAll = ImGui::Button("Frame All");
        ImGui::SameLine();
        const bool resetView = ImGui::Button("Reset View");
        ImGui::SameLine();
        const bool clipPlanesChanged = ImGui::Checkbox("Clip planes", &previewClipPlanes);
        ImGui::Checkbox("Show textures", &previewShowTextures);
        if(previewShowTextures){
            ImGui::SameLine();
            const float sliderSpace = ImGui::GetContentRegionAvail().x
                - ImGui::CalcTextSize("Texture opacity").x - ImGui::GetStyle().ItemInnerSpacing.x;
            ImGui::SetNextItemWidth(std::max(60.0f, std::min(160.0f, sliderSpace)));
            ImGui::SliderFloat("Texture opacity", &previewTextureOpacity, 0.0f, 1.0f, "%.2f");
        }else{
            previewFbo.clear();
            previewLayersFbo.clear();
        }
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextWrapped("Drag: orbit | Shift+drag / right / middle: pan | Wheel: zoom");
        ImGui::PopStyleColor();
        if(transformInput->size() != input->size()){
            ImGui::TextWrapped("T. In / Input count mismatch: transforms are ignored, as in the output.");
        }

        const ImVec2 size = ImGui::GetContentRegionAvail();
        if(size.x > 1.0f && size.y > 1.0f){
            if(resetView){
                previewYaw = 0.65f;
                previewPitch = 0.35f;
            }
            if(!previewInitialized || frameAll || resetView || clipPlanesChanged) framePreview(layers, size.x / size.y);

            const ImVec2 position = ImGui::GetCursorScreenPos();
            ImGui::InvisibleButton("##PreviewViewport", size, ImGuiButtonFlags_MouseButtonLeft
                | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
            const auto &io = ImGui::GetIO();
            if(ImGui::IsItemActive()){
                const bool pan = io.KeyShift || ImGui::IsMouseDown(ImGuiMouseButton_Right)
                    || ImGui::IsMouseDown(ImGuiMouseButton_Middle);
                if(pan){
                    const glm::vec3 right(std::cos(previewYaw), 0.0f, -std::sin(previewYaw));
                    const glm::vec3 up(-std::sin(previewYaw) * std::sin(previewPitch),
                        std::cos(previewPitch), -std::cos(previewYaw) * std::sin(previewPitch));
                    const float unitsPerPixel = 2.0f * previewDistance * std::tan(glm::radians(22.5f)) / size.y;
                    previewTarget += (-right * io.MouseDelta.x + up * io.MouseDelta.y) * unitsPerPixel;
                }else if(ImGui::IsMouseDown(ImGuiMouseButton_Left)){
                    previewYaw = std::remainder(previewYaw - io.MouseDelta.x * 0.008f, glm::two_pi<float>());
                    previewPitch = ofClamp(previewPitch + io.MouseDelta.y * 0.008f, -1.5f, 1.5f);
                }
            }
            if(ImGui::IsItemHovered() && io.MouseWheel != 0.0f){
                previewDistance = ofClamp(previewDistance * std::exp(-io.MouseWheel * 0.15f), 0.01f, 1.0e12f);
            }
            drawPreviewScene(layers, ofRectangle(position.x, position.y, size.x, size.y));
        }
    }
    ImGui::End();
    if(!open) show = false;
}

void textureBlender::drawPreviewScene(const std::vector<PreviewLayer> &layers, const ofRectangle &viewport)
{
    const glm::vec3 offset(std::sin(previewYaw) * std::cos(previewPitch),
        std::sin(previewPitch), std::cos(previewYaw) * std::cos(previewPitch));
    previewCamera.setPosition(previewTarget + offset * previewDistance);
    previewCamera.lookAt(previewTarget, glm::vec3(0, 1, 0));
    previewCamera.setFov(45.0f);
    previewCamera.setNearClip(std::max(0.00001f, previewDistance * 0.0001f));
    previewCamera.setFarClip(std::max(previewDistance * 100.0f, previewSceneRadius * 100.0f));
    const glm::mat4 projection = previewCamera.getModelViewProjectionMatrix(viewport);
    auto *drawList = ImGui::GetWindowDrawList();
    const ImVec2 topLeft(viewport.x, viewport.y);
    const ImVec2 bottomRight(viewport.getRight(), viewport.getBottom());
    drawList->PushClipRect(topLeft, bottomRight, true);
    drawList->AddRectFilled(topLeft, bottomRight, IM_COL32(22, 25, 30, 255));
    auto line = [&](const glm::vec3 &a, const glm::vec3 &b, ImU32 color, float thickness = 1.0f){
        glm::vec4 clipA = projection * glm::vec4(a, 1.0f);
        glm::vec4 clipB = projection * glm::vec4(b, 1.0f);
        if(clipPreviewLine(clipA, clipB)){
            drawList->AddLine(previewScreenPoint(clipA, viewport), previewScreenPoint(clipB, viewport), color, thickness);
        }
    };
    auto label = [&](const glm::vec3 &point, ImU32 color, const std::string &text){
        const glm::vec4 clip = projection * glm::vec4(point, 1.0f);
        if(!finitePoint(clip) || clip.w <= 0.0f
            || std::abs(clip.x) > clip.w || std::abs(clip.y) > clip.w || std::abs(clip.z) > clip.w) return;
        ImVec2 screen = previewScreenPoint(clip, viewport);
        screen.x += 5.0f;
        screen.y += 3.0f;
        drawList->AddText(screen, color, text.c_str());
    };
    auto rectangle = [&](const std::array<glm::vec3, 4> &corners, ImU32 color, float thickness){
        for(std::size_t i = 0; i < 4; ++i) line(corners[i], corners[(i + 1) % 4], color, thickness);
    };
    auto marker = [&](const glm::vec4 &point, ImU32 color, float radius){
        const glm::vec4 clip = projection * point;
        if(!finitePoint(clip) || clip.w <= 0.0f
            || std::abs(clip.x) > clip.w || std::abs(clip.y) > clip.w || std::abs(clip.z) > clip.w) return;
        // A screen-sized ring stays legible while orbiting and zooming.
        drawList->AddCircle(previewScreenPoint(clip, viewport), radius, color, 16, 1.5f);
    };

    updatePreviewGridStep(viewport.height);
    const float gridStep = previewGridStep;
    const float gridX = std::floor(previewTarget.x / gridStep) * gridStep;
    const float gridZ = std::floor(previewTarget.z / gridStep) * gridStep;
    // Cover the visible floor independently of the grid spacing, so refining
    // the grid does not suddenly collapse the floor patch around the origin.
    const int gridLines = std::min(240, static_cast<int>(std::ceil(previewDistance * 6.0f / gridStep)));
    const float extent = gridStep * gridLines;
    ofMesh grid;
    grid.setMode(OF_PRIMITIVE_LINES);
    auto gridLine = [&](const glm::vec3 &a, const glm::vec3 &b, ImU32 color){
        if(previewShowTextures){
            const ImVec4 rgba = ImGui::ColorConvertU32ToFloat4(color);
            const ofFloatColor gridColor(rgba.x, rgba.y, rgba.z, rgba.w);
            grid.addVertex(a);
            grid.addVertex(b);
            grid.addColor(gridColor);
            grid.addColor(gridColor);
        }else{
            line(a, b, color);
        }
    };
    for(int i = -gridLines; i <= gridLines; ++i){
        const float x = gridX + i * gridStep;
        const float z = gridZ + i * gridStep;
        const ImU32 xColor = IM_COL32(65, 70, 80, std::fmod(std::abs(x / gridStep), 5.0f) < 0.5f ? 200 : 95);
        const ImU32 zColor = IM_COL32(65, 70, 80, std::fmod(std::abs(z / gridStep), 5.0f) < 0.5f ? 200 : 95);
        gridLine(glm::vec3(x, 0, gridZ - extent), glm::vec3(x, 0, gridZ + extent), xColor);
        gridLine(glm::vec3(gridX - extent, 0, z), glm::vec3(gridX + extent, 0, z), zColor);
    }
    if(previewShowTextures){
        renderPreviewTextures(layers, viewport, grid);
        const ImTextureID textureID = (ImTextureID)(uintptr_t)previewFbo.getTexture().getTextureData().textureID;
        drawList->AddImage(textureID, topLeft, bottomRight, ImVec2(0, 1), ImVec2(1, 0));
    }

    const float axisLength = previewSceneRadius * 0.3f;
    const std::array<ImU32, 3> axisColors = {{IM_COL32(255, 80, 80, 255), IM_COL32(90, 230, 110, 255), IM_COL32(90, 150, 255, 255)}};
    for(int i = 0; i < 3; ++i){
        glm::vec3 end(0.0f);
        end[i] = axisLength;
        line(glm::vec3(0.0f), end, axisColors[i], 2.0f);
        label(end, axisColors[i], std::string(1, "XYZ"[i]));
    }
    label(glm::vec3(0.0f), IM_COL32(190, 195, 205, 255), "0");

    const ImU32 cameraColor = IM_COL32(255, 200, 85, 255);
    const glm::vec3 eye = camera.getGlobalPosition();
    // The usual camera icon reaches the Z=0 image plane. Full near/far clip
    // volumes are optional because the far plane can dwarf the texture scene.
    const float imageDistance = ofClamp(eye.z, camera.getNearClip(), camera.getFarClip());
    const auto imageCorners = getCameraPreviewCorners(imageDistance);
    const auto nearCorners = getCameraPreviewCorners(camera.getNearClip());
    rectangle(imageCorners, cameraColor, 1.5f);
    for(std::size_t i = 0; i < 4; ++i){
        line(camera.getOrtho() ? nearCorners[i] : eye, imageCorners[i], cameraColor);
    }
    if(camera.getOrtho()) rectangle(nearCorners, cameraColor, 1.0f);
    if(previewClipPlanes){
        const auto farCorners = getCameraPreviewCorners(camera.getFarClip());
        const ImU32 clipColor = IM_COL32(255, 200, 85, 110);
        rectangle(nearCorners, clipColor, 1.0f);
        rectangle(farCorners, clipColor, 1.0f);
        for(std::size_t i = 0; i < 4; ++i) line(nearCorners[i], farCorners[i], clipColor);
    }
    std::size_t composerPivots = 0;
    for(const auto &layer : layers){
        const int alpha = static_cast<int>(getPreviewLayerAlpha(layer) * 255.0f + 0.5f);
        const ImU32 color = IM_COL32(0, 255, 255, alpha);
        rectangle(layer.corners, color, 2.0f);
        marker(layer.origin, IM_COL32(255, 80, 80, alpha), 4.0f);
        if(layer.hasComposerPivot){
            marker(glm::vec4(layer.composerPivot, 1.0f), IM_COL32(90, 150, 255, alpha), 6.0f);
            ++composerPivots;
        }
        glm::vec3 center(0.0f);
        for(const auto &corner : layer.corners) center += corner * 0.25f;
        label(center, color, "[" + ofToString(layer.index) + "]");
    }
    drawList->AddText(ImVec2(viewport.x + 10.0f, viewport.y + 10.0f), IM_COL32(175, 185, 200, 255),
        ("XZ floor | Grid: " + ofToString(gridStep) + " px | Textures: " + ofToString(layers.size())).c_str());
    const float legendY = viewport.y + 10.0f + ImGui::GetTextLineHeightWithSpacing();
    drawList->AddText(ImVec2(viewport.x + 10.0f, legendY), IM_COL32(255, 80, 80, 255), "Origin");
    const float legendX = viewport.x + 24.0f + ImGui::CalcTextSize("Origin").x;
    drawList->AddText(ImVec2(legendX, legendY), IM_COL32(90, 150, 255, 255),
        composerPivots > 0 ? "Composer pivot" : "Composer pivot (unavailable)");
    drawList->AddRect(topLeft, bottomRight, IM_COL32(75, 80, 90, 255));
    drawList->PopClipRect();
}

void textureBlender::configureBlending()
{
    glBlendColor(blendColor->r, blendColor->g, blendColor->b, blendColor->a);
    glBlendFuncSeparate(blendFunction(blendSrcColorFunction), blendFunction(blendDstColorFunction),
        blendFunction(blendSrcAlphaFunction), blendFunction(blendDstAlphaFunction));
    glBlendEquationSeparate(blendEquation(blendColorEquation), blendEquation(blendAlphaEquation));
}

void textureBlender::renderPreviewTextures(const std::vector<PreviewLayer> &layers, const ofRectangle &viewport, const ofMesh &grid)
{
    const int previewWidth = std::max(1, static_cast<int>(viewport.width));
    const int previewHeight = std::max(1, static_cast<int>(viewport.height));
    if(!previewFbo.isAllocated() || previewFbo.getWidth() != previewWidth || previewFbo.getHeight() != previewHeight){
        ofFboSettings settings;
        settings.width = previewWidth;
        settings.height = previewHeight;
        settings.internalformat = GL_RGBA;
        settings.textureTarget = GL_TEXTURE_2D;
        settings.useDepth = false;
        settings.useStencil = false;
        settings.numSamples = 0;
        previewFbo.allocate(settings);
        // Blend against the same black, opaque starting color and floating
        // point format as the output, independently of the preview's grid.
        settings.internalformat = GL_RGBA32F;
        settings.useStencil = true;
        previewLayersFbo.allocate(settings);
    }

    const PreviewGLState savedState;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    previewLayersFbo.begin(OF_FBOMODE_NODEFAULTS);
    previewCamera.begin(ofRectangle(0, 0, viewport.width, viewport.height));
    glStencilMask(0xff);
    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT);
    ofClear(0, 0, 0, 255);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    configureBlending();
    bool hasBaseLayer = false;
    for(std::size_t offset = 0; offset < layers.size(); ++offset){
        const auto &layer = layers[layerOrder == 0 ? offset : layers.size() - 1 - offset];
        auto *texture = input->at(layer.index);
        if(hasBaseLayer) glEnable(GL_BLEND);
        else glDisable(GL_BLEND);
        // Apply the node's per-input controls before the selected blend mode.
        // Out-of-clip instances remain dimmed in this diagnostic view.
        const float fade = getPreviewLayerAlpha(layer) / 0.75f;
        const float brightness = opacity->empty() ? 1.0f
            : opacity->at(opacity->size() == input->size() ? layer.index : 0);
        const float sourceAlpha = alpha->empty() ? 1.0f
            : alpha->at(alpha->size() == input->size() ? layer.index : 0);
        ofSetColor(brightness * fade * 255.0f, brightness * fade * 255.0f,
            brightness * fade * 255.0f, sourceAlpha * fade * 255.0f);
        ofMesh quad;
        quad.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
        for(std::size_t i = 0; i < 4; ++i){
            quad.addVertex(layer.corners[i]);
            quad.addTexCoord(layer.texCoords[i]);
        }
        texture->bind();
        quad.draw();
        texture->unbind();
        hasBaseLayer = true;
    }
    previewCamera.end();
    // Clear alpha only outside the geometry. Keeping the opaque clear during
    // blending is necessary for custom destination-alpha blend functions.
    glDisable(GL_BLEND);
    glStencilFunc(GL_EQUAL, 0, 0xff);
    glStencilMask(0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
    ofSetColor(0, 0, 0, 0);
    drawPreviewFullscreenQuad(false);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_STENCIL_TEST);
    previewLayersFbo.end();

    previewFbo.begin(OF_FBOMODE_NODEFAULTS);
    previewCamera.begin(ofRectangle(0, 0, viewport.width, viewport.height));
    ofClear(22, 25, 30, 255);
    ofEnableAlphaBlending();
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
    ofSetColor(255);
    ofSetLineWidth(1.0f);
    grid.draw();
    previewCamera.end();
    // The preview slider fades the completed blend, leaving the node's blend
    // weights and layer order unchanged. Preserve opaque alpha for ImGui.
    ofSetColor(ofFloatColor(1.0f, 1.0f, 1.0f, previewTextureOpacity));
    previewLayersFbo.getTexture().bind();
    drawPreviewFullscreenQuad(true);
    previewLayersFbo.getTexture().unbind();
    previewFbo.end();
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
