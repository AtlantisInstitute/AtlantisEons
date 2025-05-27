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
#include "BP_ItemInterface.h"
#include "BP_ItemInfo.h"
#include "WBP_Main.h"
class UWBP_CharacterInfo;
#include "WBP_GuideText.h"
#include "WBP_InventorySlot.h"

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

UCLASS(config=Game)
class ATLANTISEONS_API AAtlantisEonsCharacter : public ACharacter, public IGenericTeamAgentInterface
{
    GENERATED_BODY()

public:

    /** Camera boom positioning the camera behind the character */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    class USpringArmComponent* CameraBoom;

    /** Follow camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    class UCameraComponent* FollowCamera;

    /** AI Perception component */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
    class UAIPerceptionStimuliSourceComponent* AIPerceptionStimuliSourceComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Health", meta = (AllowPrivateAccess = "true"))
    float CurrentHealth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Health", meta = (AllowPrivateAccess = "true"))
    float MaxHealth;

    UPROPERTY(BlueprintAssignable, Category = "Character|Stats")
    FOnStatsUpdatedSignature OnStatsUpdated;

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

    // Inventory and Equipment
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    TArray<UBP_ItemInfo*> InventoryItems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    TArray<UWBP_InventorySlot*> InventorySlotWidgets;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    TArray<UBP_ItemInfo*> EquipmentSlots;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    bool InventoryToggleLock;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    float InventoryToggleLockDuration;

    FTimerHandle InventoryToggleLockTimer;

    /** UI Components */
    UPROPERTY(BlueprintReadWrite, Category = "UI")
    class UWBP_Main* Main;

    // UI Progress Bars
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

    FORCEINLINE UInputMappingContext* GetDefaultMappingContext() { return DefaultMappingContext; }

    UFUNCTION(BlueprintImplementableEvent, Category = "Character|Combat")
    void PlayHitReactMontage();

    /** Equipment Components */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* Helmet;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* Weapon;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* Shield;

    /** Equipment Functions */
    UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
    void EquipItem(EItemEquipSlot Slot, const TSoftObjectPtr<UStaticMesh>& MeshToEquip, UTexture2D* Thumbnail, 
        int32 ItemIndex, UMaterialInterface* Material1 = nullptr, UMaterialInterface* Material2 = nullptr);

    UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
    void DisarmItem(EItemEquipSlot ItemEquipSlot, const TSoftObjectPtr<UStaticMesh>& StaticMeshID, int32 ItemIndex);

    /** Helper function to get the game instance */
    UFUNCTION(BlueprintImplementableEvent, Category = "Game Instance")
    UAtlantisEonsGameInstance* GetGameInstanceHelper() const;

    /** Helper function to get item info */
    UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
    bool GetItemInfo(int32 ItemIndex, FStructure_ItemInfo& OutItemInfo) const;

    /** Add status effects to character based on equipped item */
    UFUNCTION(BlueprintImplementableEvent, Category = "Stats")
    void AddingCharacterStatus(int32 ItemIndex);

    /** Remove status effects from character based on unequipped item */
    UFUNCTION(BlueprintImplementableEvent, Category = "Stats")
    void SubtractingCharacterStatus(int32 ItemIndex);

protected:
    /** Equipment Slot UI Components */
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

    /** MappingContext */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    class UInputMappingContext* DefaultMappingContext;

    /** Input Action for melee attack */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    class UInputAction* MeleeAttackAction;

    /** Movement Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* MoveAction;

    /** Look Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* LookAction;

    /** Pickup Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* PickupAction;

    /** Jump Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* JumpAction;

    /** Animation montage for the melee attack action */
    // Combat state
    bool bIsInvulnerable = false;
    bool bCanAttack;
    bool bIsAttacking;

    // Timer handles
    FTimerHandle AttackCooldownTimer;
    FTimerHandle DashCooldownTimer;
    FTimerHandle InvulnerabilityTimer;
    FTimerHandle DashTimer;
    FTimerHandle CameraLagTimer;
    FTimerHandle CameraRotationLagTimer;

    /** Melee attack animation montage */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* MeleeAttackMontage;

    /** Invulnerability effect */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|VFX", meta = (AllowPrivateAccess = "true"))
    UParticleSystem* InvulnerabilityEffect;

    /** Material for invulnerability effect */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|VFX", meta = (AllowPrivateAccess = "true"))
    UMaterialInterface* InvulnerabilityMaterial;

    // Camera settings for dodge removed for manual implementation

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

    /** Debug Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* DebugDamageAction;

    /** Inventory Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* InventoryAction;
    
    /** Resume/ESC Input Action - Used to close inventory and UI elements */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* ResumeAction;
    

    /** Whether the inventory is currently open */
    UPROPERTY(BlueprintReadOnly, Category = "UI|Inventory")
    bool bIsInventoryOpen;
    
    /** Whether the inventory toggle is locked (to prevent conflicts) */
    UPROPERTY(BlueprintReadOnly, Category = "UI|Inventory")
    bool bInventoryToggleLocked;
    
    /** Timer handle for the inventory toggle lock */

    
    /** Unlocks the inventory toggle after a delay */
    void UnlockInventoryToggle();
    
    /** The inventory widget class */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Inventory", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UUserWidget> InventoryWidgetClass;
    
    /** The created inventory widget instance */
    UPROPERTY(BlueprintReadOnly, Category = "UI|Inventory", meta = (AllowPrivateAccess = "true"))
    UUserWidget* InventoryWidget;

    /** Amount of damage dealt by attacks */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float DamageAmount;

    // Dodge movement properties removed for manual implementation

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

private:
    bool bHealthDelegateBound = false;

