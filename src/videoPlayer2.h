//
//  videoPlayer2.h
//  Enhanced video player with background frame decoding
//

#ifndef videoPlayer2_h
#define videoPlayer2_h

#include "ofxOceanodeNodeModel.h"
#include <atomic>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

// Structure to hold decoded frame data
struct VideoFrame {
	ofPixels pixels;
	float position;
	int frameNumber;
	uint64_t timestamp;
};

class videoPlayer2 : public ofxOceanodeNodeModel {
public:
	videoPlayer2() : ofxOceanodeNodeModel("Video Player 2") {}
	
	~videoPlayer2() {
		// Clean shutdown
		shouldExit = true;
		isVideoLoaded = false;
		
		// Wake up threads if waiting
		frameNeededCondition.notify_all();
		
		// Wait for threads to finish
		if(decodeThread.joinable()) {
			decodeThread.join();
		}
	}
	
	void setup() {
		// Initialize textures
		blackTexture.allocate(1, 1, GL_RGBA32F);
		
		// Scan Movies folder for dropdown
		refreshMoviesFolder();
		
		// Create parameters
		addParameterDropdown(fileIndex, "DataFile", 0, movieFiles);
		addParameter(manualPath.set("Path", ""));
		addParameter(browsePath.set("Browse"));
		addParameter(currentFile.set("Current", "None"));
		
		// Add buffer size control
		addParameter(bufferSize.set("Buffer Size", 10, 1, 30));
		addParameter(bufferStatus.set("Buffer Status", "Empty"));
		
		addParameter(loop.set("Loop", true));
		addParameter(play.set("Play", false));
		addParameter(speed.set("Speed", 1, 0, 10));
		addParameter(position.set("Position", 0, 0, 1));
		addOutputParameter(texture.set("Output", nullptr));
		
		// Set up listeners
		setupListeners();
		
		// Start decode thread
		shouldExit = false;
		isVideoLoaded = false;
		decodeThread = std::thread(&videoPlayer2::decodeThreadFunction, this);
	}
	
	void update(ofEventArgs &a) {
		// Handle seek requests
		if(requestedPosition != -1 && vPlayer.isLoaded()) {
			vPlayer.setPosition(requestedPosition);
			requestedPosition = -1;
			
			// Clear buffer on seek
			std::lock_guard<std::mutex> lock(bufferMutex);
			clearBuffer();
		}
		
		// Update main video player
		if(vPlayer.isLoaded() && vPlayer.isPlaying()) {
			vPlayer.update();
		}
		
		// Try to use buffered frame
		bool usedBufferedFrame = false;
		{
			std::unique_lock<std::mutex> lock(bufferMutex);
			
			if(!decodedFrames.empty() && vPlayer.isLoaded() && play) {
				// Get the next frame
				VideoFrame& frame = decodedFrames.front();
				
				// Calculate if this frame is current (within 2 frames tolerance)
				float frameTime = 1.0f / vPlayer.getTotalNumFrames();
				float tolerance = frameTime * 2;
				
				if(abs(frame.position - vPlayer.getPosition()) < tolerance) {
					// Update texture with decoded frame
					if(!displayTexture.isAllocated() ||
					   displayTexture.getWidth() != frame.pixels.getWidth() ||
					   displayTexture.getHeight() != frame.pixels.getHeight()) {
						displayTexture.allocate(frame.pixels.getWidth(),
											  frame.pixels.getHeight(),
											  ofGetGLInternalFormatFromPixelFormat(frame.pixels.getPixelFormat()));
					}
					
					displayTexture.loadData(frame.pixels);
					texture = &displayTexture;
					
					// Remove used frame
					decodedFrames.pop();
					usedBufferedFrame = true;
					
					// Update position
					positionSetByItself = true;
					position = frame.position;
				}
			}
		}
		
		// Fallback to direct texture if no buffered frame used
		if(!usedBufferedFrame) {
			if(vPlayer.isLoaded() && vPlayer.isFrameNew()) {
				texture = &vPlayer.getTexture();
				positionSetByItself = true;
				position = vPlayer.getPosition();
			} else if(!vPlayer.isLoaded() || !play) {
				texture = &blackTexture;
			}
		}
		
		// Update buffer status
		updateBufferStatus();
		
		// Notify decode thread if buffer is low
		if(isVideoLoaded && decodedFrames.size() < bufferSize / 2) {
			frameNeededCondition.notify_one();
		}
	}
	
private:
	// Original videoPlayer member variables
	ofParameter<int> fileIndex;
	ofParameter<std::string> manualPath;
	ofParameter<void> browsePath;
	ofParameter<std::string> currentFile;
	
