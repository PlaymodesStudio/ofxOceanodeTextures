//
//  textureReader.h
//  ofxOceanodeTextures
//
//  Created by Eduard Frigola Bagué on 08/02/2021.
//

#ifndef textureReader_h
#define textureReader_h

#include "ofxOceanodeNodeModel.h"

class textureReader : public ofxOceanodeNodeModel{
public:
    textureReader() : ofxOceanodeNodeModel("Texture Reader"){
        addParameter(input.set("Input", nullptr));
        addInspectorParameter(separator.set("Add Separator", false));
		addOutputParameter(r.set("Red", {0}, {0}, {1}));
		addOutputParameter(g.set("Green", {0}, {0}, {1}));
		addOutputParameter(b.set("Blue", {0}, {0}, {1}));
        addOutputParameter(output.set("Output", {0}, {0}, {1}));
    };
    
    
    void draw(ofEventArgs &a) override{
        ofTexture* tex = input;
        if((tex != nullptr)&&(tex->isAllocated())){
            if(tex->getWidth() == 0 || tex->getHeight() == 0) return;
            if(tempOutput.size() != (tex->getWidth()+separator) * tex->getHeight()){
                tempOutput.resize((tex->getWidth()+separator) * tex->getHeight(), 0);
                temp_r.resize((tex->getWidth()+separator) * tex->getHeight(), 0);
                temp_g.resize((tex->getWidth()+separator) * tex->getHeight(), 0);
                temp_b.resize((tex->getWidth()+separator) * tex->getHeight(), 0);
            }
            ofFloatPixels pixels;
            tex->readToPixels(pixels);
            float *p = pixels.begin();
            int numChannels = pixels.getNumChannels();
            if(numChannels >= 3){
                int j = 0;
                for(int i = 0; i < tex->getWidth() * tex->getHeight(); i++){
                    temp_r[j] = p[i*numChannels];
                    temp_g[j] = p[(i*numChannels)+1];
                    temp_b[j] = p[(i*numChannels)+2];
                    tempOutput[j] = max(max(temp_r[j], temp_g[j]), temp_b[j]);
                    j++;
                    if(separator && ((i+1) % int(tex->getWidth()) == 0)){
                        temp_r[j] = -1;
                        temp_g[j] = -1;
                        temp_b[j] = -1;
                        tempOutput[j] = max(max(temp_r[j], temp_g[j]), temp_b[j]);
                        j++;
                    }
                }
            }else{
                for(int i = 0; i < tex->getWidth() * tex->getHeight(); i++){
                    tempOutput[i] = p[i*numChannels];
                }
            }
            r = temp_r;
            g = temp_g;
            b = temp_b;
            output = tempOutput;
        }
    }
    
    ~textureReader(){};
    
private:
    ofParameter<ofTexture*> input;
    ofParameter<vector<float>> output, r, g, b;
    ofParameter<bool> separator;
    
    vector<float> tempOutput, temp_r, temp_g, temp_b;
};

#endif /* textureReader_h */
