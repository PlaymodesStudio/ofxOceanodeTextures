#ifndef svg2Texture_h
#define svg2Texture_h

#include "ofxOceanodeNodeModel.h"
#include "ofxSvg.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <regex>

class svg2Texture : public ofxOceanodeNodeModel {
public:
    svg2Texture() : ofxOceanodeNodeModel("SVG 2 Texture") {}

    void setup() override {
        addCustomRegion(loadButton.set("Load SVG", [this]() { drawLoadButton(); }),
                        [this]() { drawLoadButton(); });
        addParameter(filename.set("File", ""));
        addParameter(width.set("Width", 512, 1, INT_MAX));
        addParameter(height.set("Height", 512, 1, INT_MAX));
        addParameter(opacity.set("Opacity", {1.0f}, {0.0f}, {1.0f}));
        addParameter(x.set("X", {0.0f}, {-FLT_MAX}, {FLT_MAX}));
        addParameter(y.set("Y", {0.0f}, {-FLT_MAX}, {FLT_MAX}));

        addOutputParameter(output.set("Texture", nullptr));
        addOutputParameter(numObjects.set("numObjects", 0));

        addInspectorParameter(embed.set("Embed", false));

        listeners.push(filename.newListener([this](string &path) {
            loadSvg(path);
        }));
        listeners.push(width.newListener([this](int &) {
            render();
        }));
        listeners.push(height.newListener([this](int &) {
            render();
        }));
        listeners.push(opacity.newListener([this](vector<float> &) {
            render();
        }));
        listeners.push(x.newListener([this](vector<float> &) {
            render();
        }));
        listeners.push(y.newListener([this](vector<float> &) {
            render();
        }));
    }

    void presetRecallAfterSettingParameters(ofJson &) override {
        preservePresetDimensions = false;
        render();
    }

    void loadBeforeConnections(ofJson &json) override {
        preservePresetDimensions = true;
        deserializeParameter(json, width);
        deserializeParameter(json, height);
    }

    void macroSave(ofJson &json, string presetFolderPath) override {
        if (!embed || filename.get().empty()) {
            json.erase("EmbeddedFile");
            if (!externalFilePath.empty()) {
                json[filename.getEscapedName()] = externalFilePath;
            } else if (!embeddedFilePath.empty() && filename.get() == embeddedFileRelative) {
                json[filename.getEscapedName()] = embeddedFilePath;
            }
            return;
        }

        string sourcePath = resolvePath(filename.get());
        if (!ofFilePath::isAbsolute(sourcePath)) {
            sourcePath = ofFilePath::getAbsolutePath(sourcePath, true);
        }
        ofFile source(sourcePath);
        if (!source.exists()) {
            ofLogWarning("svg2Texture") << "Cannot embed SVG; file does not exist: " << sourcePath;
            return;
        }

        const string relativePath = "data/svg2texture_" + ofToString(getNumIdentifier()) + "_" +
                                    ofFilePath::getFileName(sourcePath);
        const string presetPath = ofFilePath::getAbsolutePath(presetFolderPath, true);
        const string destinationPath = presetPath + "/" + relativePath;

        if (!ofDirectory::createDirectory(presetPath + "/data", false, true) ||
            !ofFile::copyFromTo(sourcePath, destinationPath, false, true)) {
            ofLogWarning("svg2Texture") << "Cannot embed SVG at: " << destinationPath;
            return;
        }

        const bool wasEmbeddedFile = !embeddedFileRelative.empty() && filename.get() == embeddedFileRelative;
        embeddedFileRelative = relativePath;
        embeddedFilePath = destinationPath;
        if (externalFilePath.empty() || !wasEmbeddedFile) {
            externalFilePath = filename.get();
        }
        json["EmbeddedFile"] = relativePath;
        json["ExternalFile"] = externalFilePath;
        json[filename.getEscapedName()] = relativePath;
    }

