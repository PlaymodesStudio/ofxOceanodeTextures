//
//  textureReader.h
//  ofxOceanodeTextures
//
//  Created by Eduard Frigola Bagué on 08/02/2021.
//

#ifndef textureReader_h
#define textureReader_h

#include "ofxOceanodeNodeModel.h"

class textureReader : public ofxOceanodeNodeModel{
public:
    textureReader() : ofxOceanodeNodeModel("Texture Reader"){
        addParameter(input.set("Input", nullptr));
        addInspectorParameter(separator.set("Add Separator", false));
        addInspectorParameter(asyncReadback.set("Async Readback", false));
		addOutputParameter(r.set("Red", {0}, {0}, {1}));
		addOutputParameter(g.set("Green", {0}, {0}, {1}));
		addOutputParameter(b.set("Blue", {0}, {0}, {1}));
        addOutputParameter(output.set("Output", {0}, {0}, {1}));
    };
    
    
    void draw(ofEventArgs &a) override{
        ofTexture* tex = input;
        if(!tex || !tex->isAllocated() || tex->getWidth() <= 0 || tex->getHeight() <= 0){
            resetReadback();
            return;
        }
#ifndef TARGET_OPENGLES
        if(asyncReadback && (GLEW_VERSION_3_2 || GLEW_ARB_sync) && (GLEW_VERSION_3_0 || GLEW_ARB_map_buffer_range)){
            drawAsync(*tex);
            return;
        }
#endif
        resetReadback();
        tex->readToPixels(pixels);
        publish(pixels.getData(), pixels.getWidth(), pixels.getHeight(), pixels.getNumChannels());
    }

    void deactivate() override { resetReadback(); }
    ~textureReader(){ resetReadback(); }

private:
    ofParameter<ofTexture*> input;
    ofParameter<vector<float>> output, r, g, b;
    ofParameter<bool> separator, asyncReadback;
    ofFloatPixels pixels;
    
    void publish(const float* data, int width, int height, int channels){
        const size_t count = size_t(width + (separator ? 1 : 0)) * height;
        tempOutput.resize(count); temp_r.resize(count); temp_g.resize(count); temp_b.resize(count);
        size_t j = 0;
        for(int y = 0; y < height; ++y){
            for(int x = 0; x < width; ++x, ++j){
                const float* p = data + (size_t(y) * width + x) * channels;
                temp_r[j] = p[0];
                temp_g[j] = channels >= 3 ? p[1] : p[0];
                temp_b[j] = channels >= 3 ? p[2] : p[0];
                tempOutput[j] = std::max(temp_r[j], std::max(temp_g[j], temp_b[j]));
            }
            if(separator){
                temp_r[j] = temp_g[j] = temp_b[j] = tempOutput[j] = -1;
                ++j;
            }
        }
        r = temp_r; g = temp_g; b = temp_b; output = tempOutput;
    }

#ifndef TARGET_OPENGLES
    struct ReadbackSlot { GLuint buffer = 0; GLsync fence = nullptr; };
    ReadbackSlot slots[2];
    int oldest = 0, pending = 0, readWidth = 0, readHeight = 0;
    GLuint readTexture = 0;
    GLint readFormat = 0;
    bool readSeparator = false;

