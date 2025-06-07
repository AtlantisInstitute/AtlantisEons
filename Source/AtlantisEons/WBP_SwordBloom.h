#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "WBP_SwordBloom.generated.h"

/**
 * SwordBloom UI Widget for weapon effects
 */
UCLASS(BlueprintType, Blueprintable)
class ATLANTISEONS_API UWBP_SwordBloom : public UUserWidget
{
    GENERATED_BODY()

public:
    UWBP_SwordBloom(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;

protected:
    /** SwordSpark image component - bound to blueprint and hidden in C++ */
    UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "Sword Effects")
    class UImage* SwordSpark;

    /** SwordBloom image component - bound to blueprint and hidden in C++ */
    UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "Sword Effects")
    class UImage* SwordBloom;

    /** Bloom timing window properties */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Effects")
    bool bBloomWindowActive = false;
    
    /** Timer handle for the bloom window duration (1 second) */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Effects")
    FTimerHandle BloomWindowTimer;
    
    /** Timer handle for bloom circle scaling animation */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Effects")
    FTimerHandle BloomScaleTimer;
    
    /** Current scale of the bloom circle during animation */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Effects")
    float CurrentBloomScale = 1.0f;
    
    /** Animation progress for bloom scaling (0.0 to 1.0) */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Effects")
    float BloomScaleProgress = 0.0f;
    
    /** Critical timing window threshold (40% progress - original timing) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Effects")
    float CriticalTimingThreshold = 0.4f;
    
    /** Current bloom duration being used for logging */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Effects")
    float CurrentBloomDuration = 0.8f;

public:
    /** Get SwordSpark image component */
    UFUNCTION(BlueprintPure, Category = "Sword Effects")
    UImage* GetSwordSpark() const { return SwordSpark; }

    /** Get SwordBloom image component */
    UFUNCTION(BlueprintPure, Category = "Sword Effects")
    UImage* GetSwordBloom() const { return SwordBloom; }

    /** Show sword spark effect */
    UFUNCTION(BlueprintCallable, Category = "Sword Effects")
    void ShowSwordSpark();

    /** Hide sword spark effect */
    UFUNCTION(BlueprintCallable, Category = "Sword Effects")
    void HideSwordSpark();

    /** Show sword bloom effect */
    UFUNCTION(BlueprintCallable, Category = "Sword Effects")
    void ShowSwordBloom();

    /** Hide sword bloom effect */
    UFUNCTION(BlueprintCallable, Category = "Sword Effects")
    void HideSwordBloom();

    /** Show both effects */
    UFUNCTION(BlueprintCallable, Category = "Sword Effects")
    void ShowAllEffects();

    /** Hide both effects */
    UFUNCTION(BlueprintCallable, Category = "Sword Effects")
    void HideAllEffects();

    /** Set the scale of the sword bloom effect */
    UFUNCTION(BlueprintCallable, Category = "Sword Effects")
    void SetSwordBloomScale(float Scale);

    /** Start bloom circle with scaling animation and timing window */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void StartBloomCircle();

    /** Start bloom circle with custom duration (for programmatic triggers) */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    void StartBloomCircleWithDuration(float Duration);

    /** Try to trigger spark effect if in timing window */
    UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
    bool TryTriggerSpark();

    /** Update bloom scaling animation */
    UFUNCTION()
    void UpdateBloomScaling();

    /** Called when bloom window times out */
    UFUNCTION()
    void OnBloomWindowTimeout();

    /** Check if bloom window is currently active */
    UFUNCTION(BlueprintPure, Category = "Combat|Effects")
    bool IsBloomWindowActive() const { return bBloomWindowActive; }
}; 