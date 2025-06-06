#include "WBP_ZombieHealthBar.h"
#include "Components/ProgressBar.h"
#include "Engine/Engine.h"

UWBP_ZombieHealthBar::UWBP_ZombieHealthBar(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Initialize default values
    HealthPercentage = 1.0f;
    CurrentHealth = 100.0f;
    MaxHealth = 100.0f;

    // Set default colors
    HealthyColor = FLinearColor::Red;
    DamagedColor = FLinearColor::Yellow;
    CriticalColor = FLinearColor::Red;

    UE_LOG(LogTemp, Warning, TEXT("WBP_ZombieHealthBar: Constructor called"));
}

void UWBP_ZombieHealthBar::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogTemp, Warning, TEXT("WBP_ZombieHealthBar: NativeConstruct called"));

    // Initialize the health bar appearance
    UpdateHealthBarAppearance();
}

void UWBP_ZombieHealthBar::NativePreConstruct()
{
    Super::NativePreConstruct();

    UE_LOG(LogTemp, Warning, TEXT("WBP_ZombieHealthBar: NativePreConstruct called"));

    // Update appearance in design time as well
    UpdateHealthBarAppearance();
}

void UWBP_ZombieHealthBar::UpdateHealth(float InCurrentHealth, float InMaxHealth, float InHealthPercentage)
{
    // Update health values
    CurrentHealth = InCurrentHealth;
    MaxHealth = InMaxHealth;
    HealthPercentage = FMath::Clamp(InHealthPercentage, 0.0f, 1.0f);

    UE_LOG(LogTemp, Warning, TEXT("WBP_ZombieHealthBar: UpdateHealth called - Current: %.1f, Max: %.1f, Percentage: %.2f"), 
           CurrentHealth, MaxHealth, HealthPercentage);

    // Update visual appearance
    UpdateHealthBarAppearance();
}

void UWBP_ZombieHealthBar::SetHealthPercentage(float Percentage)
{
    HealthPercentage = FMath::Clamp(Percentage, 0.0f, 1.0f);
    
    // Calculate current health based on percentage
    CurrentHealth = MaxHealth * HealthPercentage;

    UE_LOG(LogTemp, Warning, TEXT("WBP_ZombieHealthBar: SetHealthPercentage called - Percentage: %.2f, Calculated Health: %.1f"), 
           HealthPercentage, CurrentHealth);

    // Update visual appearance
    UpdateHealthBarAppearance();
}

void UWBP_ZombieHealthBar::UpdateHealthBarAppearance()
{
    // Update progress bar if available
    if (ZombieHealthBar)
    {
        // Ensure the percentage is properly clamped and calculated
        float ClampedPercentage = FMath::Clamp(HealthPercentage, 0.0f, 1.0f);
        ZombieHealthBar->SetPercent(ClampedPercentage);
        
        // Set color based on health percentage
        FLinearColor BarColor = GetHealthBarColor();
        ZombieHealthBar->SetFillColorAndOpacity(BarColor);

        UE_LOG(LogTemp, Warning, TEXT("WBP_ZombieHealthBar: Updated progress bar - Percentage: %.2f, Color: R=%.2f G=%.2f B=%.2f"), 
               ClampedPercentage, BarColor.R, BarColor.G, BarColor.B);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("WBP_ZombieHealthBar: ZombieHealthBar is NULL - make sure to bind it in Blueprint"));
    }
}

FLinearColor UWBP_ZombieHealthBar::GetHealthBarColor() const
{
    // Return color based on health percentage
    if (HealthPercentage > 0.6f)
    {
        // Healthy - use green
        return HealthyColor;
    }
    else if (HealthPercentage > 0.3f)
    {
        // Damaged - interpolate between yellow and green
        float Alpha = (HealthPercentage - 0.3f) / 0.3f; // 0-1 range for interpolation
        return FLinearColor::LerpUsingHSV(DamagedColor, HealthyColor, Alpha);
    }
    else
    {
        // Critical - interpolate between red and yellow
        if (HealthPercentage > 0.0f)
        {
            float Alpha = HealthPercentage / 0.3f; // 0-1 range for interpolation
            return FLinearColor::LerpUsingHSV(CriticalColor, DamagedColor, Alpha);
        }
        else
        {
            // Dead - pure red
            return CriticalColor;
        }
    }
} 