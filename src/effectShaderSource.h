#pragma once

// CPU-only contract shared by Simple Effect and the live Shader editor.
#include <algorithm>
#include <array>
#include <cctype>
#include <cfloat>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <cmath>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace effectShaderSource {
inline std::string trim(const std::string &s){
    const auto first = s.find_first_not_of(" \t\r\n");
    return first == std::string::npos ? "" : s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}
inline std::string lower(std::string s){
    for(char &c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
inline std::vector<std::string> split(const std::string &s, const std::string &delimiter,
                                     bool ignoreEmpty = false, bool trimFields = false){
    std::vector<std::string> result;
    size_t start = 0;
    do{
        const auto end = s.find(delimiter, start);
        std::string field = s.substr(start, end == std::string::npos ? end : end - start);
        if(trimFields) field = trim(field);
        if(!ignoreEmpty || !field.empty()) result.push_back(field);
        if(end == std::string::npos) break;
        start = end + delimiter.size();
    }while(true);
    return result;
}
    enum class ParameterType {
        Float,
        Bool,
        Color,
        Texture
    };

    struct EffectParameterSpec {
        std::string name;
        ParameterType type = ParameterType::Float;
        float defaultValue = 0.0f;
        float minValue = -FLT_MAX;
        float maxValue = FLT_MAX;
        bool defaultBool = false;
        std::array<float, 4> defaultColor = {{1, 1, 1, 1}};
    };

    static bool isValidUniformName(const std::string &name){
        if(name.empty() || !(std::isalpha(static_cast<unsigned char>(name.front())) || name.front() == '_')){
            return false;
        }

        for(char c : name){
            if(!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')){
                return false;
            }
        }
        return true;
    }

    static bool parseFiniteFloat(const std::string &text, float &value){
        try{
            size_t parsedCharacters = 0;
            value = std::stof(text, &parsedCharacters);
            return parsedCharacters == text.size() && std::isfinite(value);
        }catch(...){
            return false;
        }
    }

    static void parseConfiguration(const std::string &conf, std::vector<EffectParameterSpec> &effectParameters,
                                   std::vector<std::string> &metadataWarnings){
        effectParameters.clear();
        metadataWarnings.clear();

        const std::string trimmedConfig = trim(conf);
        if(trimmedConfig.empty()){
            return;
        }

        const auto parameterConfigs = split(trimmedConfig, ",", true, true);
        for(const auto &parameterConfig : parameterConfigs){
            const auto fields = split(parameterConfig, ":", false, true);
            if(fields.empty() || !isValidUniformName(fields[0])){
                metadataWarnings.push_back("Invalid parameter name in metadata: " + parameterConfig);
                continue;
            }

            EffectParameterSpec spec;
            spec.name = fields[0];

            if(fields.size() >= 2 && lower(fields[1]) == "bool"){
                spec.type = ParameterType::Bool;
                if(fields.size() != 2 && fields.size() != 3){
                    metadataWarnings.push_back("Boolean parameter " + spec.name + " expects Name:bool[:false|true|0|1]");
                    continue;
                }
                if(fields.size() == 3){
                    const std::string value = lower(fields[2]);
                    if(value != "false" && value != "true" && value != "0" && value != "1"){
                        metadataWarnings.push_back("Invalid boolean default for " + spec.name + ": " + fields[2]);
                        continue;
                    }
                    spec.defaultBool = value == "true" || value == "1";
                }
            }else if(fields.size() >= 2 && lower(fields[1]) == "color"){
                spec.type = ParameterType::Color;
                if(fields.size() != 2 && fields.size() != 5 && fields.size() != 6){
                    metadataWarnings.push_back("Color parameter " + spec.name + " expects Name:color or Name:color:R:G:B[:A]");
                    continue;
                }

                if(fields.size() >= 5){
                    float components[4] = {1.0f, 1.0f, 1.0f, 1.0f};
                    bool validColor = true;
                    const size_t componentCount = fields.size() - 2;
                    for(size_t component = 0; component < componentCount; component++){
                        if(!parseFiniteFloat(fields[component + 2], components[component])){
                            metadataWarnings.push_back("Invalid color component for " + spec.name + ": " + fields[component + 2]);
                            validColor = false;
                            break;
                        }
                        components[component] = std::clamp(components[component], 0.0f, 1.0f);
                    }

                    if(!validColor){
                        continue;
                    }
                    spec.defaultColor = {{components[0], components[1], components[2], components[3]}};
                }
            }else if(fields.size() == 2 && lower(fields[1]) == "texture"){
                spec.type = ParameterType::Texture;
            }else{
                if(fields.size() > 4){
                    metadataWarnings.push_back("Too many fields for float parameter " + spec.name);
                    continue;
                }

                if(fields.size() > 1 && !parseFiniteFloat(fields[1], spec.defaultValue)){
                    metadataWarnings.push_back("Invalid default value for " + spec.name + ": " + fields[1]);
                    continue;
                }

                if(fields.size() > 2 && fields[2] != "min" && !parseFiniteFloat(fields[2], spec.minValue)){
                    metadataWarnings.push_back("Invalid minimum for " + spec.name + ": " + fields[2]);
                    continue;
                }

                if(fields.size() > 3 && fields[3] != "max" && !parseFiniteFloat(fields[3], spec.maxValue)){
                    metadataWarnings.push_back("Invalid maximum for " + spec.name + ": " + fields[3]);
                    continue;
                }

                if(spec.minValue > spec.maxValue){
                    std::swap(spec.minValue, spec.maxValue);
                    metadataWarnings.push_back("Swapped reversed range for " + spec.name);
                }

                if(spec.defaultValue < spec.minValue || spec.defaultValue > spec.maxValue){
                    spec.defaultValue = std::clamp(spec.defaultValue, spec.minValue, spec.maxValue);
                    metadataWarnings.push_back("Clamped out-of-range default for " + spec.name);
                }
            }

            effectParameters.push_back(spec);
        }
    }


// Preserve line boundaries so compiler directives can be checked without
// accepting declarations hidden in either kind of GLSL comment.
inline std::string withoutComments(const std::string &source){
    std::string result = source;
    bool line = false, block = false;
    for(size_t i = 0; i < source.size(); ++i){
        if(line && source[i] == '\n') line = false;
        if(block && source[i] == '*' && i + 1 < source.size() && source[i + 1] == '/'){
            result[i] = result[i + 1] = ' '; ++i; block = false; continue;
        }
        if(!line && !block && i + 1 < source.size() && source[i] == '/'){
            if(source[i + 1] == '/') line = true;
            else if(source[i + 1] == '*') block = true;
        }
        if((line || block) && source[i] != '\n') result[i] = ' ';
    }
    return result;
}
// Match OF's include-once convention while surfacing missing includes instead
// of silently producing an incomplete export. The resulting editor text has
// no dependency on the imported file's location.
inline std::string readStandalone(const std::filesystem::path &path,
                                  std::set<std::filesystem::path> &included, int depth){
    if(depth > 32) throw std::runtime_error("Shader include depth exceeds 32: " + path.string());
    const auto absolute = std::filesystem::weakly_canonical(path);
    if(!included.insert(absolute).second) return "";
    std::ifstream file(absolute);
    if(!file) throw std::runtime_error("Cannot read shader/include: " + absolute.string());
    const std::string source{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    std::istringstream original(source), uncommented(withoutComments(source));
    std::string result, line, codeLine;
    const std::regex includePattern(R"INCLUDE(^\s*#\s*(?:pragma\s+)?include\s*["<]([^">]+)[">]\s*$)INCLUDE");
    while(std::getline(original, line)){
        std::getline(uncommented, codeLine);
        std::smatch match;
        if(std::regex_match(codeLine, match, includePattern))
            result += readStandalone(absolute.parent_path() / match[1].str(), included, depth + 1) + "\n";
        else result += line + "\n";
    }
    return result;
}
inline std::string readStandalone(const std::filesystem::path &path){
    std::set<std::filesystem::path> included;
    return readStandalone(path, included, 0);
}

inline std::string uniformType(ParameterType type){
    switch(type){
        case ParameterType::Float: return "float";
        case ParameterType::Bool: return "bool";
        case ParameterType::Color: return "vec4";
        case ParameterType::Texture: return "sampler2D";
    }
    return "";
}
inline std::string configuration(const std::string &source){
    const std::string first = trim(source.substr(0, source.find('\n')));
    return first.rfind("//", 0) == 0 ? trim(first.substr(2)) : "";
}
inline std::vector<std::string> validate(const std::string &source){
    std::vector<std::string> errors;
    const std::string first = trim(source.substr(0, source.find('\n')));
    std::vector<EffectParameterSpec> specs;
    parseConfiguration(configuration(source), specs, errors);
    // parseConfiguration clears its warnings destination.
    if(first.rfind("//", 0) != 0)
        errors.insert(errors.begin(), "Line 1 must be parameter metadata, e.g. //Amount:1:0:2 (or // for no controls).");
    const auto code = withoutComments(source);
    if(std::regex_search(code, std::regex(R"(#\s*(?:pragma\s+)?include\b)")))
        errors.push_back("Inline external includes before compiling/saving. Load Shader expands includes automatically.");
    const auto lines = split(trim(code), "\n");
    if(lines.empty() || !std::regex_match(trim(lines.front()), std::regex(R"(#\s*version\s+410(\s+core)?\s*)")))
        errors.push_back("Use #version 410 (optionally core) as the first non-comment directive, matching Simple Effect's vertex shader.");
    std::set<std::string> names = {"Input", "Output", "Bypass", "Clear", "Shader_Editor",
        "Draw_On_Event", "Reload_Shader", "Shader_Valid", "Shader_Status",
        "Shader_Source", "Shader_Name", "Load_Shader", "Save_Shader", "Live_Compile",
        "Editor_Lines", "Editor_Width", "Editor_Font_Size", "expanded", "requesty_model",
        "shaderEditorActiveSource", "modelViewProjectionMatrix",
        "Shader_Category", "AI_Description_On_Save", "Shader_Save_Dialog", "Save_Status",
        "Git_Repository", "Git_Branch", "Git_Effects_Folder", "Git_SSH_Key", "Git_Author_Name", "Git_Author_Email",
        "tSource", "tSourceConnected", "tPreviousSource", "tPreviousSourceConnected",
        "tPreviousOutput", "tPreviousOutputConnected", "uResolution", "resolution",
        "uTexelSize", "uTime", "time", "uFrame", "frame"};
    auto reserve = [&](const std::string &name){
        if(!names.insert(name).second || name.rfind("gl_", 0) == 0 || name.find("__") != std::string::npos)
            errors.push_back("Duplicate or reserved parameter/uniform name: " + name);
    };
    for(const auto &spec : specs) reserve(spec.name);
    for(const auto &spec : specs){
        if(spec.type == ParameterType::Float){ reserve(spec.name + "Tex"); reserve(spec.name + "TexConnected"); }
        if(spec.type == ParameterType::Texture) reserve(spec.name + "Connected");
        const std::string type = uniformType(spec.type);
        if(!std::regex_search(code, std::regex("\\buniform\\s+" + type + "\\s+" + spec.name + "\\s*;")))
            errors.push_back("Metadata parameter " + spec.name + " requires uniform " + type + " " + spec.name + ";");
    }
    if(!std::regex_search(code, std::regex(R"(\bout\s+vec4\s+\w+\s*;)")))
        errors.push_back("Declare a fragment output, e.g. out vec4 out_color;.");
    // Other standard uniforms may be omitted when unused, but must have their
    // host-supplied types if declared. Compilation/linking checks main/output.
    const std::vector<std::pair<std::string, std::string>> standard = {
        {"tSource", "sampler2D"}, {"tSourceConnected", "int"}, {"uResolution", "vec2"},
        {"resolution", "vec2"}, {"uTexelSize", "vec2"}, {"uTime", "float"}, {"time", "float"},
        {"uFrame", "int"}, {"frame", "int"}, {"tPreviousSource", "sampler2D"},
        {"tPreviousSourceConnected", "int"}, {"tPreviousOutput", "sampler2D"}, {"tPreviousOutputConnected", "int"}};
    auto checkType = [&](const std::string &name, const std::string &expected){
        std::smatch match;
        if(std::regex_search(code, match, std::regex("\\buniform\\s+(\\w+)\\s+" + name + "\\s*(?:;|\\[)")) && match[1] != expected)
            errors.push_back(name + " must be declared as " + expected + ".");
    };
    for(const auto &entry : standard) checkType(entry.first, entry.second);
    for(const auto &spec : specs){
        if(spec.type == ParameterType::Float){ checkType(spec.name + "Tex", "sampler2D"); checkType(spec.name + "TexConnected", "int"); }
        if(spec.type == ParameterType::Texture) checkType(spec.name + "Connected", "int");
    }
    for(const auto &line : split(source, "\n")){
        const std::string prefix = "// @swap-dimensions ";
        const auto value = trim(line);
        if(value.rfind(prefix, 0) != 0) continue;
        const auto name = trim(value.substr(prefix.size()));
        if(std::none_of(specs.begin(), specs.end(), [&](const auto &s){ return s.name == name && s.type == ParameterType::Bool; }))
            errors.push_back("@swap-dimensions must name a boolean metadata parameter: " + name);
    }
    return errors;
}
} // namespace effectShaderSource