    void macroLoad(ofJson &json, string presetFolderPath) override {
        embeddedFileRelative.clear();
        embeddedFilePath.clear();
        externalFilePath.clear();

        if (json.contains("ExternalFile") && json["ExternalFile"].is_string()) {
            externalFilePath = json["ExternalFile"].get<string>();
        }

        if (json.contains("EmbeddedFile") && json["EmbeddedFile"].is_string()) {
            embeddedFileRelative = json["EmbeddedFile"].get<string>();
            embeddedFilePath = ofFilePath::getAbsolutePath(
                presetFolderPath + "/" + embeddedFileRelative, true);

            // Load immediately for the pre-connection phase. The File parameter
            // is loaded again later and resolvePath() maps the saved relative
            // path to this same embedded copy.
            loadSvg(embeddedFilePath);
        }
    }

    void deactivate() override {
        fbo.clear();
        svgLoaded = false;
        output = nullptr;
        numObjects = 0;
    }

private:
    ofParameter<string> filename;
    ofParameter<int> width, height;
    ofParameter<vector<float>> opacity, x, y;
    ofParameter<ofTexture*> output;
    ofParameter<int> numObjects;
    ofParameter<bool> embed;
    customGuiRegion loadButton;

    ofxSvg svg;
    ofFbo fbo;
    ofEventListeners listeners;
    string embeddedFileRelative;
    string embeddedFilePath;
    string externalFilePath;
    bool svgLoaded = false;
    bool preservePresetDimensions = false;

    static float vectorValue(const vector<float> &values, size_t index, float fallback) {
        if (values.empty()) return fallback;
        if (values.size() == 1 || index >= values.size()) return values.front();
        return values[index];
    }

    struct CssStyle {
        string fill;
        string stroke;
    };

    static void inlineCssStyles(ofXml &document) {
        map<string, CssStyle> styles;
        const std::regex rulePattern(R"(\.([A-Za-z_][A-Za-z0-9_-]*)\s*\{([^}]*)\})");
        const std::regex propertyPattern(R"((fill|stroke)\s*:\s*([^;]+))");

        for (ofXml &styleElement : document.find("//style")) {
            const string stylesheet = styleElement.getValue();
            for (std::sregex_iterator ruleIt(stylesheet.begin(), stylesheet.end(), rulePattern), ruleEnd;
                 ruleIt != ruleEnd; ++ruleIt) {
                CssStyle &style = styles[(*ruleIt)[1].str()];
                const string declarations = (*ruleIt)[2].str();
                for (std::sregex_iterator propertyIt(declarations.begin(), declarations.end(), propertyPattern), propertyEnd;
                     propertyIt != propertyEnd; ++propertyIt) {
                    const string property = (*propertyIt)[1].str();
                    const string value = ofTrim((*propertyIt)[2].str());
                    if (property == "fill") {
                        style.fill = value;
                    } else {
                        style.stroke = value;
                    }
                }
            }
        }

        for (ofXml &element : document.find("//*[@class]")) {
            const vector<string> classNames = ofSplitString(element.getAttribute("class").getValue(), " ", true, true);
            for (const string &className : classNames) {
                const auto styleIt = styles.find(className);
                if (styleIt == styles.end()) continue;
                if (!styleIt->second.fill.empty() && !element.getAttribute("fill")) {
                    element.setAttribute("fill", styleIt->second.fill);
                }
                if (!styleIt->second.stroke.empty() && !element.getAttribute("stroke")) {
                    element.setAttribute("stroke", styleIt->second.stroke);
                }
            }
        }
    }

    string resolvePath(const string &path) const {
        if (!embeddedFileRelative.empty() && path == embeddedFileRelative && !embeddedFilePath.empty()) {
            return embeddedFilePath;
        }
        return path;
    }

    void drawLoadButton() {
        if (ImGui::Button("Load SVG")) {
            ofFileDialogResult result = ofSystemLoadDialog("Select SVG file", false);
            if (result.bSuccess) {
                const string selectedPath = result.getPath();
                if (ofToLower(ofFilePath::getFileExt(selectedPath)) == "svg") {
                    embeddedFileRelative.clear();
                    embeddedFilePath.clear();
                    externalFilePath = selectedPath;
                    filename = selectedPath;
                } else {
                    ofLogWarning("svg2Texture") << "Selected file is not an SVG: " << selectedPath;
                }
            }
        }
        if (!filename.get().empty()) {
            ImGui::SameLine();
            ImGui::TextWrapped("%s", ofFilePath::getFileName(filename.get()).c_str());
        }
    }

