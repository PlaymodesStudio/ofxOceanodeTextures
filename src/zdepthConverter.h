#pragma once

#include "ofxOceanodeNodeModel.h"
#include "ofxOceanodeShared.h"
#include "imgui.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>

class zdepthConverter : public ofxOceanodeNodeModel {
public:
    zdepthConverter() : ofxOceanodeNodeModel("Z Depth Converter") {}

    void setup() override {
        description = "Displaces a grid mesh using input texture luminance as a height map, previews it as a closed solid, and can export the current triangulated model as STL.";
        color = ofColor(60, 120, 190);

        addParameter(input.set("Input", nullptr));
        addParameter(longestSideMm.set("Longest Side mm", 100.0f, 1.0f, 2000.0f));
        addParameter(reliefHeightMm.set("Relief mm", 12.0f, 0.0f, 500.0f));
        addParameter(baseThicknessMm.set("Base mm", 24.0f, 0.0f, 500.0f));
        addParameter(meshRes.set("Mesh Res", 64, 4, 4096));
        addParameter(showTexture.set("Show Texture", false));
        addParameter(showWireframe.set("Wireframe", false));
        addParameter(show.set("Show", false));
        addOutputParameter(output.set("Output", nullptr));

        addInspectorParameter(renderW.set("Render W", 800, 64, 2048));
        addInspectorParameter(renderH.set("Render H", 600, 64, 2048));
        addInspectorParameter(bgR.set("Bg R", 0.05f, 0.0f, 1.0f));
        addInspectorParameter(bgG.set("Bg G", 0.05f, 0.0f, 1.0f));
        addInspectorParameter(bgB.set("Bg B", 0.10f, 0.0f, 1.0f));
        addInspectorParameter(surfaceGray.set("Surface", 0.82f, 0.0f, 1.0f));
        addInspectorParameter(ambient.set("Ambient", 0.32f, 0.0f, 1.0f));
        addInspectorParameter(diffuse.set("Diffuse", 0.95f, 0.0f, 2.0f));
        addInspectorParameter(lightAzimuth.set("Light Az", 35.0f, -180.0f, 180.0f));
        addInspectorParameter(lightElevation.set("Light El", 55.0f, -89.0f, 89.0f));

        listeners.push(meshRes.newListener([this](int &){ needMeshRebuild = true; }));
        listeners.push(longestSideMm.newListener([this](float &){ needMeshRebuild = true; autoFitPending = true; }));
        listeners.push(reliefHeightMm.newListener([this](float &){ autoFitPending = true; }));
        listeners.push(baseThicknessMm.newListener([this](float &){ autoFitPending = true; }));
        listeners.push(renderW.newListener([this](int &){ needFboRealloc = true; }));
        listeners.push(renderH.newListener([this](int &){ needFboRealloc = true; }));

        setupShader();
        buildMesh();
        allocateFbo();
    }

    void update(ofEventArgs &) override {
        if(needMeshRebuild) { buildMesh(); needMeshRebuild = false; }
        if(needFboRealloc)  { allocateFbo(); needFboRealloc = false; }

        if(input.get() == nullptr || !input.get()->isAllocated()) {
            inputSize = glm::ivec2(0);
            autoFitPending = true;
            output = nullptr;
            return;
        }

        glm::ivec2 newInputSize(input.get()->getWidth(), input.get()->getHeight());
        if(newInputSize != inputSize) {
            inputSize = newInputSize;
            buildMesh();
            autoFitPending = true;
        }

        if(autoFitPending) {
            adaptViewToObject();
            autoFitPending = false;
        }

        renderScene();
        output = &renderFbo.getTexture();
    }

    void draw(ofEventArgs &) override {
        if(!show.get()) return;

        std::string title = (canvasID == "Canvas" ? "" : canvasID + "/") +
                           "Z Depth Converter " + ofToString(getNumIdentifier());

        ImGui::SetNextWindowSize(ImVec2(840, 660), ImGuiCond_FirstUseEver);
        if(ImGui::Begin(title.c_str(), (bool *)&show.get())) {
            if(ImGui::Button("Adapt View")) {
                adaptViewToObject();
            }
            ImGui::SameLine();
            if(ImGui::Button("Export STL")) {
                exportStlDialog();
            }

            glm::vec2 planSize = getPlanDimensionsMm();
            ImGui::SameLine();
            ImGui::Text("Size: %.1f x %.1f x %.1f mm", planSize.x, planSize.y, baseThicknessMm.get() + reliefHeightMm.get());

            if(!exportStatus.empty()) {
                ImGui::TextWrapped("%s", exportStatus.c_str());
            }

            ImGui::Separator();
            ImVec2 avail = ImGui::GetContentRegionAvail();
            if(avail.x < 64) avail.x = 64;
            if(avail.y < 64) avail.y = 64;
            drawViewport(avail.x, avail.y);
        }
        ImGui::End();
    }

