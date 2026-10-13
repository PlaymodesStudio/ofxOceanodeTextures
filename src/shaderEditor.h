#pragma once

#include "simpleEffect.h"
#include "ofxOceanodeAIAssistant.h"
#include "ofxOceanodeShared.h"
#include "imgui_internal.h"
#include <cstring>
#include <map>

// Live authoring uses the same renderer, uniforms and history as disk effects.
class shaderEditor : public simpleEffect {
public:
    shaderEditor() : simpleEffect("Shader", effectShaderSource::configuration(initialSource())) {}

    void setup() override {
        source.set("Shader Source", initialSource());
        addInspectorParameter(source);
        addInspectorParameter(shaderName.set("Shader Name", "MyShader"));
        addInspectorParameter(loadButton.set("Load Shader"));
        addInspectorParameter(saveButton.set("Save Shader"));
        addInspectorParameter(liveCompile.set("Live Compile", true));
        addInspectorParameter(editorLines.set("Editor Lines", 20, 4, 80));
        addInspectorParameter(editorWidth.set("Editor Width", 540, 240, 1200));
        addInspectorParameter(editorFontSize.set("Editor Font Size", 14, 8, 32));
        editorRegion.set("Shader Editor", [this](){ drawEditor(); });
        addCustomRegion(editorRegion, editorRegion.get());
        simpleEffect::setup();
        // Load/save and all port changes happen in update, outside GUI iteration.
        editorListeners.push(source.newListener([this](std::string &){ markDirty(); }));
        editorListeners.push(loadButton.newListener([this](){ loadRequested = true; }));
        editorListeners.push(saveButton.newListener([this](){ saveRequested = true; }));
        setupAssistant();
    }

    void update(ofEventArgs &args) override {
        ai.update();
        if(loadRequested){ loadRequested = false; loadFile(); }
        if(compileRequested || (dirty && liveCompile && ofGetElapsedTimeMillis() - lastEditTime >= 400)){
            compileRequested = false;
            applySource();
        }
        if(saveRequested){ saveRequested = false; saveFile(); }
        simpleEffect::update(args);
    }

    void loadBeforeConnections(ofJson &json) override {
        // Restore the last working ports even if the preset was saved mid-edit.
        deserializeParameter(json, source);
        deserializeParameter(json, liveCompile);
        compileRequested = false;
        const std::string draft = source.get();
        if(json.contains("shaderEditorActiveSource") && json["shaderEditorActiveSource"].is_string()){
            source.setWithoutEventNotifications(json["shaderEditorActiveSource"].get<std::string>());
            applySource();
        }
        source.setWithoutEventNotifications(draft);
        if(liveCompile || activeSource.empty() || draft == activeSource) applySource();
        else markDirty();
    }

    void presetSave(ofJson &json) override {
        ai.save(json);
        json["shaderEditorActiveSource"] = activeSource;
    }

