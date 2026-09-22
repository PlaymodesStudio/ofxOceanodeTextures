//
//  mixerAlpha.h
//  Alpha-aware mixer with Photoshop-like layer blending
//
//  Created based on mixer.h
//

#ifndef mixerAlpha_h
#define mixerAlpha_h

#include "ofxOceanodeNodeModel.h"
#include "ofxOceanodeShared.h"
#include "imgui.h"
#include <algorithm>

#define STRINGIFY(A) #A

class mixerAlpha : public ofxOceanodeNodeModel {
public:
	mixerAlpha() : ofxOceanodeNodeModel("Mixer Alpha"){};
	
	void setup(){
		addInspectorParameter(numTextures.set("Num Textures", 2, 2, 16));
		addInspectorParameter(layerPreviews.set("Layer Previews", [this](){
			drawLayerPreviews();
		}));
		addParameter(width.set("Width", 100, 1, 50000));
		addParameter(height.set("Height", 100, 1, 50000));
		addOutputParameter(output.set("Output", nullptr));
		
		inputs.resize(numTextures);
		blendmodes.resize(numTextures, 0);
		opacities.resize(numTextures);
		textures.resize(numTextures, nullptr);
		previewAspectModes.resize(numTextures, PreviewAspectMode::Automatic);
		
		auto createNewParams = [this](int start, int size){
			auto vector_getter = [](void* vec, int idx, const char** out_text)
			{
				auto& vector = *static_cast<std::vector<std::string>*>(vec);
				if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
				*out_text = vector.at(idx).c_str();
				return true;
			};
			
			
			for(int i = start; i < (size+start); i++){
				auto parameterRef = addParameter(inputs[i].set("In " + ofToString(i), [i, vector_getter, this](){
					const float zoomLevel = ofxOceanodeShared::getZoomLevel();
					const float ww = ofxOceanodeShared::getNodeWidthWidget() * zoomLevel;
					const float wt = ofxOceanodeShared::getNodeWidthText() * zoomLevel;
					const bool renderWidgets = (zoomLevel > 0.5f);
					if (!renderWidgets) {
						const float rowH = ofxOceanodeShared::getBaseFrameHeight() * zoomLevel;
						// Row 1: matches the 1px spacer Dummy emitted at normal zoom
						ImGui::Dummy(ImVec2(wt + ww, 1.0f));
						// Row 2: matches the Text + Combo layer row
						ImGui::Dummy(ImVec2(wt + ww, rowH));
						return;
					}
					ImGui::Dummy(ImVec2(10 * zoomLevel, 1));
					ImGui::Text("%s", ("Layer " + ofToString(i+1, 2, '0')).c_str());
					ImGui::SameLine(wt, 0.0f);
					ImGui::SetNextItemWidth(ww);
					vector<string> options = {"Normal",
						"Multiply",
						"Average",
						"Add",
						"Substract",
						"Difference",
						"Negation",
						"Exclusion",
						"Screen",
						"Overlay",
						"SoftLight",
						"HardLight",
						"ColorDodge",
						"ColorBurn",
						"LinearLight",
						"VividLight",
						"PinLight",
						"HardMix",
						"Reflect",
						"Glow",
						"Phoenix",
						"Hue",
						"Saturation",
						"Color",
						"Luminosity",
						"Max",
						"Min"
					};
					ImGui::Combo("##Dropdown", &blendmodes[i], vector_getter, static_cast<void*>(&options), options.size());
				}));
				
				addParameter(opacities[i].set("Opac " + ofToString(i+1), 1, 0, 1));
				
				parameterRef->addReceiveFunc<ofTexture*>([this, i](ofTexture *const &tex){
					textures[i] = (ofTexture*)tex;
				});
				
				parameterRef->addDisconnectFunc([this, i](){
					if(i < textures.size()){
						textures[i] = nullptr;
					}
				});
				
				
			}
		};
		
		createNewParams(0, numTextures);
		
		
		listener = numTextures.newListener([this, createNewParams](int &i){
			if(inputs.size() != i){
				int oldSize = inputs.size();
				bool remove = oldSize > i;
				inputs.resize(numTextures);
				blendmodes.resize(numTextures, 0);
				opacities.resize(numTextures);
				textures.resize(numTextures, nullptr);
				previewAspectModes.resize(numTextures, PreviewAspectMode::Automatic);
				
				if(remove){
					for(int j = oldSize-1; j >= i; j--){
						removeParameter("In " + ofToString(j));
						removeParameter("Opac " + ofToString(j+1));
					}
				}else{
					createNewParams(oldSize, i-oldSize);
				}
			}
		});
		
		string defaultVertSource =
#include "shaders/defaultVertexShader.h"
		;
		
		string drawFragSource =
#include "shaders/mixerAlphaShader.h"
		;
		
		shader.setupShaderFromSource(GL_VERTEX_SHADER, defaultVertSource);
		shader.setupShaderFromSource(GL_FRAGMENT_SHADER, drawFragSource);
		shader.bindDefaults();
		shader.linkProgram();
	}
	
