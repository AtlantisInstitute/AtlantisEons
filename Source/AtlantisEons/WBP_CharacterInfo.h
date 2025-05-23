#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/CanvasPanel.h"
#include "WBP_InventorySlot.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Styling/SlateBrush.h"
#include "Math/Vector2D.h"
#include "Math/Color.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "UI/CharacterInfoWidget.h"
#include "WBP_CharacterInfo.generated.h"

class AAtlantisEonsHUD;
class AAtlantisEonsCharacter;

UCLASS()
class ATLANTISEONS_API UWBP_CharacterInfo : public UUserWidget
{
    GENERATED_BODY()

    friend class UWBP_Main;

    virtual UWorld* GetWorld() const override
    {
        if (GEngine)
        {
            if (const UWorld* World = GEngine->GetWorld())
            {
                return const_cast<UWorld*>(World);
            }
        }
        return nullptr;
    }

public:
    // Character reference - moved to public for HUD access
    UPROPERTY()
    AAtlantisEonsCharacter* Character;
    
    // Initialize character preview - moved to public for HUD access
    void InitializeCharacterPreview();

    // Character stat getters
    UFUNCTION(BlueprintPure, Category = "Character Info")
    FText GetText_CharacterHP();

    UFUNCTION(BlueprintPure, Category = "Character Info")
    FText GetText_CharacterMP();

    UFUNCTION(BlueprintPure, Category = "Character Info")
    FText GetText_CharacterDamage();

    UFUNCTION(BlueprintPure, Category = "Character Info")
    FText GetText_CharacterDefence();

    UFUNCTION(BlueprintPure, Category = "Character Info")
    FText GetText_CharacterSTR();

    UFUNCTION(BlueprintPure, Category = "Character Info")
    FText GetText_CharacterDEX();

    UFUNCTION(BlueprintPure, Category = "Character Info")
    FText GetText_CharacterINT();

    // Value text getters
    UFUNCTION(BlueprintPure, Category = "Character Info")
    FText GetText_HPPercentage();

    UFUNCTION(BlueprintPure, Category = "Character Info")
    FText GetText_MPPercentage();

    UFUNCTION(BlueprintPure, Category = "Character Info")
    FText GetText_DamageValue();

    UFUNCTION(BlueprintPure, Category = "Character Info")
    FText GetText_DefenceValue();

    UFUNCTION(BlueprintPure, Category = "Character Info")
    FText GetText_StrengthValue();

    UFUNCTION(BlueprintPure, Category = "Character Info")
    FText GetText_DexValue();

    UFUNCTION(BlueprintPure, Category = "Character Info")
    FText GetText_IntelligenceValue();

    // Character stat updaters
    UFUNCTION(BlueprintCallable, Category = "Character Info")
    void UpdateAllStats();

    UFUNCTION(BlueprintCallable, Category = "Character Info")
    void UpdateHPBar();

    UFUNCTION(BlueprintCallable, Category = "Character Info")
    void UpdateMPBar(float Percentage);

    /** Event dispatcher for when the resume button is clicked */
    UPROPERTY(BlueprintAssignable, Category = "Game")
    FOnResumeGameDelegate OnResumeGame;

    /** Function to handle resume button click */
    UFUNCTION(BlueprintCallable, Category = "Game")
    void HandleResumeClicked();

protected:
    virtual void NativeConstruct() override;

    // Native text binding functions
    UFUNCTION()
    FText GetHPText() const;

    UFUNCTION()
    FText GetMPText() const;

    UFUNCTION()
    FText GetDamageText() const;

    UFUNCTION()
    FText GetDefenceText() const;

    UFUNCTION()
    FText GetStrengthText() const;

    UFUNCTION()
    FText GetDexText() const;

    UFUNCTION()
    FText GetIntelligenceText() const;

    // Setup text bindings
    void SetupTextBindings();

    UFUNCTION()
    void OnQuitButtonClicked();

    UFUNCTION()
    void OnResumeButtonClicked();

    // Root Canvas Panel
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UCanvasPanel* CanvasPanel;

    // Character preview image
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UImage* CharacterPreviewImage;

    // Text Blocks
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* HP;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* MP;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* Damage;

    // Buttons
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* QuitButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* ResumeButton;

    // HUD reference
    UPROPERTY()
    AAtlantisEonsHUD* AtlantisEonsHUD;

    // Text blocks for stats
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* Label1;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* Label2;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* Defence;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* Strength;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* Dex;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* Intelligence;

    // Value text blocks
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* HPPercentage;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* MPPercentage;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* DamageValue;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* DefenceValue;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* StrengthValue;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* DexValue;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* IntelligenceValue;

    // Inventory slots
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UWBP_InventorySlot* WBP_InventorySlot_0;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UWBP_InventorySlot* WBP_InventorySlot_1;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UWBP_InventorySlot* WBP_InventorySlot_2;

    // Circular bars
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UImage* CircularBarHP;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UImage* CircularBarMP;

    // Timer handles for input recovery
    FTimerHandle PrimaryRecoveryTimer;
    FTimerHandle SecondaryRecoveryTimer;
    FTimerHandle FinalRecoveryTimer;
};
