#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "WBP_ZombieHealthBar.generated.h"

/**
 * Widget class for displaying zombie health bar
 * This can be inherited by Blueprint widgets to create visual health bars
 */
UCLASS(BlueprintType, Blueprintable)
class ATLANTISEONS_API UWBP_ZombieHealthBar : public UUserWidget
{
    GENERATED_BODY()

public:
    UWBP_ZombieHealthBar(const FObjectInitializer& ObjectInitializer);

    /** Update the health bar with current health values */
    UFUNCTION(BlueprintCallable, Category = "Health Bar")
    void UpdateHealth(float CurrentHealth, float MaxHealth, float HealthPercentage);

    /** Set the health percentage directly (0.0 to 1.0) */
    UFUNCTION(BlueprintCallable, Category = "Health Bar")
    void SetHealthPercentage(float Percentage);

    /** Get current health percentage */
    UFUNCTION(BlueprintPure, Category = "Health Bar")
    float GetHealthPercentage() const { return HealthPercentage; }

    /** Get current health value */
    UFUNCTION(BlueprintPure, Category = "Health Bar")
    float GetCurrentHealth() const { return CurrentHealth; }

    /** Get max health value */
    UFUNCTION(BlueprintPure, Category = "Health Bar")
    float GetMaxHealth() const { return MaxHealth; }

protected:
    virtual void NativeConstruct() override;
    virtual void NativePreConstruct() override;

    /** Health bar progress bar widget (can be bound in Blueprint) */
    UPROPERTY(meta = (BindWidget))
    class UProgressBar* ZombieHealthBar;

    /** Current health percentage (0.0 to 1.0) */
    UPROPERTY(BlueprintReadOnly, Category = "Health Bar")
    float HealthPercentage;

    /** Current health value */
    UPROPERTY(BlueprintReadOnly, Category = "Health Bar")
    float CurrentHealth;

    /** Maximum health value */
    UPROPERTY(BlueprintReadOnly, Category = "Health Bar")
    float MaxHealth;

    /** Health bar color when healthy */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar|Appearance")
    FLinearColor HealthyColor;

    /** Health bar color when damaged */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar|Appearance")
    FLinearColor DamagedColor;

    /** Health bar color when critically low */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar|Appearance")
    FLinearColor CriticalColor;

private:
    /** Update the visual appearance of the health bar */
    void UpdateHealthBarAppearance();

    /** Get the appropriate color based on health percentage */
    FLinearColor GetHealthBarColor() const;
}; 