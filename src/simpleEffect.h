//
//  simpleEffect.h
//  17820_Cam
//
//  Created by Eduard Frigola Bagué on 26/9/22.
//

#ifndef simpleEffect_h
#define simpleEffect_h

#include "ofMain.h"
#include "ofxOceanodeNodeModel.h"
#include "ofxOceanodeShared.h"

#include <cctype>
#include <cmath>
#include <cstdint>
#include <regex>
#include "effectShaderSource.h"

class simpleEffect : public ofxOceanodeNodeModel {
public:
    simpleEffect(std::string name, std::string config, std::string shaderPath = "")
    : ofxOceanodeNodeModel(name)
    , effectName(std::move(name))
    , conf(std::move(config))
    , fragmentPath(std::move(shaderPath)){};

    void setup() override{
        allocateBlackTexture();

        auto inputParameter = addParameter(input.set("Input", nullptr));
        inputParameter->addConnectFunc([this](){ clearHistory(); });
        inputParameter->addDisconnectFunc([this](){ clearHistory(); });
        addParameter(bypass.set("Bypass", false));
        addEffectParameters();
        addOutputParameter(output.set("Output", nullptr));

        addInspectorParameter(drawOnEvent.set("Draw On Event", false));
        addInspectorParameter(reloadShader.set("Reload Shader"));

        shaderValid.set("Shader Valid", false);
        shaderValid.setSerializable(false);
        addInspectorParameter(shaderValid);

        shaderStatus.set("Shader Status", "Not loaded");
        shaderStatus.setSerializable(false);
        addInspectorParameter(shaderStatus);

        listeners.push(input.newListener([this](ofTexture* &){
            ++inputRevision;
            if(input.get() == nullptr || !input.get()->isAllocated()){
                clearHistory();
            }
            requestCompute();
        }));
        listeners.push(bypass.newListener([this](bool &){
            clearHistory();
            requestCompute();
        }));
        listeners.push(clearButton.newListener([this](){
            clearHistory();
            requestCompute();
        }));
        listeners.push(drawOnEvent.newListener([this](bool &enabled){
            if(enabled){
                compute();
            }
        }));
        listeners.push(reloadShader.newListener([this](){
            loadShader();
            requestCompute();
        }));

        loadShader();
        syncHistoryClearControl(); // Initial setup runs before the node GUI exists.
    }

    void update(ofEventArgs &) override{
        // A Reload Shader click can happen while the Inspector iterates its
        // controls. Apply any control change safely on the next update.
        if(historyClearControlDirty){
            syncHistoryClearControl();
        }
    }

    void draw(ofEventArgs &) override{
        if(!drawOnEvent){
            compute();
        }
    }

