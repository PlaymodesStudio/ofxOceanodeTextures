// macOS: verify real GLSL compilation/linking and the starter shader's pixels.
#include "effectShaderSource.h"
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl3.h>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
std::string read(const std::filesystem::path &path){
    std::ifstream stream(path);
    assert(stream.good());
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
GLuint compile(GLenum type, const std::string &source){
    GLuint shader = glCreateShader(type);
    const char *text = source.c_str();
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if(!ok){
        char log[8192] = {};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << log << '\n';
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}
GLuint link(GLuint vertex, GLuint fragment){
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex); glAttachShader(program, fragment);
    glLinkProgram(program);
    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if(!ok){
        char log[8192] = {};
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << log << '\n';
        glDeleteProgram(program);
        return 0;
    }
    return program;
}
int main(int argc, char **argv){
    assert(argc == 2);
    const std::filesystem::path root(argv[1]);
    CGLPixelFormatAttribute attrs[] = {kCGLPFAOpenGLProfile, (CGLPixelFormatAttribute)kCGLOGLPVersion_3_2_Core,
                                      kCGLPFAAccelerated, (CGLPixelFormatAttribute)0};
    CGLPixelFormatObj format = nullptr;
    GLint count = 0;
    if(CGLChoosePixelFormat(attrs, &format, &count) != kCGLNoError || !format){
        std::cerr << "No OpenGL 4.1 context available\n"; return 2;
    }
    CGLContextObj context = nullptr;
    assert(CGLCreateContext(format, nullptr, &context) == kCGLNoError);
    CGLDestroyPixelFormat(format);
    assert(CGLSetCurrentContext(context) == kCGLNoError);
    const std::string vertexSource =
#include "defaultVertexShader.h"
    ;
    const GLuint vertex = compile(GL_VERTEX_SHADER, vertexSource);
    assert(vertex);
    int shaderCount = 0;
    for(const auto &entry : std::filesystem::recursive_directory_iterator(root / "SimpleEffectsLibrary")){
        if(entry.path().extension() != ".glsl") continue;
        const auto fragment = compile(GL_FRAGMENT_SHADER, effectShaderSource::readStandalone(entry.path()));
        if(!fragment){ std::cerr << entry.path() << '\n'; return 1; }
        const auto program = link(vertex, fragment);
        if(!program){ std::cerr << entry.path() << '\n'; return 1; }
        glDeleteProgram(program); glDeleteShader(fragment);
        ++shaderCount;
    }
    const auto fragment = compile(GL_FRAGMENT_SHADER, read(root / "SimpleEffectsLibrary/SimpleEffectTemplate.glsl"));
    const auto program = link(vertex, fragment);
    assert(program);
    glUseProgram(program);
    const GLfloat identity[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    glUniformMatrix4fv(glGetUniformLocation(program, "modelViewProjectionMatrix"), 1, GL_FALSE, identity);
    glUniform2f(glGetUniformLocation(program, "uResolution"), 1, 1);
    glUniform1f(glGetUniformLocation(program, "Amount"), 0.5f);
    glUniform4f(glGetUniformLocation(program, "Tint"), 1, 0.5f, 0.25f, 1);
    glUniform1i(glGetUniformLocation(program, "tSource"), 0);
    glUniform1i(glGetUniformLocation(program, "AmountTex"), 1);
    glUniform1i(glGetUniformLocation(program, "Mask"), 2);
    GLuint textures[4]; glGenTextures(4, textures);
    const float pixels[3][4] = {{0.8f,0.6f,0.4f,0.7f}, {0.75f,0,0,1}, {0.5f,0,0,1}};
    for(int i = 0; i < 4; ++i){
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1, 1, 0, GL_RGBA, GL_FLOAT, i < 3 ? pixels[i] : nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    GLuint fbo; glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[3], 0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glViewport(0, 0, 1, 1);
    GLuint vao, vbo; glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    glGenBuffers(1, &vbo); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    const GLfloat triangle[] = {-1,-1,0,1, 3,-1,0,1, -1,3,0,1};
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    for(int connected = 0; connected < 2; ++connected){
        glUniform1i(glGetUniformLocation(program, "AmountTexConnected"), connected);
        glUniform1i(glGetUniformLocation(program, "MaskConnected"), connected);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        float pixel[4] = {};
        glReadPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, pixel);
        const float amount = connected ? 0.75f * 0.5f : 0.5f;
        const float expected[] = {0.8f * amount, 0.6f * 0.5f * amount, 0.4f * 0.25f * amount, 0.7f};
        for(int c = 0; c < 4; ++c) assert(std::abs(pixel[c] - expected[c]) < 1e-5f);
    }
    assert(glGetError() == GL_NO_ERROR);
    glDeleteBuffers(1, &vbo); glDeleteVertexArrays(1, &vao); glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(4, textures); glDeleteProgram(program); glDeleteShader(fragment); glDeleteShader(vertex);
    CGLSetCurrentContext(nullptr); CGLDestroyContext(context);
    std::cout << "PASS OpenGL 4.1 compile/link: " << shaderCount << " shaders; starter pixels, alpha and optional textures\n";
}
