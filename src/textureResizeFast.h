//  textureResize.h
//  PLedNodes
//
//  Hybrid fast path scaler:
//  - Tries glBlitFramebuffer (NEAREST/LINEAR) for GPU-optimized scaling
//  - Falls back to single-quad draw
//  - Uses vertex-sampled per-texel mesh only when upscaling tiny sources a lot
//
//  Notes:
//  * Keeps your parameters and output pointer semantics
//  * No depth/stencil/MSAA; CLAMP_TO_EDGE
//

#ifndef textureResizeFast_h
#define textureResizeFast_h

#include "ofxOceanodeNodeModel.h"
#include "ofMain.h"

class textureResizeFast : public ofxOceanodeNodeModel {
public:
	textureResizeFast() : ofxOceanodeNodeModel("Texture Resizer Fast") {}
	~textureResizeFast() {}

	void setup() override {
		addParameter(input.set("Input", nullptr));
		addParameter(width.set("Width", 100, 1, 16384));
		addParameter(height.set("Height", 100, 1, 16384));
		addParameter(interpolate.set("Interpol.", false));
		addOutputParameter(output.set("Output", nullptr));

		inputSize = {0,0};
		targetSize = {width, height};
		ensureFboAllocated(true);

		setupVertexSampleShader();
	}

	void activate() override {
		ensureFboAllocated(true);
	}

	void deactivate() override {
		fbo.clear();
		readFbo.clear();
		perTexelMesh.clear();
		// output = nullptr; // optional
	}

	void draw(ofEventArgs&) override {
		auto* inTex = input.get();
		if (!inTex) { output = nullptr; return; }

		const int inW = (int)inTex->getWidth();
		const int inH = (int)inTex->getHeight();

		// Track changes
		if (inputSize.x != inW || inputSize.y != inH) {
			inputSize = {inW, inH};
			perTexelMeshBuiltFor = {0,0}; // invalidate cached mesh
			ensureFboAllocated(false, inTex->texData.glInternalFormat);
		}
		if (targetSize.x != width || targetSize.y != height) {
			targetSize = {width, height};
			ensureFboAllocated(true, inTex->texData.glInternalFormat);
		}

		// Passthrough when sizes match
		if (inW == targetSize.x && inH == targetSize.y) {
			output = inTex;
			return;
		}

		// Choose path
		const double areaIn  = std::max(1, inW * inH);
		const double areaOut = std::max(1, targetSize.x * targetSize.y);
		const double upscaleFactor = areaOut / areaIn;

		// 1) Try blit first (fast on many GPUs)
		if (blitScale(*inTex)) {
			output = &fbo.getTexture();
			return;
		}

		// 2) For large upscales from small sources, use vertex-sampled per-texel mesh
		// Guard: only if input is reasonably small (so mesh is cheap)
		const bool hugeUpscale = upscaleFactor >= 8.0;            // area > 8x
		const bool smallInput  = (inW * inH) <= 256 * 256;        // tweak as needed
		if (hugeUpscale && smallInput) {
			drawVertexSampled(*inTex);
			output = &fbo.getTexture();
			return;
		}

		// 3) Fallback: single-quad draw with hardware filter
		drawSingleQuad(*inTex);
		output = &fbo.getTexture();
	}

private:
	// ---------- Allocation ----------
	void ensureFboAllocated(bool force, GLint desiredInternalFormat = -1) {
		const int w = std::max(1, targetSize.x);
		const int h = std::max(1, targetSize.y);

		if (desiredInternalFormat == -1) {
			desiredInternalFormat = fbo.isAllocated()
			  ? fbo.getTexture().texData.glInternalFormat
			  : GL_RGBA32F;
		}

		const bool needAlloc =
			force ||
			!fbo.isAllocated() ||
			fbo.getWidth()  != w ||
			fbo.getHeight() != h ||
			fbo.getTexture().texData.glInternalFormat != desiredInternalFormat;

		if (needAlloc) {
			ofFbo::Settings s;
			s.width = w;
			s.height = h;
			s.internalformat = desiredInternalFormat;
			s.useDepth = false;
			s.useStencil = false;
			s.numSamples = 0;
			s.textureTarget = GL_TEXTURE_2D;
			s.numColorbuffers = 1;
			s.minFilter = GL_NEAREST; // output texture filter; input handled per path
			s.maxFilter = GL_NEAREST;
			s.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
			s.wrapModeVertical   = GL_CLAMP_TO_EDGE;
			fbo.allocate(s);
			fbo.begin(); ofClear(0,0,0,0); fbo.end();
		}

		// Read FBO used to attach arbitrary ofTexture for blit
		if (!readFbo.isAllocated()) {
			ofFbo::Settings sr;
			sr.width = 1; sr.height = 1; // size is irrelevant for texture attachment
			sr.internalformat = desiredInternalFormat;
			sr.useDepth = false; sr.useStencil = false; sr.numSamples = 0;
			sr.textureTarget = GL_TEXTURE_2D;
			sr.numColorbuffers = 1;
			readFbo.allocate(sr);
		}
	}

