#include "effectShaderSource.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace effectShaderSource;

std::string read(const std::filesystem::path &path){
    std::ifstream file(path);
    assert(file.good());
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
void rejects(const std::string &source, const std::string &message){
    const auto errors = validate(source);
    if(std::none_of(errors.begin(), errors.end(), [&](const auto &error){ return error.find(message) != std::string::npos; })){
        std::cerr << "Expected: " << message << "\n";
        for(const auto &error : errors) std::cerr << error << "\n";
        std::abort();
    }
}
int main(int argc, char **argv){
    assert(argc == 2);
    const auto root = std::filesystem::path(argv[1]);
    const auto shader = read(root / "SimpleEffectsLibrary/SimpleEffectTemplate.glsl");
    assert(validate(shader).empty());
    const std::string body = "\n#version 410\nout vec4 out_color;\nvoid main(){out_color=vec4(1);}\n";
    assert(validate("//" + body).empty());
    rejects("#version 410\nvoid main(){}", "Line 1");
    rejects("\n//" + body, "Line 1");
    rejects("//\n// #version 410\nout vec4 out_color;\nvoid main(){}", "#version 410");
    rejects("//\n#version 300 es\nout vec4 out_color;\nvoid main(){}", "#version 410");
    rejects("//\nvoid main(){}\n#version 410", "#version 410");
    rejects("//Amount:1:0:2" + body, "requires uniform float Amount");
    rejects("//Amount:1:0:2" + body + "// uniform float Amount;", "requires uniform float Amount");
    rejects("//Amount:1:0:2" + body + "/* uniform float Amount; */", "requires uniform float Amount");
    rejects("//Amount:nope" + body, "Invalid default");
    rejects("//Amount:nan" + body, "Invalid default");
    rejects("//Amount:1:2:0" + body, "reversed range");
    rejects("//Amount:4:0:2" + body, "out-of-range");
    rejects("//Enabled:bool:maybe" + body, "boolean default");
    rejects("//Tint:color:1:0" + body, "expects Name:color");
    rejects("//Amount:1,Amount:2" + body, "Duplicate or reserved");
    rejects("//Input:1" + body, "Duplicate or reserved");
    rejects("//Shader_Editor:1" + body, "Duplicate or reserved");
    rejects("//Shader_Source:1" + body, "Duplicate or reserved");
    rejects("//modelViewProjectionMatrix:1" + body, "Duplicate or reserved");
    rejects("//tSource:texture" + body, "Duplicate or reserved");
    rejects("//A:1,ATex:texture" + body, "Duplicate or reserved");
    rejects("//Mask:texture,MaskConnected:1" + body, "Duplicate or reserved");
    rejects("//gl_Test:1" + body, "Duplicate or reserved");
    rejects("//" + body + "uniform vec3 uResolution;", "uResolution must");
    rejects("//A:1" + body + "uniform float A; uniform float ATexConnected;", "ATexConnected must");
    rejects("//" + body + "// @swap-dimensions Missing\n", "@swap-dimensions");
    rejects("//\n#version 410\nvoid main(){}\n//out vec4 out_color;", "fragment output");
    assert(validate("//Enabled:bool:true" + body + "uniform bool Enabled;\n// @swap-dimensions Enabled").empty());
    assert(validate("//" + body + "uniform float uTime; uniform vec2 uResolution;").empty());

    std::vector<EffectParameterSpec> specs;
    std::vector<std::string> warnings;
    parseConfiguration(" A : 0.5 : min : max , B:bool:TRUE, C:color:0.1:0.2:0.3:0.4, D:texture", specs, warnings);
    assert(warnings.empty() && specs.size() == 4);
    assert(specs[0].defaultValue == 0.5f && specs[0].minValue == -FLT_MAX && specs[0].maxValue == FLT_MAX);
    assert(specs[1].defaultBool && specs[1].type == ParameterType::Bool);
    assert(specs[2].defaultColor[3] == 0.4f && specs[2].type == ParameterType::Color);
    assert(specs[3].type == ParameterType::Texture);

    rejects("//" + body + "#pragma include <common.inc>", "Inline external includes");
    bool missing = false;
    try{ readStandalone(root / "does-not-exist.glsl"); }catch(const std::runtime_error &){ missing = true; }
    assert(missing);

    int count = 0;
    for(const auto &entry : std::filesystem::recursive_directory_iterator(root / "SimpleEffectsLibrary")){
        if(entry.path().extension() != ".glsl") continue;
        auto errors = validate(readStandalone(entry.path()));
        if(!errors.empty()){
            std::cerr << entry.path() << "\n";
            for(const auto &error : errors) std::cerr << error << "\n";
            return 1;
        }
        ++count;
    }
    std::cout << "PASS shader contract cases and " << count << " library shaders\n";
}
