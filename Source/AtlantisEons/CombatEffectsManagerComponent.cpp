#include "CombatEffectsManagerComponent.h"
#include "AtlantisEonsCharacter.h"
#include "WBP_SwordBloom.h"
#include "CameraStabilizationComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraComponent.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Character.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"
#include "StoreSystemFix.h"

UCombatEffectsManagerComponent::UCombatEffectsManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    
    // Initialize state
    bCanAttack = true;
    bIsAttacking = false;
    bAttackNotifyInProgress = false;
    bIsInvulnerable = false;
    CurrentAttackIndex = 0;
    bIsInCombo = false;
    bHitCriticalWindow = false;
    bBloomWindowActive = false;
    CurrentMontageLength = 0.0f;
    
    // Initialize configuration
    AttackCooldown = 0.5f;
    MaxComboAttacks = 4;
    ComboWindowDuration = 2.0f;
    AttackRange = 300.0f;
    AttackRadius = 200.0f;
    
    // Initialize component references
    OwnerCharacter = nullptr;
    SwordEffectComponent = nullptr;
    Dragon1Component = nullptr;
    Dragon2Component = nullptr;
    InvulnerabilityPSC = nullptr;
}

void UCombatEffectsManagerComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Get reference to owning character
    OwnerCharacter = Cast<AAtlantisEonsCharacter>(GetOwner());
    
    if (OwnerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ CombatEffectsManager: Initialized with character %s"), *OwnerCharacter->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("🗡️ CombatEffectsManager: Failed to get character reference!"));
    }
}

void UCombatEffectsManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    // Handle any per-frame combat updates if needed
}

void UCombatEffectsManagerComponent::InitializeWithCharacter(AAtlantisEonsCharacter* Character)
{
    OwnerCharacter = Character;
    
    if (OwnerCharacter)
    {
        // Find SwordEffect and Dragon components
        FindSwordEffectComponent();
        FindDragonComponents();
        
        UE_LOG(LogTemp, Warning, TEXT("🗡️ CombatEffectsManager: Manual initialization complete"));
    }
}

// ========== COMBAT SYSTEM ==========

