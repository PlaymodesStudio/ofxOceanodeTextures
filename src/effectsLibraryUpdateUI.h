#pragma once

#include "effectsLibraryUpdater.h"
#include "ofxOceanode.h"
#include "simpleEffect.h"
#include <future>

class effectsLibraryUpdateUI {
public:
    explicit effectsLibraryUpdateUI(ofxOceanode &owner) : oceanode(owner) {}

    bool busy() const { return download.valid() || reviewing || applyRequested; }

    void request(){
        if(busy()) return;
        root = ofToDataPath("Effects", true);
        backupRoot = ofToDataPath("EffectsBackups", true);
        plan = {};
        warnings.clear();
        newEffects.clear();
        status = "Downloading Effects library...";
        openRequested = true;
        try{ download = std::async(std::launch::async, [](){ return effectsLibraryUpdater::fetch(); }); }
        catch(const std::exception &error){ status = error.what(); }
    }

    void update(){
        if(download.valid() && download.wait_for(std::chrono::seconds(0)) == std::future_status::ready){
            try{
                plan = effectsLibraryUpdater::prepare(download.get(), root);
                if(plan.conflicts.empty()) applyChanges();
                else{
                    reviewing = true;
                    status = std::to_string(plan.added.size()) + " new effects, " + std::to_string(plan.unchanged) +
                        " unchanged, " + std::to_string(plan.conflicts.size()) + " with differences.";
                    warnings = plan.warnings;
                }
            }catch(const std::exception &error){ status = error.what(); }
        }
        if(applyRequested){ applyRequested = false; applyChanges(); }
    }

    void draw(){
        ImGui::PushID(this);
        if(openRequested){ ImGui::OpenPopup("Update Effects"); openRequested = false; }
        ImGui::SetNextWindowSize(ImVec2(850, 0), ImGuiCond_Appearing);
        ImGui::SetNextWindowSizeConstraints(ImVec2(600, 0), ImVec2(FLT_MAX, ImGui::GetMainViewport()->WorkSize.y * 0.85f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 20));
        if(ImGui::BeginPopupModal("Update Effects", nullptr, ImGuiWindowFlags_AlwaysAutoResize)){
            ImGui::TextWrapped("%s", status.c_str());
            if(!newEffects.empty()){
                ImGui::Spacing();
                ImGui::SeparatorText("New effects");
                const float height = std::min(180.0f, float(newEffects.size()) * ImGui::GetTextLineHeightWithSpacing() + 16.0f);
                ImGui::BeginChild("Imported new effects", ImVec2(0, height));
                for(const auto &effect : newEffects) ImGui::BulletText("%s", effect.generic_string().c_str());
                ImGui::EndChild();
            }
            if(reviewing){
                ImGui::Spacing();
                ImGui::TextWrapped("Choose which version to keep for each changed effect. Expand its name to inspect the differences.");
                const float height = std::min(360.0f, std::max(120.0f, float(plan.conflicts.size()) * 48.0f));
                ImGui::BeginChild("Changed effects", ImVec2(0, height), true);
                if(ImGui::BeginTable("Choices", 2, ImGuiTableFlags_SizingStretchProp)){
                    ImGui::TableSetupColumn("Effect", ImGuiTableColumnFlags_WidthStretch, 0.65f);
                    ImGui::TableSetupColumn("Keep", ImGuiTableColumnFlags_WidthStretch, 0.35f);
                    ImGui::TableHeadersRow();
                    for(auto &file : plan.conflicts){
                        const auto name = file.relative.generic_string();
                        ImGui::PushID(name.c_str());
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        const bool expanded = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth);
                        if(ImGui::IsItemHovered()) ImGui::SetTooltip("%s", name.c_str());
                        if(expanded){
                            ImGui::TextDisabled("- Local   + Repository");
                            ImGui::BeginChild("Difference", ImVec2(0, 180), true, ImGuiWindowFlags_HorizontalScrollbar);
                            const auto diff = effectsLibraryUpdater::difference(file.localDisplay, file.incoming);
                            ImGui::TextUnformatted(diff.c_str());
                            ImGui::EndChild();
                            ImGui::TreePop();
                        }
                        ImGui::TableSetColumnIndex(1);
                        int choice = static_cast<int>(file.choice);
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        if(ImGui::Combo("##Keep", &choice, "Keep local\0Use repository\0Back up local + use repository\0"))
                            file.choice = static_cast<effectsLibraryUpdater::Choice>(choice);
                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }
                ImGui::EndChild();
                if(!plan.added.empty() && ImGui::TreeNode("New effects")){
                    ImGui::BeginChild("New files", ImVec2(0, 120), true);
                    for(const auto &file : plan.added) ImGui::TextUnformatted(file.relative.generic_string().c_str());
                    ImGui::EndChild();
                    ImGui::TreePop();
                }
            }
            if(!warnings.empty() && ImGui::TreeNode("Details")){
                for(const auto &warning : warnings) ImGui::TextWrapped("%s", warning.c_str());
                ImGui::TreePop();
            }
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            const bool controlsDisabled = download.valid() || applyRequested;
            if(controlsDisabled) ImGui::BeginDisabled();
            if(reviewing){
                if(ImGui::Button("Apply selected changes")) applyRequested = true;
                ImGui::SameLine();
            }
            if(ImGui::Button(reviewing ? "Cancel" : "Close")){
                reviewing = false;
                plan = {};
                ImGui::CloseCurrentPopup();
            }
            if(controlsDisabled) ImGui::EndDisabled();
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();
        ImGui::PopID();
    }

private:
    void applyChanges(){
        try{
            auto result = effectsLibraryUpdater::apply(plan, backupRoot);
            newEffects = std::move(result.addedFiles);
            for(const auto &relative : result.changed){
                const auto path = shaderSaveTools::containedPath(root, relative);
                const auto source = shaderSaveTools::readOptional(path);
                const auto parent = relative.parent_path().generic_string();
                const auto category = parent.empty() ? "Effects" : "Effects/" + parent;
                if(!oceanode.refreshModel<simpleEffect>(category, relative.stem().string(),
                        effectShaderSource::configuration(source), "Effects/" + relative.generic_string()))
                    result.warnings.push_back(relative.generic_string() + ": the node name is already used by a different category.");
            }
            status = result.message();
            if(!result.changed.empty()) status += "\nNew effects are available in New Node. Use Reload Shader on existing effect nodes to load replacements.";
            warnings = std::move(result.warnings);
            if(!warnings.empty()) status += "\nSome files need attention; see Details.";
        }catch(const std::exception &error){ status = error.what(); }
        reviewing = false;
        plan = {};
    }

    ofxOceanode &oceanode;
    std::future<effectsLibraryUpdater::Snapshot> download;
    effectsLibraryUpdater::Plan plan;
    shaderSaveTools::fs::path root, backupRoot;
    std::vector<shaderSaveTools::fs::path> newEffects;
    std::string status;
    std::vector<std::string> warnings;
    bool openRequested = false, reviewing = false, applyRequested = false;
};