    void drawAsync(ofTexture& tex){
        const auto& td = tex.getTextureData();
        if(readTexture != td.textureID || readWidth != int(tex.getWidth()) ||
           readHeight != int(tex.getHeight()) || readFormat != td.glInternalFormat || readSeparator != separator.get()){
            resetReadback();
            readTexture = td.textureID; readWidth = tex.getWidth(); readHeight = tex.getHeight();
            readFormat = td.glInternalFormat; readSeparator = separator;
        }
        const size_t bytes = size_t(readWidth) * readHeight * 4 * sizeof(float);
        GLint previousBuffer = 0;
        glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &previousBuffer);
        // Only consume an earlier frame, and only when its fence is ready.
        // A busy GPU holds the previous output instead of blocking the main thread.
        if(pending){
            auto& slot = slots[oldest];
            const GLenum status = glClientWaitSync(slot.fence, 0, 0);
            if(status == GL_WAIT_FAILED){
                resetReadback();
                return;
            }
            if(status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED){
                glBindBuffer(GL_PIXEL_PACK_BUFFER, slot.buffer);
                const auto* data = static_cast<const float*>(glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, bytes, GL_MAP_READ_BIT));
                if(data){
                    // Copy before notifying downstream nodes (which may also use GL).
                    pixels.allocate(readWidth, readHeight, OF_PIXELS_RGBA);
                    std::copy(data, data + size_t(readWidth) * readHeight * 4, pixels.getData());
                    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
                }
                glBindBuffer(GL_PIXEL_PACK_BUFFER, previousBuffer);
                glDeleteSync(slot.fence); slot.fence = nullptr;
                oldest = (oldest + 1) % 2; --pending;
                if(data){
                    const int channels = ofGetNumChannelsFromGLFormat(ofGetGLFormatFromInternal(readFormat));
                    if(channels < 3){
                        for(size_t i = 0; i < pixels.size(); i += 4) pixels[i+1] = pixels[i+2] = pixels[i];
                    }
                    publish(pixels.getData(), readWidth, readHeight, 4);
                }
            }
        }
        if(pending < 2){
            auto& slot = slots[(oldest + pending) % 2];
            if(!slot.buffer){
                glGenBuffers(1, &slot.buffer);
                glBindBuffer(GL_PIXEL_PACK_BUFFER, slot.buffer);
                glBufferData(GL_PIXEL_PACK_BUFFER, bytes, nullptr, GL_STREAM_READ);
            }else glBindBuffer(GL_PIXEL_PACK_BUFFER, slot.buffer);
            GLint previousTexture = 0, alignment = 0, rowLength = 0, skipRows = 0, skipPixels = 0;
            const GLenum binding = td.textureTarget == GL_TEXTURE_RECTANGLE_ARB ? GL_TEXTURE_BINDING_RECTANGLE_ARB : GL_TEXTURE_BINDING_2D;
            glGetIntegerv(binding, &previousTexture);
            glGetIntegerv(GL_PACK_ALIGNMENT, &alignment);
            glGetIntegerv(GL_PACK_ROW_LENGTH, &rowLength);
            glGetIntegerv(GL_PACK_SKIP_ROWS, &skipRows);
            glGetIntegerv(GL_PACK_SKIP_PIXELS, &skipPixels);
            glPixelStorei(GL_PACK_ALIGNMENT, 1); glPixelStorei(GL_PACK_ROW_LENGTH, 0);
            glPixelStorei(GL_PACK_SKIP_ROWS, 0); glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
            glBindTexture(td.textureTarget, td.textureID);
            glGetTexImage(td.textureTarget, 0, GL_RGBA, GL_FLOAT, nullptr);
            slot.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
            glBindTexture(td.textureTarget, previousTexture);
            glPixelStorei(GL_PACK_ALIGNMENT, alignment); glPixelStorei(GL_PACK_ROW_LENGTH, rowLength);
            glPixelStorei(GL_PACK_SKIP_ROWS, skipRows); glPixelStorei(GL_PACK_SKIP_PIXELS, skipPixels);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, previousBuffer);
            // No glFlush here: the frame's swap (or any later driver flush) submits
            // the read, and the fence is only polled on a later draw anyway. An
            // explicit flush per reader per frame cost ~4% of the frame in Lightnet.
            if(slot.fence) ++pending;
        }
    }
#endif
    void resetReadback(){
#ifndef TARGET_OPENGLES
        for(auto& slot : slots){
            if(slot.fence) glDeleteSync(slot.fence);
            if(slot.buffer) glDeleteBuffers(1, &slot.buffer);
            slot = ReadbackSlot{};
        }
        oldest = pending = readWidth = readHeight = 0;
        readTexture = 0;
#endif
    }

    vector<float> tempOutput, temp_r, temp_g, temp_b;
};

#endif /* textureReader_h */