void UCombatEffectsManagerComponent::MeleeAttack(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("🗡️ ATTACK INPUT at %.3f seconds"), GetWorld()->GetTimeSeconds());
    
    if (!IsValidForCombat())
    {
        return;
    }
    
    // CRITICAL FIX: If bloom window is active, ONLY handle spark attempts, don't start new attacks
    if (bBloomWindowActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ ⚡ Bloom window active - attempting spark trigger"));
        UE_LOG(LogTemp, Warning, TEXT("🗡️ DEBUG: Component bBloomWindowActive = TRUE"));
        UE_LOG(LogTemp, Warning, TEXT("🗡️ DEBUG: Attack input timing - Window still open at %.3f seconds"), GetWorld()->GetTimeSeconds());
        bool bTriggeredSpark = TryTriggerSparkEffect();
        
        if (bTriggeredSpark)
        {
            UE_LOG(LogTemp, Warning, TEXT("🗡️ ✨ Critical window hit! Setting flag for combo continuation"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("🗡️ ⚠️ Attack input during bloom window but outside critical timing"));
        }
        return; // ALWAYS return when bloom is active - don't start new attacks
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ DEBUG: Component bBloomWindowActive = FALSE - will start new attack"));
        UE_LOG(LogTemp, Warning, TEXT("🗡️ DEBUG: Attack input timing - No bloom window at %.3f seconds"), GetWorld()->GetTimeSeconds());
    }

    if (!bCanAttack)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ Cannot attack - on cooldown"));
        return;
    }

    // COMBO LOGIC: Check if this should be a combo continuation
    bool bShouldContinueCombo = (bIsInCombo && bHitCriticalWindow && CurrentAttackIndex < MaxComboAttacks - 1);
    
    if (bShouldContinueCombo)
    {
        // This is a combo continuation - advance to next attack
        AdvanceComboChain();
        UE_LOG(LogTemp, Warning, TEXT("🗡️ ⚡ COMBO CONTINUATION! Advanced to attack %d"), CurrentAttackIndex + 1);
    }
    else if (bIsInCombo && !bHitCriticalWindow)
    {
        // Failed to hit critical window for combo - reset and start new attack chain
        UE_LOG(LogTemp, Warning, TEXT("🗡️ ❌ Combo chain broken - missed critical window, starting fresh"));
        ResetComboChain();
    }
    else if (!bIsInCombo)
    {
        // Starting a new attack sequence
        UE_LOG(LogTemp, Warning, TEXT("🗡️ 🆕 Starting new attack sequence"));
    }

    // NOW reset the critical window flag for this new attack
    bHitCriticalWindow = false;

    // Face the nearest enemy before attacking
    FaceNearestEnemy();
    
    // Enable attack camera stabilization
    EnableAttackCameraStabilization();

    // Get the appropriate attack montage based on current attack index
    UAnimMontage* CurrentMontage = GetCurrentAttackMontage();
    
    if (CurrentMontage && OwnerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ Playing attack montage %d: %s"), 
               CurrentAttackIndex + 1, *CurrentMontage->GetName());
        
        // Try to play the montage
        float MontageLength = OwnerCharacter->PlayAnimMontage(CurrentMontage);
        
        if (MontageLength > 0.0f)
        {
            // Store the montage length for bloom effect timing
            CurrentMontageLength = MontageLength;
            
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Successfully playing attack montage with duration: %f"), MontageLength);
            
            // Start the bloom circle immediately when attack begins
            UE_LOG(LogTemp, Warning, TEXT("🔵 ⚡ BLOOM WINDOW ACTIVATED - Starting immediately with attack at %.3f seconds"), GetWorld()->GetTimeSeconds());
            UE_LOG(LogTemp, Warning, TEXT("🔵 🎯 Bloom window will be ACTIVE for the next 0.8 seconds (until %.3f seconds)"), GetWorld()->GetTimeSeconds() + 0.8f);
            
            // Clear any existing bloom hide timers to prevent conflicts
            GetWorld()->GetTimerManager().ClearTimer(SwordBloomHideTimer);
            UE_LOG(LogTemp, Warning, TEXT("🔧 Cleared existing bloom hide timers for new attack"));
            
            bBloomWindowActive = true;
            UE_LOG(LogTemp, Warning, TEXT("🔵 ✅ Component bBloomWindowActive flag set to TRUE"));
            
            if (UWBP_SwordBloom* SwordBloom = GetSwordBloomWidget()) 
            { 
                SwordBloom->StartBloomCircle(); 
                UE_LOG(LogTemp, Warning, TEXT("🔵 ✅ SwordBloom widget found - StartBloomCircle() called at attack start")); 
                UE_LOG(LogTemp, Warning, TEXT("🔵 💡 READY: Press attack again during the next 0.8 seconds to trigger spark!"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("🔵 ❌ SwordBloom widget NOT found - no visual effects will show")); 
            }
            
            // Start the combo window after this attack
            StartComboWindow();
            
            // Add a backup timer to call OnMeleeAttackNotify
            float NotifyTiming = MontageLength * 0.6f; // Call at 60% through the animation
            GetWorld()->GetTimerManager().SetTimer(
                AttackNotifyTimer,
                FTimerDelegate::CreateUObject(this, &UCombatEffectsManagerComponent::OnMeleeAttackNotify),
                NotifyTiming,
                false
            );
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Set backup timer to call OnMeleeAttackNotify in %.2f seconds"), NotifyTiming);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("🗡️ Failed to play attack montage"));
            ResetAttack();
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("🗡️ No attack montage available for index %d!"), CurrentAttackIndex);
        ResetAttack();
    }
    
    // Set timer for cooldown reset
    GetWorld()->GetTimerManager().SetTimer(
        AttackCooldownTimer,
        FTimerDelegate::CreateUObject(this, &UCombatEffectsManagerComponent::ResetAttack),
        AttackCooldown,
        false
    );
}

void UCombatEffectsManagerComponent::ResetAttack()
{
    bCanAttack = true;
    bIsAttacking = false;
    
    // Disable attack camera stabilization when attack ends
    DisableAttackCameraStabilization();
    
    // Always reset critical window flag after processing
    if (bHitCriticalWindow)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ Resetting critical window flag after combo processing"));
        bHitCriticalWindow = false;
    }
    else
    {
        // CRITICAL FIX: Don't reset combo chain if bloom window is still active
        if (bBloomWindowActive)
        {
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Attack finished but bloom window still active - keeping combo chain alive"));
            UE_LOG(LogTemp, Warning, TEXT("🗡️ 💡 Player still has time to hit the spark timing!"));
        }
        else
        {
            // If we're not continuing a combo, reset the chain
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Attack finished without critical window hit - resetting combo"));
            ResetComboChain();
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🗡️ Attack cooldown reset. Can attack again."));
}

void UCombatEffectsManagerComponent::OnMeleeAttackNotify()
{
    UE_LOG(LogTemp, Warning, TEXT("🗡️ OnMeleeAttackNotify called - Player attacking"));
    
    if (!OwnerCharacter || !IsValidForCombat())
    {
        return;
    }
    
    // Prevent multiple calls in the same attack sequence
    if (bAttackNotifyInProgress)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ Attack notify already in progress, skipping duplicate call"));
        return;
    }
    
    bAttackNotifyInProgress = true;
    
    // Reset the flag after a short delay to allow for the next attack
    GetWorld()->GetTimerManager().SetTimer(
        AttackNotifyFlagResetTimer,
        [this]() { 
            bAttackNotifyInProgress = false;
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Attack notify flag reset - ready for next attack"));
        },
        0.5f,
        false
    );
    
    // Calculate total damage including weapon bonuses
    float TotalDamage = CalculateTotalDamage();
    
    // Perform damage detection and application
    bool bSuccessfulHit = PerformDamageDetection(TotalDamage);
    
    if (bSuccessfulHit)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ ✅ Successfully hit at least one enemy!"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ ❌ No valid enemies found in attack range"));
    }
    
    // Schedule hiding bloom effects after a short delay
    float BloomDuration = FMath::Max(CurrentMontageLength - 0.2f, 0.5f);
    UE_LOG(LogTemp, Warning, TEXT("🗡️ 🔵 Bloom effects will last for %.2f seconds"), BloomDuration);
    
    GetWorld()->GetTimerManager().SetTimer(
        SwordBloomHideTimer,
        [this]() { 
            HideBloomEffectsNotify();
            UE_LOG(LogTemp, Warning, TEXT("🗡️ 🔒 SwordBloom effects hidden after attack"));
        },
        BloomDuration,
        false
    );
}

