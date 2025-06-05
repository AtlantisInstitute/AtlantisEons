// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "NiagaraComponent.h"
#include "BP_ItemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/WidgetComponent.h"
#include "BP_ItemInterface.h"
#include "BP_ItemInfo.h"
#include "WBP_Main.h"
#include "Particles/ParticleSystemComponent.h"
class UWBP_CharacterInfo;
#include "WBP_GuideText.h"
#include "WBP_InventorySlot.h"
#include "WBP_SwordBloom.h"
#include "CharacterStatsComponent.h"
#include "EquipmentComponent.h"
#include "InventoryComponent.h"

#include "AtlantisEonsCharacter.generated.h"

class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UAnimMontage;
class USoundBase;
class UParticleSystem;
class UMaterialInterface;
class UTexture2D;
class UWBP_Store;

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UAIPerceptionStimuliSourceComponent;
struct FInputActionValue;

class ADamageNumberSystem;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStatsUpdatedSignature);

/**
 * REFACTORING STRATEGY - PRESERVING BLUEPRINT COMPATIBILITY
 * 
 * This character class serves as the parent of BP_Character Blueprint.
 * We're refactoring using component delegation while maintaining Blueprint compatibility:
 * 
 * KEPT IN CHARACTER (Blueprint needs direct access):
 * - All UPROPERTY Blueprint-visible properties (EditAnywhere, BlueprintReadWrite, etc.)
 * - All UFUNCTION Blueprint-callable methods
 * - Core character components (Camera, Equipment meshes, AI perception)
 * - Input actions and animation montages (set in Blueprint)
 * - Base stats and current stats (Blueprint reads/writes these)
 * - UI widget references (Blueprint sets these)
 * 
 * DELEGATED TO COMPONENTS (internal logic only):
 * - Implementation details of inventory management → InventoryComponent
 * - Implementation details of equipment logic → EquipmentComponent  
 * - Implementation details of stats calculations → CharacterStatsComponent
 * 
 * TRANSITION APPROACH:
 * - Keep all Blueprint-visible properties in character class
 * - Delegate implementation to components via getter/setter methods
 * - Components handle internal logic, character provides Blueprint interface
 * - No breaking changes to BP_Character Blueprint functionality
 */

UCLASS(config=Game)
class ATLANTISEONS_API AAtlantisEonsCharacter : public ACharacter, public IGenericTeamAgentInterface
{
    GENERATED_BODY()

public:

    // ========== CORE CHARACTER COMPONENTS (MUST STAY - Blueprint sets these) ==========
    
    /** Camera boom positioning the camera behind the character */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    class USpringArmComponent* CameraBoom;

    /** Follow camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    class UCameraComponent* FollowCamera;

    /** AI Perception component */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
    class UAIPerceptionStimuliSourceComponent* AIPerceptionStimuliSourceComponent;

    /** SwordEffect Niagara component - found by name from Blueprint */
    UPROPERTY()
    class UNiagaraComponent* SwordEffectComponent;

    /** Dragon skeletal mesh components - found by name from Blueprint */
    UPROPERTY()
    class USkeletalMeshComponent* Dragon1Component;

    UPROPERTY()
    class USkeletalMeshComponent* Dragon2Component;

    // ========== COMPONENT REFERENCES (MUST STAY - Components visible to Blueprint) ==========
    
    /** Character stats component - handles health, mana, stats, and gold */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UCharacterStatsComponent* StatsComponent;

    /** Equipment component - handles equipment management and visual updates */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UEquipmentComponent* EquipmentComponent;

    /** Inventory component - handles inventory management and item operations */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UInventoryComponent* InventoryComp;

    // ========== BLUEPRINT-VISIBLE STATS (MUST STAY - Blueprint reads/writes) ==========
    
    UPROPERTY(BlueprintAssignable, Category = "Character|Stats")
    FOnStatsUpdatedSignature OnStatsUpdated;