    void deactivate() override {
        renderFbo.clear();
        output = nullptr;
    }

    void presetSave(ofJson &json) override {
        json["camAz"]   = camAzimuth;
        json["camEl"]   = camElevation;
        json["camDist"] = camDistance;
    }

    void presetRecallAfterSettingParameters(ofJson &json) override {
        if(json.contains("camAz"))   camAzimuth   = json["camAz"];
        if(json.contains("camEl"))   camElevation = json["camEl"];
        if(json.contains("camDist")) camDistance  = json["camDist"];
    }

private:
    ofParameter<ofTexture*> input;
    ofParameter<float>      longestSideMm;
    ofParameter<float>      reliefHeightMm;
    ofParameter<float>      baseThicknessMm;
    ofParameter<int>        meshRes;
    ofParameter<bool>       showTexture;
    ofParameter<bool>       showWireframe;
    ofParameter<bool>       show;
    ofParameter<ofTexture*> output;

    ofParameter<int>   renderW, renderH;
    ofParameter<float> bgR, bgG, bgB;
    ofParameter<float> surfaceGray, ambient, diffuse, lightAzimuth, lightElevation;

    ofMesh             mesh;
    ofFbo              renderFbo;
    ofShader           shader;
    ofEventListeners   listeners;

    bool  needMeshRebuild = false;
    bool  needFboRealloc  = false;
    float camAzimuth      = 30.0f;
    float camElevation    = 35.0f;
    float camDistance     = 180.0f;
    glm::ivec2 inputSize  = glm::ivec2(0);
    bool  autoFitPending  = true;
    std::string exportStatus;

    // ──────────────────────────────────────────────
    glm::vec2 getPlanDimensionsMm() const {
        float widthMm = longestSideMm.get();
        float depthMm = longestSideMm.get();
        if(inputSize.x > 0 && inputSize.y > 0) {
            if(inputSize.x >= inputSize.y) {
                depthMm = longestSideMm.get() * static_cast<float>(inputSize.y) / static_cast<float>(inputSize.x);
            } else {
                widthMm = longestSideMm.get() * static_cast<float>(inputSize.x) / static_cast<float>(inputSize.y);
            }
        }
        return glm::vec2(widthMm, depthMm);
    }

    float getBaseBottomMm() const {
        return -baseThicknessMm.get();
    }

    glm::vec3 getObjectCenterMm() const {
        return glm::vec3(0.0f, (reliefHeightMm.get() + getBaseBottomMm()) * 0.5f, 0.0f);
    }

    glm::vec3 getObjectSizeMm() const {
        glm::vec2 planSize = getPlanDimensionsMm();
        return glm::vec3(planSize.x, reliefHeightMm.get() + baseThicknessMm.get(), planSize.y);
    }

    float getObjectRadiusMm() const {
        return std::max(1.0f, glm::length(getObjectSizeMm()) * 0.5f);
    }

    void fitCameraToObject(float aspect) {
        aspect = std::max(0.1f, aspect);
        float verticalFov = glm::radians(60.0f);
        float horizontalFov = 2.0f * std::atan(std::tan(verticalFov * 0.5f) * aspect);
        float limitingFov = std::min(verticalFov, horizontalFov);
        camDistance = getObjectRadiusMm() / std::tan(limitingFov * 0.5f) * 1.15f;
        camDistance = std::max(1.0f, camDistance);
    }