	// ---------- Path 1: glBlitFramebuffer ----------
	bool blitScale(ofTexture& inTex) {
		// Attach input texture to READ_FBO and our fbo to DRAW_FBO, then blit.
		// Works for NEAREST/LINEAR; very fast on many drivers.
		if (!fbo.isAllocated()) return false;

		// Save current bindings
		GLint prevReadFbo = 0, prevDrawFbo = 0;
		glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFbo);
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDrawFbo);

		// Bind our helper read FBO and attach the texture
		glBindFramebuffer(GL_READ_FRAMEBUFFER, readFbo.getId());
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
							   GL_TEXTURE_2D, inTex.getTextureData().textureID, 0);

		// Bind destination fbo as draw
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo.getId());

		// Check completeness of read FBO
		GLenum status = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			// Restore and bail out (fallback to other paths)
			glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFbo);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFbo);
			return false;
		}

		const GLbitfield mask = GL_COLOR_BUFFER_BIT;
		const GLenum filter = interpolate ? GL_LINEAR : GL_NEAREST;

		const int srcW = (int)inTex.getWidth();
		const int srcH = (int)inTex.getHeight();
		const int dstW = targetSize.x;
		const int dstH = targetSize.y;

		// Perform blit
		glBlitFramebuffer(
			0, 0, srcW, srcH,                 // src rect
			0, 0, dstW, dstH,                 // dst rect
			mask, filter
		);

		// Restore previous bindings
		glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFbo);

		return true;
	}

	// ---------- Path 2: Vertex-sampled mesh (caps tex fetches) ----------
	void setupVertexSampleShader() {
		const std::string vs = R"(#version 410
		uniform mat4 modelViewProjectionMatrix;
		uniform sampler2D tex0;
		in vec4 position;
		in vec2 texcoord;
		noperspective out vec4 vColor;
		void main() {
			gl_Position = modelViewProjectionMatrix * position;
			// Sample once per-vertex at texel centers (texcoord provided by mesh).
			vColor = texture(tex0, texcoord);
		})";

		const std::string fs = R"(#version 410
		noperspective in vec4 vColor;
		out vec4 fragColor;
		void main() { fragColor = vColor; })";

		vertexSampleShader.setupShaderFromSource(GL_VERTEX_SHADER, vs);
		vertexSampleShader.setupShaderFromSource(GL_FRAGMENT_SHADER, fs);
		vertexSampleShader.bindDefaults();
		vertexSampleShader.linkProgram();
	}

	void buildPerTexelMesh(int inW, int inH, int outW, int outH) {
		perTexelMesh.clear();
		perTexelMesh.setMode(OF_PRIMITIVE_TRIANGLES);
		perTexelMesh.enableTextures();

		// Each input texel becomes a constant-colored quad in output
		// UV at the center of that texel for all 4 vertices of the quad.
		const float cellW = float(outW) / float(inW);
		const float cellH = float(outH) / float(inH);

		perTexelMesh.getTexCoords().reserve(inW * inH * 4);
		perTexelMesh.getVertices().reserve(inW * inH * 4);
		perTexelMesh.getIndices().reserve(inW * inH * 6);

		auto idxOf = [](int i){ return (ofIndexType)i; };
		int vertBase = 0;

		for (int y = 0; y < inH; ++y) {
			const float y0 = y * cellH;
			const float y1 = y0 + cellH;
			const float vCenter = (y + 0.5f) / float(inH);
			for (int x = 0; x < inW; ++x) {
				const float x0 = x * cellW;
				const float x1 = x0 + cellW;
				const float uCenter = (x + 0.5f) / float(inW);

				// 4 verts with identical UV (forces constant color across quad)
				perTexelMesh.addVertex({x0, y0, 0});
				perTexelMesh.addVertex({x1, y0, 0});
				perTexelMesh.addVertex({x1, y1, 0});
				perTexelMesh.addVertex({x0, y1, 0});

				perTexelMesh.addTexCoord({uCenter, vCenter});
				perTexelMesh.addTexCoord({uCenter, vCenter});
				perTexelMesh.addTexCoord({uCenter, vCenter});
				perTexelMesh.addTexCoord({uCenter, vCenter});

				perTexelMesh.addIndex(idxOf(vertBase + 0));
				perTexelMesh.addIndex(idxOf(vertBase + 1));
				perTexelMesh.addIndex(idxOf(vertBase + 2));
				perTexelMesh.addIndex(idxOf(vertBase + 0));
				perTexelMesh.addIndex(idxOf(vertBase + 2));
				perTexelMesh.addIndex(idxOf(vertBase + 3));

				vertBase += 4;
			}
		}
		perTexelMeshBuiltFor = {inW, inH};
		lastMeshOutSize = {outW, outH};
	}

	void drawVertexSampled(ofTexture& inTex) {
		const int outW = targetSize.x;
		const int outH = targetSize.y;

		if (perTexelMeshBuiltFor != inputSize || lastMeshOutSize != targetSize) {
			buildPerTexelMesh(inputSize.x, inputSize.y, outW, outH);
		}

		// State
		inTex.setTextureWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		fbo.begin();
		ofClear(0,0,0,0);
		vertexSampleShader.begin();
		vertexSampleShader.setUniformTexture("tex0", inTex, 0);
		perTexelMesh.draw();
		vertexSampleShader.end();
		fbo.end();
	}

	// ---------- Path 3: Single-quad textured draw ----------
	void drawSingleQuad(ofTexture& inTex) {
		// Hardware filter: LINEAR or NEAREST
		inTex.setTextureMinMagFilter(interpolate ? GL_LINEAR : GL_NEAREST,
									 interpolate ? GL_LINEAR : GL_NEAREST);
		inTex.setTextureWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		fbo.begin();
		ofClear(0,0,0,0);
		inTex.draw(0, 0, targetSize.x, targetSize.y);
		fbo.end();
	}

	// ---------- Params & state ----------
	ofParameter<ofTexture*> input;
	ofParameter<bool>       interpolate;
	ofParameter<int>        width;
	ofParameter<int>        height;
	ofParameter<ofTexture*> output;

	ofFbo fbo;        // destination
	ofFbo readFbo;    // helper for attaching arbitrary textures for blit

	glm::ivec2 inputSize {0,0};
	glm::ivec2 targetSize {1,1};

	// Vertex-sampled mesh cache
	ofVboMesh perTexelMesh;
	glm::ivec2 perTexelMeshBuiltFor {0,0};
	glm::ivec2 lastMeshOutSize {0,0};
	ofShader   vertexSampleShader;
};

#endif /* textureResizeFast_h */
