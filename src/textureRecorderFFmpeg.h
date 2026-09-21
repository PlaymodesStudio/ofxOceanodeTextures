//
//  textureRecorderFFmpeg.h
//  ofxOceanodeTextures
//
//  Records one or more textures straight into independent video files by
//  piping raw frames to ffmpeg, instead of writing image sequences.
//
//  At large resolutions the image-sequence recorder is encode-bound:
//  openFrameworks saves PNGs through FreeImage at its default zlib level and
//  ignores the quality flag for anything but JPEG. Here each frame is handed
//  to ffmpeg as raw bytes and encoded in its own process, which also removes
//  the intermediate files entirely.
//

#ifndef textureRecorderFFmpeg_h
#define textureRecorderFFmpeg_h

#include "ofxOceanodeNodeModel.h"

#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class textureRecorderFFmpeg : public ofxOceanodeNodeModel{
public:
    textureRecorderFFmpeg();
    ~textureRecorderFFmpeg();

    void loadBeforeConnections(ofJson &json) override;

private:
    // Values 0-4 are persisted in existing presets. Keep them stable and add
    // new codec choices after them.
    enum CodecOption : int{
        ProRes422Software = 0,
        ProRes4444Software = 1,
        ProRes422HQHardware = 2,
        H264Software = 3,
        HEVCMainHardware = 4,
        ProRes422ProxyHardware = 5,
        ProRes422LTHardware = 6,
        ProRes422StandardHardware = 7,
        ProRes4444Hardware = 8,
        ProRes4444XQHardware = 9,
        H264HighHardware = 10,
        HEVCMain10Hardware = 11,
        HEVCMain42210Hardware = 12,
        HEVCAlphaHardware = 13
    };

    struct StreamState{
        ofFbo fbo;
        int width = 0;
        int height = 0;
        bool recorderIsSetup = false;
        int frameCounter = 0;
        std::string outputPath;

        // A pipe is an ordered stream, so each output has one consumer. The
        // bounded queue keeps frame-stepped playback honest: a slow encoder
        // makes the producer wait instead of silently dropping frames.
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

    void phasorInListener(float &f);
    void inputListener(std::size_t index, ofTexture* &texture);
    void recordListener(bool &b);
    void resizeInputs(int newSize);
    void resetStreamSetups();

    bool startPipe(StreamState &stream, std::size_t index, int w, int h);
    int stopPipe(StreamState &stream, std::size_t index);
    void stopAllPipes();
    void writerLoop(StreamState *stream);

    // Frames are large, so buffers are moved between the queue and a pool
    // rather than allocated and copied on every frame.
    ofPixels acquireBuffer(StreamState &stream);
    void recycleBuffer(StreamState &stream, ofPixels &&pixels);

    std::string buildCommand(const std::string &outPath, int w, int h) const;
    std::string resolveFfmpeg() const;
    std::string inputName(std::size_t index) const;
    std::string outputExtension() const;

    ofParameter<float>       phasorIn;
    ofParameter<bool>        record;
    ofParameter<bool>        autoRecLoop;
    ofParameter<std::string> filename;
    ofParameter<int>         numInputs;
    ofParameter<bool>        recordAlpha;
    ofParameter<int>         codec;
    ofParameter<float>       frameRate;
    ofParameter<std::string> ffmpegPath;
    ofParameter<std::string> status;

    ofEventListeners listeners;
    std::deque<ofEventListener> inputListeners;
    std::vector<ofParameter<ofTexture*>> inputs;
    std::vector<std::unique_ptr<StreamState>> streams;

    float oldPhasor = 0;
    std::string recordingTimestamp;

    static constexpr int maxInputs = 16;
};

#endif /* textureRecorderFFmpeg_h */
