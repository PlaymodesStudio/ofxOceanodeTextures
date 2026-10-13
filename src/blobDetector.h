#pragma once

#include "ofMain.h"
#include "ofxOceanodeNodeModel.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

class blobDetector : public ofxOceanodeNodeModel {
public:
    blobDetector() : ofxOceanodeNodeModel("blobDetector") {}

    void setup() override {
        description = "Thresholds a texture, detects blobs, sorts them by area, and outputs normalized blob metrics plus debug textures.";

        addParameter(textureIn.set("Texture In", nullptr));
        addParameter(threshold.set("Threshold", 0.5f, 0.0f, 1.0f));
        addParameter(minArea.set("Min Area", 0.001f, 0.0f, 1.0f));
        addParameter(blobOutIdx.set("Blob Out Idx", {0}, {0}, {INT_MAX}));

        addOutputParameter(bboxCtdX.set("bbox_ctd_x", {0.0f}, {0.0f}, {1.0f}));
        addOutputParameter(bboxCtdY.set("bbox_ctd_y", {0.0f}, {0.0f}, {1.0f}));
        addOutputParameter(areas.set("areas", {0.0f}, {0.0f}, {1.0f}));
        addOutputParameter(bboxHeight.set("bbox_height", {0.0f}, {0.0f}, {1.0f}));
        addOutputParameter(bboxWidth.set("bbox_width", {0.0f}, {0.0f}, {1.0f}));

        addOutputParameter(binOut.set("binOut", nullptr));
        addOutputParameter(blobOut.set("blobOut", nullptr));
        addOutputParameter(bboxDisplay.set("bboxDisplay", nullptr));
    }

    void draw(ofEventArgs &) override {
        ofTexture *source = textureIn.get();
        if(source == nullptr || !source->isAllocated() || source->getWidth() <= 0 || source->getHeight() <= 0) {
            clearOutputs();
            return;
        }

        const int width = static_cast<int>(source->getWidth());
        const int height = static_cast<int>(source->getHeight());
        ensureAllocation(width, height);

        source->readToPixels(sourcePixels);
        if(!sourcePixels.isAllocated() || sourcePixels.getWidth() != width || sourcePixels.getHeight() != height) {
            clearOutputs();
            return;
        }

        buildBinaryMask(width, height);
        detectBlobs(width, height);
        updateNumericOutputs(width, height);
        updateBinaryTexture(width, height);
        updateBlobTexture(width, height);
        updateBboxDisplay(width, height);

        binOut = &binTexture;
        blobOut = &blobTexture;
        bboxDisplay = &bboxDisplayFbo.getTexture();
    }

    void deactivate() override {
        clearOutputs();
        sourcePixels.clear();
        binaryMask.clear();
        labels.clear();
        binPixels.clear();
        blobPixels.clear();
        recognizedBlobs.clear();
        binTexture.clear();
        blobTexture.clear();
        bboxDisplayFbo.clear();
        allocatedSize = {0, 0};
    }

private:
    struct BlobData {
        int areaPixels = 0;
        int minX = 0;
        int maxX = 0;
        int minY = 0;
        int maxY = 0;
        double sumX = 0.0;
        double sumY = 0.0;
        std::vector<int> pixels;

        float areaNorm(int totalPixels) const {
            return totalPixels > 0 ? static_cast<float>(areaPixels) / static_cast<float>(totalPixels) : 0.0f;
        }

        float bboxCenterXNorm(int width) const {
            return width > 0 ? (static_cast<float>(minX + maxX + 1) * 0.5f) / static_cast<float>(width) : 0.0f;
        }

        float bboxCenterYNorm(int height) const {
            return height > 0 ? (static_cast<float>(minY + maxY + 1) * 0.5f) / static_cast<float>(height) : 0.0f;
        }

        float bboxWidthNorm(int width) const {
            return width > 0 ? static_cast<float>(maxX - minX + 1) / static_cast<float>(width) : 0.0f;
        }

        float bboxHeightNorm(int height) const {
            return height > 0 ? static_cast<float>(maxY - minY + 1) / static_cast<float>(height) : 0.0f;
        }

        glm::vec2 centroidPixels() const {
            if(areaPixels <= 0) return {0.0f, 0.0f};
            return {
                static_cast<float>(sumX / static_cast<double>(areaPixels)),
                static_cast<float>(sumY / static_cast<double>(areaPixels))
            };
        }
    };

    ofParameter<ofTexture*> textureIn;
    ofParameter<float> threshold;
    ofParameter<float> minArea;
    ofParameter<std::vector<int>> blobOutIdx;

    ofParameter<std::vector<float>> bboxCtdX;
    ofParameter<std::vector<float>> bboxCtdY;
    ofParameter<std::vector<float>> areas;
    ofParameter<std::vector<float>> bboxHeight;
    ofParameter<std::vector<float>> bboxWidth;
    ofParameter<ofTexture*> binOut;
    ofParameter<ofTexture*> blobOut;
    ofParameter<ofTexture*> bboxDisplay;

