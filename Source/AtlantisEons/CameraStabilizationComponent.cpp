#include "CameraStabilizationComponent.h"
#include "AtlantisEonsCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

UCameraStabilizationComponent::UCameraStabilizationComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // We'll handle updates manually
    bWantsInitializeComponent = true;
}

void UCameraStabilizationComponent::BeginPlay()
{
    Super::BeginPlay();
    CacheComponentReferences();
}

void UCameraStabilizationComponent::CacheComponentReferences()
{
    OwnerCharacter = Cast<AAtlantisEonsCharacter>(GetOwner());
    if (OwnerCharacter)
    {
        CameraBoom = OwnerCharacter->GetCameraBoom();
        
        // Initialize state
        bStabilizationActive = false;
        bPositionLocked = false;
        bCameraRotationLocked = false;
        LastCombatActivity = 0.0f;
        bInCombatMode = false;
    }
}

void UCameraStabilizationComponent::EnableAttackCameraStabilization()
{
    if (!OwnerCharacter || !CameraBoom)
    {
        return;
    }

    // Lock current position and rotation
    LockedPosition = OwnerCharacter->GetActorLocation();
    LockedCameraRotation = CameraBoom->GetComponentRotation();
    
    // Store initial values for drift detection
    InitialLockedPosition = LockedPosition;
    InitialLockedCameraRotation = LockedCameraRotation;
    
    // Set AGGRESSIVE state flags - restore harsh settings
    bPositionLocked = true;
    bCameraRotationLocked = !bAllowRotationDuringAttacks;
    bStabilizationActive = true;
    bSuppressAttackRootMotion = true;
    
    // Record stabilization start time
    StabilizationStartTime = GetWorld()->GetTimeSeconds();
    
    // Mark combat activity
    LastCombatActivity = GetWorld()->GetTimeSeconds();
    if (bCombatModeStabilization)
    {
        bInCombatMode = true;
    }
    
    // NOTE: Position refresh timer DISABLED to prevent teleporting
    // The aggressive stabilization will handle corrections without refreshing
    
    // Store smoothing variables
    LastStabilizedPosition = LockedPosition;
    
    UE_LOG(LogTemp, Warning, TEXT("AGGRESSIVE Camera Stabilization ENABLED - Position: %s, Rotation: %s"), 
           *LockedPosition.ToString(), *LockedCameraRotation.ToString());
}

void UCameraStabilizationComponent::DisableAttackCameraStabilization()
{
    if (!OwnerCharacter)
    {
        return;
    }

    // MUCH MORE PERMISSIVE DISABLE - Allow immediate disable in most cases
    float CurrentTime = GetWorld()->GetTimeSeconds();
    float StabilizationDuration = CurrentTime - StabilizationStartTime;
    
    // Only block disable in very specific cases (much more permissive)
    bool bShouldBlockDisable = false;
    
    // Only block if we're in a very short attack animation (less than 0.5 seconds)
    if (bPersistentStabilizationMode && StabilizationDuration < 0.5f)
    {
        bShouldBlockDisable = true;
        UE_LOG(LogTemp, Warning, TEXT("Stabilization disable blocked - Too early: %.2f < 0.5s"), 
               StabilizationDuration);
    }
    
    // If player is actively trying to move, ALWAYS allow disable
    if (bIsPlayerTryingToMove)
    {
        bShouldBlockDisable = false;
        UE_LOG(LogTemp, Warning, TEXT("Stabilization disable FORCED - Player is actively moving"));
    }
    
    if (bShouldBlockDisable)
    {
        return;
    }
    
    // Clear state flags - ALWAYS disable when called (unless blocked above)
    bPositionLocked = false;
    bCameraRotationLocked = false;
    bStabilizationActive = false;
    bSuppressAttackRootMotion = false;
    bInCombatMode = false;
    
    // Clear any active timers
    if (PositionRefreshTimer.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(PositionRefreshTimer);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("CAMERA STABILIZATION DISABLED after %.2f seconds"), 
           StabilizationDuration);
}

bool UCameraStabilizationComponent::IsNearEnemies() const
{
    if (!OwnerCharacter || !bUseProximityStabilization)
    {
        return false;
    }
    
    // Simple proximity check - in a real game you'd check for actual enemies
    // For now, we'll use a basic distance check
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), FoundActors);
    
    FVector CharacterLocation = OwnerCharacter->GetActorLocation();
    
    for (AActor* Actor : FoundActors)
    {
        if (Actor != OwnerCharacter)
        {
            float Distance = FVector::Dist(CharacterLocation, Actor->GetActorLocation());
            if (Distance <= ProximityStabilizationDistance)
            {
                return true;
            }
        }
    }
    
    return false;
}

void UCameraStabilizationComponent::UpdateCameraStabilization(float DeltaTime)
{
    if (!OwnerCharacter || !CameraBoom || !ShouldStabilize())
    {
        return;
    }
    
    // Apply position stabilization
    ApplyPositionStabilization(DeltaTime);
    
    // Apply rotation stabilization if enabled
    if (bCameraRotationLocked)
    {
        ApplyRotationStabilization(DeltaTime);
    }
}

void UCameraStabilizationComponent::SetMovementInput(const FVector2D& MovementInput)
{
    CurrentMovementInput = MovementInput;
    // Legacy function - now that we use direct input detection, this is simplified
    const float LegacyMovementThreshold = 0.1f; // Minimum input magnitude for legacy compatibility
    bIsPlayerTryingToMove = MovementInput.Size() > LegacyMovementThreshold;
}

