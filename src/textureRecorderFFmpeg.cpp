//
//  textureRecorderFFmpeg.cpp
//  ofxOceanodeTextures
//

#include "textureRecorderFFmpeg.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
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
    addSeparator("Capture");
    addParameter(phasorIn.set("Phase", 0, 0, 1));
    addParameter(record.set("Record", false));
    addParameter(autoRecLoop.set("Auto.Rec", false));
    addParameter(filename.set("File", "recTest"));
    addInspectorParameter(numInputs.set("Num Inputs", 1, 1, maxInputs));

    inputs.reserve(maxInputs);
    streams.reserve(maxInputs);
    resizeInputs(numInputs.get());

    addParameter(recordAlpha.set("Alpha?", false));

    addParameterDropdown(codec, "Codec", ProRes422Software,
        {"ProRes 422",
         "ProRes 4444",
         "ProRes 422 HQ (HW)",
         "H.264",
         "HEVC Main (HW)",
         "ProRes 422 Proxy (HW)",
         "ProRes 422 LT (HW)",
         "ProRes 422 (HW)",
         "ProRes 4444 (HW)",
         "ProRes 4444 XQ (HW)",
         "H.264 High (HW)",
         "HEVC Main 10 (HW)",
         "HEVC 422 10-bit (HW)",
         "HEVC Alpha (HW)"});

    addSeparator("Audio");
    addParameter(wavPath.set("WAV", ""));
    addParameter(embed.set("Embed"), ofxOceanodeParameterFlags_DisableSavePreset);

    addInspectorParameter(frameRate.set("FPS", 60, 1, 240));
    addInspectorParameter(ffmpegPath.set("ffmpeg", "/opt/homebrew/bin/ffmpeg"));
    addSeparator("Output");
    addOutputParameter(status.set("Status", ""));

    listeners.push(phasorIn.newListener(this, &textureRecorderFFmpeg::phasorInListener));
    listeners.push(record.newListener(this, &textureRecorderFFmpeg::recordListener));
    listeners.push(embed.newListener([this](){ embedListener(); }));
    listeners.push(numInputs.newListener([this](int &newSize){
        if(record.get()) record = false;
        resizeInputs(newSize);
    }));
    listeners.push(recordAlpha.newListener([this](bool &){
        // The pipe and FBO channel count must agree for the entire file.
        if(record.get()) record = false;
        resetStreamSetups();
    }));
    listeners.push(codec.newListener([this](int &){
        // All files in a recording group must use the same encoder.
        if(record.get()) record = false;
    }));
}

textureRecorderFFmpeg::~textureRecorderFFmpeg(){
    stopAllPipes();
    if(embedThread.joinable()) embedThread.join();
}

void textureRecorderFFmpeg::update(ofEventArgs &){
    if(embedThread.joinable() && !embedRunning.load()){
        embedThread.join();
        std::lock_guard<std::mutex> lock(embedResultMutex);
        status = embedResult;
    }
}

void textureRecorderFFmpeg::loadBeforeConnections(ofJson &json){
    // Dynamic inputs must exist before Oceanode restores their connections.
    deserializeParameter(json, numInputs);
}

std::string textureRecorderFFmpeg::inputName(std::size_t index) const{
    // Keep the original port name for existing presets and patches.
    if(index == 0) return "Input";
    return "Input " + ofToString(index + 1, 2, '0');
}

void textureRecorderFFmpeg::resizeInputs(int newSize){
    newSize = ofClamp(newSize, 1, maxInputs);
    const std::size_t targetSize = static_cast<std::size_t>(newSize);

    while(inputs.size() > targetSize){
        const std::size_t index = inputs.size() - 1;
        stopPipe(*streams[index], index);
        inputListeners.pop_back();
        removeParameter(inputName(index));
        inputs.pop_back();
        streams.pop_back();
    }

    while(inputs.size() < targetSize){
        const std::size_t index = inputs.size();
        inputs.emplace_back();
        streams.emplace_back(std::make_unique<StreamState>());
        addParameter(inputs.back().set(inputName(index), nullptr));
        inputListeners.emplace_back(inputs.back().newListener(
            [this, index](ofTexture* &texture){ inputListener(index, texture); }));
    }
}

