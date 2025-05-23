#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "BP_Item.h"
#include "WBP_ItemDescription.generated.h"

UCLASS()
class ATLANTISEONS_API UWBP_ItemDescription : public UUserWidget
{
    GENERATED_BODY()

public:
    UWBP_ItemDescription(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UImage* ItemThumbnail;

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

    UFUNCTION(BlueprintPure, Category = "Item Description")
    FText GetDamageText() const { return DamageText ? DamageText->GetText() : FText::GetEmpty(); }

    UFUNCTION(BlueprintPure, Category = "Item Description")
    FText GetDefenceText() const { return DefenceText ? DefenceText->GetText() : FText::GetEmpty(); }

    UFUNCTION(BlueprintPure, Category = "Item Description")
    FText GetHPText() const { return HPText ? HPText->GetText() : FText::GetEmpty(); }

    UFUNCTION(BlueprintPure, Category = "Item Description")
    FText GetMPText() const { return MPText ? MPText->GetText() : FText::GetEmpty(); }

    UFUNCTION(BlueprintPure, Category = "Item Description")
    FText GetSTRText() const { return STRText ? STRText->GetText() : FText::GetEmpty(); }

    UFUNCTION(BlueprintPure, Category = "Item Description")
    FText GetDEXText() const { return DEXText ? DEXText->GetText() : FText::GetEmpty(); }

    UFUNCTION(BlueprintPure, Category = "Item Description")
    FText GetINTText() const { return INTText ? INTText->GetText() : FText::GetEmpty(); }

    UFUNCTION(BlueprintCallable, Category = "Item Description")
    void UpdateDescription(const FStructure_ItemInfo& ItemInfo);

protected:
    FString GetItemTypeString(EItemType ItemType) const;
    FString GetItemStatsString(const FStructure_ItemInfo& ItemInfo) const;
};
