#pragma once

#include "ofxOceanodeNodeModel.h"
#include "ofxOceanodeShared.h"
#include "ofxOceanodeInspectorController.h"
#include "portal.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <set>

class textureDisplay : public ofxOceanodeNodeModel {
public:
    textureDisplay() : ofxOceanodeNodeModel("Texture Display") {
        selectedPortalInstance = nullptr;
    }

    void setup() override {
        description = "Displays a texture from a portal with a resizable area.";

        setFlags(ofxOceanodeNodeModelFlags_TransparentNode);

        addInspectorParameter(displayWidth.set("Width",  320.f, 32.f, 4096.f));
        addInspectorParameter(displayHeight.set("Height", 180.f, 32.f, 4096.f));
        addInspectorParameter(globalSearch.set("Global Search", false));
        addInspectorParameter(selectedPortalName.set("Selected Portal", ""));

        updatePortalListOnly();

        ofxOceanodeInspectorController::registerInspectorDropdown("Texture Display", "Portal", portalNames);

        selectedPortalIndex.set("Portal", 0, 0, std::max(0, (int)portalNames.size() - 1));
        addInspectorParameter(selectedPortalIndex);

        addCustomRegion(displayRegion.set("Display", [this]() { drawDisplay(); }),
                        [this]() { drawDisplay(); });

        dropdownListener = selectedPortalIndex.newListener([this](int &) {
            if (!ofxOceanodeShared::isPresetLoading()) {
                updateSelectedPortalInstance();
            }
        });

        globalSearchListener = globalSearch.newListener([this](bool &) {
            updatePortalList();
            updateSelectedPortalInstance();
        });

        presetLoadedListener = ofxOceanodeShared::getPresetHasLoadedEvent().newListener([this]() {
            updatePortalList();
            restoreSelectionByName(selectedPortalName.get());
        });

        updateSelectedPortalInstance();
    }

    void update(ofEventArgs &) override {
        static int counter = 0;
        if (++counter % 60 == 0) updatePortalList();

        if (needsDelayedRestore) {
            updatePortalListOnly();
            restoreSelectionByName(selectedPortalName.get());
            needsDelayedRestore = false;
        }
    }

    void presetRecallAfterSettingParameters(ofJson &) override {
        needsDelayedRestore = true;
    }

private:
    // Inspector parameters
    ofParameter<float>  displayWidth, displayHeight;
    ofParameter<bool>   globalSearch;
    ofParameter<string> selectedPortalName;
    ofParameter<int>    selectedPortalIndex;

    // Listeners
    ofEventListener dropdownListener, presetLoadedListener, globalSearchListener;
    customGuiRegion displayRegion;

    // Portal management
    vector<string>           portalNames;
    vector<portal<ofTexture*>*> compatiblePortals;
    portal<ofTexture*>*      selectedPortalInstance;
    bool                     needsDelayedRestore = false;

    // ---- portal list helpers (mirrors slider.h pattern) ----

    string stripDisplayName(const string &displayName) {
        string n = displayName;
        size_t slash = n.find_last_of('/');
        if (slash != string::npos) n = n.substr(slash + 1);
        if (n.size() >= 2 && n.substr(n.size() - 2) == " *") n = n.substr(0, n.size() - 2);
        return n;
    }

    void buildPortalList(vector<string> &names, vector<portal<ofTexture*>*> &portals) {
        names.clear();
        portals.clear();
        set<string> seen;
        string currentScope = getParents();

        for (auto *p : ofxOceanodeShared::getAllPortals<ofTexture*>()) {
            if (!p) continue;
            bool scopeOk = globalSearch.get()
                         ? true
                         : (p->isLocal() ? p->getParents() == currentScope : true);
            if (!scopeOk) continue;

            string pname = p->getName();
            if (seen.count(pname)) continue;
            seen.insert(pname);

            string display = pname;
            if (globalSearch.get()) {
                string scope = p->getParents();
                if (!scope.empty() && scope != currentScope) display = scope + "/" + pname;
            }
            if (!p->isLocal()) display += " *";

            names.push_back(display);
            portals.push_back(p);
        }

        if (names.empty()) {
            names.push_back("No Compatible Portals");
            portals.clear();
        }
    }

    void updatePortalListOnly() {
        buildPortalList(portalNames, compatiblePortals);
        if (compatiblePortals.empty()) selectedPortalInstance = nullptr;
    }

