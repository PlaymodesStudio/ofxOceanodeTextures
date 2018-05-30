//
//  sharedResources.h
//  MIRABCN_Generator
//
//  Created by Eduard Frigola on 01/12/2016.
//
//

#ifndef sharedResources_h
#define sharedResources_h

#include "ofMain.h"

class sharedResources{
public:
    
    sharedResources(){
        lastShaderTextureLocationUsed = -1;
    }
    ~sharedResources(){};
    
    static sharedResources &getInstance()
    {
        static sharedResources instance;
        return instance;
    }
    
    int getNextAvailableShaderTextureLocation(){
        int newLocation = computeEmptyLocation();
        usedTextureLocations[newLocation] = true;
        lastShaderTextureLocationUsed = newLocation;
        ofLog() << "NewLocationFound: " << newLocation;
        return newLocation;
    }

    void makeTextureLocationAvailable(uint location){
        ofLog() << "Unuse location: " << location;
        usedTextureLocations[location] = false;
    }
    
private:
    int computeEmptyLocation(){
        int newLocation = lastShaderTextureLocationUsed + 1;
        while(newLocation > 0){
            newLocation--;
            if(!usedTextureLocations[newLocation]){
                return newLocation;
            }
        }
        newLocation = lastShaderTextureLocationUsed + 1;
        while(newLocation < usedTextureLocations.size()){
            if(!usedTextureLocations[newLocation]){
                return newLocation;
            }
            newLocation++;
        }
        usedTextureLocations.push_back(true);
        return usedTextureLocations.size()-1;
    }
    
    vector<bool> usedTextureLocations;
    uint lastShaderTextureLocationUsed = -1;
};




#endif /* sharedResources_h */
