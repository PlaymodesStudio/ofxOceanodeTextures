//
//  textureRecorder.h
//  MIRABCN_Generator
//
//  Created by Eduard Frigola on 24/05/2018.
//
//

#ifndef textureRecorder_h
#define textureRecorder_h

#include "ofxOceanodeNodeModel.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class textureRecorder : public ofxOceanodeNodeModel{
public:
    textureRecorder();
    ~textureRecorder();

    void draw(ofEventArgs &a);

private:
    // Encoding a 15-megapixel PNG takes far longer than reading the frame off
    // the GPU, and doing it inline stalls the whole graph. The pixels are
    // handed to writer threads instead.
    //
    // The queue is bounded and a full queue BLOCKS rather than dropping: this
    // node is used with a frame-stepped transport, where there is no realtime
    // deadline to miss, so the correct response to "too slow" is to let the
    // transport wait. A dropped frame would silently desynchronise the
    // recording from the audio render.
    struct pendingFrame {
        ofPixels pixels;
        std::string path;
    };

    void startWriters();
    void stopWriters();
    void enqueueFrame(ofPixels &&pixels, const std::string &path);
    void writerLoop();

    std::deque<pendingFrame> writeQueue;
    std::mutex writeMutex;
    std::condition_variable writeSpaceAvailable;
    std::condition_variable writeWorkAvailable;
    std::vector<std::thread> writers;
    std::atomic<bool> writersRunning{false};
    std::size_t maxQueuedFrames;

    void phasorInListener(float &f);
    void inputListener(ofTexture* &texture);
    void recordListener(bool &b);

    ofEventListeners listeners;
    
    ofParameter<bool> createVideo;
    ofParameter<bool> recordAlpha;
    // The frames are written as 8-bit PNGs either way, so a float buffer only
    // costs four times the readback. Kept as a choice for anyone relying on
    // the extra headroom before the 8-bit conversion.
    ofParameter<bool> highPrecision;

    ofParameter<float>  phasorIn;
    ofParameter<bool>   record;
    ofParameter<bool>   autoRecLoop;
    ofParameter<string> filename;
    ofParameter<ofTexture*>    input;

    int width, height;
    int fboInternalFormat;
    bool recorderIsSetup;
    bool lastFrame;

    float oldPhasor;
    int frameCounter;
    string initRecordingTimestamp;

    ofFbo fbo;
};

#endif /* textureRecorder_h */
