//
//  textureResize.h
//  PLedNodes
//
//  Created by Eduard Frigola Bagué on 10/03/2020.
//

#ifndef textureResize_h
#define textureResize_h

#include "ofxOceanodeNodeModel.h"
#define STRINGIFY(x) #x

class textureResize : public ofxOceanodeNodeModel {
public:
    textureResize() : ofxOceanodeNodeModel("Texture Resizer"){};
    ~textureResize(){};
    
    void setup() override{
        addParameter(input.set("Input", nullptr));
        addParameter(width.set("Width", 100, 1, 5120));
        addParameter(height.set("Height", 100, 1, 2880));
        addParameter(interpolate.set("Interpol.", false));
        addOutputParameter(output.set("Output", nullptr));
        listener = input.newListener([this](ofTexture* &tex){
            if(input != nullptr){
                if(inputSize.x != input.get()->getWidth() || inputSize.y != input.get()->getHeight()){
                    inputSize.x = input.get()->getWidth();
                    inputSize.y = input.get()->getHeight();
                    resizeMesh();
                }
                if(width != fbo.getWidth() || height != fbo.getHeight()){
                    resizeFbo();
                    resizeMesh();
                }
                fbo.begin();
                if(interpolate){
                    shader.begin();
                    shader.setUniformTexture("textureIn", *input.get(), 0);
                    mesh.draw();
                    shader.end();
                }else{
                    input.get()->draw(0,0, width, height);
                }
                fbo.end();
                output = &fbo.getTexture();
            }
        });
        
        inputSize = glm::vec2(0,0);
        resizeFbo();
        setupShader();
    }
    
    void activate(){
        resizeFbo();
    }
    
    void deactivate(){
        fbo.clear();
        output = nullptr;
    }
    
private:
    
    void resizeFbo(){
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
        ofClear(0, 0, 0, 255);
        fbo.end();
        resizeMesh();
    }
    
    void resizeMesh(){
        mesh.clear();
        float itemW = width / (inputSize.x);
        float itemH = height / inputSize.y;
        for(int i = 0 ; i < inputSize.y; i++){
            float y = i*itemH;
            float yy = (i+0.5)/inputSize.y;
            addSquareToMesh(ofRectangle(0.5/inputSize.x, yy, 0, 0), ofRectangle(0, y, itemW/2, itemH));
            for(int j = 0; j < inputSize.x-1 ; j++){
                float x = (j+0.5)*itemW;
                float xx = (j+0.5)/inputSize.x;
                
                addSquareToMesh(ofRectangle(xx, yy, 1/float(inputSize.x), 0), ofRectangle(x, y, itemW, itemH));
            }
            addSquareToMesh(ofRectangle(1-(0.5/inputSize.x), yy, 0, 0), ofRectangle(width-(itemW/2), y, itemW/2, itemH));
        }
    }
    
    void addSquareToMesh(ofRectangle inRect, ofRectangle outRect){
        mesh.addTexCoord(inRect.getTopLeft());
        mesh.addTexCoord(inRect.getTopRight());
        mesh.addTexCoord(inRect.getBottomRight());
        mesh.addTexCoord(inRect.getTopLeft());
        mesh.addTexCoord(inRect.getBottomRight());
        mesh.addTexCoord(inRect.getBottomLeft());
        
        mesh.addVertex(outRect.getTopLeft());
        mesh.addVertex(outRect.getTopRight());
        mesh.addVertex(outRect.getBottomRight());
        mesh.addVertex(outRect.getTopLeft());
        mesh.addVertex(outRect.getBottomRight());
        mesh.addVertex(outRect.getBottomLeft());
    }
    
    void setupShader(){
        const string vertSource=R"(#version 410
        
        uniform mat4 projectionMatrix;
        uniform mat4 modelViewMatrix;
        uniform mat4 modelViewProjectionMatrix;
        uniform mat4 textureMatrix;
        
        layout(location = 0) in vec4  position;
        layout(location = 1) in vec4  color;
        layout(location = 2) in vec3  normal;
        layout(location = 3) in vec2  texcoord;
        
        uniform sampler2D textureIn;
        
        noperspective out vec4 vertexColor;
        
        void main(){
            gl_Position = modelViewProjectionMatrix * position;
            vertexColor = texture(textureIn, vec2(texcoord.x, texcoord.y));
        }
        
        )";
        
        const string fragSource=R"(#version 410
        
        //flat in vec4 vertexColor;
        //smooth in vec4 vertexColor;
        noperspective in vec4 vertexColor;
        
        out vec4 out_Color;
        
        void main()
        {
            out_Color = vertexColor;
        }
        )";
        
        shader.setupShaderFromSource(GL_VERTEX_SHADER, vertSource);
        shader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragSource);
        shader.bindDefaults();
        shader.linkProgram();
    }
    
    ofParameter<ofTexture*> input;
    ofParameter<bool> interpolate;
    ofParameter<int> width;
    ofParameter<int> height;
    ofParameter<ofTexture*> output;
    
    ofEventListener listener;
    
    ofFbo fbo;
    
    ofVboMesh mesh;
    glm::vec2 inputSize;
    ofShader shader;
};

#endif /* textureResize_h */