    /** Base stats - often set in Blueprint and need direct access */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (AllowPrivateAccess = "true"))
    float BaseMovementSpeed = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (AllowPrivateAccess = "true"))
    float BaseHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (AllowPrivateAccess = "true"))
    int32 BaseSTR = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (AllowPrivateAccess = "true"))
    int32 BaseDEX = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (AllowPrivateAccess = "true"))
    int32 BaseINT = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (AllowPrivateAccess = "true"))
    int32 BaseDefence = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (AllowPrivateAccess = "true"))
    int32 BaseDamage = 10;

    /** Current stats - Blueprint needs to read these frequently */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Combat")
    int32 CurrentSTR;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Combat")
    int32 CurrentDEX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Combat")
    int32 CurrentINT;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Combat")
    int32 CurrentDefence;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Combat")
    int32 CurrentDamage;

    // ========== LEGACY COMPATIBILITY PROPERTIES (TRANSITIONAL - Blueprint compatibility) ==========
    
    /** Health properties - kept for Blueprint compatibility during transition */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Health", meta = (AllowPrivateAccess = "true"))
    float CurrentHealth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Health", meta = (AllowPrivateAccess = "true"))
    float MaxHealth;

    /** Gold properties - kept for Blueprint and store system compatibility */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int32 YourGold;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int32 Gold;

    /** MP properties - kept for Blueprint compatibility */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Status")
    int32 CurrentMP;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Status")
    int32 BaseMP;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int32 MaxMP;

    // ========== INVENTORY PROPERTIES (MUST STAY - Blueprint direct access) ==========
    
    /** Inventory arrays - Blueprint needs direct access for drag/drop and UI */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    TArray<UBP_ItemInfo*> InventoryItems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    TArray<UWBP_InventorySlot*> InventorySlotWidgets;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    TArray<UBP_ItemInfo*> EquipmentSlots;

    /** Inventory state - Blueprint checks these */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    bool InventoryToggleLock;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    float InventoryToggleLockDuration;

    FTimerHandle InventoryToggleLockTimer;

    // ========== UI WIDGET REFERENCES (MUST STAY - Blueprint sets these) ==========
    
    UPROPERTY(BlueprintReadWrite, Category = "UI")
    class UWBP_Main* Main;

    UPROPERTY(BlueprintReadWrite, Category = "UI|Circular Bar")
    class UProgressBar* HPBar;

    UPROPERTY(BlueprintReadWrite, Category = "UI|Circular Bar")
    class UProgressBar* MPBar;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<ADamageNumberSystem> DamageNumberSystemClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* PickupSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Circular Bar")
    UMaterialInstanceDynamic* CircularMP;

    // ========== EQUIPMENT MESH COMPONENTS (MUST STAY - Physical game objects) ==========
    
    /** Equipment mesh components - these are actual physical components attached to character */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* Helmet;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* Weapon;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* Shield;

    // ========== BLUEPRINT INTERFACE FUNCTIONS (MUST STAY - Blueprint calls these) ==========
    
    FORCEINLINE UInputMappingContext* GetDefaultMappingContext() { return DefaultMappingContext; }

    UFUNCTION(BlueprintCallable, Category = "Character|Combat")
    void PlayHitReactMontage();

    /** Equipment Functions - Blueprint Implementable Events must stay */
    UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
    void EquipItem(EItemEquipSlot Slot, const TSoftObjectPtr<UStaticMesh>& MeshToEquip, UTexture2D* Thumbnail, 
        int32 ItemIndex, UMaterialInterface* Material1 = nullptr, UMaterialInterface* Material2 = nullptr);

    UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
    void DisarmItem(EItemEquipSlot ItemEquipSlot, const TSoftObjectPtr<UStaticMesh>& StaticMeshID, int32 ItemIndex);

    /** Helper functions - Blueprint needs these */
    UFUNCTION(BlueprintImplementableEvent, Category = "Game Instance")
    UAtlantisEonsGameInstance* GetGameInstanceHelper() const;

    UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
    bool GetItemInfo(int32 ItemIndex, FStructure_ItemInfo& OutItemInfo) const;

    /** Stat manipulation functions - delegated to components but Blueprint interface preserved */
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void AddingCharacterStatus(int32 ItemIndex);

    UFUNCTION(BlueprintCallable, Category = "Character|Stats")
    void SubtractingCharacterStatus(int32 ItemIndex);

