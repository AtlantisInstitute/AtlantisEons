#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/SizeBox.h"
#include "WBP_StorePopup.generated.h"

UCLASS()
class ATLANTISEONS_API UWBP_StorePopup : public UUserWidget
{
    GENERATED_BODY()

public:
    // Variables
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Store")
    int32 ItemIndex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Store")
    int32 StackNumber;

    UFUNCTION(BlueprintPure, Category = "Store")
    FText GetText_StackNumber() const;

    UFUNCTION(BlueprintPure, Category = "Store")
    FText GetText_TotalPrice();

    // All widget bindings are made optional to avoid conflicts with Blueprint definitions
    
    // UI Elements - Main Structure
    // The names need to match exactly what's in the Blueprint, so we're renaming the C++ properties
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    USizeBox* SizeBox_0;  // Previously PopupBackground

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* TextBlock_0;  // Previously Text - "Are you sure.."

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UImage* Image_ItemThumbnail;  // Previously ItemThumbnail

    // Stack Counter Elements
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UOverlay* Overlay_StackCounter;  // Previously OverlayStackCounter

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UImage* Image_StackCounterBG;  // Previously StackCounterBackground

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* TextBlock_StackCounter;  // Previously StackCounterText - "000"

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UButton* Button_Up;  // Previously UpButton

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UButton* Button_Down;  // Previously DownButton

    // Gold Display
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UImage* Image_GoldCoin;  // Previously GoldCoin

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* TextBlock_GoldAmount;  // Previously GoldText - "99,999,999"
    
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* TotalPriceText;  // For showing the total price of the stack

    // Buttons
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UButton* CancelButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UButton* OKButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UImage* SideBorder;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UImage* Image_0;

    // Button Events
    UFUNCTION()
    void OnOKButtonClicked();

    UFUNCTION()
    void OnCancelButtonClicked();
    
    // Use OKButton as ConfirmButton for WBP_Main to reference
    UPROPERTY()
    UButton* ConfirmButton;

    UFUNCTION()
    void OnUpButtonClicked();

    UFUNCTION()
    void OnDownButtonClicked();

    UFUNCTION()
    void UpdateItemDisplay();
    
    // Update item details from the selected store item
    UFUNCTION(BlueprintCallable, Category = "Store")
    void UpdateItemDetails(int32 InItemIndex);

protected:
    virtual void NativeConstruct() override;
};
