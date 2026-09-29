//
//  textureEaser2.h
//  Lightnet
//
//  Texture crossfader with triggered smooth transitions
//  Provides passthrough mode with on-demand eased transitions between texture changes
//

#ifndef textureEaser_h
#define textureEaser_h

#include "ofxOceanodeNodeModel.h"

class textureEaser : public ofxOceanodeNodeModel
{
public:
    textureEaser() : ofxOceanodeNodeModel("Texture Easer") {}
    
    void setup() override;
    void update(ofEventArgs &a) override;
    
    ~textureEaser() {
        if(snapshotFbo.isAllocated()) {
            snapshotFbo.clear();
        }
        if(outputFbo.isAllocated()) {
            outputFbo.clear();
        }
    }
    
    void deactivate() override{
        snapshotFbo.clear();
        outputFbo.clear();
        texOut = nullptr;
        texCaptured = nullptr;
    }
    
private:
    // Parameters
    ofParameter<ofTexture*> texIn;
    ofParameter<float> phasor;
    ofParameter<void> trigger;
    ofParameter<float> pow;
    ofParameter<float> bipow;
    ofParameter<ofTexture*> texOut;
	ofParameter<ofTexture*> texCaptured;

    // Event listeners
    ofEventListener phasorListener;
    
    // Internal state
    ofFbo snapshotFbo;      // Stores texture at trigger moment
    ofFbo outputFbo;        // Output buffer for transitions
    ofShader mixShader;     // Crossfade shader
    
    bool isTransitioning;
    bool needsSnapshot;     // Flag to capture snapshot in next update cycle
    float accumulatedPhase; // Track total phase progress since trigger
    float lastPhasor;
    float easedPhase;
    
    // Methods
    void onTrigger();
    void onPhasorChange(float &p);
    void captureSnapshot();
    void renderPassthrough();
    void renderTransition(float easedAmount);
    float customPow(float value, float powVal);
    void allocateFbos(int width, int height);
};

#endif /* textureEaser_h */
