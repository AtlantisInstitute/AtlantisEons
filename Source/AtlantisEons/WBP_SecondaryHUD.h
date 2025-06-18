#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Animation/WidgetAnimation.h"
#include "WBP_SecondaryHUD.generated.h"

class AAtlantisEonsCharacter;

// Forward declare delegates (defined in CharacterStatsComponent.h)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSecondaryHUDLevelUpSignature, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSecondaryHUDExperienceChangedSignature, int32, CurrentExp, int32, MaxExp);

/**
 * Secondary HUD widget that displays character health and experience progression
 * This parent class can be inherited by Blueprint to create WBP_SecondaryHUD
 * Features:
 * - Health Bar with current/max health display
 * - Experience Bar with level progression system
 * - Text displays for current values and earned experience
 * - Automatic binding to character stats and experience
 */
UCLASS(BlueprintType, Blueprintable)
class ATLANTISEONS_API UWBP_SecondaryHUD : public UUserWidget
{
    GENERATED_BODY()

public:
    UWBP_SecondaryHUD(const FObjectInitializer& ObjectInitializer);

    // ========== EVENTS ==========
    
    /** Called when player levels up */
    UPROPERTY(BlueprintAssignable, Category = "Experience")
    FOnSecondaryHUDLevelUpSignature OnPlayerLevelUp;
    
    /** Called when experience changes */
    UPROPERTY(BlueprintAssignable, Category = "Experience")
    FOnSecondaryHUDExperienceChangedSignature OnExperienceChanged;

    // ========== CORE FUNCTIONS ==========

    /** Initialize the HUD with character reference */
    UFUNCTION(BlueprintCallable, Category = "Secondary HUD")
    void InitializeHUD(AAtlantisEonsCharacter* InCharacter);

    /** Update all UI elements with current character data */
    UFUNCTION(BlueprintCallable, Category = "Secondary HUD")
    void UpdateAllElements();

    /** Update only the health bar */
    UFUNCTION(BlueprintCallable, Category = "Secondary HUD")
    void UpdateHealthBar();

    /** Update only the experience bar */
    UFUNCTION(BlueprintCallable, Category = "Secondary HUD")
    void UpdateExperienceBar();

    /** Update only the mana bar */
    UFUNCTION(BlueprintCallable, Category = "Secondary HUD")
    void UpdateManaBar();

    // ========== EXPERIENCE SYSTEM ==========

    /** Add experience points to the character */
    UFUNCTION(BlueprintCallable, Category = "Experience")
    void AddExperience(int32 ExpAmount);

    /** Get current player level */
    UFUNCTION(BlueprintPure, Category = "Experience")
    int32 GetPlayerLevel() const { return PlayerLevel; }

    /** Get current experience points */
    UFUNCTION(BlueprintPure, Category = "Experience")
    int32 GetCurrentExp() const { return CurrentExp; }

    /** Get experience needed for next level */
    UFUNCTION(BlueprintPure, Category = "Experience")
    int32 GetExpForNextLevel() const { return ExpForNextLevel; }

    /** Get total experience needed for current level */
    UFUNCTION(BlueprintPure, Category = "Experience")
    int32 GetExpForCurrentLevel() const;

    /** Calculate experience required for a specific level */
    UFUNCTION(BlueprintPure, Category = "Experience")
    int32 CalculateExpRequiredForLevel(int32 Level) const;

    /** Get experience percentage for current level (0.0 to 1.0) */
    UFUNCTION(BlueprintPure, Category = "Experience")
    float GetExpPercentage() const;

    // ========== TEXT BINDING FUNCTIONS ==========

    /** Get text for current health display */
    UFUNCTION(BlueprintPure, Category = "UI Text")
    FText GetCurrentHealthText() const;

    /** Get text for max health display */
    UFUNCTION(BlueprintPure, Category = "UI Text")
    FText GetMaxHealthText() const;

    /** Get text for current level display */
    UFUNCTION(BlueprintPure, Category = "UI Text")
    FText GetPlayerLevelText() const;

    /** Get text for current exp display */
    UFUNCTION(BlueprintPure, Category = "UI Text")
    FText GetCurrentExpText() const;

    /** Get text for max exp display */
    UFUNCTION(BlueprintPure, Category = "UI Text")
    FText GetMaxExpText() const;

    /** Get text for earned experience display */
    UFUNCTION(BlueprintPure, Category = "UI Text")
    FText GetEarnedExpText() const;

    // ========== PROGRAMMATIC ANIMATION SYSTEM ==========
    
    /** Timer handle for ExpEarned animation */
    UPROPERTY()
    FTimerHandle ExpEarnedAnimationTimer;
    
    /** Timer handle for LevelUp animation */
    UPROPERTY()
    FTimerHandle LevelUpAnimationTimer;
    
    /** Animation progress for ExpEarned (0.0 to 1.0) */
    UPROPERTY()
    float ExpEarnedAnimationProgress;
    
    /** Animation progress for LevelUp (0.0 to 1.0) */
    UPROPERTY()
    float LevelUpAnimationProgress;
    
