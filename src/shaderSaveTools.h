#pragma once

#include "effectShaderSource.h"
#include <map>

namespace shaderSaveTools {
namespace fs = std::filesystem;

inline fs::path relativeFolder(const std::string &value){
    const fs::path path(effectShaderSource::trim(value));
    if(path.is_absolute() || path.has_root_name()) throw std::runtime_error("Choose a folder relative to Effects/the repository root.");
    for(const auto &part : path){
        const auto text = part.string();
        if(text == ".." || effectShaderSource::lower(text) == ".git" || text.find('\\') != std::string::npos ||
           std::any_of(text.begin(), text.end(), [](unsigned char c){ return c < 32; }))
            throw std::runtime_error("Folder paths cannot contain .., .git, backslashes, or control characters.");
    }
    return path.lexically_normal() == "." ? fs::path() : path.lexically_normal();
}
inline fs::path containedPath(const fs::path &root, const fs::path &relative){
    const auto base = fs::weakly_canonical(root);
    auto requested = base;
    for(const auto &part : relativeFolder(relative.generic_string())){
        requested /= part;
        if(fs::is_symlink(requested)) throw std::runtime_error("Shader/README destinations cannot use symlinks: " + requested.string());
    }
    const auto destination = fs::weakly_canonical(requested);
    auto b = base.begin(), d = destination.begin();
    for(; b != base.end(); ++b, ++d)
        if(d == destination.end() || *b != *d) throw std::runtime_error("The chosen path leaves the shader/repository folder (possibly through a symlink).");
    return destination;
}
inline std::string readOptional(const fs::path &path){
    if(!fs::exists(path)) return "";
    std::ifstream file(path, std::ios::binary);
    if(!file) throw std::runtime_error("Cannot read " + path.string());
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
inline void writeFile(const fs::path &path, const std::string &text){
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    file.close();
    if(!file) throw std::runtime_error("Cannot write " + path.string());
}
inline std::string singleLine(std::string text){
    if(text.find('\0') != std::string::npos) throw std::runtime_error("The description contains an invalid NUL character.");
    std::istringstream words(text);
    std::string word, result;
    while(words >> word){ if(!result.empty()) result += ' '; result += word; }
    if(!result.empty() && result.back() == '\\') result += '.';
    if(result.size() > 4096) throw std::runtime_error("The description is too long (maximum 4096 bytes).");
    return result;
}
inline std::string description(const std::string &source){
    std::string result;
    for(const auto &line : effectShaderSource::split(source, "\n")){
        const auto trimmed = effectShaderSource::trim(line);
        const std::string prefix = "// @description ";
        if(trimmed.rfind(prefix, 0) == 0){
            if(!result.empty()) result += ' ';
            result += trimmed.substr(prefix.size());
        }
    }
    return singleLine(result);
}
inline std::string withDescription(const std::string &source, const std::string &value){
    const auto text = singleLine(value);
    if(text.empty()) throw std::runtime_error("The AI returned an empty description.");
    std::istringstream lines(source);
    std::string line, result;
    if(!std::getline(lines, line)) throw std::runtime_error("The shader source is empty.");
    result = line + "\n// @description " + text + "\n";
    while(std::getline(lines, line)){
        const auto trimmed = effectShaderSource::trim(line);
        if(trimmed == "// @description" || trimmed.rfind("// @description ", 0) == 0) continue;
        result += line + '\n';
    }
    return result;
}
inline std::vector<std::string> categories(const fs::path &root){
    std::vector<std::string> result = {""};
    if(fs::exists(root)){
        for(const auto &entry : fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied)){
            if(entry.is_symlink()) continue;
            if(entry.is_directory()) result.push_back(fs::relative(entry.path(), root).generic_string());
        }
    }
    std::sort(result.begin() + 1, result.end());
    return result;
}
using Catalog = std::map<std::string, std::string>;
inline Catalog catalog(const fs::path &root){
    Catalog result;
    if(!fs::exists(root)) return result;
    for(const auto &entry : fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied)){
        if(entry.is_symlink() || !entry.is_regular_file() || effectShaderSource::lower(entry.path().extension().string()) != ".glsl") continue;
        result[fs::relative(entry.path(), root).generic_string()] = description(readOptional(entry.path()));
    }
    return result;
}
inline std::string withoutCatalog(std::string text){
    const std::string begin = "<!-- oceanode-shaders:start -->", end = "<!-- oceanode-shaders:end -->";
    while(true){
        const auto first = text.find(begin);
        if(first == std::string::npos) break;
        const auto last = text.find(end, first);
        if(last == std::string::npos) throw std::runtime_error("README has an unfinished oceanode-shaders catalog block.");
        text.erase(first, last + end.size() - first);
    }
    return effectShaderSource::trim(text);
}
inline std::string markdownText(const std::string &text){
    std::string result;
    for(char c : singleLine(text)){
        if(c == '\\' || c == '[' || c == ']' || c == '*' || c == '_' || c == '`' || c == '<' || c == '>') result += '\\';
        result += c;
    }
    return result;
}
inline std::string linkPath(const std::string &text){
    const char *hex = "0123456789ABCDEF";
    std::string result;
    for(unsigned char c : text){
        if(std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '/' || c == '~') result += static_cast<char>(c);
        else { result += '%'; result += hex[c >> 4]; result += hex[c & 15]; }
    }
    return result;
}
inline Catalog readmeCatalog(const std::string &text){
    Catalog entries;
    const auto begin = text.find("<!-- oceanode-shaders:start -->");
    const auto end = text.find("<!-- oceanode-shaders:end -->", begin);
    if(begin == std::string::npos || end == std::string::npos) return entries;
    const std::regex entryPattern(R"CATALOG(^- \[.*\]\(([^)]+)\)(?: — (.*))?$)CATALOG");
    for(const auto &line : effectShaderSource::split(text.substr(begin, end - begin), "\n")){
        std::smatch match;
        if(!std::regex_match(line, match, entryPattern)) continue;
        std::string path;
        const auto encoded = match[1].str();
        for(size_t i = 0; i < encoded.size(); ++i){
            if(encoded[i] == '%' && i + 2 < encoded.size()){
                const auto hex = encoded.substr(i + 1, 2);
                if(std::all_of(hex.begin(), hex.end(), [](unsigned char c){ return std::isxdigit(c); })){
                    path += static_cast<char>(std::stoi(hex, nullptr, 16)); i += 2; continue;
                }
            }
            path += encoded[i];
        }
        std::string description;
        const auto escaped = match[2].str();
        for(size_t i = 0; i < escaped.size(); ++i){
            if(escaped[i] == '\\' && i + 1 < escaped.size()) ++i;
            description += escaped[i];
        }
        entries[relativeFolder(path).generic_string()] = description;
    }
    return entries;
}
inline std::vector<std::string> documentBlocks(const std::string &text){
    std::vector<std::string> blocks;
    std::istringstream stream(text);
    std::string line, block, fence;
    while(std::getline(stream, line)){
        const auto trimmed = effectShaderSource::trim(line);
        if(trimmed.rfind("```", 0) == 0 || trimmed.rfind("~~~", 0) == 0){
            const auto marker = trimmed.substr(0, 3);
            if(fence.empty()) fence = marker;
            else if(fence == marker) fence.clear();
        }
        if(trimmed.empty() && fence.empty()){
            if(!block.empty()){ blocks.push_back(effectShaderSource::trim(block)); block.clear(); }
        }else block += line + '\n';
    }
    if(!block.empty()) blocks.push_back(effectShaderSource::trim(block));
    return blocks;
}
inline std::string mergeReadmes(const std::string &local, const std::string &remote, const Catalog &entries){
    // Keep handwritten documentation from both files. Only the managed catalog
    // is replaced, so repeating a save does not duplicate catalog entries.
    auto base = withoutCatalog(remote);
    const auto notes = withoutCatalog(local);
    if(base.empty()) base = notes;
    else if(!notes.empty() && base != notes && base.find(notes) == std::string::npos){
        // Append only paragraphs not already present in the repository README.
        std::string additions;
        for(const auto &block : documentBlocks(notes)){
            const auto paragraph = effectShaderSource::trim(block);
            if(!paragraph.empty() && base.find(paragraph) == std::string::npos) additions += paragraph + "\n\n";
        }
        if(!additions.empty()) base += "\n\n" + effectShaderSource::trim(additions);
    }
    if(base.empty()) base = "# Shader library";
    std::string result = base + "\n\n<!-- oceanode-shaders:start -->\n## Shader catalog\n\n";
    auto combined = readmeCatalog(remote);
    for(const auto &entry : readmeCatalog(local)) combined[entry.first] = entry.second;
    for(const auto &entry : entries) combined[entry.first] = entry.second;
    for(const auto &entry : combined){
        result += "- [" + markdownText(fs::path(entry.first).stem().string()) + "](" + linkPath(entry.first) + ")";
        if(!entry.second.empty()) result += " — " + markdownText(entry.second);
        result += '\n';
    }
    return result + "<!-- oceanode-shaders:end -->\n";
}
struct ReadmeSnapshot { std::string text; Catalog entries; };
using Readmes = std::map<std::string, ReadmeSnapshot>;
inline Readmes localReadmes(const fs::path &root, const fs::path &category){
    Readmes result;
    auto capture = [&](const fs::path &folder){
        const auto path = containedPath(root, folder);
        const auto text = readOptional(path / "README.md");
        auto entries = readmeCatalog(text);
        for(const auto &entry : catalog(path)) entries[entry.first] = entry.second;
        result[(folder / "README.md").generic_string()] = {text, entries};
    };
    capture({});
    if(!category.empty()) capture(category);
    return result;
}
inline Readmes updateLocalReadmes(const fs::path &root, const fs::path &category){
    auto result = localReadmes(root, category);
    for(auto &item : result){
        item.second.text = mergeReadmes(item.second.text, "", item.second.entries);
        writeFile(containedPath(root, item.first), item.second.text);
    }
    return result;
}
} // namespace shaderSaveTools
