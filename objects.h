#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "driver.h"
#include "offsets.h"
#include "sdk.h"
#include "settings.h"
#include "imgui/imgui.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace ESP {
    struct ObjectData {
        uintptr_t actor = 0;
        Vector3 worldPosition;
        Vector3 screenPosition;
        std::string objectType;
        std::string displayName;
        float distance = 0.0f;
        ImU32 color = IM_COL32(255, 255, 255, 255);
        bool isValid = false;
        bool isGeneratorRepaired = false;
        float generatorProgress = 0.0f; // 0.0 - 1.0
    };

    inline std::vector<ObjectData> g_ObjectList;
    inline std::unordered_set<uintptr_t> g_ProcessedObjects;
    inline ULONGLONG g_LastObjectUpdate = 0;
    inline Vector3 g_CameraPosition;

    inline std::string GetObjectTypeFromName(const std::string& actorName) {
        if (actorName.find("BP_Generator") != std::string::npos ||
            actorName.find("Generator") != std::string::npos) return "Generator";

        if (actorName.find("BP_Totem") != std::string::npos ||
            actorName.find("Totem") != std::string::npos) return "Totem";

        if (actorName.find("BP_Window") != std::string::npos ||
            actorName.find("Window") != std::string::npos ||
            actorName.find("Vault") != std::string::npos) return "Window";

        if (actorName.find("BP_Pallet") != std::string::npos ||
            actorName.find("Pallet") != std::string::npos) return "Pallet";

        if (actorName.find("BP_Hatch") != std::string::npos ||
            actorName.find("Hatch") != std::string::npos ||
            actorName.find("EscapeHatch") != std::string::npos) return "Hatch";

        if (actorName.find("BP_ExitGate") != std::string::npos ||
            actorName.find("ExitGate") != std::string::npos ||
            actorName.find("Exit_Gate") != std::string::npos ||
            actorName.find("EscapeDoor") != std::string::npos) return "Exit Gate";

        if (actorName.find("BP_Hook") != std::string::npos ||
            actorName.find("Hook") != std::string::npos ||
            actorName.find("MeatHook") != std::string::npos) return "Hook";

        if (actorName.find("BP_Chest") != std::string::npos ||
            actorName.find("Chest") != std::string::npos ||
            actorName.find("SearchableBox") != std::string::npos) return "Chest";

        return "";
    }

    inline bool ShouldShowObjectType(const std::string& objectType) {
        if (objectType == "Generator") return globals.show_generators;
        if (objectType == "Totem") return globals.show_totems;
        if (objectType == "Window") return globals.show_windows;
        if (objectType == "Pallet") return globals.show_pallets;
        if (objectType == "Hatch") return globals.show_hatch;
        if (objectType == "Exit Gate") return globals.show_exit_gates;
        if (objectType == "Hook") return globals.show_hooks;
        if (objectType == "Chest") return globals.show_chests;
        return false;
    }

    inline ImU32 GetObjectColor(const std::string& objectType) {
        if (objectType == "Generator") return IM_COL32(0, 128, 255, 255);   // Blue
        if (objectType == "Totem") return IM_COL32(128, 0, 255, 255);       // Purple
        if (objectType == "Window") return IM_COL32(255, 255, 0, 255);      // Yellow
        if (objectType == "Pallet") return IM_COL32(139, 69, 19, 255);      // Brown
        if (objectType == "Hatch") return IM_COL32(255, 165, 0, 255);       // Orange
        if (objectType == "Exit Gate") return IM_COL32(0, 255, 0, 255);     // Green
        if (objectType == "Hook") return IM_COL32(255, 69, 0, 255);         // Red-Orange
        if (objectType == "Chest") return IM_COL32(255, 215, 0, 255);       // Gold
        return IM_COL32(255, 255, 255, 255); // White
    }

    inline Vector3 GetCameraPosition() {
        try {
            uintptr_t uWorld = Kernel->read<uintptr_t>(Kernel->g_process_base + offsets::GWorld);
            if (!kernel::is_valid(uWorld)) return Vector3();

            uintptr_t gameInstance = Kernel->read<uintptr_t>(uWorld + offsets::OwningGameInstance);
            uintptr_t localPlayerPtr = Kernel->read<uintptr_t>(gameInstance + offsets::LocalPlayers);
            uintptr_t localPlayer = Kernel->read<uintptr_t>(localPlayerPtr);
            uintptr_t playerController = Kernel->read<uintptr_t>(localPlayer + offsets::PlayerController);

            if (!kernel::is_valid(playerController)) return Vector3();

            uintptr_t cameraManager = Kernel->read<uintptr_t>(playerController + offsets::PlayerCameraManager);
            if (!kernel::is_valid(cameraManager)) return Vector3();

            FCameraCacheEntry cameraCache = Kernel->read<FCameraCacheEntry>(cameraManager + offsets::CameraCachePrivate);
            return cameraCache.POV.Location;
        }
        catch (...) {
            return Vector3();
        }
    }

    inline FCameraCacheEntry GetCameraCache() {
        try {
            uintptr_t uWorld = Kernel->read<uintptr_t>(Kernel->g_process_base + offsets::GWorld);
            if (!kernel::is_valid(uWorld)) return FCameraCacheEntry();

            uintptr_t gameInstance = Kernel->read<uintptr_t>(uWorld + offsets::OwningGameInstance);
            uintptr_t localPlayerPtr = Kernel->read<uintptr_t>(gameInstance + offsets::LocalPlayers);
            uintptr_t localPlayer = Kernel->read<uintptr_t>(localPlayerPtr);
            uintptr_t playerController = Kernel->read<uintptr_t>(localPlayer + offsets::PlayerController);

            if (!kernel::is_valid(playerController)) return FCameraCacheEntry();

            uintptr_t cameraManager = Kernel->read<uintptr_t>(playerController + offsets::PlayerCameraManager);
            if (!kernel::is_valid(cameraManager)) return FCameraCacheEntry();

            return Kernel->read<FCameraCacheEntry>(cameraManager + offsets::CameraCachePrivate);
        }
        catch (...) {
            return FCameraCacheEntry();
        }
    }

    inline void UpdateObjects() {
        ULONGLONG currentTime = GetTickCount64();
        if (currentTime - g_LastObjectUpdate < 100) return;
        g_LastObjectUpdate = currentTime;

        bool anyObjectEnabled = globals.show_generators || globals.show_totems ||
            globals.show_windows || globals.show_pallets ||
            globals.show_hatch || globals.show_hooks ||
            globals.show_chests;

        if (!anyObjectEnabled) {
            g_ObjectList.clear();
            return;
        }

        std::vector<ObjectData> tempObjects;
        g_ProcessedObjects.clear();
        g_CameraPosition = GetCameraPosition();

        try {
            uintptr_t uWorld = Kernel->read<uintptr_t>(Kernel->g_process_base + offsets::GWorld);
            if (!kernel::is_valid(uWorld)) return;

            uintptr_t persistentLevel = Kernel->read<uintptr_t>(uWorld + offsets::PersistentLevel);
            uintptr_t actorsArray = Kernel->read<uintptr_t>(persistentLevel + offsets::ActorArray);
            int actorsCount = Kernel->read<int>(persistentLevel + offsets::ActorCount);

            if (!kernel::is_valid(actorsArray) || actorsCount <= 0 || actorsCount > 5000) return;

            for (int i = 0; i < actorsCount && i < 3000; i++) {
                uintptr_t actor = Kernel->read<uintptr_t>(actorsArray + i * 0x8);
                if (!kernel::is_valid(actor) || g_ProcessedObjects.count(actor)) continue;

                g_ProcessedObjects.insert(actor);

                int objectId = Kernel->read<int>(actor + offsets::ActorID);
                std::string actorName = GetNameById(objectId);
                std::string objectType = GetObjectTypeFromName(actorName);

                if (objectType.empty() || !ShouldShowObjectType(objectType)) continue;

                uintptr_t rootComponent = Kernel->read<uintptr_t>(actor + offsets::RootComponent);
                if (!kernel::is_valid(rootComponent)) continue;

                Vector3 objectPos = Kernel->read<Vector3>(rootComponent + offsets::RelativeLocation);
                if (!objectPos.IsValid()) continue;

                float distance = ToMeters(g_CameraPosition.DistTo(objectPos));
                if (distance > (globals.object_esp_distance / 100.0f)) continue;

                if (objectType == "Generator") {
                    try {
                        bool isRepaired = Kernel->read<bool>(actor + offsets::GeneratorState);

                        if (isRepaired) {
                            continue;
                        }

                        float progress = Kernel->read<float>(actor + offsets::GeneratorProgress);

                        progress = std::max(0.0f, std::min(1.0f, progress));

                        ObjectData objData;
                        objData.actor = actor;
                        objData.worldPosition = objectPos;
                        objData.objectType = objectType;
                        objData.distance = distance;
                        objData.isGeneratorRepaired = isRepaired;
                        objData.generatorProgress = progress;
                        objData.isValid = true;

                        int progressPercent = static_cast<int>(progress * 100.0f);
                        objData.displayName = "Generator [" + std::to_string(progressPercent) + "%]";

                        if (progressPercent < 25) {
                            objData.color = IM_COL32(255, 50, 50, 255);
                        }
                        else if (progressPercent < 50) {
                            objData.color = IM_COL32(255, 165, 0, 255);
                        }
                        else if (progressPercent < 75) {
                            objData.color = IM_COL32(255, 255, 0, 255);
                        }
                        else {
                            objData.color = IM_COL32(50, 255, 50, 255);
                        }
                        tempObjects.push_back(objData);
                    }
                    catch (...) {
                        continue;
                    }
                }
                else {
                    ObjectData objData;
                    objData.actor = actor;
                    objData.worldPosition = objectPos;
                    objData.objectType = objectType;
                    objData.displayName = objectType;
                    objData.distance = distance;
                    objData.color = GetObjectColor(objectType);
                    objData.isValid = true;
                    tempObjects.push_back(objData);
                }
            }
            g_ObjectList = tempObjects;
        }
        catch (...) {
            g_ObjectList.clear();
        }
    }

    inline void DrawObjects(ImDrawList* drawList) {
    if (!drawList || g_ObjectList.empty()) return;
        FCameraCacheEntry cameraCache = GetCameraCache();
        if (cameraCache.POV.Location.x == 0 && cameraCache.POV.Location.y == 0) return;
        std::unordered_set<std::string> renderedPositions;
        for (auto& obj : g_ObjectList) {
            if (!obj.isValid) continue;
            Vector3 screenPos = WorldToScreen(cameraCache.POV, obj.worldPosition);
            if (screenPos.z <= 0 || screenPos.x < -100 || screenPos.x > globals.ScreenWidth + 100 || screenPos.y < -100 || screenPos.y > globals.ScreenHeight + 100) continue;
            obj.screenPosition = screenPos;

            std::string posKey = obj.objectType + "_" + std::to_string((int)(screenPos.x / 10.0f)) + "_" + std::to_string((int)(screenPos.y / 10.0f));

            if (renderedPositions.count(posKey)) continue; renderedPositions.insert(posKey);

            std::string displayText = obj.displayName + " [" + std::to_string((int)obj.distance) + "m]";

            ImVec2 textSize = ImGui::CalcTextSize(displayText.c_str());
            float textX = screenPos.x - textSize.x / 2.0f;
            float textY = screenPos.y;

            drawList->AddRectFilled(ImVec2(textX - 3, textY - 1), ImVec2(textX + textSize.x + 3, textY + textSize.y + 1), IM_COL32(0, 0, 0, 150), 2.0f);
            drawList->AddText(ImVec2(textX + 1, textY + 1), IM_COL32(0, 0, 0, 200), displayText.c_str());
            drawList->AddText(ImVec2(textX, textY), obj.color, displayText.c_str());

            if (obj.objectType == "Generator" && obj.generatorProgress > 0.0f) {
                float barWidth = textSize.x;
                float barHeight = 3.0f;
                float barY = textY + textSize.y + 2.0f;
                drawList->AddRectFilled(ImVec2(textX, barY), ImVec2(textX + barWidth, barY + barHeight), IM_COL32(50, 50, 50, 200), 1.0f);
                float progressWidth = barWidth * obj.generatorProgress;
                drawList->AddRectFilled(ImVec2(textX, barY), ImVec2(textX + progressWidth, barY + barHeight), obj.color, 1.0f);
            }
        }
    }
}