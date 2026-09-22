//
//  textureComposerSimple.h
//  ofxOceanodeTextures
//

#ifndef textureComposerSimple_h
#define textureComposerSimple_h

#include "ofxOceanodeNodeModel.h"

class textureComposerSimple : public ofxOceanodeNodeModel {
public:
    textureComposerSimple() : ofxOceanodeNodeModel("Texture Composer Simple"){}

    void setup() override{
        addInspectorParameter(numTextures.set("Num Textures", 2, 1, 32));
        addParameter(width.set("Width", 100, 1, 50000));
        addParameter(height.set("Height", 100, 1, 50000));
        addOutputParameter(output.set("Output", nullptr));

        inputs.resize(numTextures);
        xPositions.resize(numTextures);
        yPositions.resize(numTextures);
        blendModes.resize(numTextures);
        opacities.resize(numTextures);
        mattes.resize(numTextures);

        const std::vector<std::string> blendOptions = {
            "Normal",
            "Multiply",
            "Average",
            "Add",
            "Substract",
            "Difference",
            "Negation",
            "Exclusion",
            "Screen",
            "Overlay",
            "SoftLight",
            "HardLight",
            "ColorDodge",
            "ColorBurn",
            "LinearLight",
            "VividLight",
            "PinLight",
            "HardMix",
            "Reflect",
            "Glow",
            "Phoenix",
            "Hue",
            "Saturation",
            "Color",
            "Luminosity",
            "Maximum",
            "Minimum"
        };

        // Track matte: the layer uses the layer directly above it (lower number,
        // e.g. layer 01 for layer 02) as its matte. The matte layer is not drawn.
        const std::vector<std::string> matteOptions = {
            "None",
            "Alpha",
            "Alpha Inv",
            "Luma",
            "Luma Inv"
        };

        auto addLayerParameters = [this, blendOptions, matteOptions](int start, int count){
            for(int i = start; i < start + count; i++){
                const std::string suffix = ofToString(i + 1, 2, '0');

                addParameter(inputs[i].set("In " + suffix, nullptr));
                addParameter(xPositions[i].set("X " + suffix, 0, -50000, 50000));
                addParameter(yPositions[i].set("Y " + suffix, 0, -50000, 50000));
                addParameterDropdown(blendModes[i], "Blend " + suffix, 0, blendOptions);
                addParameterDropdown(mattes[i], "Matte " + suffix, 0, matteOptions);
                addParameter(opacities[i].set("Opacity " + suffix, 1.0f, 0.0f, 1.0f));
            }
        };

        addLayerParameters(0, numTextures);

        listener = numTextures.newListener([this, addLayerParameters](int &newSize){
            const int oldSize = inputs.size();
            if(oldSize == newSize){
                return;
            }

            if(oldSize > newSize){
                for(int i = oldSize - 1; i >= newSize; i--){
                    const std::string suffix = ofToString(i + 1, 2, '0');
                    removeParameter("In " + suffix);
                    removeParameter("X " + suffix);
                    removeParameter("Y " + suffix);
                    removeParameter("Blend " + suffix);
                    removeParameter("Matte " + suffix);
                    removeParameter("Opacity " + suffix);
                }
            }

            inputs.resize(newSize);
            xPositions.resize(newSize);
            yPositions.resize(newSize);
            blendModes.resize(newSize);
            opacities.resize(newSize);
            mattes.resize(newSize);

            if(oldSize < newSize){
                addLayerParameters(oldSize, newSize - oldSize);
            }
        });

        const std::string defaultVertexSource =
#include "shaders/defaultVertexShader.h"
        ;
        const std::string blendFragmentSource =
#include "shaders/mixerAlphaShader.h"
        ;

        shader.setupShaderFromSource(GL_VERTEX_SHADER, defaultVertexSource);
        shader.setupShaderFromSource(GL_FRAGMENT_SHADER, blendFragmentSource);
        shader.bindDefaults();
        shader.linkProgram();

        matteShader.setupShaderFromSource(GL_VERTEX_SHADER, defaultVertexSource);
        matteShader.setupShaderFromSource(GL_FRAGMENT_SHADER, matteFragmentSource);
        matteShader.bindDefaults();
        matteShader.linkProgram();
    }

