#pragma once

#include "shaderGitPublisher.h"

// Read-only repository access and file review, independent of OF/ImGui.
namespace effectsLibraryUpdater {
using namespace shaderSaveTools;
enum class Choice { KeepLocal, UseRepository, BackupAndUseRepository };
struct Snapshot {
    bool success = false;
    std::string error;
    std::map<fs::path, std::string> shaders, readmes;
    std::vector<std::string> warnings;
};

inline std::string standalone(const fs::path &root, const fs::path &relative,
                              std::set<fs::path> &included, int depth = 0){
    if(depth > 32) throw std::runtime_error("Shader include depth exceeds 32.");
    const auto path = containedPath(root, relative);
    if(!included.insert(path).second) return "";
    if(!fs::is_regular_file(path)) throw std::runtime_error("Missing shader/include: " + relative.generic_string());
    if(fs::file_size(path) > 2 * 1024 * 1024) throw std::runtime_error("Shader/include exceeds 2 MiB.");
    const auto source = readOptional(path);
    std::istringstream original(source), uncommented(effectShaderSource::withoutComments(source));
    const std::regex includePattern(R"INCLUDE(^\s*#\s*(?:pragma\s+)?include\s*["<]([^">]+)[">]\s*$)INCLUDE");
    std::string result, line, codeLine;
    while(std::getline(original, line)){
        std::getline(uncommented, codeLine);
        if(!line.empty() && line.back() == '\r') line.pop_back();
        std::smatch match;
        if(std::regex_match(codeLine, match, includePattern)){
            const fs::path include(match[1].str());
            if(include.is_absolute()) throw std::runtime_error("Shader includes must stay inside the Effects library.");
            // Resolve ../Common.inc normally, then check every path component.
            const auto next = (relative.parent_path() / include).lexically_normal();
            result += standalone(root, next, included, depth + 1) + "\n";
        }else result += line + "\n";
        if(result.size() > 8 * 1024 * 1024) throw std::runtime_error("Expanded shader exceeds 8 MiB.");
    }
    return result;
}
inline std::string standalone(const fs::path &root, const fs::path &relative){
    std::set<fs::path> included;
    return standalone(root, relative, included);
}

inline Snapshot fetch(shaderGitPublisher::Settings settings = {}){
    Snapshot result;
    fs::path temporary;
    try{
        // Reading the public library needs neither an SSH key nor write access.
        settings.sshKey.clear();
        shaderGitPublisher::validate(settings);
        auto pattern = (fs::temp_directory_path() / "oceanode-effects-update-XXXXXX").string();
        std::vector<char> name(pattern.begin(), pattern.end()); name.push_back('\0');
        if(!mkdtemp(name.data())) throw std::runtime_error("Cannot create the temporary library download.");
        temporary = name.data();
        const auto work = temporary / "repo";
        using shaderGitPublisher::check;
        using shaderGitPublisher::git;
        check(git({"clone", "--depth", "1", "--single-branch", "--branch", settings.branch,
                   "--filter=blob:none", "--no-checkout", "--template=", "--", settings.repository, work.string()}, settings), "Download Effects library");
        check(git({"-C", work.string(), "sparse-checkout", "set", "--cone", "--", settings.effectsFolder}, settings), "Select Effects library");
        check(git({"-C", work.string(), "checkout"}, settings), "Read Effects library");
        const auto root = containedPath(work, relativeFolder(settings.effectsFolder));
        if(!fs::is_directory(root)) throw std::runtime_error("The repository has no Effects library at " + settings.effectsFolder + ".");
        for(const auto &entry : fs::recursive_directory_iterator(root)){
            const auto relative = entry.path().lexically_relative(root);
            try{
                if(entry.is_symlink()) throw std::runtime_error("Library symlinks are not imported.");
                if(!entry.is_regular_file()) continue;
                if(effectShaderSource::lower(entry.path().extension().string()) == ".glsl"){
                    auto source = standalone(root, relative);
                    const auto errors = effectShaderSource::validate(source);
                    if(!errors.empty()) throw std::runtime_error(errors.front());
                    result.shaders[relative] = std::move(source);
                }else if(entry.path().filename() == "README.md"){
                    if(entry.file_size() > 2 * 1024 * 1024) throw std::runtime_error("README exceeds 2 MiB.");
                    result.readmes[relative] = readOptional(entry.path());
                }
            }catch(const std::exception &error){ result.warnings.push_back(relative.generic_string() + ": " + error.what()); }
        }
        result.success = true;
    }catch(const std::exception &error){ result.error = error.what(); }
    if(!temporary.empty()){
        std::error_code ignored;
        fs::remove_all(temporary, ignored);
    }
    return result;
}

struct File {
    fs::path relative;
    std::string incoming, baseline, localDisplay;
    std::map<fs::path, std::string> dependencies;
    bool exists = false;
    Choice choice = Choice::KeepLocal;
};
struct Plan {
    fs::path root;
    std::vector<File> added, conflicts;
    size_t unchanged = 0;
    std::map<fs::path, std::string> readmes;
    std::vector<std::string> warnings;
};
inline Plan prepare(Snapshot snapshot, const fs::path &root){
    if(!snapshot.success) throw std::runtime_error(snapshot.error);
    Plan plan;
    plan.root = root;
    plan.readmes = std::move(snapshot.readmes);
    plan.warnings = std::move(snapshot.warnings);
    for(auto &item : snapshot.shaders){
        try{
            File file;
            file.relative = item.first;
            file.incoming = std::move(item.second);
            const auto destination = containedPath(root, file.relative);
            file.exists = fs::exists(destination);
            if(!file.exists){ plan.added.push_back(std::move(file)); continue; }
            if(!fs::is_regular_file(destination)) throw std::runtime_error("Local destination is not a regular file.");
            file.baseline = readOptional(destination);
            std::set<fs::path> included;
            try{ file.localDisplay = standalone(root, file.relative, included); }
            catch(const std::exception &){ file.localDisplay = file.baseline; }
            if(file.localDisplay == file.incoming) ++plan.unchanged;
            else{
                for(const auto &dependency : included){
                    if(dependency == destination || !fs::is_regular_file(dependency)) continue;
                    file.dependencies[fs::relative(dependency, root)] = readOptional(dependency);
                }
                plan.conflicts.push_back(std::move(file));
            }
        }catch(const std::exception &error){ plan.warnings.push_back(item.first.generic_string() + ": " + error.what()); }
    }
    return plan;
}

inline void writeAtomic(const fs::path &destination, const std::string &content){
    fs::create_directories(destination.parent_path());
    auto pattern = (destination.parent_path() / ".oceanode-effect-XXXXXX").string();
    std::vector<char> name(pattern.begin(), pattern.end()); name.push_back('\0');
    const int descriptor = mkstemp(name.data());
    if(descriptor < 0) throw std::runtime_error("Cannot prepare " + destination.string());
    close(descriptor);
    const fs::path temporary(name.data());
    try{
        writeFile(temporary, content);
        fs::rename(temporary, destination);
    }catch(...){ std::error_code ignored; fs::remove(temporary, ignored); throw; }
}

struct Result {
    size_t added = 0, replaced = 0, kept = 0, unchanged = 0;
    std::vector<fs::path> changed, addedFiles;
    fs::path backup;
    std::vector<std::string> warnings;
    std::string message() const {
        std::string text = std::to_string(added) + " new, " + std::to_string(replaced) + " replaced, " +
            std::to_string(unchanged) + " unchanged, " + std::to_string(kept) + " kept locally.";
        if(!backup.empty()) text += "\nBackup: " + backup.string();
        return text;
    }
};
inline Result apply(const Plan &plan, const fs::path &backupRoot){
    Result result;
    result.unchanged = plan.unchanged;
    result.warnings = plan.warnings;
    const auto import = [&](const File &file){
        if(file.exists && file.choice == Choice::KeepLocal){ ++result.kept; return; }
        try{
            const auto destination = containedPath(plan.root, file.relative);
            if(fs::exists(destination) != file.exists || (file.exists && readOptional(destination) != file.baseline))
                throw std::runtime_error("Local file changed during review; run Update Effects again.");
            if(file.exists){
                std::string current = file.baseline;
                try{ current = standalone(plan.root, file.relative); }catch(const std::exception &){}
                if(current != file.localDisplay)
                    throw std::runtime_error("Local shader/include changed during review; run Update Effects again.");
            }
            if(file.exists && file.choice == Choice::BackupAndUseRepository){
                if(result.backup.empty()){
                    fs::create_directories(backupRoot);
                    auto pattern = (backupRoot / "Update-XXXXXX").string();
                    std::vector<char> name(pattern.begin(), pattern.end()); name.push_back('\0');
                    if(!mkdtemp(name.data())) throw std::runtime_error("Cannot create the Effects backup folder.");
                    result.backup = name.data();
                }
                for(const auto &dependency : file.dependencies)
                    writeAtomic(containedPath(result.backup, dependency.first), dependency.second);
                writeAtomic(containedPath(result.backup, file.relative), file.baseline);
            }
            writeAtomic(destination, file.incoming);
            result.changed.push_back(file.relative);
            if(file.exists) ++result.replaced;
            else{
                ++result.added;
                result.addedFiles.push_back(file.relative);
            }
        }catch(const std::exception &error){ result.warnings.push_back(file.relative.generic_string() + ": " + error.what()); }
    };
    for(const auto &file : plan.added) import(file);
    for(const auto &file : plan.conflicts) import(file);
    // Catalogs describe the files actually retained, including local-only ones.
    std::set<fs::path> readmes;
    if(!result.changed.empty()) readmes.insert("README.md");
    for(const auto &file : result.changed) readmes.insert(file.parent_path() / "README.md");
    for(const auto &item : plan.readmes) readmes.insert(item.first);
    for(const auto &relative : readmes){
        try{
            const auto destination = containedPath(plan.root, relative);
            const auto local = readOptional(destination);
            const auto remote = plan.readmes.find(relative);
            const auto merged = mergeReadmes(local, remote == plan.readmes.end() ? "" : remote->second, catalog(destination.parent_path()));
            if(merged != local) writeAtomic(destination, merged);
        }catch(const std::exception &error){ result.warnings.push_back(relative.generic_string() + ": " + error.what()); }
    }
    return result;
}

// Show the changed range, omitting the identical beginning and end.
inline std::string difference(const std::string &local, const std::string &remote){
    const auto before = effectShaderSource::split(local, "\n");
    const auto after = effectShaderSource::split(remote, "\n");
    size_t start = 0, endBefore = before.size(), endAfter = after.size();
    while(start < endBefore && start < endAfter && before[start] == after[start]) ++start;
    while(endBefore > start && endAfter > start && before[endBefore - 1] == after[endAfter - 1]){ --endBefore; --endAfter; }
    std::string text = "@@ Local " + std::to_string(start + 1) + "," + std::to_string(endBefore - start) +
        " | Repository " + std::to_string(start + 1) + "," + std::to_string(endAfter - start) + " @@\n";
    for(size_t i = start; i < endBefore; ++i) text += "- " + before[i] + "\n";
    for(size_t i = start; i < endAfter; ++i) text += "+ " + after[i] + "\n";
    return text;
}
} // namespace effectsLibraryUpdater
