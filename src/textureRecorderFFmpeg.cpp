//
//  textureRecorderFFmpeg.cpp
//  ofxOceanodeTextures
//

#include "textureRecorderFFmpeg.h"

#include <algorithm>
#include <utility>

namespace {
std::string shellQuote(const std::string &value){
    std::string quoted = "'";
    for(char c : value){
        if(c == '\'') quoted += "'\\''";
        else quoted += c;
    }
    return quoted + "'";
}
}

textureRecorderFFmpeg::textureRecorderFFmpeg() : ofxOceanodeNodeModel("Texture Recorder FFmpeg"){
    addParameter(phasorIn.set("Phase", 0, 0, 1));
    addParameter(record.set("Record", false));
    addParameter(autoRecLoop.set("Auto.Rec", false));
    addParameter(filename.set("File", "recTest"));
    addParameter(input.set("Input", nullptr));
    addParameter(recordAlpha.set("Alpha?", false));

    addParameterDropdown(codec, "Codec", 0,
        {"ProRes 422", "ProRes 4444", "ProRes (HW)", "H.264", "HEVC (HW)"});
    addInspectorParameter(frameRate.set("FPS", 60, 1, 240));
    addInspectorParameter(ffmpegPath.set("ffmpeg", "/opt/homebrew/bin/ffmpeg"));
    addOutputParameter(status.set("Status", ""));

    listeners.push(phasorIn.newListener(this, &textureRecorderFFmpeg::phasorInListener));
    listeners.push(record.newListener(this, &textureRecorderFFmpeg::recordListener));
    listeners.push(input.newListener(this, &textureRecorderFFmpeg::inputListener));
}

textureRecorderFFmpeg::~textureRecorderFFmpeg(){
    stopPipe();
}

void textureRecorderFFmpeg::phasorInListener(float &f){
    if(autoRecLoop){
        if(f < oldPhasor){
            if(!record) record = true;
            else record = false;
        }
    }
    oldPhasor = f;
}

std::string textureRecorderFFmpeg::resolveFfmpeg() const {
    if(!ffmpegPath.get().empty() && ofFile::doesFileExist(ffmpegPath.get())) return ffmpegPath.get();
    for(const std::string &candidate : {"/opt/homebrew/bin/ffmpeg", "/usr/local/bin/ffmpeg", "/usr/bin/ffmpeg"}){
        if(ofFile::doesFileExist(candidate)) return candidate;
    }
    return "ffmpeg"; // fall back to PATH
}

std::string textureRecorderFFmpeg::buildCommand(const std::string &outPath, int w, int h) const {
    const bool alpha = recordAlpha.get();
    std::string cmd = shellQuote(resolveFfmpeg());
    cmd += " -y -f rawvideo -pix_fmt ";
    cmd += (alpha ? "rgba" : "rgb24");
    cmd += " -s " + ofToString(w) + "x" + ofToString(h);
    cmd += " -r " + ofToString(std::max(1.0f, frameRate.get()), 4);
    cmd += " -i -";

    switch(codec.get()){
        case 0: cmd += " -c:v prores_ks -profile:v 3 -pix_fmt yuv422p10le"; break;
        case 1: cmd += " -c:v prores_ks -profile:v 4 -pix_fmt yuva444p10le"; break;
        case 2: cmd += " -c:v prores_videotoolbox -profile:v 3"; break;
        case 3: cmd += " -c:v libx264 -preset fast -crf 18 -pix_fmt yuv420p"; break;
        default: cmd += " -c:v hevc_videotoolbox -tag:v hvc1"; break;
    }
    // ffmpeg's own progress output would flood the console; its errors still
    // reach stderr, which is where a failure needs to be visible.
    cmd += " -loglevel error " + shellQuote(outPath);
    return cmd;
}

bool textureRecorderFFmpeg::startPipe(int w, int h){
    if(pipe != nullptr) return true;

    const std::string folder = ofToDataPath("recordings", true);
    ofDirectory::createDirectory(folder, true, true);
    const std::string ext = (codec.get() == 3) ? ".mp4" : ((codec.get() == 4) ? ".mp4" : ".mov");
    outputPath = folder + "/" + filename.get() + "_" + ofGetTimestampString() + ext;

    const std::string command = buildCommand(outputPath, w, h);
    ofLogNotice("textureRecorderFFmpeg") << "Launching: " << command;

    pipe = popen(command.c_str(), "w");
    if(pipe == nullptr){
        ofLogError("textureRecorderFFmpeg") << "Could not start ffmpeg. Check the path in the inspector.";
        status = "ffmpeg failed to start";
        return false;
    }

    pipeBroken = false;
    frameCounter = 0;
    writerRunning = true;
    writer = std::thread(&textureRecorderFFmpeg::writerLoop, this);
    status = "recording -> " + ofFilePath::getFileName(outputPath);
    return true;
}