    ofPixels sourcePixels;
    ofPixels binPixels;
    ofPixels blobPixels;
    ofTexture binTexture;
    ofTexture blobTexture;
    ofFbo bboxDisplayFbo;

    std::vector<unsigned char> binaryMask;
    std::vector<int> labels;
    std::vector<BlobData> recognizedBlobs;
    glm::ivec2 allocatedSize = {0, 0};

    void clearOutputs() {
        bboxCtdX = {};
        bboxCtdY = {};
        areas = {};
        bboxHeight = {};
        bboxWidth = {};
        binOut = nullptr;
        blobOut = nullptr;
        bboxDisplay = nullptr;
    }

    void ensureAllocation(int width, int height) {
        if(allocatedSize.x == width && allocatedSize.y == height && bboxDisplayFbo.isAllocated()) return;

        allocatedSize = {width, height};

        binPixels.allocate(width, height, OF_PIXELS_RGBA);
        blobPixels.allocate(width, height, OF_PIXELS_RGBA);
        binTexture.allocate(width, height, GL_RGBA);
        blobTexture.allocate(width, height, GL_RGBA);

        ofFbo::Settings settings;
        settings.width = width;
        settings.height = height;
        settings.internalformat = GL_RGBA;
        settings.useDepth = false;
        settings.useStencil = false;
        settings.textureTarget = GL_TEXTURE_2D;
        settings.minFilter = GL_NEAREST;
        settings.maxFilter = GL_NEAREST;
        settings.numColorbuffers = 1;
        bboxDisplayFbo.allocate(settings);
        bboxDisplayFbo.begin();
        ofClear(0, 0, 0, 255);
        bboxDisplayFbo.end();
    }

    void buildBinaryMask(int width, int height) {
        const int numPixels = width * height;
        binaryMask.assign(numPixels, 0);

        const unsigned char *src = sourcePixels.getData();
        unsigned char *dst = binPixels.getData();
        const int channels = sourcePixels.getNumChannels();
        const float threshold255 = ofClamp(threshold.get(), 0.0f, 1.0f) * 255.0f;

        for(int i = 0; i < numPixels; i++) {
            const int base = i * channels;

            unsigned char r = 0;
            unsigned char g = 0;
            unsigned char b = 0;
            unsigned char a = 255;

            if(channels == 1) {
                r = g = b = src[base];
            } else if(channels == 2) {
                r = g = b = src[base];
                a = src[base + 1];
            } else if(channels >= 3) {
                r = src[base];
                g = src[base + 1];
                b = src[base + 2];
                if(channels >= 4) a = src[base + 3];
            }

            const float luminance = 0.2126f * static_cast<float>(r)
                                  + 0.7152f * static_cast<float>(g)
                                  + 0.0722f * static_cast<float>(b);
            const bool isWhite = a > 0 && luminance >= threshold255;
            const unsigned char value = isWhite ? 255 : 0;

            binaryMask[i] = isWhite ? 1 : 0;

            const int dstBase = i * 4;
            dst[dstBase + 0] = value;
            dst[dstBase + 1] = value;
            dst[dstBase + 2] = value;
            dst[dstBase + 3] = 255;
        }
    }

    void detectBlobs(int width, int height) {
        recognizedBlobs.clear();

        const int numPixels = width * height;
        labels.assign(numPixels, -1);
        std::vector<int> stack;
        std::vector<int> componentPixels;
        stack.reserve(1024);
        componentPixels.reserve(1024);

        int label = 0;
        for(int y = 0; y < height; y++) {
            for(int x = 0; x < width; x++) {
                const int startIndex = y * width + x;
                if(binaryMask[startIndex] == 0 || labels[startIndex] != -1) continue;

                BlobData blob;
                blob.minX = blob.maxX = x;
                blob.minY = blob.maxY = y;

                stack.clear();
                componentPixels.clear();
                stack.push_back(startIndex);
                labels[startIndex] = label;

                while(!stack.empty()) {
                    const int index = stack.back();
                    stack.pop_back();
                    componentPixels.push_back(index);

                    const int px = index % width;
                    const int py = index / width;

                    blob.areaPixels++;
                    blob.sumX += static_cast<double>(px) + 0.5;
                    blob.sumY += static_cast<double>(py) + 0.5;
                    blob.minX = std::min(blob.minX, px);
                    blob.maxX = std::max(blob.maxX, px);
                    blob.minY = std::min(blob.minY, py);
                    blob.maxY = std::max(blob.maxY, py);

                    for(int oy = -1; oy <= 1; oy++) {
                        for(int ox = -1; ox <= 1; ox++) {
                            if(ox == 0 && oy == 0) continue;

                            const int nx = px + ox;
                            const int ny = py + oy;
                            if(nx < 0 || nx >= width || ny < 0 || ny >= height) continue;

                            const int neighborIndex = ny * width + nx;
                            if(binaryMask[neighborIndex] == 0 || labels[neighborIndex] != -1) continue;

                            labels[neighborIndex] = label;
                            stack.push_back(neighborIndex);
                        }
                    }
                }

                blob.pixels = componentPixels;
                if(blob.areaNorm(numPixels) >= minArea.get()) recognizedBlobs.push_back(std::move(blob));
                label++;
            }
        }

        std::sort(recognizedBlobs.begin(), recognizedBlobs.end(), [](const BlobData &a, const BlobData &b) {
            if(a.areaPixels != b.areaPixels) return a.areaPixels > b.areaPixels;
            if(a.minY != b.minY) return a.minY < b.minY;
            return a.minX < b.minX;
        });
    }

