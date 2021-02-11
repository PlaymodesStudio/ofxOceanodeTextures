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
        addParameter(filename.set("File", ""));
        addParameter(texture.set("Output", nullptr));
        
        listener = filename.newListener([this](string &s){
            image.load(s);
        });
    }
    
    void draw(ofEventArgs &a){
        texture = &image.getTexture();
    }
    
private:
    ofParameter<string> filename;
    ofParameter<ofTexture*> texture;
    
    ofEventListener listener;
    
    ofImage image;
};


#endif /* imageLoader_h */
