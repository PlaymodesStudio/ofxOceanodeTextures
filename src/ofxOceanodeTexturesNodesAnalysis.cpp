//
//  ofxOceanodeTexturesNodesAnalysis.cpp
//
//  The analysis: reading pixels, brightness, presence, blobs, color tracking and debug nodes of ofxOceanodeTextures.
//  Called from ofxOceanodeTextures::registerModels.
//

#include "ofxOceanode.h"
#include "textureReader.h"
#include "averageBrightness.h"
#include "presenceDetector.h"
#include "textureInfo.h"
#include "blobDetector.h"
#include "colorTracker.h"
#include "textureUnitMonitor.h"

namespace ofxOceanodeTextures{

void registerAnalysisNodes(ofxOceanode &o){
    o.registerModel<textureReader>("Textures");
    o.registerModel<AverageBrightness>("Textures");
    o.registerModel<presenceDetector>("Textures");
    o.registerModel<textureInfo>("Textures");
    o.registerModel<blobDetector>("Textures");
    o.registerModel<colorTracker>("Textures");
    o.registerModel<textureUnitMonitor>("Debug");
}

}
