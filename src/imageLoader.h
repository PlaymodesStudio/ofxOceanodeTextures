//
//  imageLoader.h
//  lille
//
//  Created by Eduard Frigola Bagué on 04/02/2021.
//

#ifndef imageLoader_h
#define imageLoader_h

#include "ofxOceanodeNodeModel.h"

class imageLoader : public ofxOceanodeNodeModel {
public:
    imageLoader() : ofxOceanodeNodeModel("Image Loader"){}
    
    void setup(){
        loaded = false;
        
        ofDirectory dir;
        dir.open("Images");
        dir.sort();
        vector<string> files = {"None"};
        for(int i = 0; i < dir.listDir(); i++){
            files.push_back(dir.getName(i));
        }
        
        addParameterDropdown(fileIndex, "File", 0, files);
        addOutputParameter(texture.set("Output", nullptr));
        
        listener = fileIndex.newListener([this, files](int &i){
            string filename = files[i];
            if(filename == "None"){
                loaded = false;
            }else{
                loaded = image.load("Images/" + filename);
            }
        });
    }
    
    void draw(ofEventArgs &a){
        if(loaded){
            texture = &image.getTexture();
        }else{
            texture = nullptr;
        }
    }
    
private:
    ofParameter<int> fileIndex;
    ofParameter<ofTexture*> texture;
    
    ofEventListener listener;
    
    ofImage image;
    bool loaded;
};


#endif /* imageLoader_h */