	ofParameter<bool> loop;
	ofParameter<bool> play;
	ofParameter<float> speed;
	ofParameter<float> position;
	ofParameter<ofTexture*> texture;
	
	bool positionSetByItself = false;
	float requestedPosition = -1;
	std::string currentLoadedFile;
	std::vector<std::string> movieFiles;
	
	ofEventListeners listeners;
	ofVideoPlayer vPlayer;
	ofTexture blackTexture;
	
	// New threaded decoding member variables
	std::queue<VideoFrame> decodedFrames;
	std::mutex bufferMutex;
	std::condition_variable frameNeededCondition;
	std::atomic<bool> shouldExit{false};
	std::atomic<bool> isVideoLoaded{false};
	std::atomic<bool> isDecoderReady{false};
	std::thread decodeThread;
	
	// Secondary video player for decoding
	ofVideoPlayer decoderPlayer;
	std::mutex decoderMutex;
	
	// Display texture
	ofTexture displayTexture;
	
	// Buffer parameters
	ofParameter<int> bufferSize;
	ofParameter<std::string> bufferStatus;
	
	// Helper methods from original
	void refreshMoviesFolder() {
		ofDirectory dir;
		dir.open("Movies");
		dir.sort();
		movieFiles = {"None"};
		for(int i = 0; i < dir.listDir(); i++){
			movieFiles.push_back(dir.getName(i));
		}
	}
	
	void loadVideoFile(const std::string& path, const std::string& displayName) {
		// First, signal that we're loading
		isVideoLoaded = false;
		isDecoderReady = false;
		
		// Clear buffer
		{
			std::lock_guard<std::mutex> bufferLock(bufferMutex);
			clearBuffer();
		}
		
		// Close decoder player
		{
			std::lock_guard<std::mutex> decoderLock(decoderMutex);
			if(decoderPlayer.isLoaded()) {
				decoderPlayer.close();
			}
		}
		
		// Close main player
		if(vPlayer.isLoaded()) {
			vPlayer.close();
		}
		
		if(ofFile::doesFileExist(path)) {
			bool success = vPlayer.load(path);
			if(success) {
				currentLoadedFile = path;
				currentFile = displayName;
				vPlayer.setLoopState(loop ? OF_LOOP_NORMAL : OF_LOOP_NONE);
				
				play = false;
				position = 0;
				requestedPosition = -1;
				
				// Signal that video is loaded
				isVideoLoaded = true;
				
				ofLogNotice("videoPlayer2") << "Successfully loaded: " << path;
				
				// Wake up decode thread
				frameNeededCondition.notify_one();
			} else {
				currentLoadedFile = "";
				currentFile = "Load Failed: " + displayName;
				ofLogError("videoPlayer2") << "Failed to load: " << path;
			}
		} else {
			currentLoadedFile = "";
			currentFile = "File Not Found: " + displayName;
			ofLogError("videoPlayer2") << "File does not exist: " << path;
		}
	}
	
	void openFileDialog() {
		ofFileDialogResult result = ofSystemLoadDialog("Select Video File", false, "");
		
		if(result.bSuccess) {
			std::string selectedPath = result.getPath();
			
			fileIndex = 0;
			manualPath = "";
			
			std::string filename = ofFilePath::getFileName(selectedPath);
			loadVideoFile(selectedPath, filename);
		}
	}
	
	// New threaded decoding methods
	void decodeThreadFunction() {
		ofLogNotice("videoPlayer2") << "Decode thread started";
		
		while(!shouldExit) {
			std::unique_lock<std::mutex> lock(bufferMutex);
			
			// Wait if buffer is full or video not playing/loaded
			frameNeededCondition.wait(lock, [this] {
				return shouldExit ||
					   (isVideoLoaded &&
						decodedFrames.size() < bufferSize &&
						play);
			});
			
			if(shouldExit) break;
			
			lock.unlock();
			
			// Initialize decoder if needed
			if(isVideoLoaded && !isDecoderReady) {
				initializeDecoder();
			}
			
			// Decode frames only if decoder is ready
			if(isDecoderReady) {
				decodeNextFrames();
			}
		}
		
		ofLogNotice("videoPlayer2") << "Decode thread ended";
	}
	
	void initializeDecoder() {
		std::lock_guard<std::mutex> decoderLock(decoderMutex);
		
		if(!currentLoadedFile.empty() && !decoderPlayer.isLoaded()) {
			ofLogNotice("videoPlayer2") << "Initializing decoder for: " << currentLoadedFile;
			
			decoderPlayer.setUseTexture(false); // We only need pixels
			bool success = decoderPlayer.load(currentLoadedFile);
			
			if(success) {
				decoderPlayer.play();
				decoderPlayer.setPaused(true);
				decoderPlayer.setLoopState(loop ? OF_LOOP_NORMAL : OF_LOOP_NONE);
				isDecoderReady = true;
				ofLogNotice("videoPlayer2") << "Decoder initialized successfully";
			} else {
				ofLogError("videoPlayer2") << "Failed to initialize decoder";
				isDecoderReady = false;
			}
		}
	}
	
