#pragma once

#include "ofMain.h"
#include "ofxOceanodeNodeModel.h"

class cameraInput : public ofxOceanodeNodeModel {
public:
	cameraInput()
	: ofxOceanodeNodeModel("Camera Input") {

		addParameter(refreshDeviceList.set("Refresh Devices"));
		refreshDeviceList.addListener(this, &cameraInput::onRefreshDevices);

		addParameter(deviceID.set("Device ID", 0, 0, 0));
		deviceID.addListener(this, &cameraInput::onDeviceSelected);

		addParameter(applySettings.set("Apply Settings"));
		applySettings.addListener(this, &cameraInput::onApplySettings);

		addParameter(hFlip.set("H Flip", false));
		hFlip.addListener(this, &cameraInput::onFlipChanged);

		addParameter(vFlip.set("V Flip", false));
		vFlip.addListener(this, &cameraInput::onFlipChanged);

		addParameter(width.set("Width", 640, 1, 3840));
		width.addListener(this, &cameraInput::onResizeChanged);

		addParameter(height.set("Height", 480, 1, 2160));
		height.addListener(this, &cameraInput::onResizeChanged);

		addParameter(deviceNames.set("Devices", {""}, {""}, {""}));

		// Read-only outputs: actual camera resolution
		addOutputParameter(deviceW.set("Device W", 0, 0, 7680));
		addOutputParameter(deviceH.set("Device H", 0, 0, 4320));

		addParameter(textureOut.set("Texture Out", nullptr));
	}

	~cameraInput() override {
		refreshDeviceList.removeListener(this, &cameraInput::onRefreshDevices);
		deviceID.removeListener(this, &cameraInput::onDeviceSelected);
		applySettings.removeListener(this, &cameraInput::onApplySettings);
		hFlip.removeListener(this, &cameraInput::onFlipChanged);
		vFlip.removeListener(this, &cameraInput::onFlipChanged);
		width.removeListener(this, &cameraInput::onResizeChanged);
		height.removeListener(this, &cameraInput::onResizeChanged);
		grabber.close();
	}

	void setup() override {
		refreshDevices();
		scheduleRestart();
	}

	void update(ofEventArgs &e) override {
		if(needsRestart){
			if(restartCountdown > 0){
				restartCountdown--;
				return;
			}
			doRestart();
			needsRestart = false;
		}

		grabber.update();
		if(grabber.isFrameNew()){
			waitingForFirstFrame = false;
			renderToFbo();
			textureOut = &fbo.getTexture();
		}
	}

	void draw(ofEventArgs &e) override {}

private:
	ofParameter<void>            refreshDeviceList;
	ofParameter<int>             deviceID;
	ofParameter<void>            applySettings;
	ofParameter<bool>            hFlip;
	ofParameter<bool>            vFlip;
	ofParameter<int>             width;   // desired output size
	ofParameter<int>             height;
	ofParameter<vector<string>>  deviceNames;
	ofParameter<int>             deviceW; // actual camera resolution (outputs)
	ofParameter<int>             deviceH;
	ofParameter<ofTexture*>      textureOut;

	ofVideoGrabber               grabber;
	ofFbo                        fbo;
	vector<ofVideoDevice>        devices;
	bool                         needsRestart         = false;
	int                          restartCountdown     = 0;
	bool                         waitingForFirstFrame = true;

	// -------------------------------------------------------
	void onRefreshDevices() {
		refreshDevices();
		scheduleRestart();
	}

	void onDeviceSelected(int &) {
		scheduleRestart();
	}

	void onApplySettings() {
		scheduleRestart();
	}

	void onFlipChanged(bool &) {
		if(!waitingForFirstFrame) renderToFbo();
	}

	// Resize FBO when output size changes — no grabber restart needed
	void onResizeChanged(int &) {
		if(fbo.isAllocated() && !waitingForFirstFrame){
			reallocFbo(width.get(), height.get());
			renderToFbo();
		}
	}

	// -------------------------------------------------------
	void scheduleRestart(){
		textureOut = nullptr;
		waitingForFirstFrame = true;
		grabber.close();
		needsRestart     = true;
		restartCountdown = 4;
	}

	void refreshDevices(){
		ofVideoGrabber temp;
		devices = temp.listDevices();

		vector<string> names;
		names.reserve(devices.size());
		for(const auto &d : devices){
			names.emplace_back(ofToString(d.id) + ": " + d.deviceName + (d.bAvailable ? "" : " (Unavailable)"));
		}
		deviceNames = names.empty() ? vector<string>{"No devices found"} : names;

		if(!devices.empty()){
			deviceID.setMin(0);
			deviceID.setMax((int)devices.size() - 1);
			int clamped = ofClamp(deviceID.get(), 0, (int)devices.size() - 1);
			if(clamped != deviceID.get()) deviceID = clamped;
		} else {
			deviceID.setMin(0);
			deviceID.setMax(0);
		}
	}

	void doRestart(){
		grabber.close();

		int id = (!devices.empty()) ? ofClamp(deviceID.get(), 0, (int)devices.size()-1) : 0;
		grabber.setDeviceID(id);
		grabber.setDesiredFrameRate(30);

		// Init at native resolution — let the driver pick what it supports
		bool ok = grabber.initGrabber(3840, 2160);

		if(ok){
			int nativeW = grabber.getWidth();
			int nativeH = grabber.getHeight();

			// Report actual device resolution
			deviceW = nativeW;
			deviceH = nativeH;

			// FBO uses user-requested output size (or native if not set yet)
			int outW = (width.get() > 0)  ? width.get()  : nativeW;
			int outH = (height.get() > 0) ? height.get() : nativeH;
			reallocFbo(outW, outH);

			ofLogNotice("cameraInput") << "Device " << id << ": native "
				<< nativeW << "x" << nativeH << ", output " << outW << "x" << outH;
		} else {
			ofLogError("cameraInput") << "initGrabber failed for device " << id;
		}
	}

	void reallocFbo(int w, int h){
		fbo.allocate(w, h, GL_RGB);
		fbo.begin(); ofClear(0); fbo.end();
	}

	void renderToFbo(){
		if(!fbo.isAllocated()) return;
		fbo.begin();
		ofClear(0,0,0,255);
		float w = fbo.getWidth(), h = fbo.getHeight();
		bool fh = hFlip.get(), fv = vFlip.get();
		ofPushMatrix();
		ofTranslate(fh ? w : 0, fv ? h : 0);
		ofScale(fh ? -1 : 1, fv ? -1 : 1);
		grabber.getTexture().draw(0, 0, w, h);
		ofPopMatrix();
		fbo.end();
	}
};
