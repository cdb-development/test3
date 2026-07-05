#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <unordered_map>
#include "driver.h"
#include "offsets.h"
#include "sdk.h"
#include "settings.h"
#include "imgui/imgui.h"

namespace ESP {
    struct StableESPData {
        Vector3 lastWorldPos;
        Vector3 lastScreenPos;
        ULONGLONG lastUpdateTime;
        bool isValid;
    };

    inline std::unordered_map<uintptr_t, StableESPData> stableESPCache;

    struct ESPBoxInfo {
        float left, right, top, bottom;
        float width, height;
        float centerX, centerY;
        bool isValid = false;
    };

    inline bool IsPositionWithinMapBounds(const Vector3& worldPos) {
        const float MAP_MIN_X = -15000.0f;
        const float MAP_MAX_X = 15000.0f;
        const float MAP_MIN_Y = -15000.0f;
        const float MAP_MAX_Y = 15000.0f;
        const float MAP_MIN_Z = -10000.0f;
        const float MAP_MAX_Z = 10000.0f;

        return (worldPos.x >= MAP_MIN_X && worldPos.x <= MAP_MAX_X &&
            worldPos.y >= MAP_MIN_Y && worldPos.y <= MAP_MAX_Y &&
            worldPos.z >= MAP_MIN_Z && worldPos.z <= MAP_MAX_Z);
    }

    inline bool IsValidScreenCoordinate(const Vector3& screenPos) {
        if (screenPos.z <= 0.0) return false;
        if (!std::isfinite(screenPos.x) || !std::isfinite(screenPos.y) || !std::isfinite(screenPos.z)) return false;

        const float SCREEN_MARGIN = 150.0f;
        return (screenPos.x > -SCREEN_MARGIN &&
        screenPos.y > -SCREEN_MARGIN &&
        screenPos.x < globals.ScreenWidth + SCREEN_MARGIN &&
        screenPos.y < globals.ScreenHeight + SCREEN_MARGIN);
    }