void textureRecorderFFmpeg::resetStreamSetups(){
    for(auto &stream : streams){
        stream->recorderIsSetup = false;
        stream->width = 0;
        stream->height = 0;
        stream->fbo.clear();
    }
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

std::string textureRecorderFFmpeg::resolveInputPath(const std::string &path) const{
    if(path.empty()) return "";
    if(ofFile::doesFileExist(path)) return ofFilePath::getAbsolutePath(path);

    const std::string dataPath = ofToDataPath(path, true);
    if(ofFile::doesFileExist(dataPath)) return dataPath;
    return "";
}

std::string textureRecorderFFmpeg::outputExtension() const{
    switch(codec.get()){
        case H264Software:
        case HEVCMainHardware:
        case H264HighHardware:
        case HEVCMain10Hardware:
            return ".mp4";
        default:
            // ProRes, HEVC 4:2:2 and HEVC alpha have the broadest Apple
            // compatibility in a QuickTime container.
            return ".mov";
    }
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
        case ProRes422Software:
            cmd += " -c:v prores_ks -profile:v 3 -pix_fmt yuv422p10le";
            break;
        case ProRes4444Software:
            cmd += " -c:v prores_ks -profile:v 4 -pix_fmt yuva444p10le";
            break;
        case ProRes422HQHardware:
            cmd += " -c:v prores_videotoolbox -profile:v 3";
            break;
        case H264Software:
            cmd += " -c:v libx264 -preset fast -crf 18 -pix_fmt yuv420p";
            break;
        case HEVCMainHardware:
            cmd += " -c:v hevc_videotoolbox -profile:v main -pix_fmt yuv420p -tag:v hvc1";
            break;
        case ProRes422ProxyHardware:
            cmd += " -c:v prores_videotoolbox -profile:v 0";
            break;
        case ProRes422LTHardware:
            cmd += " -c:v prores_videotoolbox -profile:v 1";
            break;
        case ProRes422StandardHardware:
            cmd += " -c:v prores_videotoolbox -profile:v 2";
            break;
        case ProRes4444Hardware:
            cmd += " -c:v prores_videotoolbox -profile:v 4 -pix_fmt bgra";
            break;
        case ProRes4444XQHardware:
            cmd += " -c:v prores_videotoolbox -profile:v 5 -pix_fmt bgra";
            break;
        case H264HighHardware:
            cmd += " -c:v h264_videotoolbox -profile:v high -pix_fmt yuv420p";
            break;
        case HEVCMain10Hardware:
            cmd += " -c:v hevc_videotoolbox -profile:v main10 -pix_fmt p010le -tag:v hvc1";
            break;
        case HEVCMain42210Hardware:
            cmd += " -c:v hevc_videotoolbox -profile:v main42210 -pix_fmt p210le -tag:v hvc1";
            break;
        case HEVCAlphaHardware:
            cmd += " -c:v hevc_videotoolbox -pix_fmt bgra -alpha_quality 1 -tag:v hvc1";
            break;
        default:
            cmd += " -c:v prores_ks -profile:v 3 -pix_fmt yuv422p10le";
            break;
    }
    // ffmpeg's own progress output would flood the console; its errors still
    // reach stderr, which is where a failure needs to be visible.
    cmd += " -loglevel error " + shellQuote(outPath);
    return cmd;
}

