//
//  textureUnitMonitor.h
//  Texture Unit Usage Monitor - Oceanode Node
//
//  Real-time monitoring of OpenGL texture unit usage
//  Helps diagnose GL_INVALID_OPERATION (1282) errors
//

#ifndef textureUnitMonitor_h
#define textureUnitMonitor_h

#include "ofxOceanodeNodeModel.h"

class textureUnitMonitor : public ofxOceanodeNodeModel {
public:
    textureUnitMonitor() : ofxOceanodeNodeModel("Texture Unit Monitor") {}
    
    void setup() {
        // Output parameters
        addOutputParameter(activeUnits.set("Active Units", 0, 0, 256));
        addOutputParameter(maxUnits.set("Max Units", 0, 0, 256));
        addOutputParameter(usagePercent.set("Usage %", 0, 0, 100));
        addOutputParameter(warningFlag.set("Warning", false));
        
        // Control parameters
        addParameter(autoScan.set("Auto Scan", true));
        addParameter(scanRate.set("Scan Rate", 60, 1, 120)); // Hz
        addParameter(warningThreshold.set("Warning %", 75, 50, 95));
        addParameter(printReport.set("Print Report", false));
        addParameter(clearBindings.set("Clear All", false));
        
        // Get initial capabilities
        queryCapabilities();
        
        // Setup listeners
        printReport.addListener(this, &textureUnitMonitor::onPrintReport);
        clearBindings.addListener(this, &textureUnitMonitor::onClearBindings);
        
        frameCounter = 0;
    }
    
    void update(ofEventArgs &e) {
        if(!autoScan) return;
        
        // Scan at specified rate
        frameCounter++;
        int frameInterval = ofGetFrameRate() / scanRate;
        if(frameInterval < 1) frameInterval = 1;
        
        if(frameCounter >= frameInterval) {
            frameCounter = 0;
            scanTextureUnits();
        }
		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR) {
			ofLog() << "OpenGL error: " << err;
		}

    }
    