	void decodeNextFrames() {
		std::lock_guard<std::mutex> decoderLock(decoderMutex);
		
		if(!decoderPlayer.isLoaded() || !isVideoLoaded) return;
		
		// Get current playback position
		float currentPos = vPlayer.getPosition();
		
		// Calculate how many frames we need
		int currentBufferSize = 0;
		{
			std::lock_guard<std::mutex> bufferLock(bufferMutex);
			currentBufferSize = decodedFrames.size();
		}
		
		int framesToDecode = std::min(3, bufferSize - currentBufferSize);
		
		for(int i = 0; i < framesToDecode; i++) {
			// Calculate target frame position
			float frameTime = 1.0f / vPlayer.getTotalNumFrames();
			float targetPos = currentPos + (frameTime * (currentBufferSize + i + 1));
			
			// Handle looping
			if(targetPos > 1.0 && loop) {
				targetPos -= 1.0;
			}
			
			// Seek decoder to target position
			decoderPlayer.setPosition(targetPos);
			decoderPlayer.update();
			
			// Get pixels
			VideoFrame frame;
			frame.pixels = decoderPlayer.getPixels();
			frame.position = targetPos;
			frame.frameNumber = decoderPlayer.getCurrentFrame();
			frame.timestamp = ofGetElapsedTimeMillis();
			
			// Add to buffer
			{
				std::lock_guard<std::mutex> bufferLock(bufferMutex);
				decodedFrames.push(frame);
			}
		}
	}
	
	void clearBuffer() {
		while(!decodedFrames.empty()) {
			decodedFrames.pop();
		}
	}
	
	void updateBufferStatus() {
		std::stringstream ss;
		int size = decodedFrames.size();
		ss << size << "/" << bufferSize;
		
		if(size == 0) {
			ss << " (Empty)";
		} else if(size >= bufferSize) {
			ss << " (Full)";
		} else if(size < bufferSize / 3) {
			ss << " (Low)";
		}
		
		bufferStatus = ss.str();
	}
	
	void setupListeners() {
		// File dropdown listener
		listeners.push(fileIndex.newListener([this](int &i){
			if(i == 0) {
				isVideoLoaded = false;
				currentLoadedFile = "";
				currentFile = "None";
				vPlayer.close();
			} else {
				std::string filename = movieFiles[i];
				std::string fullPath = "Movies/" + filename;
				loadVideoFile(fullPath, filename);
			}
		}));
		
		// Manual path listener
		listeners.push(manualPath.newListener([this](std::string &path){
			if(!path.empty()) {
				fileIndex = 0;
				std::string filename = ofFilePath::getFileName(path);
				loadVideoFile(path, filename);
			}
		}));
		
		// Browse button listener
		listeners.push(browsePath.newListener([this](void){
			openFileDialog();
		}));
		
		// Loop listener
		listeners.push(loop.newListener([this](bool &b){
			if(vPlayer.isLoaded()) {
				vPlayer.setLoopState(b ? OF_LOOP_NORMAL : OF_LOOP_NONE);
			}
			if(decoderPlayer.isLoaded()) {
				std::lock_guard<std::mutex> lock(decoderMutex);
				decoderPlayer.setLoopState(b ? OF_LOOP_NORMAL : OF_LOOP_NONE);
			}
		}));
		
		// Play listener
		listeners.push(play.newListener([this](bool &b){
			if(vPlayer.isLoaded()) {
				if(b) {
					vPlayer.play();
					frameNeededCondition.notify_one(); // Wake decode thread
				} else {
					vPlayer.stop();
				}
			}
		}));
		
		// Speed listener
		listeners.push(speed.newListener([this](float &f){
			if(vPlayer.isLoaded()) {
				vPlayer.setSpeed(f);
			}
		}));
		
		// Position listener
		listeners.push(position.newListener([this](float &f){
			if(!positionSetByItself && vPlayer.isLoaded()){
				requestedPosition = f;
				
				// Clear buffer on seek
				std::lock_guard<std::mutex> lock(bufferMutex);
				clearBuffer();
			}
			positionSetByItself = false;
		}));
		
		// Buffer size listener
		listeners.push(bufferSize.newListener([this](int &size) {
			// Clear and resize buffer when size changes
			std::lock_guard<std::mutex> lock(bufferMutex);
			clearBuffer();
			frameNeededCondition.notify_one();
		}));
	}
};

#endif /* videoPlayer2_h */