void textureRecorderFFmpeg::stopPipe(){
    if(!writerRunning.load() && pipe == nullptr) return;

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        writerRunning = false;
    }
    workAvailable.notify_all();
    spaceAvailable.notify_all();
    if(writer.joinable()) writer.join();

    if(pipe != nullptr){
        // Closing stdin is how ffmpeg is told the stream ended; pclose then
        // waits for it to finish writing the container.
        const int result = pclose(pipe);
        pipe = nullptr;
        if(result != 0){
            ofLogError("textureRecorderFFmpeg") << "ffmpeg exited with " << result;
            status = "ffmpeg exited with " + ofToString(result);
        }else{
            ofLogNotice("textureRecorderFFmpeg") << "Wrote " << frameCounter << " frames to " << outputPath;
            status = ofToString(frameCounter) + " frames -> " + ofFilePath::getFileName(outputPath);
        }
    }

    std::lock_guard<std::mutex> lock(queueMutex);
    frameQueue.clear();
    bufferPool.clear();
}

ofPixels textureRecorderFFmpeg::acquireBuffer(){
    std::lock_guard<std::mutex> lock(queueMutex);
    if(bufferPool.empty()) return ofPixels();
    ofPixels buffer = std::move(bufferPool.back());
    bufferPool.pop_back();
    return buffer;
}

void textureRecorderFFmpeg::recycleBuffer(ofPixels &&pixels){
    std::lock_guard<std::mutex> lock(queueMutex);
    if(bufferPool.size() < maxQueuedFrames + 2) bufferPool.push_back(std::move(pixels));
}

void textureRecorderFFmpeg::writerLoop(){
    while(true){
        ofPixels frame;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            workAvailable.wait(lock, [this]{
                return !frameQueue.empty() || !writerRunning.load();
            });
            // Drain whatever is queued even after a stop, so the file contains
            // every frame that was captured.
            if(frameQueue.empty()) return;
            frame = std::move(frameQueue.front());
            frameQueue.pop_front();
        }
        spaceAvailable.notify_one();

        if(pipe != nullptr && !pipeBroken.load()){
            const std::size_t bytes = frame.size();
            if(fwrite(frame.getData(), 1, bytes, pipe) != bytes){
                // Almost always means ffmpeg died: a bad codec argument, or no
                // permission to write the output.
                pipeBroken = true;
                ofLogError("textureRecorderFFmpeg") << "ffmpeg stopped accepting frames";
            }
        }
        recycleBuffer(std::move(frame));
    }
}

void textureRecorderFFmpeg::inputListener(ofTexture* &texture){
    if(input == nullptr) return;

    const int inWidth = input.get()->getWidth();
    const int inHeight = input.get()->getHeight();
    if(!recorderIsSetup || inWidth != width || inHeight != height){
        // Resolution cannot change mid-file: ffmpeg was told the frame size up
        // front, so a resize has to close the current recording.
        if(pipe != nullptr){
            ofLogWarning("textureRecorderFFmpeg") << "Input resolution changed; closing the current file";
            stopPipe();
            record = false;
            return;
        }
        width = inWidth;
        height = inHeight;
        fbo.allocate(width, height, recordAlpha ? GL_RGBA8 : GL_RGB8);
        fbo.begin();
        ofClear(0, 0, 0, recordAlpha ? 0 : 255);
        input.get()->draw(0, 0);
        fbo.end();
        recorderIsSetup = true;
    }

    if(!record) return;
    if(pipe == nullptr && !startPipe(width, height)){
        record = false;
        return;
    }
    if(pipeBroken.load()){
        record = false;
        return;
    }

    fbo.begin();
    ofClear(0, 0, 0, recordAlpha ? 0 : 255);
    input.get()->draw(0, 0);
    fbo.end();

    ofPixels pixels = acquireBuffer();
    fbo.getTexture().readToPixels(pixels);

    {
        std::unique_lock<std::mutex> lock(queueMutex);
        spaceAvailable.wait(lock, [this]{
            return frameQueue.size() < maxQueuedFrames || !writerRunning.load();
        });
        if(!writerRunning.load()) return;
        frameQueue.push_back(std::move(pixels));
    }
    workAvailable.notify_one();
    frameCounter++;
}

void textureRecorderFFmpeg::recordListener(bool &b){
    if(b){
        setFlags(ofxOceanodeNodeModelFlags_ForceFrameMode);
        // The pipe itself is opened on the first frame, once the input
        // resolution is known.
    }else{
        autoRecLoop = false;
        setFlags(ofxOceanodeNodeModelFlags_None);
        stopPipe();
        recorderIsSetup = false;
    }
}