    /** Stored original materials for invulnerability effect */
    TArray<UMaterialInterface*> OriginalMaterials;

    /** Active particle system components */

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
public:
    UFUNCTION(BlueprintCallable, Category = "Character|Input")
    UInputMappingContext* GetDefaultMappingContext() const { return DefaultMappingContext; }

// Input handling
virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

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

    /** Called for dodge input */
    // Dodge function removed for manual implementation

    /** Called for melee attack input */
    void MeleeAttack(const FInputActionValue& Value);

// ... (rest of the code remains the same)
    // Dash and dodge reset functions removed for manual implementation in Unreal Engine

    /** Called when invulnerability period expires */
    UFUNCTION()
    void ResetInvulnerability();

public:
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
    UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
    void ApplyCharacterDamage(float Amount);

    /** Handle character death */
    UFUNCTION(BlueprintImplementableEvent, Category = "Character")
    void HandleDeath();

public:
    UFUNCTION(BlueprintImplementableEvent, Category = "Character")
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

    UFUNCTION(BlueprintImplementableEvent, Category = "Character")
    void RespawnCharacter();

    UFUNCTION(BlueprintImplementableEvent, Category = "Debug")
    void DebugDamage(const FInputActionValue& Value);

    UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
    void FaceNearestEnemy();

    // ... (rest of the code remains the same)
    UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
    void OnMeleeAttackNotify();

    UFUNCTION(BlueprintImplementableEvent, Category = "Debug")
    void ApplyDebugDamage();

    UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
    void ShowDamageNumber(float InDamageAmount, FVector Location, bool bIsCritical = false);

    UFUNCTION(BlueprintPure, Category = "Health")
    float GetCurrentHealth() const { return CurrentHealth; }

    UFUNCTION(BlueprintPure, Category = "Character|Stats")
    float GetBaseHealth() const { return BaseHealth; }

    UFUNCTION(BlueprintImplementableEvent, Category = "Stats")
    void SetBaseHealth(float NewBaseHealth);

    UFUNCTION(BlueprintPure, Category = "Character|Stats")
    float GetBaseMovementSpeed() const { return BaseMovementSpeed; }

    UFUNCTION(BlueprintPure, Category = "Character|Stats")
    int32 GetCurrentSTR() const { return CurrentSTR; }

    UFUNCTION(BlueprintPure, Category = "Character|Stats")
    int32 GetCurrentDEX() const { return CurrentDEX; }

    UFUNCTION(BlueprintPure, Category = "Character|Stats")
    int32 GetCurrentINT() const { return CurrentINT; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int32 YourGold;
    
    // Gold property that mirrors YourGold for compatibility with store system
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int32 Gold;

    UFUNCTION(BlueprintPure, Category = "Character|Stats")
    int32 GetYourGold() 
    { 
        // Keep Gold synchronized with YourGold
        Gold = YourGold;
        return YourGold; 
    }

    UFUNCTION(BlueprintPure, Category = "Character|Stats")
    int32 GetCurrentMP() const { return CurrentMP; }

    UFUNCTION(BlueprintPure, Category = "Character|Stats")
    int32 GetMaxMP() const { return MaxMP; }

    UFUNCTION(BlueprintPure, Category = "Character|Stats")
    int32 GetCurrentDefence() const { return CurrentDefence; }

UFUNCTION(BlueprintPure, Category = "Character|Stats")
int32 GetCurrentDamage() const { return CurrentDamage; }

UFUNCTION(BlueprintCallable, Category = "Character|Stats")
void UpdateAllStats();

UFUNCTION(BlueprintCallable, Category = "Character|Stats")
void RefreshStatsDisplay();

UFUNCTION(BlueprintCallable, Category = "Character|Inventory")
void SettingStore();

UFUNCTION(BlueprintCallable, Category = "Character|Inventory")
void SetInventorySlot(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotWidgetRef);

UFUNCTION(BlueprintCallable, Category = "Character|Inventory")
bool PickingItem(int32 ItemIndex, int32 ItemStackNumber);

UFUNCTION(BlueprintCallable, Category = "Character|Inventory")
void ContextMenuUse(UWBP_InventorySlot* InventorySlot);

UFUNCTION(BlueprintCallable, Category = "Character|Inventory")
void ContextMenuThrow(UWBP_InventorySlot* InventorySlot);

UFUNCTION(BlueprintCallable, Category = "Character|Inventory")
void ContextMenuUse_EquipItem(UBP_ItemInfo* ItemInfoRef);

UFUNCTION(BlueprintCallable, Category = "Character|Inventory")
void ContextMenuUse_ConsumeItem(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotRef,
    int32 RecoverHP, int32 RecoverMP, EItemType ItemType);

UFUNCTION(BlueprintCallable, Category = "Character|Inventory")
bool BuyingItem(int32 ItemIndex, int32 ItemStackNumber, int32 ItemPrice);

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

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
UStaticMeshComponent* HeadMesh;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
UStaticMeshComponent* BodyMesh;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
UStaticMeshComponent* WeaponMesh;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
UStaticMeshComponent* AccessoryMesh;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Status")
int32 CurrentMP;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Status")
int32 BaseMP;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
int32 MaxMP;

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

private:
    UPROPERTY()
    FGenericTeamId TeamId;

    UPROPERTY()
    FTimerHandle DelayedCaptureTimer;

    UPROPERTY()
    FTimerHandle InventoryUpdateTimer;

    UPROPERTY(VisibleAnywhere, Category = "Stats")
    bool bIsDead = false;

public:
    /** Implementation of inventory closing */
    UFUNCTION()
    void CloseInventoryImpl();

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
};
