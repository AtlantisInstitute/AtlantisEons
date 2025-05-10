#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamageNumberWidget.generated.h"

class UTextBlock;

/**
 * Widget for displaying floating damage numbers in the world
 */
UCLASS()
class ATLANTISEONS_API UDamageNumberWidget : public UUserWidget
{
    GENERATED_BODY()
    
public:
    // Constructor
    UDamageNumberWidget(const FObjectInitializer& ObjectInitializer);
    
    // Initialize the widget with damage amount and whether it's player damage
    UFUNCTION(BlueprintCallable, Category = "Damage")
    void InitializeDamageNumber(float InDamageAmount, bool bInIsPlayerDamage);
    
protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual void NativeDestruct() override;
    
    // Damage properties
    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    float DamageAmount;
    
    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    bool bIsPlayerDamage;
    
    // Animation settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float AnimationDuration;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float VerticalSpeed;
    
    // The text block that displays the damage amount
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* DamageText;
    
private:
    // Animation state
    float CurrentLifetime;
    bool bIsAnimating;
    
    // Timer for auto-removal
    FTimerHandle RemoveTimerHandle;
    
    // Helper functions
    void RemoveSelf();
};