    void updateNumericOutputs(int width, int height) {
        std::vector<float> outBboxCtdX;
        std::vector<float> outBboxCtdY;
        std::vector<float> outAreas;
        std::vector<float> outBboxHeight;
        std::vector<float> outBboxWidth;

        outBboxCtdX.reserve(recognizedBlobs.size());
        outBboxCtdY.reserve(recognizedBlobs.size());
        outAreas.reserve(recognizedBlobs.size());
        outBboxHeight.reserve(recognizedBlobs.size());
        outBboxWidth.reserve(recognizedBlobs.size());

        const int totalPixels = width * height;
        for(const BlobData &blob : recognizedBlobs) {
            outBboxCtdX.push_back(blob.bboxCenterXNorm(width));
            outBboxCtdY.push_back(blob.bboxCenterYNorm(height));
            outAreas.push_back(blob.areaNorm(totalPixels));
            outBboxHeight.push_back(blob.bboxHeightNorm(height));
            outBboxWidth.push_back(blob.bboxWidthNorm(width));
        }

        bboxCtdX = outBboxCtdX;
        bboxCtdY = outBboxCtdY;
        areas = outAreas;
        bboxHeight = outBboxHeight;
        bboxWidth = outBboxWidth;
    }

    void updateBinaryTexture(int width, int height) {
        if(width <= 0 || height <= 0) return;
        binTexture.loadData(binPixels);
    }

    void updateBlobTexture(int width, int height) {
        if(width <= 0 || height <= 0) return;

        blobPixels.set(0);
        unsigned char *dst = blobPixels.getData();

        std::unordered_set<int> requestedIndices;
        for(int index : blobOutIdx.get()) {
            if(index >= 0 && index < static_cast<int>(recognizedBlobs.size())) requestedIndices.insert(index);
        }

        for(int index : requestedIndices) {
            const BlobData &blob = recognizedBlobs[index];
            for(int pixelIndex : blob.pixels) {
                const int base = pixelIndex * 4;
                dst[base + 0] = 255;
                dst[base + 1] = 255;
                dst[base + 2] = 255;
                dst[base + 3] = 255;
            }
        }

        blobTexture.loadData(blobPixels);
    }

    void updateBboxDisplay(int width, int height) {
        bboxDisplayFbo.begin();
        ofClear(0, 0, 0, 255);
        ofSetColor(255);
        binTexture.draw(0, 0, width, height);

        ofPushStyle();
        ofSetLineWidth(2.0f);
        for(size_t i = 0; i < recognizedBlobs.size(); i++) {
            const BlobData &blob = recognizedBlobs[i];
            const ofColor color = ofColor::fromHsb(static_cast<unsigned char>((i * 43) % 255), 200, 255);
            const glm::vec2 centroid = blob.centroidPixels();
            const float areaValue = blob.areaNorm(width * height);

            ofNoFill();
            ofSetColor(color);
            ofDrawRectangle(blob.minX, blob.minY, blob.maxX - blob.minX + 1, blob.maxY - blob.minY + 1);

            ofFill();
            ofDrawCircle(centroid.x, centroid.y, 3.0f);

            const std::string label = "#" + ofToString(i)
                                    + " A:" + ofToString(areaValue, 3)
                                    + "\nC:" + ofToString(centroid.x / static_cast<float>(width), 3)
                                    + "," + ofToString(centroid.y / static_cast<float>(height), 3);
            const float labelX = ofClamp(static_cast<float>(blob.minX) + 3.0f, 0.0f, static_cast<float>(std::max(0, width - 80)));
            const float labelY = ofClamp(static_cast<float>(blob.minY) - 6.0f, 10.0f, static_cast<float>(std::max(10, height - 18)));
            ofDrawBitmapStringHighlight(label, labelX, labelY, ofColor(0, 0, 0, 180), color);
        }
        ofPopStyle();

        bboxDisplayFbo.end();
    }
};
