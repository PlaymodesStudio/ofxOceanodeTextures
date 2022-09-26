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
        addParameter(posX.set("PosX", 0, -FLT_MAX, FLT_MAX));
        addParameter(posY.set("PosY", 0, -FLT_MAX, FLT_MAX));
        auto scaleXRef = addParameter(scaleX.set("ScaleX", 0.1, 0, 1));
		scaleXRef->addReceiveFunc<ofTexture*>([this](ofTexture *const &tex){
            scaleXTex = (ofTexture*)tex;
        });
        scaleXRef->addDisconnectFunc([this](){
            scaleXTex = nullptr;
        });
		
        auto scaleYRef = addParameter(scaleY.set("ScaleY", 0.1, 0, 1));
		scaleYRef->addReceiveFunc<ofTexture*>([this](ofTexture *const &tex){
            scaleYTex = (ofTexture*)tex;
        });
        scaleYRef->addDisconnectFunc([this](){
            scaleYTex = nullptr;
        });
		
		auto offsetXRef = addParameter(offsetX.set("OffsetX", 0, 0, 1));
		offsetXRef->addReceiveFunc<ofTexture*>([this](ofTexture *const &tex){
            offsetXTex = (ofTexture*)tex;
        });
        offsetXRef->addDisconnectFunc([this](){
            offsetXTex = nullptr;
        });
		
		auto offsetYRef = addParameter(offsetY.set("OffsetY", 0, 0, 1));
		offsetYRef->addReceiveFunc<ofTexture*>([this](ofTexture *const &tex){
            offsetYTex = (ofTexture*)tex;
        });
        offsetYRef->addDisconnectFunc([this](){
            offsetYTex = nullptr;
        });
		
        addParameter(warping.set("Warp", 1, 1, 10));
        addParameter(modulator.set("Mod", 0, 0, 1));
        addParameter(value.set("f", 0, -FLT_MAX, FLT_MAX));
		addParameter(fTex.set("f_tex", nullptr));
		
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
                                        
            shader.setupShaderFromSource(GL_VERTEX_SHADER, defaultVertSource);
            shader.setupShaderFromFile(GL_FRAGMENT_SHADER, "noiseFragShader.glsl");
            shader.bindDefaults();
            shader.linkProgram();
        });
		
//		sizeListeners.push(width.newListener([this](int &i){
//			scaleX.setMax(i);
//			offsetX.setMax(i);
//		}));
//		
//		
//		sizeListeners.push(height.newListener([this](int &i){
//			scaleY.setMax(i);
//			offsetY.setMax(i);
//		}));
		
		blackTexture.allocate(1, 1, GL_RGBA32F);
		
		scaleXTex = scaleYTex = offsetXTex = offsetYTex = fTex = nullptr;
    }
    
    void draw(ofEventArgs &a){
        ofPushStyle();
        ofSetColor(255, 255, 255, 255);
        if(!fbo.isAllocated() || fbo.getWidth() != width || fbo.getHeight() != height){
            ofFbo::Settings fboSettings;
            fboSettings.width = width;
            fboSettings.height = height;
            fboSettings.internalformat = GL_RGBA32F;
            //fboSettings.numSamples = 4;
            fboSettings.useDepth = false;
            fboSettings.useStencil = false;
            fboSettings.textureTarget = GL_TEXTURE_2D;
            fboSettings.maxFilter = GL_NEAREST;
            fboSettings.minFilter = GL_NEAREST;
			fboSettings.numColorbuffers = 1;
            //fboSettings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
            //fboSettings.wrapModeVertical = GL_CLAMP_TO_EDGE;
            fbo.allocate(fboSettings);
        }

        fbo.begin();
		ofClear(0, 0, 0, 255);
        shader.begin();
		
		shader.setUniformTexture("scaleXTex", scaleXTex != nullptr ? *scaleXTex : blackTexture, 1);
		shader.setUniformTexture("scaleYTex", scaleYTex != nullptr ? *scaleYTex : blackTexture, 2);
		shader.setUniformTexture("offsetXTex", offsetXTex != nullptr ? *offsetXTex : blackTexture, 3);
		shader.setUniformTexture("offsetYTex", offsetYTex != nullptr ? *offsetYTex : blackTexture, 4);
		shader.setUniformTexture("fTex", fTex != nullptr ? *fTex.get() : blackTexture, 5);
        shader.setUniform1i("type", noiseType);
        shader.setUniform2f("size", width, height);
        shader.setUniform1f("f", value);
        shader.setUniform2f("pos", posX, posY);
        shader.setUniform2f("scale", scaleX, scaleY);
		shader.setUniform2f("offset", offsetX, offsetY);
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
    ofParameter<float> value, scaleX, scaleY, offsetX, offsetY, modulator, warping, posX, posY;
	ofParameter<ofTexture*> fTex;
	ofTexture* scaleXTex;
	ofTexture* scaleYTex;
	ofTexture* offsetXTex;
	ofTexture* offsetYTex;
	
	ofTexture blackTexture;
    
    ofEventListener listener;
	ofEventListeners sizeListeners;
    
    ofFbo fbo;
};


#endif /* noiseTexture_h */