    void presetRecallAfterSettingParameters(ofJson &json) override { ai.load(json); }

private:
    static std::string initialSource(){
        return R"SHADER_TEMPLATE(//Amount:1:0:2, Tint:color:1:1:1:1, Mask:texture
// @description Demonstrates the standard Simple Effect shader structure.
// @param Amount: Scalar strength; it can also be driven by a texture.
// @param Tint: Color multiplied with the source.
// @param Mask: Optional texture limiting where the effect is applied.
#version 410

// Required source texture. simpleEffect renders at this texture's resolution.
uniform sampler2D tSource;

// Standard uniforms supplied automatically by simpleEffect.
uniform vec2 uResolution;
uniform vec2 uTexelSize;
uniform float uTime;
uniform int uFrame;

// Float parameter. Every float can optionally receive a texture connection.
uniform float Amount;
uniform sampler2D AmountTex;
uniform int AmountTexConnected;

// Color parameters are exposed as ofFloatColor and bound as vec4.
uniform vec4 Tint;

// Explicit texture parameter and its connection flag.
uniform sampler2D Mask;
uniform int MaskConnected;

out vec4 out_color;

float readAmount(vec2 uv)
{
    return AmountTexConnected != 0 ? texture(AmountTex, uv).r : Amount;
}

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 source = texture(tSource, uv);

    float amount = readAmount(uv);
    vec3 processed = source.rgb * Tint.rgb * amount;

    if(MaskConnected != 0){
        processed *= texture(Mask, uv).r;
    }

    // Preserve source alpha unless the effect intentionally changes it.
    out_color = vec4(processed, source.a);
}
)SHADER_TEMPLATE";
    }

    // Called by the base setup/Reload Shader control. Defer GUI reloads to update.
    void loadShader() override {
        if(!initialized){ initialized = true; applySource(); }
        else compileRequested = true;
    }

    void markDirty(){
        dirty = true;
        lastEditTime = ofGetElapsedTimeMillis();
        ai.clearStatus();
    }

    bool applySource(){
        dirty = false;
        const auto errors = effectShaderSource::validate(source.get());
        if(!errors.empty()){
            draftError = ofJoinString(errors, "\n");
            setShaderFailure(draftError);
            return false;
        }
        auto previousSpecs = effectParameters;
        auto previousWarnings = metadataWarnings;
        effectShaderSource::parseConfiguration(effectShaderSource::configuration(source.get()), effectParameters, metadataWarnings);
        if(!compileShaderSource(source.get())){
            effectParameters = std::move(previousSpecs);
            metadataWarnings = std::move(previousWarnings);
            draftError = shaderStatus.get();
            return false;
        }
        // Compilation succeeded before changing ports or discarding the old shader.
        syncEffectParameters();
        // Also create history's Clear port before preset connections are read.
        // applySource is only called during setup/update/preset loading.
        syncHistoryClearControl();
        conf = effectShaderSource::configuration(source.get());
        activeSource = source.get();
        draftError.clear();
        fileStatus.clear();
        orderPorts();
        requestCompute();
        return true;
    }

    void orderPorts(){
        std::vector<std::string> order = {"Shader Editor", "Input", "Bypass"};
        if(historyClearControlVisible) order.push_back("Clear");
        for(const auto &spec : effectParameters) order.push_back(spec.name);
        order.push_back("Output");
        getParameterGroup().reorder(order);
    }

    std::string candidateInterfaceError(const ofShader &candidate) const override {
        std::map<std::string, GLenum> expected = {
            {"modelViewProjectionMatrix", GL_FLOAT_MAT4}, {"tSource", GL_SAMPLER_2D},
            {"tSourceConnected", GL_INT}, {"uResolution", GL_FLOAT_VEC2}, {"resolution", GL_FLOAT_VEC2},
            {"uTexelSize", GL_FLOAT_VEC2}, {"uTime", GL_FLOAT}, {"time", GL_FLOAT},
            {"uFrame", GL_INT}, {"frame", GL_INT}, {"tPreviousSource", GL_SAMPLER_2D},
            {"tPreviousSourceConnected", GL_INT}, {"tPreviousOutput", GL_SAMPLER_2D},
            {"tPreviousOutputConnected", GL_INT}};
        for(const auto &spec : effectParameters){
            switch(spec.type){
                case ParameterType::Float:
                    expected[spec.name] = GL_FLOAT;
                    expected[spec.name + "Tex"] = GL_SAMPLER_2D;
                    expected[spec.name + "TexConnected"] = GL_INT;
                    break;
                case ParameterType::Bool: expected[spec.name] = GL_BOOL; break;
                case ParameterType::Color: expected[spec.name] = GL_FLOAT_VEC4; break;
                case ParameterType::Texture:
                    expected[spec.name] = GL_SAMPLER_2D;
                    expected[spec.name + "Connected"] = GL_INT;
                    break;
            }
        }
        GLint count = 0, maxLength = 0;
        glGetProgramiv(candidate.getProgram(), GL_ACTIVE_UNIFORMS, &count);
        glGetProgramiv(candidate.getProgram(), GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);
        std::vector<GLchar> name(std::max(1, maxLength));
        std::vector<std::string> errors;
        for(GLint i = 0; i < count; ++i){
            GLsizei length = 0;
            GLint size = 0;
            GLenum type = 0;
            glGetActiveUniform(candidate.getProgram(), i, static_cast<GLsizei>(name.size()), &length, &size, &type, name.data());
            const std::string key(name.data(), length);
            const auto match = expected.find(key);
            if(match == expected.end()) errors.push_back("Simple Effect cannot supply uniform " + key + ". Add a supported parameter to line 1 or use a standard uniform.");
            else if(size != 1 || type != match->second) errors.push_back("Uniform " + key + " has a type or array size incompatible with Simple Effect.");
        }
        return ofJoinString(errors, "\n");
    }

    static std::string safeName(std::string name){
        name = ofTrim(name);
        if(ofToLower(ofFilePath::getFileExt(name)) == "glsl") name = ofFilePath::getBaseName(name);
        std::string result;
        for(unsigned char c : name){
            if(std::isalnum(c) || c == '-' || c == '_') result += static_cast<char>(c);
            else if(std::isspace(c)) result += '_';
        }
        return result.empty() ? "MyShader" : result;
    }

    void saveFile(){
        // Re-check the actual draft, even with live compilation disabled. Never
        // write a stale active shader in place of the text the user is saving.
        if(!applySource()){
            ofSystemAlertDialog("Shader was not saved. Fix these deficiencies:\n\n" + draftError);
            return;
        }
        const std::string directory = ofToDataPath("Effects", true);
        const std::string name = safeName(shaderName.get());
        const std::string path = ofFilePath::join(directory, name + ".glsl");
        if(!ofDirectory::doesDirectoryExist(directory, false) && !ofDirectory::createDirectory(directory, false, true)){
            ofSystemAlertDialog("Shader was not saved. Could not create:\n" + directory);
            return;
        }
        if(!ofBufferToFile(path, ofBuffer(source.get().data(), source.get().size()), false)){
            ofSystemAlertDialog("Shader was not saved. Could not write:\n" + path);
            return;
        }
        shaderName.setWithoutEventNotifications(name);
        fileStatus = "Saved " + path + ". Restart the app to discover a new Effects node.";
        shaderStatus = fileStatus;
    }

    void loadFile(){
        auto result = ofSystemLoadDialog("Load Shader (.glsl)", false, ofToDataPath("Effects", true));
        if(!result.bSuccess) return;
        try{
            const auto loaded = effectShaderSource::readStandalone(result.getPath());
            if(ofTrim(loaded).empty()) throw std::runtime_error("The file is empty.");
            shaderName.setWithoutEventNotifications(safeName(ofFilePath::getBaseName(result.getPath())));
            source = loaded;
            applySource();
        }catch(const std::exception &error){
            ofSystemAlertDialog("Could not load shader:\n" + std::string(error.what()));
        }
    }

    void setupAssistant(){
        ofxOceanodeAIAssistant::Spec spec;
        spec.title = "Write a Simple Effect shader with Requesty";
        spec.subtitle = [](){ return std::string("Complete GLSL 410 fragment shaders; controls come from line 1."); };
        spec.generateHint = "Describe the image effect, animation, and controls you want.";
        spec.fixButton = "Fix shader";
        spec.readyMessage = "AI shader compiled";
        spec.pendingHint = "The result replaces the editor text. A failed compile keeps the previous shader running.";
        spec.systemPrompt = [](){ return std::string(R"PROMPT(You write complete fragment shaders for Oceanode's Shader and Simple Effect nodes.
Return ONLY GLSL source, without Markdown fences or prose. Always include the complete shader.
The FIRST physical line is comma-separated parameter metadata, or // if there are no custom controls:
//Amount:1:0:2, Enabled:bool:true, Tint:color:1:1:1:1, Mask:texture
Float syntax is Name[:default[:min[:max]]]. Literal min/max mean open-ended bounds.
Bool syntax is Name:bool[:true|false|0|1]. Color is Name:color[:R:G:B[:A]], components 0..1.
Texture syntax is Name:texture. Names must be unique GLSL identifiers and cannot collide with host uniforms, generated texture uniforms, or node ports Input/Output/Bypass/Clear.
After the metadata and optional comments, use #version 410, matching the host vertex shader.
Declare a matching uniform float/bool/vec4/sampler2D for every metadata parameter.
The host supplies uniform sampler2D tSource; uniform vec2 uResolution; uniform vec2 uTexelSize;
uniform float uTime; uniform int uFrame; uniform int tSourceConnected;
Legacy resolution/time/frame aliases also work. Omit unused uniforms if desired.
For every float Name, optional sampler2D NameTex and int NameTexConnected support texture control:
float value = NameTexConnected != 0 ? texture(NameTex, uv).r : Name;
For texture parameters Name, optional int NameConnected indicates a connected texture.
Only sample optional textures when their flag is nonzero. Unconnected textures are 1x1 black.
Use out vec4 out_color; void main(); and vec2 uv = gl_FragCoord.xy / uResolution.
No vertex shader, varying UVs, Shadertoy iTime/iResolution/iChannel uniforms, external includes, or uniforms the host cannot supply.
The Input texture is required and determines the output resolution. Preserve source alpha unless asked to change it.
Protect zero denominators and invalid smoothstep ranges. No ES precision/version declarations.
Optional history: sampler2D tPreviousSource and int tPreviousSourceConnected are previous notified input;
sampler2D tPreviousOutput and int tPreviousOutputConnected are previous rendered output. Do NOT put these in metadata.
History is 1x1 black until the corresponding Connected flag is nonzero; seed with tSource when zero.
Feedback advances every render. Both histories require normalized GL_TEXTURE_2D inputs.
Optional // @swap-dimensions BoolName swaps output dimensions when a metadata bool is true.
Use textureSize(tSource, 0) for source size in that case. Optional // @description and // @param comments explain the effect.
Keep existing parameter names/types when fixing unless the request requires changing them.
Here is a valid complete starting example:
)PROMPT") + initialSource(); };
        spec.buildPrompt = [this](const std::string &request, bool fix){
            return std::string(fix ? "Fix the current shader.\n" : "Create a shader for this request.\n") +
                "Request: " + request + "\n\nCurrent source:\n" + source.get() + "\n\nDiagnostics:\n" + draftError;
        };
        spec.apply = [this](const std::string &code){
            source = code;
            return applySource() ? std::string() : "AI shader needs editing: " + draftError;
        };
        ai.setup(spec);
    }

    void drawEditor(){
        const float zoom = ofxOceanodeShared::getZoomLevel();
        const auto &context = ofxOceanodeShared::getCustomRegionRenderContext();
        const float parentFont = ImGui::GetFontSize();
        const float width = context.active ? std::max(1.0f, context.width) : editorWidth * zoom;
        const float height = context.active ? std::max(1.0f, context.height)
            : (editorLines * editorFontSize + 76.0f) * zoom;
        ImGui::BeginChild("ShaderEditor", ImVec2(width, height), true, ImGuiWindowFlags_NoScrollWithMouse);
        const float base = std::max(1.0f, ImGui::GetFont()->LegacySize);
        ImGui::SetWindowFontScale(std::max(0.05f, parentFont / base));
        if(ImGui::BeginTabBar("ShaderTabs")){
            if(ImGui::BeginTabItem("Code")){
                if(ImGui::Button("Compile")) compileRequested = true;
                ImGui::SameLine();
                const std::string status = dirty ? "Edited" : !draftError.empty() ? "Error (see Inspector)" : "Compiled";
                ImGui::TextUnformatted(status.c_str());
                if(ImGui::IsItemHovered()){
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(560);
                    ImGui::TextUnformatted(draftError.empty() ? shaderStatus.get().c_str() : draftError.c_str());
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
                buffer.assign(source.get().begin(), source.get().end());
                buffer.push_back('\0');
                ImGui::PushFont(nullptr, base * (editorFontSize / 14.0f));
                const ImGuiID editorID = ImGui::GetID("##source");
                const bool changed = ImGui::InputTextMultiline("##source", buffer.data(), buffer.size(),
                    ImVec2(-1, std::max(1.0f, ImGui::GetContentRegionAvail().y)),
                    ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackResize,
                    [](ImGuiInputTextCallbackData *data){
                        if(data->EventFlag == ImGuiInputTextFlags_CallbackResize){
                            auto &buf = *static_cast<std::vector<char>*>(data->UserData);
                            buf.resize(data->BufTextLen + 1);
                            data->Buf = buf.data();
                        }
                        return 0;
                    }, &buffer);
                ImGui::PopFont();
                // Match Formula: wheel capture belongs only to an active editor.
                bool capture = ImGui::IsItemActive();
                ImGuiWindow *textWindow = nullptr;
                for(auto *child : ImGui::GetCurrentWindow()->DC.ChildWindows){
                    if(child->ChildId == editorID){
                        textWindow = child;
                        capture |= GImGui->ActiveId != 0 && GImGui->ActiveIdWindow == child;
                        break;
                    }
                }
                if(textWindow){
                    if(capture) textWindow->Flags &= ~ImGuiWindowFlags_NoScrollWithMouse;
                    else textWindow->Flags |= ImGuiWindowFlags_NoScrollWithMouse;
                }
                if(changed) source = std::string(buffer.data());
                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("AI")){
                ImGui::TextWrapped("Describe an effect or repair the current shader with the same Requesty assistant used by Formula.");
                ai.drawButton(ImVec2(70 * zoom, 0));
                ImGui::TextWrapped("%s", ai.getStatus().c_str());
                ai.drawPopup();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::EndChild();
    }

    ofParameter<std::string> source, shaderName;
    ofParameter<void> loadButton, saveButton;
    ofParameter<bool> liveCompile;
    ofParameter<int> editorLines;
    ofParameter<float> editorWidth, editorFontSize;
    customGuiRegion editorRegion;
    ofEventListeners editorListeners;
    ofxOceanodeAIAssistant ai;
    std::vector<char> buffer;
    std::string activeSource, draftError, fileStatus;
    uint64_t lastEditTime = 0;
    bool dirty = false, initialized = false, compileRequested = false;
    bool saveRequested = false, loadRequested = false;
};
