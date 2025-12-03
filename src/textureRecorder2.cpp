//
//  textureRecorder2.cpp
//  MIRABCN_Generator
//
//  Created by Eduard Frigola on 24/05/2018.
//
//

#include "textureRecorder2.h"

textureRecorder2::textureRecorder2() : ofxOceanodeNodeModel("Texture Recorder 2"){
    addParameter(phasorIn.set("Phase", 0, 0, 1));
    addParameter(record.set("Record", false));
    addParameter(autoRecLoop.set("Auto.Rec", false));
    addParameter(filename.set("File", "recTest"));
    addParameter(input.set("Input", nullptr));
    
    addParameterDropdown(format, "Format", 1, {"None", "GL_RGB8", "GL_RGBA8", "GL_RGB16", "GL_RGBA16", "GL_RGB16F", "GL_RGBA16F", "GL_RGB32F", "GL_RGBA32F"});
    addParameterDropdown(imageFormat, "Image Format", 1, {"JPG", "PNG", "TGA", "TIFF"});
    
    addInspectorParameter(createVideo.set("Create Video", false));

    listeners.push(phasorIn.newListener(this, &textureRecorder2::phasorInListener));
    listeners.push(record.newListener(this, &textureRecorder2::recordListener));
    listeners.push(input.newListener(this, &textureRecorder2::inputListener));
    listeners.push(format.newListener(this, &textureRecorder2::formatListener));
    listeners.push(imageFormat.newListener(this, &textureRecorder2::imageFormatListener));
    oldPhasor = 0;
    frameCounter = 0;
    recorderIsSetup = false;
    lastFrame = false;
}

void textureRecorder2::phasorInListener(float &f){
    if(autoRecLoop){
        if(f < oldPhasor){
            if(!record) record = true;
            else record = false;
        }
    }
    oldPhasor = f;
}

void textureRecorder2::draw(ofEventArgs &a){

}

void textureRecorder2::formatListener(int &f){
    if(f != 0 && recorderIsSetup){
        allocateFboWithFormat(f);
    }
}

void textureRecorder2::imageFormatListener(int &f){
    // Image format listener - can be extended in the future
}

void textureRecorder2::allocateFboWithFormat(int formatIndex){
    if(width > 0 && height > 0){
        ofFboSettings settings;
        settings.width = width;
        settings.height = height;
        
        // Map format index to GL internal format
        switch(formatIndex){
            case 0: // None - default to GL_RGB8
                settings.internalformat = GL_RGB8;
                break;
            case 1: // GL_RGB8
                settings.internalformat = GL_RGB8;
                break;
            case 2: // GL_RGBA8
                settings.internalformat = GL_RGBA8;
                break;
            case 3: // GL_RGB16
                settings.internalformat = GL_RGB16;
                break;
            case 4: // GL_RGBA16
                settings.internalformat = GL_RGBA16;
                break;
            case 5: // GL_RGB16F
                settings.internalformat = GL_RGB16F;
                break;
            case 6: // GL_RGBA16F
                settings.internalformat = GL_RGBA16F;
                break;
            case 7: // GL_RGB32F
                settings.internalformat = GL_RGB32F;
                break;
            case 8: // GL_RGBA32F
                settings.internalformat = GL_RGBA32F;
                break;
            default:
                settings.internalformat = GL_RGB8;
                break;
        }
        
        fbo.allocate(settings);
        fbo.begin();
        if(isRGBAFormat(formatIndex)){
            ofClear(0, 0, 0, 0);  // Clear with full transparency for RGBA
        }else{
            ofClear(0, 0, 0, 255);  // Clear with opaque black for RGB
        }
        if(input != nullptr){
            input.get()->draw(0,0);
        }
        fbo.end();
    }
}

bool textureRecorder2::isRGBAFormat(int formatIndex){
    // RGBA variants are at indices 2, 4, 6, 8
    return (formatIndex == 2 || formatIndex == 4 || formatIndex == 6 || formatIndex == 8);
}

