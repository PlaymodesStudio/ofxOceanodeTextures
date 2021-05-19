//
//  textureHelpers.h
//  lille
//
//  Created by Eduard Frigola Bagué on 08/02/2021.
//

#ifndef textureHelpers_h
#define textureHelpers_h

#include "ofxOceanodeNodeModel.h"

class interactiveCanvas : public ofxOceanodeNodeModel{
public:
    interactiveCanvas() : ofxOceanodeNodeModel("Interactive Canvas"){}
    
    void setup(){
        addParameter(input.set("Input", nullptr));
        addParameter(x.set("X", 0.5, 0, 1));
        addParameter(y.set("Y", 0.5, 0, 1));
        addParameter(force.set("Force", 0, -1, 1));
        
        black.allocate(4, 4, GL_RGB);
//        black.begin();
//        ofClear(0, 0, 0, 255);
//        black.end();
    }
    
    void draw(ofEventArgs &a){
        if(ImGui::Begin(("Interactive Canvas " + ofToString(getNumIdentifier())).c_str())){
            auto screenSize = ImGui::GetContentRegionAvail();
            auto screenPos = ImGui::GetCursorScreenPos();
            ImTextureID textureID = (ImTextureID)(uintptr_t)black.texData.textureID;
            if(input.get() != nullptr){
                textureID = (ImTextureID)(uintptr_t)input.get()->texData.textureID;
            }
            ImGui::Image(textureID, screenSize);
            if(ImGui::IsWindowHovered()){
                if(ImGui::IsMouseDown(0)){
                    auto normPos = (ImGui::GetMousePos() - screenPos) / screenSize;
                    x = normPos.x;
                    y = normPos.y;
                    force = 1;
                }
                else if(ImGui::IsMouseDown(1)){
                    auto normPos = (ImGui::GetMousePos() - screenPos) / screenSize;
                    x = normPos.x;
                    y = normPos.y;
                    force = -1;
                }
                else{
                    force = 0;
                }
            }
        }
        ImGui::End();
    }
private:
    ofParameter<ofTexture*> input;
    ofTexture black;
    ofParameter<float> x, y, force;
};

class textureReader : public ofxOceanodeNodeModel{
public:
    textureReader() : ofxOceanodeNodeModel("Texture Reader"){
        addParameter(input.set("Input", nullptr));
		addParameter(r.set("Red", {0}, {0}, {1}));
		addParameter(g.set("Green", {0}, {0}, {1}));
		addParameter(b.set("Blue", {0}, {0}, {1}));
        addParameter(output.set("Output", {0}, {0}, {1}));
        listener = input.newListener([this](ofTexture* &tex){
            if(tex != nullptr){
				if(tex->getWidth() == 0 || tex->getHeight() == 0) return;
                if(tempOutput.size() != tex->getWidth() * tex->getHeight()){
                    tempOutput.resize(tex->getWidth() * tex->getHeight(), 0);
					temp_r.resize(tex->getWidth() * tex->getHeight(), 0);
					temp_g.resize(tex->getWidth() * tex->getHeight(), 0);
					temp_b.resize(tex->getWidth() * tex->getHeight(), 0);
                }
                ofFloatPixels pixels;
                tex->readToPixels(pixels);
                float *p = pixels.begin();
                int pixelSize = pixels.getPixelFormat();
				if(pixelSize >= 3){
					for(int i = 0; i < tex->getWidth() * tex->getHeight(); i++){
						temp_r[i] = p[i*pixelSize];
						temp_g[i] = p[(i*pixelSize)+1];
						temp_b[i] = p[(i*pixelSize)+2];
						tempOutput[i] = max(max(temp_r[i], temp_g[i]), temp_b[i]);
					}
				}else{
					for(int i = 0; i < tex->getWidth() * tex->getHeight(); i++){
						tempOutput[i] = p[i*pixelSize];
					}
				}
				r = temp_r;
				g = temp_g;
				b = temp_b;
                output = tempOutput;
            }
        });
    };
    
    ~textureReader(){};
    
private:
    ofParameter<ofTexture*> input;
    ofParameter<vector<float>> output, r, g, b;
	
    
    vector<float> tempOutput, temp_r, temp_g, temp_b;
    ofEventListener listener;
};

class vectorToTexture : public ofxOceanodeNodeModel{
public:
    vectorToTexture() : ofxOceanodeNodeModel("Vector to Texture"){};
    
    void setup(){
        addParameter(input.set("Input", {0}, {0}, {1}));
        addParameter(size.set("Size", "32x4"));
        addParameter(output.set("Output Tex", nullptr));
        
        listener = size.newListener([this](string &s){
            changedSize = true;
        });
        
        changedSize = true;
    }
    
    void draw(ofEventArgs &a){
        if(changedSize){
            vector<string> dimensions = ofSplitString(size, "x");
            width = ofToInt(dimensions[0]);
            height = ofToInt(dimensions[1]);
            fbo.allocate(width, height, GL_RGBA);
            fbo.getTexture().setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
            changedSize = false;
        }
        vector<float> rgbInput(input->size()*3, 0);
        for(int i = 0; i < input->size(); i++){
            rgbInput[(i*3)] = input->at(i);
            rgbInput[(i*3)+1] = input->at(i);
            rgbInput[(i*3)+2] = input->at(i);
        }
        tex.loadData(rgbInput.data(), width, height, GL_RGB);
        tex.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
        fbo.begin();
        tex.draw(0, 0, width, height);
        fbo.end();
        output = &fbo.getTexture();
    }
    
    void loadBeforeConnections(ofJson &json){
        deserializeParameter(json, size);
    }
    
private:
    ofParameter<vector<float>> input;
    ofParameter<ofTexture*> output;
    ofParameter<string> size;
    ofTexture tex;
    ofFbo fbo;
    int width;
    int height;
    
    bool changedSize;
    
    ofEventListener listener;
};

#endif /* textureHelpers_h */