	void draw(ofEventArgs &a){
		int numActive = 0;
		int activeIndex = 0;
		for(int i = 0; i < opacities.size(); i++){
			if(opacities[i] != 0){
				numActive++;
				activeIndex = i;
			}
		}
		if(numActive == 1 && opacities[activeIndex] == 1){
			output = textures[activeIndex];
		}else{
			int i = numTextures-1;
			ofTexture* bottom = nullptr;
			bottom = textures[numTextures-1];
			while(bottom == nullptr || !bottom->isAllocated() || opacities[i] == 0){
				i--;
				if(i < 0){
					bottom = nullptr;
					break;
				}
				bottom = textures[i];
			}
			int fboIndex = 0;
			pingPongIndex = 0;
			if(!pingPongFbo[0].isAllocated() || pingPongFbo[0].getWidth() != width || pingPongFbo[0].getHeight() != height){
				ofFbo::Settings fboSettings;
				fboSettings.width = width;
				fboSettings.height = height;
				fboSettings.internalformat = GL_RGBA32F;
				fboSettings.numColorbuffers = 1;
				fboSettings.useDepth = false;
				fboSettings.useStencil = false;
				fboSettings.textureTarget = GL_TEXTURE_2D;
				fboSettings.maxFilter = GL_NEAREST;
				fboSettings.minFilter = GL_NEAREST;
				pingPongFbo[0].allocate(fboSettings);
				pingPongFbo[1].allocate(fboSettings);
			}
			if(bottom != nullptr){
				ofPushStyle();
				ofSetColor(255, 255, 255, 255);
				
				pingPongFbo[pingPongIndex].begin();
				ofClear(0, 0, 0, 0);
				ofEnableAlphaBlending();
				ofSetColor(255, 255, 255, 255*opacities[i]);
				bottom->draw(0, 0, width, height);
				ofDisableAlphaBlending();
				pingPongFbo[pingPongIndex].end();
				ofSetColor(255, 255, 255, 255);
				
				for(i--; i >= 0; i--){
					ofTexture* up = nullptr;
					up = textures[i];
					while(up == nullptr || !up->isAllocated() || opacities[i] == 0){
						i--;
						if(i < 0) break;
						up = textures[i];
					}
					if(i >= 0 && up != nullptr && up->isAllocated() && opacities[i] != 0){
						pingPongFbo[!pingPongIndex].begin();
						ofClear(0, 0, 0, 0);
						
						shader.begin();
						ofSetColor(255, 255, 255, 255);
						// Use units 0 and 1 to avoid conflicts with other nodes
						shader.setUniformTexture("base", pingPongFbo[pingPongIndex].getTexture(), 0);
						shader.setUniformTexture("blendTgt", *up, 1);
						shader.setUniform1i("mode", blendmodes[i]);
						shader.setUniform1f("opacity", opacities[i]);
						shader.setUniform1i("premultipliedInput", 0);
						ofDrawRectangle(0, 0, width, height);
						shader.end();
						pingPongFbo[!pingPongIndex].end();
						
						// CRITICAL: Unbind textures to prevent conflicts with other nodes
						// This prevents the same texture from being bound to multiple units
						glActiveTexture(GL_TEXTURE0);
						glBindTexture(GL_TEXTURE_2D, 0);
						glActiveTexture(GL_TEXTURE1);
						glBindTexture(GL_TEXTURE_2D, 0);
						
						pingPongIndex = !pingPongIndex;
					}
				}
				output = &pingPongFbo[pingPongIndex].getTexture();
//				GLenum err;
//				while ((err = glGetError()) != GL_NO_ERROR) {
//					ofLog() << "OpenGL error: " << err;
//				}
				ofPopStyle();
			}else{
				pingPongFbo[pingPongIndex].begin();
				ofClear(0, 0, 0, 0);
				pingPongFbo[pingPongIndex].end();
				output = &pingPongFbo[pingPongIndex].getTexture();
			}
		}
	}
	
