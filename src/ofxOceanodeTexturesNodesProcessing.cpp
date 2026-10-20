//
//  ofxOceanodeTexturesNodesProcessing.cpp
//
//  The processing: mixing, compositing, resizing, alpha, feedback and color nodes of ofxOceanodeTextures.
//  Called from ofxOceanodeTextures::registerModels.
//

#include "ofxOceanode.h"
#include "mixer.h"
#include "mixerAlpha.h"
#include "textureComposer.h"
#include "textureComposerSimple.h"
#include "textureBlender.h"
#include "textureResize.h"
#include "textureResizeFast.h"
#include "textureUnifier.h"
#include "subTexture.h"
#include "injectAlpha.h"
#include "AlphaTrails.h"
#include "textureFeedbackDelay.h"
#include "TextureChannels.h"
#include "textureFlip.h"
#include "paletteQuantizer.h"
#include "zdepthConverter.h"

namespace ofxOceanodeTextures{

void registerProcessingNodes(ofxOceanode &o){
    o.registerModel<mixer>("Textures");
    o.registerModel<mixerAlpha>("Textures");
    o.registerModel<textureComposer>("Textures");
    o.registerModel<textureComposerSimple>("Textures");
    o.registerModel<textureBlender>("Textures");
    o.registerModel<textureResize>("Textures");
    o.registerModel<textureResizeFast>("Textures");
    o.registerModel<textureUnifier>("Textures");
    o.registerModel<subTexture>("Textures");
    o.registerModel<injectAlpha>("Textures");
    o.registerModel<AlphaTrails>("Textures");
    o.registerModel<textureFeedbackDelay>("Textures");
    o.registerModel<TextureChannels>("Textures");
    o.registerModel<textureFlip>("Textures");
    o.registerModel<paletteQuantizer>("Textures");
    o.registerModel<zdepthConverter>("Textures");
}

}