    void draw(ofEventArgs &args) override{
        (void)args;

        if(!canvasIsAllocated()){
            allocateCanvas();
        }

        if(!canvasIsAllocated()){
            output = nullptr;
            return;
        }

        pingPongIndex = 0;
        pingPongFbo[pingPongIndex].begin();
        ofClear(0, 0, 0, 0);
        pingPongFbo[pingPongIndex].end();

        const int layerCount = numTextures;
        bool hasBaseLayer = false;
        for(int i = layerCount - 1; i >= 0; i--){
            // A layer that serves as the matte of the layer below it is not drawn.
            if(layerIsUsedAsMatte(i)){
                continue;
            }

            // Standard texture connections update the parameter itself.
            ofTexture *texture = inputs[i].get();
            if(texture == nullptr || !texture->isAllocated() || opacities[i] <= 0.0f){
                continue;
            }

            // Render each source at its native size. X/Y use the canvas top-left as origin.
            layerFbo.begin();
            ofClear(0, 0, 0, 0);
            ofPushStyle();
            // Copy the source RGBA values verbatim. Most texture-producing
            // nodes render into a transparent FBO, so their RGB is already
            // weighted by alpha. Alpha blending here would premultiply the
            // texture a second time before it even reaches the blend shader.
            ofDisableAlphaBlending();
            ofSetColor(255, 255, 255, 255);
            texture->draw(xPositions[i], yPositions[i]);
            ofPopStyle();
            layerFbo.end();

            ofFbo *sourceFbo = &layerFbo;
            if(layerHasMatte(i)){
                applyMatte(i);
                sourceFbo = &maskedFbo;
            }

            pingPongFbo[!pingPongIndex].begin();
            ofClear(0, 0, 0, 0);
            ofPushStyle();
            // The shader computes the complete RGBA result. Fixed-function
            // alpha blending here would composite that result onto the newly
            // cleared FBO once more. It is especially destructive for empty
            // upper layers: even when the shader returns baseCol unchanged,
            // GL_SRC_ALPHA would multiply it by its own alpha again.
            ofDisableAlphaBlending();
            shader.begin();
            shader.setUniformTexture("base", pingPongFbo[pingPongIndex].getTexture(), 0);
            shader.setUniformTexture("blendTgt", sourceFbo->getTexture(), 1);
            // The first visible layer has no base to blend against, so it is composited normally.
            shader.setUniform1i("mode", hasBaseLayer ? blendModes[i].get() : 0);
            shader.setUniform1f("opacity", opacities[i].get());
            shader.setUniform1i("premultipliedInput", 1);
            ofDrawRectangle(0, 0, width, height);
            shader.end();
            ofPopStyle();
            pingPongFbo[!pingPongIndex].end();

            unbindTextureUnits();
            pingPongIndex = !pingPongIndex;
            hasBaseLayer = true;
        }

        output = &pingPongFbo[pingPongIndex].getTexture();
    }

    void deactivate(){
        layerFbo.clear();
        matteFbo.clear();
        maskedFbo.clear();
        pingPongFbo[0].clear();
        pingPongFbo[1].clear();
        output = nullptr;
    }

    void loadBeforeConnections(ofJson &json){
        deserializeParameter(json, numTextures);
    }

private:
    bool layerHasMatte(int i) const{
        return i > 0 && i < (int)mattes.size() && mattes[i].get() != 0;
    }

    bool layerIsUsedAsMatte(int i) const{
        return layerHasMatte(i + 1);
    }

