#include "effectsLibraryUpdater.h"
#include <cassert>
#include <iostream>
using namespace effectsLibraryUpdater;
using shaderGitPublisher::git;
using shaderGitPublisher::check;

int main(int argc, char **argv){
    assert(argc == 2);
    auto pattern = (fs::temp_directory_path() / "effects-update-test-XXXXXX").string();
    std::vector<char> name(pattern.begin(), pattern.end()); name.push_back('\0');
    assert(mkdtemp(name.data()));
    const fs::path temp(name.data());
    struct Cleanup { fs::path path; ~Cleanup(){ fs::remove_all(path); } } cleanup{temp};
    shaderGitPublisher::Settings settings;
    settings.repository = (temp / "remote.git").string();
    const auto seed = temp / "seed", library = seed / settings.effectsFolder;
    const auto local = temp / "data/Effects", backups = temp / "data/EffectsBackups";
    const std::string shader = "//Amount:1:0:2\n// @description Original shader.\n#version 410\nuniform float Amount;\nout vec4 out_color;\nvoid main(){out_color=vec4(Amount);}\n";
    const std::string included = "//\n// @description Shared include.\n#version 410\n#pragma include \"../Common.inc\"\nout vec4 out_color;\nvoid main(){out_color=sharedColor();}\n";
    const auto samePath = fs::path("Generate/Same.glsl"), newPath = fs::path("Generate/New.glsl");
    check(git({"init", "--bare", "--initial-branch", settings.branch, settings.repository}, settings), "Init remote");
    check(git({"init", "--initial-branch", settings.branch, seed.string()}, settings), "Init seed");
    const auto publish = [&](){
        check(git({"-C", seed.string(), "add", "--", "."}, settings), "Stage fixture");
        check(git({"-C", seed.string(), "commit", "-m", "test update"}, settings), "Commit fixture");
        check(git({"-C", seed.string(), "push", "origin", settings.branch}, settings), "Push fixture");
    };
    check(git({"-C", seed.string(), "remote", "add", "origin", settings.repository}, settings), "Seed remote");
    writeFile(library / samePath, included);
    writeFile(library / "Common.inc", "vec4 sharedColor(){return vec4(1);}\n");
    writeFile(library / newPath, shader);
    writeFile(library / "README.md", "# Remote effects\n\nRemote usage notes.\n");
    writeFile(seed / "src/not-an-effect.glsl", shader);
    publish();
    writeFile(local / samePath, included);
    writeFile(local / "Common.inc", readOptional(library / "Common.inc"));
    writeFile(local / "LocalOnly.glsl", shader);
    writeFile(local / "README.md", "# Local effects\n\nLocal usage notes.\n");

    auto snapshot = fetch(settings);
    assert(snapshot.success && snapshot.shaders.size() == 2 && snapshot.warnings.empty());
    auto plan = prepare(snapshot, local);
    assert(plan.added.size() == 1 && plan.conflicts.empty() && plan.unchanged == 1);
    auto result = apply(plan, backups);
    assert(result.added == 1 && result.replaced == 0 && result.warnings.empty());
    assert(result.addedFiles == std::vector<fs::path>{newPath});
    assert(readOptional(local / newPath) == shader);
    assert(!fs::exists(local / "src/not-an-effect.glsl"));
    assert(readOptional(local / "README.md").find("Local usage notes") != std::string::npos);
    assert(readOptional(local / "README.md").find("Remote usage notes") != std::string::npos);
    const auto readme = readOptional(local / "README.md");

    // A newer commit outside the library must not produce changed effects.
    writeFile(seed / "src/unrelated.cpp", "unrelated addon code\n");
    publish();
    plan = prepare(fetch(settings), local);
    assert(plan.added.empty() && plan.conflicts.empty() && plan.unchanged == 2);
    result = apply(plan, backups);
    assert(result.changed.empty() && readOptional(local / "README.md") == readme);
    assert(result.addedFiles.empty());

    // A shared include change is a real difference, even with identical .glsl.
    writeFile(library / "Common.inc", "vec4 sharedColor(){return vec4(0.5);}\n");
    publish();
    plan = prepare(fetch(settings), local);
    assert(plan.conflicts.size() == 1 && plan.conflicts.front().relative == samePath);
    const auto diff = difference(plan.conflicts.front().localDisplay, plan.conflicts.front().incoming);
    assert(diff.find("- vec4 sharedColor(){return vec4(1);}") != std::string::npos);
    assert(diff.find("+ vec4 sharedColor(){return vec4(0.5);}") != std::string::npos);
    result = apply(plan, backups); // default decision preserves the local version
    assert(result.kept == 1 && result.changed.empty() && readOptional(local / samePath) == included);

    writeFile(library / newPath, withDescription(shader, "New repository description."));
    publish();
    plan = prepare(fetch(settings), local);
    assert(plan.conflicts.size() == 2);
    for(auto &file : plan.conflicts)
        file.choice = file.relative == samePath ? Choice::BackupAndUseRepository : Choice::UseRepository;
    result = apply(plan, backups);
    assert(result.replaced == 2 && result.warnings.empty() && !result.backup.empty());
    assert(result.addedFiles.empty());
    assert(readOptional(result.backup / samePath) == included);
    assert(standalone(result.backup, samePath).find("vec4(1)") != std::string::npos);
    assert(!fs::exists(result.backup / newPath));
    assert(readOptional(local / samePath).find("#pragma include") == std::string::npos);
    assert(readOptional(local / samePath).find("vec4(0.5)") != std::string::npos);
    assert(readOptional(local / "Common.inc").find("vec4(1)") != std::string::npos);
    assert(prepare(fetch(settings), local).conflicts.empty());

    // A decision cannot overwrite an edit made after the review was prepared.
    auto race = snapshot;
    race.shaders[newPath] = withDescription(shader, "Another repository version.");
    plan = prepare(race, local);
    for(auto &file : plan.conflicts) file.choice = Choice::UseRepository;
    const auto newerLocal = withDescription(shader, "Edited locally during review.");
    writeFile(local / newPath, newerLocal);
    result = apply(plan, backups);
    assert(readOptional(local / newPath) == newerLocal && !result.warnings.empty());
    race.shaders["NewCategory/Race.glsl"] = shader;
    plan = prepare(race, local);
    writeFile(local / "NewCategory/Race.glsl", newerLocal);
    result = apply(plan, backups);
    assert(readOptional(local / "NewCategory/Race.glsl") == newerLocal && !result.warnings.empty());
    assert(result.addedFiles.empty());

    writeFile(local / samePath, included);
    plan = prepare(fetch(settings), local);
    for(auto &file : plan.conflicts) file.choice = Choice::BackupAndUseRepository;
    writeFile(local / "Common.inc", "vec4 sharedColor(){return vec4(0.25);}\n");
    result = apply(plan, backups);
    assert(readOptional(local / samePath) == included && !result.warnings.empty());

    // Import neither invalid source nor includes escaping the library.
    writeFile(library / "Invalid.glsl", "not a shader\n");
    writeFile(library / "Escape.glsl", "//\n#version 410\n#pragma include \"../../src/not-an-effect.glsl\"\n");
    fs::create_symlink(seed / "src/not-an-effect.glsl", library / "Linked.glsl");
    publish();
    snapshot = fetch(settings);
    assert(snapshot.success && snapshot.shaders.size() == 2 && snapshot.warnings.size() == 3);
    assert(!fs::exists(local / "Invalid.glsl") && !fs::exists(local / "Escape.glsl"));
    fs::create_directory_symlink(temp, local / "Outside");
    snapshot.shaders["Outside/Escape.glsl"] = shader;
    plan = prepare(snapshot, local);
    assert(!plan.warnings.empty());
    result = apply(plan, backups);
    assert(!fs::exists(temp / "Escape.glsl"));

    // Repository deletion never deletes local-only or previously downloaded files.
    fs::remove(library / newPath);
    publish();
    plan = prepare(fetch(settings), local);
    apply(plan, backups);
    assert(fs::exists(local / newPath) && fs::exists(local / "LocalOnly.glsl"));
    settings.branch = "missing-branch";
    assert(!fetch(settings).success);

    size_t count = 0;
    const auto shipped = fs::path(argv[1]) / "SimpleEffectsLibrary/EffectsLibrary";
    for(const auto &entry : fs::recursive_directory_iterator(shipped)){
        if(entry.path().extension() != ".glsl") continue;
        assert(effectShaderSource::validate(standalone(shipped, fs::relative(entry.path(), shipped))).empty());
        ++count;
    }
    std::cout << "PASS Effects updates: sparse downloads, unrelated commits, conflict choices, backups, review races, safe includes, retained local files and " << count << " library shaders\n";
}