    /** Animation duration for ExpEarned in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings")
    float ExpEarnedAnimationDuration = 0.75f;
    
    /** Animation duration for LevelUp in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings")
    float LevelUpAnimationDuration = 1.5f;
    
    /** Animation update frequency (updates per second) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings")
    float AnimationUpdateRate = 60.0f;

    // ========== PROGRAMMATIC ANIMATION FUNCTIONS ==========
    
    /** Update ExpEarned animation progress */
    UFUNCTION()
    void UpdateExpEarnedAnimation();
    
    /** Update LevelUp animation progress */
    UFUNCTION()
    void UpdateLevelUpAnimation();
    
    /** Apply animation properties to ExpEarnedText widget */
    UFUNCTION()
    void ApplyExpEarnedAnimationFrame(float Progress);
    
    /** Apply animation properties to LevelUpTxt widget */
    UFUNCTION()
    void ApplyLevelUpAnimationFrame(float Progress);
    
    /** Reset ExpEarnedText to default state */
    UFUNCTION()
    void ResetExpEarnedTextState();
    
    /** Reset LevelUpTxt to default state */
    UFUNCTION()
    void ResetLevelUpTextState();

    /** Play the experience earned animation (programmatic) */
    UFUNCTION(BlueprintCallable, Category = "Animation")
    void PlayExpEarnedAnimation();
    
    /** Play the level up animation (programmatic) */
    UFUNCTION(BlueprintCallable, Category = "Animation")
    void PlayLevelUpAnimation();

    /** Force refresh SecondaryHUD visibility and appearance (fixes UI conflicts) */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ForceRefreshSecondaryHUD();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // ========== WIDGET COMPONENTS (BIND IN BLUEPRINT) ==========

    /** Main canvas panel container */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UCanvasPanel* CanvasPanel;

    /** Health progress bar widget */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UProgressBar* HealthBar;

    /** Mana bar widget */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UProgressBar* ManaBar;

    /** Experience progress bar widget */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UProgressBar* ExpBar;

    /** Text block showing "Level 1" */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* PlayerLevelText;

    /** Text block showing current exp earned */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* ExpEarnedText;

    /** Text block showing current level text */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* LevelUpTxt;

    /** Text block for weapon label */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* WeaponText;

    /** Text block for dodge label */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* DodgeText;

    /** Text block for shield label */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* ShieldText;

    // ========== EXPERIENCE SYSTEM PROPERTIES ==========

    /** Current player level */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Experience")
    int32 PlayerLevel = 1;

    /** Current experience points */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Experience")
    int32 CurrentExp = 0;

    /** Experience needed for next level */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Experience")
    int32 ExpForNextLevel = 100;

    /** Base experience for level 1 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Experience")
    int32 BaseExpRequirement = 100;

    /** Experience multiplier per level (exponential growth) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Experience")
    float ExpGrowthMultiplier = 1.5f;

    /** Recently earned experience (for display purposes) */
    UPROPERTY(BlueprintReadOnly, Category = "Experience")
    int32 RecentlyEarnedExp = 0;

    /** Maximum level cap */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Experience")
    int32 MaxLevel = 100;

    // ========== HEALTH SYSTEM PROPERTIES ==========

    /** Health bar color when healthy */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar|Appearance")
    FLinearColor HealthyColor = FLinearColor::Green;

    /** Health bar color when damaged */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar|Appearance")
    FLinearColor DamagedColor = FLinearColor::Yellow;

    /** Health bar color when critically low */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar|Appearance")
    FLinearColor CriticalColor = FLinearColor::Red;

    /** Mana bar color */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mana Bar|Appearance")
    FLinearColor ManaBarColor = FLinearColor(0.5f, 0.8f, 1.0f, 1.0f); // Light blue

    /** Experience bar color */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Experience Bar|Appearance")
    FLinearColor ExpBarColor = FLinearColor::Blue;

private:
    // ========== CACHED REFERENCES ==========

    /** Reference to the character */
    UPROPERTY()
    AAtlantisEonsCharacter* Character;

    // ========== HELPER FUNCTIONS ==========

    /** Update health bar color based on health percentage */
    void UpdateHealthBarColor();

    /** Get appropriate health bar color based on current health */
    FLinearColor GetHealthBarColor() const;

    /** Handle level up logic */
    void HandleLevelUp();

    /** Bind to character events */
    void BindToCharacterEvents();

    /** Unbind from character events */
    void UnbindFromCharacterEvents();

    /** Event handler for character health changes */
    UFUNCTION()
    void OnCharacterHealthChanged(float NewHealthPercent);

    /** Event handler for character experience changes */
    UFUNCTION()
    void OnCharacterExperienceChanged(int32 NewCurrentExp, int32 NewMaxExp);

    /** Event handler for character level up */
    UFUNCTION()
    void OnCharacterLevelUp(int32 NewLevel);

    /** Setup text binding for all text elements */
    void SetupTextBindings();
}; 