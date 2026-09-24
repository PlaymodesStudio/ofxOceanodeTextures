#pragma once

#include "ofMain.h"
#include "ofxOceanodeNodeModel.h"

// A patchable one-iteration boundary. Output is published only from update();
// Return notifications copy into the other FBO without changing Output.
class textureFeedbackDelay : public ofxOceanodeNodeModel {
public:
    textureFeedbackDelay() : ofxOceanodeNodeModel("Texture Feedback Delay") {
        description = "A one-iteration texture delay for patchable feedback. Connect an initial image to Seed, "
                      "send Output through any processing nodes, and connect the final result to Return. "
                      "The returned image is published on the next update, never during its Return callback.\n\n"
                      "Run: advance once per application update. Turn off to hold the displayed frame.\n"
                      "Step: while paused, process the displayed frame once; press again to show its returned result.\n"
                      "Reset: discard the loop and seed again on the next update.\n"
                      "Iteration: number of returned images published since reset.\n"
                      "Inputs must be GL_TEXTURE_2D; use Texture Resizer for other texture targets.";
    }

    void setup() override {
        addParameter(seed.set("Seed", nullptr));
        addParameter(returnTexture.set("Return", nullptr));
        addParameter(run.set("Run", true));
        addParameter(step.set("Step"));
        addParameter(reset.set("Reset"));
        addOutputParameter(output.set("Output", nullptr));

        iteration.set("Iteration", 0);
        iteration.setSerializable(false);
        addInspectorParameter(iteration);
        status.set("Status", "Starting");
        status.setSerializable(false);
        addInspectorParameter(status);

        listeners.push(returnTexture.newListener([this](ofTexture* &texture) {
            // This callback may run synchronously inside an Output notification.
            // It must never publish Output or recurse into the processing chain.
            if(!acceptReturn) return;
            if(texture == nullptr || !texture->isAllocated()) {
                pendingValid = false;
                return;
            }
            if(copyTexture(*texture, frames[1 - frontIndex])) {
                pendingValid = true; // Last returned image in this iteration wins.
            }else{
                pendingValid = false;
            }
        }));
        listeners.push(step.newListener([this]() { stepRequested = true; }));
        listeners.push(reset.newListener([this]() { resetRequested = true; }));

        const std::string vertexSource =
        #include "shaders/defaultVertexShader.h"
        ;
        const std::string fragmentSource = R"(#version 410
uniform sampler2D tSource;
out vec4 out_color;
void main(){ out_color = texelFetch(tSource, ivec2(gl_FragCoord.xy), 0); }
)";
        if(!copyShader.setupShaderFromSource(GL_VERTEX_SHADER, vertexSource) ||
           !copyShader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragmentSource) ||
           !copyShader.bindDefaults()) {
            status = "ERROR: Could not compile the texture copy shader";
            return;
        }
        copyShader.linkProgram();
        GLint linked = GL_FALSE;
        glGetProgramiv(copyShader.getProgram(), GL_LINK_STATUS, &linked);
        if(linked != GL_TRUE) {
            status = "ERROR: Could not link the texture copy shader";
            return;
        }
        copyReady = true;
        status = "Waiting for Seed";
    }

    void update(ofEventArgs &) override {
        if(resetRequested) {
            resetState();
            resetRequested = false;
        }
        if(!copyReady) return;

        if(!frontValid) {
            ofTexture* source = seed.get();
            if(source == nullptr || !source->isAllocated()) {
                status = "Waiting for Seed";
                return;
            }
            if(!copyTexture(*source, frames[frontIndex])) return;
            frontValid = true;
            status = "Seeded";
        }else if(run || stepRequested) {
            if(pendingValid) {
                frontIndex = 1 - frontIndex;
                pendingValid = false;
                iteration = iteration.get() + 1;
            }
        }else{
            return; // Paused: keep the published texture and do not send a new event.
        }

        const bool advancing = run || stepRequested;
        stepRequested = false;
        publishedTexture = frames[frontIndex].getTexture();
        acceptReturn = advancing;
        status = advancing ? "Running" : "Paused";
        output = &publishedTexture; // Starts at most one new loop iteration here.
    }

    void deactivate() override {
        resetState();
    }

private:
    void resetState() {
        acceptReturn = false;
        pendingValid = false;
        frontValid = false;
        frontIndex = 0;
        stepRequested = false;
        publishedTexture.clear();
        frames[0].clear();
        frames[1].clear();
        iteration = 0;
        output = nullptr;
        status = "Waiting for Seed";
    }

    bool copyTexture(const ofTexture &source, ofFbo &target) {
        if(source.getTextureData().textureTarget != GL_TEXTURE_2D ||
           source.getWidth() <= 0 || source.getHeight() <= 0) {
            status = "ERROR: Seed and Return require GL_TEXTURE_2D images";
            return false;
        }
        if(!target.isAllocated() || target.getWidth() != source.getWidth() ||
           target.getHeight() != source.getHeight()) {
            ofFbo::Settings settings;
            settings.width = source.getWidth();
            settings.height = source.getHeight();
            settings.internalformat = GL_RGBA32F;
            settings.textureTarget = GL_TEXTURE_2D;
            settings.minFilter = GL_NEAREST;
            settings.maxFilter = GL_NEAREST;
            settings.numColorbuffers = 1;
            settings.useDepth = false;
            settings.useStencil = false;
            target.allocate(settings);
        }
        if(!target.isAllocated()) {
            status = "ERROR: Could not allocate feedback texture";
            return false;
        }
        if(source.getTextureData().textureID == target.getTexture().getTextureData().textureID) {
            status = "ERROR: Return cannot be copied onto itself";
            return false;
        }

        target.begin();
        ofPushStyle();
        ofDisableAlphaBlending();
        ofFill();
        ofSetColor(255);
        copyShader.begin();
        copyShader.setUniformTexture("tSource", source, 0);
        ofDrawRectangle(0, 0, source.getWidth(), source.getHeight());
        copyShader.end();
        ofPopStyle();
        target.end();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        return true;
    }

    ofParameter<ofTexture*> seed;
    ofParameter<ofTexture*> returnTexture;
    ofParameter<ofTexture*> output;
    ofParameter<bool> run;
    ofParameter<void> step;
    ofParameter<void> reset;
    ofParameter<int> iteration;
    ofParameter<std::string> status;
    ofEventListeners listeners;

    ofFbo frames[2];
    ofTexture publishedTexture; // Stable Output pointer across FBO swaps.
    ofShader copyShader;
    bool copyReady = false;
    bool frontValid = false;
    bool pendingValid = false;
    bool acceptReturn = false;
    bool stepRequested = false;
    bool resetRequested = false;
    int frontIndex = 0;
};