    void adaptViewToObject() {
        glm::vec2 planSize = getPlanDimensionsMm();
        float aspect = std::max(0.1f, planSize.x / std::max(0.1f, planSize.y));
        int dominant = std::max(renderW.get(), renderH.get());
        if(aspect >= 1.0f) {
            renderW.set(dominant);
            renderH.set(std::max(64, (int)std::round((float)dominant / aspect)));
        } else {
            renderH.set(dominant);
            renderW.set(std::max(64, (int)std::round((float)dominant * aspect)));
        }
        fitCameraToObject((float)renderW.get() / std::max(1.0f, (float)renderH.get()));
    }

    void buildMesh() {
        int res = std::max(2, meshRes.get());
        glm::vec2 planSize = getPlanDimensionsMm();
        auto xzFromUV = [&](float u, float v) {
            return glm::vec2((u - 0.5f) * planSize.x, (v - 0.5f) * planSize.y);
        };
        auto addSolidVertex = [&](const glm::vec3 &position, const glm::vec2 &uv,
                                  const glm::vec3 &vertexNormal, float displaceMask, float topSurfaceMask) {
            mesh.addVertex(position);
            mesh.addTexCoord(uv);
            mesh.addNormal(vertexNormal);
            mesh.addColor(ofFloatColor(displaceMask, topSurfaceMask, 0.0f, 1.0f));
        };

        mesh.clear();
        mesh.setMode(OF_PRIMITIVE_TRIANGLES);

        for(int row = 0; row <= res; row++) {
            for(int col = 0; col <= res; col++) {
                float u = (float)col / (float)res;
                float v = (float)row / (float)res;
                glm::vec2 xz = xzFromUV(u, v);
                addSolidVertex(glm::vec3(xz.x, 0.0f, xz.y), glm::vec2(u, v), glm::vec3(0.0f, 1.0f, 0.0f), 1.0f, 1.0f);
            }
        }

        for(int row = 0; row < res; row++) {
            for(int col = 0; col < res; col++) {
                int tl = row * (res + 1) + col;
                int tr = tl + 1;
                int bl = tl + (res + 1);
                int br = bl + 1;
                mesh.addIndex(tl); mesh.addIndex(bl); mesh.addIndex(tr);
                mesh.addIndex(tr); mesh.addIndex(bl); mesh.addIndex(br);
            }
        }

        auto addWall = [&](int steps, bool varyU, float fixedCoord, const glm::vec3 &wallNormal) {
            int startIndex = mesh.getNumVertices();
            for(int i = 0; i <= steps; i++) {
                float t = (float)i / (float)steps;
                glm::vec2 uv = varyU ? glm::vec2(t, fixedCoord) : glm::vec2(fixedCoord, t);
                glm::vec2 xz = xzFromUV(uv.x, uv.y);
                addSolidVertex(glm::vec3(xz.x, 0.0f, xz.y), uv, wallNormal, 1.0f, 0.0f);
                addSolidVertex(glm::vec3(xz.x, 0.0f, xz.y), uv, wallNormal, 0.0f, 0.0f);
            }

            for(int i = 0; i < steps; i++) {
                int topA = startIndex + i * 2;
                int botA = topA + 1;
                int topB = topA + 2;
                int botB = topA + 3;
                mesh.addIndex(topA); mesh.addIndex(botA); mesh.addIndex(topB);
                mesh.addIndex(topB); mesh.addIndex(botA); mesh.addIndex(botB);
            }
        };

        addWall(res, true, 0.0f, glm::vec3(0.0f, 0.0f, -1.0f));
        addWall(res, true, 1.0f, glm::vec3(0.0f, 0.0f, 1.0f));
        addWall(res, false, 0.0f, glm::vec3(-1.0f, 0.0f, 0.0f));
        addWall(res, false, 1.0f, glm::vec3(1.0f, 0.0f, 0.0f));

        int bottomStart = mesh.getNumVertices();
        addSolidVertex(glm::vec3(-0.5f * planSize.x, 0.0f, -0.5f * planSize.y), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), 0.0f, 0.0f);
        addSolidVertex(glm::vec3( 0.5f * planSize.x, 0.0f, -0.5f * planSize.y), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), 0.0f, 0.0f);
        addSolidVertex(glm::vec3(-0.5f * planSize.x, 0.0f,  0.5f * planSize.y), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), 0.0f, 0.0f);
        addSolidVertex(glm::vec3( 0.5f * planSize.x, 0.0f,  0.5f * planSize.y), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), 0.0f, 0.0f);
        mesh.addIndex(bottomStart + 0); mesh.addIndex(bottomStart + 2); mesh.addIndex(bottomStart + 1);
        mesh.addIndex(bottomStart + 1); mesh.addIndex(bottomStart + 2); mesh.addIndex(bottomStart + 3);
    }

    void allocateFbo() {
        ofFbo::Settings s;
        s.width              = renderW.get();
        s.height             = renderH.get();
        s.internalformat     = GL_RGBA;
        s.useDepth           = true;
        s.textureTarget      = GL_TEXTURE_2D;
        renderFbo.allocate(s);
        renderFbo.begin();
        ofClear(0, 0, 0, 255);
        renderFbo.end();
    }

    float sampleHeightFromPixels(const ofPixels &pixels, float u, float v) const {
        if(!pixels.isAllocated() || pixels.getWidth() <= 0 || pixels.getHeight() <= 0) return 0.0f;

        int width = static_cast<int>(pixels.getWidth());
        int height = static_cast<int>(pixels.getHeight());
        float px = ofClamp(u, 0.0f, 1.0f) * (width - 1);
        float py = ofClamp(v, 0.0f, 1.0f) * (height - 1);
        int x0 = (int)std::floor(px);
        int y0 = (int)std::floor(py);
        int x1 = std::min(x0 + 1, width - 1);
        int y1 = std::min(y0 + 1, height - 1);
        float tx = px - x0;
        float ty = py - y0;

        auto luminance = [](const ofColor &c) {
            return (0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b) / 255.0f;
        };

        float h00 = luminance(pixels.getColor(x0, y0));
        float h10 = luminance(pixels.getColor(x1, y0));
        float h01 = luminance(pixels.getColor(x0, y1));
        float h11 = luminance(pixels.getColor(x1, y1));
        float hx0 = ofLerp(h00, h10, tx);
        float hx1 = ofLerp(h01, h11, tx);
        return ofLerp(hx0, hx1, ty);
    }

    ofMesh buildExportMesh(const ofPixels &pixels) const {
        ofMesh exportMesh;
        exportMesh.setMode(OF_PRIMITIVE_TRIANGLES);

        int res = std::max(2, meshRes.get());
        glm::vec2 planSize = getPlanDimensionsMm();
        float baseBottom = getBaseBottomMm();
        auto xzFromUV = [&](float u, float v) {
            return glm::vec2((u - 0.5f) * planSize.x, (v - 0.5f) * planSize.y);
        };
        auto addVertex = [&](const glm::vec3 &position) {
            exportMesh.addVertex(position);
        };
        auto topPosition = [&](const glm::vec2 &uv) {
            glm::vec2 xz = xzFromUV(uv.x, uv.y);
            float y = sampleHeightFromPixels(pixels, uv.x, uv.y) * reliefHeightMm.get();
            return glm::vec3(xz.x, y, xz.y);
        };

        for(int row = 0; row <= res; row++) {
            for(int col = 0; col <= res; col++) {
                glm::vec2 uv((float)col / (float)res, (float)row / (float)res);
                addVertex(topPosition(uv));
            }
        }

        for(int row = 0; row < res; row++) {
            for(int col = 0; col < res; col++) {
                int tl = row * (res + 1) + col;
                int tr = tl + 1;
                int bl = tl + (res + 1);
                int br = bl + 1;
                exportMesh.addIndex(tl); exportMesh.addIndex(bl); exportMesh.addIndex(tr);
                exportMesh.addIndex(tr); exportMesh.addIndex(bl); exportMesh.addIndex(br);
            }
        }

        auto addWall = [&](int steps, bool varyU, float fixedCoord) {
            int startIndex = exportMesh.getNumVertices();
            for(int i = 0; i <= steps; i++) {
                float t = (float)i / (float)steps;
                glm::vec2 uv = varyU ? glm::vec2(t, fixedCoord) : glm::vec2(fixedCoord, t);
                glm::vec3 top = topPosition(uv);
                glm::vec2 xz = xzFromUV(uv.x, uv.y);
                addVertex(top);
                addVertex(glm::vec3(xz.x, baseBottom, xz.y));
            }

            for(int i = 0; i < steps; i++) {
                int topA = startIndex + i * 2;
                int botA = topA + 1;
                int topB = topA + 2;
                int botB = topA + 3;
                exportMesh.addIndex(topA); exportMesh.addIndex(botA); exportMesh.addIndex(topB);
                exportMesh.addIndex(topB); exportMesh.addIndex(botA); exportMesh.addIndex(botB);
            }
        };

        addWall(res, true, 0.0f);
        addWall(res, true, 1.0f);
        addWall(res, false, 0.0f);
        addWall(res, false, 1.0f);

        int bottomStart = exportMesh.getNumVertices();
        addVertex(glm::vec3(-0.5f * planSize.x, baseBottom, -0.5f * planSize.y));
        addVertex(glm::vec3( 0.5f * planSize.x, baseBottom, -0.5f * planSize.y));
        addVertex(glm::vec3(-0.5f * planSize.x, baseBottom,  0.5f * planSize.y));
        addVertex(glm::vec3( 0.5f * planSize.x, baseBottom,  0.5f * planSize.y));
        exportMesh.addIndex(bottomStart + 0); exportMesh.addIndex(bottomStart + 2); exportMesh.addIndex(bottomStart + 1);
        exportMesh.addIndex(bottomStart + 1); exportMesh.addIndex(bottomStart + 2); exportMesh.addIndex(bottomStart + 3);

        return exportMesh;
    }

    bool writeBinaryStl(const std::string &path, const ofMesh &sourceMesh) const {
        const auto &vertices = sourceMesh.getVertices();
        const auto &indices = sourceMesh.getIndices();
        if(indices.size() < 3 || indices.size() % 3 != 0) return false;

        std::ofstream stream(path, std::ios::binary);
        if(!stream.is_open()) return false;

        std::array<char, 80> header = {};
        const std::string title = "ofxOceanodeTextures Z Depth Converter";
        std::copy(title.begin(), title.begin() + std::min(title.size(), header.size()), header.begin());
        stream.write(header.data(), header.size());

        uint32_t triangleCount = static_cast<uint32_t>(indices.size() / 3);
        stream.write(reinterpret_cast<const char *>(&triangleCount), sizeof(triangleCount));

        for(size_t i = 0; i < indices.size(); i += 3) {
            glm::vec3 a = vertices[indices[i + 0]];
            glm::vec3 b = vertices[indices[i + 1]];
            glm::vec3 c = vertices[indices[i + 2]];
            glm::vec3 n = glm::normalize(glm::cross(b - a, c - a));
            if(!std::isfinite(n.x) || !std::isfinite(n.y) || !std::isfinite(n.z)) n = glm::vec3(0.0f, 0.0f, 0.0f);

            auto writeVec3 = [&](const glm::vec3 &value) {
                float coords[3] = {value.x, value.y, value.z};
                stream.write(reinterpret_cast<const char *>(coords), sizeof(coords));
            };

            writeVec3(n);
            writeVec3(a);
            writeVec3(b);
            writeVec3(c);

            uint16_t attributeCount = 0;
            stream.write(reinterpret_cast<const char *>(&attributeCount), sizeof(attributeCount));
        }

        return stream.good();
    }

    void exportStlDialog() {
        if(input.get() == nullptr || !input.get()->isAllocated()) {
            exportStatus = "Export skipped: no input texture.";
            return;
        }

        ofFileDialogResult result = ofSystemSaveDialog("zdepthConverter.stl", "Export STL");
        if(!result.bSuccess) {
            exportStatus = "Export cancelled.";
            return;
        }

        std::string filePath = result.getPath();
        if(ofFilePath::getFileExt(filePath).empty()) filePath += ".stl";

        ofPixels pixels;
        input.get()->readToPixels(pixels);
        ofMesh exportMesh = buildExportMesh(pixels);
        if(writeBinaryStl(filePath, exportMesh))
            exportStatus = "Exported STL: " + ofFilePath::getFileName(filePath);
        else
            exportStatus = "Failed to export STL.";
    }

    void setupShader() {
        const std::string vert = R"(
#version 410
uniform sampler2D heightMap;
uniform float reliefHeightMm;
uniform float baseBottomMm;
uniform vec2 texelSize;
uniform vec2 planSizeMm;
uniform mat4 modelViewProjectionMatrix;
layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 texcoord;
out vec2 vUV;
out vec3 vNormal;
out float vTopSurface;
float sampleHeight(vec2 uv) {
    return clamp(dot(texture(heightMap, uv).rgb, vec3(0.2126, 0.7152, 0.0722)), 0.0, 1.0);
}
void main() {
    vUV = texcoord;
    vTopSurface = color.g;
    float height = sampleHeight(texcoord);
    if(vTopSurface > 0.5) {
        float hL = sampleHeight(clamp(texcoord - vec2(texelSize.x, 0.0), vec2(0.0), vec2(1.0)));
        float hR = sampleHeight(clamp(texcoord + vec2(texelSize.x, 0.0), vec2(0.0), vec2(1.0)));
        float hD = sampleHeight(clamp(texcoord - vec2(0.0, texelSize.y), vec2(0.0), vec2(1.0)));
        float hU = sampleHeight(clamp(texcoord + vec2(0.0, texelSize.y), vec2(0.0), vec2(1.0)));
        vec3 dX = vec3(planSizeMm.x * texelSize.x * 2.0, (hR - hL) * reliefHeightMm, 0.0);
        vec3 dZ = vec3(0.0, (hU - hD) * reliefHeightMm, planSizeMm.y * texelSize.y * 2.0);
        vNormal = normalize(cross(dZ, dX));
    } else {
        vNormal = normalize(normal);
    }
    vec4 pos = position;
    pos.y += mix(baseBottomMm, height * reliefHeightMm, color.r);
    gl_Position = modelViewProjectionMatrix * pos;
}
)";

        const std::string frag = R"(
