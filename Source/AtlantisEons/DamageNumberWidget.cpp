#include "DamageNumberWidget.h"
#include "Components/TextBlock.h"

UDamageNumberWidget::UDamageNumberWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Initialize defaults
    DamageAmount = 0.0f;
    bIsPlayerDamage = false;
    AnimationDuration = 1.5f;
    VerticalSpeed = 50.0f;
    CurrentLifetime = 0.0f;
    bIsAnimating = false;
}

void UDamageNumberWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Clear timer if one exists
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(RemoveTimerHandle);
    }
    
    // Only log the message if this is the first time we're building
    if (!DamageText)
    {
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberWidget: TextBlock not found. Your Blueprint must have a TextBlock named 'DamageText'"));
    }
}

void UDamageNumberWidget::NativeDestruct()
{
    // Clear timer to prevent crashes
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(RemoveTimerHandle);
    }
    
    Super::NativeDestruct();
}

void UDamageNumberWidget::InitializeDamageNumber(float InDamageAmount, bool bInIsPlayerDamage)
{
    // Store damage values
    DamageAmount = InDamageAmount;
    bIsPlayerDamage = bInIsPlayerDamage;
    
    // Reset animation state
    CurrentLifetime = 0.0f;
    bIsAnimating = true;
    
    // Safety check for damage text
    if (!DamageText || !IsValid(DamageText))
    {
        UE_LOG(LogTemp, Error, TEXT("DamageNumberWidget: Cannot initialize - TextBlock is null"));
        RemoveSelf(); // Remove the widget since it's unusable
        return;
    }
    
    // Format and set number text
    FText FormattedNumber = FText::AsNumber(FMath::RoundToInt(DamageAmount));
    DamageText->SetText(FormattedNumber);
    
    // Set color based on damage type (red for player, gold for enemy)
    FLinearColor TextColor = bIsPlayerDamage ? FLinearColor(1.0f, 0.2f, 0.2f) : FLinearColor(1.0f, 0.8f, 0.2f);
    DamageText->SetColorAndOpacity(TextColor);
    
    // Make sure text is visible
    DamageText->SetVisibility(ESlateVisibility::HitTestInvisible);
    DamageText->SetRenderOpacity(1.0f);
    
    // Set timer for auto-removal
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(RemoveTimerHandle);
        GetWorld()->GetTimerManager().SetTimer(RemoveTimerHandle, this, &UDamageNumberWidget::RemoveSelf, AnimationDuration, false);
    }
}

void UDamageNumberWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    
    // Skip animation if not active or text is invalid
    if (!bIsAnimating || !DamageText || !IsValid(DamageText))
    {
        return;
    }
    
    // Update animation time
    CurrentLifetime += InDeltaTime;
    float Progress = FMath::Clamp(CurrentLifetime / AnimationDuration, 0.0f, 1.0f);
    
    // Float upward with time
    float VerticalOffset = -Progress * VerticalSpeed;
    SetRenderTranslation(FVector2D(0, VerticalOffset));
    
    // Fade out near the end
    if (Progress > 0.7f)
    {
        float Alpha = 1.0f - ((Progress - 0.7f) / 0.3f);
        DamageText->SetRenderOpacity(Alpha);
    }
    
    // Check if animation is complete
    if (Progress >= 1.0f)
    {
        bIsAnimating = false;
        RemoveSelf();
    }
}

void UDamageNumberWidget::RemoveSelf()
{
    // Only remove if we're still valid
    if (IsValid(this))
    {
        // Clear timer first
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(RemoveTimerHandle);
        }
        
        // Remove widget
        RemoveFromParent();
    }
}