	void presetSave(ofJson &json){
		for(int i = 0; i < numTextures; i++){
			json["LayerInfo"][i]["Blend"] = blendmodes[i];
		}
	}
	
	void loadBeforeConnections(ofJson &json){
		deserializeParameter(json, numTextures);
	}
	
	void presetRecallAfterSettingParameters(ofJson &json){
		for(int i = 0; i < numTextures; i++){
			try{
				blendmodes[i] = json["LayerInfo"][i]["Blend"];
			}catch (ofJson::exception& e)
			{
				ofLog() << e.what();
			}
		}
	}
	
	void deactivate(){
		baseFbo.clear();
		canvasFbo.clear();
		pingPongFbo[0].clear();
		pingPongFbo[1].clear();
        output = nullptr;
	}
	
private:
	enum class PreviewAspectMode { Automatic, Keep, Square };

	static string textureFormatName(int format){
		// Include the channel count and per-channel precision for common mixer inputs.
		switch(format){
			case GL_RGBA32F: return "RGBA 32-f";
			case GL_RGBA16F: return "RGBA 16-f";
			case GL_RGBA16: return "RGBA 16";
			case GL_RGBA8: return "RGBA 8";
			case GL_RGB32F: return "RGB 32-f";
			case GL_RGB16F: return "RGB 16-f";
			case GL_RGB16: return "RGB 16";
			case GL_RGB8: return "RGB 8";
			case GL_RG32F: return "RG 32-f";
			case GL_RG16F: return "RG 16-f";
			case GL_RG16: return "RG 16";
			case GL_RG8: return "RG 8";
			case GL_R32F: return "R 32-f";
			case GL_R16F: return "R 16-f";
			case GL_R16: return "R 16";
			case GL_R8: return "R 8";
			default: {
				const string name = ofGetGLInternalFormatName(format);
				return name == "unknown glInternalFormat" ? "Unknown (0x" + ofToHex(format) + ")" : name;
			}
		}
	}

	static const char *blendModeName(int mode){
		static const char *names[] = {
			"Normal", "Multiply", "Average", "Add", "Substract", "Difference",
			"Negation", "Exclusion", "Screen", "Overlay", "SoftLight", "HardLight",
			"ColorDodge", "ColorBurn", "LinearLight", "VividLight", "PinLight",
			"HardMix", "Reflect", "Glow", "Phoenix", "Hue", "Saturation", "Color",
			"Luminosity", "Max", "Min"
		};
		constexpr int nameCount = static_cast<int>(sizeof(names) / sizeof(names[0]));
		return mode >= 0 && mode < nameCount ? names[mode] : "Unknown";
	}