void UCombatEffectsManagerComponent::ApplyDamage(float InDamageAmount)
{
    if (!OwnerCharacter)
    {
        return;
    }
    
    // Delegate to character's stats component or handle internally
    UE_LOG(LogTemp, Warning, TEXT("🗡️ CombatEffectsManager: Applying %.1f damage"), InDamageAmount);
    
    // This could be delegated to character's ApplyDamage or handled here
    // For now, delegate back to character
    OwnerCharacter->ApplyDamage(InDamageAmount);
}

void UCombatEffectsManagerComponent::PlayHitReactMontage()
{
    if (!OwnerCharacter)
    {
        return;
    }
    
    // Delegate to character for now since it has access to the montages
    OwnerCharacter->PlayHitReactMontage();
}

// ========== COMBO SYSTEM ==========

UAnimMontage* UCombatEffectsManagerComponent::GetCurrentAttackMontage() const
{
    if (!OwnerCharacter)
    {
        return nullptr;
    }
    
    UAnimMontage* SelectedMontage = nullptr;
    
    switch (CurrentAttackIndex)
    {
        case 0:
            SelectedMontage = OwnerCharacter->MeleeAttackMontage1;
            break;
        case 1:
            SelectedMontage = OwnerCharacter->MeleeAttackMontage2;
            break;
        case 2:
            SelectedMontage = OwnerCharacter->MeleeAttackMontage4; // Skip MeleeAttackMontage3
            break;
        case 3:
            SelectedMontage = OwnerCharacter->MeleeAttackMontage5;
            break;
        default:
            SelectedMontage = OwnerCharacter->MeleeAttackMontage1;
            break;
    }
    
    return SelectedMontage;
}

void UCombatEffectsManagerComponent::ResetComboChain()
{
    CurrentAttackIndex = 0;
    bIsInCombo = false;
    bHitCriticalWindow = false;
    
    // Reset bloom window when combo chain ends
    bBloomWindowActive = false;
    UE_LOG(LogTemp, Warning, TEXT("🔧 Reset bloom window flag when resetting combo chain"));
    
    // Disable camera stabilization when combo chain resets
    DisableAttackCameraStabilization();
    
    // Deactivate SwordEffect when reverting to first attack
    UE_LOG(LogTemp, Warning, TEXT("🔒 🌟 Combo chain reset - DEACTIVATING SwordEffect"));
    DeactivateSwordEffect();
    
    // Hide Dragons when combo ends
    UE_LOG(LogTemp, Warning, TEXT("🔒 🐉 Combo chain reset - HIDING Dragons"));
    HideDragons();
    
    // Clear combo reset timer
    GetWorld()->GetTimerManager().ClearTimer(ComboResetTimer);
    
    UE_LOG(LogTemp, Warning, TEXT("🔄 Combo chain reset to first attack"));
}

void UCombatEffectsManagerComponent::AdvanceComboChain()
{
    if (CurrentAttackIndex < MaxComboAttacks - 1)
    {
        CurrentAttackIndex++;
        UE_LOG(LogTemp, Warning, TEXT("⚡ Advanced combo to attack %d/%d"), CurrentAttackIndex + 1, MaxComboAttacks);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚡ Reached maximum combo attacks, resetting chain"));
        ResetComboChain();
    }
}

bool UCombatEffectsManagerComponent::CanContinueCombo() const
{
    return bIsInCombo && bHitCriticalWindow && (CurrentAttackIndex < MaxComboAttacks - 1);
}

void UCombatEffectsManagerComponent::StartComboWindow()
{
    bIsInCombo = true;
    
    // Clear any existing combo reset timer
    GetWorld()->GetTimerManager().ClearTimer(ComboResetTimer);
    
    // Set timer to reset combo if no critical window is hit
    GetWorld()->GetTimerManager().SetTimer(
        ComboResetTimer,
        FTimerDelegate::CreateUObject(this, &UCombatEffectsManagerComponent::EndComboChain),
        ComboWindowDuration,
        false
    );
    
    UE_LOG(LogTemp, Warning, TEXT("⏰ Started combo window (%.1fs to hit critical timing)"), ComboWindowDuration);
}

