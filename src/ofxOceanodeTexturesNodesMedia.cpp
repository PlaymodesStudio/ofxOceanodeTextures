//
//  ofxOceanodeTexturesNodesMedia.cpp
//
//  The media input and output: images, video, camera, senders, recorders and displays nodes of ofxOceanodeTextures.
//  Called from ofxOceanodeTextures::registerModels.
//

#include "ofxOceanode.h"
#include "imageLoader.h"
#include "videoPlayer.h"
#include "videoPlayerScrub.h"
#include "cameraInput.h"
#include "textureSender.h"
#include "textureRecorder.h"
#include "textureRecorderFFmpeg.h"
#include "textureSnapshot.h"
#include "displayOutput.h"
#include "textureDisplay.h"

namespace ofxOceanodeTextures{

void registerMediaNodes(ofxOceanode &o){
    o.registerModel<imageLoader>("Textures");
    o.registerModel<videoPlayer>("Textures");
    o.registerModel<videoPlayerScrub>("Textures");
    o.registerModel<cameraInput>("Textures");
    o.registerModel<textureSender>("Textures");
    o.registerModel<textureReceiver>("Textures");
    o.registerModel<textureRecorder>("Textures");
    o.registerModel<textureRecorderFFmpeg>("Textures");
    o.registerModel<textureSnapshot>("Textures");
    o.registerModel<displayOutput>("Textures");
    o.registerModel<textureDisplay>("Textures");
}

}
