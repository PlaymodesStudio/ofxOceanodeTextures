//
//  TextureChannels.h
//  ofxOceanodeTextures
//
//  Created to split texture into separate RGBA channel outputs
//  Uses Multiple Render Targets (MRT) for optimal performance
//

#ifndef TextureChannels_h
#define TextureChannels_h

#include "ofxOceanodeNodeModel.h"

#define STRINGIFY(A) #A

class TextureChannels : public ofxOceanodeNodeModel {
public:
    TextureChannels() : ofxOceanodeNodeModel("Texture Channels"){};
    
    void setup(){
        addParameter(input.set("Texture In", nullptr));
        addOutputParameter(redOutput.set("Red", nullptr));
        addOutputParameter(greenOutput.set("Green", nullptr));
        addOutputParameter(blueOutput.set("Blue", nullptr));
        addOutputParameter(alphaOutput.set("Alpha", nullptr));
        
        string defaultVertSource =
        #include "shaders/defaultVertexShader.h"
        ;

        string fragSource =
        #include "shaders/TextureChannelsShader.h"
        ;
                                    
        shader.setupShaderFromSource(GL_VERTEX_SHADER, defaultVertSource);
        shader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragSource);
        shader.bindDefaults();
        shader.linkProgram();
    }
    
    void draw(ofEventArgs &a){
        if(input.get() != nullptr){
            int width = input.get()->getWidth();
            int height = input.get()->getHeight();
            
            // Allocate FBO with 4 color attachments if needed
            if(!fbo.isAllocated() || fbo.getWidth() != width || fbo.getHeight() != height){
                ofFbo::Settings settings;
                settings.width = width;
                settings.height = height;
                settings.internalformat = GL_RGBA32F;
                settings.maxFilter = GL_NEAREST;
                settings.minFilter = GL_NEAREST;
                settings.numColorbuffers = 4;  // 4 color attachments for MRT
                settings.useDepth = false;
                settings.useStencil = false;
                settings.textureTarget = GL_TEXTURE_2D;
                
                fbo.allocate(settings);
            }
            
            ofPushStyle();
            ofSetColor(255, 255, 255, 255);
            
            // Single shader pass renders to all 4 color attachments
            fbo.begin();
            
            // Enable writing to all 4 color attachments
            GLenum targetBuffers[] = {
                GL_COLOR_ATTACHMENT0_EXT,
                GL_COLOR_ATTACHMENT1_EXT,
                GL_COLOR_ATTACHMENT2_EXT,
                GL_COLOR_ATTACHMENT3_EXT
            };
            glDrawBuffers(4, targetBuffers);
            
            ofClear(0, 0, 0, 0);
            shader.begin();
            shader.setUniformTexture("tSource", *input.get(), 0);
            ofDrawRectangle(0, 0, width, height);
            shader.end();
            
            fbo.end();
            
            // Reset to default draw buffer
            glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
            
            ofPopStyle();
            
            // Set outputs from the 4 color attachments
            redOutput = &fbo.getTexture(0);    // Red channel
            greenOutput = &fbo.getTexture(1);  // Green channel
            blueOutput = &fbo.getTexture(2);   // Blue channel
            alphaOutput = &fbo.getTexture(3);  // Alpha channel
        }
    }
    
    void deactivate(){
        fbo.clear();
    }
    
private:
    ofShader shader;
    
    ofParameter<ofTexture*> input;
    ofParameter<ofTexture*> redOutput;
    ofParameter<ofTexture*> greenOutput;
    ofParameter<ofTexture*> blueOutput;
    ofParameter<ofTexture*> alphaOutput;
    
    ofFbo fbo;  // Single FBO with 4 color attachments
};

#endif /* TextureChannels_h */