bool textureRecorderFFmpeg::startPipe(StreamState &stream, std::size_t index, int w, int h){
    if(stream.pipe != nullptr) return true;

    const std::string folder = ofToDataPath("recordings", true);
    ofDirectory::createDirectory(folder, true, true);
    if(recordingTimestamp.empty()) recordingTimestamp = ofGetTimestampString();

    const std::string suffix = streams.size() == 1
        ? ""
        : "_" + ofToString(index + 1, 2, '0');
    stream.outputPath = folder + "/" + filename.get() + "_" + recordingTimestamp + suffix + outputExtension();

    const std::string command = buildCommand(stream.outputPath, w, h);
    ofLogNotice("textureRecorderFFmpeg") << "Launching input " << (index + 1) << ": " << command;

    stream.pipe = popen(command.c_str(), "w");
    if(stream.pipe == nullptr){
        ofLogError("textureRecorderFFmpeg") << "Could not start ffmpeg for input " << (index + 1)
                                             << ". Check the path in the inspector.";
        status = "ffmpeg failed for input " + ofToString(index + 1);
        return false;
    }

    stream.pipeBroken = false;
    stream.frameCounter = 0;
    stream.writerRunning = true;
    stream.writer = std::thread(&textureRecorderFFmpeg::writerLoop, this, &stream);

    if(streams.size() == 1){
        status = "recording -> " + ofFilePath::getFileName(stream.outputPath);
    }else{
        int activeStreams = 0;
        for(const auto &candidate : streams){
            if(candidate->pipe != nullptr) activeStreams++;
        }
        status = "recording " + ofToString(activeStreams) + "/" + ofToString(streams.size()) + " streams";
    }
    return true;
}

int textureRecorderFFmpeg::stopPipe(StreamState &stream, std::size_t index){
    if(!stream.writerRunning.load() && stream.pipe == nullptr) return -1;

    {
        std::lock_guard<std::mutex> lock(stream.queueMutex);
        stream.writerRunning = false;
    }
    stream.workAvailable.notify_all();
    stream.spaceAvailable.notify_all();
    if(stream.writer.joinable()) stream.writer.join();

    int result = -1;
    if(stream.pipe != nullptr){
        // Closing stdin tells ffmpeg the stream ended; pclose then waits for it
        // to finish writing the container.
        result = pclose(stream.pipe);
        stream.pipe = nullptr;
        if(result != 0){
            ofLogError("textureRecorderFFmpeg") << "ffmpeg for input " << (index + 1)
                                                 << " exited with " << result;
            status = "ffmpeg input " + ofToString(index + 1) + " exited with " + ofToString(result);
        }else{
            ofLogNotice("textureRecorderFFmpeg") << "Wrote " << stream.frameCounter
                                                  << " frames to " << stream.outputPath;
            if(streams.size() == 1){
                status = ofToString(stream.frameCounter) + " frames -> "
                       + ofFilePath::getFileName(stream.outputPath);
            }
        }
    }

    std::lock_guard<std::mutex> lock(stream.queueMutex);
    stream.frameQueue.clear();
    stream.bufferPool.clear();
    return result;
}

void textureRecorderFFmpeg::stopAllPipes(){
    int filesWritten = 0;
    int failures = 0;
    for(std::size_t i = 0; i < streams.size(); i++){
        const int result = stopPipe(*streams[i], i);
        if(result == 0){
            filesWritten++;
            if(!streams[i]->outputPath.empty()) lastRecordedPaths.push_back(streams[i]->outputPath);
        }
        else if(result > 0) failures++;
    }

    if(streams.size() > 1){
        if(failures > 0){
            status = ofToString(failures) + " ffmpeg stream(s) failed";
        }else if(filesWritten > 0){
            status = ofToString(filesWritten) + " files written";
        }
    }
}

