#include "DodgeComponent.h"
#include "AtlantisEonsCharacter.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/BlueprintGeneratedClass.h"

UDodgeComponent::UDodgeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UDodgeComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Get reference to owning character
    OwnerCharacter = Cast<AAtlantisEonsCharacter>(GetOwner());
    
    if (OwnerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("🚀 DodgeComponent BeginPlay: Owner = %s"), *OwnerCharacter->GetName());
        UE_LOG(LogTemp, Warning, TEXT("🚀 DodgeComponent BeginPlay: Owner class = %s"), *OwnerCharacter->GetClass()->GetName());
        UE_LOG(LogTemp, Warning, TEXT("🚀 DodgeComponent BeginPlay: Is Blueprint instance? %s"), 
               OwnerCharacter->GetClass()->IsChildOf(UBlueprintGeneratedClass::StaticClass()) ? TEXT("✅ YES") : TEXT("❌ NO"));
        
        UE_LOG(LogTemp, Warning, TEXT("✅ DodgeComponent: Successfully set OwnerCharacter = %s"), *OwnerCharacter->GetName());
        
        // Check if backward dodge montage is set in Blueprint
        if (OwnerCharacter->BackwardDashMontage) // Note: Keep the property name as is for now
        {
            UE_LOG(LogTemp, Warning, TEXT("✅ DodgeComponent: BackwardDashMontage is SET in Blueprint = %s"), *OwnerCharacter->BackwardDashMontage->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ DodgeComponent: BackwardDashMontage is NOT SET in Blueprint"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ DodgeComponent: Failed to get OwnerCharacter!"));
    }
}

void UDodgeComponent::Initialize(AAtlantisEonsCharacter* InOwnerCharacter)
{
    OwnerCharacter = InOwnerCharacter;
    UE_LOG(LogTemp, Warning, TEXT("DodgeComponent: Initialized with character %s"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("NULL"));
}

void UDodgeComponent::AttemptDodge(const FInputActionValue& Value)
{
    // 🚫 NO GetOwner() - Use only the reference set in BeginPlay
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ DodgeComponent: OwnerCharacter is NULL! Hot-reload corruption. Restart the game."));
        return;
    }
    
    if (!IsValid(OwnerCharacter))
    {
        UE_LOG(LogTemp, Error, TEXT("❌ DodgeComponent: OwnerCharacter is invalid! Hot-reload corruption. Restart the game."));
        return;
    }
    
    // 🔍 Detect hot-reload corruption by checking if reference was replaced with CDO
    FString CharacterName = OwnerCharacter->GetName();
    if (CharacterName.Contains(TEXT("CDO")) || CharacterName.Contains(TEXT("Default__")) || CharacterName.Contains(TEXT("BPGC_ARCH")))
    {
        UE_LOG(LogTemp, Error, TEXT("🚨 DodgeComponent: HOT-RELOAD CORRUPTION DETECTED! OwnerCharacter = %s"), *CharacterName);
        UE_LOG(LogTemp, Error, TEXT("💡 SOLUTION: Restart the game to get fresh Blueprint instance"));
        return;
    }
    
    // 🔥 DEBUG: Verify we're using the correct instance
    UE_LOG(LogTemp, Warning, TEXT("🎯 DodgeComponent AttemptDodge: Using OwnerCharacter = %s"), *OwnerCharacter->GetName());
    UE_LOG(LogTemp, Warning, TEXT("🎯 DodgeComponent AttemptDodge: OwnerCharacter class = %s"), *OwnerCharacter->GetClass()->GetName());

    // Check if dodge can be performed (cooldown, state, health checks)
    if (!CanPerformDodge())
    {
        return;
    }

    // Determine dodge direction based on movement input
    if (IsMovingForward())
    {
        // Player is pressing W + Dodge = Forward Dodge
        UE_LOG(LogTemp, Warning, TEXT("🏃‍♂️ FORWARD DODGE: Player pressing W + Dodge"));
        ExecuteForwardDodge();
    }
    else
    {
        // Player is moving backward or neutral = Backward Dodge (original behavior)
        UE_LOG(LogTemp, Warning, TEXT("🔙 BACKWARD DODGE: Player moving backward or neutral"));
        ExecuteBackwardDodge();
    }
}

bool UDodgeComponent::CanPerformDodge() const
{
    if (!OwnerCharacter)
    {
        return false;
    }

    // Cannot dodge if already dodging
    if (bIsDodging)
    {
        return false;
    }

    // Cannot dodge if on cooldown (unless cooldown duration is 0)
    if (bDodgeOnCooldown && DodgeCooldownDuration > 0.0f)
    {
        return false;
    }

    // Cannot dodge if character is dead
    if (OwnerCharacter->GetCurrentHealth() <= 0.0f)
    {
        return false;
    }

    return true;
}