    // Renders the matte layer (i - 1) at its own X/Y, then writes layerFbo
    // multiplied by the matte factor into maskedFbo. All composer buffers hold
    // premultiplied RGBA, so scaling every channel keeps soft edges correct.
    void applyMatte(int i){
        if(!matteFbo.isAllocated() || matteFbo.getWidth() != width || matteFbo.getHeight() != height){
            ofFbo::Settings settings = canvasSettings();
            matteFbo.allocate(settings);
            maskedFbo.allocate(settings);
        }

        const int matteIndex = i - 1;
        ofTexture *matteTexture = inputs[matteIndex].get();

        matteFbo.begin();
        ofClear(0, 0, 0, 0);
        if(matteTexture != nullptr && matteTexture->isAllocated()){
            ofPushStyle();
            ofDisableAlphaBlending();
            ofSetColor(255, 255, 255, 255);
            matteTexture->draw(xPositions[matteIndex], yPositions[matteIndex]);
            ofPopStyle();
        }
        matteFbo.end();

        maskedFbo.begin();
        ofClear(0, 0, 0, 0);
        ofPushStyle();
        ofDisableAlphaBlending();
        matteShader.begin();
        matteShader.setUniformTexture("layerTex", layerFbo.getTexture(), 0);
        matteShader.setUniformTexture("matteTex", matteFbo.getTexture(), 1);
        matteShader.setUniform1i("matteMode", mattes[i].get());
        matteShader.setUniform1f("matteOpacity", opacities[matteIndex].get());
        ofDrawRectangle(0, 0, width, height);
        matteShader.end();
        ofPopStyle();
        maskedFbo.end();

        unbindTextureUnits();
    }

    ofFbo::Settings canvasSettings() const{
        ofFbo::Settings settings;
        settings.width = width;
        settings.height = height;
        settings.internalformat = GL_RGBA8;
        settings.numColorbuffers = 1;
        settings.useDepth = false;
        settings.useStencil = false;
        settings.textureTarget = GL_TEXTURE_2D;
        settings.maxFilter = GL_NEAREST;
        settings.minFilter = GL_NEAREST;
        return settings;
    }

    bool canvasIsAllocated() const{
        return pingPongFbo[0].isAllocated() &&
               pingPongFbo[1].isAllocated() &&
               layerFbo.isAllocated() &&
               pingPongFbo[0].getWidth() == width &&
               pingPongFbo[0].getHeight() == height &&
               layerFbo.getWidth() == width &&
               layerFbo.getHeight() == height;
    }

    void allocateCanvas(){
        ofFbo::Settings settings;
        settings.width = width;
        settings.height = height;
        // Composition does not need float render targets. RGBA8 keeps very large
        // canvases practical (three canvas-sized FBOs are used while composing).
        settings.internalformat = GL_RGBA8;
        settings.numColorbuffers = 1;
        settings.useDepth = false;
        settings.useStencil = false;
        settings.textureTarget = GL_TEXTURE_2D;
        settings.maxFilter = GL_NEAREST;
        settings.minFilter = GL_NEAREST;

        pingPongFbo[0].allocate(settings);
        pingPongFbo[1].allocate(settings);
        layerFbo.allocate(settings);
    }

    void unbindTextureUnits(){
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
    }

    ofShader shader;
    ofShader matteShader;

    // Matte layer opacity scales the matte (like a track matte's opacity).
    // Luma is computed from premultiplied RGB, so transparent matte areas count as black.
    const std::string matteFragmentSource = R"(
#version 410

uniform sampler2D layerTex;
uniform sampler2D matteTex;
uniform int matteMode;      // 1 Alpha, 2 Alpha Inv, 3 Luma, 4 Luma Inv
uniform float matteOpacity;

out vec4 fragColor;

void main(){
    ivec2 coord = ivec2(gl_FragCoord.xy);
    vec4 layerCol = texelFetch(layerTex, coord, 0);
    vec4 matteCol = texelFetch(matteTex, coord, 0);

    float m;
    if(matteMode == 1 || matteMode == 2){
        m = matteCol.a;
    }else{
        m = dot(matteCol.rgb, vec3(0.2126, 0.7152, 0.0722));
    }
    m = clamp(m * matteOpacity, 0.0, 1.0);
    if(matteMode == 2 || matteMode == 4){
        m = 1.0 - m;
    }

    fragColor = layerCol * m;
}
)";

    ofParameter<int> numTextures;
    ofParameter<int> width;
    ofParameter<int> height;
    ofParameter<ofTexture*> output;

    std::vector<ofParameter<ofTexture*>> inputs;
    std::vector<ofParameter<int>> xPositions;
    std::vector<ofParameter<int>> yPositions;
    std::vector<ofParameter<int>> blendModes;
    std::vector<ofParameter<float>> opacities;
    std::vector<ofParameter<int>> mattes;

    ofEventListener listener;

    ofFbo layerFbo;
    ofFbo matteFbo;
    ofFbo maskedFbo;
    ofFbo pingPongFbo[2];
    int pingPongIndex = 0;
};

#endif /* textureComposerSimple_h */
