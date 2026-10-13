#pragma once

#include "shaderSaveTools.h"
#include <chrono>
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;
namespace shaderGitPublisher {
using namespace shaderSaveTools;
struct Settings {
    std::string repository = "https://github.com/PlaymodesStudio/ofxOceanodeTextures.git";
    std::string branch = "megabranch26_A";
    std::string effectsFolder = "SimpleEffectsLibrary/EffectsLibrary";
    std::string sshKey;
    std::string authorName = "infoplaymodes";
    std::string authorEmail = "infoplaymodes@gmail.com";
};
inline std::string quote(const std::string &value){
    std::string result = "'";
    for(char c : value) result += c == '\'' ? "'\\''" : std::string(1, c);
    return result + '\'';
}
struct CommandResult { int status = -1; std::string output; };
inline CommandResult git(const std::vector<std::string> &arguments, const Settings &settings){
    std::vector<std::string> command = {"git", "-c", "core.hooksPath=/dev/null", "-c", "commit.gpgsign=false", "-c", "credential.interactive=false",
        "-c", "http.lowSpeedLimit=1", "-c", "http.lowSpeedTime=30"};
    command.insert(command.end(), arguments.begin(), arguments.end());
    std::vector<char*> argv;
    for(auto &item : command) argv.push_back(item.data());
    argv.push_back(nullptr);
    std::map<std::string, std::string> env;
    for(char **item = environ; *item; ++item){
        const std::string value(*item);
        const auto equals = value.find('=');
        if(equals != std::string::npos) env[value.substr(0, equals)] = value.substr(equals + 1);
    }
    // Do not inherit a terminal caller's GIT_DIR/index/worktree overrides.
    for(auto it = env.begin(); it != env.end();){
        if(it->first.rfind("GIT_", 0) == 0) it = env.erase(it); else ++it;
    }
    env["GIT_TERMINAL_PROMPT"] = "0";
    env["GIT_SSH_COMMAND"] = "ssh -o BatchMode=yes -o ConnectTimeout=10";
    if(!settings.sshKey.empty()) env["GIT_SSH_COMMAND"] += " -o IdentitiesOnly=yes -i " + quote(settings.sshKey);
    if(!settings.authorName.empty()) env["GIT_AUTHOR_NAME"] = env["GIT_COMMITTER_NAME"] = settings.authorName;
    if(!settings.authorEmail.empty()) env["GIT_AUTHOR_EMAIL"] = env["GIT_COMMITTER_EMAIL"] = settings.authorEmail;
    std::vector<std::string> environment;
    for(const auto &item : env) environment.push_back(item.first + "=" + item.second);
    std::vector<char*> envp;
    for(auto &item : environment) envp.push_back(item.data());
    envp.push_back(nullptr);

    int pipes[2];
    if(pipe(pipes)) throw std::runtime_error("Cannot create Git output pipe.");
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
    posix_spawn_file_actions_adddup2(&actions, pipes[1], STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&actions, pipes[1], STDERR_FILENO);
    posix_spawn_file_actions_addclose(&actions, pipes[0]);
    posix_spawn_file_actions_addclose(&actions, pipes[1]);
    posix_spawnattr_t attributes;
    posix_spawnattr_init(&attributes);
    posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETPGROUP);
    posix_spawnattr_setpgroup(&attributes, 0);
    pid_t child = 0;
    const int launched = posix_spawnp(&child, "git", &actions, &attributes, argv.data(), envp.data());
    posix_spawn_file_actions_destroy(&actions);
    posix_spawnattr_destroy(&attributes);
    close(pipes[1]);
    if(launched){ close(pipes[0]); throw std::runtime_error("Cannot launch Git. Install/configure Git on this computer."); }
    fcntl(pipes[0], F_SETFL, O_NONBLOCK);
    CommandResult result;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(120);
    bool finished = false, eof = false;
    int status = 0;
    while(!finished || !eof){
        pollfd ready = {pipes[0], POLLIN, 0};
        poll(&ready, 1, 50);
        char data[4096];
        ssize_t readBytes;
        while((readBytes = read(pipes[0], data, sizeof(data))) > 0){
            if(result.output.size() < 65536) result.output.append(data, std::min<size_t>(readBytes, 65536 - result.output.size()));
        }
        eof = readBytes == 0;
        if(!finished) finished = waitpid(child, &status, WNOHANG) == child;
        if(std::chrono::steady_clock::now() >= deadline){
            kill(-child, SIGKILL);
            if(!finished) waitpid(child, &status, 0);
            close(pipes[0]);
            throw std::runtime_error("Git operation timed out after 120 seconds.");
        }
    }
    close(pipes[0]);
    result.status = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return result;
}
inline void check(const CommandResult &result, const std::string &step){
    if(result.status != 0) throw std::runtime_error(step + " failed:\n" + result.output);
}
inline void validate(const Settings &settings){
    if(settings.repository.empty() || settings.repository.front() == '-' || settings.repository.find_first_of("\r\n\0", 0, 3) != std::string::npos)
        throw std::runtime_error("Configure a valid Git repository URL.");
    if(settings.repository.rfind("https://", 0) == 0){
        const auto end = settings.repository.find('/', 8);
        const auto host = settings.repository.substr(8, end == std::string::npos ? end : end - 8);
        if(host.find('@') != std::string::npos) throw std::runtime_error("Keep credentials in Git's credential helper, not in the repository URL.");
    }
    if(settings.branch.empty() || settings.branch.front() == '-') throw std::runtime_error("Configure an existing target branch.");
    relativeFolder(settings.effectsFolder);
    if(!settings.sshKey.empty() && !fs::is_regular_file(settings.sshKey)) throw std::runtime_error("The configured Git SSH key file does not exist.");
    check(git({"check-ref-format", "--branch", settings.branch}, settings), "Branch validation");
}
struct Result {
    bool success = false;
    std::string message;
    std::string recoveryCheckout;
    std::map<std::string, std::string> readmes;
};
inline Result publish(Settings settings, const fs::path &shaderRelativePath, const std::string &shaderSource,
                      const Readmes &local){
    Result result;
    fs::path work;
    try{
        validate(settings);
        // A dedicated SSH identity also selects SSH transport for a GitHub HTTPS URL.
        if(!settings.sshKey.empty() && settings.repository.rfind("https://github.com/", 0) == 0)
            settings.repository = "git@github.com:" + settings.repository.substr(19);
        auto pattern = (fs::temp_directory_path() / "oceanode-shader-publish-XXXXXX").string();
        std::vector<char> name(pattern.begin(), pattern.end()); name.push_back('\0');
        if(!mkdtemp(name.data())) throw std::runtime_error("Cannot create the temporary Git checkout.");
        work = fs::path(name.data()) / "repo";
        check(git({"clone", "--depth", "1", "--single-branch", "--branch", settings.branch, "--template=", "--",
                   settings.repository, work.string()}, settings), "Clone");
        const auto root = containedPath(work, relativeFolder(settings.effectsFolder));
        fs::create_directories(root);
        const auto relative = relativeFolder(shaderRelativePath.generic_string());
        writeFile(containedPath(root, relative), shaderSource);
        std::vector<std::string> tracked = {fs::relative(containedPath(root, relative), work).generic_string()};
        for(const auto &item : local){
            const auto readme = containedPath(root, item.first);
            const auto remoteText = readOptional(readme);
            auto entries = readmeCatalog(remoteText);
            // Repository shader files (including the just-written save) are
            // authoritative; add local-only entries without reverting remote edits.
            for(const auto &entry : catalog(readme.parent_path())) entries[entry.first] = entry.second;
            for(const auto &entry : item.second.entries)
                if(entries.find(entry.first) == entries.end()) entries[entry.first] = entry.second;
            const auto merged = mergeReadmes(item.second.text, remoteText, entries);
            writeFile(readme, merged);
            result.readmes[item.first] = merged;
            tracked.push_back(fs::relative(readme, work).generic_string());
        }
        std::vector<std::string> stage = {"-C", work.string(), "add", "--"};
        stage.insert(stage.end(), tracked.begin(), tracked.end());
        check(git(stage, settings), "Stage shader and READMEs");
        const auto diff = git({"-C", work.string(), "diff", "--cached", "--quiet"}, settings);
        if(diff.status == 0){ result.success = true; result.message = "Repository already contains this shader and README catalog."; }
        else{
            if(diff.status != 1) check(diff, "Inspect staged changes");
            check(git({"-C", work.string(), "commit", "-m", "Update shader " + relative.generic_string() + " and library catalog"}, settings), "Commit");
            check(git({"-C", work.string(), "push", "origin", "HEAD:refs/heads/" + settings.branch}, settings), "Push");
            result.success = true;
            result.message = "Pushed " + (relativeFolder(settings.effectsFolder) / relative).generic_string() +
                " and README catalogs to " + settings.branch + ".";
        }
        fs::remove_all(work.parent_path());
    }catch(const std::exception &error){
        result.readmes.clear(); // Never sync an unpublished merge back to local files.
        result.message = error.what();
        if(!work.empty() && fs::exists(work / ".git")){
            result.recoveryCheckout = work.string();
            result.message += "\nRecovery checkout: " + result.recoveryCheckout;
        }else if(!work.empty()){
            std::error_code ignored; fs::remove_all(work.parent_path(), ignored);
        }
    }
    return result;
}
} // namespace shaderGitPublisher
