#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/Character.h"

class AAtlantisEonsCharacter;
class UAnimMontage;
class UNiagaraComponent;
class USkeletalMeshComponent;
class UWBP_SwordBloom;
class UParticleSystem;
class UMaterialInterface;
class UParticleSystemComponent;

#include "CombatEffectsManagerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatStateChanged, bool, bIsInCombat);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDamageDealt, float, DamageAmount, AActor*, Target);

/**
 * CombatEffectsManagerComponent
 * 
 * Manages all combat-related functionality, attack systems, visual effects, and damage handling.
 * Extracted from AtlantisEonsCharacter to centralize combat logic and reduce character class size.
 * 
 * Features:
 * - Attack system and combo chain management
 * - Visual effects coordination (SwordEffect, Dragons, Bloom effects)
 * - Damage application and hit reaction handling
 * - Invulnerability system management
 * - Combat timing windows and critical hit detection
 * - Animation montage coordination
 * - Combat state tracking and notifications
 */
UCLASS(BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ATLANTISEONS_API UCombatEffectsManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCombatEffectsManagerComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // ========== DELEGATES ==========
    
    /** Broadcast when combat state changes */
    UPROPERTY(BlueprintAssignable, Category = "Combat Events")
    FOnCombatStateChanged OnCombatStateChanged;
    
    /** Broadcast when damage is dealt to an enemy */
    UPROPERTY(BlueprintAssignable, Category = "Combat Events")
    FOnDamageDealt OnDamageDealt;

    // ========== COMBAT SYSTEM ==========
    
    /** Main melee attack function */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void MeleeAttack(const FInputActionValue& Value);
    
    /** Reset attack state after cooldown */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ResetAttack();
    
    /** Handle animation notify for attack damage application */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void OnMeleeAttackNotify();
    
    /** Apply damage to the character */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ApplyDamage(float InDamageAmount);
    
    /** Play hit reaction animation */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void PlayHitReactMontage();

    // ========== COMBO SYSTEM ==========
    
    /** Get the current attack montage based on combo index */
    UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
    UAnimMontage* GetCurrentAttackMontage() const;
    
    /** Reset the combo chain back to the first attack */
    UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
    void ResetComboChain();
    
    /** Advance to the next attack in the combo chain */
    UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
    void AdvanceComboChain();
    
    /** Check if we can continue the combo */
    UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
    bool CanContinueCombo() const;
    
    /** Start the combo chain timing window */
    UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
    void StartComboWindow();
    
    /** End the combo chain */
    UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
    void EndComboChain();

    // ========== BLOOM EFFECTS ==========
    
    /** Animation notify for bloom circle */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void BloomCircleNotify();
    
    /** Animation notify for bloom spark */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void BloomSparkNotify();
    
    /** Hide bloom effects */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void HideBloomEffectsNotify();
    
    /** Called when bloom window times out from widget */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void OnBloomWindowClosed();
    
    /** Try to trigger spark effect during bloom window */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    bool TryTriggerSparkEffect();
    
    /** Start bloom scaling animation */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void StartBloomScaling();
    
    /** Update bloom scaling */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void UpdateBloomScaling();
    
    /** Trigger programmatic bloom effect */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void TriggerProgrammaticBloomEffect(float MontageLength);

    // ========== SWORD EFFECT MANAGEMENT ==========
    
    /** Find and cache the SwordEffect Niagara component */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void FindSwordEffectComponent();
    
    /** Activate SwordEffect Niagara system */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void ActivateSwordEffect();
    
    /** Deactivate SwordEffect Niagara system */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void DeactivateSwordEffect();
    
    /** Check if SwordEffect should be active */
    UFUNCTION(BlueprintPure, Category = "Combat|Effects")
    bool ShouldSwordEffectBeActive() const;

    // ========== DRAGON EFFECTS MANAGEMENT ==========
    
    /** Find and cache Dragon skeletal mesh components */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void FindDragonComponents();
    
    /** Show Dragon skeletal meshes */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void ShowDragons();
    
    /** Hide Dragon skeletal meshes */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void HideDragons();
    
    /** Check if Dragons should be visible */
    UFUNCTION(BlueprintPure, Category = "Combat|Effects")
    bool ShouldDragonsBeVisible() const;

    // ========== INVULNERABILITY SYSTEM ==========
    
    /** Apply invulnerability effects */
    UFUNCTION(BlueprintCallable, Category = "Combat|Invulnerability")
    void ApplyInvulnerabilityEffects();
    
    /** Remove invulnerability effects */
    UFUNCTION(BlueprintCallable, Category = "Combat|Invulnerability")
    void RemoveInvulnerabilityEffects();
    
    /** Reset invulnerability state */
    UFUNCTION(BlueprintCallable, Category = "Combat|Invulnerability")
    void ResetInvulnerability();
    
    /** Store original materials for invulnerability effect */
    UFUNCTION(BlueprintCallable, Category = "Combat|Invulnerability")
    void StoreOriginalMaterials();

    // ========== CAMERA STABILIZATION DELEGATION ==========
    
    /** Enable attack camera stabilization */
    UFUNCTION(BlueprintCallable, Category = "Combat|Camera")
    void EnableAttackCameraStabilization();
    
    /** Disable attack camera stabilization */
    UFUNCTION(BlueprintCallable, Category = "Combat|Camera")
    void DisableAttackCameraStabilization();

    // ========== UTILITY FUNCTIONS ==========
    
    /** Face the nearest enemy before attacking */
    UFUNCTION(BlueprintCallable, Category = "Combat|Utility")
    void FaceNearestEnemy();
    
    /** Get SwordBloom widget reference */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat|Utility")
    UWBP_SwordBloom* GetSwordBloomWidget();
    
    /** Initialize component with character reference */
    UFUNCTION(BlueprintCallable, Category = "Combat|Setup")
    void InitializeWithCharacter(AAtlantisEonsCharacter* Character);

    // ========== STATE GETTERS ==========
    
    /** Check if currently attacking */
    UFUNCTION(BlueprintPure, Category = "Combat|State")
    bool IsAttacking() const { return bIsAttacking; }
    
    /** Check if can attack */
    UFUNCTION(BlueprintPure, Category = "Combat|State")
    bool CanAttack() const { return bCanAttack; }
    
    /** Check if in combo */
    UFUNCTION(BlueprintPure, Category = "Combat|State")
    bool IsInCombo() const { return bIsInCombo; }
    
    /** Check if bloom window is active */
    UFUNCTION(BlueprintPure, Category = "Combat|State")
    bool IsBloomWindowActive() const { return bBloomWindowActive; }
    
    /** Get current attack index */
    UFUNCTION(BlueprintPure, Category = "Combat|State")
    int32 GetCurrentAttackIndex() const { return CurrentAttackIndex; }
    
    /** Check if attack notify is in progress */
    UFUNCTION(BlueprintPure, Category = "Combat|State")
    bool IsAttackNotifyInProgress() const { return bAttackNotifyInProgress; }
    
    /** Check if critical window was hit */
    UFUNCTION(BlueprintPure, Category = "Combat|State")
    bool HasHitCriticalWindow() const { return bHitCriticalWindow; }
    
    /** Get current montage length */
    UFUNCTION(BlueprintPure, Category = "Combat|State")
    float GetCurrentMontageLength() const { return CurrentMontageLength; }

protected:
    // ========== COMPONENT REFERENCES ==========
    
    /** Reference to the owning character */
    UPROPERTY()
    AAtlantisEonsCharacter* OwnerCharacter;
    
    /** SwordEffect Niagara component reference */
    UPROPERTY()
    UNiagaraComponent* SwordEffectComponent;
    
    /** Dragon skeletal mesh components */
    UPROPERTY()
    USkeletalMeshComponent* Dragon1Component;
    
    UPROPERTY()
    USkeletalMeshComponent* Dragon2Component;
    
    /** Active particle system components */
    UPROPERTY()
    UParticleSystemComponent* InvulnerabilityPSC;

    // ========== COMBAT STATE ==========
    
    /** Combat state flags */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|State")
    bool bCanAttack = true;
    
    UPROPERTY(BlueprintReadOnly, Category = "Combat|State")
    bool bIsAttacking = false;
    
    UPROPERTY(BlueprintReadOnly, Category = "Combat|State")
    bool bAttackNotifyInProgress = false;
    
    UPROPERTY(BlueprintReadOnly, Category = "Combat|State")
    bool bIsInvulnerable = false;

    // ========== COMBO SYSTEM STATE ==========
    
    /** Current attack combo index */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo")
    int32 CurrentAttackIndex = 0;
    
    /** Whether we're currently in a combo chain */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo")
    bool bIsInCombo = false;
    
    /** Whether we hit the critical window in the current attack */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo")
    bool bHitCriticalWindow = false;
    
    /** Whether the bloom window is currently active */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Effects")
    bool bBloomWindowActive = false;
    
    /** Current attack montage duration for bloom effect timing */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Effects")
    float CurrentMontageLength = 0.0f;

    // ========== CONFIGURATION ==========
    
    /** Attack cooldown duration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Configuration")
    float AttackCooldown = 0.5f;
    
    /** Maximum number of combo attacks */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Configuration")
    int32 MaxComboAttacks = 4;
    
    /** Combo window duration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Configuration")
    float ComboWindowDuration = 2.0f;
    
    /** Attack range */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Configuration")
    float AttackRange = 300.0f;
    
    /** Attack radius for detection */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Configuration")
    float AttackRadius = 200.0f;

    // ========== TIMER HANDLES ==========
    
    /** Timer handles for combat and effects */
    FTimerHandle AttackCooldownTimer;
    FTimerHandle AttackNotifyTimer;
    FTimerHandle AttackNotifyFlagResetTimer;
    FTimerHandle InvulnerabilityTimer;
    FTimerHandle ComboResetTimer;
    FTimerHandle SwordBloomHideTimer;
    FTimerHandle BloomWindowTimer;
    FTimerHandle BloomScaleTimer;

    // ========== INVULNERABILITY SYSTEM ==========
    
    /** Stored original materials for invulnerability effect */
    UPROPERTY()
    TArray<UMaterialInterface*> OriginalMaterials;

private:
    /** Validate component state */
    bool IsValidForCombat() const;
    
    /** Calculate total damage including weapon bonuses */
    float CalculateTotalDamage() const;
    
    /** Perform damage detection and application */
    bool PerformDamageDetection(float TotalDamage);
}; 