protected:
    // ========== INPUT AND ANIMATION PROPERTIES (MUST STAY - Blueprint sets these) ==========

    /** Equipment Slot UI Components - Blueprint needs direct access */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Equipment Slots")
    class UWBP_InventorySlot* HeadSlot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Equipment Slots")
    class UWBP_InventorySlot* WeaponSlot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Equipment Slots")
    class UWBP_InventorySlot* SuitSlot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Equipment Slots")
    class UWBP_InventorySlot* CollectableSlot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Widgets")
    class UProgressBar* CircularBarHP;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Widgets")
    class UProgressBar* CircularBarMP;

    /** Input Actions - Blueprint sets these in Input section */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    class UInputMappingContext* DefaultMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    class UInputAction* MeleeAttackAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* PickupAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* InventoryAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* ResumeAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* DebugDamageAction;

    // ========== COMBAT STATE (MUST STAY - Blueprint reads these) ==========
    
    /** Combat state flags - Blueprint checks these for UI and logic */
    bool bIsInvulnerable = false;
    bool bCanAttack;
    bool bIsAttacking;
    bool bAttackNotifyInProgress = false;
    
    // ========== CAMERA STABILIZATION SYSTEM ==========
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void EnableAttackCameraStabilization();
    
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void DisableAttackCameraStabilization();
    
    UFUNCTION(BlueprintCallable, Category = "Camera")
    bool IsNearEnemies() const;

    // ========== CAMERA STABILIZATION VARIABLES ==========
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    bool bSuppressAttackRootMotion = false;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    FVector LockedPosition = FVector::ZeroVector;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    bool bPositionLocked = false;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    FRotator LockedCameraRotation = FRotator::ZeroRotator;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    bool bCameraRotationLocked = false;
    
    // ULTRA-AGGRESSIVE: Maximum stabilization variables for rock-solid camera control
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CameraStabilizationStrength = 0.98f; // ULTRA-AGGRESSIVE: Near-maximum stabilization strength
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float StabilizationThreshold = 0.5f; // ULTRA-AGGRESSIVE: Much tighter threshold for immediate correction
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.1", ClampMax = "100.0"))
    float StabilizationLerpSpeed = 35.0f; // ULTRA-AGGRESSIVE: Much faster correction speed
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    bool bAllowVerticalMovementDuringAttacks = true; // Allow Z-axis movement during attacks
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    bool bAllowRotationDuringAttacks = false; // Prevent camera rotation during attacks
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CameraRotationStabilizationStrength = 0.99f; // ULTRA-AGGRESSIVE: Maximum rotation stabilization
    
    // BREAKABLE STABILIZATION: Allow player to override stabilization with movement input
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    bool bAllowBreakableStabilization = true; // Enable input-based stabilization override
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MovementInputStabilizationStrength = 0.0f; // DISABLED - No stabilization when player is actively moving
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float MovementInputThreshold = 0.1f; // Minimum input magnitude to trigger breakable mode
    
    // ULTRA-AGGRESSIVE: Extended sequence stabilization to prevent cumulative drift
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float PositionRefreshInterval = 1.0f; // ULTRA-AGGRESSIVE: Refresh locked position every 1 second
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float MaxCumulativeDrift = 0.8f; // ULTRA-AGGRESSIVE: Much tighter drift tolerance
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ExtendedSequenceStabilizationStrength = 0.99f; // ULTRA-AGGRESSIVE: Maximum stabilization for extended sequences
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PositionTolerance = 0.05f; // ULTRA-AGGRESSIVE: Extremely tight tolerance to prevent any micro-drift
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float RotationTolerance = 0.01f; // ULTRA-AGGRESSIVE: Virtually zero rotation tolerance for rock-solid control
    
    // Attack-specific stabilization timing
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float StabilizationStartDelay = 0.0f; // No delay - instant activation to prevent any frame distortions
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float StabilizationEndEarly = 0.2f; // End stabilization before attack animation ends
    
    // Simplified parameters (removed ultra-aggressive modes)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float MaxStabilizationDistance = 3.0f; // Reasonable distance before correction (increased from 1.0)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    bool bUseHardSnapForLargeMovements = false; // Disabled to prevent frame snaps
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    bool bUseUltraStabilizationMode = false; // Disabled to prevent aggressive corrections
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float UltraStabilizationThreshold = 0.5f; // More reasonable threshold (increased from 0.05)
    
    // Proximity-based stabilization for enemy interactions
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    bool bUseProximityStabilization = false; // Disabled - was too restrictive for normal movement
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "50.0", ClampMax = "500.0"))
    float ProximityStabilizationDistance = 200.0f; // Distance to enemies that triggers stabilization
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ProximityStabilizationStrength = 0.7f; // Weaker than attack stabilization but still effective
    
    FTimerHandle StabilizationDelayTimer;
    FTimerHandle PositionRefreshTimer; // ENHANCED: Timer for refreshing locked position during extended sequences
    
    // Smoothing variables for better interpolation
    FVector LastStabilizedPosition = FVector::ZeroVector;
    bool bStabilizationActive = false;
    
    // ENHANCED: Extended sequence tracking variables
    float StabilizationStartTime = 0.0f; // Track how long stabilization has been active
    FVector InitialLockedPosition = FVector::ZeroVector; // Store original position for drift detection
    FRotator InitialLockedCameraRotation = FRotator::ZeroRotator; // Store original rotation for drift detection
    
    // ULTRA-AGGRESSIVE: Persistent stabilization variables
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float MinimumStabilizationDuration = 3.0f; // ULTRA-AGGRESSIVE: Minimum time before stabilization can be disabled
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    bool bPersistentStabilizationMode = true; // ULTRA-AGGRESSIVE: Keep stabilization active longer during combat
    
    // COMBAT MODE: Extended stabilization that persists across multiple attack sequences
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "30.0"))
    float CombatModeStabilizationDuration = 15.0f; // Stay locked for 15 seconds of combat activity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    bool bCombatModeStabilization = true; // Enable extended combat stabilization mode
    
    float LastCombatActivity = 0.0f; // Track when combat last occurred
    bool bInCombatMode = false; // Are we in extended combat stabilization mode?
    
    // BREAKABLE STABILIZATION: Track current movement input
    FVector2D CurrentMovementInput = FVector2D::ZeroVector; // Current movement input from player
    bool bIsPlayerTryingToMove = false; // Is player actively providing movement input?

    // Timer handles for combat and effects
    FTimerHandle AttackCooldownTimer;
    FTimerHandle AttackNotifyTimer;
    FTimerHandle DashCooldownTimer;
    FTimerHandle InvulnerabilityTimer;
    FTimerHandle DashTimer;
    FTimerHandle CameraLagTimer;
    FTimerHandle CameraRotationLagTimer;
    FTimerHandle RespawnTimerHandle;
    FTimerHandle ComboResetTimer;

    // ========== ANIMATION MONTAGES (MUST STAY - Blueprint sets these) ==========
    
    /** Animation montages - set in Blueprint Animation section */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* MeleeAttackMontage1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* MeleeAttackMontage2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* MeleeAttackMontage3;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* MeleeAttackMontage4;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* MeleeAttackMontage5;

    /** Current attack combo index (0 = MeleeAttackMontage1, 1 = MeleeAttackMontage2, etc.) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Animation", meta = (AllowPrivateAccess = "true"))
    int32 CurrentAttackIndex;

    /** Maximum number of combo attacks (corresponds to number of available montages) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Animation", meta = (AllowPrivateAccess = "true"))
    int32 MaxComboAttacks = 4;

    /** Whether we're currently in a combo chain */
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    bool bIsInCombo = false;

    /** Whether we hit the critical window in the current attack */
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    bool bHitCriticalWindow = false;

    /** Combo window duration (time allowed between attacks) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    float ComboWindowDuration = 2.0f;

    /** Hit reaction animation montage */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* HitReactMontage;

    // ========== VFX AND MATERIALS (MUST STAY - Blueprint sets these) ==========
    
    /** Invulnerability effect */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|VFX", meta = (AllowPrivateAccess = "true"))
    UParticleSystem* InvulnerabilityEffect;

    /** Material for invulnerability effect */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|VFX", meta = (AllowPrivateAccess = "true"))
    UMaterialInterface* InvulnerabilityMaterial;

    // ========== CAMERA AND MOVEMENT SETTINGS (MUST STAY - Blueprint tweaks these) ==========
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Camera", meta = (AllowPrivateAccess = "true"))
    float DefaultCameraLag = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Camera", meta = (AllowPrivateAccess = "true"))
    float DefaultCameraRotationLag = 0.0f;
    
    /** Mouse/camera sensitivity multipliers */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Camera", meta = (AllowPrivateAccess = "true", ClampMin = "0.01", ClampMax = "2.0", UIMin = "0.01", UIMax = "2.0"))
    float CameraYawSensitivity = 0.7f;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Camera", meta = (AllowPrivateAccess = "true", ClampMin = "0.01", ClampMax = "2.0", UIMin = "0.01", UIMax = "2.0"))
    float CameraPitchSensitivity = 0.7f;

    /** Death animation montage */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* DeathMontage;

    /** Respawn delay in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Death", meta = (AllowPrivateAccess = "true"))
    float RespawnDelay = 3.0f;

    // ========== INVENTORY UI STATE (MUST STAY - Blueprint checks these) ==========
    
    /** Whether the inventory is currently open */
    UPROPERTY(BlueprintReadOnly, Category = "UI|Inventory")
    bool bIsInventoryOpen;
    
    /** Whether the inventory toggle is locked (to prevent conflicts) */
    UPROPERTY(BlueprintReadOnly, Category = "UI|Inventory")
    bool bInventoryToggleLocked;
    
    /** The inventory widget class */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Inventory", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UUserWidget> InventoryWidgetClass;
    
    /** The created inventory widget instance */
    UPROPERTY(BlueprintReadOnly, Category = "UI|Inventory", meta = (AllowPrivateAccess = "true"))
    UUserWidget* InventoryWidget;

    // ========== COMBAT CONFIGURATION (MUST STAY - Blueprint sets these) ==========
    
    /** Amount of damage dealt by attacks */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float DamageAmount;

    /** Radius of the damage sphere */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float DamageSphereRadius;

    /** Offset of the damage sphere from character */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float DamageSphereOffset;

    /** Range of attack */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float AttackRange;

    UPROPERTY(BlueprintReadWrite, Category = "Inventory")
    bool bInventoryNeedsUpdate;

    virtual void PostInitProperties() override;

    /** Unlocks the inventory toggle after a delay */
    void UnlockInventoryToggle();