    void updatePortalList() {
        vector<string>              newNames;
        vector<portal<ofTexture*>*> newPortals;
        buildPortalList(newNames, newPortals);

        if (newNames == portalNames) return;

        string curName;
        if (selectedPortalIndex >= 0 && selectedPortalIndex < (int)portalNames.size())
            curName = stripDisplayName(portalNames[selectedPortalIndex]);

        portalNames      = newNames;
        compatiblePortals = newPortals;

        try {
            ofxOceanodeInspectorController::registerInspectorDropdown("Texture Display", "Portal", portalNames);
            selectedPortalIndex.setMin(0);
            selectedPortalIndex.setMax(std::max(0, (int)portalNames.size() - 1));
        } catch (...) {}

        restoreSelectionByName(curName.empty() ? selectedPortalName.get() : curName);
    }

    void restoreSelectionByName(const string &name) {
        if (name.empty()) { maintainByInstance(); return; }
        for (int i = 0; i < (int)compatiblePortals.size(); i++) {
            if (!compatiblePortals[i]) continue;
            try {
                if (compatiblePortals[i]->getName() == name) {
                    selectedPortalIndex    = i;
                    selectedPortalInstance = compatiblePortals[i];
                    return;
                }
            } catch (...) {}
        }
        maintainByInstance();
    }

    void maintainByInstance() {
        if (!selectedPortalName.get().empty()) {
            for (int i = 0; i < (int)compatiblePortals.size(); i++) {
                if (!compatiblePortals[i]) continue;
                try {
                    if (compatiblePortals[i]->getName() == selectedPortalName.get()) {
                        selectedPortalIndex    = i;
                        selectedPortalInstance = compatiblePortals[i];
                        return;
                    }
                } catch (...) {}
            }
        }
        if (selectedPortalInstance) {
            for (int i = 0; i < (int)compatiblePortals.size(); i++) {
                if (compatiblePortals[i] == selectedPortalInstance) {
                    selectedPortalIndex = i;
                    try { selectedPortalName.set(selectedPortalInstance->getName()); } catch (...) {}
                    return;
                }
            }
        }
        selectedPortalIndex    = 0;
        selectedPortalInstance = (!compatiblePortals.empty() && compatiblePortals[0])
                                 ? compatiblePortals[0] : nullptr;
        if (selectedPortalInstance) {
            try { selectedPortalName.set(selectedPortalInstance->getName()); } catch (...) {}
        } else {
            selectedPortalName.set("");
        }
    }

    void updateSelectedPortalInstance() {
        int idx = selectedPortalIndex.get();
        if (idx >= 0 && idx < (int)compatiblePortals.size() && compatiblePortals[idx]) {
            selectedPortalInstance = compatiblePortals[idx];
            try {
                string n = selectedPortalInstance->getName();
                if (selectedPortalName.get() != n) selectedPortalName.set(n);
            } catch (...) {
                selectedPortalInstance = nullptr;
                selectedPortalName.set("");
            }
        } else {
            selectedPortalInstance = nullptr;
            selectedPortalName.set("");
        }
    }

    // ---- drawing ----

    void drawDisplay() {
        float w = displayWidth.get();
        float h = displayHeight.get();

        ImVec2 pos      = ImGui::GetCursorScreenPos();
        ImDrawList *dl  = ImGui::GetWindowDrawList();

        // Reserve space so the node has the right size
        ImGui::Dummy(ImVec2(w, h));

        ofTexture *tex = nullptr;
        if (selectedPortalInstance) {
            try { tex = selectedPortalInstance->getValue(); } catch (...) {
                selectedPortalInstance = nullptr;
                selectedPortalName.set("");
            }
        }

        if (tex && tex->isAllocated()) {
            ImTextureID tid = (ImTextureID)(uintptr_t)tex->getTextureData().textureID;

            // Flip UV vertically: oF textures are stored bottom-up
            dl->AddImage(tid,
                         ImVec2(pos.x, pos.y),
                         ImVec2(pos.x + w, pos.y + h),
                         ImVec2(0, 0), ImVec2(1, 1));
        } else {
            // Dark placeholder with a label
            dl->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + h), IM_COL32(30, 30, 30, 255));
            dl->AddRect      (pos, ImVec2(pos.x + w, pos.y + h), IM_COL32(80, 80, 80, 255));
            const char *label = "No texture";
            ImVec2 ts = ImGui::CalcTextSize(label);
            dl->AddText(ImVec2(pos.x + (w - ts.x) * 0.5f, pos.y + (h - ts.y) * 0.5f),
                        IM_COL32(120, 120, 120, 255), label);
        }

        if (ImGui::IsItemHovered()) {
            string tip = selectedPortalInstance
                       ? ("Portal: " + selectedPortalName.get())
                       : "No portal selected";
            if (tex && tex->isAllocated())
                tip += "\n" + ofToString((int)tex->getWidth()) + " x " + ofToString((int)tex->getHeight());
            ImGui::SetTooltip("%s", tip.c_str());
        }
    }
};
