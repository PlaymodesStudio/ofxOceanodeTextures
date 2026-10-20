//
//  ofxOceanodeTexturesNodesGenerators.cpp
//
//  The generators: shaders, oscillators, noise, gradients and colors nodes of ofxOceanodeTextures.
//  Called from ofxOceanodeTextures::registerModels.
//

#include "ofxOceanode.h"
#include "shaderEditor.h"
#include "indexerTexture.h"
#include "oscillatorTexture.h"
#include "chaoticOscillatorTexture.h"
#include "noiseTexture.h"
#include "Gradient.h"
#include "Gradient2.h"
#include "ColorByValue.h"
#include "ColorTexture.h"
#include "vectorToTexture.h"
#include "interactiveCanvas.h"

namespace ofxOceanodeTextures{

void registerGeneratorNodes(ofxOceanode &o){
    o.registerModel<shaderEditor>("Textures");
    o.registerModel<indexerTexture>("Textures");
    o.registerModel<indexerTexture2>("Textures");
    o.registerModel<oscillatorTexture>("Textures");
    o.registerModel<oscillatorTexture2>("Textures");
    o.registerModel<chaoticOscillatorTexture>("Textures");
    o.registerModel<noiseTexture>("Textures");
    o.registerModel<Gradient>("Textures");
    o.registerModel<Gradient2>("Textures");
    o.registerModel<ColorByValue>("Textures");
    o.registerModel<ColorTexture>("Textures");
    o.registerModel<vectorToTexture>("Textures");
    o.registerModel<interactiveCanvas>("Textures");
}

}
