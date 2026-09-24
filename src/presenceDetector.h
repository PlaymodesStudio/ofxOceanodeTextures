#pragma once

#include "ofMain.h"
#include "ofxOceanodeNodeModel.h"

#include <algorithm>
#include <cmath>

// Produces scalar presence data from a small GPU-downsampled camera image.
class presenceDetector : public ofxOceanodeNodeModel {
public:
    presenceDetector() : ofxOceanodeNodeModel("Presence Detector") {
        description = "Detects how much of a camera image is brighter than a pixel threshold, "
                      "or different from a captured empty background. Output is an occupancy "
                      "percentage and a debounced Present flag, not the average brightness "
                      "of only the bright pixels.\n\n"
                      "Mode 0 Lightness: bright pixels against a dark scene.\n"
                      "Mode 1 Background Difference: capture the empty scene, then compare RGB per pixel.\n"
                      "Pixel Threshold: minimum pixel lightness or RGB difference, 0..1.\n"
                      "On Area % / Off Area %: percentage of the ROI needed to enter or leave Present.\n"
                      "Capture Empty: store the current frame as the reference; keep the area empty.\n"
                      "Mean Lightness: average luminance within the ROI, 0..1.\n"
                      "Coverage %: percentage of ROI pixels above Pixel Threshold.\n"
                      "Mask: white pixels counted as foreground; black pixels ignored.\n"
                      "Inspector: normalized ROI, downsample size, and frame confirmation counts.";
    }

    void setup() override {
        addParameter(input.set("Input", nullptr));
        addParameterDropdown(mode, "Mode", 0, {"Lightness", "Background Difference"});
        addParameter(pixelThreshold.set("Pixel Threshold", 0.12f, 0.0f, 1.0f));
        addParameter(onArea.set("On Area %", 3.0f, 0.0f, 100.0f));
        addParameter(offArea.set("Off Area %", 1.5f, 0.0f, 100.0f));
        addParameter(captureEmpty.set("Capture Empty"));
        addOutputParameter(meanLightness.set("Mean Lightness", 0.0f, 0.0f, 1.0f));
        addOutputParameter(coverage.set("Coverage %", 0.0f, 0.0f, 100.0f));
        addOutputParameter(present.set("Present", false));
        addOutputParameter(mask.set("Mask", nullptr));

        addInspectorParameter(roiX.set("ROI X", 0.0f, 0.0f, 1.0f));
        addInspectorParameter(roiY.set("ROI Y", 0.0f, 0.0f, 1.0f));
        addInspectorParameter(roiWidth.set("ROI Width", 1.0f, 0.0f, 1.0f));
        addInspectorParameter(roiHeight.set("ROI Height", 1.0f, 0.0f, 1.0f));
        addInspectorParameter(sampleLongSide.set("Sample Long Side", 160, 32, 512));
        addInspectorParameter(confirmFrames.set("Confirm Frames", 2, 1, 120));
        addInspectorParameter(releaseFrames.set("Release Frames", 6, 1, 120));
        status.set("Status", "Waiting for Input");
        status.setSerializable(false);
        addInspectorParameter(status);

        listeners.push(captureEmpty.newListener([this]() { captureRequested = true; }));
        listeners.push(mode.newListener([this](int &) {
            transitionFrames = 0;
            present = false;
        }));
    }