void UCameraStabilizationComponent::ApplyPositionStabilization(float DeltaTime)
{
    if (!bPositionLocked || !OwnerCharacter)
    {
        return;
    }
    
    FVector CurrentPosition = OwnerCharacter->GetActorLocation();
    FVector TargetPosition = LockedPosition;
    
    // Allow vertical movement if configured
    if (bAllowVerticalMovementDuringAttacks)
    {
        TargetPosition.Z = CurrentPosition.Z;
    }
    
    // Check if we need to apply stabilization
    float DistanceFromLocked = FVector::Dist(CurrentPosition, TargetPosition);
    
    // Breakable stabilization check - Use direct input detection for immediate response
    if (bAllowBreakableStabilization && OwnerCharacter->IsAnyMovementKeyPressed())
    {
        // Player is actively pressing movement or dash keys - respond immediately
        if (bDisableStabilizationOnInput)
        {
            // Completely disable stabilization when any movement/dash key is pressed
            UE_LOG(LogTemp, Verbose, TEXT("Camera stabilization DISABLED - movement/dash key pressed"));
            return;
        }
        
        // Use very minimal stabilization when movement keys are pressed
        if (DistanceFromLocked > StabilizationThreshold * 3.0f) // Much more permissive threshold
        {
            FVector StabilizedPosition = FMath::VInterpTo(
                CurrentPosition, 
                TargetPosition, 
                DeltaTime, 
                StabilizationLerpSpeed * MovementInputStabilizationStrength
            );
            OwnerCharacter->SetActorLocation(StabilizedPosition);
        }
        return;
    }
    
    // Ultra-maximum mode check - Strongest possible stabilization without teleporting
    if (bUseUltraStabilizationMode && DistanceFromLocked > UltraStabilizationThreshold)
    {
        // Use maximum ultra mode with fastest possible smooth interpolation
        float EffectiveStrength = ExtendedSequenceStabilizationStrength;
        FVector StabilizedPosition = FMath::VInterpTo(
            CurrentPosition, 
            TargetPosition, 
            DeltaTime, 
            StabilizationLerpSpeed * EffectiveStrength * 5.0f // Maximum 5x speed for ultra mode
        );
        OwnerCharacter->SetActorLocation(StabilizedPosition);
        UE_LOG(LogTemp, Verbose, TEXT("Ultra-maximum stabilization applied - Distance: %.3f"), DistanceFromLocked);
        return;
    }
    
    // Standard maximum stabilization
    if (DistanceFromLocked > StabilizationThreshold)
    {
        // Check position tolerance for micro-adjustments - Apply stabilization for ANY detectable movement
        if (DistanceFromLocked > PositionTolerance)
        {
            float EffectiveStrength = CameraStabilizationStrength;
            
            // Proximity-based stabilization - Always use maximum strength when near enemies
            if (bUseProximityStabilization && IsNearEnemies())
            {
                EffectiveStrength = FMath::Max(EffectiveStrength, ProximityStabilizationStrength);
            }
            
            // Use maximum speed for strongest possible stabilization
            FVector StabilizedPosition = FMath::VInterpTo(
                CurrentPosition, 
                TargetPosition, 
                DeltaTime, 
                StabilizationLerpSpeed * EffectiveStrength * 2.0f // Double speed for maximum mode
            );
            
            OwnerCharacter->SetActorLocation(StabilizedPosition);
            LastStabilizedPosition = StabilizedPosition;
            UE_LOG(LogTemp, Verbose, TEXT("Maximum stabilization applied - Distance: %.3f, Strength: %.3f"), 
                   DistanceFromLocked, EffectiveStrength);
        }
    }
}

void UCameraStabilizationComponent::ApplyRotationStabilization(float DeltaTime)
{
    if (!bCameraRotationLocked || !CameraBoom)
    {
        return;
    }
    
    FRotator CurrentRotation = CameraBoom->GetComponentRotation();
    FRotator TargetRotation = LockedCameraRotation;
    
    // Check if rotation difference is significant
    float RotationDifference = FMath::Abs(FRotator::NormalizeAxis(CurrentRotation.Yaw - TargetRotation.Yaw)) +
                              FMath::Abs(FRotator::NormalizeAxis(CurrentRotation.Pitch - TargetRotation.Pitch));
    
    if (RotationDifference > RotationTolerance)
    {
        FRotator StabilizedRotation = FMath::RInterpTo(
            CurrentRotation,
            TargetRotation,
            DeltaTime,
            StabilizationLerpSpeed * CameraRotationStabilizationStrength
        );
        
        CameraBoom->SetWorldRotation(StabilizedRotation);
    }
}

bool UCameraStabilizationComponent::ShouldStabilize() const
{
    if (!bStabilizationActive || !OwnerCharacter)
    {
        return false;
    }
    
    // Check if we're in an attack animation
    if (OwnerCharacter->bIsAttacking)
    {
        return true;
    }
    
    // Check combat mode - extended stabilization during combat sequences
    if (bInCombatMode && bCombatModeStabilization)
    {
        float TimeSinceLastCombat = GetWorld()->GetTimeSeconds() - LastCombatActivity;
        if (TimeSinceLastCombat < CombatModeStabilizationDuration)
        {
            return true;
        }
    }
    
    // Check proximity stabilization
    if (bUseProximityStabilization && IsNearEnemies())
    {
        return true;
    }
    
    return false;
}

void UCameraStabilizationComponent::RefreshLockedPosition()
{
    // DISABLED: This function was causing teleporting issues
    // Simply do nothing to prevent any position refreshing
    return;
} 