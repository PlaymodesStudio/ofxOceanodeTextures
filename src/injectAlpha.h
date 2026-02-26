//
//  injectAlpha.h
//  Inject alpha channel from another texture or use luminance
//
//  Created for ofxOceanodeTextures
//

#ifndef injectAlpha_h
#define injectAlpha_h

#include "ofxOceanodeNodeModel.h"

#define STRINGIFY(A) #A

class injectAlpha : public ofxOceanodeNodeModel {
public:
    injectAlpha() : ofxOceanodeNodeModel("Inject Alpha"){};
    
    void setup(){
        addParameter(textureIn.set("Texture_In", nullptr));
        addParameter(alphaTexture.set("Alpha_Texture", nullptr));
        addOutputParameter(output.set("Output", nullptr));
        
        // Setup shader
        string defaultVertSource =
        #include "shaders/defaultVertexShader.h"
        ;

        string drawFragSource =
        #include "shaders/injectAlphaShader.h"
        ;
                                    
        shader.setupShaderFromSource(GL_VERTEX_SHADER, defaultVertSource);
        shader.setupShaderFromSource(GL_FRAGMENT_SHADER, drawFragSource);
        shader.bindDefaults();
        shader.linkProgram();
        
        // Initialize black texture for when no alpha texture is connected
        blackTexture.allocate(1, 1, GL_RGBA32F);
        ofPixels pix;
        pix.allocate(1, 1, OF_PIXELS_RGBA);
        pix.setColor(0, 0, ofColor(0, 0, 0, 0));
        blackTexture.loadData(pix);
    }
    
    void draw(ofEventArgs &a){
        if(textureIn.get() != nullptr){
            int width = textureIn.get()->getWidth();
            int height = textureIn.get()->getHeight();
            
            // Allocate or reallocate FBO if needed
            if(!fbo.isAllocated() || fbo.getWidth() != width || fbo.getHeight() != height){
                ofFbo::Settings settings;
                settings.width = width;
                settings.height = height;
                settings.internalformat = GL_RGBA32F;
                settings.maxFilter = GL_NEAREST;
                settings.minFilter = GL_NEAREST;
                settings.numColorbuffers = 1;
                settings.useDepth = false;
                settings.useStencil = false;
                settings.textureTarget = GL_TEXTURE_2D;
                
                fbo.allocate(settings);
                fbo.begin();
                ofClear(0, 0, 0, 0);
                fbo.end();
            }
            
            shader.begin();
            fbo.begin();
			ofClear(0, 0, 0, 0);
            ofPushStyle();
            ofSetColor(255, 255, 255, 255);
            
            // Set texture uniforms
            shader.setUniformTexture("textureIn", *textureIn.get(), 0);
            
            // Use alpha texture if connected, otherwise shader will use luminance
            if(alphaTexture.get() != nullptr && alphaTexture.get()->isAllocated()){
                shader.setUniformTexture("alphaTexture", *alphaTexture.get(), 1);
                shader.setUniform1i("hasAlphaTexture", 1);
            } else {
                shader.setUniformTexture("alphaTexture", blackTexture, 1);
                shader.setUniform1i("hasAlphaTexture", 0);
            }
            
            shader.setUniform2f("size", width, height);
            
            ofDrawRectangle(0, 0, width, height);
            ofPopStyle();
            fbo.end();
            shader.end();
            
//            // Cleanup: unbind textures
//            glActiveTexture(GL_TEXTURE0);
//            glBindTexture(GL_TEXTURE_2D, 0);
//            glActiveTexture(GL_TEXTURE1);
//            glBindTexture(GL_TEXTURE_2D, 0);
            
            output = &fbo.getTexture();
        }
    }
    
    void deactivate(){
        fbo.clear();
        output = nullptr;
    }
    
private:
    ofShader shader;
    
    ofParameter<ofTexture*> textureIn;
    ofParameter<ofTexture*> alphaTexture;
    ofParameter<ofTexture*> output;
    
    ofTexture blackTexture;
    ofFbo fbo;
};

#endif /* injectAlpha_h */