private:
    // Output parameters
    ofParameter<int> activeUnits;
    ofParameter<int> maxUnits;
    ofParameter<float> usagePercent;
    ofParameter<bool> warningFlag;
    
    // Control parameters
    ofParameter<bool> autoScan;
    ofParameter<int> scanRate;
    ofParameter<float> warningThreshold;
    ofParameter<bool> printReport;
    ofParameter<bool> clearBindings;
    
    // Internal state
    int frameCounter;
    int maxTextureUnits;
    int maxCombinedTextureUnits;
    int maxVertexTextureUnits;
    
    struct TextureUnitState {
        int unit;
        GLuint boundTexture;
        GLenum target;
    };
    
    void queryCapabilities() {
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombinedTextureUnits);
        glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &maxVertexTextureUnits);
        
        maxUnits = maxCombinedTextureUnits;
        
        ofLogNotice("textureUnitMonitor") << "GPU Texture Unit Capabilities:";
        ofLogNotice("textureUnitMonitor") << "  Fragment Shader Units: " << maxTextureUnits;
        ofLogNotice("textureUnitMonitor") << "  Combined Units: " << maxCombinedTextureUnits;
        ofLogNotice("textureUnitMonitor") << "  Vertex Shader Units: " << maxVertexTextureUnits;
    }
    
    std::vector<TextureUnitState> scanTextureUnits() {
        std::vector<TextureUnitState> boundUnits;
        
        // Save current active texture unit
        GLint currentActiveTexture;
        glGetIntegerv(GL_ACTIVE_TEXTURE, &currentActiveTexture);
        
        // Scan all texture units
        for(int i = 0; i < maxCombinedTextureUnits; i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            
            // Check GL_TEXTURE_2D binding
            GLint boundTex2D = 0;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTex2D);
            
            if(boundTex2D != 0) {
                TextureUnitState state;
                state.unit = i;
                state.boundTexture = boundTex2D;
                state.target = GL_TEXTURE_2D;
                boundUnits.push_back(state);
            }
            
            // Check GL_TEXTURE_CUBE_MAP binding
            GLint boundCube = 0;
            glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &boundCube);
            
            if(boundCube != 0) {
                TextureUnitState state;
                state.unit = i;
                state.boundTexture = boundCube;
                state.target = GL_TEXTURE_CUBE_MAP;
                boundUnits.push_back(state);
            }
        }
        
        // Restore original active texture unit
        glActiveTexture(currentActiveTexture);
        
        // Update output parameters
        activeUnits = boundUnits.size();
        float usage = (float)boundUnits.size() / maxCombinedTextureUnits * 100.0f;
        usagePercent = usage;
        warningFlag = (usage >= warningThreshold);
        
        return boundUnits;
    }
    
    void onPrintReport(bool &value) {
        if(!value) return;
        
        ofLogNotice("textureUnitMonitor") << "========================================";
        ofLogNotice("textureUnitMonitor") << "  TEXTURE UNIT DIAGNOSTIC REPORT";
        ofLogNotice("textureUnitMonitor") << "========================================";
        
        // Hardware info
        ofLogNotice("textureUnitMonitor") << "\nHardware Capabilities:";
        ofLogNotice("textureUnitMonitor") << "  Max Fragment Shader Units: " << maxTextureUnits;
        ofLogNotice("textureUnitMonitor") << "  Max Combined Units: " << maxCombinedTextureUnits;
        ofLogNotice("textureUnitMonitor") << "  Max Vertex Shader Units: " << maxVertexTextureUnits;
        
        // Current usage
        std::vector<TextureUnitState> boundUnits = scanTextureUnits();
        ofLogNotice("textureUnitMonitor") << "\nCurrent Usage:";
        ofLogNotice("textureUnitMonitor") << "  Active Units: " << boundUnits.size() << "/" << maxCombinedTextureUnits;
        ofLogNotice("textureUnitMonitor") << "  Usage: " << usagePercent.get() << "%";
        
        if(boundUnits.empty()) {
            ofLogNotice("textureUnitMonitor") << "  No texture units currently bound";
        } else {
            ofLogNotice("textureUnitMonitor") << "\nBound Textures:";
            for(const auto& state : boundUnits) {
                string targetName = (state.target == GL_TEXTURE_2D) ? "2D" : "CUBE";
                ofLogNotice("textureUnitMonitor") << "  Unit " << state.unit 
                    << " (GL_TEXTURE" << state.unit << "): "
                    << "Texture ID " << state.boundTexture 
                    << " [" << targetName << "]";
            }
        }
        
        // Warnings
        if(warningFlag) {
            ofLogWarning("textureUnitMonitor") << "\n⚠️  WARNING: High texture unit usage!";
            ofLogWarning("textureUnitMonitor") << "    This may cause GL_INVALID_OPERATION errors";
            ofLogWarning("textureUnitMonitor") << "    Consider unbinding unused textures";
        }
        
        // Check for OpenGL errors
        GLenum err;
        bool hasErrors = false;
        ofLogNotice("textureUnitMonitor") << "\nOpenGL Error Check:";
        while ((err = glGetError()) != GL_NO_ERROR) {
            hasErrors = true;
            ofLogError("textureUnitMonitor") << "  OpenGL Error: " << err;
            if(err == 1282) {
                ofLogError("textureUnitMonitor") << "    → GL_INVALID_OPERATION (1282)";
                ofLogError("textureUnitMonitor") << "    → Likely cause: Texture unit exhaustion or invalid binding";
            }
        }
        if(!hasErrors) {
            ofLogNotice("textureUnitMonitor") << "  No OpenGL errors detected";
        }
        
        ofLogNotice("textureUnitMonitor") << "========================================\n";
        
        // Reset the button
        printReport = false;
    }
    
    void onClearBindings(bool &value) {
        if(!value) return;
        
        GLint currentActiveTexture;
        glGetIntegerv(GL_ACTIVE_TEXTURE, &currentActiveTexture);
        
        int clearedCount = 0;
        for(int i = 0; i < maxCombinedTextureUnits; i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            
            GLint boundTex2D = 0;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTex2D);
            if(boundTex2D != 0) {
                glBindTexture(GL_TEXTURE_2D, 0);
                clearedCount++;
            }
            
            GLint boundCube = 0;
            glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &boundCube);
            if(boundCube != 0) {
                glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
                clearedCount++;
            }
        }
        
        glActiveTexture(currentActiveTexture);
        
        ofLogNotice("textureUnitMonitor") << "Cleared " << clearedCount << " texture bindings";
        
        // Rescan after clearing
        scanTextureUnits();
        
        // Reset the button
        clearBindings = false;
    }
};

#endif /* textureUnitMonitor_h */
