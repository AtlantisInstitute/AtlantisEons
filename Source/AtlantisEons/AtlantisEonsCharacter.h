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
#include "CharacterUIManager.h"
#include "CameraStabilizationComponent.h"
#include "EquipmentVisualsComponent.h"
#include "ItemDataCreationComponent.h"
#include "InventoryManagerComponent.h"
#include "CombatEffectsManagerComponent.h"
#include "WBP_SecondaryHUD.h"

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

    /** UI Manager component - handles all UI updates and management */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UCharacterUIManager* UIManager;

    /** Camera Stabilization component - handles ultra-aggressive camera stabilization during attacks */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UCameraStabilizationComponent* CameraStabilizationComp;

    /** Equipment Visuals component - handles all equipment mesh swapping and visual updates */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UEquipmentVisualsComponent* EquipmentVisualsComp;

    /** Item Data Creation component - handles all item data creation, validation, and asset loading */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UItemDataCreationComponent* ItemDataCreationComp;

    /** Inventory Manager component - handles all high-level inventory operations, UI management, and input handling */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UInventoryManagerComponent* InventoryManagerComp;

    /** Combat Effects Manager component - handles all combat systems, attack chains, visual effects, and damage management */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UCombatEffectsManagerComponent* CombatEffectsManagerComp;

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

    /** Mana cost for second attack sequence */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    int32 SecondAttackManaCost = 10;

    /** Mana cost for subsequent attacks */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    int32 SubsequentAttackManaCost = 10;

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

public:
    /** Equipment Slot UI Components - Blueprint needs direct access */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Equipment Slots")
    class UWBP_InventorySlot* HeadSlot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Equipment Slots")
    class UWBP_InventorySlot* WeaponSlot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Equipment Slots")
    class UWBP_InventorySlot* SuitSlot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Equipment Slots")
    class UWBP_InventorySlot* CollectableSlot;

protected:

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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Input", meta = (AllowPrivateAccess = "true"))
    class UInputAction* DebugDamageAction;



    /** Block input action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Input", meta = (AllowPrivateAccess = "true"))
    class UInputAction* BlockAction;

    // ========== COMBAT STATE (MUST STAY - Blueprint reads these) ==========
    
    /** Combat state flags - Blueprint checks these for UI and logic */
    bool bIsInvulnerable = false;
    bool bCanAttack;
    