#version 410
uniform sampler2D heightMap;
uniform bool useTexture;
uniform vec3 baseColor;
uniform vec3 lightDir;
uniform float ambientStrength;
uniform float diffuseStrength;
in vec2 vUV;
in vec3 vNormal;
in float vTopSurface;
out vec4 fragColor;
void main() {
    vec3 albedo = (useTexture && vTopSurface > 0.5) ? texture(heightMap, vUV).rgb : baseColor;
    vec3 normalDir = normalize(vNormal);
    float lambert = max(dot(normalDir, normalize(lightDir)), 0.0);
    vec3 shaded = albedo * (ambientStrength + lambert * diffuseStrength);
    fragColor = vec4(clamp(shaded, 0.0, 1.0), 1.0);
}
)";

        shader.setupShaderFromSource(GL_VERTEX_SHADER, vert);
        shader.setupShaderFromSource(GL_FRAGMENT_SHADER, frag);
        shader.bindDefaults();
        shader.linkProgram();
    }

    void renderScene() {
        if(!renderFbo.isAllocated()) return;

        glm::vec3 target = getObjectCenterMm();
        float azRad = glm::radians(camAzimuth);
        float elRad = glm::radians(camElevation);
        float dist  = std::max(0.01f, camDistance);

        glm::vec3 eye = target + glm::vec3(
            dist * std::cos(elRad) * std::sin(azRad),
            dist * std::sin(elRad),
            dist * std::cos(elRad) * std::cos(azRad)
        );

        ofCamera cam;
        cam.setPosition(eye);
        cam.lookAt(target, glm::vec3(0.0f, 1.0f, 0.0f));
        float objectRadius = getObjectRadiusMm();
        cam.setNearClip(std::max(0.1f, objectRadius * 0.01f));
        cam.setFarClip(std::max(1000.0f, dist + objectRadius * 4.0f));
        cam.setFov(60.0f);

        renderFbo.begin();
        ofClear(bgR.get() * 255.0f, bgG.get() * 255.0f, bgB.get() * 255.0f, 255);
        ofEnableDepthTest();

        cam.begin(ofRectangle(0, 0, renderFbo.getWidth(), renderFbo.getHeight()));
        shader.begin();
        glm::vec2 planSize = getPlanDimensionsMm();
        float texW = std::max(1.0f, static_cast<float>(input.get()->getWidth()));
        float texH = std::max(1.0f, static_cast<float>(input.get()->getHeight()));
        float lightAzRad = glm::radians(lightAzimuth.get());
        float lightElRad = glm::radians(lightElevation.get());
        glm::vec3 lightDir(
            std::cos(lightElRad) * std::sin(lightAzRad),
            std::sin(lightElRad),
            std::cos(lightElRad) * std::cos(lightAzRad)
        );
        shader.setUniformTexture("heightMap", *input.get(), 0);
        shader.setUniform1f("reliefHeightMm", reliefHeightMm.get());
        shader.setUniform1f("baseBottomMm", getBaseBottomMm());
        shader.setUniform2f("texelSize", 1.0f / texW, 1.0f / texH);
        shader.setUniform2f("planSizeMm", planSize.x, planSize.y);
        shader.setUniform1i("useTexture", showTexture.get() ? 1 : 0);
        shader.setUniform3f("baseColor", surfaceGray.get(), surfaceGray.get(), surfaceGray.get());
        shader.setUniform3f("lightDir", lightDir.x, lightDir.y, lightDir.z);
        shader.setUniform1f("ambientStrength", ambient.get());
        shader.setUniform1f("diffuseStrength", diffuse.get());

        if(showWireframe.get())
            mesh.drawWireframe();
        else
            mesh.draw();

        shader.end();
        cam.end();

        ofDisableDepthTest();
        renderFbo.end();
    }

    void drawViewport(float w, float h) {
        ImVec2 pos    = ImGui::GetCursorScreenPos();
        ImVec2 endPos = ImVec2(pos.x + w, pos.y + h);

        // Invisible button captures all mouse input for the 3D view
        ImGui::InvisibleButton("##zdview", ImVec2(w, h));
        bool hovered = ImGui::IsItemHovered();
        bool active  = ImGui::IsItemActive();

        // Draw rendered frame
        ImDrawList *dl = ImGui::GetWindowDrawList();
        if(renderFbo.isAllocated()) {
            ImTextureID tid = (ImTextureID)(uintptr_t)renderFbo.getTexture().getTextureData().textureID;
            float imageAspect = (float)renderFbo.getWidth() / std::max(1.0f, (float)renderFbo.getHeight());
            float drawW = w;
            float drawH = h;
            if(drawW / drawH > imageAspect) drawW = drawH * imageAspect;
            else                            drawH = drawW / imageAspect;
            ImVec2 drawPos((pos.x + endPos.x - drawW) * 0.5f, (pos.y + endPos.y - drawH) * 0.5f);
            ImVec2 drawEnd(drawPos.x + drawW, drawPos.y + drawH);
            dl->AddRectFilled(pos, endPos, IM_COL32(15, 15, 25, 255));
            dl->AddImage(tid, drawPos, drawEnd, ImVec2(0, 0), ImVec2(1, 1));
        } else {
            dl->AddRectFilled(pos, endPos, IM_COL32(15, 15, 25, 255));
            const char *msg = "No input texture";
            ImVec2 ts = ImGui::CalcTextSize(msg);
            dl->AddText(ImVec2(pos.x + (w - ts.x) * 0.5f, pos.y + (h - ts.y) * 0.5f),
                        IM_COL32(140, 140, 160, 255), msg);
        }
        dl->AddRect(pos, endPos, IM_COL32(70, 90, 120, 180));

        // Mouse drag → orbit
        if(active && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 1.0f)) {
            ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.0f);
            camAzimuth   += delta.x * 0.5f;
            camElevation -= delta.y * 0.5f;
            camElevation  = std::clamp(camElevation, -85.0f, 85.0f);
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
        }

        // Scroll wheel → zoom
        if(hovered) {
            float wheel = ImGui::GetIO().MouseWheel;
            if(wheel != 0.0f) {
                camDistance -= wheel * camDistance * 0.12f;
                camDistance  = std::max(0.05f, camDistance);
            }
        }

        // Arrow keys: Up/Down zoom, Left/Right rotate
        if(hovered || ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
            float dt = ImGui::GetIO().DeltaTime;
            if(ImGui::IsKeyDown(ImGuiKey_UpArrow))
                camDistance = std::max(0.05f, camDistance - camDistance * 1.5f * dt);
            if(ImGui::IsKeyDown(ImGuiKey_DownArrow))
                camDistance += camDistance * 1.5f * dt;
            if(ImGui::IsKeyDown(ImGuiKey_LeftArrow))
                camAzimuth -= 80.0f * dt;
            if(ImGui::IsKeyDown(ImGuiKey_RightArrow))
                camAzimuth += 80.0f * dt;
        }

        if(hovered) {
            ImGui::SetTooltip("Drag: rotate  |  Scroll / Up-Down: zoom  |  Left-Right: rotate");
        }
    }
};