    inline bool IsValidWorldPosition(const Vector3& pos) {
        if (abs(pos.x) > 100000.0f || abs(pos.y) > 100000.0f) return false;
        if (pos.z < -10000.0f || pos.z > 10000.0f) return false;
        if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z)) return false;
        return IsPositionWithinMapBounds(pos);
    }

    inline Vector3 GetStableScreenPosition(uintptr_t entityId, const Vector3& worldPos, const FCameraCacheEntry& currentCamera) {
        if (!IsValidWorldPosition(worldPos)) return Vector3();

        ULONGLONG currentTime = GetTickCount64();
        Vector3 directScreenPos = WorldToScreen(currentCamera.POV, worldPos);

        bool isValidScreenPos = IsValidScreenCoordinate(directScreenPos);

        if (isValidScreenPos) {
            auto& cache = stableESPCache[entityId];
            cache.lastScreenPos = directScreenPos;
            cache.lastWorldPos = worldPos;
            cache.lastUpdateTime = currentTime;
            cache.isValid = true;
            return directScreenPos;
        }
        auto cacheIt = stableESPCache.find(entityId);
        if (cacheIt != stableESPCache.end() && cacheIt->second.isValid) {
            if (IsValidWorldPosition(cacheIt->second.lastWorldPos)) {
                ULONGLONG timeDiff = currentTime - cacheIt->second.lastUpdateTime;
                if (timeDiff < 100) { 
                    return cacheIt->second.lastScreenPos;
                }
            }
        }

        return Vector3();
    }

    inline ESPBoxInfo CalculateBoxDimensions(const EntityList& entity, const FCameraCacheEntry& currentCamera) {
        ESPBoxInfo boxInfo = {};

        if (!IsValidWorldPosition(entity.origin) || !IsValidWorldPosition(entity.TopLocation)) {
            return boxInfo;
        }

        Vector3 feetScreen = GetStableScreenPosition(entity.instance, entity.origin, currentCamera);
        Vector3 headScreen = GetStableScreenPosition(entity.instance + 1, entity.TopLocation, currentCamera);

        if (!IsValidScreenCoordinate(feetScreen) || !IsValidScreenCoordinate(headScreen)) {
            return boxInfo;
        }

        float height = std::abs((float)(feetScreen.y - headScreen.y));

        if (height < 15.0f) {
            float baseHeight = entity.name == "Killer" ? 80.0f : 70.0f;
            float distanceToEntity = entity.dist;
            if (distanceToEntity > 0.0f) {
                float expectedHeight = baseHeight * (50.0f / std::max(distanceToEntity, 10.0f));
                height = std::max(height, std::min(expectedHeight, baseHeight));
            }
            else {
                height = baseHeight;
            }
        }

        float width = height * (entity.name == "Killer" ? 0.45f : 0.4f);
        float distanceMultiplier = std::max(0.8f, std::min(1.2f, 45.0f / std::max(entity.dist, 20.0f)));

        width *= distanceMultiplier;
        height *= distanceMultiplier;

        boxInfo.centerX = (float)feetScreen.x;
        boxInfo.centerY = (float)(feetScreen.y - height * 0.1f);
        boxInfo.width = width;
        boxInfo.height = height;
        boxInfo.left = boxInfo.centerX - width / 2.0f;
        boxInfo.right = boxInfo.centerX + width / 2.0f;
        boxInfo.top = boxInfo.centerY - height / 2.0f + 10.0f;
        boxInfo.bottom = boxInfo.centerY + height / 2.0f + 10.0f;

        boxInfo.isValid = (boxInfo.left > -100 && boxInfo.right < globals.ScreenWidth + 100 &&
            boxInfo.top > -100 && boxInfo.bottom < globals.ScreenHeight + 100 &&
            boxInfo.width > 5.0f && boxInfo.height > 10.0f);

        return boxInfo;
    }

    inline void DrawTracer(ImDrawList* drawList, const EntityList& entity, const FCameraCacheEntry& currentCamera, ImU32 color) {
        if (!drawList) return;

        try {
            Vector3 screenPos = GetStableScreenPosition(entity.instance, entity.origin, currentCamera);
            if (!IsValidScreenCoordinate(screenPos)) return;

            ImVec2 startPos, endPos;

            if (globals.tracer_type == 0) {
                startPos = ImVec2(globals.ScreenWidth / 2.0f, globals.ScreenHeight / 2.0f);
            }
            else if (globals.tracer_type == 1) {
                startPos = ImVec2(globals.ScreenWidth / 2.0f, globals.ScreenHeight);
            }
            else {
                startPos = ImVec2(globals.ScreenWidth / 2.0f, 0.0f);
            }

            endPos = ImVec2((float)screenPos.x, (float)screenPos.y);

            if (endPos.x < -50 || endPos.x > globals.ScreenWidth + 50 ||
                endPos.y < -50 || endPos.y > globals.ScreenHeight + 50) {
                return;
            }

            float tracerThickness = std::max(0.8f, std::min(2.0f, 1.0f + (60.0f / std::max(entity.dist, 25.0f))));

            ImU32 tracerColor = color;
            if (entity.name == "Killer") {
                tracerColor = IM_COL32(
                    (int)(globals.killer_tracer_color[0] * 255),
                    (int)(globals.killer_tracer_color[1] * 255),
                    (int)(globals.killer_tracer_color[2] * 255),
                    (int)(globals.killer_tracer_color[3] * 255)
                );
            }
            else if (entity.name == "Survivor") {
                tracerColor = IM_COL32(
                    (int)(globals.survivor_tracer_color[0] * 255),
                    (int)(globals.survivor_tracer_color[1] * 255),
                    (int)(globals.survivor_tracer_color[2] * 255),
                    (int)(globals.survivor_tracer_color[3] * 255)
                );
            }

            drawList->AddLine(startPos, endPos, tracerColor, tracerThickness);

        }
        catch (...) {}
    }

    inline ESPBoxInfo DrawPlayerESPBox(ImDrawList* drawList, const EntityList& entity, const FCameraCacheEntry& currentCamera, ImU32 color) {
        ESPBoxInfo boxInfo = {};

        try {
            boxInfo = CalculateBoxDimensions(entity, currentCamera);

            if (!boxInfo.isValid) {
                return boxInfo;
            }

            float distanceThickness = std::max(1.2f, std::min(2.8f, 1.5f + (80.0f / std::max(entity.dist, 25.0f))));
            ImU32 finalColor = entity.name == "Killer" ? IM_COL32(255, 80, 80, 255) : color;

            if (globals.esp_box_type == 0) {
                drawList->AddRect(ImVec2(boxInfo.left, boxInfo.top),
                    ImVec2(boxInfo.right, boxInfo.bottom),
                    finalColor, 0.0f, 0, distanceThickness);
            }
            else if (globals.esp_box_type == 1) {
                ImU32 fillColor = IM_COL32(10, 10, 10, 130);

                drawList->AddRectFilled(
                    ImVec2(boxInfo.left, boxInfo.top),
                    ImVec2(boxInfo.right, boxInfo.bottom),
                    fillColor,
                    0.0f
                );

                drawList->AddRect(
                    ImVec2(boxInfo.left, boxInfo.top),
                    ImVec2(boxInfo.right, boxInfo.bottom),
                    finalColor,
                    0.0f, 0,
                    distanceThickness
                );
            }
            else if (globals.esp_box_type == 2) {
                float cornerLength = std::min(boxInfo.width * 0.3f, boxInfo.height * 0.25f);

                drawList->AddLine(ImVec2(boxInfo.left, boxInfo.top),
                    ImVec2(boxInfo.left + cornerLength, boxInfo.top),
                    finalColor, distanceThickness);
                drawList->AddLine(ImVec2(boxInfo.left, boxInfo.top),
                    ImVec2(boxInfo.left, boxInfo.top + cornerLength),
                    finalColor, distanceThickness);

                drawList->AddLine(ImVec2(boxInfo.right, boxInfo.top),
                    ImVec2(boxInfo.right - cornerLength, boxInfo.top),
                    finalColor, distanceThickness);
                drawList->AddLine(ImVec2(boxInfo.right, boxInfo.top),
                    ImVec2(boxInfo.right, boxInfo.top + cornerLength),
                    finalColor, distanceThickness);

                drawList->AddLine(ImVec2(boxInfo.left, boxInfo.bottom),
                    ImVec2(boxInfo.left + cornerLength, boxInfo.bottom),
                    finalColor, distanceThickness);
                drawList->AddLine(ImVec2(boxInfo.left, boxInfo.bottom),
                    ImVec2(boxInfo.left, boxInfo.bottom - cornerLength),
                    finalColor, distanceThickness);

                drawList->AddLine(ImVec2(boxInfo.right, boxInfo.bottom),
                    ImVec2(boxInfo.right - cornerLength, boxInfo.bottom),
                    finalColor, distanceThickness);
                drawList->AddLine(ImVec2(boxInfo.right, boxInfo.bottom),
                    ImVec2(boxInfo.right, boxInfo.bottom - cornerLength),
                    finalColor, distanceThickness);
            }

        }
        catch (...) {}

        return boxInfo;
    }

    inline void RenderESP(ImDrawList* drawList, const std::vector<EntityList>& entityList, const std::string& killerName, const FCameraCacheEntry& currentCamera) {
        if (!drawList) return;
        if (!killerName.empty() && globals.show_names) {
            std::string text = "KILLER: " + killerName;
            ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
            ImU32 borderColor = IM_COL32(255, 80, 80, 220);
            ImU32 textColor = IM_COL32(255, 255, 255, 255);

            drawList->AddRectFilled(ImVec2(10, 10), ImVec2(30 + textSize.x, 30 + textSize.y), IM_COL32(0, 0, 0, 180), 5.0f);
            drawList->AddRect(ImVec2(10, 10), ImVec2(30 + textSize.x, 30 + textSize.y), borderColor, 5.0f, 0, 2.0f);
            drawList->AddText(ImVec2(18, 18), IM_COL32(0, 0, 0, 200), text.c_str());
            drawList->AddText(ImVec2(17, 17), textColor, text.c_str());
        }

        if (!globals.esp_enabled) return;

        for (const auto& entity : entityList) {
            // Frühzeitige Prüfung auf gültige Welt-Position
            if (!IsValidWorldPosition(entity.origin)) {
                continue;
            }

            ImU32 color = IM_COL32(255, 255, 255, 255);
            bool shouldRender = false;
            bool isSurvivor = false;
            bool isKiller = false;

            std::string actorNameFromId = GetNameById(entity.objectId);
            std::string survivorName = GetSurvivorName(actorNameFromId);

            if (entity.name != "Killer" && (entity.name == "Survivor" || survivorName != "Unknown Survivor")) {
                if (globals.show_survivors) {
                    color = IM_COL32((int)(globals.survivor_color[0] * 255), (int)(globals.survivor_color[1] * 255), (int)(globals.survivor_color[2] * 255), 255);
                    shouldRender = true;
                    isSurvivor = true;
                }
            }
            else if (entity.name == "Killer") {
                if (globals.show_killers) {
                    color = IM_COL32(255, 80, 80, 255);
                    shouldRender = true;
                    isKiller = true;
                }
            }

            if (shouldRender) {
                Vector3 screenPos = GetStableScreenPosition(entity.instance, entity.origin, currentCamera);
                if (!IsValidScreenCoordinate(screenPos)) continue;

                ESPBoxInfo boxInfo = DrawPlayerESPBox(drawList, entity, currentCamera, color);

                // Nur rendern wenn die Box gültig ist
                if (!boxInfo.isValid) continue;

                // Tracer zeichnen
                if ((isSurvivor && globals.show_survivor_tracers) ||
                    (isKiller && globals.show_killer_tracers)) {
                    DrawTracer(drawList, entity, currentCamera, color);
                }

                // Namen und Distanz anzeigen
                if ((globals.show_names || globals.show_distance)) {
                    std::string displayText = "";

                    if (globals.show_names) {
                        if (isKiller && !killerName.empty()) {
                            displayText = killerName;
                        }
                        else if (isSurvivor) {
                            if (survivorName != "Unknown Survivor" && globals.show_survivor_names) {
                                displayText = survivorName;
                            }
                            else {
                                displayText = entity.name;
                            }
                        }
                    }

                    if (globals.show_distance) {
                        if (!displayText.empty()) {
                            displayText += " [" + std::to_string((int)entity.dist) + "m]";
                        }
                        else {
                            displayText = std::to_string((int)entity.dist) + "m";
                        }
                    }

                    if (!displayText.empty()) {
                        ImVec2 textSize = ImGui::CalcTextSize(displayText.c_str());

                        float textX = boxInfo.centerX - textSize.x / 2.0f;
                        float textY = boxInfo.top - textSize.y - 8.0f;

                        if (textY > 10 && textY < globals.ScreenHeight - textSize.y - 10 &&
                            textX > 10 && textX < globals.ScreenWidth - textSize.x - 10) {

                            drawList->AddRectFilled(
                                ImVec2(textX - 4, textY - 2),
                                ImVec2(textX + textSize.x + 4, textY + textSize.y + 2),
                                IM_COL32(0, 0, 0, 160), 3.0f);

                            drawList->AddText(
                                ImVec2(textX + 1, textY + 1),
                                IM_COL32(0, 0, 0, 220), displayText.c_str());

                            ImU32 textColor = isKiller ? IM_COL32(255, 120, 120, 255) : color;
                            drawList->AddText(ImVec2(textX, textY), textColor, displayText.c_str());
                        }
                    }
                }
            }
        }

        // Alte Cache-Einträge bereinigen
        ULONGLONG currentTime = GetTickCount64();
        for (auto it = stableESPCache.begin(); it != stableESPCache.end(); ) {
            if (currentTime - it->second.lastUpdateTime > 5000) { // 5 Sekunden
                it = stableESPCache.erase(it);
            }
            else {
                ++it;
            }
        }
    }
}