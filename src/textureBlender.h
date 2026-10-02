//
//  textureBlender.h
//  ofxOceanodeTextures
//
//  Created by Eduard Frigola on 22/12/23.
//

#ifndef textureBlender_h
#define textureBlender_h

#include "ofxOceanodeNodeModel.h"
#include "ofCamera.h"
#include <array>

class textureComposer;

class textureBlender : public ofxOceanodeNodeModel{
public:
    textureBlender();
    
    void setup() override;
    void draw(ofEventArgs &) override;

    void presetRecallBeforeSettingParameters(ofJson &) override{
        loadingPreset = true;
    }

    void presetRecallAfterSettingParameters(ofJson &) override{
        loadingPreset = false;
        updateBlendModeFromParameters();
        render();
    }
    
    void deactivate(){
        fbo.clear();
        previewFbo.clear();
        previewLayersFbo.clear();
        output = nullptr;
    }
    
private:
    struct PreviewLayer{
        std::size_t index;
        std::array<glm::vec3, 4> corners;
        std::array<glm::vec2, 4> texCoords;
        glm::vec4 origin;
        glm::vec3 composerPivot = glm::vec3(0.0f);
        bool hasComposerPivot = false;
    };

    void render();
    void configureCamera();
    void configureBlending();
    textureComposer *getPreviewComposer();
    std::vector<PreviewLayer> getPreviewLayers();
    glm::mat4 getPreviewWorldTransform() const;
    std::array<glm::vec3, 4> getCameraPreviewCorners(float distance) const;
    float getPreviewLayerAlpha(const PreviewLayer &layer) const;
    void updatePreviewGridStep(float viewportHeight);
    void framePreview(const std::vector<PreviewLayer> &layers, float aspectRatio);
    void drawPreviewScene(const std::vector<PreviewLayer> &layers, const ofRectangle &viewport);
    void renderPreviewTextures(const std::vector<PreviewLayer> &layers, const ofRectangle &viewport, const ofMesh &grid);
    void applyBlendMode(int mode);
    void updateBlendModeFromParameters();

    ofEventListener listener;
    ofEventListeners blendModeListeners;
    ofEventListeners cameraListeners;

    ofParameter<int> blendMode;
    ofParameter<int> layerOrder;
    
    ofParameter<int> width;
    ofParameter<int> height;
    
    ofParameter<std::vector<ofTexture*>> input;
    ofParameter<std::vector<glm::mat4>> transformInput;
    
    ofParameter<int> blendSrcColorFunction;
    ofParameter<int> blendSrcAlphaFunction;
    ofParameter<int> blendDstColorFunction;
    ofParameter<int> blendDstAlphaFunction;
    ofParameter<int> blendColorEquation;
    ofParameter<int> blendAlphaEquation;
    ofParameter<ofFloatColor> blendColor;
    
    ofParameter<std::vector<float>> opacity;
    ofParameter<std::vector<float>> alpha;
    
    ofParameter<ofTexture*> output;
    
    ofFbo fbo;
    ofCamera camera;
    ofParameter<int> cameraProjection;
    ofParameter<float> cameraFov;
    ofParameter<bool> cameraAutoDistance;
    ofParameter<float> cameraDistance;
    ofParameter<float> cameraNearClip;
    ofParameter<float> cameraFarClip;
    
    ofParameter<bool> active;
    ofParameter<bool> show;

    ofCamera previewCamera;
    ofFbo previewFbo;
    ofFbo previewLayersFbo;
    glm::vec3 previewTarget = glm::vec3(0.0f);
    float previewYaw = 0.65f;
    float previewPitch = 0.35f;
    float previewDistance = 1000.0f;
    float previewSceneRadius = 100.0f;
    float previewGridStep = 0.0f;
    float previewTextureOpacity = 0.5f;
    bool previewInitialized = false;
    bool previewClipPlanes = false;
    bool previewShowTextures = false;

    bool updatingBlendMode = false;
    bool loadingPreset = false;
};
#endif /* textureBlender_h */