    void compute(){
        ofTexture* source = input.get();
        if(source == nullptr || !source->isAllocated() || source->getWidth() <= 0 || source->getHeight() <= 0){
            clearHistory();
            output = nullptr;
            return;
        }

        if(bypass){
            clearHistory();
            output = source;
            return;
        }

        if(!shaderValid || !shader.isLoaded()){
            output = nullptr;
            return;
        }

        if((usesHistory || usesOutputHistory) && source->getTextureData().textureTarget != GL_TEXTURE_2D){
            clearHistory();
            output = nullptr;
            return;
        }

        // Opt in by declaring an active tPreviousSource sampler. Keep a stable
        // pair across redraws and control edits; only Input notifications advance it.
        if(usesHistory){
            if(!prepareHistory(*source)){
                output = nullptr;
                return;
            }
            source = &history[historyIndex].getTexture();
        }

        int outputWidth = static_cast<int>(source->getWidth());
        int outputHeight = static_cast<int>(source->getHeight());
        if(swapDimensionsParameter >= 0 && controls[swapDimensionsParameter]->boolean.get()){
            std::swap(outputWidth, outputHeight);
        }

        ofFbo *renderTarget = &fbo;
        if(usesOutputHistory){
            if(!prepareOutputHistory(outputWidth, outputHeight)){
                output = nullptr;
                return;
            }
            // Never sample the texture attached to the current render target.
            renderTarget = &outputHistory[1 - outputHistoryIndex];
        }else if(!fbo.isAllocated() || fbo.getWidth() != outputWidth || fbo.getHeight() != outputHeight || fbo.getTexture().getTextureData().glInternalFormat != ofxOceanodeShared::getColorTextureInternalFormat()){
            ofFbo::Settings settings;
            settings.height = outputHeight;
            settings.width = outputWidth;
            settings.internalformat = ofxOceanodeShared::getColorTextureInternalFormat();
            settings.maxFilter = GL_NEAREST;
            settings.minFilter = GL_NEAREST;
            settings.numColorbuffers = 1;
            settings.useDepth = false;
            settings.useStencil = false;
            settings.textureTarget = GL_TEXTURE_2D;

            fbo.allocate(settings);
        }

        if(!renderTarget->isAllocated()){
            output = nullptr;
            return;
        }

        boundTextureUnits.clear();

        renderTarget->begin();
        ofClear(0, 0, 0, 0);
        shader.begin();
        ofPushStyle();
        if(usesHistory || usesOutputHistory || swapDimensionsParameter >= 0){
            ofDisableAlphaBlending(); // Preserve RGBA exactly for history and canvas transforms.
            ofFill();
        }
        ofSetColor(255, 255, 255, 255);

        bindTextureUniform("tSource", *source, 0);
        if(usesHistory){
            bindTextureUniform("tPreviousSource", historyValid ? history[1 - historyIndex].getTexture() : blackTexture, 1);
            shader.setUniform1i("tPreviousSourceConnected", historyValid ? 1 : 0);
        }
        if(usesOutputHistory){
            bindTextureUniform("tPreviousOutput", outputHistoryValid ? outputHistory[outputHistoryIndex].getTexture() : blackTexture,
                               usesHistory ? 2 : 1);
            shader.setUniform1i("tPreviousOutputConnected", outputHistoryValid ? 1 : 0);
        }
        bindStandardUniforms(outputWidth, outputHeight);
        bindEffectUniforms();

        ofDrawRectangle(0, 0, renderTarget->getWidth(), renderTarget->getHeight());

        ofPopStyle();
        shader.end();
        renderTarget->end();

        cleanupTextureUnits();
        if(usesOutputHistory){
            outputHistoryIndex = 1 - outputHistoryIndex;
            outputHistoryValid = true;
            // ofTexture assignment shares GPU storage. Publish a stable pointer
            // so consumers do not mistake buffer alternation for reconnection.
            feedbackOutput = renderTarget->getTexture();
            output = &feedbackOutput;
        }else{
            output = &fbo.getTexture();
        }
    }

    void deactivate() override{
        fbo.clear();
        clearHistory();
        output = nullptr;
    }

protected:
    void syncHistoryClearControl(){
        const bool needed = usesHistory || usesOutputHistory;
        historyClearControlDirty = false;
        if(needed == historyClearControlVisible) return;

        if(historyClearControlVisible){
            removeParameter("Clear");
        }
        if(needed){
            addParameter(clearButton.set("Clear"));
            getParameterGroup().reorder({"Input", "Bypass", "Clear"});
        }
        historyClearControlVisible = needed;
    }

    void clearHistory(){
        history[0].clear();
        history[1].clear();
        historyIndex = 0;
        historyCaptured = false;
        historyValid = false;
        clearOutputHistory();
    }

    void clearOutputHistory(){
        feedbackOutput.clear();
        outputHistory[0].clear();
        outputHistory[1].clear();
        outputHistoryIndex = 0;
        outputHistoryValid = false;
    }

    bool prepareOutputHistory(int width, int height){
        if(!outputHistory[0].isAllocated() || !outputHistory[1].isAllocated() ||
           outputHistory[0].getWidth() != width || outputHistory[0].getHeight() != height || outputHistory[0].getTexture().getTextureData().glInternalFormat != ofxOceanodeShared::getColorTextureInternalFormat()){
            clearOutputHistory();
            ofFbo::Settings settings;
            settings.width = width;
            settings.height = height;
            settings.internalformat = ofxOceanodeShared::getColorTextureInternalFormat();
            settings.textureTarget = GL_TEXTURE_2D;
            settings.minFilter = GL_NEAREST;
            settings.maxFilter = GL_NEAREST;
            settings.numColorbuffers = 1;
            settings.useDepth = false;
            settings.useStencil = false;
            outputHistory[0].allocate(settings);
            outputHistory[1].allocate(settings);
            if(!outputHistory[0].isAllocated() || !outputHistory[1].isAllocated()){
                clearOutputHistory();
                return false;
            }
        }
        return true;
    }