	void drawLayerPreviews(){
		for(size_t i = 0; i < textures.size(); ++i){
			const ofTexture *texture = textures[i];
			const bool allocated = texture != nullptr && texture->isAllocated();
			const float textureWidth = allocated ? texture->getWidth() : 0.0f;
			const float textureHeight = allocated ? texture->getHeight() : 0.0f;
			const float aspectRatio = textureWidth > 0 && textureHeight > 0 ? textureWidth / textureHeight : 1.0f;
			// Follow live dimensions until the user chooses an explicit mode for this layer.
			bool keepAspectRatio = previewAspectModes[i] == PreviewAspectMode::Automatic
				? aspectRatio >= 0.125f && aspectRatio <= 8.0f
				: previewAspectModes[i] == PreviewAspectMode::Keep;

			ImGui::Separator();
			ImGui::Text("Layer %02d", static_cast<int>(i + 1));
			ImGui::SameLine();
			ImGui::PushID(static_cast<int>(i));
			if(keepAspectRatio){
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
			}
			const bool toggleAspectRatio = ImGui::SmallButton("[AR]");
			if(keepAspectRatio) ImGui::PopStyleColor();
			if(toggleAspectRatio){
				keepAspectRatio = !keepAspectRatio;
				previewAspectModes[i] = keepAspectRatio ? PreviewAspectMode::Keep : PreviewAspectMode::Square;
			}
			if(ImGui::IsItemHovered()){
				ImGui::SetTooltip("%s", keepAspectRatio ? "Keep texture aspect ratio" : "Stretch preview to a square");
			}
			ImGui::PopID();

			if(!allocated){
				ImGui::SameLine(0.0f, 6.0f);
				ImGui::TextDisabled("| %s | %s", blendModeName(blendmodes[i]),
					texture == nullptr ? "No texture connected" : "Texture not allocated");
				ImGui::Spacing();
				continue;
			}

			const auto &data = texture->getTextureData();
			ImGui::SameLine(0.0f, 6.0f);
			ImGui::Text("| %s | %.0f x %.0f | %s", blendModeName(blendmodes[i]), textureWidth, textureHeight,
				textureFormatName(data.glInternalFormat).c_str());
			ImGui::NewLine();
			if(textureWidth > 0 && textureHeight > 0 && data.textureTarget == GL_TEXTURE_2D){
				// Fit both landscape and portrait inputs without letting one layer fill the Inspector.
				const float availableWidth = std::max(1.0f, ImGui::GetContentRegionAvail().x);
				const float previewAspectRatio = keepAspectRatio ? aspectRatio : 1.0f;
				const float previewWidth = std::min(std::min(availableWidth, 320.0f), 180.0f * previewAspectRatio);
				const ImVec2 size(previewWidth, previewWidth / previewAspectRatio);
				const ImVec2 start = ImGui::GetCursorScreenPos();
				const ImVec2 end(start.x + size.x, start.y + size.y);
				auto *drawList = ImGui::GetWindowDrawList();
				// A neutral background and border keep transparent inputs visible as separate previews.
				drawList->AddRectFilled(start, end, IM_COL32(40, 40, 40, 255));
				ImGui::Image((ImTextureID)(uintptr_t)data.textureID, size);
				drawList->AddRect(start, end, ImGui::GetColorU32(ImGuiCol_Border));
			}else{
				// The ImGui renderer samples GL_TEXTURE_2D; do not bind an incompatible target.
				ImGui::TextDisabled("Preview unavailable for this texture");
			}

			ImGui::Spacing();
		}
	}

	ofShader shader;
	
	ofParameter<ofTexture*> output;
	ofParameter<int> width, height;
	ofParameter<int> numTextures;
	customGuiRegion layerPreviews;
	vector<customGuiRegion> inputs;
	vector<ofParameter<float>> opacities;
	vector<ofTexture*> textures;
	vector<PreviewAspectMode> previewAspectModes;
	vector<int> blendmodes;
	
	ofEventListener listener;
	
	ofFbo baseFbo, canvasFbo;
	ofFbo pingPongFbo[2];
	int pingPongIndex;
};

#endif /* mixerAlpha_h */
