#ifndef colorTracker_h
#define colorTracker_h

#include "ofxOceanodeNodeModel.h"
#include "ofxOceanodeShared.h"
#include "imgui.h"
#include <algorithm>
#include <functional>

class colorTracker : public ofxOceanodeNodeModel {
public:
    colorTracker() : ofxOceanodeNodeModel("colorTracker") {}

    void setup() override {
        addParameter(show.set("Show", false));
        addParameter(input.set("Texture In", nullptr));
        addParameter(trackedColor.set("Tracked Color", ofFloatColor(1.0f, 0.0f, 0.0f, 1.0f),
                                      ofFloatColor(0.0f, 0.0f, 0.0f, 0.0f),
                                      ofFloatColor(1.0f, 1.0f, 1.0f, 1.0f)));
        addParameter(hueTolerance.set("Hue Tol", 0.08f, 0.0f, 0.5f));
        addParameter(saturationTolerance.set("Sat Tol", 0.25f, 0.0f, 1.0f));
        addParameter(valueTolerance.set("Val Tol", 0.25f, 0.0f, 1.0f));
        addParameter(softness.set("Softness", 0.05f, 0.0f, 0.5f));
        addParameter(alphaBackground.set("Alpha Bg", true));
        addOutputParameter(output.set("Output", nullptr));

        addInspectorParameter(widgetWidth.set("Widget Width", 240.0f, 80.0f, 1200.0f));
        addInspectorParameter(widgetHeight.set("Widget Height", 180.0f, 40.0f, 1200.0f));

        addCustomRegion(
            ofParameter<std::function<void()>>().set("Color Tracker", [this](){ drawPickerWidget(); }),
            ofParameter<std::function<void()>>().set("Color Tracker", [this](){ drawPickerWidget(); })
        );

        setupShader();
    }

    void draw(ofEventArgs &) override {
        ofTexture *source = input.get();
        if(source == nullptr || !source->isAllocated() || source->getWidth() <= 0 || source->getHeight() <= 0) {
            output = nullptr;
        } else {
            allocateOutput(*source);
            renderMask(*source);
            output = &outputFbo.getTexture();
        }

        if(show.get()) {
            std::string title = (canvasID == "Canvas" ? "" : canvasID + "/") + "Color Tracker " + ofToString(getNumIdentifier());
            if(ImGui::Begin(title.c_str(), (bool *)&show.get())) {
                ImVec2 avail = ImGui::GetContentRegionAvail();
                if(avail.x < 120) avail.x = 120;
                if(avail.y < 90) avail.y = 90;
                drawPicker(avail.x, avail.y);
            }
            ImGui::End();
        }
    }

    void deactivate() override {
        outputFbo.clear();
        sourcePixels.clear();
        output = nullptr;
    }

private:
    ofParameter<bool> show;
    ofParameter<ofTexture*> input;
    ofParameter<ofFloatColor> trackedColor;
    ofParameter<float> hueTolerance;
    ofParameter<float> saturationTolerance;
    ofParameter<float> valueTolerance;
    ofParameter<float> softness;
    ofParameter<bool> alphaBackground;
    ofParameter<ofTexture*> output;

    ofParameter<float> widgetWidth;
    ofParameter<float> widgetHeight;

    ofFbo outputFbo;
    ofShader shader;
    ofFloatPixels sourcePixels;

    void setupShader() {
        const std::string vertSource = R"(
            #version 410
            uniform mat4 modelViewProjectionMatrix;
            layout(location = 0) in vec4 position;
            void main() {
                gl_Position = modelViewProjectionMatrix * position;
            }
        )";

        const std::string fragSource = R"(
            #version 410

            uniform sampler2D tSource;
            uniform vec3 trackedColor;
            uniform float hueTolerance;
            uniform float saturationTolerance;
            uniform float valueTolerance;
            uniform float softness;
            uniform int alphaBackground;

            out vec4 fragColor;

            vec3 rgb2hsv(vec3 c) {
                vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
                vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
                vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
                float d = q.x - min(q.w, q.y);
                float e = 1.0e-10;
                return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
            }

            float hueDistance(float a, float b) {
                float d = abs(a - b);
                return min(d, 1.0 - d);
            }

