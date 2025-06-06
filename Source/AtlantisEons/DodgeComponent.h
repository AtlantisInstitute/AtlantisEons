#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "DodgeComponent.generated.h"

class UAnimMontage;
class AAtlantisEonsCharacter;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ATLANTISEONS_API UDodgeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDodgeComponent();

    // ========== DODGE SYSTEM CONFIGURATION ==========
    
    /** Dodge cooldown duration in seconds - SET TO 0 FOR NO COOLDOWN */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float DodgeCooldownDuration = 0.0f;

    /** Whether dodge is currently on cooldown */
    UPROPERTY(BlueprintReadOnly, Category = "Dodge")
    bool bDodgeOnCooldown = false;

    /** Whether the character is currently dodging */
    UPROPERTY(BlueprintReadOnly, Category = "Dodge")
    bool bIsDodging = false;

    // ========== DODGE SYSTEM FUNCTIONS ==========
    
    /** Initialize the dodge component with owner character reference */
    UFUNCTION(BlueprintCallable, Category = "Dodge")
    void Initialize(AAtlantisEonsCharacter* InOwnerCharacter);

    /** Attempt to perform a dodge based on current movement input */
    UFUNCTION(BlueprintCallable, Category = "Dodge")
    void AttemptDodge(const FInputActionValue& Value);

    /** Check if dodge can be performed based on movement conditions */
    UFUNCTION(BlueprintPure, Category = "Dodge")
    bool CanPerformDodge() const;

    /** Get the current movement input for dodge condition checking */
    UFUNCTION(BlueprintPure, Category = "Dodge")
    FVector2D GetCurrentMovementInput() const;

    /** Check if the character is currently dodging */
    UFUNCTION(BlueprintPure, Category = "Dodge")
    bool IsDodging() const { return bIsDodging; }

protected:
    virtual void BeginPlay() override;

private:
    /** Reference to the owning character - UPROPERTY prevents garbage collection */
    UPROPERTY(Transient)
    AAtlantisEonsCharacter* OwnerCharacter;

    /** Timer handle for dodge cooldown */
    FTimerHandle DodgeCooldownTimerHandle;

    /** Execute the backward dodge */
    void ExecuteBackwardDodge();

    /** Execute the forward dodge */
    void ExecuteForwardDodge();

    /** Called when dodge cooldown expires */
    UFUNCTION()
    void OnDodgeCooldownComplete();

    /** Called when dodge animation completes */
    UFUNCTION()
    void OnDodgeAnimationComplete();

    /** Check if player is moving forward (Y > 0.1) */
    bool IsMovingForward() const;

    /** Check if player is moving backward (Y < -0.1) */
    bool IsMovingBackward() const;

    /** Check if player has no significant forward/backward input */
    bool HasNoForwardBackwardInput() const;
}; 