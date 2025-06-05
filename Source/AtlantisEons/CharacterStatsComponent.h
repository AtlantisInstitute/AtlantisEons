#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CharacterStatsComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStatsChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChangedSignature, float, NewHealthPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnManaChangedSignature, float, NewManaPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDamageTakenSignature, float, DamageAmount, bool, bIsAlive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterDeathSignature);

/**
 * Component responsible for managing character stats like Health, Mana, STR, DEX, INT, etc.
 * This separates stat management from the main character class for better organization.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ATLANTISEONS_API UCharacterStatsComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UCharacterStatsComponent();

    // Core stat events
    UPROPERTY(BlueprintAssignable, Category = "Stats")
    FOnStatsChangedSignature OnStatsChanged;

    UPROPERTY(BlueprintAssignable, Category = "Stats")
    FOnHealthChangedSignature OnHealthChanged;

    UPROPERTY(BlueprintAssignable, Category = "Stats")
    FOnManaChangedSignature OnManaChanged;

    // Enhanced damage system events
    UPROPERTY(BlueprintAssignable, Category = "Damage")
    FOnDamageTakenSignature OnDamageTaken;

    UPROPERTY(BlueprintAssignable, Category = "Damage")
    FOnCharacterDeathSignature OnCharacterDeath;

    // Base stats (set in editor/defaults)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
    float BaseHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
    float BaseMovementSpeed = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
    int32 BaseSTR = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
    int32 BaseDEX = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
    int32 BaseINT = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
    int32 BaseDefence = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
    int32 BaseDamage = 10;

    // Current stats (runtime values)
    UPROPERTY(BlueprintReadOnly, Category = "Current Stats")
    float CurrentHealth;

    UPROPERTY(BlueprintReadOnly, Category = "Current Stats")
    float MaxHealth;

    UPROPERTY(BlueprintReadOnly, Category = "Current Stats")
    int32 CurrentMP = 100;

    UPROPERTY(BlueprintReadOnly, Category = "Current Stats")
    int32 MaxMP = 100;

    UPROPERTY(BlueprintReadOnly, Category = "Current Stats")
    int32 CurrentSTR;

    UPROPERTY(BlueprintReadOnly, Category = "Current Stats")
    int32 CurrentDEX;

    UPROPERTY(BlueprintReadOnly, Category = "Current Stats")
    int32 CurrentINT;

    UPROPERTY(BlueprintReadOnly, Category = "Current Stats")
    int32 CurrentDefence;

    UPROPERTY(BlueprintReadOnly, Category = "Current Stats")
    int32 CurrentDamage;

    // Gold and currency
    UPROPERTY(BlueprintReadOnly, Category = "Currency")
    int32 Gold = 0;

    // Stat modification functions
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void InitializeStats();

    UFUNCTION(BlueprintCallable, Category = "Stats")
    void UpdateAllStats();

    // Health functions
    UFUNCTION(BlueprintCallable, Category = "Health")
    void RecoverHealth(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Health")
    void TakeDamage(float DamageAmount);

    UFUNCTION(BlueprintCallable, Category = "Health")
    bool IsAlive() const { return CurrentHealth > 0.0f; }

    UFUNCTION(BlueprintPure, Category = "Health")
    float GetHealthPercent() const;

    // Mana functions
    UFUNCTION(BlueprintCallable, Category = "Mana")
    void RecoverMana(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Mana")
    void ConsumeMana(int32 Amount);

    UFUNCTION(BlueprintPure, Category = "Mana")
    float GetManaPercent() const;

    // Stat getters
    UFUNCTION(BlueprintPure, Category = "Stats")
    float GetCurrentHealth() const { return CurrentHealth; }

    UFUNCTION(BlueprintPure, Category = "Stats")
    float GetMaxHealth() const { return MaxHealth; }

    UFUNCTION(BlueprintPure, Category = "Stats")
    int32 GetCurrentMP() const { return CurrentMP; }

    UFUNCTION(BlueprintPure, Category = "Stats")
    int32 GetMaxMP() const { return MaxMP; }

    UFUNCTION(BlueprintPure, Category = "Stats")
    int32 GetCurrentSTR() const { return CurrentSTR; }

    UFUNCTION(BlueprintPure, Category = "Stats")
    int32 GetCurrentDEX() const { return CurrentDEX; }

    UFUNCTION(BlueprintPure, Category = "Stats")
    int32 GetCurrentINT() const { return CurrentINT; }

    UFUNCTION(BlueprintPure, Category = "Stats")
    int32 GetCurrentDefence() const { return CurrentDefence; }

    UFUNCTION(BlueprintPure, Category = "Stats")
    int32 GetCurrentDamage() const { return CurrentDamage; }

    UFUNCTION(BlueprintPure, Category = "Stats")
    int32 GetGold() const { return Gold; }

    // Stat modification (for equipment bonuses)
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void AddStatModifier(int32 STRBonus, int32 DEXBonus, int32 INTBonus, int32 DefenceBonus, int32 DamageBonus, int32 HPBonus, int32 MPBonus);

    UFUNCTION(BlueprintCallable, Category = "Stats")
    void RemoveStatModifier(int32 STRBonus, int32 DEXBonus, int32 INTBonus, int32 DefenceBonus, int32 DamageBonus, int32 HPBonus, int32 MPBonus);

    // Currency functions
    UFUNCTION(BlueprintCallable, Category = "Currency")
    bool SpendGold(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Currency")
    void AddGold(int32 Amount);

    // Enhanced damage system functions
    UFUNCTION(BlueprintCallable, Category = "Damage")
    void ApplyDamageAdvanced(float DamageAmount, AActor* DamageSource = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Damage")
    bool CheckForDeath();

    UFUNCTION(BlueprintPure, Category = "Damage")
    bool IsDead() const { return bIsDead; }

    UFUNCTION(BlueprintCallable, Category = "Damage")
    void ResetDeathState();

    UFUNCTION(BlueprintCallable, Category = "Damage")
    void ReviveCharacter();

protected:
    virtual void BeginPlay() override;

private:
    // Helper function to clamp health/mana values
    void ClampHealthMana();

    // Helper function to broadcast stat changes
    void BroadcastStatChanges();

    // Death state tracking
    UPROPERTY()
    bool bIsDead = false;

    // UI update helpers
    void NotifyUIUpdate();
}; 