            void main() {
                ivec2 uv = ivec2(gl_FragCoord.xy);
                vec4 src = texelFetch(tSource, uv, 0);

                if(src.a <= 0.0) {
                    fragColor = vec4(0.0);
                    return;
                }

                vec3 srcHsv = rgb2hsv(src.rgb);
                vec3 targetHsv = rgb2hsv(trackedColor);

                float hNorm = hueDistance(srcHsv.x, targetHsv.x) / max(hueTolerance, 1e-5);
                float sNorm = abs(srcHsv.y - targetHsv.y) / max(saturationTolerance, 1e-5);
                float vNorm = abs(srcHsv.z - targetHsv.z) / max(valueTolerance, 1e-5);

                float normDist = max(hNorm, max(sNorm, vNorm));
                float edge = max(softness, 1e-5);
                float matchAmount = 1.0 - smoothstep(1.0 - edge, 1.0 + edge, normDist);

                float outAlpha = alphaBackground == 1 ? matchAmount * src.a : src.a;
                vec3 outRgb = vec3(matchAmount);
                fragColor = vec4(outRgb, outAlpha);
            }
        )";

        shader.setupShaderFromSource(GL_VERTEX_SHADER, vertSource);
        shader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragSource);
        shader.bindDefaults();
        shader.linkProgram();
    }

    void allocateOutput(ofTexture &source) {
        if(outputFbo.isAllocated() && outputFbo.getWidth() == source.getWidth() && outputFbo.getHeight() == source.getHeight()) return;

        ofFbo::Settings settings;
        settings.width = source.getWidth();
        settings.height = source.getHeight();
        settings.internalformat = GL_RGBA32F;
        settings.maxFilter = GL_NEAREST;
        settings.minFilter = GL_NEAREST;
        settings.numColorbuffers = 1;
        settings.useDepth = false;
        settings.useStencil = false;
        settings.textureTarget = GL_TEXTURE_2D;
        outputFbo.allocate(settings);
        outputFbo.begin();
        ofClear(0, 0, 0, 0);
        outputFbo.end();
    }

    void renderMask(ofTexture &source) {
        outputFbo.begin();
        ofClear(0, 0, 0, 0);
        ofSetColor(255, 255, 255, 255);
        shader.begin();
        shader.setUniformTexture("tSource", source, 0);
        shader.setUniform3f("trackedColor", trackedColor.get().r, trackedColor.get().g, trackedColor.get().b);
        shader.setUniform1f("hueTolerance", hueTolerance);
        shader.setUniform1f("saturationTolerance", saturationTolerance);
        shader.setUniform1f("valueTolerance", valueTolerance);
        shader.setUniform1f("softness", softness);
        shader.setUniform1i("alphaBackground", alphaBackground ? 1 : 0);
        ofDrawRectangle(0, 0, outputFbo.getWidth(), outputFbo.getHeight());
        shader.end();
        outputFbo.end();
    }

    void drawPickerWidget() {
        float zoom = ofxOceanodeShared::getZoomLevel();
        const auto &ctx = ofxOceanodeShared::getCustomRegionRenderContext();
        float w = ctx.active ? std::max(1.0f, ctx.width) : widgetWidth.get() * zoom;
        float h = ctx.active ? std::max(1.0f, ctx.height) : widgetHeight.get() * zoom;

        drawPicker(w, h);
        ImGui::Dummy(ImVec2(0, 4.0f * zoom));
    }

    void drawPicker(float targetW, float targetH) {
        ImVec2 start = ImGui::GetCursorScreenPos();
        ImDrawList *drawList = ImGui::GetWindowDrawList();

        ofTexture *source = input.get();
        bool hasTexture = source != nullptr && source->isAllocated();

        float drawW = targetW;
        float drawH = targetH;
        if(hasTexture) {
            float aspect = source->getWidth() / source->getHeight();
            float targetAspect = targetW / targetH;
            if(targetAspect > aspect) drawW = targetH * aspect;
            else drawH = targetW / aspect;
        }

        ImGui::Dummy(ImVec2(drawW, drawH));
        ImVec2 end = ImVec2(start.x + drawW, start.y + drawH);

        if(hasTexture) {
            ImTextureID tid = (ImTextureID)(uintptr_t)source->getTextureData().textureID;
            drawList->AddImage(tid, start, end, ImVec2(0, 0), ImVec2(1, 1));
            drawList->AddRect(start, end, IM_COL32(255, 255, 255, 80));

            if(ImGui::IsItemHovered()) {
                if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    ImVec2 mousePos = ImGui::GetMousePos();
                    float nx = ofClamp((mousePos.x - start.x) / drawW, 0.0f, 1.0f);
                    float ny = ofClamp((mousePos.y - start.y) / drawH, 0.0f, 1.0f);
                    sampleTrackedColor(*source, nx, ny);
                }
                ImGui::SetTooltip("Click to pick tracked color");
            }
        } else {
            drawList->AddRectFilled(start, end, IM_COL32(30, 30, 30, 255));
            drawList->AddRect(start, end, IM_COL32(80, 80, 80, 255));
            const char *label = "No texture";
            ImVec2 ts = ImGui::CalcTextSize(label);
            drawList->AddText(ImVec2(start.x + (drawW - ts.x) * 0.5f, start.y + (drawH - ts.y) * 0.5f), IM_COL32(140, 140, 140, 255), label);
        }

        ImVec2 swatchMin(end.x - 28.0f, start.y + 8.0f);
        ImVec2 swatchMax(end.x - 8.0f, start.y + 28.0f);
        ImU32 swatchColor = ImGui::ColorConvertFloat4ToU32(ImVec4(trackedColor.get().r, trackedColor.get().g, trackedColor.get().b, 1.0f));
        drawList->AddRectFilled(swatchMin, swatchMax, swatchColor);
        drawList->AddRect(swatchMin, swatchMax, IM_COL32(255, 255, 255, 180));
    }

    void sampleTrackedColor(ofTexture &source, float nx, float ny) {
        source.readToPixels(sourcePixels);
        if(sourcePixels.size() == 0) return;

        int px = ofClamp(static_cast<int>(std::round(nx * (source.getWidth() - 1))), 0, static_cast<int>(source.getWidth() - 1));
        int py = ofClamp(static_cast<int>(std::round(ny * (source.getHeight() - 1))), 0, static_cast<int>(source.getHeight() - 1));

        ofFloatColor sampled = sourcePixels.getColor(px, py);
        sampled.a = 1.0f;
        trackedColor = sampled;
    }
};

#endif /* colorTracker_h */