void UCombatEffectsManagerComponent::EndComboChain()
{
    UE_LOG(LogTemp, Warning, TEXT("🔚 Ending combo chain - resetting to first attack"));
    
    // Reset bloom window when combo chain ends
    bBloomWindowActive = false;
    UE_LOG(LogTemp, Warning, TEXT("🔧 Reset bloom window flag when ending combo chain"));
    
    // Disable camera stabilization when combo chain ends
    DisableAttackCameraStabilization();
    
    // Deactivate SwordEffect when combo times out
    UE_LOG(LogTemp, Warning, TEXT("🔒 🌟 Combo chain timed out - DEACTIVATING SwordEffect"));
    DeactivateSwordEffect();
    
    // Hide Dragons when combo times out
    UE_LOG(LogTemp, Warning, TEXT("🔒 🐉 Combo chain timed out - HIDING Dragons"));
    HideDragons();
    
    ResetComboChain();
}

// ========== BLOOM EFFECTS ==========

void UCombatEffectsManagerComponent::BloomCircleNotify()
{
    UE_LOG(LogTemp, Warning, TEXT("🔵 BloomCircleNotify called - implement in Blueprint"));
}

void UCombatEffectsManagerComponent::BloomSparkNotify()
{
    UE_LOG(LogTemp, Warning, TEXT("✨ BloomSparkNotify called - implement in Blueprint"));
}

void UCombatEffectsManagerComponent::HideBloomEffectsNotify()
{
    bBloomWindowActive = false;
    UE_LOG(LogTemp, Warning, TEXT("🔒 HideBloomEffectsNotify called - bloom window deactivated"));
}

void UCombatEffectsManagerComponent::OnBloomWindowClosed()
{
    bBloomWindowActive = false;
    UE_LOG(LogTemp, Warning, TEXT("🔒 OnBloomWindowClosed called at %.3f seconds - widget notified component that bloom window timed out"), GetWorld()->GetTimeSeconds());
    UE_LOG(LogTemp, Warning, TEXT("🔒 ❌ Component bBloomWindowActive flag set to FALSE - window is now CLOSED"));
}

bool UCombatEffectsManagerComponent::TryTriggerSparkEffect()
{
    // Check if we're currently in a bloom window
    if (!bBloomWindowActive)
    {
        return false;
    }
    
    // Get the SwordBloom widget and check if we hit the critical window
    if (UWBP_SwordBloom* SwordBloom = GetSwordBloomWidget())
    {
        bool bSuccessfulSpark = SwordBloom->TryTriggerSpark();
        
        if (bSuccessfulSpark)
        {
            UE_LOG(LogTemp, Warning, TEXT("✨ ✅ CRITICAL WINDOW HIT! Processing spark effect"));
            
            // Mark that we hit the critical window - this enables combo continuation
            bHitCriticalWindow = true;
            
            // If this is the first attack (not in combo yet), mark combo as ready but don't advance yet
            if (!bIsInCombo)
            {
                bIsInCombo = true;
                UE_LOG(LogTemp, Warning, TEXT("🗡️ ⚡ First attack critical hit - combo ready for continuation!"));
                // DON'T advance combo chain here - wait for next attack input
                return true;
            }
            
            // If we're already in a combo chain, advance to the next attack
            UE_LOG(LogTemp, Warning, TEXT("🗡️ ⚡ Combo chain critical hit - advancing to next attack"));
            AdvanceComboChain();
            
            // Activate SwordEffect when hitting critical windows on second montage and beyond
            if (CurrentAttackIndex == 2)
            {
                UE_LOG(LogTemp, Warning, TEXT("⚡ 🌟 Critical window hit on second montage - ACTIVATING SwordEffect for third montage!"));
                ActivateSwordEffect();
            }
            
            // Show Dragons when transitioning to final attack
            if (CurrentAttackIndex == 3)
            {
                UE_LOG(LogTemp, Warning, TEXT("🐉 Critical window hit on third montage - SHOWING Dragons for final attack!"));
                ShowDragons();
            }
            
            // Chain to next attack montage if available
            if (CurrentAttackIndex < MaxComboAttacks)
            {
                UAnimMontage* NextMontage = GetCurrentAttackMontage();
                if (NextMontage && OwnerCharacter)
                {
                    // Chain to next attack with small delay
                    FTimerHandle ChainTimer;
                    GetWorld()->GetTimerManager().SetTimer(
                        ChainTimer,
                        [this, NextMontage]()
                        {
                            FaceNearestEnemy();
                            bAttackNotifyInProgress = false;
                            
                            float MontageLength = OwnerCharacter->PlayAnimMontage(NextMontage);
                            if (MontageLength > 0.0f)
                            {
                                CurrentMontageLength = MontageLength;
                                UE_LOG(LogTemp, Warning, TEXT("🗡️ ⚡ Successfully chained to next attack"));
                                
                                // Start bloom circle for the chained attack
                                GetWorld()->GetTimerManager().ClearTimer(SwordBloomHideTimer);
                                bBloomWindowActive = true;
                                if (UWBP_SwordBloom* SwordBloom = GetSwordBloomWidget()) 
                                { 
                                    SwordBloom->StartBloomCircle(); 
                                }
                                
                                // Set backup attack notify timer for chained attack
                                float NotifyTiming = MontageLength * 0.6f;
                                GetWorld()->GetTimerManager().SetTimer(
                                    AttackNotifyTimer,
                                    FTimerDelegate::CreateUObject(this, &UCombatEffectsManagerComponent::OnMeleeAttackNotify),
                                    NotifyTiming,
                                    false
                                );
                                
                                StartComboWindow();
                            }
                        },
                        0.05f,
                        false
                    );
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("🔚 Ending combo chain - resetting to first attack"));
                ResetComboChain();
            }
            
            return true;
        }
    }
    
    return false;
}

