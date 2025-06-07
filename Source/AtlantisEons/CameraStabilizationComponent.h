#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "CameraStabilizationComponent.generated.h"

class AAtlantisEonsCharacter;
class USpringArmComponent;

/**
 * Handles all camera stabilization functionality for the AtlantisEons Character
 * Manages ultra-aggressive camera stabilization during attacks to prevent movement
 * This component reduces the main character class size while preserving functionality
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ATLANTISEONS_API UCameraStabilizationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCameraStabilizationComponent();

    // ========== MAIN STABILIZATION CONTROL ==========
    
    /** Enable attack camera stabilization */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void EnableAttackCameraStabilization();
    
    /** Disable attack camera stabilization */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void DisableAttackCameraStabilization();
    
    /** Check if character is near enemies (for proximity stabilization) */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    bool IsNearEnemies() const;

    // ========== STABILIZATION CONFIGURATION ==========
    
    // MAXIMUM: Strongest possible stabilization variables for rock-solid camera control during attacks
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CameraStabilizationStrength = 0.99f; // MAXIMUM: Near-perfect stabilization strength (original working level)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float StabilizationThreshold = 0.1f; // MAXIMUM: Extremely tight threshold for instant correction (original working level)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.1", ClampMax = "100.0"))
    float StabilizationLerpSpeed = 100.0f; // MAXIMUM: Extremely fast correction speed (original working level)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    bool bAllowVerticalMovementDuringAttacks = true; // Allow Z-axis movement during attacks
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    bool bAllowRotationDuringAttacks = false; // Prevent camera rotation during attacks
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CameraRotationStabilizationStrength = 0.999f; // MAXIMUM: Near-perfect rotation stabilization
    
    // BREAKABLE STABILIZATION: Allow player to override stabilization with direct input detection
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    bool bAllowBreakableStabilization = true; // Enable input-based stabilization override
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MovementInputStabilizationStrength = 0.1f; // MINIMAL - Very weak stabilization when movement keys are pressed
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    bool bDisableStabilizationOnInput = true; // Completely disable stabilization when movement/dash keys are pressed
    
    // ULTRA-AGGRESSIVE: Extended sequence stabilization to prevent cumulative drift
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float PositionRefreshInterval = 1.0f; // ULTRA-AGGRESSIVE: Refresh locked position every 1 second
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float MaxCumulativeDrift = 0.8f; // ULTRA-AGGRESSIVE: Much tighter drift tolerance
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ExtendedSequenceStabilizationStrength = 0.999f; // MAXIMUM: Near-perfect stabilization for extended sequences
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PositionTolerance = 0.01f; // MAXIMUM: Ultra-tight tolerance to prevent any micro-drift
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float RotationTolerance = 0.01f; // ORIGINAL: Tight but stable rotation tolerance (original working level)
    
    // Attack-specific stabilization timing
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float StabilizationStartDelay = 0.0f; // No delay - instant activation to prevent any frame distortions
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float StabilizationEndEarly = 0.2f; // End stabilization before attack animation ends
    
    // MAXIMUM STABILIZATION PARAMETERS
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float MaxStabilizationDistance = 1.0f; // MAXIMUM: Tight distance for immediate correction
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    bool bUseHardSnapForLargeMovements = false; // Keep disabled to prevent teleporting
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    bool bUseUltraStabilizationMode = true; // Enabled for maximum camera stability during attacks
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float UltraStabilizationThreshold = 0.8f; // ORIGINAL: Triggers much earlier (original working level)
    
    // Proximity-based stabilization for enemy interactions
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    bool bUseProximityStabilization = false; // Disabled - was too restrictive for normal movement
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "50.0", ClampMax = "500.0"))
    float ProximityStabilizationDistance = 200.0f; // Distance to enemies that triggers stabilization
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ProximityStabilizationStrength = 0.95f; // MAXIMUM: Strong proximity stabilization

    // ========== STATE VARIABLES ==========
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    bool bSuppressAttackRootMotion = false;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    FRotator LockedCameraRotation = FRotator::ZeroRotator;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    bool bCameraRotationLocked = false;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    FVector LockedPosition = FVector::ZeroVector;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    bool bPositionLocked = false;
    
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

    // ========== TICK AND UPDATE ==========
    
    /** Update camera stabilization every frame */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void UpdateCameraStabilization(float DeltaTime);
    
    /** Set current movement input for breakable stabilization */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void SetMovementInput(const FVector2D& MovementInput);
    
    /** Get the current locked position for direct character access */
    UFUNCTION(BlueprintPure, Category = "Camera")
    FVector GetLockedPosition() const { return LockedPosition; }
    
    /** Check if stabilization should be active based on current conditions (public for character access) */
    UFUNCTION(BlueprintPure, Category = "Camera")
    bool ShouldStabilize() const;

protected:
    virtual void BeginPlay() override;

private:
    // ========== CACHED REFERENCES ==========
    
    /** Reference to the owning character */
    UPROPERTY()
    AAtlantisEonsCharacter* OwnerCharacter;

    /** Cached reference to camera boom */
    UPROPERTY()
    USpringArmComponent* CameraBoom;

    // ========== TIMER HANDLES ==========
    
    FTimerHandle StabilizationDelayTimer;
    FTimerHandle PositionRefreshTimer;

    // ========== HELPER FUNCTIONS ==========
    
    /** Cache component references */
    void CacheComponentReferences();
    
    /** Apply stabilization to character position */
    void ApplyPositionStabilization(float DeltaTime);
    
    /** Apply stabilization to camera rotation */
    void ApplyRotationStabilization(float DeltaTime);
    
    /** Refresh locked position during extended sequences */
    UFUNCTION()
    void RefreshLockedPosition();
}; 