    void loadSvg(const string &requestedPath) {
        if (requestedPath.empty()) {
            svg = ofxSvg();
            svgLoaded = false;
            numObjects = 0;
            output = nullptr;
            return;
        }

        const string path = resolvePath(requestedPath);
        ofFile file(path);
        if (!file.exists() || ofToLower(ofFilePath::getFileExt(path)) != "svg") {
            ofLogWarning("svg2Texture") << "Cannot load SVG: " << path;
            svg = ofxSvg();
            svgLoaded = false;
            numObjects = 0;
            output = nullptr;
            return;
        }

        // Adobe Illustrator and other exporters commonly omit width/height
        // and only provide a viewBox. ofxSvg/libsvgtiny reports 0x0 for that
        // form, which would otherwise make the output texture invalid.
        ofBuffer buffer = ofBufferFromFile(path);
        string svgData = buffer.getText();
        ofXml document;
        if (document.parse(svgData)) {
            inlineCssStyles(document);
            ofXml root = document.getChild("svg");
            const ofXml::Attribute viewBoxAttribute = root.getAttribute("viewBox");
            if (root && viewBoxAttribute) {
                string viewBox = viewBoxAttribute.getValue();
                ofStringReplace(viewBox, ",", " ");
                const vector<string> values = ofSplitString(viewBox, " ", true, true);
                if (values.size() == 4) {
                    const float viewBoxWidth = ofToFloat(values[2]);
                    const float viewBoxHeight = ofToFloat(values[3]);
                    if (viewBoxWidth > 0.0f && viewBoxHeight > 0.0f) {
                        if (!root.getAttribute("width")) {
                            root.setAttribute("width", viewBoxWidth);
                        }
                        if (!root.getAttribute("height")) {
                            root.setAttribute("height", viewBoxHeight);
                        }
                        svgData = document.toString();
                    }
                }
            }
        }

        svg = ofxSvg();
        svg.loadFromString(svgData, path);
        numObjects = svg.getNumPath();
        svgLoaded = numObjects > 0;

        if (svgLoaded && !preservePresetDimensions && svg.getWidth() > 0.0f && svg.getHeight() > 0.0f) {
            width = std::max(1, static_cast<int>(std::round(svg.getWidth())));
            height = std::max(1, static_cast<int>(std::round(svg.getHeight())));
        }
        render();
    }

    void render() {
        if (width <= 0 || height <= 0) return;

        if (!fbo.isAllocated() || fbo.getWidth() != width || fbo.getHeight() != height) {
            fbo.allocate(width, height, GL_RGBA);
        }

        fbo.begin();
        ofClear(0, 0, 0, 0);
        ofEnableAlphaBlending();

        if (svgLoaded && numObjects > 0) {
            const float svgWidth = svg.getWidth();
            const float svgHeight = svg.getHeight();
            if (svgWidth <= 0.0f || svgHeight <= 0.0f) {
                ofDisableAlphaBlending();
                fbo.end();
                output = nullptr;
                return;
            }

            const float scaleX = static_cast<float>(width) / svgWidth;
            const float scaleY = static_cast<float>(height) / svgHeight;

            for (int i = 0; i < numObjects; ++i) {
                // ofxSvg exposes paths in reverse order compared with the
                // object order in the source SVG. Render and index vectors
                // in the same order users see in the SVG file.
                const int pathIndex = numObjects - 1 - i;
                const float alpha = ofClamp(vectorValue(opacity.get(), static_cast<size_t>(i), 1.0f), 0.0f, 1.0f);
                if (alpha <= 0.0f) continue;

                ofPath path = svg.getPathAt(pathIndex);
                ofFloatColor fillColor = path.getFillColor();
                ofFloatColor strokeColor = path.getStrokeColor();
                fillColor.a *= alpha;
                strokeColor.a *= alpha;
                path.setFillColor(fillColor);
                path.setStrokeColor(strokeColor);

                ofPushMatrix();
                ofTranslate(vectorValue(x.get(), static_cast<size_t>(i), 0.0f),
                            vectorValue(y.get(), static_cast<size_t>(i), 0.0f));
                ofScale(scaleX, scaleY, 1.0f);
                path.draw();
                ofPopMatrix();
            }
        }

        ofDisableAlphaBlending();
        fbo.end();
        output = numObjects > 0 ? &fbo.getTexture() : nullptr;
    }
};

#endif /* svg2Texture_h */