    void draw(ofEventArgs &) override {
        ofTexture* source = input.get();
        if(source == nullptr || !source->isAllocated() || source->getWidth() <= 0 || source->getHeight() <= 0) {
            clearInput();
            return;
        }

        const int sourceWidth = static_cast<int>(source->getWidth());
        const int sourceHeight = static_cast<int>(source->getHeight());
        const int longSide = std::max(sourceWidth, sourceHeight);
        const int sampleWidth = std::max(1, static_cast<int>(std::lround(
            static_cast<double>(sampleLongSide.get()) * sourceWidth / longSide)));
        const int sampleHeight = std::max(1, static_cast<int>(std::lround(
            static_cast<double>(sampleLongSide.get()) * sourceHeight / longSide)));

        if(!sampleFbo.isAllocated() || sampleFbo.getWidth() != sampleWidth ||
           sampleFbo.getHeight() != sampleHeight ||
           previousSourceWidth != sourceWidth || previousSourceHeight != sourceHeight) {
            ofFbo::Settings settings;
            settings.width = sampleWidth;
            settings.height = sampleHeight;
            settings.internalformat = GL_RGBA8;
            settings.textureTarget = GL_TEXTURE_2D;
            settings.minFilter = GL_LINEAR;
            settings.maxFilter = GL_LINEAR;
            settings.numColorbuffers = 1;
            settings.numSamples = 0;
            settings.useDepth = false;
            settings.useStencil = false;
            sampleFbo.allocate(settings);
            reference.clear();
            maskTexture.clear();
            transitionFrames = 0;
            present = false;
            previousSourceWidth = sourceWidth;
            previousSourceHeight = sourceHeight;
        }
        if(!sampleFbo.isAllocated()) {
            status = "ERROR: Could not allocate sample image";
            clearMeasurements();
            return;
        }

        sampleFbo.begin();
        ofClear(0, 0, 0, 0);
        ofPushStyle();
        ofDisableAlphaBlending();
        ofFill();
        ofSetColor(255);
        source->draw(0, 0, sampleWidth, sampleHeight);
        ofPopStyle();
        sampleFbo.end();

        ofPixels pixels;
        sampleFbo.readToPixels(pixels);
        if(pixels.getNumChannels() < 3 || pixels.getWidth() != sampleWidth || pixels.getHeight() != sampleHeight) {
            status = "ERROR: Could not read sample pixels";
            clearMeasurements();
            return;
        }

        if(captureRequested) {
            reference = pixels;
            captureRequested = false;
            transitionFrames = 0;
            present = false;
        }

        ofPixels maskPixels;
        maskPixels.allocate(sampleWidth, sampleHeight, OF_PIXELS_RGBA);
        const bool differenceMode = mode.get() == 1;
        const bool referenceReady = reference.isAllocated() &&
                                    reference.getWidth() == sampleWidth &&
                                    reference.getHeight() == sampleHeight;
        const float threshold = pixelThreshold.get();
        const float x0 = roiX.get();
        const float y0 = roiY.get();
        const float x1 = std::min(1.0f, x0 + roiWidth.get());
        const float y1 = std::min(1.0f, y0 + roiHeight.get());
        float luminanceSum = 0.0f;
        int roiPixelCount = 0;
        int qualifyingPixelCount = 0;

        for(int y = 0; y < sampleHeight; ++y) {
            for(int x = 0; x < sampleWidth; ++x) {
                const float u = (x + 0.5f) / sampleWidth;
                const float v = (y + 0.5f) / sampleHeight;
                const bool inside = u >= x0 && u < x1 && v >= y0 && v < y1;
                bool qualifies = false;
                if(inside) {
                    const ofColor colour = pixels.getColor(x, y);
                    const float lightness = (0.2126f * colour.r + 0.7152f * colour.g +
                                             0.0722f * colour.b) / 255.0f;
                    luminanceSum += lightness;
                    ++roiPixelCount;
                    if(differenceMode) {
                        if(referenceReady) {
                            const ofColor background = reference.getColor(x, y);
                            const int difference = std::max({
                                std::abs(int(colour.r) - int(background.r)),
                                std::abs(int(colour.g) - int(background.g)),
                                std::abs(int(colour.b) - int(background.b))
                            });
                            qualifies = difference / 255.0f > threshold;
                        }
                    }else{
                        qualifies = lightness > threshold;
                    }
                    if(qualifies) ++qualifyingPixelCount;
                }
                maskPixels.setColor(x, y, qualifies ? ofColor::white : ofColor::black);
            }
        }

        if(!maskTexture.isAllocated() || maskTexture.getWidth() != sampleWidth ||
           maskTexture.getHeight() != sampleHeight) {
            maskTexture.allocate(sampleWidth, sampleHeight, GL_RGBA8, false);
            maskTexture.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
        }
        if(maskTexture.isAllocated()) {
            maskTexture.loadData(maskPixels);
            mask = &maskTexture;
        }else{
            mask = nullptr;
        }

        meanLightness = roiPixelCount > 0 ? luminanceSum / roiPixelCount : 0.0f;
        coverage = roiPixelCount > 0 ? 100.0f * qualifyingPixelCount / roiPixelCount : 0.0f;

        if(roiPixelCount == 0 || (differenceMode && !referenceReady)) {
            present = false;
            transitionFrames = 0;
            status = roiPixelCount == 0 ? "Empty ROI" : "Capture Empty with nobody in view";
            return;
        }

        const float enter = onArea.get();
        const float leave = std::min(offArea.get(), enter);
        const bool candidate = present.get() ? coverage.get() >= leave : coverage.get() >= enter;
        if(candidate == present.get()) {
            transitionFrames = 0;
        }else if(++transitionFrames >= (candidate ? confirmFrames.get() : releaseFrames.get())) {
            present = candidate;
            transitionFrames = 0;
        }
        status = "Ready";
    }

    void deactivate() override {
        sampleFbo.clear();
        maskTexture.clear();
        reference.clear();
        captureRequested = false;
        previousSourceWidth = 0;
        previousSourceHeight = 0;
        clearMeasurements();
    }

private:
    void clearMeasurements() {
        meanLightness = 0.0f;
        coverage = 0.0f;
        present = false;
        mask = nullptr;
        transitionFrames = 0;
    }

    void clearInput() {
        captureRequested = false;
        if(previousSourceWidth != 0 || previousSourceHeight != 0) {
            reference.clear();
            sampleFbo.clear();
            maskTexture.clear();
            previousSourceWidth = 0;
            previousSourceHeight = 0;
        }
        clearMeasurements();
        status = "Waiting for Input";
    }

    ofParameter<ofTexture*> input;
    ofParameter<int> mode;
    ofParameter<float> pixelThreshold;
    ofParameter<float> onArea;
    ofParameter<float> offArea;
    ofParameter<void> captureEmpty;
    ofParameter<float> meanLightness;
    ofParameter<float> coverage;
    ofParameter<bool> present;
    ofParameter<ofTexture*> mask;
    ofParameter<float> roiX;
    ofParameter<float> roiY;
    ofParameter<float> roiWidth;
    ofParameter<float> roiHeight;
    ofParameter<int> sampleLongSide;
    ofParameter<int> confirmFrames;
    ofParameter<int> releaseFrames;
    ofParameter<std::string> status;
    ofEventListeners listeners;
    ofFbo sampleFbo;
    ofTexture maskTexture;
    ofPixels reference;
    bool captureRequested = false;
    int transitionFrames = 0;
    int previousSourceWidth = 0;
    int previousSourceHeight = 0;
};