public:
    bool bIsAttacking;
    bool bAttackNotifyInProgress = false;


    
    // ========== CAMERA STABILIZATION SYSTEM (DELEGATED TO COMPONENT) ==========
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void EnableAttackCameraStabilization();
    
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void DisableAttackCameraStabilization();
    
    UFUNCTION(BlueprintCallable, Category = "Camera")
    bool IsNearEnemies() const;

    // ========== MOVEMENT INPUT TRACKING (for camera stabilization component) ==========
    FVector2D CurrentMovementInput = FVector2D::ZeroVector;
    bool bIsPlayerTryingToMove = false;
    
    /** Direct input state tracking for more responsive camera stabilization */
    UPROPERTY(BlueprintReadOnly, Category = "Input State")
    bool bWKeyPressed = false;
    
    UPROPERTY(BlueprintReadOnly, Category = "Input State")
    bool bAKeyPressed = false;
    
    UPROPERTY(BlueprintReadOnly, Category = "Input State")
    bool bSKeyPressed = false;
    
    UPROPERTY(BlueprintReadOnly, Category = "Input State")
    bool bDKeyPressed = false;
    
    UPROPERTY(BlueprintReadOnly, Category = "Input State")
    bool bDashKeyPressed = false;
    
    /** Check if any movement or dash keys are currently pressed */
    UFUNCTION(BlueprintPure, Category = "Input State")
    bool IsAnyMovementKeyPressed() const;
    
    /** Update input state for camera stabilization (called from Blueprint) */
    UFUNCTION(BlueprintCallable, Category = "Input State")
    void UpdateInputState(bool bW, bool bA, bool bS, bool bD, bool bDash);

    // Timer handles for combat and effects
    FTimerHandle AttackCooldownTimer;
    FTimerHandle AttackNotifyTimer;
    FTimerHandle InvulnerabilityTimer;
    FTimerHandle CameraLagTimer;
    FTimerHandle CameraRotationLagTimer;
    FTimerHandle RespawnTimerHandle;
    FTimerHandle ComboResetTimer;
    FTimerHandle BlockCooldownTimer;

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



    // ========== BLOCKING SYSTEM (Blueprint sets these) ==========
    
    /** Shield blueprint class - set this to BP_Master_Shield_01 in Blueprint */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Blocking", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<AActor> ShieldBlueprintClass;

    /** Block animation montage */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* BlockMontage;

    /** Block start animation montage */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* BlockStartMontage;

    /** Block end animation montage */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* BlockEndMontage;

    /** Current blocking state */
    UPROPERTY(BlueprintReadOnly, Category = "Character|Blocking")
    bool bIsBlocking = false;

    /** Whether the player can block (not on cooldown, alive, etc.) */
    UPROPERTY(BlueprintReadOnly, Category = "Character|Blocking")
    bool bCanBlock = true;

    /** Current spawned shield instance */
    UPROPERTY(BlueprintReadOnly, Category = "Character|Blocking")
    AActor* CurrentShieldActor = nullptr;

    /** Block cooldown duration in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Blocking", meta = (ClampMin = "0.0"))
    float BlockCooldownDuration = 0.0f;

    /** Whether blocking is currently on cooldown */
    UPROPERTY(BlueprintReadOnly, Category = "Character|Blocking")
    bool bBlockOnCooldown = false;

    /** Shield positioning offset from character center (for fine-tuning) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Blocking")
    FVector ShieldPositionOffset = FVector(0.0f, 0.0f, 0.0f);

    /** Shield scale multiplier to adjust size around character */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Blocking", meta = (ClampMin = "0.1", ClampMax = "3.0"))
    float ShieldScale = 1.0f;

    /** Shield height offset from character feet (positive = higher, negative = lower) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Blocking", meta = (ClampMin = "-200.0", ClampMax = "200.0"))
    float ShieldHeightOffset = 50.0f;

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
    
public:
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

public:
    /** Main widget reference - needs to be accessible by UI Manager */
    UPROPERTY()
    class UWBP_Main* MainWidget;

    // ========== SECONDARY HUD SYSTEM ==========
    
    /** Secondary HUD widget class - set this in Blueprint to the WBP_SecondaryHUD class */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|SecondaryHUD", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<class UWBP_SecondaryHUD> SecondaryHUDClass;

    /** Secondary HUD widget instance - automatically created and managed */
    UPROPERTY(BlueprintReadOnly, Category = "UI|SecondaryHUD", meta = (AllowPrivateAccess = "true"))
    class UWBP_SecondaryHUD* SecondaryHUDWidget;

    /** Create and initialize the secondary HUD widget */
    UFUNCTION(BlueprintCallable, Category = "UI|SecondaryHUD")
    void CreateSecondaryHUD();

    /** Destroy the secondary HUD widget */
    UFUNCTION(BlueprintCallable, Category = "UI|SecondaryHUD")
    void DestroySecondaryHUD();

    /** Check if secondary HUD is active */
    UFUNCTION(BlueprintPure, Category = "UI|SecondaryHUD")
    bool IsSecondaryHUDActive() const { return SecondaryHUDWidget != nullptr; }

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

    /** Called when the character is removed from the world */
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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

    /** Event handler for damage dealt by player from CombatEffectsManagerComponent */
    UFUNCTION()
    void OnDamageDealtFromCombat(float DealtDamage, AActor* Target);

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

    /** Called when movement input is released */
    void StopMoving(const FInputActionValue& Value);

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



    /** Called for block input */
    void PerformBlock(const FInputActionValue& Value);

    /** Called when block input is released */
    void ReleaseBlock(const FInputActionValue& Value);
    
    // ========== DASH DIRECTION HELPER FUNCTIONS FOR BLUEPRINT ==========
    
    /** Check if player should dash backward (not pressing W or standing still) */
    UFUNCTION(BlueprintPure, Category = "Movement|Dash")
    bool ShouldDashBackward() const;
    
    /** Check if player should dash forward (pressing W) */
    UFUNCTION(BlueprintPure, Category = "Movement|Dash")
    bool ShouldDashForward() const;
    
    /** Get the current movement input vector (for advanced dash logic) */
    UFUNCTION(BlueprintPure, Category = "Movement|Dash")
    FVector2D GetCurrentMovementInput() const { return CurrentMovementInput; }

    // ========== CAMERA STABILIZATION - ROOT MOTION CONTROL ==========
    
    /** Override to handle root motion extraction and filtering during attacks */
    virtual void Tick(float DeltaSeconds) override;
    
    // Removed dangerous direct stabilization functions that were causing editor crashes

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

    // ========== EXPERIENCE SYSTEM FUNCTIONS ==========
    
    /** Add experience points to the character */
    UFUNCTION(BlueprintCallable, Category = "Experience")
    void AddExperience(int32 ExpAmount);

    /** Get current player level */
    UFUNCTION(BlueprintPure, Category = "Experience")
    int32 GetPlayerLevel() const;

    /** Get current experience points */
    UFUNCTION(BlueprintPure, Category = "Experience")
    int32 GetCurrentExp() const;

    /** Get experience needed for next level */
    UFUNCTION(BlueprintPure, Category = "Experience")
    int32 GetExpForNextLevel() const;

    /** Get experience percentage for current level */
    UFUNCTION(BlueprintPure, Category = "Experience")
    float GetExpPercentage() const;

    /** Get the stats component */
    UFUNCTION(BlueprintPure, Category = "Components")
    UCharacterStatsComponent* GetStatsComponent() const { return StatsComponent; }

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

    // ========== BLOCKING SYSTEM FUNCTIONS ==========
    
    /** Start blocking - spawns shield and plays animation */
    UFUNCTION(BlueprintCallable, Category = "Combat|Blocking")
    void StartBlocking();

    /** Stop blocking - destroys shield and ends animation */
    UFUNCTION(BlueprintCallable, Category = "Combat|Blocking")
    void StopBlocking();

    /** Check if character can currently block */
    UFUNCTION(BlueprintPure, Category = "Combat|Blocking")
    bool CanPerformBlock() const;

    /** Get current blocking state */
    UFUNCTION(BlueprintPure, Category = "Combat|Blocking")
    bool IsBlocking() const { return bIsBlocking; }
    
    /** Check if character is dead */
    UFUNCTION(BlueprintPure, Category = "Character|State")
    bool IsDead() const { return bIsDead; }

    /** Called when block cooldown expires */
    UFUNCTION()
    void OnBlockCooldownComplete();

    /** Spawn the shield actor and attach to character */
    UFUNCTION(BlueprintCallable, Category = "Combat|Blocking")
    void SpawnShield();

    /** Destroy the current shield actor */
    UFUNCTION(BlueprintCallable, Category = "Combat|Blocking")
    void DestroyShield();

    /** Handle successful block (damage mitigation) */
    UFUNCTION(BlueprintCallable, Category = "Combat|Blocking")
    void OnSuccessfulBlock(float BlockedDamage);

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

    /** Whether the character has enough mana for an attack */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool HasEnoughMana(int32 ManaCost) const;

    /** Consume mana for an attack */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ConsumeMana(int32 ManaCost);

    /** Attack montage for first attack */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    UAnimMontage* AttackMontage1;

    /** Attack montage for second attack */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    UAnimMontage* AttackMontage2;

    /** Attack montage for third attack */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    UAnimMontage* AttackMontage3;

    /** Current attack sequence (0-2) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    int32 AttackSequence;

    /** Attack recovery timer handle */
    FTimerHandle AttackRecoveryTimer;

    /** Attack recovery time */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float AttackRecoveryTime;

    /** Reset attack state */
    void ResetAttackState();

    /** Perform attack */
    void Attack();
};