void textureRecorder2::inputListener(ofTexture* &texture){
    if(input != nullptr){
        if(!recorderIsSetup || input.get()->getWidth() != width || input.get()->getHeight() != height){
            width = input.get()->getWidth();
            height = input.get()->getHeight();
            allocateFboWithFormat(format.get());
            recorderIsSetup = true;
        }
        if(record){
            if(input != nullptr){
                fbo.begin();
                if(isRGBAFormat(format.get())){
                    ofClear(0, 0, 0, 0);  // Clear with full transparency before each frame
                }else{
                    ofClear(0, 0, 0, 255);  // Clear with opaque black before each frame
                }
                input.get()->draw(0,0);
                fbo.end();
                // Determine if we're working with a float texture
                bool isFloatFormat = (format.get() >= 5 && format.get() <= 8); // Float formats: GL_RGB16F, GL_RGBA16F, GL_RGB32F, GL_RGBA32F
                
                // Determine file extension and ofImageFormat
                string extension;
                ofImageFormat imageFormatEnum;
                ofImageQualityType quality = OF_IMAGE_QUALITY_BEST;
                
                switch(imageFormat.get()){
                    case 0: // JPG
                        extension = ".jpg";
                        imageFormatEnum = OF_IMAGE_FORMAT_JPEG;
                        break;
                    case 1: // PNG
                        extension = ".png";
                        imageFormatEnum = OF_IMAGE_FORMAT_PNG;
                        break;
                    case 2: // TGA
                        extension = ".tga";
                        imageFormatEnum = OF_IMAGE_FORMAT_TARGA;
                        break;
                    case 3: // TIFF
                        extension = ".tiff";
                        imageFormatEnum = OF_IMAGE_FORMAT_TIFF;
                        break;
                    default:
                        extension = ".png";
                        imageFormatEnum = OF_IMAGE_FORMAT_PNG;
                        break;
                }
                
                string filepath = "recordings/" + filename.get() + "_" + initRecordingTimestamp + "/" + filename.get() + "_" + ofToString(frameCounter, 9, '0') + extension;
                
                // Use ofFloatPixels for float textures to preserve precision
                if(isFloatFormat){
                    ofFloatPixels floatPixels;
                    fbo.getTexture().readToPixels(floatPixels);
                    
                    // Note: TIFF is recommended for float data, but we'll save with user's choice
                    // JPG and other 8-bit formats will quantize float data
                    if(imageFormat.get() == 0 && floatPixels.getNumChannels() == 4){
                        // Convert RGBA to RGB for JPEG
                        ofFloatPixels floatSavePixels;
                        floatSavePixels.allocate(floatPixels.getWidth(), floatPixels.getHeight(), OF_PIXELS_RGB);
                        for(int i = 0; i < floatPixels.getWidth() * floatPixels.getHeight(); i++){
                            floatSavePixels[i * 3] = floatPixels[i * 4];
                            floatSavePixels[i * 3 + 1] = floatPixels[i * 4 + 1];
                            floatSavePixels[i * 3 + 2] = floatPixels[i * 4 + 2];
                        }
                        ofSaveImage(floatSavePixels, filepath, quality);
                    } else {
                        ofSaveImage(floatPixels, filepath, quality);
                    }
                } else {
                    // Use regular ofPixels for 8-bit textures (existing code path)
                    ofPixels pixels;
                    fbo.getTexture().readToPixels(pixels);
                    
                    // Convert from premultiplied to straight alpha if recording with RGBA format
                    if(isRGBAFormat(format.get()) && pixels.getNumChannels() == 4){
                        for(int i = 0; i < pixels.size(); i += 4){
                            float alpha = pixels[i + 3] / 255.0f;
                            if(alpha > 0.0f){
                                // Unpremultiply: divide RGB by alpha
                                pixels[i] = ofClamp(pixels[i] / alpha, 0, 255);
                                pixels[i + 1] = ofClamp(pixels[i + 1] / alpha, 0, 255);
                                pixels[i + 2] = ofClamp(pixels[i + 2] / alpha, 0, 255);
                            }
                        }
                    }
                    
                    // For JPG, convert RGBA to RGB
                    if(imageFormat.get() == 0 && pixels.getNumChannels() == 4){
                        ofPixels savePixels;
                        savePixels.allocate(pixels.getWidth(), pixels.getHeight(), OF_PIXELS_RGB);
                        for(int i = 0; i < pixels.getWidth() * pixels.getHeight(); i++){
                            savePixels[i * 3] = pixels[i * 4];
                            savePixels[i * 3 + 1] = pixels[i * 4 + 1];
                            savePixels[i * 3 + 2] = pixels[i * 4 + 2];
                        }
                        ofSaveImage(savePixels, filepath, quality);
                    } else {
                        ofSaveImage(pixels, filepath, quality);
                    }
                }
//                image.clear();
            }
            frameCounter++;
            if(lastFrame){
                record = false;
                lastFrame = false;
            }
        }
    }
}

void textureRecorder2::recordListener(bool &b){
    if(b){
        initRecordingTimestamp = ofGetTimestampString();
        frameCounter = 0;
        setFlags(ofxOceanodeNodeModelFlags_ForceFrameMode);
    }else{
        autoRecLoop = false;
        recorderIsSetup = false;
        setFlags(ofxOceanodeNodeModelFlags_None);
        if(createVideo){
            // Determine file extension pattern for ffmpeg
            string filePattern;
            switch(imageFormat.get()){
                case 0: filePattern = "*.jpg"; break;
                case 1: filePattern = "*.png"; break;
                case 2: filePattern = "*.tga"; break;
                case 3: filePattern = "*.tiff"; break;
                default: filePattern = "*.png"; break;
            }
            
   string command = "cd " +ofToString("\"") + ofToDataPath("recordings/" + filename.get() +  "_" + initRecordingTimestamp, true) +ofToString("\"");
            if(isRGBAFormat(format.get())){
                command += " && /opt/homebrew/bin/ffmpeg -f image2 -framerate " + ofToString(ofGetTargetFrameRate()) + " -pattern_type glob -i '" + filePattern + "' -c:v prores_ks -profile:v 4444 -pix_fmt yuva444p10le " + filename.get() + ".mov";
            }else{
                command += " && /opt/homebrew/bin/ffmpeg -f image2 -framerate " + ofToString(ofGetTargetFrameRate()) + " -pattern_type glob -i '" + filePattern + "' -c:v prores_ks -profile:v 4 " + filename.get() + ".mov";
            }
            system(command.c_str());
        }
    }
}