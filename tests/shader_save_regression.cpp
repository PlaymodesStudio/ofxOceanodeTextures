#include "shaderGitPublisher.h"
#include <cassert>
#include <iostream>
using namespace shaderSaveTools;
using shaderGitPublisher::git;
using shaderGitPublisher::check;

int main(){
    auto pattern = (fs::temp_directory_path() / "shader-save-test-XXXXXX").string();
    std::vector<char> name(pattern.begin(), pattern.end()); name.push_back('\0');
    assert(mkdtemp(name.data()));
    const fs::path temp(name.data());
    struct Cleanup { fs::path path; ~Cleanup(){ fs::remove_all(path); } } cleanup{temp};
    const auto shader = std::string("//Amount:1:0:2\n// @description Old description.\n#version 410\nuniform float Amount;\nout vec4 out_color;\nvoid main(){out_color=vec4(Amount);}\n");
    const auto annotated = withDescription(shader, "Controls\n\nbrightness with Amount.");
    assert(annotated.rfind("//Amount:1:0:2\n// @description Controls brightness with Amount.\n", 0) == 0);
    assert(annotated.find("Old description") == std::string::npos);
    assert(effectShaderSource::validate(annotated).empty());
    assert(description(annotated) == "Controls brightness with Amount.");
    assert(effectShaderSource::validate(withDescription(shader, "Never end a comment with \\")).empty());
    for(const auto &bad : {"../outside", "/tmp/outside", ".git", "folder/../outside", "a\\b"}){
        bool rejected = false;
        try{ relativeFolder(bad); }catch(const std::exception &){ rejected = true; }
        assert(rejected);
    }
    const auto fenced = std::string("# Usage\n\n```glsl\nfloat a = 1;\n\nfloat b = 2;\n```\n");
    const auto documented = mergeReadmes(fenced, "# Remote docs", {});
    assert(documented.find("```glsl\nfloat a = 1;\n\nfloat b = 2;\n```") != std::string::npos);
    assert(mergeReadmes(documented, documented, {}) == documented);
    const auto local = temp / "Effects";
    const auto category = fs::path("Blur & detail") / "New category";
    fs::create_directories(local);
    fs::create_directory_symlink(temp, local / "Escape");
    bool escaped = false;
    try{ containedPath(local, "Escape/outside.glsl"); }catch(const std::exception &){ escaped = true; }
    assert(escaped);
    writeFile(containedPath(local, category / "MyShader.glsl"), annotated);
    writeFile(local / "README.md", "# Local library\n\nLocal usage notes.\n");
    writeFile(local / category / "README.md", "# Local category\n\nLocal category notes.\n");
    assert(categories(local).size() == 3); // root, category and nested category; no symlink.
    auto readmes = updateLocalReadmes(local, category);
    assert(readmes.size() == 2);
    const auto localText = readOptional(local / "README.md");
    assert(localText.find("MyShader.glsl") != std::string::npos);
    assert(localText.find("Blur%20%26%20detail/New%20category/MyShader.glsl") != std::string::npos);
    const auto again = updateLocalReadmes(local, category);
    assert(again.at("README.md").text == localText);

    shaderGitPublisher::Settings settings;
    settings.repository = (temp / "remote.git").string();
    assert(settings.effectsFolder == "SimpleEffectsLibrary/EffectsLibrary");
    const auto seed = temp / "seed";
    check(git({"init", "--bare", "--initial-branch", settings.branch, settings.repository}, settings), "Init remote");
    check(git({"init", "--initial-branch", settings.branch, seed.string()}, settings), "Init seed");
    writeFile(seed / "unrelated.txt", "must remain untouched\n");
    writeFile(seed / settings.effectsFolder / "RemoteOnly.glsl", withDescription(shader, "An existing repository shader."));
    writeFile(seed / settings.effectsFolder / "README.md", "# Remote library\n\nRemote usage notes.\n");
    check(git({"-C", seed.string(), "add", "."}, settings), "Seed stage");
    check(git({"-C", seed.string(), "commit", "-m", "seed"}, settings), "Seed commit");
    check(git({"-C", seed.string(), "remote", "add", "origin", settings.repository}, settings), "Seed remote");
    check(git({"-C", seed.string(), "push", "origin", settings.branch}, settings), "Seed push");
    const auto relative = category / "MyShader.glsl";
    const auto published = shaderGitPublisher::publish(settings, relative, annotated, readmes);
    if(!published.success){ std::cerr << published.message << '\n'; return 1; }
    assert(published.readmes.size() == 2);
    const auto &merged = published.readmes.at("README.md");
    assert(merged.find("Local usage notes") != std::string::npos);
    assert(merged.find("Remote usage notes") != std::string::npos);
    assert(merged.find("Controls brightness with Amount") != std::string::npos);
    assert(merged.find("RemoteOnly.glsl") != std::string::npos);
    for(const auto &item : published.readmes) writeFile(local / item.first, item.second);
    assert(mergeReadmes(merged, merged, readmes.at("README.md").entries) == merged);
    const auto inspect = temp / "inspect";
    check(git({"clone", "--", settings.repository, inspect.string()}, settings), "Inspect clone");
    assert(readOptional(inspect / settings.effectsFolder / relative) == annotated);
    assert(readOptional(inspect / settings.effectsFolder / "README.md") == readOptional(local / "README.md"));
    assert(readOptional(inspect / "unrelated.txt") == "must remain untouched\n");
    auto count = git({"-C", inspect.string(), "rev-list", "--count", "HEAD"}, settings);
    assert(count.status == 0 && effectShaderSource::trim(count.output) == "2");
    auto files = git({"-C", inspect.string(), "diff-tree", "--no-commit-id", "--name-only", "-r", "HEAD"}, settings);
    assert(files.status == 0);
    assert(files.output.find("unrelated.txt") == std::string::npos);
    assert(effectShaderSource::split(effectShaderSource::trim(files.output), "\n").size() == 3);
    auto author = git({"-C", inspect.string(), "log", "-1", "--format=%ae"}, settings);
    assert(effectShaderSource::trim(author.output) == settings.authorEmail);
    const auto unchanged = shaderGitPublisher::publish(settings, relative, annotated, localReadmes(local, category));
    assert(unchanged.success && unchanged.message.find("already contains") != std::string::npos);
    writeFile(inspect / settings.effectsFolder / "RemoteOnly.glsl", withDescription(shader, "A newer repository description."));
    check(git({"-C", inspect.string(), "add", "."}, settings), "Remote update stage");
    check(git({"-C", inspect.string(), "commit", "-m", "update remote description"}, settings), "Remote update commit");
    check(git({"-C", inspect.string(), "push", "origin", settings.branch}, settings), "Remote update push");
    const auto refreshed = shaderGitPublisher::publish(settings, relative, annotated, localReadmes(local, category));
    assert(refreshed.success && refreshed.readmes.at("README.md").find("A newer repository description") != std::string::npos);
    const auto originalRemote = settings.repository;
    settings.repository = seed.string(); // Non-bare checked-out branch rejects a real push.
    const auto rejectedPush = shaderGitPublisher::publish(settings, relative, annotated, readmes);
    assert(!rejectedPush.success && rejectedPush.message.find("Push failed") != std::string::npos);
    assert(rejectedPush.readmes.empty() && !rejectedPush.recoveryCheckout.empty());
    assert(readOptional(fs::path(rejectedPush.recoveryCheckout) / settings.effectsFolder / relative) == annotated);
    fs::remove_all(fs::path(rejectedPush.recoveryCheckout).parent_path());
    settings.repository = originalRemote;
    settings.branch = "missing-branch";
    const auto failed = shaderGitPublisher::publish(settings, relative, annotated, readmes);
    assert(!failed.success && failed.readmes.empty());
    assert(readOptional(local / relative) == annotated);
    std::cout << "PASS shader descriptions, folder containment, README merging, isolated commits, nested folders, repeat saves and publish failures\n";
}