void UCombatEffectsManagerComponent::StartBloomScaling()
{
    UE_LOG(LogTemp, Warning, TEXT("📈 StartBloomScaling called - implement in Blueprint"));
}

void UCombatEffectsManagerComponent::UpdateBloomScaling()
{
    UE_LOG(LogTemp, Warning, TEXT("📈 UpdateBloomScaling called - implement in Blueprint"));
}

void UCombatEffectsManagerComponent::TriggerProgrammaticBloomEffect(float MontageLength)
{
    // CRITICAL FIX: Set component bloom window flag FIRST
    bBloomWindowActive = true;
    UE_LOG(LogTemp, Warning, TEXT("🔵 ⚡ COMPONENT BLOOM WINDOW ACTIVATED - flag set to TRUE"));
    
    if (UWBP_SwordBloom* SwordBloom = GetSwordBloomWidget()) 
    { 
        // FIXED: Use the actual montage length instead of hardcoded timing
        SwordBloom->StartBloomCircleWithDuration(MontageLength); 
        UE_LOG(LogTemp, Warning, TEXT("🔵 ✅ SwordBloom widget found - StartBloomCircleWithDuration(%.2f) called"), MontageLength); 
    } 
    else 
    { 
        UE_LOG(LogTemp, Error, TEXT("🔵 ❌ SwordBloom widget NOT found - no visual effects will show")); 
    }
    UE_LOG(LogTemp, Warning, TEXT("🎯 ⚡ Bloom effects will last for %.2f seconds (montage: %.2f)"), MontageLength, MontageLength);
}

// ========== SWORD EFFECT MANAGEMENT ==========

