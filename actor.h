#define IMGUI_DEFINE_MATH_OPERATORS
#define NOMINMAX
#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <unordered_map>
#include "driver.h"
#include "offsets.h"
#include "sdk.h"
#include "settings.h"
#include "esp.h"
#include "imgui/imgui.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

enum class EPalletState : uint8_t {
    Up = 0,
    Falling = 1,
    Fallen = 2,
    Destroyed = 3,
    EPalletState_MAX = 4,
};

namespace ActorSystem {
    inline FCameraCacheEntry cameraCache;
    inline FCameraCacheEntry originalCameraCache;
    inline uintptr_t uWorld, gameInstance, persistentLevel, localPlayerPtr, localPlayer;
    inline uintptr_t playerController, PlayerStateLocalPlayer, Localpawn, cameraManager;
    inline uintptr_t actorsArray;
    inline int actorsCount;
    inline std::string g_KillerName = "";

    std::vector<EntityList> entityList;
    std::unordered_map<uintptr_t, float> distanceCache;
    std::unordered_map<uintptr_t, Vector3> positionCache;
    std::unordered_map<uintptr_t, std::vector<Vector3>> boneWorldCache;
    std::unordered_map<uintptr_t, ULONGLONG> boneCacheTime;

    inline ULONGLONG lastSkillCheckTime = 0;
    inline bool wasSkillCheckActive = false;

    inline double VectorDistance(const Vector3& a, const Vector3& b) {
        Vector3 diff = Vector3(b.x - a.x, b.y - a.y, b.z - a.z);
        return sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    }

    inline bool IsLocalPlayer(uintptr_t actor, uintptr_t playerState, uintptr_t pawn) {
        if (Localpawn != 0 && actor == Localpawn) return true;
        if (PlayerStateLocalPlayer != 0 && playerState != 0 && playerState == PlayerStateLocalPlayer) return true;
        if (Localpawn != 0 && pawn != 0 && pawn == Localpawn) return true;
        return false;
    }