private:
    bool bHealthDelegateBound = false;

    /** Stored original materials for invulnerability effect */
    TArray<UMaterialInterface*> OriginalMaterials;

    /** Active particle system components */
    UPROPERTY()
    UParticleSystemComponent* InvulnerabilityPSC;

    float AttackCooldown = 0.5f;

    /** Called when the game starts or when spawned */
    virtual void BeginPlay() override;

    UPROPERTY()
    class UWBP_Main* MainWidget;

    UPROPERTY()
    class UWBP_GuideText* GuideTextWidget;

    UPROPERTY()
    class UMaterialInstanceDynamic* CircularHPMaterial;

    UPROPERTY()
    class UMaterialInstanceDynamic* CircularMPMaterial;

    // UI initialization functions
    void InitializeUI();
    void SetupCircularBars();

    UPROPERTY()
    FGenericTeamId TeamId;

    UPROPERTY()
    FTimerHandle DelayedCaptureTimer;

    UPROPERTY()
    FTimerHandle InventoryUpdateTimer;

    UPROPERTY(VisibleAnywhere, Category = "Stats")
    bool bIsDead = false;

    /** Helper function to sync legacy properties with component values */
    void SyncLegacyStatsFromComponent();

    // ========== ENHANCED DAMAGE SYSTEM EVENT HANDLERS ==========
    
    /** Event handler for damage taken from StatsComponent */
    UFUNCTION()
    void OnDamageTakenEvent(float IncomingDamage, bool bIsAlive);

    /** Event handler for character death from StatsComponent */
    UFUNCTION()
    void OnCharacterDeathEvent();

    /** Event handler for health changes from StatsComponent */
    UFUNCTION()
    void OnHealthChangedEvent(float NewHealthPercent);

    /** Event handler for inventory changes from InventoryComponent */
    UFUNCTION()
    void OnInventoryChangedEvent();

