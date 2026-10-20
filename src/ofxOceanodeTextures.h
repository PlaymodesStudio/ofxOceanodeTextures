//
//  ofxOceanodeTextures.h
//  example
//
//  Created by Eduard Frigola Bagué on 14/10/2020.
//

#ifndef ofxOceanodeTextures_h
#define ofxOceanodeTextures_h

// Declarations only: the node headers and the bodies live in ofxOceanodeTextures.cpp, so
// including this header does not compile every node of the addon again.

class ofxOceanode;

namespace ofxOceanodeTextures{

void registerModels(ofxOceanode &o);
void registerType(ofxOceanode &o);
void registerScope(ofxOceanode &o);
void registerCollection(ofxOceanode &o);

}

#endif /* ofxOceanodeTextures_h */