int textureRecorderFFmpeg::embedAudio(const std::string &ffmpegExecutable,
                                      const std::string &videoPath,
                                      const std::string &audioPath) const{
    const std::string extension = ofToLower(ofFilePath::getFileExt(videoPath));
    const std::string base = ofFilePath::removeExt(videoPath);
    const std::string tempPath = base + ".audio-" + ofGetTimestampString("%Y%m%d%H%M%S%i")
                               + "." + extension;

    std::string command = shellQuote(ffmpegExecutable);
    command += " -y -nostdin -loglevel error";
    command += " -i " + shellQuote(videoPath);
    command += " -i " + shellQuote(audioPath);
    command += " -map 0:v:0 -map 1:a:0 -c:v copy";
    // QuickTime accepts uncompressed PCM, preserving the NRT render. MP4 does
    // not, so use a high bitrate AAC stream for that container.
    command += extension == "mp4" ? " -c:a aac -b:a 320k" : " -c:a pcm_s24le";
    command += " -map_metadata 0 " + shellQuote(tempPath);

    ofLogNotice("textureRecorderFFmpeg") << "Embedding " << audioPath
                                          << " into " << videoPath;
    const int result = std::system(command.c_str());
    if(result != 0 || !ofFile::doesFileExist(tempPath)){
        ofFile::removeFile(tempPath, false);
        return result == 0 ? -1 : result;
    }

    // The temporary file lives beside the original, so POSIX rename replaces
    // it atomically. Until this succeeds the recorded video remains untouched.
    if(std::rename(tempPath.c_str(), videoPath.c_str()) != 0){
        ofLogError("textureRecorderFFmpeg") << "Could not replace " << videoPath
                                             << ": " << std::strerror(errno);
        ofFile::removeFile(tempPath, false);
        return -1;
    }
    return 0;
}

void textureRecorderFFmpeg::embedListener(){
    if(embedRunning.load()){
        status = "audio embedding is already running";
        return;
    }
    if(record.get()){
        status = "stop recording before embedding audio";
        return;
    }
    if(lastRecordedPaths.empty()){
        status = "no recorded video to embed";
        return;
    }

    const std::string audioPath = resolveInputPath(wavPath.get());
    if(audioPath.empty()){
        status = "WAV file not found";
        return;
    }

    if(embedThread.joinable()) embedThread.join();
    const std::vector<std::string> videos = lastRecordedPaths;
    const std::string ffmpegExecutable = resolveFfmpeg();
    embedRunning = true;
    status = videos.size() == 1 ? "embedding audio" : "embedding audio in " + ofToString(videos.size()) + " videos";
    embedThread = std::thread([this, videos, audioPath, ffmpegExecutable](){
        int completed = 0;
        for(const auto &video : videos){
            if(embedAudio(ffmpegExecutable, video, audioPath) == 0) completed++;
        }

        std::lock_guard<std::mutex> lock(embedResultMutex);
        if(completed == (int)videos.size()){
            embedResult = videos.size() == 1
                ? "audio embedded -> " + ofFilePath::getFileName(videos.front())
                : "audio embedded in " + ofToString(completed) + " videos";
        }else{
            embedResult = "audio embedding failed (" + ofToString(completed) + "/"
                        + ofToString(videos.size()) + ")";
        }
        embedRunning = false;
    });
}

ofPixels textureRecorderFFmpeg::acquireBuffer(StreamState &stream){
    std::lock_guard<std::mutex> lock(stream.queueMutex);
    if(stream.bufferPool.empty()) return ofPixels();
    ofPixels buffer = std::move(stream.bufferPool.back());
    stream.bufferPool.pop_back();
    return buffer;
}

void textureRecorderFFmpeg::recycleBuffer(StreamState &stream, ofPixels &&pixels){
    std::lock_guard<std::mutex> lock(stream.queueMutex);
    if(stream.bufferPool.size() < stream.maxQueuedFrames + 2){
        stream.bufferPool.push_back(std::move(pixels));
    }
}