    inline bool IsValidWorldPosition(const Vector3& pos) {
        if (abs(pos.x) > 100000.0f || abs(pos.y) > 100000.0f) return false;
        if (pos.z < -10000.0f || pos.z > 10000.0f) return false;
        if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z)) return false;
        return true;
    }

    inline Vector3 GetSmoothedPosition(uintptr_t entityId, const Vector3& worldPos) {
        if (!IsValidWorldPosition(worldPos)) return Vector3();

        auto& smoothPos = positionCache[entityId];
        if (VectorDistance(smoothPos, worldPos) > 500.0f) smoothPos = worldPos;
        else {
            smoothPos = Vector3(
                smoothPos.x * 0.85f + worldPos.x * 0.15f,
                smoothPos.y * 0.85f + worldPos.y * 0.15f,
                smoothPos.z * 0.85f + worldPos.z * 0.15f
            );
        }
        return smoothPos;
    }

    inline FCameraCacheEntry GetCorrectedCameraCache() {
        FCameraCacheEntry correctedCache = originalCameraCache;
        if (globals.fov_enabled && globals.custom_fov != globals.original_fov && globals.original_fov > 0.0f) {
            correctedCache.POV.FOV = globals.custom_fov;
        }
        return correctedCache;
    }

    inline Vector3 GetBoneWorldPosition(uintptr_t mesh, int boneIndex) {
        return(Kernel->read<Vector3>(mesh + offsets::BoneArray + (boneIndex * 0x30) + 0x10));
    }

    inline std::string GetEntityType(const std::string& actorName) {
        if (IsKillerEntity(actorName)) return "Killer";

        if (actorName.find("BP_Camper") != std::string::npos) return "Survivor";
        if (actorName.find("Camper") != std::string::npos) return "Survivor";

        std::string survivorName = GetSurvivorName(actorName);
        if (survivorName != "Unknown Survivor") return "Survivor";

        return "";
    }

    inline std::vector<Vector3> GetAllBonePositions(uintptr_t actor) {
        ULONGLONG currentTime = GetTickCount64();
        auto cacheIt = boneCacheTime.find(actor);
        if (cacheIt != boneCacheTime.end() && (currentTime - cacheIt->second) < 50) {
            auto boneIt = boneWorldCache.find(actor);
            if (boneIt != boneWorldCache.end() && !boneIt->second.empty()) {
                return boneIt->second;
            }
        }

        std::vector<Vector3> bones(20);
        try {
            uintptr_t mesh = Kernel->read<uintptr_t>(actor + offsets::CharacterMesh);
            if (!kernel::is_valid(mesh)) return bones;

            for (int i = 0; i < 20; i++) {
                bones[i] = GetBoneWorldPosition(mesh, i);
            }

            int validBones = 0;
            for (const auto& bone : bones) {
                if (bone.IsValid()) validBones++;
            }

            if (validBones >= 5) {
                boneWorldCache[actor] = bones;
                boneCacheTime[actor] = currentTime;
            }
            else bones.clear();

            return bones;
        }
        catch (...) { return bones; }
    }

    inline std::string GetKillerName(const std::string& actorName) {
        return GetKillerNameFromActor(actorName);
    }

    inline uintptr_t FindOutlineComponent(uintptr_t actor) {
        try {
            uintptr_t blueprintComponents = Kernel->read<uintptr_t>(actor + 0x288);
            if (!kernel::is_valid(blueprintComponents)) return 0;

            int componentCount = Kernel->read<int>(blueprintComponents + 0x8);
            if (componentCount <= 0 || componentCount > 100) return 0;

            uintptr_t componentArray = Kernel->read<uintptr_t>(blueprintComponents + 0x0);

            for (int i = 0; i < componentCount; i++) {
                uintptr_t component = Kernel->read<uintptr_t>(componentArray + i * 0x8);
                if (!kernel::is_valid(component)) continue;

                std::string componentName = GetNameById(Kernel->read<int>(component + 0x18));
                if (componentName.find("DBDOutline") != std::string::npos) {
                    return component;
                }
            }
        }
        catch (...) {}
        return 0;
    }

    inline void ProcessAura() {
        if (globals.aura) {
            for (const auto& entity : entityList) {
                if (!kernel::is_valid(entity.instance)) continue;
                std::string actorName = GetNameById(entity.objectId);
                if (actorName.find("BP_Camper") == std::string::npos) continue;

                uintptr_t outlineComponent = Kernel->read<uintptr_t>(entity.instance + offsets::SurvivorDBDOutline);
                if (!kernel::is_valid(outlineComponent)) continue;

                Kernel->write<float>(outlineComponent + 0x2E0, 0.001f);
                Kernel->write<float>(outlineComponent + 0x344, globals.aura_survivor_color[0]);
                Kernel->write<float>(outlineComponent + 0x348, globals.aura_survivor_color[1]);
                Kernel->write<float>(outlineComponent + 0x34C, globals.aura_survivor_color[2]);
                Kernel->write<float>(outlineComponent + 0x350, globals.aura_survivor_color[3]);

                Kernel->write<float>(outlineComponent + 0x330, globals.aura_survivor_color[0]);
                Kernel->write<float>(outlineComponent + 0x334, globals.aura_survivor_color[1]);
                Kernel->write<float>(outlineComponent + 0x338, globals.aura_survivor_color[2]);
                Kernel->write<float>(outlineComponent + 0x33C, globals.aura_survivor_color[3]);
            }
        }
        if (globals.aurakiller) {
            for (const auto& entity : entityList) {
                if (!kernel::is_valid(entity.instance)) continue;
                std::string actorName = GetNameById(entity.objectId);
                if (actorName.find("BP_Slasher") == std::string::npos) continue;

                uintptr_t outlineComponent = Kernel->read<uintptr_t>(entity.instance + offsets::KillerDBDOutline);
                if (!kernel::is_valid(outlineComponent)) continue;

                Kernel->write<float>(outlineComponent + 0x2E0, 0.001f);
                Kernel->write<float>(outlineComponent + 0x344, globals.aura_killer_color[0]);
                Kernel->write<float>(outlineComponent + 0x348, globals.aura_killer_color[1]);
                Kernel->write<float>(outlineComponent + 0x34C, globals.aura_killer_color[2]);
                Kernel->write<float>(outlineComponent + 0x350, globals.aura_killer_color[3]);

                Kernel->write<float>(outlineComponent + 0x330, globals.aura_killer_color[0]);
                Kernel->write<float>(outlineComponent + 0x334, globals.aura_killer_color[1]);
                Kernel->write<float>(outlineComponent + 0x338, globals.aura_killer_color[2]);
                Kernel->write<float>(outlineComponent + 0x33C, globals.aura_killer_color[3]);
            }
        }
        if (globals.PalletsAura) {
            for (const auto& entity : entityList) {
                if (!kernel::is_valid(entity.instance)) continue;
                std::string actorName = GetNameById(entity.objectId);
                if (actorName.find("BP_Pallet") == std::string::npos) continue;

                uintptr_t outlineComponent = Kernel->read<uintptr_t>(entity.instance + offsets::PalletDBDOutline);
                if (!kernel::is_valid(outlineComponent)) continue;

                Kernel->write<float>(outlineComponent + 0x2E0, 0.001f);
                Kernel->write<float>(outlineComponent + 0x344, globals.aura_pallets_color[0]);
                Kernel->write<float>(outlineComponent + 0x348, globals.aura_pallets_color[1]);
                Kernel->write<float>(outlineComponent + 0x34C, globals.aura_pallets_color[2]);
                Kernel->write<float>(outlineComponent + 0x350, globals.aura_pallets_color[3]);

                Kernel->write<float>(outlineComponent + 0x330, globals.aura_pallets_color[0]);
                Kernel->write<float>(outlineComponent + 0x334, globals.aura_pallets_color[1]);
                Kernel->write<float>(outlineComponent + 0x338, globals.aura_pallets_color[2]);
                Kernel->write<float>(outlineComponent + 0x33C, globals.aura_pallets_color[3]);
            }
        }
        if (globals.WindowsAura) {
            for (const auto& entity : entityList) {
                if (!kernel::is_valid(entity.instance)) continue;
                std::string actorName = GetNameById(entity.objectId);
                if (actorName.find("WindowStandard") == std::string::npos) continue;

                uintptr_t outlineComponent = Kernel->read<uintptr_t>(entity.instance + offsets::WindowDBDOutline);
                if (!kernel::is_valid(outlineComponent)) continue;

                Kernel->write<float>(outlineComponent + 0x2E0, 0.001f);
                Kernel->write<float>(outlineComponent + 0x344, globals.aura_windows_color[0]);
                Kernel->write<float>(outlineComponent + 0x348, globals.aura_windows_color[1]);
                Kernel->write<float>(outlineComponent + 0x34C, globals.aura_windows_color[2]);
                Kernel->write<float>(outlineComponent + 0x350, globals.aura_windows_color[3]);

                Kernel->write<float>(outlineComponent + 0x330, globals.aura_windows_color[0]);
                Kernel->write<float>(outlineComponent + 0x334, globals.aura_windows_color[1]);
                Kernel->write<float>(outlineComponent + 0x338, globals.aura_windows_color[2]);
                Kernel->write<float>(outlineComponent + 0x33C, globals.aura_windows_color[3]);
            }
        }
        if (globals.MeatHookAura) {
                for (const auto& entity : entityList) {
                if (!kernel::is_valid(entity.instance)) continue;
                std::string actorName = GetNameById(entity.objectId);
                if (actorName.find("BP_MeatHook") == std::string::npos) continue;

                uintptr_t outlineComponent = Kernel->read<uintptr_t>(entity.instance + offsets::MeatHookDBDOutline);
                if (!kernel::is_valid(outlineComponent)) continue;

                Kernel->write<float>(outlineComponent + 0x2E0, 0.001f);
                Kernel->write<float>(outlineComponent + 0x344, globals.aura_hooks_color[0]);
                Kernel->write<float>(outlineComponent + 0x348, globals.aura_hooks_color[1]);
                Kernel->write<float>(outlineComponent + 0x34C, globals.aura_hooks_color[2]);
                Kernel->write<float>(outlineComponent + 0x350, globals.aura_hooks_color[3]);

                Kernel->write<float>(outlineComponent + 0x330, globals.aura_hooks_color[0]);
                Kernel->write<float>(outlineComponent + 0x334, globals.aura_hooks_color[1]);
                Kernel->write<float>(outlineComponent + 0x338, globals.aura_hooks_color[2]);
                Kernel->write<float>(outlineComponent + 0x33C, globals.aura_hooks_color[3]);
            }
        }
        if (globals.GeneratorAura) {
            for (const auto& entity : entityList) {
                if (!kernel::is_valid(entity.instance)) continue;
                std::string actorName = GetNameById(entity.objectId);
                if (actorName.find("GeneratorStandard") == std::string::npos &&
                    actorName.find("GeneratorNoPole") == std::string::npos) continue;

                uintptr_t outlineComponent = Kernel->read<uintptr_t>(entity.instance + offsets::GeneratorDBDOutline);
                if (!kernel::is_valid(outlineComponent)) continue;

                Kernel->write<float>(outlineComponent + 0x2E0, 0.001f);
                Kernel->write<float>(outlineComponent + 0x344, globals.aura_generator_color[0]);
                Kernel->write<float>(outlineComponent + 0x348, globals.aura_generator_color[1]);
                Kernel->write<float>(outlineComponent + 0x34C, globals.aura_generator_color[2]);
                Kernel->write<float>(outlineComponent + 0x350, globals.aura_generator_color[3]);

                Kernel->write<float>(outlineComponent + 0x330, globals.aura_generator_color[0]);
                Kernel->write<float>(outlineComponent + 0x334, globals.aura_generator_color[1]);
                Kernel->write<float>(outlineComponent + 0x338, globals.aura_generator_color[2]);
                Kernel->write<float>(outlineComponent + 0x33C, globals.aura_generator_color[3]);
            }
        }
    }

    inline void ResetAura() {
        for (const auto& entity : entityList) {
            if (!kernel::is_valid(entity.instance)) continue;

            std::string actorName = GetNameById(entity.objectId);
            uintptr_t outlineComponent = 0;

            if (actorName.find("BP_Camper") != std::string::npos) {
                outlineComponent = Kernel->read<uintptr_t>(entity.instance + offsets::SurvivorDBDOutline);
            }
            else if (actorName.find("BP_Slasher") != std::string::npos) {
                outlineComponent = Kernel->read<uintptr_t>(entity.instance + offsets::KillerDBDOutline);
            }
            else if (actorName.find("BP_Pallet") != std::string::npos) {
                outlineComponent = Kernel->read<uintptr_t>(entity.instance + offsets::PalletDBDOutline);
            }
            else if (actorName.find("WindowStandard") != std::string::npos) {
                outlineComponent = Kernel->read<uintptr_t>(entity.instance + offsets::WindowDBDOutline);
            }
            else if (actorName.find("BP_MeatHook") != std::string::npos) {
                outlineComponent = Kernel->read<uintptr_t>(entity.instance + offsets::MeatHookDBDOutline);
            }
            else if (actorName.find("GeneratorStandard") != std::string::npos ||
                actorName.find("GeneratorNoPole") != std::string::npos) {
                outlineComponent = Kernel->read<uintptr_t>(entity.instance + offsets::GeneratorDBDOutline);
            }

            if (!kernel::is_valid(outlineComponent)) continue;

            Kernel->write<float>(outlineComponent + 0x2E0, 1.00f);
            Kernel->write<float>(outlineComponent + 0x350, 1.0f);

            Kernel->write<float>(outlineComponent + 0x330, 0.0f);
            Kernel->write<float>(outlineComponent + 0x334, 0.8f);
            Kernel->write<float>(outlineComponent + 0x338, 1.0f);
            Kernel->write<float>(outlineComponent + 0x33C, 1.0f);
        }
    }


    inline void AutoNiggaSuperSkillCheck() {
        if (!globals.skillcheckauto || !kernel::is_valid(Localpawn)) {
            wasSkillCheckActive = false;
            return;
        }
        try {
            uintptr_t interactionHandler = Kernel->read<uintptr_t>(Localpawn + offsets::InteractionHandler);
            if (!kernel::is_valid(interactionHandler)) {
                wasSkillCheckActive = false;
                return;
            }

            uintptr_t skillCheck = Kernel->read<uintptr_t>(interactionHandler + offsets::SkillCheck);
            if (!kernel::is_valid(skillCheck)) {
                wasSkillCheckActive = false;
                return;
            }
            bool isDisplayed = Kernel->read<bool>(skillCheck + offsets::SkillCheck_IsDisplayed);
            float currentProgress = Kernel->read<float>(skillCheck + offsets::SkillCheck_CurrentProgress);

            if (!isDisplayed) {
                wasSkillCheckActive = false;
                return;
            }

            FSkillCheckDefinition definition = Kernel->read<FSkillCheckDefinition>(skillCheck + offsets::SkillCheck_Definition);
            ULONGLONG currentTime = GetTickCount64();
            if (!wasSkillCheckActive) {
                wasSkillCheckActive = true;
                lastSkillCheckTime = currentTime;
                return;
            }
            if (currentTime - lastSkillCheckTime < 50) {
                return;
            }
            float successZoneMiddle = (definition.SuccessZoneStart + definition.SuccessZoneEnd) / 2.0f;
            if (currentProgress >= definition.SuccessZoneStart && currentProgress <= definition.SuccessZoneEnd) {
                INPUT input = { 0 };
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = VK_SPACE;
                input.ki.dwFlags = 0;
                SendInput(1, &input, sizeof(INPUT));
                input.ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(1, &input, sizeof(INPUT));
                wasSkillCheckActive = false;
            }

        }
        catch (...) {
            wasSkillCheckActive = false;
        }
    }

    inline void ProcessFOVChanger() {
        static bool wasEnabled = false;

        if (!globals.fov_enabled) {
            if (wasEnabled && globals.camera_manager_ptr != 0) {
                try {
                    if (globals.original_fov > 0.0f) {
                        Kernel->write<float>(globals.camera_manager_ptr + 0x2BC, globals.original_fov);
                        Kernel->write<float>(globals.camera_manager_ptr + 0x24D8, globals.original_fov);
                        Kernel->write<float>(globals.camera_manager_ptr + 0x24DC, globals.original_fov);
                        Kernel->write<float>(globals.camera_manager_ptr + 0x2C0, globals.original_fov);
                    }
                    globals.camera_manager_ptr = 0;
                    globals.custom_fov = globals.original_fov;
                }
                catch (...) {}
            }
            wasEnabled = false;
            return;
        }

        wasEnabled = true;

        static ULONGLONG lastFOVUpdate = 0;
        ULONGLONG currentTime = GetTickCount64();
        if (currentTime - lastFOVUpdate < 50) return;
        lastFOVUpdate = currentTime;

        try {
            if (!kernel::is_valid(cameraManager)) return;

            if (globals.camera_manager_ptr == 0) {
                globals.camera_manager_ptr = cameraManager;
                if (globals.fov_first_read && globals.save_original_fov) {
                    try {
                        float originalFOV = Kernel->read<float>(cameraManager + 0x24d8);
                        if (originalFOV < 30.0f || originalFOV > 180.0f) {
                            originalFOV = Kernel->read<float>(cameraManager + 0x2bc);
                        }
                        if (originalFOV >= 30.0f && originalFOV <= 180.0f) {
                            globals.original_fov = originalFOV;
                            globals.fov_first_read = false;
                            if (globals.custom_fov == 87.0f) {
                                globals.custom_fov = globals.original_fov;
                            }
                        }
                    }
                    catch (...) {}
                }
            }
            float targetFOV = std::max(globals.min_fov, std::min(globals.max_fov, globals.custom_fov));

            try {
                Kernel->write<float>(cameraManager + 0x2bc, targetFOV);
                Kernel->write<float>(cameraManager + 0x24d8, targetFOV);
                Kernel->write<float>(cameraManager + 0x24dc, targetFOV);
                Kernel->write<float>(cameraManager + 0x2c0, targetFOV);

                try {
                    Kernel->write<float>(cameraManager + 0x30c, targetFOV);
                }
                catch (...) {}
            }
            catch (...) {}
        }
        catch (...) {}
    }

    inline void ProcessJump() {
        static float originalJumpZVelocity = 420.0f;
        static bool jumpSaved = false;

        if (!kernel::is_valid(Localpawn)) return;

        try {
            uintptr_t movementComponent = Kernel->read<uintptr_t>(Localpawn + offsets::CharacterMovementComponent);
            if (!kernel::is_valid(movementComponent)) return;

            if (!jumpSaved) {
                originalJumpZVelocity = Kernel->read<float>(movementComponent + offsets::JumpZVelocity);
                jumpSaved = true;
            }

            if (globals.jump) {
                Kernel->write<float>(movementComponent + offsets::JumpZVelocity, 600.0f);

                if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
                    Kernel->write<uint8_t>(Localpawn + offsets::bPressedJump, 4u);
                }
                else {
                    Kernel->write<uint8_t>(Localpawn + offsets::bPressedJump, 0u);
                }
            }
            else {
                Kernel->write<float>(movementComponent + offsets::JumpZVelocity, originalJumpZVelocity);
                Kernel->write<uint8_t>(Localpawn + offsets::bPressedJump, 0u);
            }
        }
        catch (...) {
        }
    }

    inline void ProcessMicroTeleport() {
        if (!globals.speed || !kernel::is_valid(Localpawn)) {
            return;
        }
    }

    inline void ProcessActors() {
        std::vector<EntityList> tmpList;
        g_KillerName = "";

        try {
            uWorld = Kernel->read<uintptr_t>(Kernel->g_process_base + offsets::GWorld);
            if (!kernel::is_valid(uWorld)) return;

            gameInstance = Kernel->read<uintptr_t>(uWorld + offsets::OwningGameInstance);
            persistentLevel = Kernel->read<uintptr_t>(uWorld + offsets::PersistentLevel);
            localPlayerPtr = Kernel->read<uintptr_t>(gameInstance + offsets::LocalPlayers);
            localPlayer = Kernel->read<uintptr_t>(localPlayerPtr);
            playerController = Kernel->read<uintptr_t>(localPlayer + offsets::PlayerController);

            if (!kernel::is_valid(playerController)) return;

            PlayerStateLocalPlayer = Kernel->read<uintptr_t>(playerController + offsets::PlayerStateLocalPlayer);
            Localpawn = Kernel->read<uintptr_t>(playerController + offsets::AcknowledgedPawn);
            cameraManager = Kernel->read<uintptr_t>(playerController + offsets::PlayerCameraManager);

            if (!kernel::is_valid(cameraManager)) return;

            originalCameraCache = Kernel->read<FCameraCacheEntry>(cameraManager + offsets::CameraCachePrivate);
            cameraCache = originalCameraCache;

            actorsArray = Kernel->read<uintptr_t>(persistentLevel + offsets::ActorArray);
            actorsCount = Kernel->read<int>(persistentLevel + offsets::ActorCount);

            if (!kernel::is_valid(actorsArray) || actorsCount <= 0 || actorsCount > 5000) return;

            for (int i = 0; i < actorsCount && i < 3000; i++) {
                uintptr_t actor = Kernel->read<uintptr_t>(actorsArray + i * 0x8);
                if (!kernel::is_valid(actor)) continue;

                int objectId = Kernel->read<int>(actor + offsets::ActorID);
                std::string actorName = GetNameById(objectId);

                bool isSurvivor = (actorName.find("BP_Camper") != std::string::npos);
                bool isKiller = (actorName.find("BP_Slasher") != std::string::npos);
                bool isPallet = (actorName.find("BP_Pallet") != std::string::npos);
                bool isWindow = (actorName.find("WindowStandard") != std::string::npos);
                bool isHook = (actorName.find("BP_MeatHook") != std::string::npos);
                bool isGenerator = (actorName.find("GeneratorStandard") != std::string::npos || actorName.find("GeneratorNoPole") != std::string::npos);


                if (!isSurvivor && !isKiller && !isPallet && !isWindow && !isHook && !isGenerator) continue;

                uintptr_t rootComponent = Kernel->read<uintptr_t>(actor + offsets::RootComponent);
                if (!kernel::is_valid(rootComponent)) continue;

                if (!isPallet && !isWindow && !isHook && !isGenerator) {
                    uintptr_t playerState = Kernel->read<uintptr_t>(actor + offsets::PlayerState);
                    uintptr_t pawn = Kernel->read<uintptr_t>(actor + offsets::AcknowledgedPawn);

                    if (IsLocalPlayer(actor, playerState, pawn)) continue;
                }

                if (!isPallet && !isWindow && !isHook && !isGenerator) {
                    try {
                        uintptr_t mesh = Kernel->read<uintptr_t>(actor + offsets::CharacterMesh);
                        if (!kernel::is_valid(mesh)) continue;
                    }
                    catch (...) {
                        continue;
                    }
                }
                if (isPallet) {
                    try {
                        uint8_t palletState = Kernel->read<uint8_t>(actor + 0x3E0);
                        if (palletState == static_cast<uint8_t>(EPalletState::Destroyed)) {
                            continue;
                        }
                    }
                    catch (...) {
                        continue;
                    }
                }

                Vector3 rawOrigin = Kernel->read<Vector3>(rootComponent + offsets::RelativeLocation);
                if (!rawOrigin.IsValid() || !IsValidWorldPosition(rawOrigin)) continue;

                Vector3 origin = GetSmoothedPosition(actor, rawOrigin);
                if (!IsValidWorldPosition(origin)) continue;

                float DistM = (float)(ToMeters(VectorDistance(originalCameraCache.POV.Location, origin)) - 6);

                auto& cachedDist = distanceCache[actor];
                cachedDist = cachedDist * 0.95f + DistM * 0.05f;
                DistM = cachedDist;

                if (DistM < 0.0f || DistM > 1000.0f) continue;

                bool shouldShow = false;
                if (isSurvivor && (globals.show_survivors || globals.aura) && DistM <= (globals.esp_distance / 100.0f)) shouldShow = true;
                if (isKiller && (globals.show_killers || globals.aurakiller)) shouldShow = true;
                if (isPallet && globals.PalletsAura) shouldShow = true;
                if (isWindow && globals.WindowsAura) shouldShow = true;
                if (isHook && globals.MeatHookAura) shouldShow = true;
                if (isGenerator && globals.GeneratorAura) shouldShow = true;

                if (!shouldShow) continue;

                if (isKiller) {
                    g_KillerName = GetKillerNameFromActor(actorName);
                }

                EntityList entity{};
                entity.instance = actor;
                entity.objectId = objectId;
                entity.root_component = rootComponent;
                entity.origin = origin;
                entity.actorName = actorName;

                float baseHeight = 80;
                if (isKiller || isSurvivor) baseHeight = 125;

                if (!isPallet && !isWindow && !isHook && !isGenerator) {
                    try {
                        uintptr_t movementComponent = Kernel->read<uintptr_t>(actor + offsets::CharacterMovementComponent);
                        if (kernel::is_valid(movementComponent)) {
                            uint8_t crouchFlags = Kernel->read<uint8_t>(movementComponent + 0x565);
                            bool isCrouching = (crouchFlags & (1 << 5)) != 0;

                            if (isCrouching) {
                                baseHeight *= 0.6f;
                            }
                            try {
                                float crouchedHalfHeight = Kernel->read<float>(movementComponent + 0x2D0);
                                if (crouchedHalfHeight > 0.0f && crouchedHalfHeight < 200.0f && isCrouching) {
                                    baseHeight = crouchedHalfHeight * 2.0f;
                                }
                            }
                            catch (...) {}
                        }
                    }
                    catch (...) {}
                }

                entity.TopLocation = Vector3(origin.x, origin.y, origin.z + baseHeight);
                entity.dist = DistM;

                if (isSurvivor) {
                    std::string survivorName = GetSurvivorName(actorName);
                    if (survivorName != "Unknown Survivor" && globals.show_survivor_names) {
                        entity.name = survivorName;
                    }
                    else {
                        entity.name = "Survivor";
                    }
                }
                else if (isKiller) entity.name = "Killer";
                else if (isPallet) entity.name = "Pallet";
                else if (isWindow) entity.name = "Window";
                else if (isHook) entity.name = "Hook";
                else if (isGenerator) entity.name = "Generator";

                if (!isPallet && !isWindow && !isHook && !isGenerator) {
                    try {
                        entity.mesh = Kernel->read<uintptr_t>(actor + offsets::CharacterMesh);
                    }
                    catch (...) {
                        entity.mesh = 0;
                    }
                }

                tmpList.push_back(entity);
            }
            entityList = tmpList;
        }
        catch (...) { entityList.clear(); }
    }

    inline void Update() {
        static auto lastUpdate = GetTickCount64();
        ULONGLONG currentTime = GetTickCount64();
        if (currentTime - lastUpdate < 16) return;
        lastUpdate = currentTime;

        uWorld = Kernel->read<uintptr_t>(Kernel->g_process_base + offsets::GWorld);
        if (!kernel::is_valid(uWorld)) {
            entityList.clear();
            return;
        }

        if (globals.esp_enabled || globals.aura || globals.aurakiller || globals.PalletsAura ||
            globals.WindowsAura || globals.MeatHookAura || globals.GeneratorAura) {
            ProcessActors();
        }

        static bool lastAuraState = false;
        static bool lastKillerAuraState = false;
        static bool lastPalletsAuraState = false;
        static bool lastWindowsAuraState = false;
        static bool lastHooksAuraState = false;
        static bool lastGeneratorAuraState = false;

        bool currentAuraState = globals.aura || globals.aurakiller || globals.PalletsAura ||
            globals.WindowsAura || globals.MeatHookAura || globals.GeneratorAura;

        if (currentAuraState) {
            ProcessAura();
        }
        else if ((lastAuraState || lastKillerAuraState || lastPalletsAuraState ||
            lastWindowsAuraState || lastHooksAuraState || lastGeneratorAuraState) && !currentAuraState) {
            ResetAura();
        }

        lastAuraState = globals.aura;
        lastKillerAuraState = globals.aurakiller;
        lastPalletsAuraState = globals.PalletsAura;
        lastWindowsAuraState = globals.WindowsAura;
        lastHooksAuraState = globals.MeatHookAura;
        lastGeneratorAuraState = globals.GeneratorAura;

        if (kernel::is_valid(Localpawn)) {
            ProcessMicroTeleport();
            ProcessFOVChanger();
            ProcessJump();
            AutoNiggaSuperSkillCheck();
        }
    }

    inline void ActorLoop(ImDrawList* drawList) {
        Update();
        if (globals.esp_enabled) {
            FCameraCacheEntry correctedCache = GetCorrectedCameraCache();
            ESP::RenderESP(drawList, entityList, g_KillerName, correctedCache);
        }
    }
}