    bool prepareHistory(const ofTexture &source){
        if(!history[0].isAllocated() || !history[1].isAllocated() ||
           history[0].getWidth() != source.getWidth() || history[0].getHeight() != source.getHeight() || history[0].getTexture().getTextureData().glInternalFormat != ofxOceanodeShared::getColorTextureInternalFormat()){
            clearHistory();
            ofFbo::Settings settings;
            settings.width = source.getWidth();
            settings.height = source.getHeight();
            settings.internalformat = ofxOceanodeShared::getColorTextureInternalFormat();
            settings.textureTarget = GL_TEXTURE_2D;
            settings.minFilter = GL_NEAREST;
            settings.maxFilter = GL_NEAREST;
            settings.numColorbuffers = 1;
            settings.useDepth = false;
            settings.useStencil = false;
            history[0].allocate(settings);
            history[1].allocate(settings);
            if(!history[0].isAllocated() || !history[1].isAllocated()){
                clearHistory();
                return false;
            }
        }

        if(!historyCaptured || capturedRevision != inputRevision){
            if(historyCaptured){
                historyIndex = 1 - historyIndex;
            }
            history[historyIndex].begin();
            ofPushStyle();
            ofDisableAlphaBlending();
            ofFill();
            ofSetColor(255);
            historyCopyShader.begin();
            historyCopyShader.setUniformTexture("tSource", source, 0);
            // Copy texels directly, without texture.draw()'s display flip or tint.
            ofDrawRectangle(0, 0, source.getWidth(), source.getHeight());
            historyCopyShader.end();
            ofPopStyle();
            history[historyIndex].end();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);

            historyValid = historyCaptured;
            historyCaptured = true;
            capturedRevision = inputRevision;
        }
        return true;
    }

    using ParameterType = effectShaderSource::ParameterType;
    using EffectParameterSpec = effectShaderSource::EffectParameterSpec;

    void parseConfiguration(){
        effectShaderSource::parseConfiguration(conf, effectParameters, metadataWarnings);
    }

    struct EffectControl {
        EffectParameterSpec spec;
        ofParameter<float> number;
        ofParameter<bool> boolean;
        ofParameter<ofFloatColor> color;
        ofParameter<ofTexture*> texture;
        ofTexture *connectedTexture = nullptr;
        ofEventListeners listeners;
    };

    void addEffectParameters(){
        parseConfiguration();
        syncEffectParameters();
    }

    void syncEffectParameters(){
        rebuildingControls = true;
        auto previous = controls;
        // Disconnect removed/retyped ports before releasing their listener state.
        for(const auto &control : previous){
            if(std::none_of(effectParameters.begin(), effectParameters.end(), [&](const auto &spec){
                return spec.name == control->spec.name && spec.type == control->spec.type;
            })) removeParameter(control->spec.name);
        }
        controls.clear();
        for(const auto &spec : effectParameters){
            auto existing = std::find_if(previous.begin(), previous.end(), [&](const auto &control){
                return spec.name == control->spec.name && spec.type == control->spec.type;
            });
            if(existing != previous.end()){
                auto control = *existing;
                control->spec = spec;
                if(spec.type == ParameterType::Float){
                    control->number.setMin(spec.minValue);
                    control->number.setMax(spec.maxValue);
                }
                controls.push_back(control);
                continue;
            }
            auto control = std::make_shared<EffectControl>();
            control->spec = spec;
            controls.push_back(control);
            // Node wrappers own the callbacks; weak captures avoid ownership cycles.
            std::weak_ptr<EffectControl> weak = control;
            auto receive = [this, weak](ofTexture *const &texture){
                if(auto c = weak.lock()) c->connectedTexture = texture;
                requestCompute();
            };
            auto disconnect = [this, weak](){
                if(auto c = weak.lock()) c->connectedTexture = nullptr;
                requestCompute();
            };
            if(spec.type == ParameterType::Bool){
                addParameter(control->boolean.set(spec.name, spec.defaultBool));
                control->listeners.push(control->boolean.newListener([this](bool &){ requestCompute(); }));
            }else if(spec.type == ParameterType::Color){
                const auto &c = spec.defaultColor;
                addParameter(control->color.set(spec.name, ofFloatColor(c[0], c[1], c[2], c[3]),
                                               ofFloatColor(0, 0, 0, 0), ofFloatColor(1, 1, 1, 1)));
                control->listeners.push(control->color.newListener([this](ofFloatColor &){ requestCompute(); }));
            }else if(spec.type == ParameterType::Texture){
                auto parameter = addParameter(control->texture.set(spec.name, nullptr));
                control->listeners.push(control->texture.newListener([receive](ofTexture* &t){ receive(t); }));
                parameter->addReceiveFunc<ofTexture*>(receive);
                parameter->addDisconnectFunc(disconnect);
            }else{
                auto parameter = addParameter(control->number.set(spec.name, spec.defaultValue, spec.minValue, spec.maxValue));
                control->listeners.push(control->number.newListener([this](float &){ requestCompute(); }));
                parameter->addReceiveFunc<ofTexture*>(receive);
                parameter->addDisconnectFunc(disconnect);
            }
        }
        rebuildingControls = false;
    }

    void requestCompute(){
        if(drawOnEvent && !rebuildingControls){
            compute();
        }
    }

    void allocateBlackTexture(){
        blackTexture.allocate(1, 1, GL_RGBA32F, false);
        const float blackPixel[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        blackTexture.loadData(blackPixel, 1, 1, GL_RGBA);
        blackTexture.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
    }

    virtual void loadShader(){
        const std::string effectFragmentPath = fragmentPath.empty()
                                             ? "Effects/" + effectName + ".glsl"
                                             : fragmentPath;
        const std::string absoluteFragmentPath = ofToDataPath(effectFragmentPath, true);
        const ofBuffer fragmentBuffer = ofBufferFromFile(absoluteFragmentPath);

        if(fragmentBuffer.size() == 0){
            setShaderFailure("Could not read " + effectFragmentPath);
            return;
        }

        compileShaderSource(fragmentBuffer.getText(), absoluteFragmentPath);
    }

    bool compileShaderSource(const std::string &source, const std::string &filePath = ""){
        std::string defaultVertSource =
        #include "defaultVertexShader.h"
        ;

        ofShader candidate;
        if(!candidate.setupShaderFromSource(GL_VERTEX_SHADER, defaultVertSource)){
            setShaderFailure("Vertex shader compilation failed: " + shaderInfoLog(candidate, GL_VERTEX_SHADER));
            return false;
        }

        if(!(filePath.empty() ? candidate.setupShaderFromSource(GL_FRAGMENT_SHADER, source)
                            : candidate.setupShaderFromFile(GL_FRAGMENT_SHADER, filePath))){
            setShaderFailure("Fragment shader compilation failed: " + shaderInfoLog(candidate, GL_FRAGMENT_SHADER));
            return false;
        }

        if(!candidate.bindDefaults()){
            setShaderFailure("Could not bind default shader attributes");
            return false;
        }

        candidate.linkProgram();
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(candidate.getProgram(), GL_LINK_STATUS, &linkStatus);
        if(linkStatus != GL_TRUE){
            setShaderFailure("Shader link failed: " + programInfoLog(candidate));
            return false;
        }

        const auto interfaceError = candidateInterfaceError(candidate);
        if(!interfaceError.empty()){
            setShaderFailure(interfaceError);
            return false;
        }

        const bool candidateUsesHistory = candidate.getUniformLocation("tPreviousSource") >= 0;
        const bool candidateUsesOutputHistory = candidate.getUniformLocation("tPreviousOutput") >= 0;
        if(candidateUsesHistory && !historyCopyShader.isLoaded()){
            ofShader copyCandidate;
            const std::string copyFragment = R"(#version 410
uniform sampler2D tSource;
out vec4 out_color;
void main(){ out_color = texelFetch(tSource, ivec2(gl_FragCoord.xy), 0); }
)";
            if(!copyCandidate.setupShaderFromSource(GL_VERTEX_SHADER, defaultVertSource) ||
               !copyCandidate.setupShaderFromSource(GL_FRAGMENT_SHADER, copyFragment) ||
               !copyCandidate.bindDefaults()){
                setShaderFailure("Could not compile/link the input history copy shader");
                return false;
            }
            copyCandidate.linkProgram();
            GLint copyLinkStatus = GL_FALSE;
            glGetProgramiv(copyCandidate.getProgram(), GL_LINK_STATUS, &copyLinkStatus);
            if(copyLinkStatus != GL_TRUE){
                setShaderFailure("Input history copy shader link failed: " + programInfoLog(copyCandidate));
                return false;
            }
            historyCopyShader = std::move(copyCandidate);
        }

        shader = std::move(candidate);
        updateDescription(source);
        usesHistory = candidateUsesHistory;
        usesOutputHistory = candidateUsesOutputHistory;
        historyClearControlDirty = true;
        clearHistory();
        if(usesOutputHistory){
            fbo.clear(); // Feedback renders directly into its two alternating buffers.
        }
        shaderValid = true;

        std::vector<std::string> warnings = metadataWarnings;
        swapDimensionsParameter = findSwapDimensionsParameter(source, warnings);
        const auto interfaceWarnings = validateShaderInterface(shader.getShaderSource(GL_FRAGMENT_SHADER));
        warnings.insert(warnings.end(), interfaceWarnings.begin(), interfaceWarnings.end());

        if(warnings.empty()){
            shaderStatus = "OK";
        }else{
            shaderStatus = "OK with warnings: " + ofJoinString(warnings, " | ");
            for(const auto &warning : warnings){
                ofLogWarning("simpleEffect") << effectName << ": " << warning;
            }
        }
        return true;
    }

    virtual std::string candidateInterfaceError(const ofShader &) const { return ""; }

    void setShaderFailure(const std::string &failure){
        const bool previousShaderStillAvailable = shaderValid.get() && shader.isLoaded();
        shaderValid = previousShaderStillAvailable;
        shaderStatus = previousShaderStillAvailable
                     ? "Reload failed; using previous shader: " + failure
                     : "ERROR: " + failure;
        ofLogError("simpleEffect") << effectName << ": " << shaderStatus.get();
    }

    void updateDescription(const std::string &source){
        std::vector<std::string> summaryLines;
        std::vector<std::string> parameterLines;

        for(const auto &line : ofSplitString(source, "\n")){
            const std::string trimmedLine = ofTrim(line);
            const std::string descriptionPrefix = "// @description ";
            const std::string parameterPrefix = "// @param ";

            if(trimmedLine.rfind(descriptionPrefix, 0) == 0){
                summaryLines.push_back(ofTrim(trimmedLine.substr(descriptionPrefix.size())));
            }else if(trimmedLine.rfind(parameterPrefix, 0) == 0){
                parameterLines.push_back(ofTrim(trimmedLine.substr(parameterPrefix.size())));
            }
        }

        description = ofJoinString(summaryLines, " ");
        if(!parameterLines.empty()){
            if(!description.empty()){
                description += "\n\n";
            }
            description += "Parameters:\n" + ofJoinString(parameterLines, "\n");
        }
    }

    int findSwapDimensionsParameter(const std::string &source, std::vector<std::string> &warnings) const{
        const std::string prefix = "// @swap-dimensions ";
        for(const auto &line : ofSplitString(source, "\n")){
            const std::string trimmedLine = ofTrim(line);
            if(trimmedLine.rfind(prefix, 0) != 0) continue;

            const std::string name = ofTrim(trimmedLine.substr(prefix.size()));
            for(size_t i = 0; i < effectParameters.size(); i++){
                if(effectParameters[i].name == name && effectParameters[i].type == ParameterType::Bool){
                    return static_cast<int>(i);
                }
            }
            warnings.push_back("@swap-dimensions must name a boolean metadata parameter: " + name);
        }
        return -1;
    }

    std::vector<std::string> validateShaderInterface(const std::string &source) const{
        std::vector<std::string> warnings;

        for(const auto &spec : effectParameters){
            std::string expectedType;
            switch(spec.type){
                case ParameterType::Float: expectedType = "float"; break;
                case ParameterType::Bool: expectedType = "bool"; break;
                case ParameterType::Color: expectedType = "vec4"; break;
                case ParameterType::Texture: expectedType = "sampler2D"; break;
            }

            const std::regex declaration("\\buniform\\s+" + expectedType + "\\s+" + spec.name + "\\s*;");
            if(!std::regex_search(source, declaration)){
                warnings.push_back("Metadata parameter " + spec.name + " expects 'uniform " + expectedType + " " + spec.name + ";'");
            }
        }

        return warnings;
    }

    static std::string shaderInfoLog(const ofShader &shaderToInspect, GLenum shaderType){
        const GLuint shaderId = shaderToInspect.getShader(shaderType);
        if(shaderId == 0){
            return "No compiler log available";
        }

        GLint length = 0;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &length);
        if(length <= 1){
            return "No compiler log available";
        }

        std::vector<GLchar> log(static_cast<size_t>(length));
        GLsizei written = 0;
        glGetShaderInfoLog(shaderId, length, &written, log.data());
        return ofTrim(std::string(log.data(), static_cast<size_t>(written)));
    }

    static std::string programInfoLog(const ofShader &shaderToInspect){
        GLint length = 0;
        glGetProgramiv(shaderToInspect.getProgram(), GL_INFO_LOG_LENGTH, &length);
        if(length <= 1){
            return "No linker log available";
        }

        std::vector<GLchar> log(static_cast<size_t>(length));
        GLsizei written = 0;
        glGetProgramInfoLog(shaderToInspect.getProgram(), length, &written, log.data());
        return ofTrim(std::string(log.data(), static_cast<size_t>(written)));
    }

    void bindStandardUniforms(float width, float height){
        const float time = ofGetElapsedTimef();
        const int frame = static_cast<int>(ofGetFrameNum());

        shader.setUniform2f("uResolution", width, height);
        shader.setUniform2f("resolution", width, height); // Legacy alias.
        shader.setUniform2f("uTexelSize", 1.0f / width, 1.0f / height);
        shader.setUniform1f("uTime", time);
        shader.setUniform1f("time", time); // Legacy alias.
        shader.setUniform1i("uFrame", frame);
        shader.setUniform1i("frame", frame); // Legacy alias.
        shader.setUniform1i("tSourceConnected", 1);
    }

    void bindEffectUniforms(){
        int textureUnit = 1 + (usesHistory ? 1 : 0) + (usesOutputHistory ? 1 : 0);

        for(size_t i = 0; i < effectParameters.size(); i++){
            const auto &spec = effectParameters[i];

            if(spec.type == ParameterType::Bool){
                shader.setUniform1i(spec.name, controls[i]->boolean.get() ? 1 : 0);
            }else if(spec.type == ParameterType::Color){
                shader.setUniform4f(spec.name, controls[i]->color);
            }else if(spec.type == ParameterType::Texture){
                const bool connected = controls[i]->connectedTexture != nullptr && controls[i]->connectedTexture->isAllocated();
                shader.setUniform1i(spec.name + "Connected", connected ? 1 : 0);

                if(shader.getUniformLocation(spec.name) >= 0){
                    ofTexture* textureToUse = connected ? controls[i]->connectedTexture : &blackTexture;
                    bindTextureUniform(spec.name, *textureToUse, textureUnit++);
                }
            }else{
                const bool connected = controls[i]->connectedTexture != nullptr && controls[i]->connectedTexture->isAllocated();
                const std::string textureUniformName = spec.name + "Tex";

                shader.setUniform1f(spec.name, controls[i]->number);
                shader.setUniform1i(textureUniformName + "Connected", connected ? 1 : 0);

                if(shader.getUniformLocation(textureUniformName) >= 0){
                    ofTexture* textureToUse = connected ? controls[i]->connectedTexture : &blackTexture;
                    bindTextureUniform(textureUniformName, *textureToUse, textureUnit++);
                }
            }
        }
    }

    void bindTextureUniform(const std::string &name, const ofTexture &texture, int textureUnit){
        if(shader.getUniformLocation(name) < 0){
            return;
        }

        shader.setUniformTexture(name, texture, textureUnit);
        boundTextureUnits.emplace_back(texture.getTextureData().textureTarget, textureUnit);
    }

    void cleanupTextureUnits(){
        for(const auto &binding : boundTextureUnits){
            glActiveTexture(GL_TEXTURE0 + binding.second);
            glBindTexture(binding.first, 0);
        }
        glActiveTexture(GL_TEXTURE0);
        boundTextureUnits.clear();
    }

    ofEventListeners listeners;

    std::vector<EffectParameterSpec> effectParameters;
    std::vector<std::string> metadataWarnings;
    std::vector<std::shared_ptr<EffectControl>> controls;
    bool rebuildingControls = false;
    std::vector<std::pair<GLenum, int>> boundTextureUnits;

    ofParameter<ofTexture*> input;
    ofParameter<ofTexture*> output;

    ofParameter<bool> drawOnEvent;
    ofParameter<bool> bypass;
    ofParameter<void> reloadShader;
    ofParameter<void> clearButton;
    bool historyClearControlVisible = false;
    bool historyClearControlDirty = false;
    ofParameter<bool> shaderValid;
    ofParameter<std::string> shaderStatus;

    ofFbo fbo;
    ofFbo history[2];
    ofFbo outputHistory[2];
    ofTexture feedbackOutput;
    ofShader historyCopyShader;
    bool usesHistory = false;
    bool usesOutputHistory = false;
    int swapDimensionsParameter = -1;
    bool outputHistoryValid = false;
    int outputHistoryIndex = 0;
    bool historyCaptured = false;
    bool historyValid = false;
    int historyIndex = 0;
    uint64_t inputRevision = 0;
    uint64_t capturedRevision = 0;
    ofTexture blackTexture;
    ofShader shader;

    std::string effectName;
    std::string conf;
    std::string fragmentPath;
};

#endif /* simpleEffect_h */