void textureRecorderFFmpeg::writerLoop(StreamState *stream){
    while(true){
        ofPixels frame;
        {
            std::unique_lock<std::mutex> lock(stream->queueMutex);
            stream->workAvailable.wait(lock, [stream]{
                return !stream->frameQueue.empty() || !stream->writerRunning.load();
            });
            // Drain queued frames after stop so every captured frame reaches
            // its file.
            if(stream->frameQueue.empty()) return;
            frame = std::move(stream->frameQueue.front());
            stream->frameQueue.pop_front();
        }
        stream->spaceAvailable.notify_one();

        if(stream->pipe != nullptr && !stream->pipeBroken.load()){
            const std::size_t bytes = frame.size();
            if(fwrite(frame.getData(), 1, bytes, stream->pipe) != bytes){
                stream->pipeBroken = true;
                ofLogError("textureRecorderFFmpeg") << "ffmpeg stopped accepting frames";
            }
        }
        recycleBuffer(*stream, std::move(frame));
    }
}

void textureRecorderFFmpeg::inputListener(std::size_t index, ofTexture* &texture){
    if(index >= inputs.size() || texture == nullptr || !texture->isAllocated()) return;
    StreamState &stream = *streams[index];

    const int inWidth = texture->getWidth();
    const int inHeight = texture->getHeight();
    if(!stream.recorderIsSetup || inWidth != stream.width || inHeight != stream.height){
        // Resolution cannot change mid-file: ffmpeg was told the frame size up
        // front, so a resize closes every stream to keep the recordings aligned.
        if(stream.pipe != nullptr){
            ofLogWarning("textureRecorderFFmpeg") << "Input " << (index + 1)
                                                   << " resolution changed; closing all current files";
            record = false;
            return;
        }

        stream.width = inWidth;
        stream.height = inHeight;

        // The convenience allocate(width, height, format) overload creates
        // desktop depth and stencil buffers. Explicit settings avoid that
        // substantial, unused allocation for every recording input.
        ofFbo::Settings settings;
        settings.width = stream.width;
        settings.height = stream.height;
        settings.internalformat = recordAlpha ? GL_RGBA8 : GL_RGB8;
        settings.numColorbuffers = 1;
        settings.useDepth = false;
        settings.useStencil = false;
        settings.numSamples = 0;
        settings.minFilter = GL_NEAREST;
        settings.maxFilter = GL_NEAREST;
        stream.fbo.allocate(settings);
        stream.recorderIsSetup = true;
    }

    if(!record) return;
    if(stream.pipe == nullptr && !startPipe(stream, index, stream.width, stream.height)){
        record = false;
        return;
    }
    if(stream.pipeBroken.load()){
        record = false;
        return;
    }

    stream.fbo.begin();
    ofClear(0, 0, 0, recordAlpha ? 0 : 255);
    texture->draw(0, 0);
    stream.fbo.end();

    ofPixels pixels = acquireBuffer(stream);
    stream.fbo.getTexture().readToPixels(pixels);

    {
        std::unique_lock<std::mutex> lock(stream.queueMutex);
        stream.spaceAvailable.wait(lock, [&stream]{
            return stream.frameQueue.size() < stream.maxQueuedFrames || !stream.writerRunning.load();
        });
        if(!stream.writerRunning.load()) return;
        stream.frameQueue.push_back(std::move(pixels));
    }
    stream.workAvailable.notify_one();
    stream.frameCounter++;
}

void textureRecorderFFmpeg::recordListener(bool &b){
    if(b){
        if(embedRunning.load()){
            status = "wait for audio embedding to finish";
            record = false;
            return;
        }
        lastRecordedPaths.clear();
        recordingTimestamp.clear();
        setFlags(ofxOceanodeNodeModelFlags_ForceFrameMode);
        // Each pipe opens on its input's first frame, once its resolution is
        // known. Disconnected inputs simply do not create empty files.
    }else{
        autoRecLoop = false;
        setFlags(ofxOceanodeNodeModelFlags_None);
        stopAllPipes();
        resetStreamSetups();
        recordingTimestamp.clear();
    }
}
