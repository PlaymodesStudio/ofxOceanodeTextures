//
//  textureRecorderFFmpeg.h
//  ofxOceanodeTextures
//
//  Records a texture straight into a video file by piping raw frames to
//  ffmpeg, instead of writing an image sequence.
//
//  At large resolutions the image-sequence recorder is encode-bound: openFrameworks
//  saves PNGs through FreeImage at its default zlib level and ignores the quality
//  flag for anything but JPEG, so a 15-megapixel frame costs seconds of CPU. Here
//  the frame is handed to ffmpeg as raw bytes and encoded in its own process,
//  which also removes the intermediate files entirely.
//

#ifndef textureRecorderFFmpeg_h
#define textureRecorderFFmpeg_h

#include "ofxOceanodeNodeModel.h"

#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class textureRecorderFFmpeg : public ofxOceanodeNodeModel{
public:
    textureRecorderFFmpeg();
    ~textureRecorderFFmpeg();

private:
    void phasorInListener(float &f);
    void inputListener(ofTexture* &texture);
    void recordListener(bool &b);

    bool startPipe(int w, int h);
    void stopPipe();
    void writerLoop();

    // Frames are large, so the buffers are recycled rather than reallocated
    // every frame.
    ofPixels acquireBuffer();
    void recycleBuffer(ofPixels &&pixels);

    std::string buildCommand(const std::string &outPath, int w, int h) const;
    std::string resolveFfmpeg() const;

    ofParameter<float>      phasorIn;
    ofParameter<bool>       record;
    ofParameter<bool>       autoRecLoop;
    ofParameter<std::string> filename;
    ofParameter<ofTexture*> input;
    ofParameter<bool>       recordAlpha;
    ofParameter<int>        codec;
    ofParameter<float>      frameRate;
    ofParameter<std::string> ffmpegPath;
    ofParameter<std::string> status;

    ofEventListeners listeners;
    ofFbo fbo;

    int width = 0, height = 0;
    bool recorderIsSetup = false;
    float oldPhasor = 0;
    int frameCounter = 0;
    std::string outputPath;

    // One consumer only: a pipe is a stream, so frames must reach it in order.
    // The queue is bounded and a full queue blocks, which is what keeps a
    // frame-stepped transport honest -- it waits rather than dropping a frame.
    FILE* pipe = nullptr;
    std::thread writer;
    std::atomic<bool> writerRunning{false};
    std::atomic<bool> pipeBroken{false};
    std::deque<ofPixels> frameQueue;
    std::vector<ofPixels> bufferPool;
    std::mutex queueMutex;
    std::condition_variable spaceAvailable;
    std::condition_variable workAvailable;
    std::size_t maxQueuedFrames = 4;
};

#endif /* textureRecorderFFmpeg_h */
