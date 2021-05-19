//
//  noiseTexture.h
//  example
//
//  Created by Eduard Frigola Bagué on 25/02/2021.
//

#ifndef noiseTexture_h
#define noiseTexture_h

#include "ofxOceanodeNodeModel.h"
#include "imgui.h"

#define STRINGIFY(A) #A

class noiseTexture : public ofxOceanodeNodeModel {
public:
    noiseTexture() : ofxOceanodeNodeModel("Noise Texture"){};
    
    void setup(){
        addParameterDropdown(noiseType, "Type", 0, {"Perlin", "Voronoi", "Gradient", "Value", "Cellular", "Meatballs", "Fbm"});
//        addParameter(reload.set("Reload"));
        addParameter(width.set("Width", 100, 1, 50000));
        addParameter(height.set("Height", 100, 1, 50000));
        addParameter(scaleX.set("ScaleX", 1, 0, 50));
        addParameter(scaleY.set("ScaleY", 1, 0, 50));
        addParameter(warping.set("Warp", 1, 1, 10));
        addParameter(modulator.set("Mod", 0, 0, 1));
        addParameter(value.set("f", 0, -1000, 1000));
        addOutputParameter(output.set("Output", nullptr));
        
        string defaultVertSource =
        #include "defaultVertexShader.h"
        ;

        string drawFragSource =
        #include "noiseFragShader.h"
        ;
                                    
        shader.setupShaderFromSource(GL_VERTEX_SHADER, defaultVertSource);
        shader.setupShaderFromSource(GL_FRAGMENT_SHADER, drawFragSource);
//        shader.setupShaderFromFile(GL_FRAGMENT_SHADER, "noiseFragShader.glsl");
        shader.bindDefaults();
        shader.linkProgram();
        
        listener = reload.newListener([this](){
            string defaultVertSource =
            #include "defaultVertexShader.h"
            ;

//            string drawFragSource =
//            #include "noiseFragShader.h"
//            ;
                                        
            shader.setupShaderFromSource(GL_VERTEX_SHADER, defaultVertSource);
            shader.setupShaderFromFile(GL_FRAGMENT_SHADER, "noiseFragShader.glsl");
            shader.bindDefaults();
            shader.linkProgram();
        });
    }
    
    void draw(ofEventArgs &a){
        ofPushStyle();
        ofSetColor(255, 255, 255, 255);
        if(!fbo.isAllocated() || fbo.getWidth() != width || fbo.getHeight() != height){
            ofFbo::Settings fboSettings;
            fboSettings.width = width;
            fboSettings.height = height;
            fboSettings.internalformat = GL_RGBA;
            //fboSettings.numSamples = 4;
            fboSettings.useDepth = false;
            fboSettings.useStencil = false;
            fboSettings.textureTarget = GL_TEXTURE_2D;
            fboSettings.maxFilter = GL_NEAREST;
            fboSettings.minFilter = GL_NEAREST;
            //fboSettings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
            //fboSettings.wrapModeVertical = GL_CLAMP_TO_EDGE;
            fbo.allocate(fboSettings);
        }

        fbo.begin();
        shader.begin();
        //shader.setUniformTexture("base", baseFbo.getTexture(), 1);
        //shader.setUniformTexture("blendTgt", *up, 2);
        //shader.setUniform1i("mode", blendmodes[i]);
        shader.setUniform1i("type", noiseType);
        shader.setUniform2f("size", width, height);
        shader.setUniform1f("f", value);
        shader.setUniform2f("scale", scaleX, scaleY);
        shader.setUniform1f("warping", warping);
        shader.setUniform1f("modulator", modulator);
        ofDrawRectangle(0, 0, width, height);
        shader.end();
        fbo.end();


        output = &fbo.getTexture();
//        GLenum err;
//        while ((err = glGetError()) != GL_NO_ERROR) {
//            ofLog() << "OpenGL error: " << err;
//        }
        ofPopStyle();
    }
    
private:
    ofShader shader;
    
    ofParameter<ofTexture*> output;
    ofParameter<int> width, height;
    ofParameter<int> noiseType;
    ofParameter<void> reload;
    ofParameter<float> value, scaleX, scaleY, modulator, warping;
    
    ofEventListener listener;
    
    ofFbo fbo;
};


#endif /* noiseTexture_h */