public:
    UFUNCTION(BlueprintCallable, Category = "Character|Input")
    UInputMappingContext* GetDefaultMappingContext() const { return DefaultMappingContext; }

    // Input handling
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // Damage handling
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

    // Item pickup functions
    void OnPickupPressed();
    AActor* FindClosestItem() const;
    void PickupItem(int32 ItemIndex, int32 StackNumber);

    /** Called for movement input */
    void Move(const FInputActionValue& Value);

    /** Called for looking input */
    void Look(const FInputActionValue& Value);

    /** Called when controller changes */
    virtual void NotifyControllerChanged() override;

    /** Called for jumping */
    virtual void Jump() override;

    /** Called for stopping jump */
    virtual void StopJumping() override;

    /** Called for melee attack input */
    void MeleeAttack(const FInputActionValue& Value);
    
    // ========== CAMERA STABILIZATION - ROOT MOTION CONTROL ==========
    
    /** Override to handle root motion extraction and filtering during attacks */
    virtual void Tick(float DeltaSeconds) override;

    /** Get the current attack montage based on combo index */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    UAnimMontage* GetCurrentAttackMontage() const;

    /** Reset the combo chain back to the first attack */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ResetComboChain();

    /** Advance to the next attack in the combo chain */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void AdvanceComboChain();

    /** Check if we can continue the combo (critical window was hit) */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool CanContinueCombo() const;

    /** Start the combo chain timing window */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StartComboWindow();

    /** End the combo chain (called when window expires) */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void EndComboChain();

    /** Called when invulnerability period expires */
    UFUNCTION()
    void ResetInvulnerability();

    /** Called when attack cooldown expires */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ResetAttack();

    /** Apply visual effects for invulnerability */
    void ApplyInvulnerabilityEffects();

    /** Remove visual effects for invulnerability */
    void RemoveInvulnerabilityEffects();

    /** Store the original materials of the character mesh */
    void StoreOriginalMaterials();

    /** Apply damage to the character */
    UFUNCTION(BlueprintCallable, Category = "Character|Health")
    void ApplyDamage(float InDamageAmount);

    /** Apply damage to the character */
    UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
    void ApplyCharacterDamage(float Amount);

    /** Handle character death */
    UFUNCTION(BlueprintCallable, Category = "Character")
    void HandleDeath();

    UFUNCTION(BlueprintCallable, Category = "Character")
    void ResetCharacter();

    /** Toggle the inventory open/closed */
    UFUNCTION()
    void ToggleInventory(const FInputActionValue& Value);

    /** Open the inventory */
    UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
    void OpenInventory();

    /** Close the inventory */
    UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
    void CloseInventory();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void UpdateInventorySlots();

    UFUNCTION()
    void DelayedUpdateInventorySlots();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void ForceUpdateInventorySlotsAfterDelay();

    AAtlantisEonsCharacter();
    virtual ~AAtlantisEonsCharacter() = default;

    virtual void PostInitializeComponents() override;

    UFUNCTION(BlueprintCallable, Category = "Character")
    void RespawnCharacter();

    UFUNCTION(BlueprintImplementableEvent, Category = "Debug")
    void DebugDamage(const FInputActionValue& Value);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void FaceNearestEnemy();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void OnMeleeAttackNotify();

    UFUNCTION(BlueprintImplementableEvent, Category = "Debug")
    void ApplyDebugDamage();

    UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
    void ShowDamageNumber(float InDamageAmount, FVector Location, bool bIsCritical = false);

    /** Get current health from stats component */
    UFUNCTION(BlueprintPure, Category = "Health")
    float GetCurrentHealth() const;

    /** Get max health from stats component */
    UFUNCTION(BlueprintPure, Category = "Health")
    float GetMaxHealth() const;

    /** Get current stats from stats component */
    UFUNCTION(BlueprintPure, Category = "Stats")
    int32 GetCurrentSTR() const;

    UFUNCTION(BlueprintPure, Category = "Stats")
    int32 GetCurrentDEX() const;

    UFUNCTION(BlueprintPure, Category = "Stats")
    int32 GetCurrentINT() const;

    UFUNCTION(BlueprintPure, Category = "Stats")
    int32 GetCurrentDefence() const;

    UFUNCTION(BlueprintPure, Category = "Stats")
    int32 GetCurrentDamage() const;

    /** Get gold from stats component */
    UFUNCTION(BlueprintPure, Category = "Currency")
    int32 GetYourGold() const;

    /** Get current MP from stats component */
    UFUNCTION(BlueprintPure, Category = "Stats")
    int32 GetCurrentMP() const;

    /** Get max MP from stats component */
    UFUNCTION(BlueprintPure, Category = "Stats")
    int32 GetMaxMP() const;

    UFUNCTION(BlueprintCallable, Category = "Character|Stats")
    void UpdateAllStats();

    UFUNCTION(BlueprintCallable, Category = "Character|Stats")
    void RefreshStatsDisplay();

    UFUNCTION(BlueprintCallable, Category = "Character|Inventory")
    void SettingStore();

    UFUNCTION(BlueprintCallable, Category = "Character|Inventory")
    void SetInventorySlot(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotWidgetRef);

    UFUNCTION(BlueprintCallable, Category = "Character|Inventory")
    void ContextMenuUse(UWBP_InventorySlot* InventorySlot);

    UFUNCTION(BlueprintCallable, Category = "Character|Inventory")
    void ContextMenuThrow(UWBP_InventorySlot* InventorySlot);

    UFUNCTION(BlueprintCallable, Category = "Character|Inventory")
    void ContextMenuUse_EquipItem(UBP_ItemInfo* ItemInfoRef);

    UFUNCTION(BlueprintCallable, Category = "Character|Inventory")
    void ContextMenuUse_ConsumeItem(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotRef,
        int32 RecoverHP, int32 RecoverMP, EItemType ItemType);

    UFUNCTION(BlueprintCallable, Category = "Character|Equipment")
    void EquipItemInSlot(EItemEquipSlot ItemEquipSlot, const TSoftObjectPtr<UStaticMesh>& StaticMeshID, const TSoftObjectPtr<UTexture2D>& Texture2D, int32 ItemIndex, UMaterialInterface* MaterialInterface, UMaterialInterface* MaterialInterface2);

    UFUNCTION(BlueprintCallable, Category = "Character|Equipment")
    void HandleDisarmItem(EItemEquipSlot ItemEquipSlot, const TSoftObjectPtr<UStaticMesh>& StaticMeshID, int32 ItemIndex);

    UFUNCTION(BlueprintCallable, Category = "Character|Equipment")
    void ProcessEquipItem(UBP_ItemInfo* ItemInfoRef);

    UFUNCTION(BlueprintCallable, Category = "Character|Equipment")
    void ProcessConsumeItem(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotRef,
        int32 RecoverHP, int32 RecoverMP, EItemType ItemType);

    UFUNCTION(BlueprintCallable, Category = "Character|Equipment")
    void UpdateEquipmentSlotUI(EItemEquipSlot EquipSlot, UBP_ItemInfo* ItemInfo);

    UFUNCTION(BlueprintCallable, Category = "Character|Equipment")
    void ClearEquipmentSlotUI(EItemEquipSlot EquipSlot);

    UFUNCTION(BlueprintCallable, Category = "Character|Equipment")
    void UpdateAllEquipmentSlotsUI();

    UFUNCTION(BlueprintCallable, Category = "Character|Equipment")
    void InitializeEquipmentSlotReferences();

    UFUNCTION(BlueprintCallable, Category = "Character|Equipment")
    void OnEquipmentSlotClicked(EItemEquipSlot EquipSlot);

    UFUNCTION()
    void OnHeadSlotClicked(int32 SlotIndex);

    UFUNCTION()
    void OnWeaponSlotClicked(int32 SlotIndex);

    UFUNCTION()
    void OnSuitSlotClicked(int32 SlotIndex);

    UFUNCTION()
    void OnCollectableSlotClicked(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Character|Inventory")
    void DragAndDropExchangeItem(UBP_ItemInfo* FromInventoryItemRef, UWBP_InventorySlot* FromInventorySlotRef,
        UBP_ItemInfo* ToInventoryItemRef, UWBP_InventorySlot* ToInventorySlotRef);

    UFUNCTION(BlueprintCallable, Category = "Character|Stats")
    void RecoverHealth(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Character Status")
    void RecoverHP(int32 RecoverHP);

    UFUNCTION(BlueprintCallable, Category = "Character Status")
    void RecoverMP(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Character|Stats")
    void SettingCircularBar_MP();

    UFUNCTION(BlueprintCallable, Category = "Character|Stats")
    void SettingCircularBar_HP();

    UFUNCTION(BlueprintCallable, Category = "Character|Inventory")
    UBP_ItemInfo* GetInventoryItemRef() const;

    UFUNCTION(BlueprintPure, Category = "Character|Inventory")
    bool IsInventoryOpen() const { return bIsInventoryOpen; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Main")
    class UWBP_CharacterInfo* WBP_CharacterInfo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Store")
    class UWBP_Store* WBP_Store;

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Character")
    class ABP_Item* FindingClosestItem();

    /**
     * Used to prevent inventory from being toggled when main menu is closing
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
    void SetInventoryToggleLock(bool bLock, float UnlockDelay = 0.0f);

    /**
     * Forcibly sets the inventory state without checking current state
     * Used to ensure state consistency in edge cases
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
    void ForceSetInventoryState(bool bNewIsOpen);

    /**
     * Directly closes the inventory if it's currently open
     * Used for ESC key and other direct close actions without toggling
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "UI|Inventory")
    void CloseInventoryIfOpen(const FInputActionValue& Value);

    // IGenericTeamAgentInterface
    virtual FGenericTeamId GetGenericTeamId() const override;
    virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;

    /** Implementation of inventory closing */
    UFUNCTION()
    void CloseInventoryImpl();

    /** Clear all active item info popups when inventory is closed */
    UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
    void ClearAllInventoryPopups();

    /** Returns CameraBoom subobject **/
    FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

    /** Returns FollowCamera subobject **/
    FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

    UFUNCTION(BlueprintCallable, Category = "Character|Movement")
    void ForceEnableMovement();

    /** 
     * Resets and reinitializes character input and movement settings
     * Use this function when resuming from menus to ensure input works correctly
     */
    UFUNCTION(BlueprintCallable, Category = "Character|Input")
    void ResetCharacterInput();

    // Helper function to create hardcoded item data for testing
    FStructure_ItemInfo CreateHardcodedItemData(int32 ItemIndex);

    // Helper function to add an item to the inventory
    bool AddItemToInventory(UBP_ItemInfo* ItemInfo);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetMainWidget(UWBP_Main* NewWidget);

    UFUNCTION(BlueprintCallable, Category = "UI")
    UWBP_Main* GetMainWidget() const { return MainWidget; }

    // Context Menu Event Handlers - these are called by inventory slots via events
    UFUNCTION()
    void OnContextMenuUse(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlot);

    UFUNCTION()
    void OnContextMenuThrow(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlot);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void EquipInventoryItem(UBP_ItemInfo* ItemInfoRef);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void UnequipInventoryItem(UBP_ItemInfo* ItemInfoRef);

    /** Timer handles for sword effects auto-hide */
    FTimerHandle SwordBloomHideTimer;
    FTimerHandle SwordSparkHideTimer;

    /** Bloom window timing properties */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Effects")
    bool bBloomWindowActive = false;
    
    /** Timer handle for the bloom window duration */
    FTimerHandle BloomWindowTimer;
    
    /** Timer handle for bloom circle scaling animation */
    FTimerHandle BloomScaleTimer;
    
    /** Current scale of the bloom circle during animation */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Effects")
    float CurrentBloomScale = 1.0f;
    
    /** Animation progress for bloom scaling (0.0 to 1.0) */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Effects")
    float BloomScaleProgress = 0.0f;

    /** Current attack montage duration for bloom effect timing */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Effects")
    float CurrentMontageLength = 0.0f;

    // Animation Notify Functions - restored to original C++ approach
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void BloomCircleNotify();
    
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void BloomSparkNotify();
    
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void HideBloomEffectsNotify();
    
    // SwordBloom functions - restored to original C++ approach
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool TryTriggerSparkEffect();
    
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StartBloomScaling();
    
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void UpdateBloomScaling();
    
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void TriggerProgrammaticBloomEffect(float MontageLength);
    
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void CreateSwordBloomWidget();
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
    UWBP_SwordBloom* GetSwordBloomWidget();

    // Inventory delegation functions for backward compatibility
    UFUNCTION(BlueprintCallable, Category = "Character|Inventory")
    bool PickingItem(int32 ItemIndex, int32 ItemStackNumber);

    UFUNCTION(BlueprintCallable, Category = "Character|Inventory")
    bool BuyingItem(int32 ItemIndex, int32 ItemStackNumber, int32 ItemPrice);

    UFUNCTION(BlueprintPure, Category = "Character|Inventory")
    UInventoryComponent* GetInventoryComponent() const { return InventoryComp; }

    /** Helper function to connect inventory component to main widget */
    void ConnectInventoryToMainWidget();

    // SwordBloom Widget Component (restored to original approach)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    class UWidgetComponent* SwordBloomWidgetComponent;
    
    UPROPERTY(BlueprintReadWrite, Category = "UI")
    class UWBP_SwordBloom* SwordBloomWidget;

    UFUNCTION(BlueprintPure, Category = "Character|Stats")
    int32 GetGold() const;

    // ========== SWORD EFFECT NIAGARA SYSTEM MANAGEMENT ==========
    
    /** Find and cache the SwordEffect Niagara component from Blueprint */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void FindSwordEffectComponent();
    
    /** Activate SwordEffect Niagara system during combo chains */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void ActivateSwordEffect();
    
    /** Deactivate SwordEffect Niagara system when combo ends */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void DeactivateSwordEffect();
    
    /** Check if SwordEffect should be active based on current attack index */
    UFUNCTION(BlueprintPure, Category = "Combat|Effects")
    bool ShouldSwordEffectBeActive() const;

    // ========== DRAGON SKELETAL MESH VISIBILITY MANAGEMENT ==========
    
    /** Find and cache the Dragon skeletal mesh components from Blueprint */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void FindDragonComponents();
    
    /** Show Dragon skeletal meshes during final attack */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void ShowDragons();
    
    /** Hide Dragon skeletal meshes when final attack ends */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void HideDragons();
    
    /** Check if Dragons should be visible based on current attack index */
    UFUNCTION(BlueprintPure, Category = "Combat|Effects")
    bool ShouldDragonsBeVisible() const;
};
