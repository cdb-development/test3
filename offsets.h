#pragma once
#include <cstdint>

namespace offsets
{
    uintptr_t GWorld = 0xB869650;
    uintptr_t GNames = 0xB5DD400;
    uintptr_t ActorID = 0x18;

    uintptr_t OwningGameInstance = 0x1E8;
    uintptr_t LocalPlayers = 0x40;
    uintptr_t PlayerController = 0x38;
    uintptr_t PlayerControlRotation = 0x320;
    uintptr_t AcknowledgedPawn = 0x350;
    uintptr_t PlayerCameraManager = 0x360;
    uintptr_t CameraCachePrivate = 0x13B0;
    uintptr_t PlayerState = 0x2C0;
    uintptr_t PlayerStateLocalPlayer = 0x2f8;
    uintptr_t RootComponent = 0x1b0;
    uintptr_t DefaultFOV = 0x2bc;
    uintptr_t PawnPrivate = 0x198;
    uintptr_t CurrentFOV = 0x3a0;
    uintptr_t RelativeLocation = 0x148;
    uintptr_t RelativeRotation = 0x160;
    uintptr_t PersistentLevel = 0x38;
    uintptr_t PlayerNamePrivate = 0x340;

    uintptr_t _selectedSurvivorIndex = 0x5D8;
	uintptr_t _selectedKillerIndex = 0x5DC;
    uintptr_t GroupID = 0x38;


    uintptr_t ActorArray = 0xA8;
    uintptr_t ActorCount = 0xB0;
    uintptr_t ComponentToWorld = 0x1F0;
    uintptr_t MinimumOutlineDistance = 0x2F0;
    uintptr_t CharacterMovementComponent = 0x330;
    uintptr_t IsAlwaysVisible = 0x2E8;
    uintptr_t ComponentSpaceTransforms = 0x640;
    uintptr_t BlueprintCreatedComponents = 0x288;

    // Extra 
    uintptr_t MovementMode = 0x241;              // EMovementMode (1 byte)
    uintptr_t GravityScale = 0x1A8;              // float
    uintptr_t MaxFlySpeed = 0x294;               // float  
    uintptr_t bCheatFlying = 0x565;              // bitfield (Bit 4)
    uintptr_t bIgnoreBaseRotation = 0x565;       // bitfield (Bit 7)
    uintptr_t MaxWalkSpeed = 0x288;
    uintptr_t MaxWalkSpeedCrouched = 0x28c;
    uintptr_t bPressedJump = 0x47C;
    uintptr_t MaxCustomMovementSpeed = 0x298;
    uintptr_t JumpZVelocity = 0x1B0;

    // Generator 
    uintptr_t GeneratorState = 0x580; // IsRepared
    uintptr_t GeneratorProgress = 0x69c; // Percent

    // Mesh / Bones 
    uintptr_t CharacterMesh = 0x328;
    uintptr_t SkeletalMesh = 0x5C8; 
    uintptr_t MeshDeformerInstances = 0x610;
    uintptr_t MorphTargets = 0x2B8;
    uintptr_t BoneArray = MorphTargets + 0x18;
    uintptr_t LODData = 0x600;

    // SkillChecks 
    uintptr_t InteractionHandler = 0xC00;        // LocalPawn -> InteractionHandler
    uintptr_t SkillCheck = 0x330;                // InteractionHandler -> SkillCheck
    uintptr_t SkillCheck_IsDisplayed = 0x178;    // SkillCheck -> bool
    uintptr_t SkillCheck_CurrentProgress = 0x17C;// SkillCheck -> float
    uintptr_t SkillCheck_CustomType = 0x1A8;     // SkillCheck -> ESkillCheckCustomType 
    uintptr_t SkillCheck_Definition = 0x1E8;     // SkillCheck -> FSkillCheckDefinition


    namespace outlines {
        uintptr_t MinimumOutlineDistance = 0x02F0;
        uintptr_t SurvivorDBDOutline = 0x1D68;
        uintptr_t KillerDBDOutline = 0x1D10;
        uintptr_t PalletDBDOutline = 0x05E8;
        uintptr_t GeneratorDBDOutline = 0x08C0;
        uintptr_t WindowDBDOutline = 0x04A8;
        uintptr_t MeatHookDBDOutline = 0x07B8;
    }
    uintptr_t SurvivorDBDOutline = outlines::SurvivorDBDOutline;
    uintptr_t KillerDBDOutline = outlines::KillerDBDOutline;
    uintptr_t PalletDBDOutline = outlines::PalletDBDOutline;
    uintptr_t GeneratorDBDOutline = outlines::GeneratorDBDOutline;
    uintptr_t WindowDBDOutline = outlines::WindowDBDOutline;
    uintptr_t MeatHookDBDOutline = outlines::MeatHookDBDOutline;
};


struct FSkillCheckDefinition
{
    float SuccessZoneStart;           // 0x00
    float SuccessZoneEnd;             // 0x04
    float BonusZoneLength;            // 0x08
    float BonusZoneStart;             // 0x0C
    float ProgressRate;               // 0x10
    float StartingTickerPosition;     // 0x14
    bool  IsDeactivatedAfterResponse; // 0x18
    float WarningSoundDelay;          // 0x1C
    bool  IsAudioMuted;               // 0x20
    bool  IsJittering;                // 0x21
    bool  IsOffCenter;                // 0x22
    bool  IsSuccessZoneMirrorred;     // 0x23
    bool  IsInsane;                   // 0x24
    bool  IsLocallyPredicted;         // 0x25
    uint8_t pad[2];                   // 0x26 
};

enum class ESkillCheckCustomType : uint8_t
{
    ESkillCheckCustomType__VE_None = 0,
    ESkillCheckCustomType__VE_OnPickedUp = 1,
    ESkillCheckCustomType__VE_OnAttacked = 2,
    ESkillCheckCustomType__VE_DecisiveStrikeWhileWiggling = 3,
    ESkillCheckCustomType__VE_GeneratorOvercharge1 = 4,
    ESkillCheckCustomType__VE_GeneratorOvercharge2 = 5,
    ESkillCheckCustomType__VE_GeneratorOvercharge3 = 6,
    ESkillCheckCustomType__VE_BrandNewPart = 7,
    ESkillCheckCustomType__VE_Struggle = 8,
    ESkillCheckCustomType__VE_OppressionPerkGeneratorKicked = 9,
    ESkillCheckCustomType__VE_SoulChemical = 10,
    ESkillCheckCustomType__VE_Wiggle = 11,
    ESkillCheckCustomType__VE_YellowGlyph = 12,
    ESkillCheckCustomType__VE_K27P03Continuous = 13,
    ESkillCheckCustomType__VE_Continuous = 14,
    ESkillCheckCustomType__VE_S42P02 = 15,
    ESkillCheckCustomType__VE_K38P03Continuous = 16,
    ESkillCheckCustomType__VE_SnapOutOfIt = 17,
    ESkillCheckCustomType__ESkillCheckCustomType_MAX = 18
};