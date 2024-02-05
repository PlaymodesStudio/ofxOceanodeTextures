
#ifndef AverageBrightness_h
#define AverageBrightness_h

#include "ofxOceanodeNodeModel.h"

#define STRINGIFY(A) #A

class AverageBrightness : public ofxOceanodeNodeModel {
public:
    AverageBrightness() : ofxOceanodeNodeModel("Average Brightness"){};
    
    void setup(){
        //addInspectorParameter(numColors.set("Num Colors", 5, 2, 10));
        addParameter(input.set("Input", nullptr));
        addParameter(avgBright.set("Avg.Bright",0,0,1));
    }
    
    void draw(ofEventArgs &a)
    {
        if(input.get() != nullptr)
        {
            if(!fbo.isAllocated() || fbo.getWidth() != input.get()->getWidth() || fbo.getHeight() != input.get()->getHeight()){
                
                ofFbo::Settings settings;
                settings.width = input.get()->getWidth();
                settings.height = input.get()->getHeight();
                settings.internalformat = GL_RGBA32F;
                settings.maxFilter = GL_NEAREST;
                settings.minFilter = GL_NEAREST;
                settings.numColorbuffers = 1;
                settings.useDepth = false;
                settings.useStencil = false;
                settings.textureTarget = GL_TEXTURE_2D;
                
                fbo.allocate(settings);
                fbo.begin();
                ofClear(0, 0, 0, 255);
                fbo.end();
            }
            
            fbo.begin();
            ofPushStyle();
            ofSetColor(255, 255, 255, 255);
            input.get()->draw(0,0, input.get()->getWidth(), input.get()->getHeight());
            ofPopStyle();
            fbo.end();
                        
            // Calculate the average brightness
            ofPixels pixels;
            fbo.readToPixels(pixels); // Read the data back to CPU
            float totalBrightness = 0.0f;
            int numChannels = pixels.getNumChannels();
            for (int i = 0; i < pixels.size(); i += numChannels)
            {
                // Assuming an RGB format
                float brightness = (pixels[i] + pixels[i + 1] + pixels[i + 2]) / 3.0f;
                totalBrightness += brightness;
            }
            avgBright = ((totalBrightness / (pixels.getWidth() * pixels.getHeight()))/255.0)*1;
        }
    }
	
	void loadBeforeConnections(ofJson &json){
		//deserializeParameter(json, numColors);
	}
    
    void deactivate(){
        fbo.clear();
    }
    
private:
    ofShader shader;
    
    ofParameter<ofTexture*> input;
    ofParameter<int> resX;
    ofParameter<int> resY;
    ofParameter<float> avgBright;

    ofFbo fbo;
    
    ofEventListener listener;
};
    

#endif /* AverageBrightness_h */
