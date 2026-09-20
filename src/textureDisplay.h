#pragma once

#include "ofxOceanodeNodeModel.h"
#include "ofxOceanodeShared.h"
#include "ofxOceanodeInspectorController.h"
#include "portal.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <set>
#include <map>

class textureDisplay : public ofxOceanodeNodeModel {
public:
    textureDisplay() : ofxOceanodeNodeModel("Texture Display") {
        selectedPortalInstance = nullptr;
    }

    void setup() override {
        description = "Displays a texture from a portal with a resizable area.\n"
                      "Accepts a direct ofTexture* input (top-left pin) which overrides the portal selection.";

        setFlags(ofxOceanodeNodeModelFlags_TransparentNode);

        addInspectorParameter(displayWidth.set("Width",  320.f, 32.f, 4096.f));
        addInspectorParameter(displayHeight.set("Height", 180.f, 32.f, 4096.f));
        addInspectorParameter(globalSearch.set("Global Search", false));
        addInspectorParameter(selectedPortalName.set("Selected Portal", ""));

        updatePortalListOnly();

        ofxOceanodeInspectorController::registerInspectorDropdown("Texture Display", "Portal", portalNames);

        selectedPortalIndex.set("Portal", 0, 0, std::max(0, (int)portalNames.size() - 1));
        addInspectorParameter(selectedPortalIndex);

        // ---- Custom display region (registered FIRST) ----
        // Added before the NoGuiWidget input parameter so that drawDisplay()
        // gets to lay out the texture rect first, and can position the
        // ImGui cursor for the next parameter (the input pin Dummy) to land
        // at the vertical center of the texture.
        addCustomRegion(displayRegion.set("Display", [this]() { drawDisplay(); }),
                        [this]() { drawDisplay(); });

        // ---- Direct texture input (connection pin only, no widget) ----
        // Registered AFTER the display region. Its pin Y is captured from the
        // ImGui cursor at the moment the NoGuiWidget Dummy(0,0) runs — and
        // drawDisplay() leaves the cursor at the vertical center of the
        // texture, so the canvas-drawn connection bullet lands centered on
        // the middle of the texture's left edge.
        // It is input-only (DisableOutConnection) and has no on-node widget
        // (NoGuiWidget).
        inputTextureParam = addParameter(inputTexture.set("Texture In", nullptr));
        if (inputTextureParam) {
            inputTextureParam->setFlags(inputTextureParam->getFlags()
                                        | ofxOceanodeParameterFlags_NoGuiWidget
                                        | ofxOceanodeParameterFlags_DisableOutConnection);
        }

        // Listen for input texture changes: when a NEW connection brings a
        // texture to the input pin, request an auto-fit.
        //
        // NOTE: ofParameter<ofTexture*> is set every frame by the upstream
        // node while a connection is active, so a naive "t != nullptr"
        // listener would fire every frame and reset the user's manual
        // resize on the next update() tick. We therefore only trigger
        // auto-fit on a *transition*: nullptr → non-null (fresh connect)
        // or pointer-change → different upstream texture.
        lastSeenInputTexture = nullptr;
        inputTextureListener = inputTexture.newListener([this](ofTexture* &t) {
            if (t != nullptr && t != lastSeenInputTexture) {
                // True connection event (or upstream texture pointer changed):
                // schedule a one-shot auto-fit.
                needsAutoFit = true;
            }
            lastSeenInputTexture = t;
        });

        dropdownListener = selectedPortalIndex.newListener([this](int &) {
            if (!ofxOceanodeShared::isPresetLoading()) {
                updateSelectedPortalInstance();
                // Only treat this as a USER-driven portal change when the
                // write to selectedPortalIndex did NOT come from our own
                // internal reconciliation (restoreSelectionByName /
                // maintainByInstance / periodic updatePortalList()).
                // Otherwise the appearance of a new portal in the patch
                // would wipe out the user's manual resize.
                if (!suppressDropdownAutoFit) {
                    needsAutoFit = true;
                }
            }
        });

        globalSearchListener = globalSearch.newListener([this](bool &) {
            updatePortalList();
            updateSelectedPortalInstance();
            // Treat as a portal-source change.
            needsAutoFit = true;
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

        // Keep the canvas synchronized with the active texture's dimensions.
        // This is checked every frame because an ofTexture/ofFbo can be
        // reallocated at a different resolution without changing its pointer.
        // The input pin has priority over the portal selection.
        ofTexture *tex = resolveActiveTexture();
        if (tex && tex->isAllocated()
            && tex->getWidth() > 0 && tex->getHeight() > 0) {
            const float texWidth  = tex->getWidth();
            const float texHeight = tex->getHeight();
            const bool dimensionsChanged = hasObservedTextureDimensions
                && (texWidth != lastObservedTextureWidth
                    || texHeight != lastObservedTextureHeight);

            if (needsAutoFit) {
                applyAutoFit(tex->getWidth() / tex->getHeight());
                needsAutoFit = false;
            } else if (dimensionsChanged) {
                // Preserve the user's current canvas width while deriving the
                // height from the new texture aspect ratio.
                applyAspectToCurrentWidth(texWidth / texHeight);
            }

            lastObservedTextureWidth  = texWidth;
            lastObservedTextureHeight = texHeight;
            hasObservedTextureDimensions = true;
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

    // Direct texture input (pin-only parameter, no widget on the node)
    ofParameter<ofTexture*> inputTexture;
    shared_ptr<ofxOceanodeAbstractParameter> inputTextureParam;
    // Tracks the last texture pointer observed on the input listener so we
    // can detect *transitions* (fresh connection / source change) and avoid
    // re-triggering auto-fit every frame while a connection is active.
    ofTexture *lastSeenInputTexture = nullptr;

    // Listeners
    ofEventListener dropdownListener, presetLoadedListener, globalSearchListener;
    ofEventListener inputTextureListener;
    customGuiRegion displayRegion;

    // Portal management
    vector<string>           portalNames;
    vector<portal<ofTexture*>*> compatiblePortals;
    portal<ofTexture*>*      selectedPortalInstance;
    bool                     needsDelayedRestore = false;

    // True while we are programmatically reconciling selectedPortalIndex
    // (e.g. after a portal list refresh). The dropdownListener consults
    // this flag to avoid mis-classifying internal updates as user actions
    // and thus avoid triggering an unwanted auto-fit / resize.
    bool                     suppressDropdownAutoFit = false;

    // Resize-handle state
    bool                     isResizing = false;

    // Auto-fit state: set true when the portal source changes (user action),
    // consumed once a valid texture is observed. NEVER set from preset-recall.
    bool                     needsAutoFit = false;

    // Last valid dimensions seen on the active texture. Texture producers
    // commonly resize an existing ofTexture/ofFbo in place, so pointer-change
    // detection alone cannot keep the canvas aspect ratio synchronized.
    float                    lastObservedTextureWidth = 0.f;
    float                    lastObservedTextureHeight = 0.f;
    bool                     hasObservedTextureDimensions = false;

    // Default width used by auto-fit (matches displayWidth default).
    static constexpr float   kDefaultWidth = 320.f;

    // ---- aspect-ratio helpers ----

    // Clamp (w, h) to [min, max] while preserving the given aspect (w/h).
    // Width is treated as the driving axis; height is derived. If height
    // clamps, width is re-derived to keep aspect.
    std::pair<float, float> applyAspectClamp(float w, float h, float aspect) const {
        const float minV = displayWidth.getMin();
        const float maxV = displayWidth.getMax();
        if (aspect <= 0.f || !std::isfinite(aspect)) {
            // Defensive: free clamp
            w = ofClamp(w, minV, maxV);
            h = ofClamp(h, displayHeight.getMin(), displayHeight.getMax());
            return {w, h};
        }
        // Drive from width, derive height
        w = ofClamp(w, minV, maxV);
        h = w / aspect;
        if (h < displayHeight.getMin()) {
            h = displayHeight.getMin();
            w = h * aspect;
            w = ofClamp(w, minV, maxV);
        } else if (h > displayHeight.getMax()) {
            h = displayHeight.getMax();
            w = h * aspect;
            w = ofClamp(w, minV, maxV);
        }
        return {w, h};
    }

    void applyAutoFit(float aspect) {
        if (aspect <= 0.f || !std::isfinite(aspect)) return;
        auto [nw, nh] = applyAspectClamp(kDefaultWidth, kDefaultWidth / aspect, aspect);
        displayWidth.set(nw);
        displayHeight.set(nh);
    }

    void applyAspectToCurrentWidth(float aspect) {
        if (aspect <= 0.f || !std::isfinite(aspect)) return;
        const float currentWidth = displayWidth.get();
        auto [nw, nh] = applyAspectClamp(currentWidth, currentWidth / aspect, aspect);
        displayWidth.set(nw);
        displayHeight.set(nh);
    }

    // Returns true if `p` is currently a live portal owned by the shared
    // portal manager. Guards against use-after-free when a portal node is
    // deleted between our periodic updatePortalList() calls.
    bool isPortalLive(portal<ofTexture*>* p) {
        if (!p) return false;
        try {
            auto allLive = ofxOceanodeShared::getAllPortals<ofTexture*>();
            for (auto *q : allLive) {
                if (q == p) return true;
            }
        } catch (...) {
            return false;
        }
        return false;
    }

    // ---- Active texture resolution ----
    // Priority: a texture connected on the input pin overrides the portal selection.
    // Returns nullptr if neither source is available.
    ofTexture *resolveActiveTexture() {
        // 1) Direct input pin (highest priority)
        ofTexture *t = inputTexture.get();
        if (t != nullptr) return t;

        // 2) Fallback to selected portal — but ONLY if it's still alive.
        //    A portal may have been deleted since our last updatePortalList(),
        //    in which case dereferencing selectedPortalInstance would crash
        //    (use-after-free). Validate against the live portal list first.
        if (selectedPortalInstance) {
            if (!isPortalLive(selectedPortalInstance)) {
                // Portal was deleted under us. Clear cached state and force
                // a refresh on the next update() tick.
                selectedPortalInstance = nullptr;
                try { selectedPortalName.set(""); } catch (...) {}
                // Also drop any stale entries in compatiblePortals so the
                // dropdown listener / inspector see a clean state.
                updatePortalListOnly();
                return nullptr;
            }
            try {
                return selectedPortalInstance->getValue();
            } catch (...) {
                selectedPortalInstance = nullptr;
                selectedPortalName.set("");
            }
        }
        return nullptr;
    }

    // True when the active texture source is the direct input pin (not the portal).
    bool isInputTextureActive() const {
        return inputTexture.get() != nullptr;
    }

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
        // Any write to selectedPortalIndex here is a programmatic
        // reconciliation, NOT a user action — suppress auto-fit.
        struct AutoFitGuard {
            bool &flag;
            AutoFitGuard(bool &f) : flag(f) { flag = true; }
            ~AutoFitGuard() { flag = false; }
        } guard(suppressDropdownAutoFit);

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
        // Any write to selectedPortalIndex here is also a programmatic
        // reconciliation — suppress auto-fit. (Nested with restoreSelectionByName's
        // own guard; both writes are no-ops to the flag while it's already true.)
        struct AutoFitGuard {
            bool &flag;
            bool  prev;
            AutoFitGuard(bool &f) : flag(f), prev(f) { flag = true; }
            ~AutoFitGuard() { flag = prev; }
        } guard(suppressDropdownAutoFit);

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

        float zoom = std::max(0.1f, ofxOceanodeShared::getZoomLevel());
        float screenW = w * zoom;
        float screenH = h * zoom;

        ImVec2 cursor   = ImGui::GetCursorScreenPos();
        ImDrawList *dl  = ImGui::GetWindowDrawList();

        // ── Visual shift: align the visible texture's top-left with the
        // canvas-drawn connection bullet for the input pin.
        //
        // The canvas draws the input bullet at
        //     bulletPos = group.MinX - NODE_WINDOW_PADDING.x * zoom
        // (= cursor.x - 8*zoom in our case, since the layout reservation
        //  below starts at cursor.x). By shifting only the visible draws
        //  (image, placeholder, grip, hit-test) LEFT by 8*zoom, the bullet
        //  ends up centered exactly on the texture's top-left pixel, while
        //  the node's body footprint stays the same.
        // NODE_WINDOW_PADDING.x in ofxOceanode is 8.0f.
        const float kBulletOffsetX = 8.f * zoom;

        ImVec2 pos    = ImVec2(cursor.x - kBulletOffsetX, cursor.y);
        ImVec2 posEnd = ImVec2(pos.x + screenW, pos.y + screenH);

        // ── Resize handle hit-test (bottom-right corner) ──────────────────
        // Handle is a small mid-gray FILLED CIRCLE in the bottom-right
        // corner, mirroring the style/size used by postItNote.h (radius
        // ~6 * zoom). The hit-test is circular (distance from center ≤ R).
        const float handleR  = std::max(3.f, 6.f * zoom);
        ImVec2 handleCenter  = ImVec2(posEnd.x - handleR, posEnd.y - handleR);

        ImVec2 mouse = ImGui::GetMousePos();
        float hdx = mouse.x - handleCenter.x;
        float hdy = mouse.y - handleCenter.y;
        bool inResize = (handleR > 0.f) && (hdx*hdx + hdy*hdy) <= handleR*handleR;

        // ── Probe current texture aspect (for aspect-locked drag) ─────────
        // The input pin has priority over the portal selection.
        float texAspect = 0.f;
        {
            ofTexture *probeTex = resolveActiveTexture();
            if (probeTex && probeTex->isAllocated()
                && probeTex->getWidth() > 0 && probeTex->getHeight() > 0) {
                texAspect = probeTex->getWidth() / probeTex->getHeight();
            }
        }

        // ── Resize drag logic (must run BEFORE Dummy/InvisibleButton) ─────
        {
            static std::map<void*, ImVec2> rdMouseMap;
            static std::map<void*, float>  rdWMap;
            static std::map<void*, float>  rdHMap;

            ImVec2& rdMp = rdMouseMap[this];
            float&  rdW  = rdWMap[this];
            float&  rdH  = rdHMap[this];

            bool mouseDown  = ImGui::IsMouseDown(ImGuiMouseButton_Left);
            bool mouseClick = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

            if (mouseClick && inResize && !isResizing) {
                isResizing = true;
                rdMp = mouse;
                rdW  = displayWidth.get();
                rdH  = displayHeight.get();
            }

            if (isResizing) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
                if (mouseDown) {
                    float z       = std::max(0.1f, ofxOceanodeShared::getZoomLevel());
                    float deltaX  = (mouse.x - rdMp.x) / z;
                    float deltaY  = (mouse.y - rdMp.y) / z;

                    // Live modifier check: Shift = free resize, otherwise aspect-locked
                    // (only if a valid texture aspect is available).
                    bool shiftHeld  = ImGui::GetIO().KeyShift;
                    bool freeResize = shiftHeld || (texAspect <= 0.f);

                    float newW, newH;
                    if (freeResize) {
                        newW = ofClamp(rdW + deltaX, displayWidth.getMin(),  displayWidth.getMax());
                        newH = ofClamp(rdH + deltaY, displayHeight.getMin(), displayHeight.getMax());
                    } else {
                        // Pick the dominant axis (normalized) to feel natural,
                        // then derive the other axis from the texture aspect.
                        float normX = (rdW > 0.f) ? (deltaX / rdW) : 0.f;
                        float normY = (rdH > 0.f) ? (deltaY / rdH) : 0.f;
                        float candW, candH;
                        if (std::fabs(normX) >= std::fabs(normY)) {
                            candW = rdW + deltaX;
                            candH = candW / texAspect;
                        } else {
                            candH = rdH + deltaY;
                            candW = candH * texAspect;
                        }
                        auto clamped = applyAspectClamp(candW, candH, texAspect);
                        newW = clamped.first;
                        newH = clamped.second;
                    }

                    displayWidth.set(newW);
                    displayHeight.set(newH);
                    // Refresh local copies so the rest of this frame uses the new size
                    w       = displayWidth.get();
                    h       = displayHeight.get();
                    screenW = w * zoom;
                    screenH = h * zoom;
                    posEnd  = ImVec2(pos.x + screenW, pos.y + screenH);
                    handleCenter = ImVec2(posEnd.x - handleR, posEnd.y - handleR);
                } else {
                    isResizing = false;
                }
            }
        }

        // ── Reserve space so the canvas knows the node footprint ──────────
        // Layout reservation stays at the original cursor position (NOT
        // shifted), so the node body width is exactly screenW. Only the
        // visible texture image (drawn via dl->AddImage below) is shifted
        // LEFT by kBulletOffsetX to overlap the bullet.
        // While resizing, use InvisibleButton so the canvas does not try to
        // move the node (mirrors postItNote pattern).
        if (isResizing) {
            ImGui::InvisibleButton("##TextureDisplayResize", ImVec2(screenW, screenH));
        } else {
            ImGui::Dummy(ImVec2(screenW, screenH));
        }

        // Resolve the active texture (input pin > portal).
        ofTexture *tex = resolveActiveTexture();

        if (tex && tex->isAllocated()) {
            ImTextureID tid = (ImTextureID)(uintptr_t)tex->getTextureData().textureID;

            // Flip UV vertically: oF textures are stored bottom-up
            dl->AddImage(tid,
                         ImVec2(pos.x, pos.y),
                         ImVec2(pos.x + screenW, pos.y + screenH),
                         ImVec2(0, 0), ImVec2(1, 1));
        } else {
            // Dark placeholder with a centered label.
            dl->AddRectFilled(pos, ImVec2(pos.x + screenW, pos.y + screenH), IM_COL32(30, 30, 30, 255));
            const char *label = "No texture";
            ImVec2 ts = ImGui::CalcTextSize(label);
            dl->AddText(ImVec2(pos.x + (screenW - ts.x) * 0.5f, pos.y + (screenH - ts.y) * 0.5f),
                        IM_COL32(120, 120, 120, 255), label);
        }

        // Fixed one-screen-pixel frame. Keep it independent of canvas zoom so
        // transparent textures always retain a clear, unobtrusive boundary.
        const float borderInset = 0.5f;
        dl->AddRect(ImVec2(pos.x + borderInset, pos.y + borderInset),
                    ImVec2(posEnd.x - borderInset, posEnd.y - borderInset),
                    IM_COL32(0, 0, 0, 255), 0.f, 0, 1.f);

        // ── Resize grip (simple mid-gray filled circle) ───────────────────
        // Mirrors the postItNote.h style: a single AddCircleFilled, darker
        // when hovered or actively dragging, lighter otherwise.
        {
            ImU32 rzCol = (inResize || isResizing)
                ? IM_COL32(160, 160, 160, 230)   // slightly brighter on hover/drag
                : IM_COL32(128, 128, 128, 200);  // mid-gray at rest
            dl->AddCircleFilled(handleCenter, handleR, rzCol);

            if (inResize || isResizing) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
            }
        }

        // Grip-specific tooltip: explain modifier behavior.
        // (No body tooltip, no in-display marker — the canvas-drawn input
        // connection bullet sits at the vertical center of the texture's
        // left edge.)
        if (inResize && !isResizing) {
            ImGui::SetTooltip("%s", "Drag to resize (aspect-locked). Hold Shift for free resize.");
        }

        // ── Position the cursor for the NEXT parameter (the NoGuiWidget
        // input pin Dummy) so that the canvas-drawn connection bullet ends
        // up vertically centered on the texture. The next Dummy(0,0) runs
        // at whatever cursor we leave here; its GetItemRectMin().y becomes
        // pin.y, and the bullet draws at (group.MinX - 8*zoom, pin.y).
        // X is kept at the original cursor.x so group.MinX is unchanged
        // (still equals cursor.x = pos.x + 8*zoom), which keeps the bullet's
        // X aligned with the texture's left edge.
        ImGui::SetCursorScreenPos(ImVec2(cursor.x,
                                         pos.y + screenH * 0.5f));
    }
};
