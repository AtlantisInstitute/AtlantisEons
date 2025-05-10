// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"

#include "AtlantisEonsCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UAIPerceptionStimuliSourceComponent;
struct FInputActionValue;

class ADamageNumberSystem;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);


UCLASS(config=Game, Blueprintable)
class ATLANTISEONS_API AAtlantisEonsCharacter : public ACharacter, public IGenericTeamAgentInterface
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Health", meta = (AllowPrivateAccess = "true"))
    float CurrentHealth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Health", meta = (AllowPrivateAccess = "true"))
    float MaxHealth;

    /** Camera boom positioning the camera behind the character */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    USpringArmComponent* CameraBoom;

    /** Follow camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FollowCamera;
    
protected:
    /** MappingContext */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    class UInputMappingContext* DefaultMappingContext;

    /** Input Action for melee attack */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    class UInputAction* MeleeAttackAction;

    /** Jump Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* JumpAction;

    /** Move Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* MoveAction;

    /** Look Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* LookAction;

    /** Dodge Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* DodgeAction;



    /** Dodge properties */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Dodge", meta = (AllowPrivateAccess = "true"))
    float DodgeDistance = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Dodge", meta = (AllowPrivateAccess = "true"))
    float DodgeCooldown = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Dodge", meta = (AllowPrivateAccess = "true"))
    float DodgeInvulnerabilityDuration = 0.5f;

    /** Animation montage for the dodge action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* TwinSword_Dodge_F_Montage;

    /** Melee attack animation montage */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Animation", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* MeleeAttackMontage;

    /** Dodge trail effect */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|VFX", meta = (AllowPrivateAccess = "true"))
    UParticleSystem* DodgeTrailEffect;

    /** Invulnerability effect */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|VFX", meta = (AllowPrivateAccess = "true"))
    UParticleSystem* InvulnerabilityEffect;

    /** Material for invulnerability effect */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|VFX", meta = (AllowPrivateAccess = "true"))
    UMaterialInterface* InvulnerabilityMaterial;

    /** Camera settings for dodge */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Camera", meta = (AllowPrivateAccess = "true"))
    float DodgeCameraLag = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Camera", meta = (AllowPrivateAccess = "true"))
    float DodgeCameraRotationLag = 0.1f;

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
    FTimerHandle InventoryToggleLockTimer;
    
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

    /** Force applied during dodge */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float DodgeForce;

    /** Threshold at which to stop dodge movement */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float DodgeStopThreshold;

    /** Radius of the damage sphere */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float DamageSphereRadius;

    /** Offset of the damage sphere from character */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float DamageSphereOffset;

    /** Range of attack */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float AttackRange;

    virtual void PostInitProperties() override;



    /** AI Perception Stimuli Source */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
    UAIPerceptionStimuliSourceComponent* StimuliSource;

private:
    FTimerHandle DodgeCooldownTimer;
    FTimerHandle InvulnerabilityTimer;
    FTimerHandle RespawnTimerHandle;
    FTimerHandle AttackCooldownTimer;
    bool bCanDodge = true;
    bool bIsInvulnerable = false;
    bool bCanAttack = true;
    bool bIsDodging = false;
    bool bIsAttacking = false;
    bool bHealthDelegateBound = false;

    /** Stored original materials for invulnerability effect */
    TArray<UMaterialInterface*> OriginalMaterials;
    /** Active particle system components */
    UPROPERTY()
    UParticleSystemComponent* InvulnerabilityPSC;

    float AttackCooldown = 0.5f;

    /** Delay before respawning */
    UPROPERTY(EditAnywhere, Category = "Character|Health", meta = (AllowPrivateAccess = "true"))
    float RespawnDelay = 3.0f;

protected:
    // Team ID for AI perception
    FGenericTeamId TeamId;

    /** Called when the game starts or when spawned */
    virtual void BeginPlay() override;

    /** Called for movement input */
    void Move(const FInputActionValue& Value);

    /** Called for looking input */
    void Look(const FInputActionValue& Value);

    /** Called for dodge input */
    void Dodge(const FInputActionValue& Value);

    /** Called for melee attack input */
    void MeleeAttack(const FInputActionValue& Value);

    /** Called when dodge cooldown expires */
    UFUNCTION()
    void ResetDodge();

    /** Called when invulnerability period expires */
    UFUNCTION()
    void ResetInvulnerability();

    /** Called when attack cooldown expires */
    UFUNCTION()
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

    /** Handle character death */
    UFUNCTION()
    void HandleDeath();

    /** Reset character after death */
    UFUNCTION()
    void ResetCharacter();

    /** Toggle the inventory open/closed */
    UFUNCTION()
    void ToggleInventory(const FInputActionValue& Value);

    /** Open the inventory */
    UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
    void OpenInventory();

public:
    /** Close the inventory */
    UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
    void CloseInventory();

    /** Respawn the character */
    UFUNCTION()
    void RespawnCharacter();

    /** Debug function to apply damage */
    void DebugDamage(const FInputActionValue& Value);

    virtual void NotifyControllerChanged() override;

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    UFUNCTION()
    void FaceNearestEnemy();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void OnMeleeAttackNotify();

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void ApplyDebugDamage();

public:
    UFUNCTION(BlueprintPure, Category = "Health")
    float GetCurrentHealth() const { return CurrentHealth; }

    UFUNCTION(BlueprintPure, Category = "Health")
    float GetMaxHealth() const { return MaxHealth; }
    
    UFUNCTION(BlueprintPure, Category = "UI|Inventory")
    bool IsInventoryOpen() const { return bIsInventoryOpen; }
    
    /** Get the inventory widget class type for identification purposes */
    UFUNCTION(BlueprintPure, Category = "UI|Inventory")
    TSubclassOf<UUserWidget> GetInventoryWidgetClass() const { return InventoryWidgetClass; }

    /**
     * Locks or unlocks the inventory toggle functionality for a short period
     * Used to prevent inventory from being toggled when main menu is closing
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
    void SetInventoryToggleLock(bool bLock, float UnlockDelay = 0.5f);
    
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
    UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
    void CloseInventoryIfOpen(const FInputActionValue& Value);

    AAtlantisEonsCharacter();

    // IGenericTeamAgentInterface
    virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }
    virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) { TeamId = NewTeamId; }

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UAIPerceptionStimuliSourceComponent* AIPerceptionStimuliSourceComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
    bool bIsDead = false;


/** Animation montage for hit reaction */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Combat")
UAnimMontage* HitReactMontage;

/** Called when the character takes damage */
virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

/** Returns CameraBoom subobject **/
FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
/** Returns FollowCamera subobject **/
FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
/** Returns DefaultMappingContext **/
FORCEINLINE class UInputMappingContext* GetDefaultMappingContext() const { return DefaultMappingContext; }

/** Play hit reaction montage */
UFUNCTION(BlueprintCallable, Category = "Character|Combat")
void PlayHitReactMontage();
};
