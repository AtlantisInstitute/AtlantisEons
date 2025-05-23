#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "BP_Item.h"
#include "WBP_StoreItemElement.generated.h"

UCLASS()
class ATLANTISEONS_API UWBP_StoreItemElement : public UUserWidget
{
    GENERATED_BODY()

public:
    // Variables
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Store Item")
    int32 ItemIndex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Store Item")
    FStructure_ItemInfo ItemInfo;

    // UI Elements
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UImage* ItemImage;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* Title;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* ItemSlotType;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* RecoveryHP;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* RecoveryMP;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* DamageText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* DefenceText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* HPText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* MPText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* STRText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* DEXText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* INTText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* DescriptionText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* BuyButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UImage* ScrollBackground;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UImage* BottomBackground;

    // Functions
    UFUNCTION(BlueprintCallable, Category = "Store Item")
    void OnBuyButtonClicked();

    // Getter functions for UI elements
    UFUNCTION(BlueprintCallable, Category = "UI")
    UImage* GetItemImage() const { return ItemImage; }

    UFUNCTION(BlueprintCallable, Category = "UI")
    UTextBlock* GetTitleText() const { return Title; }

    UFUNCTION(BlueprintCallable, Category = "UI")
    UTextBlock* GetDescriptionText() const { return DescriptionText; }

    UFUNCTION(BlueprintCallable, Category = "UI")
    UTextBlock* GetRecoveryHPText() const { return RecoveryHP; }

    UFUNCTION(BlueprintCallable, Category = "UI")
    UTextBlock* GetRecoveryMPText() const { return RecoveryMP; }

    UFUNCTION(BlueprintCallable, Category = "UI")
    UTextBlock* GetDamageText() const { return DamageText; }

    UFUNCTION(BlueprintCallable, Category = "UI")
    UTextBlock* GetDefenceText() const { return DefenceText; }

    UFUNCTION(BlueprintCallable, Category = "UI")
    const FStructure_ItemInfo& GetItemInfo() const { return ItemInfo; }

    // Getter functions for text blocks
    UFUNCTION(BlueprintCallable, Category = "UI")
    UTextBlock* GetHPText() const { return HPText; }

    UFUNCTION(BlueprintCallable, Category = "UI")
    UTextBlock* GetMPText() const { return MPText; }

    UFUNCTION(BlueprintCallable, Category = "UI")
    UTextBlock* GetSTRText() const { return STRText; }

    UFUNCTION(BlueprintCallable, Category = "UI")
    UTextBlock* GetDEXText() const { return DEXText; }

    UFUNCTION(BlueprintCallable, Category = "UI")
    UTextBlock* GetINTText() const { return INTText; }

    UFUNCTION(BlueprintCallable, Category = "UI")
    UTextBlock* GetItemSlotTypeText() const { return ItemSlotType; }
};