FVector2D UDodgeComponent::GetCurrentMovementInput() const
{
    if (!OwnerCharacter)
    {
        return FVector2D::ZeroVector;
    }

    // 🚀 INSTANT RESPONSE: Use ONLY the current input, ignore velocity delay
    FVector2D InputVector = OwnerCharacter->CurrentMovementInput;
    
    // 🎯 DEBUG: Show both input and detection
    UE_LOG(LogTemp, VeryVerbose, TEXT("🎮 DodgeComponent GetInput: InputVector=%.3f,%.3f"), InputVector.X, InputVector.Y);
    
    // Return the raw input - no velocity checking, no delay
    return InputVector;
}

void UDodgeComponent::ExecuteBackwardDodge()
{
    // 🛠️ ROBUST CHECK: Ensure OwnerCharacter is valid
    if (!OwnerCharacter || !IsValid(OwnerCharacter))
    {
        UE_LOG(LogTemp, Error, TEXT("❌ DodgeComponent ExecuteBackwardDodge: OwnerCharacter is invalid!"));
        return;
    }

    // 🔥 CRITICAL: Debug the montage property at execution time
    UAnimMontage* DodgeMontage = OwnerCharacter->BackwardDashMontage; // Note: Keep property name as is for now
    UE_LOG(LogTemp, Warning, TEXT("🎭 DodgeComponent ExecuteBackwardDodge: BackwardDashMontage pointer = %p"), DodgeMontage);
    UE_LOG(LogTemp, Warning, TEXT("🎭 DodgeComponent ExecuteBackwardDodge: Character class = %s"), *OwnerCharacter->GetClass()->GetName());
    UE_LOG(LogTemp, Warning, TEXT("🎭 DodgeComponent ExecuteBackwardDodge: Is Blueprint instance? %s"), 
           OwnerCharacter->GetClass()->IsChildOf(UBlueprintGeneratedClass::StaticClass()) ? TEXT("✅ YES") : TEXT("❌ NO"));
    
    if (!DodgeMontage)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ DodgeComponent: BackwardDashMontage is not set in character Blueprint!"));
        UE_LOG(LogTemp, Error, TEXT("💡 SOLUTION: Open BP_Character Blueprint, find 'Character|Animation' section, and assign your dodge montage to 'Backward Dash Montage' property!"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("✅ DodgeComponent: Found montage = %s"), *DodgeMontage->GetName());

    // Get the character's mesh and anim instance
    USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
    if (!Mesh)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ DodgeComponent: Character mesh is null"));
        return;
    }

    UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
    if (!AnimInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ DodgeComponent: Character anim instance is null"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("✅ DodgeComponent: AnimInstance = %s"), *AnimInstance->GetClass()->GetName());

    // Set dodge state
    bIsDodging = true;
    bDodgeOnCooldown = (DodgeCooldownDuration > 0.0f); // Only set cooldown if duration > 0

    // Play the dodge montage
    float MontageLength = AnimInstance->Montage_Play(DodgeMontage);
    
    if (MontageLength > 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎭✅ DodgeComponent: Playing backward dodge montage (length: %.2f seconds)"), MontageLength);
        
        // Set up timer to reset dodge state when animation completes
        GetWorld()->GetTimerManager().SetTimer(
            DodgeCooldownTimerHandle,
            this,
            &UDodgeComponent::OnDodgeAnimationComplete,
            MontageLength,
            false
        );
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ DodgeComponent: Failed to play dodge montage"));
        // Reset state if montage failed to play
        bIsDodging = false;
        bDodgeOnCooldown = false;
    }
}

void UDodgeComponent::ExecuteForwardDodge()
{
    // 🛠️ ROBUST CHECK: Ensure OwnerCharacter is valid
    if (!OwnerCharacter || !IsValid(OwnerCharacter))
    {
        UE_LOG(LogTemp, Error, TEXT("❌ DodgeComponent ExecuteForwardDodge: OwnerCharacter is invalid!"));
        return;
    }

    // 🔥 CRITICAL: Debug the montage property at execution time
    UAnimMontage* DodgeMontage = OwnerCharacter->ForwardDashMontage; // Note: Keep property name as is for now
    UE_LOG(LogTemp, Warning, TEXT("🎭 DodgeComponent ExecuteForwardDodge: ForwardDashMontage pointer = %p"), DodgeMontage);
    UE_LOG(LogTemp, Warning, TEXT("🎭 DodgeComponent ExecuteForwardDodge: Character class = %s"), *OwnerCharacter->GetClass()->GetName());
    UE_LOG(LogTemp, Warning, TEXT("🎭 DodgeComponent ExecuteForwardDodge: Is Blueprint instance? %s"), 
           OwnerCharacter->GetClass()->IsChildOf(UBlueprintGeneratedClass::StaticClass()) ? TEXT("✅ YES") : TEXT("❌ NO"));
    
    if (!DodgeMontage)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ DodgeComponent: ForwardDashMontage is not set in character Blueprint!"));
        UE_LOG(LogTemp, Error, TEXT("💡 SOLUTION: Open BP_Character Blueprint, find 'Character|Animation' section, and assign your forward dodge montage to 'Forward Dash Montage' property!"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("✅ DodgeComponent: Found forward montage = %s"), *DodgeMontage->GetName());

    // Get the character's mesh and anim instance
    USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
    if (!Mesh)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ DodgeComponent: Character mesh is null"));
        return;
    }

    UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
    if (!AnimInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ DodgeComponent: Character anim instance is null"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("✅ DodgeComponent: AnimInstance = %s"), *AnimInstance->GetClass()->GetName());

    // Set dodge state
    bIsDodging = true;
    bDodgeOnCooldown = (DodgeCooldownDuration > 0.0f); // Only set cooldown if duration > 0

    // Play the forward dodge montage
    float MontageLength = AnimInstance->Montage_Play(DodgeMontage);
    
    if (MontageLength > 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("🏃‍♂️✅ DodgeComponent: Playing forward dodge montage (length: %.2f seconds)"), MontageLength);
        
        // Set up timer to reset dodge state when animation completes
        GetWorld()->GetTimerManager().SetTimer(
            DodgeCooldownTimerHandle,
            this,
            &UDodgeComponent::OnDodgeAnimationComplete,
            MontageLength,
            false
        );
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ DodgeComponent: Failed to play forward dodge montage"));
        // Reset state if montage failed to play
        bIsDodging = false;
        bDodgeOnCooldown = false;
    }
}

void UDodgeComponent::OnDodgeAnimationComplete()
{
    // Reset dodge state
    bIsDodging = false;
    
    // CAMERA STABILIZATION FIX: END COMBAT MODE when dodging to prevent teleport-back
    if (OwnerCharacter && OwnerCharacter->bStabilizationActive)
    {
        // COMBAT EXIT: When player dodges, they're trying to escape combat - end stabilization entirely
        OwnerCharacter->bStabilizationActive = false;
        OwnerCharacter->bPositionLocked = false;
        OwnerCharacter->LastCombatActivity = 0.0f; // Reset combat timer to end combat mode
        
        // Clear position refresh timer to prevent any pending teleport-backs
        if (OwnerCharacter->GetWorld())
        {
            OwnerCharacter->GetWorld()->GetTimerManager().ClearTimer(OwnerCharacter->PositionRefreshTimer);
        }
        
        UE_LOG(LogTemp, Warning, TEXT("🎯 DODGE ESCAPE - ENDED camera stabilization and combat mode completely. Player is now free to move!"));
    }
    
    // Start cooldown timer only if cooldown duration > 0
    if (DodgeCooldownDuration > 0.0f && GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            DodgeCooldownTimerHandle,
            this,
            &UDodgeComponent::OnDodgeCooldownComplete,
            DodgeCooldownDuration,
            false
        );
        
        UE_LOG(LogTemp, Warning, TEXT("DodgeComponent: Dodge animation complete, starting cooldown (%.2f seconds)"), DodgeCooldownDuration);
    }
    else
    {
        // No cooldown - dodge is immediately available again
        bDodgeOnCooldown = false;
        UE_LOG(LogTemp, Warning, TEXT("🚀 DodgeComponent: Dodge animation complete, NO COOLDOWN - dodge immediately available!"));
    }
}

void UDodgeComponent::OnDodgeCooldownComplete()
{
    bDodgeOnCooldown = false;
    UE_LOG(LogTemp, Warning, TEXT("DodgeComponent: Dodge cooldown complete, dodge available"));
}

bool UDodgeComponent::IsMovingForward() const
{
    FVector2D MovementInput = GetCurrentMovementInput();
    bool bIsForward = MovementInput.Y > 0.1f; // Forward movement threshold
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("🎮 Movement Check: Forward movement = %s (Y=%.3f)"), 
           bIsForward ? TEXT("YES") : TEXT("NO"), MovementInput.Y);
    
    return bIsForward;
}

bool UDodgeComponent::IsMovingBackward() const
{
    FVector2D MovementInput = GetCurrentMovementInput();
    return MovementInput.Y < -0.1f; // Backward movement threshold
}

bool UDodgeComponent::HasNoForwardBackwardInput() const
{
    FVector2D MovementInput = GetCurrentMovementInput();
    return FMath::Abs(MovementInput.Y) <= 0.1f; // No significant forward/backward input
} 