void UCombatEffectsManagerComponent::FindSwordEffectComponent()
{
    if (!OwnerCharacter)
    {
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🔍 FindSwordEffectComponent: Searching for SwordEffect Niagara component..."));
    
    // Try to find by name first
    SwordEffectComponent = Cast<UNiagaraComponent>(OwnerCharacter->GetDefaultSubobjectByName(TEXT("SwordEffect")));
    
    if (!SwordEffectComponent)
    {
        // Search through all components
        TArray<UNiagaraComponent*> NiagaraComponents;
        OwnerCharacter->GetComponents<UNiagaraComponent>(NiagaraComponents);
        
        for (UNiagaraComponent* Component : NiagaraComponents)
        {
            if (Component && Component->GetName().Contains(TEXT("SwordEffect")))
            {
                SwordEffectComponent = Component;
                break;
            }
        }
    }
    
    if (SwordEffectComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("✅ Found SwordEffect Niagara component by name!"));
        SwordEffectComponent->Deactivate();
        UE_LOG(LogTemp, Warning, TEXT("🔧 SwordEffect component deactivated on startup"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ SwordEffect Niagara component not found!"));
    }
}

void UCombatEffectsManagerComponent::ActivateSwordEffect()
{
    if (SwordEffectComponent)
    {
        if (!SwordEffectComponent->IsActive())
        {
            SwordEffectComponent->Activate();
            UE_LOG(LogTemp, Warning, TEXT("⚡ SwordEffect Niagara system ACTIVATED for combo chain!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Cannot activate SwordEffect - component not found!"));
        FindSwordEffectComponent();
    }
}

void UCombatEffectsManagerComponent::DeactivateSwordEffect()
{
    if (SwordEffectComponent)
    {
        if (SwordEffectComponent->IsActive())
        {
            SwordEffectComponent->Deactivate();
            UE_LOG(LogTemp, Warning, TEXT("🔒 SwordEffect Niagara system DEACTIVATED - combo ended"));
        }
    }
}

bool UCombatEffectsManagerComponent::ShouldSwordEffectBeActive() const
{
    return bIsInCombo && (CurrentAttackIndex >= 2);
}

// ========== DRAGON EFFECTS MANAGEMENT ==========

void UCombatEffectsManagerComponent::FindDragonComponents()
{
    if (!OwnerCharacter)
    {
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🔍 FindDragonComponents: Searching for Dragon skeletal mesh components..."));
    
    // Search for Dragon components by name
    TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
    OwnerCharacter->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);
    
    for (USkeletalMeshComponent* Component : SkeletalMeshComponents)
    {
        if (Component)
        {
            if (Component->GetName().Contains(TEXT("Dragon1")))
            {
                Dragon1Component = Component;
                UE_LOG(LogTemp, Warning, TEXT("✅ Found Dragon1 skeletal mesh component by name!"));
                Dragon1Component->SetVisibility(false);
                UE_LOG(LogTemp, Warning, TEXT("🔧 Dragon1 component hidden on startup"));
            }
            else if (Component->GetName().Contains(TEXT("Dragon2")))
            {
                Dragon2Component = Component;
                UE_LOG(LogTemp, Warning, TEXT("✅ Found Dragon2 skeletal mesh component by name!"));
                Dragon2Component->SetVisibility(false);
                UE_LOG(LogTemp, Warning, TEXT("🔧 Dragon2 component hidden on startup"));
            }
        }
    }
}

void UCombatEffectsManagerComponent::ShowDragons()
{
    if (Dragon1Component)
    {
        Dragon1Component->SetVisibility(true);
        UE_LOG(LogTemp, Warning, TEXT("🐉 Dragon1 skeletal mesh SHOWN - final attack active!"));
    }
    
    if (Dragon2Component)
    {
        Dragon2Component->SetVisibility(true);
        UE_LOG(LogTemp, Warning, TEXT("🐉 Dragon2 skeletal mesh SHOWN - final attack active!"));
    }
}

void UCombatEffectsManagerComponent::HideDragons()
{
    if (Dragon1Component)
    {
        Dragon1Component->SetVisibility(false);
        UE_LOG(LogTemp, Warning, TEXT("🔒 Dragon1 skeletal mesh HIDDEN - final attack ended"));
    }
    
    if (Dragon2Component)
    {
        Dragon2Component->SetVisibility(false);
        UE_LOG(LogTemp, Warning, TEXT("🔒 Dragon2 skeletal mesh HIDDEN - final attack ended"));
    }
}

bool UCombatEffectsManagerComponent::ShouldDragonsBeVisible() const
{
    return bIsInCombo && (CurrentAttackIndex >= 3);
}

// ========== INVULNERABILITY SYSTEM ==========

void UCombatEffectsManagerComponent::ApplyInvulnerabilityEffects()
{
    if (!OwnerCharacter)
    {
        return;
    }
    
    // Store original materials if not already stored
    StoreOriginalMaterials();

    // Apply invulnerability material if available
    if (OwnerCharacter->InvulnerabilityMaterial && OwnerCharacter->GetMesh())
    {
        for (int32 i = 0; i < OwnerCharacter->GetMesh()->GetNumMaterials(); i++)
        {
            OwnerCharacter->GetMesh()->SetMaterial(i, OwnerCharacter->InvulnerabilityMaterial);
        }
    }

    // Spawn invulnerability particle effect
    if (OwnerCharacter->InvulnerabilityEffect && !InvulnerabilityPSC)
    {
        InvulnerabilityPSC = UGameplayStatics::SpawnEmitterAttached(
            OwnerCharacter->InvulnerabilityEffect,
            OwnerCharacter->GetMesh(),
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            FVector(1.0f),
            EAttachLocation::SnapToTarget,
            true
        );
    }
    
    bIsInvulnerable = true;
}

void UCombatEffectsManagerComponent::RemoveInvulnerabilityEffects()
{
    if (!OwnerCharacter)
    {
        return;
    }
    
    // Restore original materials
    if (OwnerCharacter->GetMesh() && OriginalMaterials.Num() > 0)
    {
        for (int32 i = 0; i < OriginalMaterials.Num(); i++)
        {
            if (OriginalMaterials[i])
            {
                OwnerCharacter->GetMesh()->SetMaterial(i, OriginalMaterials[i]);
            }
        }
    }

    // Remove invulnerability particle effect
    if (InvulnerabilityPSC)
    {
        InvulnerabilityPSC->DestroyComponent();
        InvulnerabilityPSC = nullptr;
    }
    
    bIsInvulnerable = false;
}

void UCombatEffectsManagerComponent::ResetInvulnerability()
{
    RemoveInvulnerabilityEffects();
}

void UCombatEffectsManagerComponent::StoreOriginalMaterials()
{
    if (!OwnerCharacter || OriginalMaterials.Num() > 0)
    {
        return;
    }
    
    if (OwnerCharacter->GetMesh())
    {
        OriginalMaterials.Empty();
        TArray<UMaterialInterface*> Materials = OwnerCharacter->GetMesh()->GetMaterials();
        OriginalMaterials.Append(Materials);
    }
}

// ========== CAMERA STABILIZATION DELEGATION ==========

void UCombatEffectsManagerComponent::EnableAttackCameraStabilization()
{
    if (OwnerCharacter && OwnerCharacter->CameraStabilizationComp)
    {
        OwnerCharacter->CameraStabilizationComp->EnableAttackCameraStabilization();
    }
}

void UCombatEffectsManagerComponent::DisableAttackCameraStabilization()
{
    if (OwnerCharacter && OwnerCharacter->CameraStabilizationComp)
    {
        OwnerCharacter->CameraStabilizationComp->DisableAttackCameraStabilization();
    }
}

// ========== UTILITY FUNCTIONS ==========

UWBP_SwordBloom* UCombatEffectsManagerComponent::GetSwordBloomWidget()
{
    if (OwnerCharacter)
    {
        return OwnerCharacter->GetSwordBloomWidget();
    }
    return nullptr;
}

void UCombatEffectsManagerComponent::FaceNearestEnemy()
{
    if (!OwnerCharacter || !IsValidForCombat())
    {
        UE_LOG(LogTemp, Warning, TEXT("FaceNearestEnemy: Character is invalid or being destroyed, skipping"));
        return;
    }
    
    UWorld* World = GetWorld();
    if (!World || !IsValid(World))
    {
        UE_LOG(LogTemp, Warning, TEXT("FaceNearestEnemy: World is null or invalid, skipping"));
        return;
    }
    
    if (!OwnerCharacter->GetController() || !IsValid(OwnerCharacter->GetController()))
    {
        UE_LOG(LogTemp, Warning, TEXT("FaceNearestEnemy: Controller is null or invalid, skipping"));
        return;
    }
    
    // Find the nearest enemy within a reasonable range
    const float SearchRadius = 500.0f; // 5 meter search radius
    AActor* NearestEnemy = nullptr;
    float NearestDistance = SearchRadius;
    
    // Get all actors in the world
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
    
    FGenericTeamId MyTeamId = OwnerCharacter->GetGenericTeamId();
    FVector MyLocation = OwnerCharacter->GetActorLocation();
    
    for (AActor* Actor : AllActors)
    {
        if (!Actor || Actor == OwnerCharacter || !IsValid(Actor)) continue;
        
        // Additional safety check for actor state
        if (Actor->IsActorBeingDestroyed()) continue;
        
        // Check if this is an enemy
        bool bIsEnemy = false;
        
        // Check if actor has IGenericTeamAgentInterface
        if (Actor->GetClass() && Actor->GetClass()->ImplementsInterface(UGenericTeamAgentInterface::StaticClass()))
        {
            IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Actor);
            if (TeamAgent)
            {
                FGenericTeamId ActorTeamId = TeamAgent->GetGenericTeamId();
                if (ActorTeamId != MyTeamId && ActorTeamId.GetId() != 255) // 255 is usually neutral
                {
                    bIsEnemy = true;
                }
            }
        }
        
        // Also check by class name for ZombieCharacter
        if (!bIsEnemy && Actor->GetClass())
        {
            FString ClassName = Actor->GetClass()->GetName();
            if (ClassName.Contains(TEXT("Zombie")) || ClassName.Contains(TEXT("Enemy")))
            {
                bIsEnemy = true;
            }
        }
        
        if (bIsEnemy)
        {
            float Distance = FVector::Dist(MyLocation, Actor->GetActorLocation());
            if (Distance < NearestDistance)
            {
                NearestDistance = Distance;
                NearestEnemy = Actor;
            }
        }
    }
    
    if (NearestEnemy && IsValid(NearestEnemy))
    {
        // Calculate direction to enemy
        FVector DirectionToEnemy = (NearestEnemy->GetActorLocation() - MyLocation).GetSafeNormal();
        
        // Calculate target rotation (only yaw, keep character upright)
        FRotator TargetRotation = DirectionToEnemy.Rotation();
        TargetRotation.Pitch = 0.0f; // Keep character upright
        TargetRotation.Roll = 0.0f;  // Keep character upright
        
        // Get current rotation for logging
        FRotator CurrentRotation = OwnerCharacter->GetActorRotation();
        
        // DEBUG: Log rotation details
        UE_LOG(LogTemp, Warning, TEXT("FaceNearestEnemy DEBUG:"));
        UE_LOG(LogTemp, Warning, TEXT("  Current Rotation: P=%.2f Y=%.2f R=%.2f"), CurrentRotation.Pitch, CurrentRotation.Yaw, CurrentRotation.Roll);
        UE_LOG(LogTemp, Warning, TEXT("  Target Rotation: P=%.2f Y=%.2f R=%.2f"), TargetRotation.Pitch, TargetRotation.Yaw, TargetRotation.Roll);
        UE_LOG(LogTemp, Warning, TEXT("  Direction to Enemy: X=%.2f Y=%.2f Z=%.2f"), DirectionToEnemy.X, DirectionToEnemy.Y, DirectionToEnemy.Z);
        
        // INSTANT ROTATION for testing - no interpolation
        OwnerCharacter->SetActorRotation(TargetRotation);
        
        // Verify the rotation was applied
        FRotator NewRotation = OwnerCharacter->GetActorRotation();
        UE_LOG(LogTemp, Warning, TEXT("  Applied Rotation: P=%.2f Y=%.2f R=%.2f"), NewRotation.Pitch, NewRotation.Yaw, NewRotation.Roll);
        
        UE_LOG(LogTemp, Warning, TEXT("FaceNearestEnemy: INSTANTLY rotated character to face enemy %s at distance %.1f"), 
               *NearestEnemy->GetName(), NearestDistance);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("FaceNearestEnemy: No valid enemies found within %.1f units"), SearchRadius);
    }
}

// ========== PRIVATE HELPER FUNCTIONS ==========

bool UCombatEffectsManagerComponent::IsValidForCombat() const
{
    return OwnerCharacter && !OwnerCharacter->IsDead() && IsValid(OwnerCharacter);
}

float UCombatEffectsManagerComponent::CalculateTotalDamage() const
{
    if (!OwnerCharacter)
    {
        return 0.0f;
    }
    
    // Get base damage from character
    float BaseDamage = static_cast<float>(OwnerCharacter->CurrentDamage);
    float WeaponDamage = 0.0f;
    
    // Check if we have a weapon equipped and get its damage bonus
    if (OwnerCharacter->EquipmentSlots.Num() > 2 && OwnerCharacter->EquipmentSlots[2])
    {
        UBP_ItemInfo* WeaponItem = OwnerCharacter->EquipmentSlots[2];
        if (WeaponItem)
        {
            FStructure_ItemInfo WeaponData;
            if (UStoreSystemFix::GetItemData(WeaponItem->GetItemIndex(), WeaponData))
            {
                WeaponDamage = static_cast<float>(WeaponData.Damage);
                UE_LOG(LogTemp, Warning, TEXT("🗡️ Equipped weapon '%s' adds %f damage"), 
                       *WeaponData.ItemName, WeaponDamage);
            }
        }
    }
    
    float TotalDamage = BaseDamage + WeaponDamage;
    UE_LOG(LogTemp, Warning, TEXT("🗡️ Total calculated damage: %f (Base: %f + Weapon: %f)"), 
           TotalDamage, BaseDamage, WeaponDamage);
    
    return TotalDamage;
}

bool UCombatEffectsManagerComponent::PerformDamageDetection(float TotalDamage)
{
    if (!OwnerCharacter)
    {
        return false;
    }
    
    // Use character's location for attack detection
    FVector StartLocation = OwnerCharacter->GetActorLocation();
    
    // Track which actors we've already hit to prevent multiple hits
    TSet<AActor*> AlreadyHitActors;
    bool bSuccessfulHit = false;
    
    // Sphere overlap at player location
    TArray<FOverlapResult> OverlapResults;
    FCollisionShape SphereShape = FCollisionShape::MakeSphere(AttackRadius);
    
    bool bHitDetected = GetWorld()->OverlapMultiByChannel(
        OverlapResults,
        StartLocation,
        FQuat::Identity,
        ECollisionChannel::ECC_Pawn,
        SphereShape
    );
    
    UE_LOG(LogTemp, Warning, TEXT("🗡️ Sphere overlap found %d results"), OverlapResults.Num());
    
    for (const FOverlapResult& Result : OverlapResults)
    {
        AActor* HitActor = Result.GetActor();
        if (!HitActor || HitActor == OwnerCharacter || AlreadyHitActors.Contains(HitActor))
        {
            continue;
        }
        
        // Check if this is a valid enemy target
        if (HitActor->IsA<APawn>() && !HitActor->IsA<AAtlantisEonsCharacter>())
        {
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Checking actor: %s (Class: %s)"), 
                   *HitActor->GetName(), *HitActor->GetClass()->GetName());
            
            // Apply damage
            float ActualDamage = UGameplayStatics::ApplyPointDamage(
                HitActor,
                TotalDamage,
                StartLocation,
                FHitResult(),
                OwnerCharacter->GetController(),
                OwnerCharacter,
                UDamageType::StaticClass()
            );
            
            if (ActualDamage > 0.0f)
            {
                AlreadyHitActors.Add(HitActor);
                bSuccessfulHit = true;
                
                // Broadcast damage dealt event
                OnDamageDealt.Broadcast(ActualDamage, HitActor);
                
                UE_LOG(LogTemp, Warning, TEXT("🗡️ ✅ TakeDamage returned: %.1f"), ActualDamage);
            }
        }
    }
    
    return bSuccessfulHit;
} 