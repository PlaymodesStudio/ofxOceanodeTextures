//
//  videoPlayer.h
//  lille
//
//  Created by Eduard Frigola Bagué on 16/02/2021.
//

#ifndef videoPlayer_h
#define videoPlayer_h

#include "ofxOceanodeNodeModel.h"

class videoPlayer : public ofxOceanodeNodeModel {
public:
	videoPlayer() : ofxOceanodeNodeModel("Video Player"){}
	
	void setup(){
		blackTexture.allocate(1, 1, GL_RGBA32F);
		
		// Scan Movies folder for dropdown
		refreshMoviesFolder();
		
		// Create parameters
		addParameterDropdown(fileIndex, "DataFile", 0, movieFiles);
		addParameter(manualPath.set("Path", ""));
		addParameter(browsePath.set("Browse"));
		addParameter(currentFile.set("Current", "None"));
		
		addParameter(loop.set("Loop", true));
		addParameter(play.set("Play", false));
		addParameter(speed.set("Speed", 1, 0, 10));
		addParameter(position.set("Position", 0, 0, 1));
		addOutputParameter(texture.set("Output", nullptr));
		
		// Add internal parameter for storing the actual loaded path
		addInspectorParameter(loadedFilePath.set("LoadedPath", ""));
		
		// Dropdown selection listener
		listeners.push(fileIndex.newListener([this](int &i){
			if(i == 0) {
				// "None" selected
				loadedFilePath = "";
				currentFile = "None";
				vPlayer.close();
			} else {
				string filename = movieFiles[i];
				string fullPath = "Movies/" + filename;
				loadVideoFile(fullPath, filename);
			}
		}));
		
		// Manual path listener
		listeners.push(manualPath.newListener([this](string &path){
			if(!path.empty() && !isLoadingPreset) {
				// Reset dropdown to avoid conflicts
				fileIndex = 0;
				
				// Extract filename for display
				string filename = ofFilePath::getFileName(path);
				loadVideoFile(path, filename);
			}
		}));
		
		// Browse file button listener
		listeners.push(browsePath.newListener([this](void){
			openFileDialog();
		}));
		
		listeners.push(loop.newListener([this](bool &b){
			if(vPlayer.isLoaded()) {
				vPlayer.setLoopState(b ? OF_LOOP_NORMAL : OF_LOOP_NONE);
			}
		}));
		
		listeners.push(play.newListener([this](bool &b){
			if(vPlayer.isLoaded()) {
				if(b) vPlayer.play();
				else vPlayer.stop();
			}
		}));
		
		listeners.push(speed.newListener([this](float &f){
			if(vPlayer.isLoaded()) {
				vPlayer.setSpeed(f);
			}
		}));
		
		listeners.push(position.newListener([this](float &f){
			if(!positionSetByItself && vPlayer.isLoaded()){
				requestedPosition = f;
			}
			positionSetByItself = false;
		}));
	}
	
	void update(ofEventArgs &a){
		if(vPlayer.isLoaded() && vPlayer.isPlaying()) {
			if(requestedPosition != -1){
				vPlayer.setPosition(requestedPosition);
				requestedPosition = -1;
			}
			vPlayer.update();
			if(vPlayer.isFrameNew()){
				texture = &vPlayer.getTexture();
			}
			positionSetByItself = true;
			position = vPlayer.getPosition();
		} else {
			texture = &blackTexture;
		}
	}
	
	void draw(ofEventArgs &a){
		// Optional: Could draw video info or debug info here
	}
	
	// Override preset loading to reload video
	void presetRecallAfterSettingParameters(ofJson &json) override {
		isLoadingPreset = true;
		
		// Check if we have a saved file path
		if(!loadedFilePath.get().empty()) {
			string path = loadedFilePath.get();
			string filename = ofFilePath::getFileName(path);
			
			// Try to reload the video
			if(ofFile::doesFileExist(path)) {
				loadVideoFile(path, filename);
			} else {
				ofLogWarning("videoPlayer") << "Saved video file not found: " << path;
				currentFile = "File Not Found: " + filename;
			}
		}
		// If we have a dropdown selection saved
		else if(fileIndex > 0 && fileIndex < movieFiles.size()) {
			string filename = movieFiles[fileIndex];
			string fullPath = "Movies/" + filename;
			loadVideoFile(fullPath, filename);
		}
		
		isLoadingPreset = false;
	}
	
private:
	// Parameters
	ofParameter<int> fileIndex;
	ofParameter<string> manualPath;
	ofParameter<void> browsePath;
	ofParameter<string> currentFile;  // Read-only display of current file
	ofParameter<string> loadedFilePath;  // Internal parameter to save the actual loaded path
	
	ofParameter<bool> loop;
	ofParameter<bool> play;
	ofParameter<float> speed;
	ofParameter<float> position;
	ofParameter<ofTexture*> texture;
	
	// Internal state
	bool positionSetByItself = false;
	float requestedPosition = -1;
	vector<string> movieFiles;
	bool isLoadingPreset = false;
	
	ofEventListeners listeners;
	ofVideoPlayer vPlayer;
	ofTexture blackTexture;
	
	// Helper methods
	void refreshMoviesFolder() {
		ofDirectory dir;
		dir.open("Movies");
		dir.sort();
		movieFiles = {"None"};
		for(int i = 0; i < dir.listDir(); i++){
			movieFiles.push_back(dir.getName(i));
		}
	}
	
	void loadVideoFile(const string& path, const string& displayName) {
		if(ofFile::doesFileExist(path)) {
			vPlayer.close();  // Close previous video
			
			bool success = vPlayer.load(path);
			if(success) {
				// Store the loaded path for preset saving
				loadedFilePath = path;
				currentFile = displayName;
				
				// Update manualPath if it's not from dropdown
				if(fileIndex == 0 && manualPath.get() != path) {
					manualPath = path;
				}
				
				vPlayer.setLoopState(loop ? OF_LOOP_NORMAL : OF_LOOP_NONE);
				
				// Reset playback state
				if(!isLoadingPreset) {
					play = false;
					position = 0;
				}
				requestedPosition = -1;
				
				ofLogNotice("videoPlayer") << "Successfully loaded: " << path;
			} else {
				loadedFilePath = "";
				currentFile = "Load Failed: " + displayName;
				ofLogError("videoPlayer") << "Failed to load: " << path;
			}
		} else {
			loadedFilePath = "";
			currentFile = "File Not Found: " + displayName;
			ofLogError("videoPlayer") << "File does not exist: " << path;
		}
	}
	
	void openFileDialog() {
		ofFileDialogResult result = ofSystemLoadDialog("Select Video File", false, "");
		
		if(result.bSuccess) {
			string selectedPath = result.getPath();
			
			// Reset dropdown to maintain consistency
			fileIndex = 0;
			
			// Set manual path to the selected file
			manualPath = selectedPath;
			
			// Extract filename for display
			string filename = ofFilePath::getFileName(selectedPath);
			loadVideoFile(selectedPath, filename);
		}
	}
};

#endif /* videoPlayer_h */
